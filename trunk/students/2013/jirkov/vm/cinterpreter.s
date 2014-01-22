	.file	"cinterpreter.cpp"
	.text
	.p2align 4,,15
	.globl	_ZN6mathvm15calc_stack_initEm
	.type	_ZN6mathvm15calc_stack_initEm, @function
_ZN6mathvm15calc_stack_initEm:
.LFB1752:
	.cfi_startproc
	pushq	%rbx
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
	movl	$8, %esi
	movq	%rdi, %rbx
	call	calloc
	movq	%rbx, _ZN6mathvm18g_calc_stack_countE(%rip)
	movq	%rax, _ZN6mathvm12g_calc_stackE(%rip)
	popq	%rbx
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE1752:
	.size	_ZN6mathvm15calc_stack_initEm, .-_ZN6mathvm15calc_stack_initEm
	.p2align 4,,15
	.globl	_ZN6mathvm17calc_stack_deinitEv
	.type	_ZN6mathvm17calc_stack_deinitEv, @function
_ZN6mathvm17calc_stack_deinitEv:
.LFB1753:
	.cfi_startproc
	movq	_ZN6mathvm12g_calc_stackE(%rip), %rdi
	jmp	free
	.cfi_endproc
.LFE1753:
	.size	_ZN6mathvm17calc_stack_deinitEv, .-_ZN6mathvm17calc_stack_deinitEv
	.p2align 4,,15
	.globl	_ZN6mathvm16interpreter_initEv
	.type	_ZN6mathvm16interpreter_initEv, @function
_ZN6mathvm16interpreter_initEv:
.LFB1754:
	.cfi_startproc
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	movl	$1048576, %edi
	call	_ZN6mathvm14ctx_stack_initEm
	movl	$8, %esi
	movl	$1048576, %edi
	call	calloc
	movq	$1048576, _ZN6mathvm18g_calc_stack_countE(%rip)
	movq	%rax, _ZN6mathvm12g_calc_stackE(%rip)
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE1754:
	.size	_ZN6mathvm16interpreter_initEv, .-_ZN6mathvm16interpreter_initEv
	.p2align 4,,15
	.globl	_ZN6mathvm18interpreter_deinitEv
	.type	_ZN6mathvm18interpreter_deinitEv, @function
_ZN6mathvm18interpreter_deinitEv:
.LFB1755:
	.cfi_startproc
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	call	_ZN6mathvm16ctx_stack_deinitEv
	movq	_ZN6mathvm12g_calc_stackE(%rip), %rdi
	call	free
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	jmp	_ZN6mathvm21constants_pool_deinitEv
	.cfi_endproc
.LFE1755:
	.size	_ZN6mathvm18interpreter_deinitEv, .-_ZN6mathvm18interpreter_deinitEv
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"instr: "
	.text
	.p2align 4,,15
	.globl	_ZN6mathvm9debuginfoEv
	.type	_ZN6mathvm9debuginfoEv, @function
_ZN6mathvm9debuginfoEv:
.LFB1756:
	.cfi_startproc
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	movl	$.LC0, %esi
	movl	$1, %edi
	xorl	%eax, %eax
	call	__printf_chk
	movq	_ZN6mathvm11g_registersE(%rip), %rax
	movq	stdout(%rip), %rsi
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	leaq	-1(%rax), %rdi
	jmp	_ZN6mathvm16instruction_dumpEPKhP8_IO_FILE
	.cfi_endproc
.LFE1756:
	.size	_ZN6mathvm9debuginfoEv, .-_ZN6mathvm9debuginfoEv
	.section	.rodata.str1.1
.LC1:
	.string	"INVALID OPCODE"
.LC6:
	.string	"%ld"
.LC7:
	.string	"%lf"
.LC8:
	.string	"%ld %lf %p\n"
	.text
	.p2align 4,,15
	.globl	_ZN6mathvm17interpreter_startEPh
	.type	_ZN6mathvm17interpreter_startEPh, @function
