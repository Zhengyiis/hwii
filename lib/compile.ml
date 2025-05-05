open Util
open Shared
open Directive
open Ast
open Typecheck

(** constants used for tagging values at runtime *)
let num_shift = 2

let num_mask = 0b11
let num_tag = 0b00
let bool_shift = 7
let bool_mask = 0b1111111
let bool_tag = 0b0011111
let heap_mask = 0b111
let pair_tag = 0b010
let nil_tag = 0b11111111
let fn_tag = 0b110

type symtab = int Symtab.symtab

let function_label s =
  let nasm_char c =
    match c with
    | 'a' .. 'z'
    | 'A' .. 'Z'
    | '0' .. '9'
    | '_' | '$' | '#' | '@' | '~' | '.' | '?' ->
        c
    | _ -> '_'
  in
  Printf.sprintf "function_%s_%d" (String.map nasm_char s) (Hashtbl.hash s)

(** [operand_of_num x] returns the runtime representation of the number [x] as
    an operand for instructions *)
let operand_of_num (x : int) : operand = Imm ((x lsl num_shift) lor num_tag)

(** [operand_of_bool b] returns the runtime representation of the boolean [b] as
    an operand for instructions *)
let operand_of_bool (b : bool) : operand =
  Imm (((if b then 1 else 0) lsl bool_shift) lor bool_tag)

let operand_of_nil = Imm nil_tag

let zf_to_bool =
  [
    Mov (Reg Rax, Imm 0);
    Setz (Reg Rax);
    Shl (Reg Rax, Imm bool_shift);
    Or (Reg Rax, Imm bool_tag);
  ]

let setl_bool =
  [
    Mov (Reg Rax, Imm 0);
    Setl (Reg Rax);
    Shl (Reg Rax, Imm bool_shift);
    Or (Reg Rax, Imm bool_tag);
  ]

let stack_address (index : int) : operand = MemOffset (Imm index, Reg Rsp)

let ensure condition err_msg =
  let msg_label = gensym "emsg" in
  let continue_label = gensym "continue" in
  condition
  @ [
      Je continue_label;
      LeaLabel (Reg Rdi, msg_label);
      Jmp "lisp_error";
      Label msg_label;
      DqString err_msg;
      Label continue_label;
    ]

let ensure_type mask tag op e =
  ensure
    [ Mov (Reg R8, op); And (Reg R8, Imm mask); Cmp (Reg R8, Imm tag) ]
    (string_of_expr e)

let ensure_num = ensure_type num_mask num_tag
let ensure_pair = ensure_type heap_mask pair_tag
let ensure_fn = ensure_type heap_mask fn_tag

let align_stack_index (stack_index : int) : int =
  if stack_index mod 16 = -8 then stack_index else stack_index - 8

(** [compile_0ary_primitive e prim] produces X86-64 instructions for the
    zero-arity primitive operation [prim]; if [prim] isn't a valid zero-arity
    operation, it raises an error using the expression [e] *)
let compile_0ary_primitive stack_index _e = function
  | ReadNum ->
      [
        Mov (stack_address stack_index, Reg Rdi);
        Add (Reg Rsp, Imm (align_stack_index stack_index));
        Call "read_num";
        Sub (Reg Rsp, Imm (align_stack_index stack_index));
        Mov (Reg Rdi, stack_address stack_index);
      ]
  | Newline ->
      [
        Mov (stack_address stack_index, Reg Rdi);
        Add (Reg Rsp, Imm (align_stack_index stack_index));
        Call "print_newline";
        Sub (Reg Rsp, Imm (align_stack_index stack_index));
        Mov (Reg Rdi, stack_address stack_index);
        Mov (Reg Rax, operand_of_bool true);
      ]

(** [compile_unary_primitive e prim] produces X86-64 instructions for the unary
    primitive operation [prim]; if [prim] isn't a valid unary operation, it
    raises an error using the expression [e] *)
