x:
        .zero   4
main:
        push    rbp
        mov     rbp, rsp
        mov     DWORD PTR x[rip], 5
        mov     eax, DWORD PTR x[rip]
        pop     rbp
        ret