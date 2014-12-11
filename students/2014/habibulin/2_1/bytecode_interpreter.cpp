#include "bytecode_interpreter.h"

#include <fstream>
#include <cstdlib>

void BytecodeInterpreter::interpret(Code *code) {
    try {
        _code = code;
        EMP_STR_ID = _code->makeStringConstant("");

        BytecodeFunction* fun = (BytecodeFunction *)_code->functionByName(MAIN_FUN_NAME);
        assert(fun);
        _contexts.push_back(Context());
        assert(fun->bytecode());
        interpFun(fun->bytecode());
        assert(!_contexts.empty());
        _contexts.pop_back();
        assert(_contexts.empty());
    } catch (ExceptionWithMsg& e) {
        _status.setError(e.what(), 0);
    }
}

void BytecodeInterpreter::interpFun(Bytecode* bc) {
    for(uint32_t i = 0; i != bc->length();) {
        int32_t offset = execInsn(bc, i);
        assert(offset >= 0 || i >= (uint32_t)abs(offset));
        i += offset;
    }
}

int32_t BytecodeInterpreter::execInsn(Bytecode* bc, uint32_t pos) {
    Instruction insn = bc->getInsn(pos);
    switch (insn) {
    case BC_DLOAD:
        assert(pos + 8 < bc->length());
        _programStack.push_back(StackVal(bc->getDouble(++pos)));
        return 9;
    case BC_ILOAD:
        assert(pos + 8 < bc->length());
        _programStack.push_back(StackVal(bc->getInt64(++pos)));
        return 9;
    case BC_SLOAD:
        assert(pos + 2 < bc->length());
        _programStack.push_back(StackVal((int64_t)bc->getInt16(++pos)));
        return 3;
    case BC_DLOAD0:
        _programStack.push_back(StackVal((double) 0.0));
        assert(_programStack.back().vDouble == 0);
        return 1;
    case BC_ILOAD0:
        _programStack.push_back(StackVal((int64_t) 0));
        assert(_programStack.back().vInt == 0);
        return 1;
    case BC_SLOAD0:
        _programStack.push_back(StackVal((int64_t) EMP_STR_ID));
        assert(_programStack.back().vInt == (int64_t)EMP_STR_ID);
        return 1;
    case BC_DLOAD1:
        _programStack.push_back(StackVal((double) 1.0));
        assert(_programStack.back().vDouble == 1.0);
        return 1;
    case BC_ILOAD1:
        _programStack.push_back(StackVal((int64_t) 1));
        assert(_programStack.back().vInt == 1);
        return 1;
    case BC_DLOADM1:
        _programStack.push_back(StackVal((double) -1.0));
        assert(_programStack.back().vDouble == -1.0);
        return 1;
    case BC_ILOADM1:
        _programStack.push_back(StackVal((int64_t) -1));
        assert(_programStack.back().vInt == -1);
        return 1;
    case BC_DADD: {
        auto ops = popOperands();
        _programStack.push_back(StackVal(ops.first.vDouble + ops.second.vDouble));
        return 1;
    }
    case BC_IADD: {
        auto ops = popOperands();
        _programStack.push_back(StackVal(ops.first.vInt + ops.second.vInt));
        return 1;
    }
    case BC_DSUB: {
        auto ops = popOperands();
        _programStack.push_back(StackVal(ops.first.vDouble - ops.second.vDouble));
        return 1;
    }
    case BC_ISUB: {
        auto ops = popOperands();
        _programStack.push_back(StackVal(ops.first.vInt - ops.second.vInt));
        return 1;
    }
    case BC_DMUL: {
        auto ops = popOperands();
        _programStack.push_back(StackVal(ops.first.vDouble * ops.second.vDouble));
        return 1;
    }
    case BC_IMUL: {
        auto ops = popOperands();
        _programStack.push_back(StackVal(ops.first.vInt * ops.second.vInt));
        return 1;
    }
    case BC_DDIV: {
        auto ops = popOperands();
        if(ops.second.vDouble == 0) { throw ExceptionWithMsg(divByZeroMsg("")); }
        _programStack.push_back(StackVal(ops.first.vDouble / ops.second.vDouble));
        return 1;
    }
    case BC_IDIV: {
        auto ops = popOperands();
        if(ops.second.vInt == 0) { throw ExceptionWithMsg(divByZeroMsg("")); }
        _programStack.push_back(StackVal(ops.first.vInt / ops.second.vInt));
        return 1;
    }
    case BC_IMOD: {
        auto ops = popOperands();
        if(ops.second.vInt == 0) { throw ExceptionWithMsg(divByZeroMsg("")); }
        _programStack.push_back(StackVal(ops.first.vInt % ops.second.vInt));
        return 1;
    }
    case BC_DNEG:
        assert(!_programStack.empty());
        _programStack.back().vDouble = (-1.0) * _programStack.back().vDouble;
        return 1;
    case BC_INEG:
        assert(!_programStack.empty());
        _programStack.back().vInt = (-1) * _programStack.back().vInt;
        return 1;
    case BC_IAOR: {
        auto ops = popOperands();
        _programStack.push_back(StackVal(ops.first.vInt | ops.second.vInt));
        return 1;
    }
    case BC_IAAND: {
        auto ops = popOperands();
        _programStack.push_back(StackVal(ops.first.vInt & ops.second.vInt));
        return 1;
    }
    case BC_IAXOR: {
        auto ops = popOperands();
        _programStack.push_back(StackVal(ops.first.vInt ^ ops.second.vInt));
        return 1;
    }
    case BC_IPRINT:
        assert(!_programStack.empty());
        cout << _programStack.back().vInt;
        _programStack.pop_back();
        return 1;
    case BC_DPRINT:
        assert(!_programStack.empty());
        cout << _programStack.back().vDouble;
        _programStack.pop_back();
        return 1;
    case BC_SPRINT:
        assert(!_programStack.empty());
        cout << _code->constantById((uint16_t)_programStack.back().vInt);
        _programStack.pop_back();
        return 1;
    case BC_I2D:
        assert(!_programStack.empty());
        _programStack.back().vDouble = (double) _programStack.back().vInt;
        return 1;
    case BC_D2I:
        assert(!_programStack.empty());
        _programStack.back().vInt = (int64_t) _programStack.back().vDouble;
        return 1;
    case BC_SWAP:
        assert(_programStack.size() > 1);
        std::swap(_programStack.back(), _programStack[_programStack.size() - 2]);
        return 1;
    case BC_POP:
        _programStack.pop_back();
        return 1;
    case BC_LOADDVAR:
    case BC_LOADIVAR:
    case BC_LOADSVAR:
        return loadvar(bc, ++pos);
    case BC_STOREDVAR:
        assert(!_programStack.empty());
        storeValueLocal(bc, ++pos, StackVal(_programStack.back().vDouble));
        _programStack.pop_back();
        return 3;
    case BC_STOREIVAR:
    case BC_STORESVAR:
        assert(!_programStack.empty());
        storeValueLocal(bc, ++pos, StackVal(_programStack.back().vInt));
        _programStack.pop_back();
        return 3;
    case BC_LOADCTXDVAR:
    case BC_LOADCTXIVAR:
    case BC_LOADCTXSVAR:
        return loadctxvar(bc, ++pos);
    case BC_STORECTXDVAR:
        assert(!_programStack.empty());
        storeValueGlobal(bc, ++pos, StackVal(_programStack.back().vDouble));
        _programStack.pop_back();
        return 5;
    case BC_STORECTXIVAR:
    case BC_STORECTXSVAR:
        assert(!_programStack.empty());
        storeValueGlobal(bc, ++pos, StackVal(_programStack.back().vInt));
        _programStack.pop_back();
        return 5;
    case BC_DCMP: {
        auto ops = getOperands();
        if (ops.first.vDouble == ops.second.vDouble) {
            _programStack.push_back(StackVal((int64_t)0));
        } else if (ops.first.vDouble < ops.second.vDouble) {
            _programStack.push_back(StackVal((int64_t)-1));
        } else {
            _programStack.push_back(StackVal((int64_t)1));
        }
        return 1;
    }
    case BC_ICMP: {
        auto ops = getOperands();
        if (ops.first.vInt == ops.second.vInt) {
            _programStack.push_back(StackVal((int64_t)0));
        } else if (ops.first.vInt < ops.second.vInt) {
            _programStack.push_back(StackVal((int64_t)-1));
        } else {
            _programStack.push_back(StackVal((int64_t)1));
        }
        return 1;
    }
    case BC_JA: {
        return ((int32_t) bc->getInt16(++pos)) + 1;
    }
    case BC_IFICMPNE:{
        auto ops = getOperands();
        if(ops.first.vInt != ops.second.vInt) {
            return ((int32_t) bc->getInt16(++pos)) + 1;
        }
        return 3;
    }
    case BC_IFICMPE:{
        auto ops = getOperands();
        if(ops.first.vInt == ops.second.vInt) {
            return ((int32_t) bc->getInt16(++pos)) + 1;
        }
        return 3;
    }
    case BC_IFICMPG:{
        auto ops = getOperands();
        if(ops.first.vInt > ops.second.vInt) {
            return ((int32_t) bc->getInt16(++pos)) + 1;
        }
        return 3;
    }
    case BC_IFICMPGE:{
        auto ops = getOperands();
        if(ops.first.vInt >= ops.second.vInt) {
            return ((int32_t) bc->getInt16(++pos)) + 1;
        }
        return 3;
    }
    case BC_IFICMPL:{
        auto ops = getOperands();
        if(ops.first.vInt < ops.second.vInt) {
            return ((int32_t) bc->getInt16(++pos)) + 1;
        }
        return 3;
    }
    case BC_IFICMPLE:{
        auto ops = getOperands();
        if(ops.first.vInt <= ops.second.vInt) {
            return ((int32_t) bc->getInt16(++pos)) + 1;
        }
        return 3;
    }
    case BC_CALL: {
        BytecodeFunction* fun = (BytecodeFunction *)_code->functionById(bc->getUInt16(++pos));
        assert(fun);
        _contexts.push_back(Context());
        assert(fun->bytecode());
        interpFun(fun->bytecode());
        assert(!_contexts.empty());
        _contexts.pop_back();
        return 3;
    }
    case BC_CALLNATIVE:
        assert(false);
        return 0;
    case BC_RETURN:
        return bc->length() - pos;
    default:
        throw ExceptionWithMsg(invalidBcMsg(insn));
    }
}

