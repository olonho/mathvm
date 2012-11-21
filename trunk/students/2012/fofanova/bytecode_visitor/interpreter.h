#include <iostream>
#include <vector>
#include <map>

using namespace mathvm;
using namespace std;

int length(Instruction insn) {
    #define OPERATOR_LEN(b, d, l) if(BC_##b == insn) return l;
        FOR_BYTECODES(OPERATOR_LEN)
    #undef OPERATOR_LEN
    return 1;
}

class interpreter
{
private:
    enum type {i, d, s};

    Code* code;

    std::vector<int> calls;
    std::vector<pair<int, type> > stack;

    std::vector<int> ints;
    std::vector<double> doubles;
    std::vector<int> strs;

    std::map<int, int> ivars;
    std::map<int, int> dvars;
    std::map<int, int> svars;


public:
    interpreter(Code* code_): code(code_){ }

    void executeFunction(BytecodeFunction* function);
    void loadd(double val);
    void loadi(int val);
    void loads(int val);
};

void interpreter::loadd(double val)
{
    doubles.push_back(val);
    stack.push_back(make_pair(doubles.size() - 1, d));
}

void interpreter::loadi(int val)
{
    ints.push_back(val);
    stack.push_back(make_pair(ints.size() - 1, i));
}

void interpreter::loads(int val)
{
    strs.push_back(val);
    stack.push_back(make_pair(strs.size() - 1, s));
}


