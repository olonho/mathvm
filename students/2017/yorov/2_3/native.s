    .text
    .globl	nativeInt, nativeDouble, nativeString

nativeInt:
nativeDouble:
nativeString:
    pushq %rbp
    movq %rsp, %rbp

    movq (%rdi), %xmm0
    movq 8(%rdi), %xmm1
    movq 16(%rdi), %xmm2
    movq 24(%rdi), %xmm3
    movq 32(%rdi), %xmm4
    movq 40(%rdi), %xmm5

    movq %rsi, %rax
    movq (%rax), %rdi
    movq 8(%rax), %rsi

    # push rdx to stack, because we will change it later
    pushq %rdx

    movq 16(%rax), %rdx
    movq 24(%rax), %rcx
    movq 32(%rax), %r8
    movq 40(%rax), %r9

    # pop rdx - pointer to func
    popq %rax
    call *%rax

    movq %rbp, %rsp
    popq %rbp
    retq
