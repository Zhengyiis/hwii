open Util
open S_exp
open Shared
open Error
open Directive

type symtab = int Symtab.symtab

(** Constants used for tagging values at runtime. *)

let num_shift = 2
let num_mask = 0b11
let num_tag = 0b00
let bool_shift = 7
let bool_mask = 0b1111111
let bool_tag = 0b0011111
let heap_mask = 0b111
let pair_tag = 0b010
let string_mask = 0b111
let string_tag = 0b011
let string_shift = 3
(** Constants used for tagging values at runtime. *)
let inchannel_shift = 9
let inchannel_mask = 0b111111111
let inchannel_tag = 0b011111111

let outchannel_shift = 9
let outchannel_mask = 0b111111111
let outchannel_tag = 0b001111111

let stdin_fd = 0  (* Standard file descriptor for stdin *)
let stdout_fd = 1 (* Standard file descriptor for stdout *)

(** [operand_of_num x] returns the runtime representation of the number [x] as
    an operand for instructions *)
let operand_of_num : int -> operand =
 fun x -> Imm ((x lsl num_shift) lor num_tag)

(** [operand_of_bool b] returns the runtime representation of the boolean [b] as
    an operand for instructions *)
let operand_of_bool : bool -> operand =
 fun b -> Imm (((if b then 1 else 0) lsl bool_shift) lor bool_tag)
(*
   let operand_of_string : string -> operand =
    fun s -> Imm ((s lsl s_shift) lor num_tag) *)

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

let stack_address : int -> operand = fun index -> MemOffset (Imm index, Reg Rsp)

let ensure : directive list -> string -> directive list =
 fun condition err_msg ->
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

let ensure_type : int -> int -> operand -> s_exp -> directive list =
 fun mask tag op e ->
  ensure
    [ Mov (Reg R8, op); And (Reg R8, Imm mask); Cmp (Reg R8, Imm tag) ]
    (string_of_s_exp e)

let ensure_num : operand -> s_exp -> directive list =
  ensure_type num_mask num_tag

let ensure_pair : operand -> s_exp -> directive list =
  ensure_type heap_mask pair_tag

(* Add string support functions *)
let ensure_string : operand -> s_exp -> directive list =
  ensure_type string_mask string_tag

  let ensure_inchannel : operand -> s_exp -> directive list =
    ensure_type inchannel_mask inchannel_tag
  
  let ensure_outchannel : operand -> s_exp -> directive list =
    ensure_type outchannel_mask outchannel_tag

let align_stack_index : int -> int =
 fun stack_index ->
  if stack_index mod 16 = -8 then stack_index else stack_index - 8

(** [compile_0ary_primitive e prim] produces x86-64 instructions for the
    zero-ary primitive operation named by [prim]; if [prim] isn't a valid
    zero-ary operation, it raises an exception using the s-expression [e]. *)
let compile_0ary_primitive : int -> s_exp -> string -> directive list =
 fun stack_index e prim ->
  match prim with
  | "read-num" ->
      [
        Mov (stack_address stack_index, Reg Rdi);
        Add (Reg Rsp, Imm (align_stack_index stack_index));
        Call "read_num";
        Sub (Reg Rsp, Imm (align_stack_index stack_index));
        Mov (Reg Rdi, stack_address stack_index);
      ]
  | "newline" ->
      [
        Mov (stack_address stack_index, Reg Rdi);
        Add (Reg Rsp, Imm (align_stack_index stack_index));
        Call "print_newline";
        Sub (Reg Rsp, Imm (align_stack_index stack_index));
        Mov (Reg Rdi, stack_address stack_index);
        Mov (Reg Rax, operand_of_bool true);
      ]
  | _ -> raise (Stuck e)

(** [compile_unary_primitive e prim] produces x86-64 instructions for the unary
    primitive operation named by [prim]; if [prim] isn't a valid unary
    operation, it raises an exception using the s-expression [e]. *)
