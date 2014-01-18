#include "MyCompiler.h"

#include <iostream>
#include <string>

#include <AsmJit/AsmJit.h>
using namespace AsmJit;

#include "OsSpecific.h"
#include "Interpreter.h"

#define FOR_IF_INSTR(DO) \
	DO(NE, ne) \
	DO(E,  e)  \
	DO(G,  g)  \
	DO(GE, ge) \
	DO(L,  l)  \
	DO(LE, le)


MyCompiler::MyCompiler(void) :cache_(0xFFFF), cantCompile_(0xFFFF)
{
}


MyCompiler::~MyCompiler(void)
{
}


void* MyCompiler::compile(const Bytecode_& bc, int16_t id, const vector<string>& literals, Interpreter* interp)
{
	if (cache_[id])
		return cache_[id];

	if (cantCompile_[id])
		return 0;

	ASSEMBLER a;
	vector<vector<AsmJit::Label> > labels(bc.length());

	if (!getLabels(bc, labels, a, id))
	{
		std::cerr<< "get labels failed" << id << std::endl;
		return 0;
	}

	const uint8_t* fst  = bc.getData();
	const uint8_t* inst = fst;
	const uint8_t* end  = bc.getData() + bc.length();
	
	AsmJit::Label start = a.newLabel();
	a.bind(start);

	while (inst < end)
	{
		if (labels[inst - fst].size())
			a.bind(labels[inst - fst][0]);

		switch(*inst)
		{
		case BC_STOP:
			a.push(0);
			a.call(reinterpret_cast<void*>(exit));
			++inst;
			break;

		case BC_LOADIVAR1:
			if (*(++inst) == BC_STOREIVAR2)
			{
				a.mov(rsp, rbp);
				++inst;
			}
			else
				a.push(rbp);
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
			a.and_(rsp, -16);
			a.call(reinterpret_cast<void*>(&dprint));
			a.add(rsp, 8);
			++inst;
			break;

		case BC_LOADIVAR2:
			if (*(inst + 1) == BC_STOREIVAR1)
			{
				a.mov(rbp, rsp);
				inst += 2;
			}
			else if (*(inst + 1) == BC_ILOAD && *(inst + 10) == BC_IADD)
			{
				a.add(rsp, *(uint64_t*)(inst + 2));
				inst += 12;
			}			
			else
			{
				a.push(rsp);
				++inst;
			}
			break;

		case BC_STOREIVAR2:
			a.pop(rsp);
			++inst;
			break;

		case BC_LOADCTXIVAR:
		case BC_LOADCTXDVAR:
			{
				int32_t offset = *(int32_t*)(++inst);
				inst += 4;
				 
				int8_t type = (offset >> 30) & 3;
					
				offset = offset << 16 >> 16;

				if (type == 0 || type == 3)
				{
					a.push(qword_ptr(rbp, offset));
				}
				else if (type == 1)
				{
					a.push(rbp);
					a.add(qword_ptr(rsp), offset);
				}
				else if (type == 2)
				{
					a.mov(r10, qword_ptr(rbp, offset));
					a.push(qword_ptr(r10));
				}
			}
			break;

		case BC_STORECTXIVAR:
		case BC_STORECTXDVAR:
			{
				int32_t offset = *(int32_t*)(++inst);
				inst += 4;
				 
				int8_t type = (offset >> 30) & 3;

				offset = offset << 16 >> 16;

				if (type == 0)
				{
					a.pop(qword_ptr(rbp, offset));
				}
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

		case BC_ILOADM1:
			a.push(-1l);
			++inst;
			break;

		case BC_ICMP:
			{
				// skip next LOAD0 - all logic move to if				
				// warning: it's need to check when compiler changed				
				inst += 2;
			}
			break;

#define IF_PROCESS(InstrSuf, CmdSuf) \
		case BC_IFICMP##InstrSuf: \
			{ \
				int16_t offset = *(int16_t*)(++inst); \
				a.pop(r11); \
				a.pop(r10); \
				a.cmp(r10, r11); \
				a.j##CmdSuf(labels[inst - fst + offset][0]); \
				inst += 2; \
			} \
			break;
			FOR_IF_INSTR(IF_PROCESS)
#undef IF_PROCESS

		case BC_ILOAD1:
			a.push(1l);
			++inst;
			break;

		case BC_DLOAD0:
			a.push(0.0);
			++inst;
			break;

		case BC_DLOADM1:
			{
				double val = -1.0;
				uint32_t* fst = (uint32_t*)(void*)(&val); 

				//HACK: asmjit push only 4 byte :( but rsp = rsp + 8 after push
				a.push(*fst);
				a.mov(dword_ptr(rsp, 4), *(fst + 1));
				++inst;
			}
			break;

		case BC_IADD:
			a.pop(r11);
			a.add(qword_ptr(rsp), r11);
			++inst;
			break;

		case BC_ISUB:
			a.pop(r11);
			a.sub(qword_ptr(rsp), r11);
			++inst;
			break;

		case BC_IMUL:
			a.pop(r10);
			a.imul(r10, qword_ptr(rsp));
			a.mov(qword_ptr(rsp), r10);
			++inst;
			break;

		case BC_IMOD:
			a.pop(r10);
			a.pop(rax);
			a.mov(rdx, 0);
			//a._emitInstruction(76); // cqo 
			a.idiv(r10);
			a.push(rdx);

			++inst;
			break;

		case BC_IDIV:
			a.pop(r10);
			a.pop(rax);
			a.mov(rdx, 0);
			//a._emitInstruction(76); // cqo 
			a.idiv(r10);
			a.push(rax);
			
			++inst;
			break;

		case BC_IAAND:
			a.pop(r11);
			a.and_(qword_ptr(rsp), r11);
			++inst;
			break;

		case BC_IAXOR:
			a.pop(r11);
			a.xor_(qword_ptr(rsp), r11);
			++inst;
			break;

		case BC_LOADIVAR0:
			if (*(inst + 1) == BC_STOREIVAR0)
				inst += 2;
			else
			{			
				a.push(rax);
				++inst;
			}
			break;

		case BC_STOREIVAR0:
			a.pop(rax);
			++inst;
			break;

		case BC_STOREDVAR0:
			a.movsd(xmm0, mmword_ptr(rsp));
			a.add(rsp, 8);
			++inst;
			break;

		case BC_LOADDVAR0:
			a.sub(rsp, 8);
			a.movsd(mmword_ptr(rsp), xmm0);
			++inst;
			break;

		case BC_RETURN:
			a.mov(r10, 0);
			a.ret();
			++inst;
			break;

		case BC_JA:
			{
				int16_t offset = *(int16_t*)(++inst);
				a.jmp(labels[inst - fst + offset][0]);
				inst += 2;
			}
			break;

		case BC_CALL:
			{
				int16_t callid = *(int16_t*)(++inst);
				inst += 2;

				if (callid != id)
				{
					if(cache_[callid] != 0 && cache_[callid] != (void*)(-1))
					{
						a.call(cache_[callid]);
					}
					else
					{
						cache_[id] = (void*)-1;
						void* f = compile(interp->bytecodes_[callid].second, callid, literals, interp);
						if(!f)
						{
							cache_[id] = 0;
							return cantCompile(BC_CALL, id, true);
						}
						a.mov(r10, (uint64_t)&cache_[callid]);
						a.call(qword_ptr(r10));
					}
				}
				else
					a.call(start);
			}
			break;

		case BC_CALLNATIVE:
			{
				int16_t callid = *(int16_t*)(++inst);
				inst += 2;
				string name = literals[callid];
				void* f = dlsym(RTLD_DEFAULT, name.c_str() + 1);
				if (f)
					a.call(f);
				else
					return cantCompile(BC_CALLNATIVE, id, true);
			}
			break;

		case BC_ILOAD:
		case BC_DLOAD:
			{
				uint32_t* fst = (uint32_t*)(++inst); 
				inst += 8;

				//HACK: asmjit push only 4 byte :( but rsp = rsp + 8 after push
				a.push(*fst);
				a.mov(dword_ptr(rsp, 4), *(fst + 1));
			}
			break;

		case BC_SLOAD:
			{
				int16_t id = *(int16_t*)(++inst);
				inst += 2;
				a.push((int64_t)literals[id].c_str());
			}
			break;

		case BC_D2I:
			{
				a.cvttsd2si(rax, mmword_ptr(rsp));
				a.mov(mmword_ptr(rsp), rax);
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
			}
			break;

		case BC_DSUB:
			{
				a.movsd(xmm0, mmword_ptr(rsp, 8));
				a.subsd(xmm0, mmword_ptr(rsp));
				a.add(rsp, 8);
				a.movsd(mmword_ptr(rsp), xmm0);
				++inst;
			}
			break;

		case BC_DMUL:
			{
				a.movsd(xmm0, mmword_ptr(rsp));
				a.mulsd(xmm0, mmword_ptr(rsp, 8));
				a.add(rsp, 8);
				a.movsd(mmword_ptr(rsp), xmm0);
				++inst;
			}
			break;

		case BC_DDIV:
			{
				a.movsd(xmm0, mmword_ptr(rsp, 8));
				a.divsd(xmm0, mmword_ptr(rsp));
				a.add(rsp, 8);
				a.movsd(mmword_ptr(rsp), xmm0);
				++inst;
			}
			break;

		default:
			return cantCompile((Instruction)(*inst), id, true);
		}
	}

	cache_[id] = a.make();
	//cerr << "on exit " << cache_[id] << " " << a.getError() << endl;
	return cache_[id];
}


void* MyCompiler::cantCompile(Instruction inst, int16_t id, bool isPermanent)
{
	cerr << "WARNING: unsupported instruction " << bytecodeName(inst) << " - can't compile function " << id << endl;
	cantCompile_[id] = isPermanent;
	return 0;
}


bool MyCompiler::getLabels(const Bytecode_& bc, vector<vector<AsmJit::Label> >& labels, ASSEMBLER& a, int16_t id)
{
	const uint8_t* fst  = bc.getData();
	const uint8_t* inst = fst;
	const uint8_t* end  = bc.getData() + bc.length();

	while (inst < end)
	{
		switch(*inst)
		{
		case BC_IPRINT:
		case BC_DPRINT:
		case BC_STOREIVAR1:
		case BC_LOADIVAR1:
		case BC_SPRINT:
		case BC_LOADIVAR2:
		case BC_STOREIVAR2:
		case BC_ILOAD0:
		case BC_ICMP:
		case BC_ILOAD1:
		case BC_ILOADM1:
		case BC_DLOAD0:
		case BC_DLOADM1:
		case BC_IADD:
		case BC_IMUL:
		case BC_IMOD:
		case BC_IDIV:
		case BC_IAAND:
		case BC_IAXOR:
		case BC_STOREIVAR0:
		case BC_STOREDVAR0:
		case BC_LOADDVAR0:
		case BC_LOADIVAR0:
		case BC_RETURN:
		case BC_ISUB:
		case BC_D2I:
		case BC_I2D:
		case BC_DADD:
		case BC_DSUB:
		case BC_DMUL:
		case BC_DDIV:
		case BC_STOP:
			++inst;
			break;

		case BC_CALL:
		case BC_CALLNATIVE:
		case BC_SLOAD:
			inst += 3;
			break;

		case BC_LOADCTXIVAR:
		case BC_LOADCTXDVAR:
		case BC_STORECTXIVAR:
		case BC_STORECTXDVAR:
			inst += 5;
			break;

		case BC_IFICMPNE:
		case BC_IFICMPGE:
		case BC_IFICMPG:
		case BC_IFICMPLE:
		case BC_IFICMPL:
		case BC_IFICMPE:
		case BC_JA:
			{
				int16_t offset = *(int16_t*)(++inst);
				if (labels[inst - fst + offset].empty())
				{
					AsmJit::Label dest = a.newLabel();
					labels[inst - fst + offset].push_back(dest);
				}
				inst += 2;
			}
			break;

		case BC_ILOAD:
		case BC_DLOAD:
			inst += 9;
			break;

		default:
			return cantCompile((Instruction)(*inst), id, true);
		}
	}
	return true;
}


void MyCompiler::iprint(int64_t val)
{
	printf(LONG_FORMAT, val);
}


void MyCompiler::sprint(const char* val)
{
	cout << string(val);
}


void MyCompiler::dprint(double val)
{
	printf("%e", val);
}

#undef FOR_IF_INSTR
