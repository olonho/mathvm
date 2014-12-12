#ifndef INTERPRETERCODEIMPL_H
#define INTERPRETERCODEIMPL_H
#include "mathvm.h"
#include "ast.h"
#include <vector>
#include <iostream>
#include <map>
#include <list>

namespace mathvm {

union StackValue {
    double doubleValue;
    int64_t intValue;
    const char* stringValue;


    StackValue() : intValue(0L) {}
    StackValue(double val) : doubleValue(val) {}
    StackValue(int64_t val) : intValue(val) {}
    StackValue(const char* val) : stringValue(val) {}

    operator double() const { return doubleValue; }
    operator int64_t() const { return intValue; }
    operator const char*() const { return stringValue; }

    void doConverse(Instruction conv);
};

struct CallFrame {
    uint32_t pc;
    uint32_t funId;
    StackValue *ctx;
    StackValue *prevCallFunCtx;
};

class InterpreterCodeImpl : public Code, public ErrorInfoHolder {
    static const size_t MAX_STACK_SIZE;

    std::vector<StackValue> m_stack;
    std::vector<StackValue> m_context;
    std::vector<CallFrame> m_callStack;
    std::vector<StackValue *> m_funIdToCtx;
    StackValue *m_currentCtx;
    uint32_t m_pc;
    uint32_t m_funId;
    StackValue *m_tos;
    uint32_t m_currentLocalNum;
    StackValue **m_fun_map_start;
    Bytecode *m_bytecode;

    void pushToStack(StackValue val) { *m_tos++ = val; }
    StackValue popFromStack() { return *(--m_tos); }
    void loadLocalVar(uint16_t id) { pushToStack(*(m_currentCtx + id)); }
    void storeLocalVar(uint16_t id) { *(m_currentCtx + id) = popFromStack(); }
    void doConverse(Instruction conv) { (m_tos - 1)->doConverse(conv); }
    void dumpTos() { std::cout << (int64_t)*m_tos; }

    template<class T> void doBinaryOp(TokenKind op) {
        T u = popFromStack();
        T l = popFromStack();
        switch (op) {
            case tADD: pushToStack(u + l); return;
            case tSUB: pushToStack(u - l); return;
            case tDIV : pushToStack(u / l); return;
            case tMUL : pushToStack(u * l); return;
            default : break;
        }
        assert(false);
    }

    template<class T> void doPrint() {
        T val = popFromStack();
        std::cout << val;
    }

    template<class T> void cmp() {
        T u = popFromStack();
        T l = popFromStack();
        if (u > l)
            pushToStack(1LL);
        else if (u < l)
            pushToStack(-1LL);
        else
            pushToStack(0LL);
    }

    template<class T> T getNext() {
        T val = m_bytecode->getTyped<T>(m_pc);
        m_pc += sizeof(T);
        return val;
    }

    void doCompare(TokenKind op);
    void doCall(uint16_t funId, BytecodeFunction *fun = 0);
    void doCallNative(uint16_t funId);
    void doReturn();
    void doIntBinaryOp(TokenKind op);
    void loadContextVar();
    void storeContextVar();
    void startExec();
    const char * nextString();

public:
    InterpreterCodeImpl();
    ~InterpreterCodeImpl() {}
    void error(const char* msg, ...);
    virtual Status* execute(vector<Var*>& vars);
};

}

#endif // INTERPRETERCODEIMPL_H
