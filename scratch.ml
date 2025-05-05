(***********************************)
(* let bindings don't mutate stuff *)

let x = 1
let print_x_first_version () = print_endline ("what is our first x? is it *still* 1? \t" ^  (string_of_int  x))

let x = x + 1
let print_x_second_version () = print_endline ("what is our second x? \t\t\t" ^ string_of_int  x)

(* 'let () = ...' is just a way of getting the output, making sure it's (), then throwing it away *)
let () = print_endline "----" 

let () = print_x_second_version ()
let () = print_x_first_version ()

(*********************************)
(* why don't they mutate stuff?? *)


let () = print_endline "*******"

let show_unrolled () =
    let x = 1 in
        let print_x_first_version () = print_endline ("what is our first x? is it *still* 1? \t" ^  (string_of_int  x)) in
            (* what scope are we in here?  so what's x on the line below? *)
            let x = x + 1 in 
                (* what scope are we in here?  so what's x on the line below this? *)
                let print_x_second_version () = print_endline ("what is our second x? \t\t\t" ^ string_of_int  x) in
                    let () = print_endline "----" in
                    
                        let () = print_x_second_version () in
                            let () = print_x_first_version () in 
                                ()

let () = show_unrolled ()

(******************************)
(* statements vs. expressions *)


let () = print_endline "*******"

let statement_example_first_version () =
    print_endline "hi";
    print_endline "bye"

let statement_example_second_version () = 
    let () = print_endline "hi" in
        print_endline "bye"


let () = statement_example_first_version ()
let () = statement_example_second_version ()

(* shadowing *)

(* let () = print_endline "*******"
let shadow_a = 
  let x = 1 in
    let x = 2 in
      x + x

let shadow_b = 
  let x = 1 in
  let y = (let x = 2 in x) in
  x + y *)