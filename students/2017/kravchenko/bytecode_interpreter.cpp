#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "bytecode_interpreter.h"
#include "asmjit/asmjit.h"

#include <math.h>
#include <iostream>

namespace mathvm {

Status* BytecodeInterpreter::execute(vector<Var*>& initVars)
{
    uint16_t mainScopeId = 0;

    /*
     * I don't know C++ well, but for some reason I have to do it here.
     * Otherwise the first access to frames map freezes
     */
    this->frames = std::map<uint16_t, std::stack<ScopeMap *>>();

    ScopeMap *mainScopeMap = new ScopeMap();
    frames[mainScopeId] = std::stack<ScopeMap *>();
    frames[mainScopeId].push(mainScopeMap);
    callStack.push(0);

    for (Var *var : initVars) {
        uint16_t varId = globalVarIds[var->name()];
        BcValue val;
        if (var->type() == VT_INT)
            val.intVal = var->getIntValue();
        else if (var->type() == VT_DOUBLE)
            val.doubleVal = var->getDoubleValue();
        else if (var->type() == VT_STRING) {
            uint16_t id = makeStringConstant(std::string(var->getStringValue()));
            val.stringVal = id;
        } else
            assert(false);
//        std::cout << "setting var" << std::endl;
        setVar(mainScopeId, varId, val);
//        std::cout << "set var" << std::endl;
    }

//    std::cout << "\n\n\n" << std::endl;

    execFun((BytecodeFunction *)functionById(0));

    return Status::Ok();
}

BcValue BytecodeInterpreter::getVar(uint16_t scopeId, uint16_t varId)
{
//    std::cout << "getting var from scope " << scopeId << " varId is " << varId << std::endl;
    auto &it = (--frames.upper_bound(scopeId));
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
//    std::cout << "setting var from scope " << scopeId << " varId is " << varId << std::endl;
    auto &it = (--frames.upper_bound(scopeId));
    while (true) {
        std::stack<ScopeMap *> &scopeStack = it->second;
        if (scopeStack.size() == 0) {
            --it;
            continue;
        }
        (*scopeStack.top())[scopeId][varId] = val;
        break;
    }

//    std::cout << "successfully set var to " << getVar(scopeId, varId).intVal << std::endl;
}

void BytecodeInterpreter::execFun(BytecodeFunction *fn)
{
    fun = fn;
    pc = 0;

    frameSizes.push(stack.size());

    while (pc < fun->bytecode()->length()) {
        Instruction insn = nextInsn();

        size_t length;
        std::string name(bytecodeName(insn, &length));
//        std::cout << "handling: " << pc - 1 << " " << name << std::endl;

        if (insn == BC_RETURN)
            break;
        switch (insn) {
#define CASE(b, d, l) case BC_##b: do_##b(); break;
            FOR_BYTECODES(CASE)
#undef CASE
            case BC_LAST:
                assert(false);
        }
    }

    assert(frameSizes.top() <= stack.size());

    while (stack.size() != frameSizes.top()) {
//        std::cout << "popping due to frame sizes" << std::endl;
        stack.pop();
    }

    frameSizes.pop();
    if (returnAddress.size() != 0) {
        fun = std::get<0>(returnAddress.top());
        pc = std::get<1>(returnAddress.top());
        returnAddress.pop();
//        std::cout << "popping frame " << callStack.top() << std::endl;
        frames[callStack.top()].pop();
        callStack.pop();
    }
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

//    std::cout << "pushed int " << i << std::endl;
}

void BytecodeInterpreter::pushDouble(double d)
{
    BcValue val;
    val.doubleVal = d;
    stack.push(val);
}

void BytecodeInterpreter::pushString(uint16_t id)
{
    BcValue val;
    val.stringVal = id;
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

uint16_t BytecodeInterpreter::popString()
{
    uint16_t s = stack.top().stringVal;
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

    pushString(s);
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
    pushString(0);
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
    uint16_t s = popString();

    std::cout << constantById(s);
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
    uint16_t s = popString();

    pushInt(s);
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
//    std::cout << "popping with stack size " << stack.size() << std::endl;
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
    uint16_t s = popString();

    reg0.stringVal = s;
}

void BytecodeInterpreter::do_STORESVAR1()
{
    uint16_t s = popString();

    reg1.stringVal = s;
}

void BytecodeInterpreter::do_STORESVAR2()
{
    uint16_t s = popString();

    reg2.stringVal = s;
}

void BytecodeInterpreter::do_STORESVAR3()
{
    uint16_t s = popString();

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

//    std::cout << "loading value " << vars[scopeId][varId].intVal << " from ctx " << scopeId << " from var " << varId << std::endl;

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

//    std::cout << "storing value " << i << " to ctx " << scopeId << " to var " << varId << std::endl;

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
    BytecodeFunction *fn = (BytecodeFunction *)functionById(funId);

    returnAddress.push(std::make_pair(fun, pc));

    ScopeMap *callScopeMap = new ScopeMap();
    frames[fn->scopeId()].push(callScopeMap);
//    std::cout << "pusing frame " << fn->scopeId() << std::endl;;
    callStack.push(fn->scopeId());

    execFun(fn);

    delete callScopeMap;
}

void BytecodeInterpreter::do_CALLNATIVE()
{
    // TODO
}

void BytecodeInterpreter::do_RETURN()
{
    // I handle this instruction in a different manner
    assert(false);
}

void BytecodeInterpreter::do_BREAK()
{
    // I don't produce this instruction for now
    assert(false);
}


}
