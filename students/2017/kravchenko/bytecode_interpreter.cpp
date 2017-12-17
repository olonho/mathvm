#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "bytecode_interpreter.h"
#include "asmjit/asmjit.h"

#include <math.h>
#include <iostream>

using namespace mathvm;

Status* BytecodeInterpreter::execute(vector<Var*>& initVars)
{
    fun = (BytecodeFunction *)functionById(0);
    uint16_t mainScopeId = fun->scopeId();

    ScopeMap *mainScopeMap = new ScopeMap();
    frames[mainScopeId].push(mainScopeMap);
    callStack.push(mainScopeId);
    frameSizes.push(0);

    for (Var *var : initVars) {
        uint16_t varId = globalVarIds[var->name()];
        BcValue val;
        if (var->type() == VT_INT)
            val.intVal = var->getIntValue();
        else if (var->type() == VT_DOUBLE)
            val.doubleVal = var->getDoubleValue();
        else if (var->type() == VT_STRING) {
            makeStringConstant(std::string(var->getStringValue()));
            val.stringVal = var->getStringValue();
        } else
            assert(false);
        setVar(mainScopeId, varId, val);
    }

    execFun();

    return Status::Ok();
}

BcValue BytecodeInterpreter::getVar(uint16_t scopeId, uint16_t varId)
{
    // auto& does not work with -O flags, don't know why
    std::map<uint16_t, std::stack<ScopeMap *>>::iterator it = frames.upper_bound(scopeId);
    --it;

    while (true) {
        std::stack<ScopeMap *> &scopeStack = it->second;
        if (scopeStack.size() == 0) {
            --it;
            continue;
        }
        return (*scopeStack.top())[scopeId][varId];
        break;
    }
}

void BytecodeInterpreter::setVar(uint16_t scopeId, uint16_t varId, BcValue val)
{
    std::map<uint16_t, std::stack<ScopeMap *>>::iterator it = frames.upper_bound(scopeId);
    --it;

    while (true) {
        std::stack<ScopeMap *> &scopeStack = it->second;
        if (scopeStack.size() == 0) {
            --it;
            continue;
        }
        (*scopeStack.top())[scopeId][varId] = val;
        break;
    }
}

void BytecodeInterpreter::execFun()
{
    pc = 0;

    while (pc < fun->bytecode()->length()) {
        Instruction insn = nextInsn();

//        size_t length;
//        std::string name(bytecodeName(insn, &length));
//        std::cout << "handling: " << pc - 1 << " " << name << std::endl;

        switch (insn) {
#define CASE(b, d, l) case BC_##b: do_##b(); break;
            FOR_BYTECODES(CASE)
#undef CASE
            case BC_LAST:
                assert(false);
        }
    }

    ScopeMap *mainScopeMap = frames[fun->scopeId()].top();
    frames[fun->scopeId()].pop();
    delete mainScopeMap;
    callStack.pop();
    frameSizes.pop();
}

void BytecodeInterpreter::createNativeWrapper(uint16_t nativeFunId)
{
    using namespace asmjit;
    using namespace asmjit::x86;

    X86Assembler jAsm(&this->jrt);
    X86Compiler jCompiler(&jAsm);

//    FileLogger logger(stdout);
//    jAsm.setLogger(&logger);

    const Signature *sign;
    const std::string *name;
    const void *address = nativeById(nativeFunId, &sign, &name);

    // See x86_64 ABI for registers order
    // See libs/asmjit/src/x86/x86operand.h:2187 for definitions
    X86GpReg integerRegs[6] = {rdi, rsi, rdx, rcx, r8, r9};
    X86XmmReg doubleRegs[8] = {xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7};

    size_t intRegId = 0;
    size_t doubleRegId = 0;
 
    jCompiler.addFunc(FuncBuilder2<void, void *, void *>(kCallConvX64Unix));

    // save the pointer to the result on stack, we will need him later
    jAsm.push(rsi);
    // mov params pointer to rax, as we will need rdi
    jAsm.mov(rax, rdi);

    for (size_t i = 1; i < sign->size(); i++) {
        const mathvm::VarType type = std::get<0>((*sign)[i]);
        if (type == VT_INT || type == VT_STRING) {
            assert(intRegId < sizeof(integerRegs)/sizeof(integerRegs[0]));
            X86GpReg curArgReg = integerRegs[intRegId++];
            jAsm.mov(curArgReg, qword_ptr(rax));
        } else if (type == VT_DOUBLE) {
            assert(doubleRegId < sizeof(doubleRegs)/sizeof(doubleRegs[0]));
            X86XmmReg curArgReg = doubleRegs[doubleRegId++];
            jAsm.movsd(curArgReg, qword_ptr(rax));
        }
        // now move to the next argument
        jAsm.add(rax, sizeof(BcValue));
    }

    // ok, args moved, now call native function itself
    jAsm.call((Ptr)address);

    // save the result to the result pointer
    jAsm.pop(rbx);

    const mathvm::VarType returnType = std::get<0>((*sign)[0]);
    if (returnType == VT_INT || returnType == VT_STRING)
        jAsm.mov(qword_ptr(rbx), rax);
    else if (returnType == VT_DOUBLE) {
        jAsm.movsd(qword_ptr(rbx), xmm0);
    }

    // everything is done! now just return back
    jAsm.ret();

    jCompiler.endFunc();
    jCompiler.finalize();

    nativeWrappers[nativeFunId] = (NativeFunWrapper)(jAsm.make());
}