let compile_unary_primitive : int -> s_exp -> string -> directive list =
 fun stack_index e prim ->
  match prim with
  | "add1" -> ensure_num (Reg Rax) e @ [ Add (Reg Rax, operand_of_num 1) ]
  | "sub1" -> ensure_num (Reg Rax) e @ [ Sub (Reg Rax, operand_of_num 1) ]
  | "zero?" -> [ Cmp (Reg Rax, operand_of_num 0) ] @ zf_to_bool
  | "num?" ->
      [ And (Reg Rax, Imm num_mask); Cmp (Reg Rax, Imm num_tag) ] @ zf_to_bool
  | "not" -> [ Cmp (Reg Rax, operand_of_bool false) ] @ zf_to_bool
  | "pair?" ->
      [ And (Reg Rax, Imm heap_mask); Cmp (Reg Rax, Imm pair_tag) ] @ zf_to_bool
  | "left" ->
      ensure_pair (Reg Rax) e
      @ [ Mov (Reg Rax, MemOffset (Reg Rax, Imm (-pair_tag))) ]
  | "right" ->
      ensure_pair (Reg Rax) e
      @ [ Mov (Reg Rax, MemOffset (Reg Rax, Imm (-pair_tag + 8))) ]
  | "print" ->
      [
        Mov (stack_address stack_index, Reg Rdi);
        Mov (Reg Rdi, Reg Rax);
        Add (Reg Rsp, Imm (align_stack_index stack_index));
        Call "print_value";
        Sub (Reg Rsp, Imm (align_stack_index stack_index));
        Mov (Reg Rdi, stack_address stack_index);
        Mov (Reg Rax, operand_of_bool true);
      ]
      | "open-in" ->
        ensure_string (Reg Rax) e
        @ [ 
            Mov (Reg Rdi, Reg Rax);         (* Move string pointer to first argument *)
            Sub (Reg Rdi, Imm string_tag);  (* Remove string tag *)
            Add (Reg Rsp, Imm (align_stack_index stack_index));
            Call "open_in_channel";         (* Call runtime function *)
            Sub (Reg Rsp, Imm (align_stack_index stack_index));
          ]
        
    | "open-out" ->
        ensure_string (Reg Rax) e
        @ [ 
            Mov (Reg Rdi, Reg Rax);         (* Move string pointer to first argument *)
            Sub (Reg Rdi, Imm string_tag);  (* Remove string tag *)
            Add (Reg Rsp, Imm (align_stack_index stack_index));
            Call "open_out_channel";        (* Call runtime function *)
            Sub (Reg Rsp, Imm (align_stack_index stack_index));
          ]
        
    | "close-in" ->
        ensure_inchannel (Reg Rax) e
        @ [ 
            Mov (Reg Rdi, Reg Rax);         (* Move channel to first argument *)
            Add (Reg Rsp, Imm (align_stack_index stack_index));
            Call "close_in_channel";        (* Call runtime function *)
            Sub (Reg Rsp, Imm (align_stack_index stack_index));
            Shl (Reg Rax, Imm bool_shift);  (* Convert result to boolean *)
            Or (Reg Rax, Imm bool_tag);
          ]
        
    | "close-out" ->
        ensure_outchannel (Reg Rax) e
        @ [ 
            Mov (Reg Rdi, Reg Rax);         (* Move channel to first argument *)
            Add (Reg Rsp, Imm (align_stack_index stack_index));
            Call "close_out_channel";       (* Call runtime function *)
            Sub (Reg Rsp, Imm (align_stack_index stack_index));
            Shl (Reg Rax, Imm bool_shift);  (* Convert result to boolean *)
            Or (Reg Rax, Imm bool_tag);
          ]
  | _ -> raise (Stuck e)

(** [compile_binary_primitive stack_index e prim] produces x86-64 instructions
    for the binary primitive operation named by [prim] given a stack index of
    [stack_index]; if [prim] isn't a valid binary operation, it raises an error
    using the s-expression [e]. *)
let compile_binary_primitive : int -> s_exp -> string -> directive list =
 fun stack_index e prim ->
  match prim with
  | "+" ->
      ensure_num (Reg Rax) e
      @ ensure_num (stack_address stack_index) e
      @ [ Add (Reg Rax, stack_address stack_index) ]
  | "-" ->
      ensure_num (Reg Rax) e
      @ ensure_num (stack_address stack_index) e
      @ [
          Mov (Reg R8, Reg Rax);
          Mov (Reg Rax, stack_address stack_index);
          Sub (Reg Rax, Reg R8);
        ]
  | "=" ->
      ensure_num (Reg Rax) e
      @ ensure_num (stack_address stack_index) e
      @ [ Cmp (stack_address stack_index, Reg Rax) ]
      @ zf_to_bool
  | "<" ->
      ensure_num (Reg Rax) e
      @ ensure_num (stack_address stack_index) e
      @ [ Cmp (stack_address stack_index, Reg Rax) ]
      @ setl_bool
  | "pair" ->
      [
        Mov (Reg R8, stack_address stack_index);
        Mov (MemOffset (Reg Rdi, Imm 0), Reg R8);
        Mov (MemOffset (Reg Rdi, Imm 8), Reg Rax);
        Mov (Reg Rax, Reg Rdi);
        Or (Reg Rax, Imm pair_tag);
        Add (Reg Rdi, Imm 16);
      ]
      | "input" ->
        ensure_inchannel (stack_address stack_index) e
        @ ensure_num (Reg Rax) e
        @ [ 
            (* Save heap pointer *)
            Mov (stack_address (stack_index - 8), Reg Rdi);
            
            (* Set up arguments in correct order *)
            Mov (Reg Rsi, Reg Rax);                  (* length argument *)
            Mov (Reg Rdx, Reg Rdi);                  (* heap pointer - third arg *)
            Mov (Reg Rdi, stack_address stack_index); (* channel - first arg *)
            
            (* Call input_channel *)
            Add (Reg Rsp, Imm (align_stack_index stack_index));
            Call "input_channel";
            Sub (Reg Rsp, Imm (align_stack_index stack_index));
            
            (* Restore heap pointer *)
            Mov (Reg Rdi, stack_address (stack_index - 8));
          ]
    
    | "output" ->
        ensure_outchannel (stack_address stack_index) e
        @ ensure_string (Reg Rax) e
        @ [
            Mov (stack_address (stack_index - 8), Reg Rdi);
            Mov (Reg Rsi, Reg Rax);          (* Move string to second argument *)
            Mov (Reg Rdi, stack_address stack_index); (* Move channel to first argument *)
            Add (Reg Rsp, Imm (align_stack_index stack_index));
            Call "output_channel";           (* Call runtime function *)
            Sub (Reg Rsp, Imm (align_stack_index stack_index));
            Mov (Reg Rdi, stack_address (stack_index - 8));
          ]
  | _ -> raise (Stuck e)

