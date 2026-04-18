main:
        push    rbp
        mov     rbp, rsp
        mov     DWORD PTR [rbp-4], 0
        mov     DWORD PTR [rbp-8], 0
        jmp     .L2
.L3:
        add     DWORD PTR [rbp-4], 1
        mov     eax, DWORD PTR [rbp-4]
        add     DWORD PTR [rbp-8], eax
.L2:
        cmp     DWORD PTR [rbp-4], 4
        jle     .L3
        mov     eax, DWORD PTR [rbp-8]
        pop     rbp
        ret