_ZN6mathvm17interpreter_startEPh:
.LFB1757:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	pushq	%rbx
	.cfi_def_cfa_offset 24
	.cfi_offset 3, -24
	subq	$40, %rsp
	.cfi_def_cfa_offset 64
	movq	_ZN6mathvm12g_calc_stackE(%rip), %rax
	movq	_ZN6mathvm18g_calc_stack_countE(%rip), %rdx
	movq	%rdi, _ZN6mathvm11g_registersE(%rip)
	movl	$_ZN6mathvm10stop_labelE, %edi
	movb	$0, 31(%rsp)
	leaq	-8(%rax,%rdx,8), %rax
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	call	_setjmp
	movzbl	31(%rsp), %eax
	testb	%al, %al
	je	.L127
	addq	$40, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 24
	popq	%rbx
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	ret
.L127:
	.cfi_restore_state
	movb	$1, 31(%rsp)
	.p2align 4,,10
	.p2align 3
.L97:
	movq	_ZN6mathvm11g_registersE(%rip), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, _ZN6mathvm11g_registersE(%rip)
	cmpb	$82, (%rax)
	ja	.L97
	movzbl	(%rax), %ecx
	jmp	*.L15(,%rcx,8)
	.section	.rodata
	.align 8
	.align 4
.L15:
	.quad	.L14
	.quad	.L16
	.quad	.L17
	.quad	.L18
	.quad	.L21
	.quad	.L21
	.quad	.L21
	.quad	.L22
	.quad	.L23
	.quad	.L24
	.quad	.L25
	.quad	.L26
	.quad	.L27
	.quad	.L28
	.quad	.L29
	.quad	.L30
	.quad	.L31
	.quad	.L32
	.quad	.L33
	.quad	.L34
	.quad	.L35
	.quad	.L36
	.quad	.L37
	.quad	.L38
	.quad	.L39
	.quad	.L40
	.quad	.L41
	.quad	.L42
	.quad	.L43
	.quad	.L44
	.quad	.L45
	.quad	.L46
	.quad	.L123
	.quad	.L48
	.quad	.L49
	.quad	.L50
	.quad	.L51
	.quad	.L56
	.quad	.L57
	.quad	.L58
	.quad	.L59
	.quad	.L56
	.quad	.L57
	.quad	.L58
	.quad	.L59
	.quad	.L60
	.quad	.L61
	.quad	.L62
	.quad	.L63
	.quad	.L68
	.quad	.L69
	.quad	.L70
	.quad	.L71
	.quad	.L68
	.quad	.L69
	.quad	.L70
	.quad	.L71
	.quad	.L72
	.quad	.L74
	.quad	.L74
	.quad	.L75
	.quad	.L77
	.quad	.L77
	.quad	.L78
	.quad	.L80
	.quad	.L80
	.quad	.L83
	.quad	.L82
	.quad	.L83
	.quad	.L84
	.quad	.L85
	.quad	.L86
	.quad	.L87
	.quad	.L88
	.quad	.L89
	.quad	.L90
	.quad	.L91
	.quad	.L92
	.quad	.L93
	.quad	.L94
	.quad	.L95
	.quad	.L97
	.quad	.L96
	.text
.L21:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	leaq	-8(%rax), %rdx
	movq	%rdx, _ZN6mathvm11g_registersE+8(%rip)
	movq	$0, -8(%rax)
	jmp	.L97
.L83:
	movzwl	1(%rax), %edi
	movswq	3(%rax), %rbx
	call	_ZN6mathvm9table_getEt
	movq	24(%rax), %rdx
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movsd	(%rax), %xmm0
	addq	$8, %rax
	movsd	%xmm0, 32(%rdx,%rbx,8)
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	addq	$4, _ZN6mathvm11g_registersE(%rip)
	jmp	.L97
