open Ast

let gensym : string -> string =
  let counter = ref 0 in
  fun (base : string) ->
    let number = !counter in
    counter := !counter + 1;
    Printf.sprintf "%s__%d" base number

module List = struct
  include List

  let rec range lo hi = if lo > hi then [] else lo :: range (lo + 1) hi

  let partition_at (n : int) (l : 'a list) =
    if n < 0 then raise (Invalid_argument "partition_at");
    let rec go left rest = function
      | 0 -> (List.rev left, rest)
      | n -> go (List.hd rest :: left) (List.tl rest) (n - 1)
    in
    go [] l n
end

let rec input_all (ch : in_channel) : string =
  try
    let c = input_char ch in
    String.make 1 c ^ input_all ch
  with End_of_file -> ""

let rec desugar (defns : defn list ref) (e : expr) : expr =
  match e with
  | Num x -> Num x
  | Var x -> Var x
  | True -> True
  | False -> False
  | If (a, b, c) -> If (desugar defns a, desugar defns b, desugar defns c)
  | Let (x, e, b) -> Let (x, desugar defns e, desugar defns b)
  | Prim0 p -> Prim0 p
  | Prim1 (p, a) -> Prim1 (p, desugar defns a)
  | Prim2 (p, a, b) -> Prim2 (p, desugar defns a, desugar defns b)
  | Do es -> Do (List.map (desugar defns) es)
  | Call (f, es) -> Call (desugar defns f, List.map (desugar defns) es)
  | Lambda (args, body) ->
      let name = gensym "_lambda" in
      defns := { name; args; body = desugar defns body } :: !defns;
      Closure name
  | Nil -> Nil
  | Closure _ -> failwith "shouldn't happen"

let desugar_program (program : program) : program =
  let defns_ref = ref program.defns in
  let new_body = desugar defns_ref program.body in
  { defns = !defns_ref; body = new_body }