pair<BytecodeInterpreter::StackVal, BytecodeInterpreter::StackVal> BytecodeInterpreter::getOperands() {
    assert(_programStack.size() > 1);
    StackVal upper = _programStack.back();
    StackVal lower = _programStack[_programStack.size() - 2];
    return make_pair(upper, lower);
}

pair<BytecodeInterpreter::StackVal, BytecodeInterpreter::StackVal> BytecodeInterpreter::popOperands() {
    assert(_programStack.size() > 1);
    StackVal upper = _programStack.back();
    StackVal lower = _programStack[_programStack.size() - 2];
    _programStack.pop_back();
    _programStack.pop_back();
    return make_pair(upper, lower);
}

uint8_t BytecodeInterpreter::loadvar(Bytecode *bc, uint32_t pos) {
    uint16_t varId = bc->getInt16(pos);
    assert(!_contexts.empty());
    assert(_contexts.back().size() > varId);
    StackVal sv = _contexts.back()[varId];
    _programStack.push_back(sv);
    return 3;
}

void BytecodeInterpreter::storeValueLocal(Bytecode* bc, uint32_t pos, BytecodeInterpreter::StackVal val) {
    uint16_t varId = bc->getInt16(pos);
    assert(!_contexts.empty());
    if(varId >= _contexts.back().size()) {
        _contexts.back().resize(varId + 1);
    }
    assert(_contexts.back().size() > varId);
    _contexts.back()[varId] = val;
}

uint8_t BytecodeInterpreter::loadctxvar(Bytecode *bc, uint32_t pos) {
    assert(_contexts.size() > 1);
    uint16_t ctxId = bc->getInt16(pos);
    pos += 2;
    uint16_t varId = bc->getInt16(pos);
    assert(ctxId < _contexts.size());
    assert(varId < _contexts[ctxId].size());
    StackVal sv = _contexts[ctxId][varId];
    _programStack.push_back(sv);
    return 5;
}

void BytecodeInterpreter::storeValueGlobal(Bytecode *bc, uint32_t pos, StackVal val) {
    assert(_contexts.size() > 1);
    uint16_t ctxId = bc->getInt16(pos);
    pos += 2;
    uint16_t varId = bc->getInt16(pos);
    assert(ctxId < _contexts.size());
    if(varId >= _contexts[ctxId].size()) {
        _contexts[ctxId].resize(varId + 1);
    }
    assert(varId < _contexts[ctxId].size());
    _contexts[ctxId][varId] = val;
}