let align : int -> int -> int =
 fun n alignment ->
  if n mod alignment = 0 then n else n + (alignment - (n mod alignment))

(** [compile_expr tab stack_index e] produces x86-64 instructions for the
    expression [e] given a symtab of [tab] and stack index of [stack_index]. *)
let rec compile_expr : symtab -> int -> s_exp -> directive list =
 fun tab stack_index e ->
  match e with
  | Num x -> [ Mov (Reg Rax, operand_of_num x) ]
  | Sym "true" -> [ Mov (Reg Rax, operand_of_bool true) ]
  | Sym "false" -> [ Mov (Reg Rax, operand_of_bool false) ]
  | Sym "stdin" -> 
    [  
      Mov (Reg Rax, Imm stdin_fd);
      Shl (Reg Rax, Imm inchannel_shift);
      Or (Reg Rax, Imm inchannel_tag);
    ]
| Sym "stdout" -> 
    [  
      Mov (Reg Rax, Imm stdout_fd);
      Shl (Reg Rax, Imm outchannel_shift);
      Or (Reg Rax, Imm outchannel_tag);
    ]
  | Sym var when Symtab.mem var tab ->
      [ Mov (Reg Rax, stack_address (Symtab.find var tab)) ]
  | Str s ->
      let string_label = gensym "str" in
      [
        Align 8;
        (* Define string in data section *)
        Section "data";
        Label string_label;
        DqString s;
        (* Switch back to text section *)
        Section "text";
        (* Load string address and tag it *)
        LeaLabel (Reg Rax, string_label);
        Or (Reg Rax, Imm string_tag);
      ]
  | Lst [ Sym "if"; test_expr; then_expr; else_expr ] ->
      let then_label = gensym "then" in
      let else_label = gensym "else" in
      let continue_label = gensym "continue" in
      compile_expr tab stack_index test_expr
      @ [ Cmp (Reg Rax, operand_of_bool false) ]
      @ [ Je else_label ] @ [ Label then_label ]
      @ compile_expr tab stack_index then_expr
      @ [ Jmp continue_label ] @ [ Label else_label ]
      @ compile_expr tab stack_index else_expr
      @ [ Label continue_label ]
  | Lst [ Sym "let"; Lst [ Lst [ Sym var; exp ] ]; body ] ->
      compile_expr tab stack_index exp
      @ [ Mov (stack_address stack_index, Reg Rax) ]
      @ compile_expr (Symtab.add var stack_index tab) (stack_index - 8) body
  | Lst (Sym "do" :: exps) when List.length exps > 0 ->
      List.concat_map (compile_expr tab stack_index) exps
  | Lst [ Sym f ] as exp -> compile_0ary_primitive stack_index exp f
  | Lst [ Sym f; arg ] as exp ->
      compile_expr tab stack_index arg
      @ compile_unary_primitive stack_index exp f
  | Lst [ Sym f; arg1; arg2 ] as exp ->
      compile_expr tab stack_index arg1
      @ [ Mov (stack_address stack_index, Reg Rax) ]
      @ compile_expr tab (stack_index - 8) arg2
      @ compile_binary_primitive stack_index exp f
  | e -> raise (Stuck e)

(** [compile e] produces x86-64 instructions, including frontmatter, for the
    expression [e] *)
let compile e =
  [
    Global "lisp_entry";
    Extern "lisp_error";
    Extern "read_num";
    Extern "print_value";
    Extern "print_newline";
    Extern "open_in_channel";
    Extern "open_out_channel";  
  Extern "close_in_channel";
  Extern "close_out_channel";
    Extern "input_channel";
    Extern "output_channel";
    Section "data";
    Section "text";
    Label "lisp_entry";
  ]
  @ compile_expr Symtab.empty (-8) e
  @ [ Ret ]