NativeFunWrapper BytecodeInterpreter::getNativeWrapper(uint16_t nativeFunId)
{
    if (nativeWrappers[nativeFunId] == NULL)
        createNativeWrapper(nativeFunId);
    return nativeWrappers[nativeFunId];
}

Instruction BytecodeInterpreter::nextInsn()
{
    return fun->bytecode()->getInsn(pc++);
}

void BytecodeInterpreter::pushInt(int64_t i)
{
    BcValue val;
    val.intVal = i;
    stack.push(val);
}

void BytecodeInterpreter::pushDouble(double d)
{
    BcValue val;
    val.doubleVal = d;
    stack.push(val);
}

void BytecodeInterpreter::pushString(const char *str)
{
    BcValue val;
    val.stringVal = str;
    stack.push(val);
}

int64_t BytecodeInterpreter::popInt()
{
    int64_t i = stack.top().intVal;
    stack.pop();

    return i;
}

double BytecodeInterpreter::popDouble()
{
    double d = stack.top().doubleVal;
    stack.pop();

    return d;
}

const char * BytecodeInterpreter::popString()
{
    const char *s = stack.top().stringVal;
    stack.pop();

    return s;
}

void BytecodeInterpreter::do_INVALID()
{
    assert(false);
}

void BytecodeInterpreter::do_DLOAD()
{
    double d = fun->bytecode()->getDouble(pc);
    pc += sizeof(double);

    pushDouble(d);
}

void BytecodeInterpreter::do_ILOAD()
{
    int64_t i = fun->bytecode()->getInt64(pc);
    pc += sizeof(int64_t);

    pushInt(i);
}

void BytecodeInterpreter::do_SLOAD()
{
    uint16_t s = fun->bytecode()->getUInt16(pc);
    pc += sizeof(uint16_t);

    pushString(constantById(s).c_str());
}

void BytecodeInterpreter::do_DLOAD0()
{
    pushDouble(0);
}

void BytecodeInterpreter::do_ILOAD0()
{
    pushInt(0);
}

void BytecodeInterpreter::do_SLOAD0()
{
    pushString(constantById(0).c_str());
}

void BytecodeInterpreter::do_DLOAD1()
{
    pushDouble(1);
}

void BytecodeInterpreter::do_ILOAD1()
{
    pushInt(1);
}

void BytecodeInterpreter::do_DLOADM1()
{
    pushDouble(-1);
}

void BytecodeInterpreter::do_ILOADM1()
{
    pushInt(-1);
}

void BytecodeInterpreter::do_DADD()
{
    double rhs = popDouble();
    double lhs = popDouble();

    pushDouble(lhs + rhs);
}

void BytecodeInterpreter::do_IADD()
{
    int64_t rhs = popInt();
    int64_t lhs = popInt();

    pushInt(lhs + rhs);
}

void BytecodeInterpreter::do_DSUB()
{
    double rhs = popDouble();
    double lhs = popDouble();

    pushDouble(lhs - rhs);
}

void BytecodeInterpreter::do_ISUB()
{
    int64_t rhs = popInt();
    int64_t lhs = popInt();

    pushInt(lhs - rhs);
}

void BytecodeInterpreter::do_DMUL()
{
    double rhs = popDouble();
    double lhs = popDouble();

    pushDouble(lhs * rhs);
}

void BytecodeInterpreter::do_IMUL()
{
    int64_t rhs = popInt();
    int64_t lhs = popInt();

    pushInt(lhs * rhs);
}

