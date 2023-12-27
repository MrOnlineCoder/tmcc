# TMCC

TMCC (Trying to Make C Compiler) should be a minimalistic C compiler written for educational purposes. Initial goal of the project is to be able to compile the compiler itself.

Target architecture would be either arm64 (for MacOS) or x86_64 (for Linux).

## Steps

1. **Read Dragon book (Compilers: Principles, Techniques, and Tools)**
2. Write a lexer
3. Write a parser
4. Write a code generator
5. Integrate with linker, preferably write a custom one
6. Test the programs and their code's correctness
7. Build the compiler with itself
