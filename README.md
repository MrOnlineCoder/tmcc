# TMCC

TMCC (Trying to Make C Compiler) should be a minimalistic C compiler written for educational purposes. Initial goal of the project is to be able to compile the compiler itself.

The aim is to significantly boost my skills in C programming, compiler design and understanding, assembly and formal standard comprehension.

## Constraints and Requirements

1. Compiler should fully follow one of C standards, probably ANSI C and provide fully-working standard library.
2. Target platform is x86-64 Linux with theoretical support for other platforms in the future. My development platform is Mac OS, so I will run it in QEMU or just in a VM.
3. Compiler should produce NASM assembly code, which is then assembled and linked using NASM.
4. Ideal goal - self-hosting compiler.
5. **No LLVM!** LLVM is great, but what's the fun then if all the backend will be done by it?

## TODO

Base compiler parts:

- [x] Tokenizer
- [x] Base parser / AST
- [x] Semantic analyzer
- [x] Code generator
- [x] Assembler / linker stage
- [ ] Simple AST optimizer
- [ ] Standard library implementation
- [ ] Command-line flags
- [ ] Preprocessor
- [ ] Self-hosting compiler
- [ ] Primitive register allocation
- [ ] Arena based allocation of AST nodes, types and symbols

Language features:

- [x] Primitive types (int, char, float, double)
- [x] Arithmetic binary operators (`+`, `-`, `*`, `/`)
- [x] Bitwise binary operators (`&`, `|`, `^`, `<<`, `>>`)
- [ ] Logical binary operators (`&&`, `||`)
- [ ] Unary operators (`+`, `-`, `!`, `~`)
- [x] Comparison operators (`==`, `!=`, `<`, `>`, `<=`, `>=`)
- [x] Variable declarations
- [ ] Variable initializations
- [ ] Pointers
- [ ] Arrays
- [ ] Structs
- [ ] Unions
- [ ] Enums
- [ ] Functions declarations
- [x] `if` / `else`
- [x] `while`
- [ ] `for`
- [ ] `do` / `while`
- [ ] `switch` / `case`
- [ ] `goto` / labels
- [ ] `break` / `continue`
- [x] `return`
- [ ] `sizeof`
- [ ] Type qualifiers (`const`, `volatile`, `restrict`)
- [ ] Storage class specifiers (`static`, `extern`)
- [ ] Custom typedefs
- [ ] Dereference and address-of operators (`*`, `&`)
- [ ] Function pointers
- [ ] Variadic arguments
- [ ] Preprocessor directives (`#include`, `#define`, `#ifdef`, `#endif`)
- [x] Comments
- [ ] Support for multiple source files
- [ ] Support for header files
- [ ] Attributes (`__attribute__((...))`)