void BytecodeInterpreter::do_DDIV()
{
    double rhs = popDouble();
    double lhs = popDouble();

    pushDouble(lhs / rhs);
}

void BytecodeInterpreter::do_IDIV()
{
    int64_t rhs = popInt();
    int64_t lhs = popInt();

    pushInt(lhs / rhs);
}

void BytecodeInterpreter::do_IMOD()
{
    int64_t rhs = popInt();
    int64_t lhs = popInt();

    pushInt(lhs % rhs);
}

void BytecodeInterpreter::do_DNEG()
{
    double arg = popDouble();

    pushDouble(-arg);
}

void BytecodeInterpreter::do_INEG()
{
    int64_t arg = popInt();

    pushInt(-arg);
}

void BytecodeInterpreter::do_IAOR()
{
    int64_t rhs = popInt();
    int64_t lhs = popInt();

    pushInt(lhs | rhs);
}

void BytecodeInterpreter::do_IAAND()
{
    int64_t rhs = popInt();
    int64_t lhs = popInt();

    pushInt(lhs & rhs);
}

void BytecodeInterpreter::do_IAXOR()
{
    int64_t rhs = popInt();
    int64_t lhs = popInt();

    pushInt(lhs ^ rhs);
}

void BytecodeInterpreter::do_IPRINT()
{
    int64_t arg = popInt();

    std::cout << arg;
}

void BytecodeInterpreter::do_DPRINT()
{
    double arg = popDouble();

    std::cout << arg;
}

void BytecodeInterpreter::do_SPRINT()
{
    const char *s = popString();

    std::cout << std::string(s);
}

void BytecodeInterpreter::do_I2D()
{
    int64_t i = popInt();

    pushDouble(i);
}

void BytecodeInterpreter::do_D2I()
{
    double d = popDouble();

    pushInt(d);
}

void BytecodeInterpreter::do_S2I()
{
    const char *s = popString();

    pushInt((int64_t)s);
}

void BytecodeInterpreter::do_SWAP()
{
    BcValue fst = stack.top();
    stack.pop();
    BcValue snd = stack.top();
    stack.pop();

    stack.push(fst);
    stack.push(snd);
}

void BytecodeInterpreter::do_POP()
{
    stack.pop();
}

void BytecodeInterpreter::do_LOADDVAR0()
{
    pushDouble(reg0.doubleVal);
}

void BytecodeInterpreter::do_LOADDVAR1()
{
    pushDouble(reg1.doubleVal);
}

void BytecodeInterpreter::do_LOADDVAR2()
{
    pushDouble(reg2.doubleVal);
}

void BytecodeInterpreter::do_LOADDVAR3()
{
    pushDouble(reg3.doubleVal);
}

void BytecodeInterpreter::do_LOADIVAR0()
{
    pushInt(reg0.intVal);
}

void BytecodeInterpreter::do_LOADIVAR1()
{
    pushInt(reg1.intVal);
}

void BytecodeInterpreter::do_LOADIVAR2()
{
    pushInt(reg2.intVal);
}

void BytecodeInterpreter::do_LOADIVAR3()
{
    pushInt(reg3.intVal);
}

void BytecodeInterpreter::do_LOADSVAR0()
{
    pushString(reg0.stringVal);
}

void BytecodeInterpreter::do_LOADSVAR1()
{
    pushString(reg1.stringVal);
}

void BytecodeInterpreter::do_LOADSVAR2()
{
    pushString(reg2.stringVal);
}

void BytecodeInterpreter::do_LOADSVAR3()
{
    pushString(reg3.stringVal);
}

void BytecodeInterpreter::do_STOREDVAR0()
{
    double d = popDouble();

    reg0.doubleVal = d;
}

void BytecodeInterpreter::do_STOREDVAR1()
{
    double d = popDouble();

    reg1.doubleVal = d;
}

void BytecodeInterpreter::do_STOREDVAR2()
{
    double d = popDouble();

    reg2.doubleVal = d;
}

void BytecodeInterpreter::do_STOREDVAR3()
{
    double d = popDouble();

    reg3.doubleVal = d;
}

void BytecodeInterpreter::do_STOREIVAR0()
{
    int64_t i = popInt();

    reg0.intVal = i;
}

void BytecodeInterpreter::do_STOREIVAR1()
{
    int64_t i = popInt();

    reg1.intVal = i;
}

