#include <iostream>
#include <vector>
#include <map>
#include "AsmJit/Compiler.h"
#include "AsmJit/AsmJit.h"

using namespace mathvm;
using namespace std;
using namespace AsmJit;

int length(Instruction insn) {
    #define OPERATOR_LEN(b, d, l) if(BC_##b == insn) return l;
        FOR_BYTECODES(OPERATOR_LEN)
    #undef OPERATOR_LEN
    return 1;
}

string str(Instruction insn) {
    #define OPERATOR_STR(b, d, l) if(BC_##b == insn) return #b;
        FOR_BYTECODES(OPERATOR_STR)
    #undef OPERATOR_STr
    return "BC_STOP";
}

class interpreter
{
private:
    enum type {i, d, s};

    Code* code;
    Compiler compiler;

    std::vector<int> calls;
    std::vector<VarType> stackTypes;

    std::vector<int> ints;
    std::vector<double> doubles;
    std::vector<int> strs;

    std::map<int, GPVar> ivars;
    std::map<int, GPVar> dvars;
    std::map<int, GPVar> svars;

    void* main;
    map<uint32_t, AsmJit::Label*> labels;


public:
    interpreter(Code* code_): code(code_){ }

    void generateFunction(BytecodeFunction* function, int funcIndex);
    void generateGPArithmetics(Instruction insn, Compiler& compiler);
    void generateXMMArithmetics(Instruction insn, Compiler& compiler);
    void generateCompare(Instruction insn, Bytecode* bytecode, int index, Compiler& compiler);
    void generate(BytecodeFunction* function);
    void loadd(double val, Compiler& compiler);
    void loadi(int val, Compiler& compiler);
    void loads(int val, Compiler& compiler);
    void pop(XMMVar& var, Compiler& compiler);
    void push(XMMVar& var, Compiler& compiler);
    void pop(GPVar& var, Compiler& compiler);
    void push(GPVar& var, Compiler& compiler);
};

void printi(sysint_t value) {
    cout << value;
}

void printd(double value) {
    cout << value;
}

void prints(const char* value) {
    cout << value;
}

int idiv(sysint_t up, sysint_t low) {
    return up / low;
}

int imod(sysint_t up, sysint_t low) {
    return up % low;
}

int d2i(double val) {
    return (sysint_t)val;
}

double i2d(sysint_t val) {
    return (double)val;
}

sysint_t s2i(const char* val) {
    return atoi(val);
}

void interpreter::loadd(double val, Compiler& compiler)
{
    stackTypes.push_back(VT_DOUBLE);
    GPVar var(compiler.newGP());
    compiler.mov(var, imm(*((sysint_t*)&val)));
    compiler.push(var);
    compiler.unuse(var);
}

void interpreter::loadi(int val, Compiler& compiler)
{
    stackTypes.push_back(VT_INT);
    compiler.push(imm(val));
}

void interpreter::loads(int val, Compiler& compiler)
{
    stackTypes.push_back(VT_STRING);
    const char* str = code->constantById(val).c_str();
    GPVar var = compiler.newGP();
    compiler.mov(var, imm((sysint_t)str));
    compiler.push(var);
    compiler.unuse(var);
}

void interpreter::generate(BytecodeFunction* function)
{
    generateFunction(function, 0);
    if(main) 
    {
        function_cast<void (*)()>(main)();
    }
    else
    {
        cout << "Fatal error during generating!" << endl;
    }
}

void interpreter::pop(XMMVar& xmmvar, Compiler& compiler)
{
    GPVar gpvar(compiler.newGP());
    compiler.pop(gpvar);
    compiler.movq(xmmvar, gpvar);
    compiler.unuse(gpvar);
}

void interpreter::push(XMMVar& var, Compiler& compiler)
{
    GPVar gpvar(compiler.newGP());
    compiler.movq(gpvar, var);
    compiler.push(gpvar);
    compiler.unuse(gpvar);
}

void interpreter::pop(GPVar& gpvar, Compiler& compiler)
{
    compiler.pop(gpvar);
}

void interpreter::push(GPVar& gpvar, Compiler& compiler)
{
    compiler.push(gpvar);
}

