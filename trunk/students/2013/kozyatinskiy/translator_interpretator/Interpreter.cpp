//#define DEBUG_DO(DO) DO
#define DEBUG_DO(DO) ;

#include "Interpreter.h"

#include "CompilerVisitor.h"

#include <cstdio>
#include <cmath>
#include <stdexcept>
#include <AsmJit/AsmJit.h>
using namespace AsmJit;

//#define WIN

#ifndef WIN
#include <dlfcn.h>
#define ASSEMBLER Assembler
#define IFST rdi
#define ISND rsi
#else
#define ASSEMBLER X86Assembler
#define IFST rcx
#define ISND rdx
#endif

#define FOR_COMMON_OP(DO) \
	DO(ADD, +) \
	DO(SUB, -) \
	DO(DIV, /) \
	DO(MUL, *)

// prefix suffix value
#define FOR_CONST_LOAD(DO) \
	DO(I, 0, static_cast<int64_t>(0)) \
	DO(D, 0, 0.0) \
	DO(S, 0, "") \
	DO(D, 1, 1.0) \
	DO(I, 1, static_cast<int64_t>(1)) \
	DO(D, M1, -1.0) \
	DO(I, M1, static_cast<int64_t>(-1))

#define FOR_INT_OP(DO) \
	DO(MOD, %) \
	DO(AOR, |) \
	DO(AAND, &) \
	DO(AXOR, ^)

#define FOR_VAR(DO) \
	DO(D, 0, double, deax) \
	DO(D, 1, double, dvar1) \
	DO(D, 2, double, dvar2) \
	DO(D, 3, double, dvar3) \
	DO(I, 0, int64_t, ieax) \
	DO(I, 3, int64_t, ieax) \
	DO(S, 0, char*, seax) \
	DO(S, 1, char*, svar1) \
	DO(S, 2, char*, svar2) \
	DO(S, 3, char*, svar3)

#define FOR_IF(DO) \
	DO(NE, !=) \
	DO(E, ==) \
	DO(G, >) \
	DO(GE, >=) \
	DO(L, <) \
	DO(LE, <=)

#define FOR_VALUE_TYPE(DO) \
	DO(D, double) \
	DO(I, int64_t)

#define FOR_PRINT(DO) \
	DO(D, double, cout << scientific << *top) \
	DO(I, int64_t, printf("%ld", *top)) \
	DO(S, char*, cout << string(*top))

const int StackSize = 16 * 1024 * 1024;

Interpreter::Interpreter():doubleCallWrapper(0), intCallWrapper(0), stringCallWrapper(0)
{
	stack_ = new int8_t[StackSize];
}

Interpreter::~Interpreter()
{
	delete[] stack_;
}

void Interpreter::execute(const vector<Bytecode_>& bytecodes, const vector<string>& literals)
{
	ASSEMBLER save;

	save.push(r12);
	save.mov(r12, rsp);
	save.mov(rsp, IFST);
	save.call(ISND);
	save.mov(rsp, r12);
	save.pop(r12);
	save.ret();

	doubleCallWrapper = reinterpret_cast<DoubleCall>(save.make());
	intCallWrapper    = reinterpret_cast<IntCall>(doubleCallWrapper);
	stringCallWrapper = reinterpret_cast<StringCall>(doubleCallWrapper);
	voidCallWrapper   = reinterpret_cast<VoidCall>(doubleCallWrapper);

	bytecodes_ = bytecodes;
	literals_  = literals;
	resolved_.resize(literals_.size());
	esp_ = stack_ + StackSize - 1024;
	eip_ = 0;
	ebp_ = esp_;
	call(0);
}

