# WotScript's parser

Implementation of a parser module.

## Current State

Parser for a calculator.

## Usage

### Parser

`make run args=<...>` to build the parser and run with args given (quote them if you have spaces).

`make` to only build it.

> Example:
> `make run < tokens.json` will parse the input and print the ast

### Test

`make run-test` to build and run all tests.

`make test` to only build test.

`make run-test args=--ast` to build and run all test printing the ast each time.

`make valgrind` to build the test and run them with valgrind.

## AST

The root is one of the nodes below.

### Literal

The key `type` start with `literal`

#### Number

Key `type` : `literal.float`
Key `value` : strign representation of the float

### Operator

The key `type` start with `operator`

#### Unary Operator

Key `type` : `operator.minus`
Key `operand` : node to negate

> `- eval(this->operand)`

#### Binary Operator

Key `type` : `operator.X` where `X` is `plus`, `subtract`, `multiplication` or `division`
Key `lhs` : the left operand's node
Key `rhs` : the left operand's node

> `eval(this->lhs) op eval(this->rhs)` where `op` is `+`, `-`, `*` or `/`