let compile_unary_primitive stack_index e = function
  | Add1 -> ensure_num (Reg Rax) e @ [ Add (Reg Rax, operand_of_num 1) ]
  | Sub1 -> ensure_num (Reg Rax) e @ [ Sub (Reg Rax, operand_of_num 1) ]
  | IsZero -> [ Cmp (Reg Rax, operand_of_num 0) ] @ zf_to_bool
  | IsNum ->
      [ And (Reg Rax, Imm num_mask); Cmp (Reg Rax, Imm num_tag) ] @ zf_to_bool
  | Not -> [ Cmp (Reg Rax, operand_of_bool false) ] @ zf_to_bool
  | IsPair ->
      [ And (Reg Rax, Imm heap_mask); Cmp (Reg Rax, Imm pair_tag) ] @ zf_to_bool
  | Left ->
      ensure_pair (Reg Rax) e
      @ [ Mov (Reg Rax, MemOffset (Reg Rax, Imm (-pair_tag))) ]
  | Right ->
      ensure_pair (Reg Rax) e
      @ [ Mov (Reg Rax, MemOffset (Reg Rax, Imm (-pair_tag + 8))) ]
  | IsEmpty -> [ Cmp (Reg Rax, operand_of_nil) ] @ zf_to_bool
  | Print ->
      [
        Mov (stack_address stack_index, Reg Rdi);
        Mov (Reg Rdi, Reg Rax);
        Add (Reg Rsp, Imm (align_stack_index stack_index));
        Call "print_value";
        Sub (Reg Rsp, Imm (align_stack_index stack_index));
        Mov (Reg Rdi, stack_address stack_index);
        Mov (Reg Rax, operand_of_bool true);
      ]

(** [compile_binary_primitive stack_index e prim] produces X86-64 instructions
    for the binary primitive operation [prim]; if [prim] isn't a valid binary
    operation, it raises an error using the expression [e] *)
let compile_binary_primitive stack_index e = function
  | Plus ->
      ensure_num (Reg Rax) e
      @ ensure_num (stack_address stack_index) e
      @ [ Add (Reg Rax, stack_address stack_index) ]
  | Minus ->
      ensure_num (Reg Rax) e
      @ ensure_num (stack_address stack_index) e
      @ [
          Mov (Reg R8, Reg Rax);
          Mov (Reg Rax, stack_address stack_index);
          Sub (Reg Rax, Reg R8);
        ]
  | Eq ->
      ensure_num (Reg Rax) e
      @ ensure_num (stack_address stack_index) e
      @ [ Cmp (stack_address stack_index, Reg Rax) ]
      @ zf_to_bool
  | Lt ->
      ensure_num (Reg Rax) e
      @ ensure_num (stack_address stack_index) e
      @ [ Cmp (stack_address stack_index, Reg Rax) ]
      @ setl_bool
  | Pair ->
      [
        Mov (Reg R8, stack_address stack_index);
        Mov (MemOffset (Reg Rdi, Imm 0), Reg R8);
        Mov (MemOffset (Reg Rdi, Imm 8), Reg Rax);
        Mov (Reg Rax, Reg Rdi);
        Or (Reg Rax, Imm pair_tag);
        Add (Reg Rdi, Imm 16);
      ]

let align n alignment =
  if n mod alignment = 0 then n else n + (alignment - (n mod alignment))

let rec fv (bound : string list) (exp : expr) : string list =
  match exp with
  | Var s when not (List.mem s bound) -> [ s ]
  | Prim1 (_p, e) -> fv bound e
  | Prim2 (_p, e1, e2) -> fv bound e1 @ fv bound e2
  | If (a, b, c) -> fv bound a @ fv bound b @ fv bound c
  | Call (a, b) -> fv bound a @ List.concat_map (fv bound) b
  | Let (v, e, body) -> fv bound e @ fv (v :: bound) body
  | Do es -> List.concat_map (fv bound) es
  | _ -> []

