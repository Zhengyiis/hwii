open Printf
open Util
open Shared
open Ast

type ty = Num | Bool | Function of (ty list * ty) | Nil | Pair of (ty * ty)
type tyctx = ty Symtab.symtab

let ty_of_name (s : string) : ty =
  if String.ends_with ~suffix:"-num" s then Num
  else if String.ends_with ~suffix:"-bool" s then Bool
  else failwith (Printf.sprintf "Could not get a type for '%s'" s)

(* our type system won't be super good, so we'll disable it by default *)
(* it won't handle generic functions, or functions without explicit type annotations *)
(* typecheck prog;
  let prog = desugar_program prog in
  interp_expr prog.defns Symtab.empty prog.body |> ignore *)

let infer_0ary_primitive prim : ty =
  match prim with ReadNum -> failwith "todo" | Newline -> failwith "todo"

let infer_unary_primitive prim arg : ty =
  match (prim, arg) with
  | Add1, Num -> Num
  | Sub1, Num -> Num
  | IsZero, _ -> Bool
  | IsNum, _ -> Bool
  | Not, _ -> Bool
  | IsPair, _ -> Bool
  | Left, Pair (t, _) -> t
  | Right, Pair (_, t) -> t
  | IsEmpty, _ -> Bool
  | Print, _ -> Bool
  | _ -> failwith "type error"

let infer_binary_primitive prim arg1 arg2 =
  match (prim, arg1, arg2) with
  | (Plus | Minus), Num, Num -> Num
  | (Eq | Lt), Num, Num -> Bool
  | Pair, t1, t2 -> Pair (t1, t2)
  | _ -> failwith "type error"

let infer_expr (ctx : tyctx) (expr : expr) : ty =
  match expr with
  | Num _ -> Num
  | True | False -> Bool
  | Var var as e -> failwith "todo"
  | Nil -> failwith "todo"
  | Let (var, exp, body) -> failwith "todo"
  | If (test_exp, then_exp, else_exp) -> failwith "todo"
  | Do exps -> failwith "todo"
  | Call (f_expr, args) as e -> failwith "todo"
  | Prim0 f as e -> failwith "todo"
  | Prim1 (f, arg) as e -> failwith "todo"
  | Prim2 (f, arg1, arg2) as e -> failwith "todo"
  | Lambda _ -> failwith "todo"
  | Closure f -> failwith "impossible, pre-desugar"

let check_expr (ctx : tyctx) (expr : expr) (ty : ty) : unit =
  if ty = infer_expr ctx expr then () else failwith "type error"

let check_defn (ctx : tyctx) (defn : defn) : unit = failwith "todo"

let ty_of_defn (defn : defn) : ty =
  let output_ty = ty_of_name defn.name in
  let input_tys = List.map ty_of_name defn.args in
  Function (input_tys, output_ty)

let typecheck (prog : program) : unit =
  let tyctx =
    List.map (fun d -> (d.name, ty_of_defn d)) prog.defns |> Symtab.of_list
  in
  List.iter (check_defn tyctx) prog.defns;
  check_expr tyctx prog.body Bool