.L80:
	movzwl	1(%rax), %edi
	movswq	3(%rax), %rbx
	movq	_ZN6mathvm11g_registersE+8(%rip), %rbp
	call	_ZN6mathvm9table_getEt
	movq	24(%rax), %rax
	movq	32(%rax,%rbx,8), %rax
	movq	%rax, -8(%rbp)
	addq	$4, _ZN6mathvm11g_registersE(%rip)
	subq	$8, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L77:
	call	_ZN6mathvm15get_topmost_ctxEv
	movq	_ZN6mathvm11g_registersE(%rip), %rcx
	movq	_ZN6mathvm11g_registersE+8(%rip), %rdx
	movswq	(%rcx), %rsi
	movq	(%rdx), %rdi
	addq	$2, %rcx
	addq	$8, %rdx
	movq	%rdi, 32(%rax,%rsi,8)
	movq	%rcx, _ZN6mathvm11g_registersE(%rip)
	movq	%rdx, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L74:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rbx
	leaq	-8(%rbx), %rax
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	call	_ZN6mathvm15get_topmost_ctxEv
	movq	_ZN6mathvm11g_registersE(%rip), %rdx
	movswq	(%rdx), %rcx
	addq	$2, %rdx
	movq	32(%rax,%rcx,8), %rax
	movq	%rax, -8(%rbx)
	movq	%rdx, _ZN6mathvm11g_registersE(%rip)
	jmp	.L97
.L71:
	call	_ZN6mathvm15get_topmost_ctxEv
	movq	_ZN6mathvm11g_registersE+8(%rip), %rdx
	movq	(%rdx), %rcx
	addq	$8, %rdx
	movq	%rcx, 56(%rax)
	movq	%rdx, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L70:
	call	_ZN6mathvm15get_topmost_ctxEv
	movq	_ZN6mathvm11g_registersE+8(%rip), %rdx
	movq	(%rdx), %rcx
	addq	$8, %rdx
	movq	%rcx, 48(%rax)
	movq	%rdx, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L69:
	call	_ZN6mathvm15get_topmost_ctxEv
	movq	_ZN6mathvm11g_registersE+8(%rip), %rdx
	movq	(%rdx), %rcx
	addq	$8, %rdx
	movq	%rcx, 40(%rax)
	movq	%rdx, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L68:
	call	_ZN6mathvm15get_topmost_ctxEv
	movq	_ZN6mathvm11g_registersE+8(%rip), %rdx
	movq	(%rdx), %rcx
	addq	$8, %rdx
	movq	%rcx, 32(%rax)
	movq	%rdx, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L59:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rbx
	leaq	-8(%rbx), %rax
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	call	_ZN6mathvm15get_topmost_ctxEv
	movq	56(%rax), %rax
	movq	%rax, -8(%rbx)
	jmp	.L97
.L58:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rbx
	leaq	-8(%rbx), %rax
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	call	_ZN6mathvm15get_topmost_ctxEv
	movq	48(%rax), %rax
	movq	%rax, -8(%rbx)
	jmp	.L97
.L57:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rbx
	leaq	-8(%rbx), %rax
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	call	_ZN6mathvm15get_topmost_ctxEv
	movq	40(%rax), %rax
	movq	%rax, -8(%rbx)
	jmp	.L97
.L56:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rbx
	leaq	-8(%rbx), %rax
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	call	_ZN6mathvm15get_topmost_ctxEv
	movq	32(%rax), %rax
	movq	%rax, -8(%rbx)
	jmp	.L97
.L82:
	movzwl	1(%rax), %edi
	movswq	3(%rax), %rbx
	movq	_ZN6mathvm11g_registersE+8(%rip), %rbp
	call	_ZN6mathvm9table_getEt
	movq	24(%rax), %rax
	movq	32(%rax,%rbx,8), %rax
	movq	%rax, 0(%rbp)
	addq	$4, _ZN6mathvm11g_registersE(%rip)
.L123:
	addq	$8, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L91:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rcx
	movzwl	1(%rax), %esi
	movq	(%rcx), %rbx
	cmpq	%rbx, 8(%rcx)
	jl	.L125
.L117:
	addq	$3, %rax
	movq	%rax, _ZN6mathvm11g_registersE(%rip)
.L118:
	addq	$16, %rcx
	movq	%rcx, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L92:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rcx
	movzwl	1(%rax), %esi
	movq	(%rcx), %rbx
	cmpq	%rbx, 8(%rcx)
	jg	.L117