(** [compile_expr e] produces X86-64 instructions for the expression [e] *)
let rec compile_expr (defns : defn list) (tab : symtab) (stack_index : int) :
    expr -> directive list = function
  | Num x -> [ Mov (Reg Rax, operand_of_num x) ]
  | True -> [ Mov (Reg Rax, operand_of_bool true) ]
  | False -> [ Mov (Reg Rax, operand_of_bool false) ]
  | Var var when Symtab.mem var tab ->
      [ Mov (Reg Rax, stack_address (Symtab.find var tab)) ]
  | Var var when is_defn defns var ->
      [
        LeaLabel (Reg Rax, function_label var);
        Mov (MemOffset (Reg Rdi, Imm 0), Reg Rax);
        Mov (Reg Rax, Reg Rdi);
        Or (Reg Rax, Imm fn_tag);
        Add (Reg Rdi, Imm 8);
      ]
  | Var _ as e -> raise (Error.Stuck (s_exp_of_expr e))
  | Closure f ->
      let defn = get_defn defns f in
      let bound = List.map (fun d -> d.name) defns @ defn.args in
      let fvs = fv bound defn.body in
      let fv_movs =
        List.mapi
          (fun i var ->
            [
              Mov (Reg Rax, stack_address (Symtab.find var tab));
              Mov (MemOffset (Reg Rdi, Imm (8 * (i + 1))), Reg Rax);
            ])
          fvs
      in
      [
        LeaLabel (Reg Rax, function_label f);
        Mov (MemOffset (Reg Rdi, Imm 0), Reg Rax);
      ]
      @ List.concat fv_movs
      @ [
          Mov (Reg Rax, Reg Rdi);
          Or (Reg Rax, Imm fn_tag);
          Add (Reg Rdi, Imm (8 * (List.length fvs + 1)));
        ]
  | Nil -> [ Mov (Reg Rax, operand_of_nil) ]
  | If (test_expr, then_expr, else_expr) ->
      let then_label = gensym "then" in
      let else_label = gensym "else" in
      let continue_label = gensym "continue" in
      compile_expr defns tab stack_index test_expr
      @ [ Cmp (Reg Rax, operand_of_bool false); Je else_label ]
      @ [ Label then_label ]
      @ compile_expr defns tab stack_index then_expr
      @ [ Jmp continue_label ] @ [ Label else_label ]
      @ compile_expr defns tab stack_index else_expr
      @ [ Label continue_label ]
  | Let (var, exp, body) ->
      compile_expr defns tab stack_index exp
      @ [ Mov (stack_address stack_index, Reg Rax) ]
      @ compile_expr defns
          (Symtab.add var stack_index tab)
          (stack_index - 8) body
  | Do exps -> List.concat_map (compile_expr defns tab stack_index) exps
  | Call (f, args) as e ->
      let stack_base = align_stack_index (stack_index + 8) in
      let stack_after_args = stack_base - ((List.length args + 2) * 8) in
      let compiled_f = compile_expr defns tab stack_after_args f in
      (* NOTE: we aren't checking function argument lengths anymore!
         this is bad, but you know how to do dynamic arity checking now :) *)
      let compiled_args =
        args
        |> List.mapi (fun i arg ->
               compile_expr defns tab (stack_base - ((i + 2) * 8)) arg
               @ [ Mov (stack_address (stack_base - ((i + 2) * 8)), Reg Rax) ])
        |> List.concat
      in
      compiled_args @ compiled_f @ ensure_fn (Reg Rax) e
      @ [
          Mov
            (stack_address (stack_base - (8 * (List.length args + 2))), Reg Rax);
          Sub (Reg Rax, Imm fn_tag);
          Mov (Reg Rax, MemOffset (Reg Rax, Imm 0));
        ]
      @ [
          Add (Reg Rsp, Imm stack_base);
          ComputedCall (Reg Rax);
          Sub (Reg Rsp, Imm stack_base);
        ]
  | Prim0 f as exp -> compile_0ary_primitive stack_index exp f
  | Prim1 (f, arg) as exp ->
      compile_expr defns tab stack_index arg
      @ compile_unary_primitive stack_index exp f
  | Prim2 (f, arg1, arg2) as exp ->
      compile_expr defns tab stack_index arg1
      @ [ Mov (stack_address stack_index, Reg Rax) ]
      @ compile_expr defns tab (stack_index - 8) arg2
      @ compile_binary_primitive stack_index exp f
  | Lambda _ -> failwith "shouldn't be here"

(** [compile_defn defns defn] produces X86-64 instructions for the function
    definition [defn] **)
let compile_defn (defns : defn list) defn : directive list =
  let bound = List.map (fun d -> d.name) defns @ defn.args in
  let fvs = fv bound defn.body in
  let ftab =
    defn.args @ fvs
    |> List.mapi (fun i arg -> (arg, (i + 1) * -8))
    |> Symtab.of_list
  in
  let fv_movs =
    [
      Mov (Reg Rax, stack_address (-8 * (List.length defn.args + 1)));
      Sub (Reg Rax, Imm fn_tag);
      Add (Reg Rax, Imm 8);
    ]
    @ List.concat
        (List.mapi
           (fun i _ ->
             [
               Mov (Reg R8, MemOffset (Reg Rax, Imm (i * 8)));
               Mov (stack_address (-8 * (List.length defn.args + 1 + i)), Reg R8);
             ])
           fvs)
  in
  [ Align 8; Label (function_label defn.name) ]
  @ fv_movs
  @ compile_expr defns ftab ((Symtab.cardinal ftab + 1) * -8) defn.body
  @ [ Ret ]

(** [compile] produces X86-64 instructions, including frontmatter, for the
    expression [e] *)
let compile (prog : program) =
  typecheck prog;
  let prog = desugar_program prog in
  let prog = Constantfold.fold_program prog in
  [
    Global "lisp_entry";
    Extern "lisp_error";
    Extern "read_num";
    Extern "print_value";
    Extern "print_newline";
    Section "text";
  ]
  @ List.concat_map (compile_defn prog.defns) prog.defns
  @ [ Label "lisp_entry" ]
  @ compile_expr prog.defns Symtab.empty (-8) prog.body
  @ [ Ret ]
