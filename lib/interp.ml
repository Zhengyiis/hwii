open Printf
open Util
open S_exp
open Shared
open Error

(** A [value]  is the runtime value of an expression. *)
type value =
  | Num of int
  | Bool of bool
  | Pair of (value * value)
  | Str of string
  | InChannel of in_channel
  | OutChannel of out_channel

type environment = value Symtab.symtab

let input_channel : in_channel ref = ref stdin
let output_channel : out_channel ref = ref stdout

let top_env : unit -> environment =
 fun () ->
  Symtab.empty
  |> Symtab.add "true" (Bool true)
  |> Symtab.add "false" (Bool false)
  |> Symtab.add "stdin" (InChannel !input_channel) (* Added *)
  |> Symtab.add "stdout" (OutChannel !output_channel)
(* Added *)

(** [display_value v] returns a string representation of the runtime value
    [v]. *)
let rec display_value : value -> string =
 fun v ->
  match v with
  | Num x -> sprintf "%d" x
  | Bool b -> if b then "true" else "false"
  | Pair (v1, v2) ->
      sprintf "(pair %s %s)" (display_value v1) (display_value v2)
  | Str s -> sprintf "\"%s\"" (String.escaped s)
  | InChannel _ -> "<in-channel>"
  | OutChannel _ -> "<out-channel>"

(** [interp_0ary_primitive prim] tries to evaluate the primitive operation
    named by [prim]. If [prim] does not refer to a valid primitive operation, it
    returns [None]. *)
let interp_0ary_primitive : string -> value option =
 fun prim ->
  match prim with
  | "read-num" -> Some (Num (input_line !input_channel |> int_of_string))
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
  | "print", v ->
      v |> display_value |> output_string !output_channel;
      Some (Bool true)
  | "open-in", Str filename -> Some (InChannel (open_in filename))
  | "open-out", Str filename -> Some (OutChannel (open_out filename))
  | "close-in", InChannel ch ->
      close_in ch;
      Some (Bool true)
  | "close-out", OutChannel ch ->
      close_out ch;
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
  | "input", InChannel ch, Num n when n >= 0 ->
      let buf = Bytes.create n in
      let bytes_read =
        try
          really_input ch buf 0 n;
          n
        with End_of_file -> 0
      in
      Some (Str (Bytes.sub_string buf 0 bytes_read))
  | "output", OutChannel ch, Str s ->
      output_string ch s;
      flush ch;
      Some (Bool true)
  | _ -> None

(** [interp_trinary_primitive prim arg1 arg2 arg3] tries to evaluate the
    primitive operation named by [prim] on the arguments [arg1], [arg2], and
    [arg3]. If the operation is ill-typed, or if [prim] does not refer to a
    valid primitive operation, it returns [None]. *)
let interp_trinary_primitive : string -> value -> value -> value -> value option
    =
 fun prim arg1 arg2 arg3 -> match (prim, arg1, arg2, arg3) with _ -> None

(** [interp_expr e] tries to evaluate the s_expression [e], producing a
    value. If [e] isn't a valid expression, it raises an exception. *)
let rec interp_expr : environment -> s_exp -> value =
 fun env e ->
  match e with
  | Num x -> Num x
  | Str s -> Str s
  | Sym var -> (
      match Symtab.find_opt var env with
      | Some value -> value
      | None -> raise (Stuck e))
  | Lst [ Sym "let"; Lst [ Lst [ Sym var; exp ] ]; body ] ->
      let env = env |> Symtab.add var (interp_expr env exp) in
      interp_expr env body
  | Lst [ Sym "if"; test_exp; then_exp; else_exp ] ->
      if interp_expr env test_exp <> Bool false then interp_expr env then_exp
      else interp_expr env else_exp
  | Lst (Sym "do" :: exps) when List.length exps > 0 ->
      exps |> List.rev_map (interp_expr env) |> List.hd
  | Lst [ Sym f ] -> (
      match interp_0ary_primitive f with Some v -> v | None -> raise (Stuck e))
  | Lst [ Sym f; arg ] -> (
      match interp_unary_primitive f (interp_expr env arg) with
      | Some v -> v
      | None -> raise (Stuck e))
  | Lst [ Sym f; arg1; arg2 ] -> (
      match
        let v1 = interp_expr env arg1 in
        let v2 = interp_expr env arg2 in
        interp_binary_primitive f v1 v2
      with
      | Some v -> v
      | None -> raise (Stuck e))
  | Lst [ Sym f; arg1; arg2; arg3 ] -> (
      match
        let v1 = interp_expr env arg1 in
        let v2 = interp_expr env arg2 in
        let v3 = interp_expr env arg3 in
        interp_trinary_primitive f v1 v2 v3
      with
      | Some v -> v
      | None -> raise (Stuck e))
  | e -> raise (Stuck e)

(** [interp e] evaluates the s-expression [e] using [interp_expr], reading input
    from stdin and writing output to stdout. *)
let interp : s_exp -> unit = fun e -> e |> interp_expr (top_env ()) |> ignore

(** [interp_io e input] evaluates the s-expression [e] using [interp_expr],
   reading input from the string [input] and returning the output as a
   string. *)
let interp_io e input =
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
  interp e;
  close_out !output_channel;
  let r = input_all read_output_channel in
  input_channel := stdin;
  output_channel := stdout;
  r