.L125:
	movswq	%si, %rax
	addq	%rax, %rdx
	movq	%rdx, _ZN6mathvm11g_registersE(%rip)
	jmp	.L118
.L87:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rcx
	movzwl	1(%rax), %esi
	movq	8(%rcx), %rbx
	cmpq	%rbx, (%rcx)
	jne	.L125
	jmp	.L117
.L88:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rcx
	movzwl	1(%rax), %esi
	movq	8(%rcx), %rbx
	cmpq	%rbx, (%rcx)
	jne	.L117
	jmp	.L125
.L61:
	call	_ZN6mathvm15get_topmost_ctxEv
	movq	_ZN6mathvm11g_registersE+8(%rip), %rdx
	movsd	(%rdx), %xmm0
	addq	$8, %rdx
	movq	%rdx, _ZN6mathvm11g_registersE+8(%rip)
	movsd	%xmm0, 40(%rax)
	jmp	.L97
.L60:
	call	_ZN6mathvm15get_topmost_ctxEv
	movq	_ZN6mathvm11g_registersE+8(%rip), %rdx
	movsd	(%rdx), %xmm0
	addq	$8, %rdx
	movq	%rdx, _ZN6mathvm11g_registersE+8(%rip)
	movsd	%xmm0, 32(%rax)
	jmp	.L97
.L96:
	call	_ZN6mathvm15get_topmost_ctxEv
	movq	16(%rax), %rax
	movq	%rax, _ZN6mathvm11g_registersE(%rip)
	call	_ZN6mathvm7ctx_popEv
	jmp	.L97
.L18:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rdx
	addq	$3, %rax
	leaq	-8(%rdx), %rcx
	movq	%rcx, _ZN6mathvm11g_registersE+8(%rip)
	movswq	-2(%rax), %rcx
	movq	%rcx, -8(%rdx)
	movq	%rax, _ZN6mathvm11g_registersE(%rip)
	jmp	.L97
.L84:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movsd	(%rax), %xmm0
	movsd	8(%rax), %xmm1
	ucomisd	%xmm1, %xmm0
	ja	.L124
	xorl	%edx, %edx
	ucomisd	%xmm0, %xmm1
	setbe	%dl
	subq	$1, %rdx
	movq	%rdx, 8(%rax)
.L105:
	addq	$8, %rax
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L78:
	movzwl	1(%rax), %edi
	movswq	3(%rax), %rbx
	movq	_ZN6mathvm11g_registersE+8(%rip), %rbp
	call	_ZN6mathvm9table_getEt
	movq	24(%rax), %rax
	movsd	32(%rax,%rbx,8), %xmm0
	movsd	%xmm0, -8(%rbp)
	addq	$4, _ZN6mathvm11g_registersE(%rip)
	subq	$8, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L17:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rdx
	addq	$9, %rax
	leaq	-8(%rdx), %rcx
	movq	%rcx, _ZN6mathvm11g_registersE+8(%rip)
	movq	-8(%rax), %rcx
	movq	%rcx, -8(%rdx)
	movq	%rax, _ZN6mathvm11g_registersE(%rip)
	jmp	.L97
.L16:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rdx
	addq	$9, %rax
	leaq	-8(%rdx), %rcx
	movq	%rcx, _ZN6mathvm11g_registersE+8(%rip)
	movsd	-8(%rax), %xmm0
	movsd	%xmm0, -8(%rdx)
	movq	%rax, _ZN6mathvm11g_registersE(%rip)
	jmp	.L97
.L14:
	movq	stderr(%rip), %rcx
	movl	$14, %edx
	movl	$1, %esi
	movl	$.LC1, %edi
	call	fwrite
	jmp	.L97