void interpreter::generateCompare(Instruction insn, Bytecode* bytecode, int index, Compiler& compiler)
{

    int16_t offset = bytecode->getInt16(index + 1);
    if (labels.count(index + offset + 1) == 0)
    {
        AsmJit::Label* label = new AsmJit::Label(compiler.newLabel());
        labels[index + offset + 1] = label;
    }
    GPVar var2(compiler.newGP());
    GPVar var1(compiler.newGP());
    pop(var2, compiler);
    pop(var1, compiler);
    compiler.cmp(var2, var1);
    switch (insn)
    {
        case BC_IFICMPNE:
        {
            compiler.jne(*labels[index + offset + 1]);
            break;
        }
        case BC_IFICMPE:
        {
            compiler.je(*labels[index + offset + 1]);
            break;
        }
        case BC_IFICMPG:
        {
            compiler.jg(*labels[index + offset + 1]);
            break;
        }
        case BC_IFICMPGE:
        {
            compiler.jge(*labels[index + offset + 1]);
            break;
        }
        case BC_IFICMPL:
        {
            compiler.jl(*labels[index + offset + 1]);
            break;
        }
        case BC_IFICMPLE:
        {
            compiler.jle(*labels[index + offset + 1]);
            break;
        }
        default:
            cout << "Wrong operation " << insn << " was interpretated as compare operation" << endl;
    }

    stackTypes.pop_back();
    stackTypes.pop_back();
    stackTypes.push_back(VT_INT);
    compiler.unuse(var1);
    compiler.unuse(var2);
}

void interpreter::generateXMMArithmetics(Instruction insn, Compiler& compiler)
{
    XMMVar var1(compiler.newXMM());
    XMMVar var2(compiler.newXMM());
    pop(var1, compiler);
    pop(var2, compiler);

    switch(insn)
    {
            case BC_DADD:
            {
                compiler.addsd(var1, var2);
                break;
            }
            case BC_DSUB:
            {
                compiler.subsd(var1, var2);
                break;
            }
            case BC_DMUL:
            {
                compiler.mulsd(var1, var2);
                break;
            }
            case BC_DDIV:
            {
                compiler.divsd(var1, var2);
                break;
            }
            default:
                cout << "Wrong operation " << insn << " was interpretated as XMM arithmetic operation" << endl;
    }

    push(var1, compiler);
    compiler.unuse(var1);
    compiler.unuse(var2);
    stackTypes.pop_back();
}

void interpreter::generateGPArithmetics(Instruction insn, Compiler& compiler)
{
    GPVar var1(compiler.newGP());
    GPVar var2(compiler.newGP());
    compiler.pop(var1);
    compiler.pop(var2);

    switch(insn)
    {
            case BC_IADD:
            {
                compiler.add(var1, var2);
                break;
            }
            case BC_ISUB:
            {
                compiler.sub(var1, var2);
                break;
            }
            case BC_IMUL:
            {
                compiler.imul(var1, var2);
                break;
            }
            case BC_IMOD:
            {
                GPVar ret(compiler.newGP());
                ECall* call = compiler.call((sysint_t)imod);
                call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<sysint_t, sysint_t, sysint_t>());
                call->setArgument(0, var1);
                call->setArgument(1, var2);
                call->setReturn(ret);
                compiler.push(ret);
                compiler.unuse(ret);
                compiler.unuse(var1);
                compiler.unuse(var2);
                stackTypes.pop_back();
                return;
            }
            case BC_IDIV:
            {
                GPVar ret(compiler.newGP());
                ECall* call = compiler.call((sysint_t)idiv);
                call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<sysint_t, sysint_t, sysint_t>());
                call->setArgument(0, var1);
                call->setArgument(1, var2);
                call->setReturn(ret);
                compiler.push(ret);
                compiler.unuse(ret);
                compiler.unuse(var1);
                compiler.unuse(var2);
                stackTypes.pop_back();
                return;
            }
            default:
                cout << "Wrong operation " << insn << " was interpretated as GP arithmetic operation" << endl;
    }

    compiler.push(var1);
    compiler.unuse(var1);
    compiler.unuse(var2);
    stackTypes.pop_back();
}


