# SPDX-License-Identifier: GPL-2.0-or-later
.code64
.global syscall_entry
.type syscall_entry, @function

syscall_entry:
    swapgs

    movq %rsp, %gs:16
    movq %gs:8, %rsp

    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    sub $8, %rsp

    movq %rsp, %rdi

    call syscall_handler

    addq $8, %rsp

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10 
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx

    addq $8, %rsp


    movq %gs:16, %rsp

    swapgs
    sysretq



.global exception_entry
.type exception_entry, @function

exception_entry:
    cli
    hlt
    jmp exception_entry