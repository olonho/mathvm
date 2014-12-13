#include "bytecode_interpreter.h"

#include <fstream>
#include <cstdlib>

void BytecodeInterpreter::interpret(Code *code) {
    try {
        _code = code;
        EMP_STR_ID = _code->makeStringConstant("");

        BytecodeFunction* fun = (BytecodeFunction *)_code->functionByName(MAIN_FUN_NAME);
        assert(fun);
        assert(fun->bytecode());
        _bcStream.pushBc(fun->bytecode());
        while(_bcStream.hasNext()) {
            execInsn(_bcStream.nextInsn());
        }
    } catch (ExceptionWithMsg& e) {
        _status.setError(e.what(), 0);
    }
}

void BytecodeInterpreter::execInsn(Instruction insn) {
    switch (insn) {
    case BC_DLOAD:
        _programStack.push_back(StackVal(_bcStream.nextDouble()));
        return;
    case BC_ILOAD:
        _programStack.push_back(StackVal(_bcStream.nextInt64()));
        return;
    case BC_SLOAD:
        _programStack.push_back(StackVal((int64_t)_bcStream.nextUInt16()));
        return;
    case BC_DLOAD0:
        _programStack.push_back(StackVal((double) 0.0));
        assert(_programStack.back().vDouble == 0);
        return;
    case BC_ILOAD0:
        _programStack.push_back(StackVal((int64_t) 0));
        assert(_programStack.back().vInt == 0);
        return;
    case BC_SLOAD0:
        _programStack.push_back(StackVal((int64_t) EMP_STR_ID));
        assert(_programStack.back().vInt == (int64_t)EMP_STR_ID);
        return;
    case BC_DLOAD1:
        _programStack.push_back(StackVal((double) 1.0));
        assert(_programStack.back().vDouble == 1.0);
        return;
    case BC_ILOAD1:
        _programStack.push_back(StackVal((int64_t) 1));
        assert(_programStack.back().vInt == 1);
        return;
    case BC_DLOADM1:
        _programStack.push_back(StackVal((double) -1.0));
        assert(_programStack.back().vDouble == -1.0);
        return;
    case BC_ILOADM1:
        _programStack.push_back(StackVal((int64_t) -1));
        assert(_programStack.back().vInt == -1);
        return;
    case BC_DADD: {
        auto ops = popOperands();
        _programStack.push_back(StackVal(ops.first.vDouble + ops.second.vDouble));
        return;
    }
    case BC_IADD: {
        auto ops = popOperands();
        _programStack.push_back(StackVal(ops.first.vInt + ops.second.vInt));
        return;
    }
    case BC_DSUB: {
        auto ops = popOperands();
        _programStack.push_back(StackVal(ops.first.vDouble - ops.second.vDouble));
        return;
    }
    case BC_ISUB: {
        auto ops = popOperands();
        _programStack.push_back(StackVal(ops.first.vInt - ops.second.vInt));
        return;
    }
    case BC_DMUL: {
        auto ops = popOperands();
        _programStack.push_back(StackVal(ops.first.vDouble * ops.second.vDouble));
        return;
    }
    case BC_IMUL: {
        auto ops = popOperands();
        _programStack.push_back(StackVal(ops.first.vInt * ops.second.vInt));
        return;
    }
    case BC_DDIV: {
        auto ops = popOperands();
        if(ops.second.vDouble == 0) { throw ExceptionWithMsg(divByZeroMsg("")); }
        _programStack.push_back(StackVal(ops.first.vDouble / ops.second.vDouble));
        return;
    }
    case BC_IDIV: {
        auto ops = popOperands();
        if(ops.second.vInt == 0) { throw ExceptionWithMsg(divByZeroMsg("")); }
        _programStack.push_back(StackVal(ops.first.vInt / ops.second.vInt));
        return;
    }
    case BC_IMOD: {
        auto ops = popOperands();
        if(ops.second.vInt == 0) { throw ExceptionWithMsg(divByZeroMsg("")); }
        _programStack.push_back(StackVal(ops.first.vInt % ops.second.vInt));
        return;
    }
    case BC_DNEG:
        assert(!_programStack.empty());
        _programStack.back().vDouble = (-1.0) * _programStack.back().vDouble;
        return;
    case BC_INEG:
        assert(!_programStack.empty());
        _programStack.back().vInt = (-1) * _programStack.back().vInt;
        return;
    case BC_IAOR: {
        auto ops = popOperands();
        _programStack.push_back(StackVal(ops.first.vInt | ops.second.vInt));
        return;
    }
    case BC_IAAND: {
        auto ops = popOperands();
        _programStack.push_back(StackVal(ops.first.vInt & ops.second.vInt));
        return;
    }
    case BC_IAXOR: {
        auto ops = popOperands();
        _programStack.push_back(StackVal(ops.first.vInt ^ ops.second.vInt));
        return;
    }
    case BC_IPRINT:
        assert(!_programStack.empty());
        cout << _programStack.back().vInt;
        _programStack.pop_back();
        return;
    case BC_DPRINT:
        assert(!_programStack.empty());
        cout << _programStack.back().vDouble;
        _programStack.pop_back();
        return;
    case BC_SPRINT:
        assert(!_programStack.empty());
        cout << _code->constantById((uint16_t)_programStack.back().vInt);
        _programStack.pop_back();
        return;
    case BC_I2D:
        assert(!_programStack.empty());
        _programStack.back().vDouble = (double) _programStack.back().vInt;
        return;
    case BC_D2I:
        assert(!_programStack.empty());
        _programStack.back().vInt = (int64_t) _programStack.back().vDouble;
        return;
    case BC_SWAP:
        assert(_programStack.size() > 1);
        std::swap(_programStack.back(), _programStack[_programStack.size() - 2]);
        return;
    case BC_POP:
        _programStack.pop_back();
        return;
    case BC_LOADDVAR:
    case BC_LOADIVAR:
    case BC_LOADSVAR:
        loadvar();
        return;
    case BC_STOREDVAR:
        assert(!_programStack.empty());
        storeValueLocal(StackVal(_programStack.back().vDouble));
        _programStack.pop_back();
        return;
    case BC_STOREIVAR:
    case BC_STORESVAR:
        assert(!_programStack.empty());
        storeValueLocal(StackVal(_programStack.back().vInt));
        _programStack.pop_back();
        return;
    case BC_LOADCTXDVAR:
    case BC_LOADCTXIVAR:
    case BC_LOADCTXSVAR:
        loadctxvar();
        return;
    case BC_STORECTXDVAR:
        assert(!_programStack.empty());
        storeValueGlobal(StackVal(_programStack.back().vDouble));
        _programStack.pop_back();
        return;
    case BC_STORECTXIVAR:
    case BC_STORECTXSVAR:
        assert(!_programStack.empty());
        storeValueGlobal(StackVal(_programStack.back().vInt));
        _programStack.pop_back();
        return;
    case BC_DCMP: {
        auto ops = getOperands();
        if (ops.first.vDouble == ops.second.vDouble) {
            _programStack.push_back(StackVal((int64_t)0));
        } else if (ops.first.vDouble < ops.second.vDouble) {
            _programStack.push_back(StackVal((int64_t)-1));
        } else {
            _programStack.push_back(StackVal((int64_t)1));
        }
        return;
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
        return;
    }
    case BC_JA: {
        _bcStream.jump();
        return;
    }
    case BC_IFICMPNE:{
        auto ops = getOperands();
        if(ops.first.vInt != ops.second.vInt) {
            _bcStream.jump();
        } else {
            _bcStream.nextInt16();
        }
        return;
    }
    case BC_IFICMPE:{
        auto ops = getOperands();
        if(ops.first.vInt == ops.second.vInt) {
            _bcStream.jump();
        } else {
            _bcStream.nextInt16();
        }
        return;
    }
    case BC_IFICMPG:{
        auto ops = getOperands();
        if(ops.first.vInt > ops.second.vInt) {
            _bcStream.jump();
        } else {
            _bcStream.nextInt16();
        }
        return;
    }
    case BC_IFICMPGE:{
        auto ops = getOperands();
        if(ops.first.vInt >= ops.second.vInt) {
            _bcStream.jump();
        } else {
            _bcStream.nextInt16();
        }
        return;
    }
    case BC_IFICMPL:{
        auto ops = getOperands();
        if(ops.first.vInt < ops.second.vInt) {
            _bcStream.jump();
        } else {
            _bcStream.nextInt16();
        }
        return;
    }
    case BC_IFICMPLE:{
        auto ops = getOperands();
        if(ops.first.vInt <= ops.second.vInt) {
            _bcStream.jump();
        } else {
            _bcStream.nextInt16();
        }
        return;
    }
    case BC_CALL: {
        BytecodeFunction* fun = (BytecodeFunction *)_code->functionById(_bcStream.nextUInt16());
        assert(fun);
        assert(fun->bytecode());
        _bcStream.pushBc(fun->bytecode());
        return;
    }
    case BC_CALLNATIVE:
        assert(false);
        return;
    case BC_RETURN:
        _bcStream.popBc();
        return;
    default:
        throw ExceptionWithMsg(invalidBcMsg(insn));
    }
}

