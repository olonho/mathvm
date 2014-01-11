#include "InstAsm.h"

#include <AsmJit/AsmJit.h>
using namespace AsmJit;

//#define WIN

#ifndef WIN
#define ASSEMBLER Assembler
#define IFST rdi
#define ISND rsi
#else
#define ASSEMBLER X86Assembler
#define IFST rcx
#define ISND rdx
#endif

void* asmIcmp()
{
	ASSEMBLER a;

	Label eq = a.newLabel();
	Label lt = a.newLabel();

	a.push(r12);
	a.push(r11);
	a.mov(r12, rsp);
	a.mov(rsp, IFST);
	
	a.pop(r11);
	a.cmp(r11, dword_ptr(rsp));
	a.jl(lt);
	a.je(eq);

	a.push(1l);
	a.mov(rax, rsp);
	a.mov(rsp, r12);
	a.pop(r11);
	a.pop(r12);
	a.ret();

	a.bind(lt);
	a.push(-1l);
	a.mov(rax, rsp);
	a.mov(rsp, r12);
	a.pop(r11);
	a.pop(r12);
	a.ret();

	a.bind(eq);
	a.push(0l);
	a.mov(rax, rsp);
	a.mov(rsp, r12);
	a.pop(r11);
	a.pop(r12);
	a.ret();

	return a.make();
}

void* asmILOAD0()
{
	ASSEMBLER a;

	a.push(r12);
	a.mov(r12, rsp);
	a.mov(rsp, IFST);

	a.push(0l);

	a.mov(rax, rsp);
	a.mov(rsp, r12);
	a.pop(r12);
	a.ret();

	return a.make();
}