.L95:
	movzwl	1(%rax), %edi
	call	_ZN6mathvm9table_getEt
	movq	%rax, %rbx
	movq	_ZN6mathvm11g_registersE(%rip), %rax
	movq	16(%rbx), %rdi
	leaq	2(%rax), %rsi
	call	_ZN6mathvm8ctx_pushEPKNS_12ctx_static_tEPKh
	leaq	40(%rbx), %rax
	movq	_ZN6mathvm11g_registersE+8(%rip), %rsi
	movq	%rax, _ZN6mathvm11g_registersE(%rip)
	movq	24(%rbx), %rax
	leaq	32(%rax), %rdi
	movq	16(%rbx), %rax
	movq	16(%rax), %rdx
	call	memcpy
	movq	16(%rbx), %rax
	movq	16(%rax), %rax
	andq	$-8, %rax
	addq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L85:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movq	8(%rax), %rsi
	cmpq	%rsi, (%rax)
	ja	.L124
	jae	.L106
	movq	$-1, 8(%rax)
	jmp	.L105
.L86:
	movswq	1(%rax), %rax
	addq	%rax, %rdx
	movq	%rdx, _ZN6mathvm11g_registersE(%rip)
	jmp	.L97
.L72:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rbx
	leaq	-8(%rbx), %rax
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	call	_ZN6mathvm15get_topmost_ctxEv
	movq	_ZN6mathvm11g_registersE(%rip), %rdx
	movswq	(%rdx), %rcx
	addq	$2, %rdx
	movsd	32(%rax,%rcx,8), %xmm0
	movsd	%xmm0, -8(%rbx)
	movq	%rdx, _ZN6mathvm11g_registersE(%rip)
	jmp	.L97
.L89:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rcx
	movzwl	1(%rax), %esi
	movq	(%rcx), %rbx
	cmpq	%rbx, 8(%rcx)
	jle	.L117
	jmp	.L125
.L90:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rcx
	movzwl	1(%rax), %esi
	movq	(%rcx), %rbx
	cmpq	%rbx, 8(%rcx)
	jl	.L117
	jmp	.L125
.L63:
	call	_ZN6mathvm15get_topmost_ctxEv
	movq	_ZN6mathvm11g_registersE+8(%rip), %rdx
	movsd	(%rdx), %xmm0
	addq	$8, %rdx
	movq	%rdx, _ZN6mathvm11g_registersE+8(%rip)
	movsd	%xmm0, 56(%rax)
	jmp	.L97
.L62:
	call	_ZN6mathvm15get_topmost_ctxEv
	movq	_ZN6mathvm11g_registersE+8(%rip), %rdx
	movsd	(%rdx), %xmm0
	addq	$8, %rdx
	movq	%rdx, _ZN6mathvm11g_registersE+8(%rip)
	movsd	%xmm0, 48(%rax)
	jmp	.L97
.L75:
	call	_ZN6mathvm15get_topmost_ctxEv
	movq	_ZN6mathvm11g_registersE(%rip), %rcx
	movq	_ZN6mathvm11g_registersE+8(%rip), %rdx
	movswq	(%rcx), %rsi
	movsd	(%rdx), %xmm0
	addq	$2, %rcx
	addq	$8, %rdx
	movq	%rcx, _ZN6mathvm11g_registersE(%rip)
	movq	%rdx, _ZN6mathvm11g_registersE+8(%rip)
	movsd	%xmm0, 32(%rax,%rsi,8)
	jmp	.L97
.L46:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movq	(%rax), %rdx
	movq	8(%rax), %rcx
	movq	%rdx, 8(%rax)
	movq	%rcx, (%rax)
	jmp	.L97
.L49:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rbx
	leaq	-8(%rbx), %rax
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	call	_ZN6mathvm15get_topmost_ctxEv
	movsd	40(%rax), %xmm0
	movsd	%xmm0, -8(%rbx)
	jmp	.L97
.L48:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rbx
	leaq	-8(%rbx), %rax
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	call	_ZN6mathvm15get_topmost_ctxEv
	movsd	32(%rax), %xmm0
	movsd	%xmm0, -8(%rbx)
	jmp	.L97
.L93:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movl	$.LC8, %esi
	movl	$1, %edi
	movq	(%rax), %rdx
	movl	$1, %eax
	movq	%rdx, 8(%rsp)
	movq	%rdx, %rcx
	movsd	8(%rsp), %xmm0
	call	__printf_chk
	jmp	.L97
