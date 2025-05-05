open Ast

let rec fold (top_e : expr) : expr =
  match top_e with
  | Prim1 (Add1, arg) -> (
      let e = fold arg in
      match e with Num n -> Num (n + 1) | _ -> Prim1 (Add1, e))
  | Prim1 (p, arg) -> Prim1 (p, fold arg)
  | Let (v, e, body) -> Let (v, fold e, fold body)
  | _ -> top_e

(*

(add1 5) => 6
(add1 (add1 5)) => 7

(add1 (read-num))
(add1 bigprogram)
(add1 false)

(add1 (+ (read-num) (add1 7)))

(let ((x 3))
 (add1 x))

*)

let fold_program (prog : program) =
  {
    body = fold prog.body;
    defns =
      List.map
        (fun { name; args; body } -> { name; args; body = fold body })
        prog.defns;
  }