void BytecodeInterpreter::do_STOREIVAR2()
{
    int64_t i = popInt();

    reg2.intVal = i;
}

void BytecodeInterpreter::do_STOREIVAR3()
{
    int64_t i = popInt();

    reg3.intVal = i;
}

void BytecodeInterpreter::do_STORESVAR0()
{
    const char *s = popString();

    reg0.stringVal = s;
}

void BytecodeInterpreter::do_STORESVAR1()
{
    const char *s = popString();

    reg1.stringVal = s;
}

void BytecodeInterpreter::do_STORESVAR2()
{
    const char *s = popString();

    reg2.stringVal = s;
}

void BytecodeInterpreter::do_STORESVAR3()
{
    const char *s = popString();

    reg3.stringVal = s;
}

void BytecodeInterpreter::do_LOADDVAR()
{
    // I don't produce this instruction for now
    assert(false);
}

void BytecodeInterpreter::do_LOADIVAR()
{
    // I don't produce this instruction for now
    assert(false);
}

void BytecodeInterpreter::do_LOADSVAR()
{
    // I don't produce this instruction for now
    assert(false);
}

void BytecodeInterpreter::do_STOREDVAR()
{
    // I don't produce this instruction for now
    assert(false);
}

void BytecodeInterpreter::do_STOREIVAR()
{
    // I don't produce this instruction for now
    assert(false);
}

void BytecodeInterpreter::do_STORESVAR()
{
    // I don't produce this instruction for now
    assert(false);
}

void BytecodeInterpreter::do_LOADCTXDVAR()
{
    uint16_t scopeId = fun->bytecode()->getUInt16(pc);
    pc += 2;
    uint16_t varId = fun->bytecode()->getUInt16(pc);
    pc += 2;

    pushDouble(getVar(scopeId, varId).doubleVal);
}

void BytecodeInterpreter::do_LOADCTXIVAR()
{
    uint16_t scopeId = fun->bytecode()->getUInt16(pc);
    pc += 2;
    uint16_t varId = fun->bytecode()->getUInt16(pc);
    pc += 2;

    pushInt(getVar(scopeId, varId).intVal);
}

void BytecodeInterpreter::do_LOADCTXSVAR()
{
    uint16_t scopeId = fun->bytecode()->getUInt16(pc);
    pc += 2;
    uint16_t varId = fun->bytecode()->getUInt16(pc);
    pc += 2;

    pushString(getVar(scopeId, varId).stringVal);
}

void BytecodeInterpreter::do_STORECTXDVAR()
{
    uint16_t scopeId = fun->bytecode()->getUInt16(pc);
    pc += 2;
    uint16_t varId = fun->bytecode()->getUInt16(pc);
    pc += 2;

    BcValue val;
    val.doubleVal = popDouble();
    setVar(scopeId, varId, val);
}

void BytecodeInterpreter::do_STORECTXIVAR()
{
    uint16_t scopeId = fun->bytecode()->getUInt16(pc);
    pc += 2;
    uint16_t varId = fun->bytecode()->getUInt16(pc);
    pc += 2;

    BcValue val;
    val.intVal = popInt();
    setVar(scopeId, varId, val);
}

void BytecodeInterpreter::do_STORECTXSVAR()
{
    uint16_t scopeId = fun->bytecode()->getUInt16(pc);
    pc += 2;
    uint16_t varId = fun->bytecode()->getUInt16(pc);
    pc += 2;

    BcValue val;
    val.stringVal = popString();
    setVar(scopeId, varId, val);
}

void BytecodeInterpreter::do_DCMP()
{
    double lower = popDouble();
    double upper = popDouble();

    if (upper > lower)
        pushInt(1);
    else if (upper == lower)
        pushInt(0);
    else if (upper < lower)
        pushInt(-1);
}

void BytecodeInterpreter::do_ICMP()
{
    int64_t lower = popInt();
    int64_t upper = popInt();

    if (upper > lower)
        pushInt(1);
    else if (upper == lower)
        pushInt(0);
    else if (upper < lower)
        pushInt(-1);
}

void BytecodeInterpreter::do_JA()
{
    int16_t offset = fun->bytecode()->getInt16(pc);
    if (offset < 0)
        assert((size_t)abs(offset) <= pc);
    pc += offset;
}