.L94:
	xorl	%esi, %esi
	movl	$_ZN6mathvm10stop_labelE, %edi
	call	__longjmp_chk
.L51:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rbx
	leaq	-8(%rbx), %rax
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	call	_ZN6mathvm15get_topmost_ctxEv
	movsd	56(%rax), %xmm0
	movsd	%xmm0, -8(%rbx)
	jmp	.L97
.L50:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rbx
	leaq	-8(%rbx), %rax
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	call	_ZN6mathvm15get_topmost_ctxEv
	movsd	48(%rax), %xmm0
	movsd	%xmm0, -8(%rbx)
	jmp	.L97
.L45:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rbx
	movl	$10, %edx
	xorl	%esi, %esi
	movq	(%rbx), %rdi
	call	strtol
	movq	%rax, (%rbx)
	jmp	.L97
.L44:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	cvttsd2si	(%rax), %edx
	movslq	%edx, %rdx
	movq	%rdx, (%rax)
	jmp	.L97
.L43:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	cvtsi2sdq	(%rax), %xmm0
	movsd	%xmm0, (%rax)
	jmp	.L97
.L42:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movq	stdout(%rip), %rbx
	movswl	(%rax), %edi
	call	_ZN6mathvm17constant_pool_getEs
	movq	%rbx, %rsi
	movq	%rax, %rdi
	call	fputs
	addq	$8, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L41:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movl	$.LC7, %esi
	movl	$1, %edi
	leaq	8(%rax), %rdx
	movq	%rdx, _ZN6mathvm11g_registersE+8(%rip)
	movsd	(%rax), %xmm0
	movl	$1, %eax
	call	__printf_chk
	jmp	.L97
.L40:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movl	$.LC6, %esi
	movl	$1, %edi
	leaq	8(%rax), %rdx
	movq	%rdx, _ZN6mathvm11g_registersE+8(%rip)
	movq	(%rax), %rdx
	xorl	%eax, %eax
	call	__printf_chk
	jmp	.L97
.L39:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movq	(%rax), %rdx
	xorq	%rdx, 8(%rax)
	addq	$8, %rax
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L38:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movq	(%rax), %rdx
	andq	%rdx, 8(%rax)
	addq	$8, %rax
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L37:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movq	(%rax), %rdx
	orq	%rdx, 8(%rax)
	addq	$8, %rax
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L36:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	negq	(%rax)
	jmp	.L97
.L35:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movsd	(%rax), %xmm0
	xorpd	.LC5(%rip), %xmm0
	movsd	%xmm0, (%rax)
	jmp	.L97
.L34:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rcx
	movq	8(%rcx), %rax
	cqto
	idivq	(%rcx)
	addq	$8, %rcx
	movq	%rdx, (%rcx)
	movq	%rcx, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L33:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rcx
	movq	8(%rcx), %rax
	cqto
	idivq	(%rcx)
	addq	$8, %rcx
	movq	%rax, (%rcx)
	movq	%rcx, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L32:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movsd	8(%rax), %xmm0
	addq	$8, %rax
	divsd	-8(%rax), %xmm0
	movsd	%xmm0, (%rax)
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L31:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movq	8(%rax), %rdx
	addq	$8, %rax
	imulq	-8(%rax), %rdx
	movq	%rdx, (%rax)
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L30:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movsd	8(%rax), %xmm0
	addq	$8, %rax
	mulsd	-8(%rax), %xmm0
	movsd	%xmm0, (%rax)
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L29:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movq	(%rax), %rdx
	subq	%rdx, 8(%rax)
	addq	$8, %rax
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L28:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movsd	8(%rax), %xmm0
	addq	$8, %rax
	subsd	-8(%rax), %xmm0
	movsd	%xmm0, (%rax)
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L27:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movq	(%rax), %rdx
	addq	%rdx, 8(%rax)
	addq	$8, %rax
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L26:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movsd	8(%rax), %xmm0
	addq	$8, %rax
	addsd	-8(%rax), %xmm0
	movsd	%xmm0, (%rax)
	movq	%rax, _ZN6mathvm11g_registersE+8(%rip)
	jmp	.L97
