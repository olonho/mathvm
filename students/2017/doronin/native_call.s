	.text
	.globl	native_call
native_call:
  movq %rax, %xmm0
  pushq %rax
  movq %rax, %xmm1
  pushq %rax
  movq %rax, %xmm2
  pushq %rax
  movq %rax, %xmm3
  pushq %rax
  movq %rax, %xmm4
  pushq %rax

  pushq %r8
  pushq %rcx 
  pushq %rdx 
  movq (%rdi), %xmm0
  addq $8, %rdi 
  movq (%rdi), %xmm1
  addq $8, %rdi 
  movq (%rdi), %xmm2
  addq $8, %rdi 
  movq (%rdi), %xmm3
  addq $8, %rdi 
  movq (%rdi), %xmm4

  movq %rsi, %rax
  movq (%rax), %rdi
  addq $8, %rax
  movq (%rax), %rsi
  addq $8, %rax
  movq (%rax), %rdx
  addq $8, %rax
  movq (%rax), %rcx
  addq $8, %rax
  movq (%rax), %r8
  addq $8, %rax
  movq (%rax), %r9

  popq %rax
  callq *%rax
  popq %rdi
  movq %rax, (%rdi)

  popq %rdi
  popq %rcx
  movq %rcx, %xmm4
  popq %rcx
  movq %rcx, %xmm3
  popq %rcx
  movq %rcx, %xmm2
  popq %rcx
  movq %rcx, %xmm1

  movq %xmm0, (%rdi)
  popq %rcx
  movq %rcx, %xmm0

	retq
