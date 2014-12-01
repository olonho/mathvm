#include "bcinterpreter.h"

#include "my_utils.h"

shared_ptr<Status> BcInterpreter::interpret(Code *code) {
    DEBUG_MSG("interpretation starts");
    _code = code;
    TranslatedFunction* fun = _code->functionByName(MAIN_FUN_NAME);
    assert(fun != 0);
    _ctxStack.push_back(InterpContext(fun->id()));
    interpFun(static_cast<BytecodeFunction*>(fun));

    DEBUG_MSG("interpretation finished");
    return shared_ptr<Status>(Status::Error("not implemented yet"));
}

void BcInterpreter::interpFun(BytecodeFunction* fun) {
    Bytecode* funBc = fun->bytecode();
    for(uint32_t i = 0; i != funBc->length();) {
        i += execInsn(funBc, i);
    }
}

int32_t BcInterpreter::execInsn(Bytecode* bc, uint32_t pos) {
    Instruction insn = bc->getInsn(pos);
    DEBUG_MSG("execInsn: " + string(bytecodeName(insn)));
    switch (insn) {
    case BC_DLOAD:
        return dload(bc, ++pos);
    case BC_ILOAD:
        return iload(bc, ++pos);
    case BC_SLOAD:
        return sload(bc, ++pos);
    case BC_DLOAD0:
        return pushReturn(StackVal((double) 0.0), 1);
    case BC_ILOAD0:
        return pushReturn(StackVal((int64_t) 0), 1);
    case BC_SLOAD0:
        return pushReturn(StackVal((int64_t) _code->makeStringConstant("")), 1);
    case BC_DLOAD1:
        return pushReturn(StackVal((double) 1.0), 1);
    case BC_ILOAD1:
        return pushReturn(StackVal((int64_t)1), 1);
    case BC_DLOADM1:
        return pushReturn(StackVal((double)-1.0), 1);
    case BC_ILOADM1:
        return pushReturn(StackVal((int64_t)-1), 1);
    case BC_DADD: {
        pair<double, double> dOps = popOperandsAsDoubles();
        return pushReturn(StackVal(dOps.first + dOps.second), 1);
    }
    case BC_IADD: {
        auto iOps = popOperandsAsInts();
        return pushReturn(StackVal(iOps.first + iOps.second), 1);
    }
    case BC_DSUB: {
        auto dOps = popOperandsAsDoubles();
        return pushReturn(StackVal(dOps.first - dOps.second), 1);
    }
    case BC_ISUB: {
        auto iOps = popOperandsAsInts();
        return pushReturn(StackVal(iOps.first - iOps.second), 1);
    }
    case BC_DMUL: {
        auto dOps = popOperandsAsDoubles();
        return pushReturn(StackVal(dOps.first * dOps.second), 1);
    }
    case BC_IMUL: {
        auto iOps = popOperandsAsInts();
        return pushReturn(StackVal(iOps.first * iOps.second), 1);
    }
    case BC_DDIV: {
        auto dOps = popOperandsAsDoubles();
        if(dOps.second == 0) {
            throw ExceptionWithMsg(divByZeroMsg(""));
        }
        return pushReturn(StackVal(dOps.first / dOps.second), 1);
    }
    case BC_IDIV: {
        auto iOps = popOperandsAsInts();
        if(iOps.second == 0) {
            throw ExceptionWithMsg(divByZeroMsg(""));
        }
        return pushReturn(StackVal(iOps.first / iOps.second), 1);
    }
    case BC_IMOD: {
        auto iOps = popOperandsAsInts();
        if(iOps.second == 0) {
            throw ExceptionWithMsg(divByZeroMsg(""));
        }
        return pushReturn(StackVal(iOps.first % iOps.second), 1);
    }
    case BC_DNEG:
        return updateTopReturn(StackVal(_programStack.back().vDouble * (-1.0)));
    case BC_INEG:
        return updateTopReturn(StackVal(_programStack.back().vInt * (-1)));
    case BC_IAOR: {
        auto iOps = popOperandsAsInts();
        return pushReturn(StackVal(iOps.first | iOps.second), 1);
    }
    case BC_IAAND: {
        auto iOps = popOperandsAsInts();
        return pushReturn(StackVal((int64_t)(iOps.first & iOps.second)), 1);
    }
    case BC_IAXOR: {
        auto iOps = popOperandsAsInts();
        return pushReturn(StackVal(iOps.first ^ iOps.second), 1);
    }
    case BC_IPRINT:
        cout << _programStack.back().vInt;
        return popReturn();
    case BC_DPRINT:
        cout << _programStack.back().vDouble;
        return popReturn();
    case BC_SPRINT:
        cout << _code->constantById((uint16_t)_programStack.back().vInt);
        return popReturn();
    case BC_I2D:
        return updateTopReturn(StackVal((double)_programStack.back().vInt));
    case BC_D2I:
        return updateTopReturn(StackVal((int64_t)_programStack.back().vDouble));
    case BC_SWAP:
        std::swap(_programStack.back(), _programStack[_programStack.size() - 2]);
        return 1;
    case BC_POP:
        return popReturn();
    case BC_LOADDVAR:
    case BC_LOADIVAR:
    case BC_LOADSVAR:
        return loadvar(bc, ++pos);
    case BC_STOREDVAR:
        return storedvar(bc, ++pos);
    case BC_STOREIVAR:
    case BC_STORESVAR:
        return storeivar(bc, ++pos);
    case BC_LOADCTXDVAR:
    case BC_LOADCTXIVAR:
    case BC_LOADCTXSVAR:
        return loadctxvar(bc, ++pos);
    case BC_STORECTXDVAR:
        return storectxdvar(bc, ++pos);
    case BC_STORECTXIVAR:
    case BC_STORECTXSVAR:
        return storectxivar(bc, ++pos);
    case BC_DCMP: {
        auto dOps = getOperandsAsDoubles();
        if (dOps.first == dOps.second)     {
            _programStack.push_back(StackVal((int64_t)0));
            DEBUG_MSG("prog stack PUSH: " + to_string(0));
        } else if (dOps.first < dOps.second) {
            _programStack.push_back(StackVal((int64_t)-1));
            DEBUG_MSG("prog stack PUSH: " + to_string(-1));
        } else {
            _programStack.push_back(StackVal((int64_t)1));
            DEBUG_MSG("prog stack PUSH: " + to_string(1));
        }
        return 1;
    }
    case BC_ICMP: {
        auto iOps = getOperandsAsInts();
        if (iOps.first == iOps.second) {
            _programStack.push_back(StackVal((int64_t)0));
            DEBUG_MSG("prog stack PUSH: " + to_string(0));
        } else if (iOps.first < iOps.second) {
            _programStack.push_back(StackVal((int64_t)-1));
            DEBUG_MSG("prog stack PUSH: " + to_string(-1));
        } else {
            _programStack.push_back(StackVal((int64_t)1));
            DEBUG_MSG("prog stack PUSH: " + to_string(1));
        }
        return 1;
    }
    case BC_JA: {
        return ((int32_t) bc->getInt16(++pos)) + 1;
    }
    case BC_IFICMPNE:{
        auto iOps = getOperandsAsInts();
        if(iOps.first != iOps.second) {
            return ((int32_t) bc->getInt16(++pos)) + 1;
        }
        return 3;
    }
    case BC_IFICMPE:{
        auto iOps = getOperandsAsInts();
        if(iOps.first == iOps.second) {
            return ((int32_t) bc->getInt16(++pos)) + 1;
        }
        return 3;
    }
    case BC_IFICMPG:{
        auto iOps = getOperandsAsInts();
        if(iOps.first > iOps.second) {
            return ((int32_t) bc->getInt16(++pos)) + 1;
        }
        return 3;
    }
    case BC_IFICMPGE:{
        auto iOps = getOperandsAsInts();
        if(iOps.first >= iOps.second) {
            return ((int32_t) bc->getInt16(++pos)) + 1;
        }
        return 3;
    }
    case BC_IFICMPL:{
        auto iOps = getOperandsAsInts();
        if(iOps.first < iOps.second) {
            return ((int32_t) bc->getInt16(++pos)) + 1;
        }
        return 3;
    }
    case BC_IFICMPLE:{
        auto iOps = getOperandsAsInts();
        if(iOps.first <= iOps.second) {
            return ((int32_t) bc->getInt16(++pos)) + 1;
        }
        return 3;
    }
    case BC_CALL: {
        TranslatedFunction* fun = _code->functionById(bc->getUInt16(++pos));
        assert(fun != 0);
        _ctxStack.push_back(InterpContext(fun->id()));
        interpFun(static_cast<BytecodeFunction*>(fun));
        _ctxStack.pop_back();
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
