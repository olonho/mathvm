#ifndef BC_INTERPRETER_H_
#define BC_INTERPRETER_H_

#include <iostream>
#include <vector>
#include <map>
#include <cstdlib>

#include "mathvm.h"

using namespace mathvm;
using namespace std;


class BytecodeInterpreter
{
public:
    BytecodeInterpreter(Code* code_): code(code_){ }

    void run(BytecodeFunction* function);
    void loadDouble(double val);
    void loadInt(long long val);
    void loadString(long long val);

private:
    enum TYPE {d, i, s};
    Code* code;
    std::vector<pair<long long, TYPE> > stack;
};

void BytecodeInterpreter::loadDouble(double val)
{
    stack.push_back(make_pair(reinterpret_cast<long long&>(val), d));
}

void BytecodeInterpreter::loadInt(long long val)
{
    stack.push_back(make_pair(val, i));
}

void BytecodeInterpreter::loadString(long long val)
{
    stack.push_back(make_pair(val, s));
}


void BytecodeInterpreter::run(BytecodeFunction* function)
{
    std::map<long long, long long> ivars;
    std::map<long long, double> dvars;
    std::map<long long, long long> svars;

    Bytecode* bytecode = function->bytecode();
    uint32_t index = 0;
    while (index < bytecode->length())
    {
        Instruction insn = bytecode->getInsn(index);
        switch(insn)
        {
            case BC_DLOAD:
            {
                double val = bytecode->getDouble(index + 1);
                loadDouble(val);
                break;
            }
            case BC_ILOAD:
            {
                long long val = bytecode->getInt64(index + 1);
                loadInt(val);
                break;
            }
            case BC_SLOAD:
            {
                long long val = bytecode->getInt16(index + 1);
                loadString(val);
                break;
            }
            case BC_CALL:
            {
                long long id = bytecode->getInt16(index + 1);
                BytecodeFunction *fun = (BytecodeFunction *)code->functionById(id);
                run(fun);
                break;
            }
            case BC_RETURN:
            {
                return;
            }
            case BC_CALLNATIVE:
            {
                cout << "BC_CALLNATIVE is not implemented." << endl;
                break;
            }
            case BC_LOADDVAR:
            {
                long long id = bytecode->getInt16(index + 1);
                double value = dvars[id];
                loadDouble(value);
                break;
            }
            case BC_STOREDVAR:
            {
                long long id = bytecode->getInt16(index + 1);
                double value = reinterpret_cast<double&>(stack.back().first);
                stack.pop_back();
                dvars[id] = value;
                break;
            }
            case BC_LOADIVAR:
            {
                long long id = bytecode->getInt16(index + 1);
                long long value = ivars[id];
                stack.push_back(make_pair(value, i));
                break;
            }
            case BC_STOREIVAR:
            {
                long long id = bytecode->getInt16(index + 1);
                long long value = stack.back().first;
                stack.pop_back();
                ivars[id] = value;
                break;
            }
            case BC_LOADSVAR:
            {
                long long id = bytecode->getInt16(index + 1);
                long long value = svars[id];
                stack.push_back(make_pair(value, i));
                break;
            }
            case BC_STORESVAR:
            {
                long long id = bytecode->getInt16(index + 1);
                long long value = stack.back().first;
                stack.pop_back();
                svars[id] = value;
                break;
            }
            case BC_IFICMPNE:
            {
                int16_t offset = bytecode->getInt16(index + 1);
                long long val1 = stack.back().first;
                stack.pop_back();
                long long val2 = stack.back().first;
                stack.pop_back();
                if (val1 != val2)
                {
                    index += (offset + 1);
                    continue;
                }
                break;
            }
            case BC_IFICMPE:
            {
                int16_t offset = bytecode->getInt16(index + 1);
                long long val1 = stack.back().first;
                stack.pop_back();
                long long val2 = stack.back().first;
                stack.pop_back();
                if (val1 == val2)
                {
                    index += (offset + 1);
                    continue;
                }
                break;
            }
            case BC_IFICMPG:
            {
                int16_t offset = bytecode->getInt16(index + 1);
                long long val1 = stack.back().first;
                stack.pop_back();
                long long val2 = stack.back().first;
                stack.pop_back();
                if (val1 > val2)
                {
                    index += (offset + 1);
                    continue;
                }
                break;
            }
            case BC_IFICMPGE:
            {
                int16_t offset = bytecode->getInt16(index + 1);
                long long val1 = stack.back().first;
                stack.pop_back();
                long long val2 = stack.back().first;
                stack.pop_back();
                if (val1 >= val2)
                {
                    index += (offset + 1);
                    continue;
                }
                break;
            }
            case BC_IFICMPL:
            {
                int16_t offset = bytecode->getInt16(index + 1);
                long long val1 = stack.back().first;
                stack.pop_back();
                long long val2 = stack.back().first;
                stack.pop_back();
                if (val1 < val2)
                {
                    index += (offset + 1);
                    continue;
                }
                break;
            }
            case BC_IFICMPLE:
            {
                int16_t offset = bytecode->getInt16(index + 1);
                long long val1 = stack.back().first;
                stack.pop_back();
                long long val2 = stack.back().first;
                stack.pop_back();
                if (val1 <= val2)
                {
                    index += (offset + 1);
                    continue;
                }
                break;
            }
            case BC_JA:
            {
                int16_t offset = bytecode->getInt16(index + 1);
                index += (offset + 1);
                continue;
            }

            case BC_DLOAD0:
            {
                loadDouble(0.0);
                break;
            }
            case BC_ILOAD0:
            {
                loadInt(0);
                break;
            }
            case BC_SLOAD0:
            {
                loadString(code->makeStringConstant(""));
                break;
            }
            case BC_DLOAD1:
            {
                loadDouble(1.0);
                break;
            }
            case BC_ILOAD1:
            {
                loadInt(1);
                break;
            }
            case BC_DLOADM1:
            {
                loadDouble(-1.0);
                break;
            }
            case BC_ILOADM1:
            {
                loadInt(-1);
                break;
            }
            case BC_DADD:
            {
            	double val1;
            	if (stack.back().second == d) {
            		val1 = reinterpret_cast<double&>(stack.back().first);
            	} else {
            		val1 = stack.back().first;
            	}
                stack.pop_back();
            	double val2;
            	if (stack.back().second == d) {
            		val2 = reinterpret_cast<double&>(stack.back().first);
            	} else {
            		val2 = stack.back().first;
            	}
                stack.pop_back();
                loadDouble(val1 + val2);
                break;
            }
            case BC_IADD:
            {
                long long val1 = stack.back().first;
                stack.pop_back();
                long long val2 = stack.back().first;
                stack.pop_back();
                loadInt(val1 + val2);
                break;
            }
            case BC_DSUB:
            {
                double val1 = *reinterpret_cast<double*>(&stack.back().first);
                stack.pop_back();
                double val2 = *reinterpret_cast<double*>(&stack.back().first);
                stack.pop_back();
                loadDouble(val1 - val2);
                break;
            }
            case BC_ISUB:
            {
                long long val1 = stack.back().first;
                stack.pop_back();
                long long val2 = stack.back().first;
                stack.pop_back();
                loadInt(val1 - val2);
                break;
            }
            case BC_DMUL:
            {
                double val1 = *reinterpret_cast<double*>(&stack.back().first);
                stack.pop_back();
                double val2 = *reinterpret_cast<double*>(&stack.back().first);
                stack.pop_back();
                loadDouble(val1 * val2);
                break;
            }
            case BC_IMUL:
            {
                long long val1 = stack.back().first;
                stack.pop_back();
                long long val2 = stack.back().first;
                stack.pop_back();
                loadInt(val1 * val2);
                break;
            }
            case BC_DDIV:
            {
                double val1 = *reinterpret_cast<double*>(&stack.back().first);
                stack.pop_back();
                double val2 = *reinterpret_cast<double*>(&stack.back().first);
                stack.pop_back();
                loadDouble(val1 / val2);
                break;
            }
            case BC_IDIV:
            {
                long long val1 = stack.back().first;
                stack.pop_back();
                long long val2 = stack.back().first;
                stack.pop_back();
                loadInt(val1 / val2);
                break;
            }
            case BC_IMOD:
            {
                long long val1 = stack.back().first;
                stack.pop_back();
                long long val2 = stack.back().first;
                stack.pop_back();
                loadInt(val1 % val2);
                break;
            }
            case BC_DNEG:
            {
                double val = *reinterpret_cast<double*>(&stack.back().first);
                stack.pop_back();
                loadDouble((-1) * val);
                break;
            }
            case BC_INEG:
            {
                long long val = stack.back().first;
                stack.pop_back();
                loadInt((-1) * val);
                break;
            }
            case BC_IAOR:
            {
                long long val1 = stack.back().first;
                stack.pop_back();
                long long val2 = stack.back().first;
                stack.pop_back();
                loadInt(val1 | val2);
                break;
            }
            case BC_IAAND:
            {
            	long long val1 = stack.back().first;
            	stack.pop_back();
            	long long val2 = stack.back().first;
            	stack.pop_back();
            	loadInt(val1 & val2);
            	break;
            }
            case BC_IAXOR:
            {
            	long long val1 = stack.back().first;
            	stack.pop_back();
            	long long val2 = stack.back().first;
            	stack.pop_back();
            	loadInt(val1 ^ val2);
            	break;
            }
            case BC_IPRINT:
            {
                cout << stack.back().first;
                stack.pop_back();
                break;
            }
            case BC_DPRINT:
            {
                cout << reinterpret_cast<double&>(stack.back().first);
                stack.pop_back();
                break;
            }
            case BC_SPRINT:
            {
                cout << code->constantById(stack.back().first);
                stack.pop_back();
                break;
            }
            case BC_I2D:
            {
                stack.back().second = d;
                double value = stack.back().first;
                stack.back().first = reinterpret_cast<long long&>(value);
                break;
            }
            case BC_D2I:
            {
                stack.back().second = i;
                stack.back().first = reinterpret_cast<double&>(stack.back().first);
                break;
            }
            case BC_S2I:
            {
                long long val = atoi(code->constantById(stack.back().first).c_str());
                stack.back().first = val;
                stack.back().second = i;
                break;
            }
            case BC_SWAP:
            {
                pair<long long, TYPE> val = stack.at(stack.size() - 2);
                stack.at(stack.size() - 2) = stack.back();
                stack.back() = val;

                break;
            }
            case BC_POP:
            {
                stack.pop_back();
                break;
            }

            default:
                return;

        }

        //Go to next instruction
        int len = 0;
		#define INSN_LEN(b, d, l) if(BC_##b == insn) len = l;
        FOR_BYTECODES(INSN_LEN)
		#undef INSN_LEN

        index += len;
    }
};


#endif