void interpreter::generateFunction(BytecodeFunction* function, int funcIndex)
{
    bool start = true;
    if (!funcIndex)
    {
        compiler.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<void>());
        compiler.getFunction()->setHint(FUNCTION_HINT_NAKED, true);
    }

    Bytecode* bytecode = function->bytecode();
    uint32_t index = 0;
    while (index < bytecode->length())
    {
        bool jump = false;
        if (labels.count(index) > 0) {
            compiler.bind(*labels[index]);
        }
        Instruction insn = bytecode->getInsn(index);
        cout << str(insn) << endl;
        if (insn != BC_JA) start = false;
        switch(insn)
        {
        //one
            case BC_DLOAD:
            {
                double val = bytecode->getDouble(index + 1);
                loadd(val, compiler);
                break;
            }
            case BC_ILOAD:
            {
                int val = bytecode->getInt64(index + 1);
                loadi(val, compiler);
                break;
            }
            case BC_SLOAD:
            {
                int val = bytecode->getInt16(index + 1);
                loads(val, compiler);
                break;
            }
            case BC_CALL:
            {
                int id = bytecode->getInt16(index + 1);
                BytecodeFunction *fun = (BytecodeFunction *)code->functionById(id);
                generateFunction(fun, id); 
                break;
            }
            case BC_RETURN: 
            {
                return;
            }
            case BC_CALLNATIVE:
            {
                cout << "No native calls" << endl;
            }
            case BC_LOADDVAR:
            {
                int id = bytecode->getInt16(index + 1);
                GPVar var = dvars[id];
                push(var, compiler);
                stackTypes.push_back(VT_DOUBLE);
                break;
            }
            case BC_LOADIVAR:
            {
                int id = bytecode->getInt16(index + 1);
                GPVar var = ivars[id];
                push(var, compiler);
                stackTypes.push_back(VT_INT);
                break;
            }
            case BC_LOADSVAR:
            {
                int id = bytecode->getInt16(index + 1);
                GPVar var = svars[id];
                push(var, compiler);
                stackTypes.push_back(VT_STRING);
                break;
            }
            case BC_STOREDVAR:
            {
                int id = bytecode->getInt16(index + 1);
                GPVar var(compiler.newGP());
                pop(var, compiler);
                dvars[id] = var;
                stackTypes.pop_back();
                break;
            }
            case BC_STOREIVAR:
            {
                int id = bytecode->getInt16(index + 1);
                GPVar var(compiler.newGP());
                pop(var, compiler);
                ivars[id] = var;
                stackTypes.pop_back();
                break;
            }
            case BC_STORESVAR:
            {
                int id = bytecode->getInt16(index + 1);
                GPVar var(compiler.newGP());
                pop(var, compiler);
                svars[id] = var;
                stackTypes.pop_back();
                break;
            }
            case BC_IFICMPNE:
            case BC_IFICMPE:
            case BC_IFICMPG:
            case BC_IFICMPGE:
            case BC_IFICMPL:
            case BC_IFICMPLE:
            {
                generateCompare(insn, bytecode, index, compiler);
                break;
            }
            case BC_JA:
            {
                int16_t offset = bytecode->getInt16(index + 1);
                if (labels.count(index + offset + 1) == 0)
                {
                    AsmJit::Label* label = new AsmJit::Label(compiler.newLabel());
                    labels[index + offset + 1] = label;
                }
                compiler.jmp(*labels[index + offset + 1]);
                if (!funcIndex && start)
                {
                    index += (offset + 1);
                    jump = true;
                }
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
                cout << "Use other loads" << endl;
            }

         //none 
            case BC_DLOAD0:
            {
                loadd(0.0, compiler);
                break;
            }
            case BC_ILOAD0:
            {
                loadi(0, compiler);
                break;
            }
            case BC_SLOAD0:
            {
                loads(code->makeStringConstant(""), compiler);
                break;
            }
            case BC_DLOAD1:
            {
                loadd(1.0, compiler);
                break;
            }
            case BC_ILOAD1:
            {
                loadi(1, compiler);
                break;
            }
            case BC_DLOADM1:
            {
                loadd(-1.0, compiler);
                break;
            }
            case BC_ILOADM1:
            {
                loadi(-1, compiler);
                break;
            }
            case BC_DADD:
            case BC_DSUB:
            case BC_DMUL:
            case BC_DDIV:
            {
                generateXMMArithmetics(insn, compiler);
                break;
            }
            case BC_IADD:
            case BC_ISUB:
            case BC_IMUL:
            case BC_IMOD:
            case BC_IDIV:
            {
                generateGPArithmetics(insn, compiler);
                break;
            }
            case BC_DNEG:
            {
                XMMVar var(compiler.newXMM());
                loadd(-1.0, compiler);
                XMMVar sign(compiler.newXMM());
                pop(sign, compiler);
                pop(var, compiler);
                compiler.mulsd(var, sign);
                push(var, compiler);
                compiler.unuse(var);
                compiler.unuse(sign);
                break;
            }
            case BC_INEG:
            {
                GPVar var(compiler.newGP());
                pop(var, compiler);
                compiler.imul(var, imm(-1));
                push(var, compiler);
                compiler.unuse(var); 
                break;
            }
            case BC_IPRINT:
            {
                GPVar var(compiler.newGP());
                pop(var, compiler);
                ECall* call = compiler.call((sysint_t)printi);
                call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, sysint_t>());
                call->setArgument(0, var);
                compiler.unuse(var);
                stackTypes.pop_back();
                break;
            }
            case BC_DPRINT:
            {
                XMMVar var(compiler.newXMM());
                pop(var, compiler);
                ECall* call = compiler.call((sysint_t)printd);
                call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, double>());
                call->setArgument(0, var);
                compiler.unuse(var);
                stackTypes.pop_back();
                break;
            }
            case BC_SPRINT:
            {
                GPVar var(compiler.newGP());
                pop(var, compiler);
                ECall* call = compiler.call((sysint_t)prints);
                call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, const char*>());
                call->setArgument(0, var);
                compiler.unuse(var);
                stackTypes.pop_back();
                break;
            }
            case BC_I2D:
            {
                GPVar var(compiler.newGP());
                XMMVar ret(compiler.newXMM());
                pop(var, compiler);
                ECall* call = compiler.call((sysint_t)i2d);
                call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<double, sysint_t>());
                call->setArgument(0, var);
                call->setReturn(ret);
                push(ret, compiler);
                compiler.unuse(ret);
                compiler.unuse(var);
                stackTypes.pop_back();
                stackTypes.push_back(VT_DOUBLE);
                break;
            }
            case BC_D2I:
            {
                XMMVar var(compiler.newXMM());
                GPVar ret(compiler.newGP());
                pop(var, compiler);
                ECall* call = compiler.call((sysint_t)d2i);
                call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<sysint_t, double>());
                call->setArgument(0, var);
                call->setReturn(ret);
                push(ret, compiler);
                compiler.unuse(ret);
                compiler.unuse(var);
                stackTypes.pop_back();
                stackTypes.push_back(VT_INT);
                break;
            }
            case BC_S2I:
            {
                GPVar var(compiler.newGP());
                GPVar ret(compiler.newGP());
                pop(var, compiler);
                ECall* call = compiler.call((sysint_t)s2i);
                call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<sysint_t, const char*>());
                call->setArgument(0, var);
                call->setReturn(ret);
                push(ret, compiler);
                compiler.unuse(ret);
                compiler.unuse(var);
                stackTypes.pop_back();
                stackTypes.push_back(VT_INT);
                break;
            }
            case BC_SWAP:
            {
                GPVar var1(compiler.newGP());
                GPVar var2(compiler.newGP());
                pop(var1, compiler);
                pop(var2, compiler);
                push(var1, compiler);
                push(var2, compiler);
                compiler.unuse(var1);
                compiler.unuse(var2);
                VarType type1;
                VarType type2;
                type1 = stackTypes.back();
                stackTypes.pop_back();
                type2 = stackTypes.back();
                stackTypes.pop_back();
                stackTypes.push_back(type1);
                stackTypes.push_back(type2);
                break;
            }
            case BC_POP:
            {
                GPVar var(compiler.newGP());
                pop(var, compiler);
                compiler.unuse(var);
                stackTypes.pop_back();
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
            {
                compiler.ret();
                compiler.endFunction();  
                main = compiler.make();
                return;  
            }

        }
        if (!jump)
        {
            index += (length(insn));
        }
    }
}