void BytecodeInterpreter::do_IFICMPNE()
{
    int64_t lower = popInt();
    int64_t upper = popInt();

    if (upper == lower)
        pc += sizeof(uint16_t);
    else {
        int16_t offset = fun->bytecode()->getInt16(pc);
        if (offset < 0)
            assert((size_t)abs(offset) <= pc);
        pc += offset;
    }
}

void BytecodeInterpreter::do_IFICMPE()
{
    int64_t lower = popInt();
    int64_t upper = popInt();

    if (upper != lower)
        pc += sizeof(uint16_t);
    else {
        int16_t offset = fun->bytecode()->getInt16(pc);
        if (offset < 0)
            assert((size_t)abs(offset) <= pc);
        pc += offset;
    }
}

void BytecodeInterpreter::do_IFICMPG()
{
    int64_t lower = popInt();
    int64_t upper = popInt();

    if (upper <= lower)
        pc += sizeof(uint16_t);
    else {
        int16_t offset = fun->bytecode()->getInt16(pc);
        if (offset < 0)
            assert((size_t)abs(offset) <= pc);
        pc += offset;
    }
}

void BytecodeInterpreter::do_IFICMPGE()
{
    int64_t lower = popInt();
    int64_t upper = popInt();

    if (upper < lower)
        pc += sizeof(uint16_t);
    else {
        int16_t offset = fun->bytecode()->getInt16(pc);
        if (offset < 0)
            assert((size_t)abs(offset) <= pc);
        pc += offset;
    }
}

void BytecodeInterpreter::do_IFICMPL()
{
    int64_t lower = popInt();
    int64_t upper = popInt();

    if (upper >= lower)
        pc += sizeof(uint16_t);
    else {
        int16_t offset = fun->bytecode()->getInt16(pc);
        if (offset < 0)
            assert((size_t)abs(offset) <= pc);
        pc += offset;
    }
}

void BytecodeInterpreter::do_IFICMPLE()
{
    int64_t lower = popInt();
    int64_t upper = popInt();

    if (upper > lower)
        pc += sizeof(uint16_t);
    else {
        int16_t offset = fun->bytecode()->getInt16(pc);
        if (offset < 0)
            assert((size_t)abs(offset) <= pc);
        pc += offset;
    }
}

void BytecodeInterpreter::do_DUMP()
{
    // I don't produce this instruction for now
    assert(false);
}

void BytecodeInterpreter::do_STOP()
{
    // I don't produce this instruction for now
    assert(false);
}

void BytecodeInterpreter::do_CALL()
{
    uint16_t funId = fun->bytecode()->getUInt16(pc);
    pc += sizeof(uint16_t);

    returnAddress.push(std::make_pair(fun, pc));

    BytecodeFunction *fn = (BytecodeFunction *)functionById(funId);
    assert(fn->id() != 0);

    ScopeMap *callScopeMap = new ScopeMap();
    frames[fn->scopeId()].push(callScopeMap);
    callStack.push(fn->scopeId());
    assert(fn->parametersNumber() <= stack.size());
    frameSizes.push(stack.size() - fn->parametersNumber());

    pc = 0;
    fun = fn;
}

void BytecodeInterpreter::do_CALLNATIVE()
{
    uint16_t funId = fun->bytecode()->getUInt16(pc);
    pc += 2;
    NativeFunWrapper fn = getNativeWrapper(funId);

    const Signature *sign;
    const std::string *name;
    nativeById(funId, &sign, &name);

    BcValue args[sign->size() - 1];
    for (int i = 1; i < (int)sign->size(); i++) {
        const VarType paramType = std::get<0>((*sign)[i]);
        if (paramType == VT_INT)
            args[i - 1].intVal = popInt();
        else if (paramType == VT_STRING)
            args[i - 1].stringVal = popString();
        else if (paramType == VT_DOUBLE)
            args[i - 1].doubleVal = popDouble();
        else
            assert(false);
    }

    fn(&args, &reg0);
}

void BytecodeInterpreter::do_RETURN()
{
    if (fun->id() == 0)
        return;

    assert(frameSizes.top() <= stack.size());

    while (stack.size() != frameSizes.top())
        stack.pop();

    fun = std::get<0>(returnAddress.top());
    pc = std::get<1>(returnAddress.top());

    returnAddress.pop();
    frameSizes.pop();
    ScopeMap *currentScopeMap = frames[callStack.top()].top();
    frames[callStack.top()].pop();
    delete currentScopeMap;
    callStack.pop();
}

void BytecodeInterpreter::do_BREAK()
{
    // I don't produce this instruction for now
    assert(false);
}
