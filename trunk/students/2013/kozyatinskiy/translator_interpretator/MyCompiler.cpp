#include "MyCompiler.h"

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


MyCompiler::MyCompiler(void) :cache_(0xFFFF), cantCompile_(0xFFFF)
{
}


MyCompiler::~MyCompiler(void)
{
}


void* MyCompiler::compile(const Bytecode_& bc, int16_t id)
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

				if (type != 0)
					return cantCompile(BC_LOADCTXIVAR, id);

				offset = offset << 16 >> 16;

				a.push(qword_ptr(rbp, offset));
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
				a.ja(after);
				
				a.bind(lt);
				a.push(-1l);
				a.ja(after);
				
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

		case BC_ILOAD1:
			a.push(1l);
			++inst;
			break;

		case BC_IADD:
			a.pop(r10);
			a.pop(r11);
			a.add(r10, r11);
			a.push(r10);

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