.L25:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	leaq	-8(%rax), %rdx
	movq	%rdx, _ZN6mathvm11g_registersE+8(%rip)
	movq	$-1, -8(%rax)
	jmp	.L97
.L24:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movsd	.LC4(%rip), %xmm3
	leaq	-8(%rax), %rdx
	movq	%rdx, _ZN6mathvm11g_registersE+8(%rip)
	movsd	%xmm3, -8(%rax)
	jmp	.L97
.L23:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	leaq	-8(%rax), %rdx
	movq	%rdx, _ZN6mathvm11g_registersE+8(%rip)
	movq	$1, -8(%rax)
	jmp	.L97
.L22:
	movq	_ZN6mathvm11g_registersE+8(%rip), %rax
	movsd	.LC3(%rip), %xmm2
	leaq	-8(%rax), %rdx
	movq	%rdx, _ZN6mathvm11g_registersE+8(%rip)
	movsd	%xmm2, -8(%rax)
	jmp	.L97
.L124:
	movq	$1, 8(%rax)
	jmp	.L105
.L106:
	movq	$0, 8(%rax)
	jmp	.L105
	.cfi_endproc
.LFE1757:
	.size	_ZN6mathvm17interpreter_startEPh, .-_ZN6mathvm17interpreter_startEPh
	.section	.text.startup,"ax",@progbits
	.p2align 4,,15
	.type	_GLOBAL__sub_I__ZN6mathvm12g_calc_stackE, @function
_GLOBAL__sub_I__ZN6mathvm12g_calc_stackE:
.LFB2074:
	.cfi_startproc
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	movl	$_ZStL8__ioinit, %edi
	call	_ZNSt8ios_base4InitC1Ev
	movl	$__dso_handle, %edx
	movl	$_ZStL8__ioinit, %esi
	movl	$_ZNSt8ios_base4InitD1Ev, %edi
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	jmp	__cxa_atexit
	.cfi_endproc
.LFE2074:
	.size	_GLOBAL__sub_I__ZN6mathvm12g_calc_stackE, .-_GLOBAL__sub_I__ZN6mathvm12g_calc_stackE
	.section	.init_array,"aw"
	.align 8
	.quad	_GLOBAL__sub_I__ZN6mathvm12g_calc_stackE
	.globl	_ZN6mathvm10stop_labelE
	.bss
	.align 32
	.type	_ZN6mathvm10stop_labelE, @object
	.size	_ZN6mathvm10stop_labelE, 200
_ZN6mathvm10stop_labelE:
	.zero	200
	.globl	_ZN6mathvm11g_registersE
	.align 16
	.type	_ZN6mathvm11g_registersE, @object
	.size	_ZN6mathvm11g_registersE, 16
_ZN6mathvm11g_registersE:
	.zero	16
	.globl	_ZN6mathvm18g_calc_stack_countE
	.align 8
	.type	_ZN6mathvm18g_calc_stack_countE, @object
	.size	_ZN6mathvm18g_calc_stack_countE, 8
_ZN6mathvm18g_calc_stack_countE:
	.zero	8
	.globl	_ZN6mathvm12g_calc_stackE
	.align 8
	.type	_ZN6mathvm12g_calc_stackE, @object
	.size	_ZN6mathvm12g_calc_stackE, 8
_ZN6mathvm12g_calc_stackE:
	.zero	8
	.local	_ZStL8__ioinit
	.comm	_ZStL8__ioinit,1,1
	.section	.rodata.cst8,"aM",@progbits,8
	.align 8
.LC3:
	.long	0
	.long	1072693248
	.align 8
.LC4:
	.long	0
	.long	-1074790400
	.section	.rodata.cst16,"aM",@progbits,16
	.align 16
.LC5:
	.long	0
	.long	-2147483648
	.long	0
	.long	0
	.hidden	__dso_handle
	.ident	"GCC: (Ubuntu/Linaro 4.8.1-10ubuntu9) 4.8.1"
	.section	.note.GNU-stack,"",@progbits
