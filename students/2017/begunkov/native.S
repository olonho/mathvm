    .code64
    .align 8
    .text
    .globl run_native_foo_double, run_native_foo_int, run_native_foo_char
    .type   run_native_foo_double, @function
    .type   run_native_foo_int, @function
    .type   run_native_foo_char, @function

#
#   floats, integers, foo
#   %rdi    %rsi      %rdx
#
run_native_foo_double:
run_native_foo_int:
run_native_foo_char:
    mov %rdi, %r12
    mov %rsi, %r13
    mov %rdx, %r14

    movsd 56(%rdi), %xmm7
    movsd 48(%rdi), %xmm6
    movsd 40(%rdi), %xmm5
    movsd 32(%rdi), %xmm4
    movsd 24(%rdi), %xmm3
    movsd 16(%rdi), %xmm2
    movsd 8(%rdi), %xmm1
    movsd (%rdi), %xmm0

    mov 0(%r13), %rdi
    mov 8(%r13), %rsi
    mov 16(%r13), %rdx
    mov 24(%r13), %rcx
    mov 32(%r13), %r8
    mov 40(%r13), %r9

    jmp %r14
