open S_exp
open Shared
open Error
open Directive

(* Constants used for tagging values at runtime.
 *)

let num_shift =
  2

let num_mask =
  0b11

let num_tag =
  0b00

let bool_shift =
  7

let bool_mask =
  0b1111111

let bool_tag =
  0b0011111

(* `operand_of_num x` returns the runtime representation of the number `x` as
 * an operand for instructions.
 *)
let operand_of_num : int -> operand =
  fun x ->
    Imm ((x lsl num_shift) lor num_tag)

(* `operand_of_bool b` returns the runtime representation of the boolean `b` as
 * an operand for instructions.
 *)
let operand_of_bool : bool -> operand =
  fun b ->
    Imm (((if b then 1 else 0) lsl bool_shift) lor bool_tag)

(* Helper instructions to convert the ZF flag to the runtime representation of a
 * boolean.
 *)
let zf_to_bool : directive list =
  [ Mov (Reg Rax, Imm 0)
  ; Setz (Reg Rax)
  ; Shl (Reg Rax, Imm bool_shift)
  ; Or (Reg Rax, Imm bool_tag)
  ]

(* `compile_primitive e prim` produces x86-64 instructions for the primitive
 * operation named by `prim`; if `prim` isn't a valid operation, it raises an
 * error using the s-expression `e`.
 *)
let compile_primitive : s_exp -> string -> directive list =
  fun e prim ->
    begin match prim with
      | "add1" ->
          [ Add (Reg Rax, operand_of_num 1)
          ]

      | "sub1" ->
          [ Sub (Reg Rax, operand_of_num 1)
          ]

      | "zero?" ->
          [ Cmp (Reg Rax, operand_of_num 0)
          ]
          @ zf_to_bool

      | "num?" ->
          [ And (Reg Rax, Imm num_mask)
          ; Cmp (Reg Rax, Imm num_tag)
          ]
          @ zf_to_bool

      | "not" ->
          [ Cmp (Reg Rax, operand_of_bool false)
          ]
          @ zf_to_bool

      | _ ->
          raise (Stuck e)
    end

(* `compile_expr e` produces x86-64 instructions for the expression `e`.
 *)
let rec compile_expr : s_exp -> directive list =
  fun e ->
    begin match e with
      | Num x ->
          [Mov (Reg Rax, operand_of_num x)
          ]

      | Sym "true" ->
          [ Mov (Reg Rax, operand_of_bool true)
          ]

      | Sym "false" ->
          [ Mov (Reg Rax, operand_of_bool false)
          ]

      | Lst [Sym f; arg] as exp ->
          compile_expr arg @ compile_primitive exp f

      | e ->
          raise (Stuck e)
    end

(* `compile e` produces x86-64 instructions, including frontmatter, for the
 * expression `e`.
 *)
let compile : s_exp -> directive list =
  fun e ->
    [ Global "lisp_entry"
    ; Label "lisp_entry"
    ]
    @ compile_expr e
    @ [Ret]
