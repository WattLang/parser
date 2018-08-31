# WotScript's parser

Implementation of a parser module.

## Current State

Parser for a calculator.

## Usage

### Parser

`make run args=<...>` to build the parser and run with args given (quote them if you have spaces).

`make` to only build it.

> Example:
> `tokens.json > make run` will parse the input and print the ast

### Test

`make run-test` to build and run all tests.

`make test` to only build test.

`make run-test args=--ast` to build and run all test printing the ast each time.

`make valgrind` to build the test and run them with valgrind.