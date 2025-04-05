open Printf
open Util
open S_exp
open Shared
open Error

(** A [value]  is the runtime value of an expression. *)
type value = Num of int | Bool of bool | Pair of (value * value) | Nil

type environment = value Symtab.symtab

let input_channel : in_channel ref = ref stdin
let output_channel : out_channel ref = ref stdout

let top_env : unit -> environment =
 fun () ->
  Symtab.empty
  |> Symtab.add "true" (Bool true)
  |> Symtab.add "false" (Bool false)

(** [display_value v] returns a string representation of the runtime value
    [v]. *)
let rec display_value v =
  match v with
  | Num x -> sprintf "%d" x
  | Bool b -> if b then "true" else "false"
  | Pair (v1, v2) ->
      sprintf "(pair %s %s)" (display_value v1) (display_value v2)
  | Nil -> "()"

(** [interp_0ary_primitive prim] tries to evaluate the primitive operation
    named by [prim]. If [prim] does not refer to a valid primitive operation, it
    returns [None]. *)
let interp_0ary_primitive : string -> value option =
 fun prim ->
  match prim with
  | "read-num" -> Some (Num (int_of_string (input_line !input_channel)))
  | "newline" ->
      output_string !output_channel "\n";
      Some (Bool true)
  | _ -> None

(** [interp_unary_primitive prim arg] tries to evaluate the primitive operation
    named by [prim] on the argument [arg]. If the operation is ill-typed, or if
    [prim] does not refer to a valid primitive operation, it returns [None]. *)
let interp_unary_primitive : string -> value -> value option =
 fun prim arg ->
  match (prim, arg) with
  | "add1", Num x -> Some (Num (x + 1))
  | "sub1", Num x -> Some (Num (x - 1))
  | "zero?", Num 0 -> Some (Bool true)
  | "zero?", _ -> Some (Bool false)
  | "num?", Num _ -> Some (Bool true)
  | "num?", _ -> Some (Bool false)
  | "not", Bool false -> Some (Bool true)
  | "not", _ -> Some (Bool false)
  | "pair?", Pair _ -> Some (Bool true)
  | "pair?", _ -> Some (Bool false)
  | "left", Pair (v, _) -> Some v
  | "right", Pair (_, v) -> Some v
  | "empty?", Nil -> Some (Bool true)
  | "empty?", _ -> Some (Bool false)
  | "print", v ->
      v |> display_value |> output_string !output_channel;
      Some (Bool true)
  | _ -> None

(** [interp_binary_primitive prim arg1 arg2] tries to evaluate the primitive
    operation named by [prim] on the arguments [arg1] and [arg2]. If the
    operation is ill-typed, or if [prim] does not refer to a valid primitive
    operation, it returns [None]. *)
let interp_binary_primitive : string -> value -> value -> value option =
 fun prim arg1 arg2 ->
  match (prim, arg1, arg2) with
  | "+", Num x1, Num x2 -> Some (Num (x1 + x2))
  | "-", Num x1, Num x2 -> Some (Num (x1 - x2))
  | "=", Num x1, Num x2 -> Some (Bool (x1 = x2))
  | "<", Num x1, Num x2 -> Some (Bool (x1 < x2))
  | "pair", v1, v2 -> Some (Pair (v1, v2))
  | _ -> None

(** Helper function to extract arguments from a list for apply *)
let rec extract_list_args : value -> value list =
 fun v ->
  match v with
  | Nil -> []
  | Pair (head, tail) -> head :: extract_list_args tail
  | _ -> raise (Failure "apply expected a list as its second argument")

(** Helper function to convert OCaml list to Lisp list *)
let rec make_lisp_list : value list -> value =
 fun values ->
  match values with [] -> Nil | v :: vs -> Pair (v, make_lisp_list vs)

(** [interp_expr defns env e] tries to evaluate the s_expression [e] in the
    environment [env] using definitions [defns], producing a value. If [e] isn't
    a valid expression, it raises an exception. *)