void* testCall()
{
	ASSEMBLER a;

	Label start = a.newLabel();
	// for recursion
	a.bind(start);
	//LOADIVAR1
	a.push(rbp);
	//LOADIVAR2
	a.mov(rbp, rsp);

	// ICMP
	Label eq    = a.newLabel();
	Label lt    = a.newLabel();
	Label after = a.newLabel();

	a.cmp(qword_ptr(rbp, 24), 0l);
	a.jl(lt);
	a.je(eq);

	a.push(1l);
	a.ja(after);

	a.bind(lt);
	a.push(-1l);
	a.ja(after);

	a.bind(eq);
	a.push(0l);

	a.bind(after);

	//IFICMPNE 29
	Label dest = a.newLabel();
	a.pop(r10);
	a.cmp(r10, 0l);
	a.jne(dest);

	//LOADCTXIVAR 16,0
	a.push(qword_ptr(rbp, 16));

	//IADD
	a.pop(r10);
	a.add(r10, 1l);
	a.push(r10);

	a.pop(rax);
	
	a.mov(rsp, rbp);
	a.pop(rbp);
	a.ret();

	a.bind(dest);

	// 35: ICMP
	Label eq1    = a.newLabel();
	Label lt1    = a.newLabel();
	Label after1 = a.newLabel();

	a.cmp(qword_ptr(rbp, 16), 0);
	a.jl(lt1);
	a.je(eq1);

	a.push(1l);
	a.ja(after1);

	a.bind(lt1);
	a.push(-1l);
	a.ja(after1);

	a.bind(eq1);
	a.push(0l);

	a.bind(after1);

	// 37: IFICMPNE 72
	Label dest1 = a.newLabel();
	a.pop(r10);
	a.cmp(r10, 0l);
	a.jne(dest1);

	//40: LOADCTXIVAR 24,0
	a.push(qword_ptr(rbp, 24));

	//46: ISUB
	a.pop(r10);
	a.sub(r10, 1l);
	a.push(r10);

	//47: ILOAD1
	a.push(1l);
	a.call(start);
	
	// 51 - 62
	a.add(rsp, 16);

	a.mov(rsp, rbp);
	a.pop(rbp);
	a.ret();

	a.bind(dest1);

	//72: LOADCTXIVAR @24:0
	a.push(qword_ptr(rbp, 24));

	a.pop(r10);
	a.sub(r10, 1l);
	a.push(r10);

	// 79: LOADCTXIVAR @24:0
	a.push(qword_ptr(rbp, 24));
	
	a.push(qword_ptr(rbp, 16));
	
	a.pop(r10);
	a.sub(r10, 1l);
	a.push(r10);

	// 91: CALL *1
	a.call(start);
	
	// 94 - 105
	a.add(rsp, 16);
	
	a.push(rax);
	// 107: CALL *1
	a.call(start);

	// 110 - 121
	a.add(rsp, 16);

	a.mov(rsp, rbp);
	a.pop(rbp);
	a.ret();

	return a.make();
}
/*
	ASSEMBLER a;

	Label start = a.newLabel();
	// for recursion
	a.bind(start);
	//LOADIVAR1
	a.push(rbp);
	//LOADIVAR2
	a.mov(rbp, rsp);

	a.push(rsp);
	//STOREIVAR1
	a.pop(rbp);
	//LOADCTXIVAR with type = 0
	a.push(qword_ptr(rbp, 24));
	// ILOAD0
	a.push(0l);

	// ICMP
	Label eq    = a.newLabel();
	Label lt    = a.newLabel();
	Label after = a.newLabel();

	a.pop(r11);
	a.pop(r10);
	a.cmp(r10, r11);
	a.jl(lt);
	a.je(eq);

	a.push(1l);
	a.ja(after);

	a.bind(lt);
	a.push(-1l);
	a.ja(after);

	a.bind(eq);
	a.push(0l);

	a.bind(after);

	// ILOAD0
	a.push(0l);

	//IFICMPNE 29
	Label dest = a.newLabel();
	a.pop(r11);
	a.pop(r10);
	a.cmp(r10, r11);
	a.jne(dest);

	//LOADCTXIVAR 16,0
	a.push(qword_ptr(rbp, 16));

	//ILOAD1
	a.push(1l);

	//IADD
	a.pop(r10);
	a.pop(r11);
	a.add(r10, r11);
	a.push(r10);

	a.pop(rax);
	
	a.push(rbp);
	a.pop(rsp);
	a.pop(rbp);
	a.ret();

	a.ja(dest);

	a.bind(dest);

	// 29: LOADCTXIVAR 16,0
	a.push(qword_ptr(rbp, 16));
	a.push(0);

	// 35: ICMP
	Label eq1    = a.newLabel();
	Label lt1    = a.newLabel();
	Label after1 = a.newLabel();

	a.pop(r11);
	a.pop(r10);
	a.cmp(r10, r11);
	a.jl(lt1);
	a.je(eq1);

	a.push(1l);
	a.ja(after1);

	a.bind(lt1);
	a.push(-1l);
	a.ja(after1);

	a.bind(eq1);
	a.push(0l);

	a.bind(after1);

	// 36: ILOAD0
	a.push(0l);

	// 37: IFICMPNE 72
	Label dest1 = a.newLabel();
	a.pop(r11);
	a.pop(r10);
	a.cmp(r10, r11);
	a.jne(dest1);

	//40: LOADCTXIVAR 24,0
	a.push(qword_ptr(rbp, 24));
	a.push(1l);

	//46: ISUB
	a.pop(r11);
	a.pop(r10);
	a.sub(r10, r11);
	a.push(r10);

	//47: ILOAD1
	a.push(1l);
	a.call(start);
	
	// 51 - 62
	a.add(rsp, 16);

	// 63: LOADIVAR0
	a.push(rax);
	a.pop(rax);

	a.push(rbp);
	a.pop(rsp);
	a.pop(rbp);
	a.ret();

	a.ja(dest1);
	a.bind(dest1);

	//72: LOADCTXIVAR @24:0
	a.push(qword_ptr(rbp, 24));
	a.push(1l);

	a.pop(r11);
	a.pop(r10);
	a.sub(r10, r11);
	a.push(r10);

	// 79: LOADCTXIVAR @24:0
	a.push(qword_ptr(rbp, 24));
	a.push(qword_ptr(rbp, 16));
	a.push(1l);

	a.pop(r11);
	a.pop(r10);
	a.sub(r10, r11);
	a.push(r10);

	// 91: CALL *1
	a.call(start);
	
	// 94 - 105
	a.add(rsp, 16);
	
	a.push(rax);
	// 107: CALL *1
	a.call(start);

	// 110 - 121
	a.add(rsp, 16);

	a.push(rax);
	a.pop(rax);

	a.push(rbp);
	a.pop(rsp);
	a.pop(rbp);
	a.ret();

	return a.make();
	*/

#undef WIN
#undef IFST
#undef ISND
