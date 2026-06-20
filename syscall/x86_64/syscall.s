.global syscall_entry
.type syscall_entry, @function

syscall_entry:
    swapgs
    mov %rsp, %gs:16
    mov %gs:8, %rsp

    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
    sub $8, %rsp


    mov %r10, %rcx

    call syscall_handler



    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10 
    pop %r9
    pop %r8
    pop %rbp
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rbx
    add $16, %rsp


    mov %rsp, %gs:8
    mov %gs:16, %rsp

    swapgs
    sysret