void Interpreter::call(int id)
{
	eip_ = bytecodes_[id].getData();

	while (true)
	{
		//cout << bytecodeName((Instruction)*(eip_)) << endl;
		switch (*eip_)
		{
		case BC_STOP:
			return;

		case BC_CALL:
			{
				int16_t id = *(int16_t*)(++eip_);
				eip_ += 2;
				uint64_t cur_eip = (uint64_t)eip_; 
				pushValue(cur_eip);
				eip_ = bytecodes_[id].getData();
			}
			break;
		case BC_CALLNATIVE:
			{
				int16_t id = *(int16_t*)(++eip_);
				eip_ += 2;
				pair<void*, VarType>& func = resolved_[id];
				if (!func.first)
				{
					string name = literals_[id];
					//void* f = static_cast<double(*)(double)>(&sqrt);
					void* f = dlsym(RTLD_DEFAULT, name.c_str() + 1);					
					if (!f)
						throw std::invalid_argument("can't resolve function");
					switch(*name.begin())
					{
						case 'v':
							func = make_pair(f, VT_VOID);
							break;
						case 'i':
							func = make_pair(f, VT_INT);
							break;
						case 'd':
							func = make_pair(f, VT_DOUBLE);
							break;
						case 's':
							func = make_pair(f, VT_STRING);
							break;
						default: break;
					}
				}
				switch(func.second)
				{
					case VT_VOID:
						voidCallWrapper(esp_, func.first);
						break;
					case VT_INT:
						ieax = intCallWrapper(esp_, func.first);
						break;
					case VT_DOUBLE:
						deax = doubleCallWrapper(esp_, func.first);
						break;
					case VT_STRING:
						seax = stringCallWrapper(esp_, func.first);
						break;
					default: break;
				}
			}
			break;

		case BC_D2I:
			{
				int64_t res = static_cast<int64_t>(*popValue<double>());
				pushValue(res);
				++eip_;
			}
			break;
		case BC_I2D:
			{
				double res = static_cast<double>(*popValue<int64_t>()); 
				pushValue(res);
				++eip_;
			}
			break;
		case BC_DLOAD:
			{
				eip_++;
				pushValue(*(double*)(eip_));
				eip_ += 8;
			}
			break;
		case BC_ILOAD:
			{
				eip_++;
				pushValue(*(int64_t*)(eip_));
				eip_ += 8;
			}
			break;
		case BC_SLOAD:
			{
				eip_++;
				pushValue(literals_[*(int16_t*)(eip_)].c_str());
				eip_ += 2;
			}
			break;
#define LOAD(Prefix, Suffix, Value) \
		case BC_##Prefix##LOAD##Suffix: \
			pushValue(Value); \
			eip_++; \
		break;
		FOR_CONST_LOAD(LOAD)
#undef LOAD

#define PROCESS_INT_OP(Op, Char) \
		case BC_I##Op: \
			{ \
				int64_t* top1 = popValue<int64_t>(); \
				int64_t* top2 = popValue<int64_t>(); \
				pushValue(*top2 Char *top1); \
				++eip_; \
			} \
			break;
#define PROCESS_DOUBLE_OP(Op, Char) \
		case BC_D##Op: \
			{ \
				double* top1 = popValue<double>(); \
				double* top2 = popValue<double>(); \
				pushValue(*top2 Char *top1); \
				++eip_; \
			} \
			break;
		FOR_COMMON_OP(PROCESS_INT_OP)
		FOR_COMMON_OP(PROCESS_DOUBLE_OP)
		FOR_INT_OP(PROCESS_INT_OP)
#undef PROCESS_INT_OP
#undef PROCESS_DOUBLE_OP

#define VAR(Prefix, Num, Type, Place) \
		case BC_LOAD##Prefix##VAR##Num: \
			{ \
				pushValue( Place ); \
				++eip_; \
			} \
			break; \
		case BC_STORE##Prefix##VAR##Num: \
			{ \
				Place = *popValue< Type >(); \
				++eip_; \
			} \
			break;
		FOR_VAR(VAR)
#undef VAR

		case BC_LOADIVAR1:
			pushValue<int64_t>((int64_t)(ebp_));
			++eip_;
			break;

		case BC_STOREIVAR1:
			ebp_ = (int8_t*)*(popValue<int64_t>());
			++eip_;
			break;

		case BC_LOADIVAR2:
			pushValue<int64_t>((int64_t)(esp_));
			++eip_;
			break;

		case BC_STOREIVAR2:
			esp_ = (int8_t*)*(popValue<int64_t>());
			++eip_;
			break;


#define IF(Suffix, Op) \
		case BC_IFICMP##Suffix: \
			{ \
				int64_t* top1 = popValue<int64_t>(); \
				int64_t* top2 = popValue<int64_t>(); \
				++eip_; \
				int16_t offset = *(int16_t*)(eip_); \
				if (*top1 Op *top2) \
					eip_ += offset; \
				else \
					eip_ += 2; \
			} \
			break;
		FOR_IF(IF)
#undef IF
		case BC_DNEG:
			pushValue(-1.0 * *popValue<double>());
			++eip_;
			break;

		case BC_INEG:
			pushValue(-1l * *popValue<int64_t>());
			++eip_;
			break;

#define PROCESS(Prefix, Type, Expr) \
		case BC_##Prefix##PRINT: \
			{ \
				Type* top = popValue<Type>(); \
				Expr; \
				++eip_; \
			} \
			break;
			FOR_PRINT(PROCESS)
#undef PROCESS
		case BC_DCMP:
			{
				double top1 = *popValue<double>();
				double top2 = *popValue<double>();
				pushValue<int64_t>(top1 > top2 ? 1 : top1 < top2 ? -1 : 0);
				++eip_;
			}
			break;
		case BC_ICMP:
			{
				int64_t top1 = *popValue<int64_t>();
				int64_t top2 = *popValue<int64_t>();
				pushValue<int64_t>(top1 > top2 ? 1 : top1 < top2 ? -1 : 0);
				++eip_;
			}
			break;
		case BC_JA:
			{
				++eip_;
				int16_t offset = *(int16_t*)(eip_);
				eip_ += offset;
			}
			break;
		case BC_RETURN:
			{
				//cout << "return" << endl;
				int64_t val = *popValue<int64_t>();
				eip_ = (uint8_t*)(val);
			}
			break;
#define VAR(Prefix, Type) \
		case BC_LOADCTX##Prefix##VAR: \
			{ \
				int32_t offset = *(int32_t*)(++eip_); \
				eip_ += 4; \
				 \
				int type = (offset >> 30) & 3; \
				/*printf("ebp: %d id: %d %d isPtr: %d\n", ebp_, offset, offset >> 15, isPtr1);*/ \
				offset = offset << 16 >> 16; \
				int16_t realOffset = offset; \
				\
				DEBUG_DO(cout << "load " << realOffset << ":" << type << ":" << ebp_ << endl;) \
				switch (type){ \
				/* load value from stack as value*/ \
				case 0: pushValue(*(Type*)(ebp_ + realOffset)); break; \
				/* load ptr of stack value */ \
				case 1:	pushValue((Type*)(ebp_ + realOffset)); break; \
				/* load value by ptr in stack*/ \
				case 2:	pushValue(**(Type**)(ebp_ + realOffset)); break; \
				/* load ptr by ptr */ \
				case 3:	pushValue(*(Type**)(ebp_ + realOffset)); break; \
				} \
			} \
			break; \
		case BC_STORECTX##Prefix##VAR: \
			{ \
				uint32_t offset = *(uint32_t*)(++eip_); \
				eip_ += 4; \
				 /*printf("store ebp: %d id: %d\n", ebp_, id);*/ \
				 \
				int type = (offset >> 30) & 3; \
				/*printf("ebp: %d id: %d %d isPtr: %d\n", ebp_, offset, offset >> 15, isPtr1);*/ \
				offset = offset << 16 >> 16; \
				int16_t realOffset = offset; \
				\
				DEBUG_DO(cout << "store " << realOffset << ":" << type << ":" << ebp_ << endl;) \
				if (type == 0) \
					memcpy(ebp_ + realOffset, popValue< Type >(), sizeof(Type)); \
				else \
					memcpy(*(Type**)(ebp_ + realOffset), popValue< Type >(), sizeof(Type)); \
			} \
			break;
	FOR_VALUE_TYPE(VAR)
#undef VAR
		default:
			{
				throw std::invalid_argument("unsupported instruction");
			}
			break;
		}
	}
}

#undef DEBUG_DO
