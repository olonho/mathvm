#include "InstAsm.h"

#include <AsmJit/AsmJit.h>
using namespace AsmJit;

#include "OsSpecific.h"

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

void* asmNCALL()
{
	ASSEMBLER ncall;

	ncall.push(r12);
	ncall.push(r13);
	ncall.mov(r12, rsp);
	ncall.mov(r13, rbp);
	ncall.mov(rsp, IFST);
	ncall.mov(rbp, ISND);
	ncall.call(ITHD);
	ncall.mov(rsp, r12);
	ncall.mov(rbp, r13);
	ncall.pop(r13);
	ncall.pop(r12);
	ncall.ret();

	return ncall.make();
}

void* getRIP()
{
	static void* func = 0;
	if (!func)
	{
		ASSEMBLER a;

		a.mov(r15, qword_ptr(rsp));
		a.ret();

		func = a.make();
	}
	return func;
}

void* asmCALL()
{
	// from code - type f(void** rsp, void** rbp, void** rip, void* f);

	// save rsp, rbp
	// call function
	// restore rsp, rbp
	// restore interpretator esp, ebp, eip (only if it's return on unsupported instruction)
	// pass restores params by volatile registers r9, r10, r11 on windows
	ASSEMBLER call;

	// save r12, r13
	call.push(r12);
	call.push(r13);
	call.push(r14);
	call.push(r15);
	call.push(rsi);
	call.push(rbp);

	// save rsp
	call.mov(r12, rsp);

	call.mov(r13, IFST);
	call.mov(r14, ISND);
	call.mov(rsi, ITHD);

	// call compiled function
	call.mov(rsp, qword_ptr(r13));
	call.mov(rbp, qword_ptr(r14));

	call.call(getRIP());
	call.add(r15, 7);

	call.call(IFTH);

	// restore rsp, rbp, rip of interpreter
	call.mov(qword_ptr(r13), rsp);
	call.mov(qword_ptr(r14), rbp);
	call.mov(qword_ptr(rsi), r10);
	
	// restore rsp
	call.mov(rsp, r12);
	
	// restore r12, r13
	call.pop(rbp);
	call.pop(rsi);
	call.pop(r15);
	call.pop(r14);
	call.pop(r13);
	call.pop(r12);
	call.ret();

	return call.make();
}

void* asmRet()
{
	static void* func = 0;
	if (!func)
	{
		ASSEMBLER a;

		a.pop(r8);
		a.mov(rsp, IFST);
		a.mov(rbp, ISND);
		a.push(r8);
		a.ret();

		func = a.make();
	}
	return func;
}

#undef IFST
#undef ISND
