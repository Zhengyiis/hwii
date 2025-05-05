# Class Compiler

This is the compiler that we built in lecture.
I will keep it up to date as we move through the course.

Changes won't necessarily be purely additive, so 
you may want to look at the commit history 
if you want a point-in-time snapshot.

## Unofficial Install Guide

For [utop history](https://github.com/ocaml-community/utop/issues/478): `mkdir -r ~/.local/state`

## How to run

```sh
# to run the tests
dune runtest -f

# to run the interpreter...
# on a string
dune exec bin/interp.exe -- -e "(add1 5)"
# or on a file
dune exec bin/interp.exe examples/unary-nums.lisp 


# to run the compiler
# it puts the executable and assembly in directory "output/"
dune exec bin/compile.exe examples/unary-nums.lisp output/
```