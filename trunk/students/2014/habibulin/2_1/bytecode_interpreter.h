#ifndef BYTECODE_INTERPRETER_H
#define BYTECODE_INTERPRETER_H

#include "mathvm.h"
#include "my_utils.h"

using namespace mathvm;

union StackVal {
    int64_t vInt;
    double vDouble;

    StackVal(): vInt(0) {} // this one is just to use resize
    StackVal(int64_t vi): vInt(vi) {}
    StackVal(double vd): vDouble(vd) {}
};
typedef vector<StackVal> Context;

class BytecodeStream {
    vector<Bytecode*> _bcs;
    vector<uint32_t> _positions;

public:
    vector<Context> contexts;

    void pushBc(Bytecode* bc) {
        _bcs.push_back(bc);
        _positions.push_back(0);
        contexts.push_back(Context());
    }

    void popBc() {
        assert(!_bcs.empty() && _positions.size() == _bcs.size() && contexts.size() == _bcs.size());
        _bcs.pop_back();
        _positions.pop_back();
        contexts.pop_back();
    }

    bool hasNext() {
        assert(_positions.size() == _bcs.size());
        assert(_bcs.back()->length() >= _positions.back());
        if(_bcs.empty()) {
            return false;
        } else if(_bcs.size() == 1) {
            return _positions.back() != _bcs.back()->length();
        }
        return true;
    }

    Instruction nextInsn() {
        assert(hasNext());
        popIfNeeded();
        return _bcs.back()->getInsn(_positions.back()++);
    }

    int16_t nextInt16() {
        assert(hasNext());
        popIfNeeded();
        int16_t res = _bcs.back()->getInt16(_positions.back());
        _positions.back() += 2;
        return res;
    }

    int64_t nextInt64() {
        assert(hasNext());
        popIfNeeded();
        int64_t res = _bcs.back()->getInt64(_positions.back());
        _positions.back() += 8;
        return res;
    }

    double nextDouble() {
        assert(hasNext());
        popIfNeeded();
        double res = _bcs.back()->getDouble(_positions.back());
        _positions.back() += 8;
        return res;
    }

    uint16_t nextUInt16() {
        assert(hasNext());
        popIfNeeded();
        uint16_t res = _bcs.back()->getUInt16(_positions.back());
        _positions.back() += 2;
        return res;
    }

    void jump() {
        int16_t offs = nextInt16();
        _positions.back() -= 3;
        _positions.back() += (((int32_t) offs) + 1);
    }

private:
    inline void popIfNeeded() {
        if(_positions.back() == _bcs.back()->length()) {
            popBc();
        }
    }
};

class BytecodeInterpreter {
    Code* _code;

    BytecodeStream _bcStream;
    vector<StackVal> _programStack;
    ExecStatus _status;

    const string MAIN_FUN_NAME = "<top>";
    uint16_t EMP_STR_ID;

public:
    void interpret(Code* code);
    ExecStatus status() const { return _status; }

private:
    void execInsn(Instruction insn);

    pair<StackVal, StackVal> getOperands();
    pair<StackVal, StackVal> popOperands();

    void loadvar();
    void storeValueLocal(StackVal val);

    void loadctxvar();
    void storeValueGlobal(StackVal val);

    string invalidBcMsg(Instruction insn) {
        return "invalid bytecode instruction encountered: " + string(bytecodeName(insn));
    }
    string divByZeroMsg(string const& funName) {
        return "division by zero in func: " + funName;
    }
};

#endif // INTERPRETER_H
