main:
        push    rbp
        mov     rbp, rsp
        mov     DWORD PTR [rbp-4], 5
        mov     DWORD PTR [rbp-8], 10
        mov     edx, DWORD PTR [rbp-4]
        mov     eax, DWORD PTR [rbp-8]
        add     eax, edx
        pop     rbp
        ret