main:
        push    rbp
        mov     rbp, rsp
        mov     DWORD PTR [rbp-4], 2
        mov     DWORD PTR [rbp-8], 1
        mov     eax, DWORD PTR [rbp-4]
        cmp     eax, DWORD PTR [rbp-8]
        jle     .L2
        mov     eax, 42
        jmp     .L3
.L2:
        mov     eax, 0
.L3:
        pop     rbp
        ret