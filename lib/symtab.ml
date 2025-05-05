open String

module Symtab = Map.Make (struct
  type t = string

  let compare = compare
end)

type 'a symtab = 'a Symtab.t

include Symtab

let of_list l = l |> List.to_seq |> of_seq
let add_list s l = Seq.append (to_seq s) (List.to_seq l) |> of_seq
