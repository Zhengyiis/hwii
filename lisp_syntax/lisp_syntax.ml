open S_exp
open Ast

exception ParseError of s_exp

let prim0_of_string : string -> prim0 option =
 fun s ->
  match s with
  | "read-num" -> Some ReadNum
  | "newline" -> Some Newline
  | _ -> None

let prim1_of_string : string -> prim1 option =
 fun s ->
  match s with
  | "add1" -> Some Add1
  | "sub1" -> Some Sub1
  | "zero?" -> Some IsZero
  | "num?" -> Some IsNum
  | "pair?" -> Some IsPair
  | "empty?" -> Some IsEmpty
  | "not" -> Some Not
  | "left" -> Some Left
  | "right" -> Some Right
  | "print" -> Some Print
  | _ -> None

let prim2_of_string : string -> prim2 option =
 fun s ->
  match s with
  | "+" -> Some Plus
  | "-" -> Some Minus
  | "=" -> Some Eq
  | "<" -> Some Lt
  | "pair" -> Some Pair
  | _ -> None

let get_sym : s_exp -> string = function
  | Sym s -> s
  | _ -> failwith "not a sym"

let rec expr_of_s_exp : s_exp -> expr =
 fun s_exp ->
  match s_exp with
  | Num x -> Num x
  | Sym "true" -> True
  | Sym "false" -> False
  | Sym var -> Var var
  | Lst [] -> Nil
  | Lst [ Sym "let"; Lst [ Lst [ Sym var; exp ] ]; body ] ->
      Let (var, expr_of_s_exp exp, expr_of_s_exp body)
  | Lst (Sym "do" :: exps) when List.length exps > 0 ->
      Do (List.map expr_of_s_exp exps)
  | Lst [ Sym "if"; test_s; then_s; else_s ] ->
      If (expr_of_s_exp test_s, expr_of_s_exp then_s, expr_of_s_exp else_s)
  | Lst [ Sym prim ] when Option.is_some (prim0_of_string prim) ->
      Prim0 (Option.get (prim0_of_string prim))
  | Lst [ Sym prim; arg ] when Option.is_some (prim1_of_string prim) ->
      Prim1 (Option.get (prim1_of_string prim), expr_of_s_exp arg)
  | Lst [ Sym prim; arg1; arg2 ] when Option.is_some (prim2_of_string prim) ->
      Prim2
        ( Option.get (prim2_of_string prim),
          expr_of_s_exp arg1,
          expr_of_s_exp arg2 )
  | Lst [ Sym "lambda"; Lst args; body ] ->
      Lambda (List.map get_sym args, expr_of_s_exp body)
  | Lst (f :: args) -> Call (expr_of_s_exp f, List.map expr_of_s_exp args)
  | e -> raise (ParseError e)

let program_of_s_exps : s_exp list -> program =
  let rec get_args : s_exp list -> string list =
   fun args ->
    match args with
    | Sym v :: args -> v :: get_args args
    | e :: _ -> raise (ParseError e)
    | [] -> []
  in
  let get_defn : s_exp -> defn =
   fun s_exp ->
    match s_exp with
    | Lst [ Sym "define"; Lst (Sym name :: args); body ] ->
        { name; args = get_args args; body = expr_of_s_exp body }
    | e -> raise (ParseError e)
  in
  let rec go : defn list -> s_exp list -> program =
   fun defns s_exps ->
    match s_exps with
    | [ e ] -> { defns = List.rev defns; body = expr_of_s_exp e }
    | d :: rest -> go (get_defn d :: defns) rest
    | _ -> raise (ParseError (Sym "empty"))
  in
  go []

let parse : string -> program =
 fun s -> s |> S_exp.parse_many |> program_of_s_exps
