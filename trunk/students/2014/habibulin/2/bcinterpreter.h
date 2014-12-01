#ifndef BCINTERPRETER_H
#define BCINTERPRETER_H

#include "mathvm.h"
#include "varscontext.h"
#include "my_utils.h"

using namespace mathvm;

class BcInterpreter {
    Code* _code;
    vector<StackVal> _programStack;
    vector<InterpContext> _ctxStack;

    const string MAIN_FUN_NAME = "<top>";

public:
    BcInterpreter() {}

    shared_ptr<Status> interpret(Code* code);

private:
    void interpFun(BytecodeFunction* fun);

    /*
     * Executes instruction from bc at pos if possible and returns
     * the offset for the next pos
     */
    int32_t execInsn(Bytecode* bc, uint32_t pos);

    inline uint8_t iload(Bytecode* bc, uint32_t pos) {
        _programStack.push_back(StackVal(bc->getInt64(pos)));
        DEBUG_MSG("prog stack PUSH: " + to_string(bc->getInt64(pos)));
        return 9;
    }

    inline uint8_t dload(Bytecode* bc, uint32_t pos) {
        _programStack.push_back(StackVal(bc->getDouble(pos)));
        DEBUG_MSG("prog stack PUSH: " + to_string(bc->getDouble(pos)));
        return 9;
    }

    inline uint8_t sload(Bytecode* bc, uint32_t pos) {
        _programStack.push_back(StackVal((int64_t)bc->getInt16(pos)));
        DEBUG_MSG("prog stack PUSH: " + to_string((int64_t)bc->getInt16(pos)));
        return 3;
    }

    inline uint8_t pushReturn(StackVal stVal, uint8_t retVal) {
        _programStack.push_back(stVal);
        DEBUG_MSG("prog stack PUSH: " + to_string(stVal.vInt) + "(int), " + to_string(stVal.vDouble) + "(double)");
        return retVal;
    }

    inline pair<int64_t, int64_t> getOperandsAsInts() {
        int64_t upper = _programStack.back().vInt;
        int64_t lower = _programStack[_programStack.size() - 2].vInt;
        return make_pair(upper, lower);
    }

    inline pair<int64_t, int64_t> popOperandsAsInts() {
        pair<int64_t, int64_t> ops = getOperandsAsInts();
        _programStack.pop_back();
        _programStack.pop_back();
        DEBUG_MSG("prog stack POP 2: " + to_string(ops.first) + "(upper), " + to_string(ops.second) + "(lower)");
        return ops;
    }

    inline pair<double, double> getOperandsAsDoubles() {
        double upper = _programStack.back().vDouble;
        double lower = _programStack[_programStack.size() - 2].vDouble;
        return make_pair(upper, lower);
    }

    inline pair<double, double> popOperandsAsDoubles() {
        pair<double, double> ops = getOperandsAsDoubles();
        _programStack.pop_back();
        _programStack.pop_back();
        DEBUG_MSG("prog stack POP 2: " + to_string(ops.first) + "(upper), " + to_string(ops.second) + "(lower)");
        return ops;
    }

    inline uint8_t updateTopReturn(StackVal newSv) {
        _programStack.pop_back();
        _programStack.push_back(newSv);
        DEBUG_MSG("prog stack UPDATE: " + to_string(newSv.vInt) + "(int), " + to_string(newSv.vDouble) + "(double)");
        return 1;
    }

    inline uint8_t popReturn() {
        _programStack.pop_back();
        DEBUG_MSG("prog stack POP");
        return 1;
    }

    inline uint8_t loadvar(Bytecode* bc, uint32_t pos) {
        uint16_t varId = bc->getInt16(pos);
        shared_ptr<StackVal> svPtr = _ctxStack.back().get(varId).second;
        assert(svPtr);
        _programStack.push_back(*svPtr);
        DEBUG_MSG("prog stack PUSH: " + to_string(svPtr->vInt) + "(int), " + to_string(svPtr->vDouble) + "(double)");
        return 3;
    }

    inline uint8_t storedvar(Bytecode* bc, uint32_t pos) {
        uint16_t varId = bc->getInt16(pos);
        _ctxStack.back().update(varId, _programStack.back().vDouble);
        _programStack.pop_back();
        DEBUG_MSG("prog stack POP");
        return 3;
    }

    inline uint8_t storeivar(Bytecode* bc, uint32_t pos) {
        uint16_t varId = bc->getInt16(pos);
        _ctxStack.back().update(varId, _programStack.back().vInt);
        _programStack.pop_back();
        DEBUG_MSG("prog stack POP");
        return 3;
    }

    inline uint8_t loadctxvar(Bytecode* bc, uint32_t pos) {
        assert(_ctxStack.size() > 1);
        uint16_t ctxId = bc->getInt16(pos);
        pos += 2;
        uint16_t varId = bc->getInt16(pos);
        size_t ctxIndex = _ctxStack.size() - 2;
        while(_ctxStack[ctxIndex].id() != ctxId) { --ctxIndex; }
        shared_ptr<StackVal> svPtr = _ctxStack[ctxIndex].get(varId).second;
        assert(svPtr);
        _programStack.push_back(*svPtr);
        DEBUG_MSG("prog stack PUSH: " + to_string(svPtr->vInt) + "(int), " + to_string(svPtr->vDouble) + "(double)");
        return 5;
    }

    inline uint8_t storectxdvar(Bytecode* bc, uint32_t pos) {
        uint16_t ctxId = bc->getInt16(pos);
        pos += 2;
        uint16_t varId = bc->getInt16(pos);
        size_t ctxIndex = _ctxStack.size() - 2;
        while(_ctxStack[ctxIndex].id() != ctxId) { --ctxIndex; }
        _ctxStack[ctxIndex].update(varId, _programStack.back().vDouble);
        _programStack.pop_back();
        DEBUG_MSG("prog stack POP");
        return 5;
    }

    inline uint8_t storectxivar(Bytecode* bc, uint32_t pos) {
        uint16_t ctxId = bc->getInt16(pos);
        pos += 2;
        uint16_t varId = bc->getInt16(pos);
        size_t ctxIndex = _ctxStack.size() - 2;
        while(_ctxStack[ctxIndex].id() != ctxId) { --ctxIndex; }
        _ctxStack[ctxIndex].update(varId, _programStack.back().vInt);
        _programStack.pop_back();
        DEBUG_MSG("prog stack POP");
        return 5;
    }

    string invalidBcMsg(Instruction insn) {
        return "invalid bytecode instruction encountered: " + string(bytecodeName(insn));
    }

    string divByZeroMsg(string const& funName) {
        return "division by zero in func: " + funName;
    }
};

#endif // BCINTERPRETER_H