void interpreter::executeFunction(BytecodeFunction* function)
{
    Bytecode* bytecode = function->bytecode();
    uint32_t index = 0;
    cout << "len " << bytecode->length() << endl;
    while (index < bytecode->length())
    {
        Instruction insn = bytecode->getInsn(index);
        bool jump = false;
        switch(insn)
        {
        //one
            case BC_DLOAD:
            {
                double val = bytecode->getDouble(index + 1);
                loadd(val);
                break;
            }
            case BC_ILOAD:
            {
                int val = bytecode->getInt64(index + 1);
                loadi(val);
                break;
            }
            case BC_SLOAD:
            {
                int val = bytecode->getInt16(index + 1);
                loads(val);
                break;
            }
            case BC_CALL:
            {
                int id = bytecode->getInt16(index + 1);
                BytecodeFunction *fun = (BytecodeFunction *)code->functionById(id);
                cout << "calling " << fun->name() << " with id " << id << endl;  
                executeFunction(fun); 
                break;
            }
            case BC_RETURN: 
            {
                return;
            }
            case BC_CALLNATIVE:
            {
                cout << "callnat";
            }
            case BC_LOADDVAR:
            {
                int id = bytecode->getInt16(index + 1);
                double value = dvars[id];
                doubles.push_back(value);
                stack.push_back(make_pair(doubles.size() - 1, i));
                break;
            }
            case BC_STOREDVAR:
            {
                int id = bytecode->getInt16(index + 1);
                double value = doubles[stack.back().first];
                stack.pop_back();
                dvars[id] = value;
                break;
            }
            case BC_LOADIVAR:
            {
                int id = bytecode->getInt16(index + 1);
                int value = ivars[id];
                ints.push_back(value);
                stack.push_back(make_pair(ints.size() - 1, i));
                break;
            }
            case BC_STOREIVAR:
            {
                int id = bytecode->getInt16(index + 1);
                int value = ints[stack.back().first];
                stack.pop_back();
                ivars[id] = value;
                break;
            }
            case BC_LOADSVAR:
            {
                int id = bytecode->getInt16(index + 1);
                int value = svars[id];
                strs.push_back(value);
                stack.push_back(make_pair(strs.size() - 1, i));
                break;
            }
            case BC_STORESVAR:
            {
                int id = bytecode->getInt16(index + 1);
                int value = strs[stack.back().first];
                stack.pop_back();
                svars[id] = value;
                break;
            }
            case BC_IFICMPNE:
            {
                int16_t offset = bytecode->getInt16(index + 1);
                int val1 = ints[stack.back().first];
                stack.pop_back();
                int val2 = ints[stack.back().first];
                stack.pop_back();
                if (val1 != val2) 
                {
                    jump = true;
                    index += (offset + 1);
                }
                break;
            }
            case BC_IFICMPE:
            {
                int16_t offset = bytecode->getInt16(index + 1);
                int val1 = ints[stack.back().first];
                stack.pop_back();
                int val2 = ints[stack.back().first];
                stack.pop_back();
                if (val1 == val2) 
                {
                    jump = true;
                    index += (offset + 1);
                }
                break;
            }
            case BC_IFICMPG:
            {
                int16_t offset = bytecode->getInt16(index + 1);
                int val1 = ints[stack.back().first];
                stack.pop_back();
                int val2 = ints[stack.back().first];
                stack.pop_back();
                if (val1 > val2) 
                {
                    jump = true;
                    index += (offset + 1);
                }
                break;
            }
            case BC_IFICMPGE:
            {
                int16_t offset = bytecode->getInt16(index + 1);
                int val1 = ints[stack.back().first];
                stack.pop_back();
                int val2 = ints[stack.back().first];
                stack.pop_back();
                if (val1 >= val2) 
                {
                    jump = true;
                    index += (offset + 1);
                }
                break;
            }
            case BC_IFICMPL:
            {
                int16_t offset = bytecode->getInt16(index + 1);
                int val1 = ints[stack.back().first];
                stack.pop_back();
                int val2 = ints[stack.back().first];
                stack.pop_back();
                if (val1 < val2) 
                {
                    jump = true;
                    index += (offset + 1);
                }
                break;
            }
            case BC_IFICMPLE:
            {
                int16_t offset = bytecode->getInt16(index + 1);
                int val1 = ints[stack.back().first];
                stack.pop_back();
                int val2 = ints[stack.back().first];
                stack.pop_back();
                if (val1 <= val2) 
                {
                    jump = true;
                    index += (offset + 1);
                }
                break;
            }
            case BC_JA:
            {
                int16_t offset = bytecode->getInt16(index + 1);
                index += (offset + 1);
                jump = true;
                break;
            }
        // two
            case BC_LOADCTXDVAR:
            case BC_STORECTXDVAR:
            case BC_LOADCTXIVAR:
            case BC_STORECTXIVAR:
            case BC_LOADCTXSVAR:
            case BC_STORECTXSVAR:
            {
                cout << "x";
            }

         //none 
            case BC_DLOAD0:
            {
                loadd(0.0);
                break;
            }
            case BC_ILOAD0:
            {
                loadi(0);
                break;
            }
            case BC_SLOAD0:
            {
                loads(code->makeStringConstant(""));
                break;
            }
            case BC_DLOAD1:
            {
                loadd(1.0);
                break;
            }
            case BC_ILOAD1:
            {
                loadi(1);
                break;
            }
            case BC_DLOADM1:
            {
                loadd(-1.0);
                break;
            }
            case BC_ILOADM1:
            {
                loadi(-1);
                break;
            }
            case BC_DADD:
            {
                double val1 = doubles[stack.back().first];
                stack.pop_back();
                double val2 = doubles[stack.back().first];
                stack.pop_back();
                loadd(val1 + val2);
                break;
            }
            case BC_IADD:
            {
                int val1 = ints[stack.back().first];
                stack.pop_back();
                int val2 = ints[stack.back().first];
                stack.pop_back();
                loadi(val1 + val2);
                break;
            }
            case BC_DSUB:
            {
                double val1 = doubles[stack.back().first];
                stack.pop_back();
                double val2 = doubles[stack.back().first];
                stack.pop_back();
                loadd(val1 - val2);
                break;
            }
            case BC_ISUB:
            {
                int val1 = ints[stack.back().first];
                stack.pop_back();
                int val2 = ints[stack.back().first];
                stack.pop_back();
                loadi(val1 - val2);
                break;
            }
            case BC_DMUL:
            {
                double val1 = doubles[stack.back().first];
                stack.pop_back();
                double val2 = doubles[stack.back().first];
                stack.pop_back();
                loadd(val1 * val2);
                break;
            }
            case BC_IMUL:
            {
                int val1 = ints[stack.back().first];
                stack.pop_back();
                int val2 = ints[stack.back().first];
                stack.pop_back();
                loadi(val1 * val2);
                break;
            }
            case BC_DDIV:
            {
                double val1 = doubles[stack.back().first];
                stack.pop_back();
                double val2 = doubles[stack.back().first];
                stack.pop_back();
                loadd(val1 / val2);
                break;
            }
            case BC_IDIV:
            {
                int val1 = ints[stack.back().first];
                stack.pop_back();
                int val2 = ints[stack.back().first];
                stack.pop_back();
                loadi(val1 / val2);
                break;
            }
            case BC_IMOD:
            {
                int val1 = ints[stack.back().first];
                stack.pop_back();
                int val2 = ints[stack.back().first];
                stack.pop_back();
                loadi(val1 % val2);
                break;
            }
            case BC_DNEG:
            {
                double val = doubles[stack.back().first];
                stack.pop_back();
                loadd((-1) * val);
                break;
            }
            case BC_INEG:
            {
                int val = ints[stack.back().first];
                stack.pop_back();
                loadi((-1) * val);
                break;
            }
            case BC_IPRINT:
            {
                cout << ints[stack.back().first]; 
                stack.pop_back();
                break;
            }
            case BC_DPRINT:
            {
                cout << doubles[stack.back().first]; 
                stack.pop_back();
                break;
            }
            case BC_SPRINT:
            {
                cout << code->constantById(strs[stack.back().first]); 
                stack.pop_back();
                break;
            }
            case BC_I2D:
            {
                stack.back().second = d;
                doubles.push_back(ints[stack.back().first]);
                stack.back().first = doubles.size() - 1;
                break;
            }
            case BC_D2I:
            {
                stack.back().second = i;
                ints.push_back(doubles[stack.back().first]);
                stack.back().first = ints.size() - 1;
                break;
            }
            case BC_S2I:
            {
                stack.back().first = atoi(code->constantById(strs[stack.back().first]).c_str());  
                stack.back().second = i;
                break;
            }
            case BC_SWAP:
            {
                pair<int, type> val1 = stack.back();
                stack.pop_back();
                pair<int, type> val2 = stack.back();
                stack.pop_back();

                stack.push_back(val1);
                stack.push_back(val2);

                break;

            }
            case BC_POP:
            {
                stack.pop_back();
                break;
            }
            case BC_LOADDVAR0:
            case BC_LOADDVAR1:
            case BC_LOADDVAR2:
            case BC_LOADDVAR3:
            case BC_LOADIVAR0:
            case BC_LOADIVAR1:
            case BC_LOADIVAR2:
            case BC_LOADIVAR3:
            case BC_LOADSVAR0:
            case BC_LOADSVAR1:
            case BC_LOADSVAR2:
            case BC_LOADSVAR3:
            case BC_STOREDVAR0:
            case BC_STOREDVAR1:
            case BC_STOREDVAR2:
            case BC_STOREDVAR3:
            case BC_STOREIVAR0:
            case BC_STOREIVAR1:
            case BC_STOREIVAR2:
            case BC_STOREIVAR3:
            case BC_STORESVAR0:
            case BC_STORESVAR1:
            case BC_STORESVAR2:
            case BC_STORESVAR3:
            default:
                return;  

        }
        if (!jump)
        {
            index += (length(insn));
        }
    }
}