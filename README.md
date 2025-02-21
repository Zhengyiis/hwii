# Homework 2: Characters Welcome

_**Note:** This is a 1-week assignment (due 1 week after release)._

## Introduction

In this homework, you'll be building upon an interpreter and a compiler for a
language with numbers, booleans, and several unary operations. You'll be
extending both the compiler and the interpreter with a new type for individual
characters.

**Note:** For this homework, you'll be using
[ASCII codes](http://www.asciitable.com/)
when working with characters. We define a _permissible_ character to be one
whose ASCII code is either 10 (corresponding to a newline) or in the range 32 to
126 inclusive (corresponding to a "printable character"). You may assume that
all character literals emitted from the parser are permissible characters.

You will implement three unary operations for working with characters:

- `(char? e)`, which returns `true` if its argument is a character and `false`
  otherwise
- `(char->num e)`, which converts its argument from a character to a number
  representing its ASCII code. When the argument is not a character, our
  implementations can do anything; this is undefined behavior.  
- `(num->char e)`, which converts its argument from a number representing an
  ASCII code to its corresponding character. When the argument is not a number
  that corresponds to the ASCII code of a permissible character, our
  implementations can do anything; this is undefined behavior.

When you need to raise an exception in the above operations, you are free to
choose any type of exception that you wish, even those that you define yourself;
moreover, the operations need not raise distinct exceptions.

The parser in the `S_exp` module handles parsing characters. In our Lisp
dialect, characters are written as `#\` followed by a permissible character
_except_ for a newline or space, which is written out in full as either
"newline" or "space" respectively. For example, all of the following are
characters:

- `#\a`
- `#\A`
- `#\;`
- `#\newline`
- `#\space`

The same format will be used when printing out the result of the program.
For example the result of the program `(num->char (add1 (char->num #\a)))` will
be printed as `#\b`.

### Working with the infrastructure

For additional guidance in working with the infrastructure for this homework,
please feel free to take a look at the
[Section 3 materials](https://classroom.github.com/a/5MelbZP0)!

## Grammar

```bnf
<bool> ::= true
         | false


<unary_op> ::= add1
             | sub1
             | zero?
             | num?
             | not
             | char?
             | char->num
             | num->char

<expr> ::= <num>
         | <bool>
         | <char>
         | (<unary_op> <expr>)
```

## Testing

We provide you with a differential testing framework for your interpreter and
compiler.

In the `examples` directory, you can provide test cases for your interpreter
and compiler by creating a file `X.lisp` (where `X` is any test name you want)
that contains an program you want your interpreter and compiler to support.

The testing framework will run your interpreter and compiler on the `X.lisp`
file. **If the output of your interpreter and compiler agree, then the framework
will consider that test to be passing**; otherwise, that test is considered to
be failing.

If you additionally provide a file `X.out` (where `X` is the same test name as
in `X.lisp`) that contains the expected output for `X.lisp`, **the testing
framework will ensure that both your interpreter and compiler match the output
exactly**, passing if that is the case and failing otherwise.

If instead you provide a file `X.err`, the testing framework will ensure that
**the testing framework will ensure that both your interpreter and compiler
throw an exception**, passing if that is the case and failing otherwise. The
contents of `X.err` can be anything, including nothing at all (the contents
are only used for printing out failing test cases); what matters is only that
the file exists.

As a convenience, you can also provide a file `examples/examples.csv` of the
following format (where the "expected output" column is optional, mimicking the
behavior in the preceding paragraphs):
```
input program 0, expected output 0
input program 1, expected output 1
... etc
```
By using the convenience file `examples/examples.csv`, you can write multiple
input-output tests all in one file.

Note that the `examples/examples.csv` shorthand does not support any equivalent
of `*.err` files, so you cannot use this shorthand to check that your
interpreter and compiler both throw an exception.

To run the testing framework on your tests in the `examples` directory, use the
following command:

```
dune runtest -f
```

### Gradescope

When you submit your implementation to Gradescope (to the assignment
`hw2-code`), **your suite of examples will be run against a reference
interpreter and compiler.** This is analogous to comparing your compiler with an
existing implementation, one of the testing strategies we mentioned in class. If
the reference implementation fails on any of your examples, Gradescope will show
you how its output differed from the expected output of your example (if you
wrote a `*.out` or `*.err` file for it or had expected output in the
`examples/example.csv` file).

**You can do this as many times as you want.** We encourage you to use this
option to develop a good set of examples *before* you start working on your
interpreter and compiler!

### Running the compiler and the interpreter manually

In addition to using `dune runtest -f`, you can manually run the interpreter
and compiler on input files.

To run the interpreter on a file, execute the following command:

```
dune exec bin/interp.exe -- <file.lisp>
```

To run the compiler on a file, execute the following command:

```
dune exec bin/compile.exe -- <file.lisp> output
```

The resulting `.s` file (containing assembly code) and `.exe` file (an
executable) will be in the `output/` directory. You can then run the executable
with:

```
./output/<file.lisp>.exe
```

You can also tell the compiler to run the executable immediately after compiling
by adding `-r`, e.g.:

```
dune exec bin/compile.exe -- <file.lisp> output -r
```


## 1. Testing in action (1 subtask)

**Task 1.1:** Write a set of examples in the `examples` directory that exercise
all the character operations described at the beginning of this write-up.

_Hint:_ If you run `dune runtest -f` after completing this task, you should see
a bunch of failing tests. We're about to implement all those operations now!

_Hint:_ For this testing task, pretend that you're a test engineer, and you've
been handed a compiler. What tests would you run to persuade another programmer
that it'll do the right thing on all the future programs that programmers might
throw at it?  If you think your suite of tests would persuade a fellow compiler
designer, expect Task 1 to get a good grade!

## 2. Extending the interpreter (3 subtasks)

We'll now extend the interpreter with support for characters by modifying
`lib/interp.ml`.

**Task 2.1:** Add `Char` as a variant of the `value` type (consisting of an
[OCaml `char`](https://ocaml.org/api/Char.html)) and extend `display_value` to
support it.  Remember that characters need to printed in the same format that we
use in the input S-expressions. So the character `b` is printed as `#\b`.

_Note:_ Be careful to handle `#\space` and `#\newline` correctly, as OCaml's
`sprintf` will format them as '` `' and '`\n`'.

**Task 2.2:** Add support for character constants to `interp_expr`.

_Tip:_ Take a look at
[shared/s_exp/exp.ml](./shared/s_exp/exp.ml)
to see how an s-expression is defined for this homework.

**Task 2.3:** Add support for the three new primitives operations to
`interp_primitive`.

## 3. Extending the runtime (1 subtask)

Before extending the compiler, we must extend the runtime so that it is aware of
characters.  We'll represent characters during runtime as their ASCII values
shifted left 8 places and tagged with `0b00001111`.

**Task 3.1:** Inside of `lib/runtime/runtime.c`, extend the `print_value`
function to handle printing characters in the `#\a` form our Lisp dialect uses.

_Note:_ Be careful to handle `#\space` and `#\newline` correctly, as C's
`printf` will format them as '` `' and '`\n`'.

## 4. Extending the compiler (4 subtasks)

We can now extend the compiler with support for characters by modifying
`lib/compile.ml`.

**Task 4.1:** Add constants `char_mask` and `char_tag` to tag character
immediates as required by the runtime (similar to how we've tagged numbers and
booleans).

**Task 4.2:** Implement a function `operand_of_char : char -> operand` to make
an immediate from a character constant.

**Task 4.3:** Add handling for character constants to `compile_expr`. This
should be similar to the handling for `Num` and include a call to
`operand_of_char`.

**Task 4.4:** Add support for the three new character primitive operations to
`compile_primitive`.

_Tip:_ Take a look at
[shared/directive.ml](./shared/directive.ml)
to see how a directive is defined for this homework.

## 5. Written reflection (1 subtask)

The ASCII code for a newline character is the decimal number 10 (`0xA` in
hexadecimal). If you run this Lisp code with either your interpreter or your
compiler:

```
(char->num #\newline)
```

it should produce 10.

**Task 5.1:** In the Gradescope assignment `hw2-written`, submit a SHORT
response that answers the following two questions:

1. Where is the fact that a newline maps to the number 10 encoded in your
   interpreter?
2. How about your compiler?