let rec interp_expr : defn list -> environment -> s_exp -> value =
 fun defns env e ->
  match e with
  | Num x -> Num x
  | Sym var -> (
      match Symtab.find_opt var env with
      | Some value -> value
      | None -> raise (Stuck e))
  | Lst [] -> Nil
  | Lst [ Sym "let"; Lst [ Lst [ Sym var; exp ] ]; body ] ->
      let env = env |> Symtab.add var (interp_expr defns env exp) in
      interp_expr defns env body
  | Lst [ Sym "if"; test_exp; then_exp; else_exp ] ->
      if interp_expr defns env test_exp <> Bool false then
        interp_expr defns env then_exp
      else interp_expr defns env else_exp
  | Lst (Sym "do" :: exps) when List.length exps > 0 ->
      exps |> List.rev_map (interp_expr defns env) |> List.hd
  | Lst [ Sym "apply"; Sym f; arg ] when is_defn defns f -> (
      let defn = get_defn defns f in
      let list_value = interp_expr defns env arg in

      (* Check if the second argument is a list *)
      match list_value with
      | Nil | Pair _ -> (
          (* Extract arguments from the list *)
          let args = extract_list_args list_value in

          (* Process based on whether the function is variadic *)
          match defn.rest with
          | None ->
              (* Regular function - check exact arity *)
              if List.length args = List.length defn.args then
                let fenv = args |> List.combine defn.args |> Symtab.of_list in
                interp_expr defns fenv defn.body
              else raise (Stuck e) (* Wrong number of arguments *)
          | Some rest_param ->
              (* Variadic function - check minimum arity *)
              if List.length args < List.length defn.args then raise (Stuck e)
              else
                (* Split arguments *)
                let req_args, rest_args =
                  List.partition_at (List.length defn.args) args
                in

                (* Create environment with required and rest parameters *)
                let req_env =
                  List.combine defn.args req_args |> Symtab.of_list
                in

                let fenv =
                  Symtab.add rest_param (make_lisp_list rest_args) req_env
                in

                interp_expr defns fenv defn.body)
      | _ -> raise (Stuck e)
      (* Second argument is not a list *))
  | Lst (Sym f :: args) when is_defn defns f -> (
      let defn = get_defn defns f in

      (* Process function arguments based on whether it's variadic or not *)
      match defn.rest with
      | None ->
          (* Non-variadic function - same as before *)
          if List.length args = List.length defn.args then
            let fenv =
              args
              |> List.map (interp_expr defns env)
              |> List.combine defn.args |> Symtab.of_list
            in
            interp_expr defns fenv defn.body
          else raise (Stuck e)
      | Some rest_param ->
          (* Variadic function *)
          (* Must have at least the required number of arguments *)
          if List.length args < List.length defn.args then raise (Stuck e)
          else
            (* Split arguments into required and rest *)
            let req_args, rest_args =
              List.partition_at (List.length defn.args) args
            in

            (* Evaluate all arguments *)
            let eval_req_args = List.map (interp_expr defns env) req_args in
            let eval_rest_args = List.map (interp_expr defns env) rest_args in

            (* Create environment with required args bound normally *)
            let req_env =
              List.combine defn.args eval_req_args |> Symtab.of_list
            in

            (* Add rest parameter bound to a list of remaining args *)
            let fenv =
              Symtab.add rest_param (make_lisp_list eval_rest_args) req_env
            in

            interp_expr defns fenv defn.body)
  | Lst [ Sym f ] -> (
      match interp_0ary_primitive f with Some v -> v | None -> raise (Stuck e))
  | Lst [ Sym f; arg ] -> (
      match interp_unary_primitive f (interp_expr defns env arg) with
      | Some v -> v
      | None -> raise (Stuck e))
  | Lst [ Sym f; arg1; arg2 ] -> (
      match
        let v1 = interp_expr defns env arg1 in
        let v2 = interp_expr defns env arg2 in
        interp_binary_primitive f v1 v2
      with
      | Some v -> v
      | None -> raise (Stuck e))
  | _ -> raise (Stuck e)

(** [interp exps] evaluates the list of s-expressions [exps] using
    [interp_expr], reading input from stdin and writing output to stdout. *)
let interp : s_exp list -> unit =
 fun exps ->
  let defns, body = defns_and_body exps in
  interp_expr defns (top_env ()) body |> ignore

(** [interp_io exps input] evaluates the list of s-expressions [exps] using
    [interp_expr], reading input from the string [input] and returning the
    output as a string. *)
let interp_io : s_exp list -> string -> string =
 fun exps input ->
  let input_pipe_ex, input_pipe_en = Unix.pipe () in
  let output_pipe_ex, output_pipe_en = Unix.pipe () in
  input_channel := Unix.in_channel_of_descr input_pipe_ex;
  set_binary_mode_in !input_channel false;
  output_channel := Unix.out_channel_of_descr output_pipe_en;
  set_binary_mode_out !output_channel false;
  let write_input_channel = Unix.out_channel_of_descr input_pipe_en in
  set_binary_mode_out write_input_channel false;
  let read_output_channel = Unix.in_channel_of_descr output_pipe_ex in
  set_binary_mode_in read_output_channel false;
  output_string write_input_channel input;
  close_out write_input_channel;
  interp exps;
  close_out !output_channel;
  let r = input_all read_output_channel in
  input_channel := stdin;
  output_channel := stdout;
  r
