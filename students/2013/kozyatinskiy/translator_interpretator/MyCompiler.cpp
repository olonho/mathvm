#include "MyCompiler.h"

#include <iostream>
#include <string>

#include <AsmJit/AsmJit.h>
using namespace AsmJit;

//#define WIN

#ifndef WIN
#define ASSEMBLER Assembler
#define IFST rdi
#define ISND rsi
#define DFST xmm0
#else
#define ASSEMBLER X86Assembler
#define IFST rcx
#define ISND rdx
#define DFST xmm0
#endif


MyCompiler::MyCompiler(void) :cache_(0xFFFF), cantCompile_(0xFFFF)
{
}


MyCompiler::~MyCompiler(void)
{
}


void* MyCompiler::compile(const Bytecode_& bc, int16_t id, const vector<string>& literals)
{
	if (cache_[id])
		return cache_[id];

	if (cantCompile_[id])
		return 0;

	vector<vector<AsmJit::Label> > labels(bc.length());

	const uint8_t* fst  = bc.getData();
	const uint8_t* inst = fst;
	const uint8_t* end  = bc.getData() + bc.length();

	ASSEMBLER a;

	AsmJit::Label start = a.newLabel();
	a.bind(start);

	//vector<int16_t> lastLocal;
	//vector<int64_t> lastConstants;

	while (inst < end)
	{
		for (size_t i = 0; i < labels[inst - fst].size(); ++i)
			a.bind(labels[inst - fst][i]);

		switch(*inst)
		{
		case BC_LOADIVAR1:
			a.push(rbp);
			++inst;
			break;

		case BC_STOREIVAR1:
			a.pop(rbp);
			++inst;
			break;

		case BC_IPRINT:
			a.mov(IFST, dword_ptr(rsp));
			a.call(reinterpret_cast<void*>(&iprint));
			a.add(rsp, 8);
			++inst;
			break;

		case BC_SPRINT:
			a.mov(IFST, dword_ptr(rsp));
			a.call(reinterpret_cast<void*>(&sprint));
			a.add(rsp, 8);
			++inst;
			break;

		case BC_DPRINT:
			a.movsd(xmm0, mmword_ptr(rsp));
			a.call(reinterpret_cast<void*>(&dprint));
			a.add(rsp, 8);
			++inst;
			break;

		case BC_LOADIVAR2:
			if (*(++inst) == BC_STOREIVAR1)
			{
				a.mov(rbp, rsp);
				++inst;
			}
			else
				a.push(rsp);
			break;

		case BC_STOREIVAR2:
			a.pop(rsp);
			++inst;
			break;

		case BC_LOADCTXIVAR:
			{
				int32_t offset = *(int32_t*)(++inst);
				inst += 4;
				 
				int type = (offset >> 30) & 3;

				//if (type != 0)
				//	return cantCompile(BC_LOADCTXIVAR, id);

				offset = offset << 16 >> 16;

				if (type == 0 || type == 3)
					a.push(qword_ptr(rbp, offset));
				else if (type == 1)
				{
					a.push(rbp);
					a.add(rsp, offset);
				}else if (type == 2)
				{
					a.mov(r10, qword_ptr(rbp, offset));
					a.push(qword_ptr(r10));
				}
			}
			break;

		case BC_STORECTXIVAR:
			{
				int32_t offset = *(int32_t*)(++inst);
				inst += 4;
				 
				int type = (offset >> 30) & 3;

				//if (type != 0)
				//	return cantCompile(BC_STORECTXIVAR, id);

				offset = offset << 16 >> 16;

				if (type == 0)
					a.pop(qword_ptr(rbp, offset));
				else
				{
					a.mov(r10, qword_ptr(rbp, offset));
					a.pop(qword_ptr(r10));
				}
			}
			break;

		case BC_ILOAD0:
			a.push(0l);
			++inst;
			break;

		case BC_ICMP:
			{
				AsmJit::Label after = a.newLabel();
				AsmJit::Label lt = a.newLabel();
				AsmJit::Label eq = a.newLabel();

				a.pop(r11);
				a.pop(r10);
				a.cmp(r10, r11);
				a.jl(lt);
				a.je(eq);
				
				a.push(1l);
				a.jmp(after);
				
				a.bind(lt);
				a.push(-1l);
				a.jmp(after);
				
				a.bind(eq);
				a.push(0l);
				
				a.bind(after);

				++inst;
			}
			break;

		case BC_IFICMPNE:
			{
				int16_t offset = *(int16_t*)(++inst);
				if (offset < 0)
					return cantCompile(BC_IFICMPNE, id);

				AsmJit::Label dest = a.newLabel();
				a.pop(r11);
				a.pop(r10);
				a.cmp(r10, r11);
				a.jne(dest);

				labels[inst - fst + offset].push_back(dest);
				inst += 2;
			}
			break;

		case BC_IFICMPGE:
			{
				int16_t offset = *(int16_t*)(++inst);
				if (offset < 0)
					return cantCompile(BC_IFICMPGE, id);

				AsmJit::Label dest = a.newLabel();
				a.pop(r11);
				a.pop(r10);
				a.cmp(r10, r11);
				a.jge(dest);

				labels[inst - fst + offset].push_back(dest);
				inst += 2;
			}
			break;

		case BC_IFICMPG:
			{
				int16_t offset = *(int16_t*)(++inst);
				if (offset < 0)
					return cantCompile(BC_IFICMPG, id);

				AsmJit::Label dest = a.newLabel();
				a.pop(r11);
				a.pop(r10);
				a.cmp(r10, r11);
				a.jg(dest);

				labels[inst - fst + offset].push_back(dest);
				inst += 2;
			}
			break;

		case BC_IFICMPLE:
			{
				int16_t offset = *(int16_t*)(++inst);
				if (offset < 0)
					return cantCompile(BC_IFICMPLE, id);

				AsmJit::Label dest = a.newLabel();
				a.pop(r11);
				a.pop(r10);
				a.cmp(r10, r11);
				a.jle(dest);

				labels[inst - fst + offset].push_back(dest);
				inst += 2;
			}
			break;

		case BC_ILOAD1:
			a.push(1l);
			++inst;
			break;

		case BC_IADD:
			a.pop(r10);
			a.add(r10, qword_ptr(rsp));
			a.mov(qword_ptr(rsp), r10);

			++inst;
			break;

		case BC_STOREIVAR0:
			a.pop(rax);
			++inst;
			break;

		case BC_LOADIVAR0:
			a.push(rax);
			++inst;
			break;


		case BC_RETURN:
			a.ret();
			++inst;
			break;

		case BC_JA:
			{
				int16_t offset = *(int16_t*)(++inst);
				if (offset < 0)
					return cantCompile(BC_JA, id);

				AsmJit::Label dest = a.newLabel();
				labels[inst - fst + offset].push_back(dest);

				a.jmp(dest);
				inst += 2;
			}
			break;

		case BC_ISUB:
			a.pop(r11);
			a.pop(r10);
			a.sub(r10, r11);
			a.push(r10);
			++inst;
			break;

		case BC_CALL:
			{
				int16_t callid = *(int16_t*)(++inst);
				inst += 2;

				if (callid != id)
					return cantCompile(BC_CALL, id);

				a.call(start);
			}
			break;

		case BC_ILOAD:
			{
				int64_t val = *(int64_t*)(++inst);
				inst += 8;
				a.push(val);
			}
			break;

		case BC_SLOAD:
			{
				int64_t id = *(int16_t*)(++inst);
				inst += 2;
				a.push((int64_t)literals[id].c_str());
			}
			break;

		case BC_DLOAD:
			{
				uint32_t m[2];
				m[1] = *(uint32_t*)(inst);
				inst += 4;
				m[0] = *(uint32_t*)(inst);
				inst += 4;

				//HACK: asmjit push only 4 byte :( but rsp = rsp + 8 after push
				a.push(m[0]);
				a.mov(dword_ptr(rsp, 4), m[1]);
			}
			break;

		case BC_D2I:
			{
				a.cvttsd2si(rax, mmword_ptr(rsp));
				a.add(rsp, 8);
				a.pop(rax);
				++inst;
			}
			break;

		case BC_I2D:
			{
				a.cvtsi2sd(xmm0, qword_ptr(rsp));
				a.movsd(qword_ptr(rsp), xmm0);
				++inst;
			}
			break;

		case BC_DADD:
			{
				a.movsd(xmm0, mmword_ptr(rsp));
				a.addsd(xmm0, mmword_ptr(rsp, 8));
				a.add(rsp, 8);
				a.movsd(mmword_ptr(rsp), xmm0);
				++inst;
				//a.push(xmm1);
			}
			break;

		default:
			return cantCompile((Instruction)*inst, id);
		}
	}

	return a.make();
}


void* MyCompiler::cantCompile(Instruction inst, int16_t id)
{
	cout << "WARNING: unsupported instruction " << bytecodeName(inst) << " - can't compile function " << id << endl;
	cantCompile_[id] = true;
	return 0;
}


void MyCompiler::iprint(int64_t val)
{
	printf("%ld", val);
}


void MyCompiler::sprint(const char* val)
{
	cout << string(val);
}


void MyCompiler::dprint(double val)
{
	cout << scientific << val;
}