pair<StackVal, StackVal> BytecodeInterpreter::getOperands() {
    assert(_programStack.size() > 1);
    StackVal upper = _programStack.back();
    StackVal lower = _programStack[_programStack.size() - 2];
    return make_pair(upper, lower);
}

pair<StackVal, StackVal> BytecodeInterpreter::popOperands() {
    assert(_programStack.size() > 1);
    StackVal upper = _programStack.back();
    StackVal lower = _programStack[_programStack.size() - 2];
    _programStack.pop_back();
    _programStack.pop_back();
    return make_pair(upper, lower);
}

void BytecodeInterpreter::loadvar() {
    uint16_t varId = _bcStream.nextUInt16();
    assert(!_bcStream.contexts.empty());
    assert(_bcStream.contexts.back().size() > varId);
    StackVal sv = _bcStream.contexts.back()[varId];
    _programStack.push_back(sv);
}

void BytecodeInterpreter::storeValueLocal(StackVal val) {
    uint16_t varId = _bcStream.nextUInt16();
    assert(!_bcStream.contexts.empty());
    if(varId >= _bcStream.contexts.back().size()) {
        _bcStream.contexts.back().resize(varId + 1);
    }
    assert(_bcStream.contexts.back().size() > varId);
    _bcStream.contexts.back()[varId] = val;
}

void BytecodeInterpreter::loadctxvar() {
    assert(_bcStream.contexts.size() > 1);
    uint16_t ctxId = _bcStream.nextUInt16();
    uint16_t varId = _bcStream.nextUInt16();
    assert(ctxId < _bcStream.contexts.size());
    assert(varId < _bcStream.contexts[ctxId].size());
    StackVal sv = _bcStream.contexts[ctxId][varId];
    _programStack.push_back(sv);
}

void BytecodeInterpreter::storeValueGlobal(StackVal val) {
    assert(_bcStream.contexts.size() > 1);
    uint16_t ctxId = _bcStream.nextUInt16();
    uint16_t varId = _bcStream.nextUInt16();
    assert(ctxId < _bcStream.contexts.size());
    if(varId >= _bcStream.contexts[ctxId].size()) {
        _bcStream.contexts[ctxId].resize(varId + 1);
    }
    assert(varId < _bcStream.contexts[ctxId].size());
    _bcStream.contexts[ctxId][varId] = val;
}
