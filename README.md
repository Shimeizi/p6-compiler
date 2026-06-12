# 🛠️ P6 Compiler

A project developed for the **Compilers** course.

## 📖 Overview

This project implements a compiler for **P6**, an imperative programming language designed for the course.

The compiler parses P6 source files, performs semantic analysis and generates assembly code using the provided Compiler Development Kit (CDK) and Runtime System (RTS).

P6 supports variables, functions, pointers, control-flow instructions, modules and custom numeric types.

## ✨ Features

* 📝 Lexical analysis
* 🌳 Syntax analysis (parsing)
* ✅ Semantic analysis
* ⚙️ Assembly code generation
* 📦 Multi-module programs
* 🔢 Support for P6 numeric types
* 🔗 Function declarations and calls
* 📍 Pointer operations
* 🔄 Control-flow instructions

## 🛠️ Built With

* C++
* CDK
* RTS
* YASM

## ⚙️ Compilation

Build the compiler using:

```bash
make
```

## ▶️ Running

Compile a P6 source file:

```bash
p6 --target asm program.p6
```

Assemble and link:

```bash
yasm -felf32 program.asm
ld -melf_i386 -o program program.o -lrts
```

Run the executable:

```bash
./program
```

## 🎯 Supported Language Features

* Variables and declarations
* Functions and return values
* Conditional statements
* Loops
* Strings
* Pointers
* Expressions and operators
* Input and output instructions
* Multi-file programs

## 📄 Additional Information

For a detailed description of the language and project requirements, please refer to the P6 Language Reference Manual included in this repository.
