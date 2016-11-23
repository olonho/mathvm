#pragma once

#include "mathvm.h"
#include "include/ex/execution_exception.h"

#include <vector>
#include "data_holder.h"
#include "local_variable_storage.h"

using namespace mathvm;

class Interpreter {
public:
    typedef std::vector<DataHolder> DataStack;
    typedef std::pair<DataHolder, DataHolder> HolderPair;
    typedef std::vector<int32_t> CounterStack;
    typedef std::vector<TranslatedFunction *> FunctionStack;

    const size_t preallocatedMemorySize = 1024 * 1024;

    Interpreter(Code *code)
            : code(code),
              defaultStringId(code->makeStringConstant("")), localStorage(defaultStringId) {
        preallocateMemory();
        auto topFunction = code->functionByName(topFunctionName);
        functionStack.push_back(topFunction);
        newCounter();
        newContext(topFunction->localsNumber());
    }


    void execute() {
        // code->disassemble(std::cout);
        while (!functionStack.empty()) {
            executeInstruction();
        }
    }

private:
    void preallocateMemory() {
        functionStack.reserve(preallocatedMemorySize);
        programCounterStack.reserve(preallocatedMemorySize);
        dataStack.reserve(preallocatedMemorySize);
        localVariableStack.reserve(preallocatedMemorySize);
    }

    void executeInstruction();

    void call() {
        int16_t id = int16();
        functionStack.push_back(code->functionById(id));
        // currentFunction()->disassemble(std::cout);
        newCounter();
        newContext(currentFunction()->localsNumber());
    }

    void ja() {
        int16_t relativeOffset = currentBytecode()->getInt16(programCounter());
        addRelativeOffset(relativeOffset);
    }

    void ificmpne() {
        auto args = topPair();
        if (args.first.intValue != args.second.intValue) {
            ja();
        } else {
            int16();
        }
    }

    void ificmpe() {
        auto args = topPair();
        if (args.first.intValue == args.second.intValue) {
            ja();
        } else {
            int16();
        }
    }

    void ificmpg() {
        auto args = topPair();
        push(args.second);
        if (args.first.intValue > args.second.intValue) {
            ja();
        } else {
            int16();
        }
    }

    void ificmpge() {
        auto args = topPair();
        push(args.second);
        if (args.first.intValue >= args.second.intValue) {
            ja();
        } else {
            int16();
        }
    }

    void ificmpl() {
        auto args = topPair();
        push(args.second);
        if (args.first.intValue < args.second.intValue) {
            ja();
        } else {
            int16();
        }
    }

    void ificmple() {
        auto args = topPair();
        push(args.second);
        if (args.first.intValue <= args.second.intValue) {
            ja();
        } else {
            int16();
        }
    }

    void loadivar() {
        int16_t id = int16();
        ipush(localStorage.iload(id));
    }

    void loaddvar() {
        int16_t id = int16();
        dpush(localStorage.dload(id));
    }

    void loadsvar() {
        int16_t id = int16();
        spush(localStorage.sload(id));
    }

    void storeivar() {
        int16_t id = int16();
        auto arg = tos();
        localStorage.istore(id, arg.intValue);
    }

    void storedvar() {
        int16_t id = int16();
        auto arg = tos();
        localStorage.dstore(id, arg.doubleValue);
    }

    void storesvar() {
        int16_t id = int16();
        auto arg = tos();
        localStorage.sstore(id, arg.stringId);
    }

    void loadctxivar() {
        int16_t ctxId = int16();
        int16_t id = int16();
        ipush(localStorage.iload(ctxId, id));
    }

    void loadctxdvar() {
        int16_t ctxId = int16();
        int16_t id = int16();
        dpush(localStorage.dload(ctxId, id));
    }

    void loadctxsvar() {
        int16_t ctxId = int16();
        int16_t id = int16();
        spush(localStorage.sload(ctxId, id));
    }

    void storectxivar() {
        int16_t ctxId = int16();
        int16_t id = int16();
        auto arg = tos();
        localStorage.istore(ctxId, id, arg.intValue);
    }

    void storectxdvar() {
        int16_t ctxId = int16();
        int16_t id = int16();
        auto arg = tos();
        localStorage.dstore(ctxId, id, arg.doubleValue);
    }

    void storectxsvar() {
        int16_t ctxId = int16();
        int16_t id = int16();
        auto arg = tos();
        localStorage.sstore(ctxId, id, arg.stringId);
    }

    void dload() {
        dpush(currentBytecode()->getDouble(programCounter()));
        next8();
    }

    void iload() {
        ipush(currentBytecode()->getInt64(programCounter()));
        next8();
    }

    void sload() {
        int16_t id = int16();
        spush(id);
    }

    void dload0() {
        dpush((double) 0.0);
    }

    void iload0() {
        ipush((int64_t) 0);
    }

    void sload0() {
        int16_t id = code->makeStringConstant("");
        spush(id);
    }

    void iload1() {
        ipush((int64_t) 1);
    }

    void dload1() {
        dpush((double) 1.0);
    }

    void iloadm1() {
        ipush((int64_t) -1);
    }

    void dloadm1() {
        dpush((double) -1.0);
    }

    void iadd() {
        auto args = topPair();
        ipush(args.first.intValue + args.second.intValue);
    }

    void dadd() {
        auto args = topPair();
        dpush(args.first.doubleValue + args.second.doubleValue);
    }

    void isub() {
        auto args = topPair();
        ipush(args.first.intValue - args.second.intValue);
    }

    void dsub() {
        auto args = topPair();
        dpush(args.first.doubleValue - args.second.doubleValue);
    }

    void imul() {
        auto args = topPair();
        ipush(args.first.intValue * args.second.intValue);
    }

    void dmul() {
        auto args = topPair();
        dpush(args.first.doubleValue * args.second.doubleValue);
    }

    void idiv() {
        auto args = topPair();
        ipush(args.first.intValue / args.second.intValue);
    }

    void ddiv() {
        auto args = topPair();
        dpush(args.first.doubleValue / args.second.doubleValue);
    }

    void imod() {
        auto args = topPair();
        ipush(args.first.intValue % args.second.intValue);
    }

    void ineg() {
        auto args = tos();
        ipush((int64_t) -args.intValue);
    }

    void dneg() {
        auto args = tos();
        dpush(-args.doubleValue);
    }

    void iaor() {
        auto args = topPair();
        ipush(args.first.intValue | args.second.intValue);
    }

    void iaand() {
        auto args = topPair();
        ipush(args.first.intValue & args.second.intValue);
    }

    void iaxor() {
        auto args = topPair();
        ipush(args.first.intValue ^ args.second.intValue);
    }

    void iprint() {
        auto args = tos();
        printf("%ld", args.intValue);
    }

    void dprint() {
        auto args = tos();
        printf("%g", args.doubleValue);
    }

    void sprint() {
        auto args = tos();
        printf("%s", code->constantById(args.stringId).c_str());
    }

    void i2d() {
        auto args = tos();
        dpush((double) args.intValue);
    }

    void d2i() {
        auto args = tos();
        ipush((double) args.doubleValue);
    }

    void bc_return() {
        functionStack.pop_back();
        programCounterStack.pop_back();
        popContext();
    }

    void swap() {
        std::iter_swap(dataStack.rbegin(), dataStack.rbegin() + 1);
    }

    void icmp() {
        auto args = topPair();
        if (args.first.intValue == args.second.intValue) {
            ipush((int64_t) 0);
        } else if (args.first.intValue < args.second.intValue) {
            ipush((int64_t) -1);
        } else {
            ipush((int64_t) 1);
        }
    }

    void dcmp() {
        auto args = topPair();
        if (args.first.doubleValue == args.second.doubleValue) {
            ipush((int64_t) 0);
        } else if (args.first.doubleValue < args.second.doubleValue) {
            ipush((int64_t) -1);
        } else {
            ipush((int64_t) 1);
        }
    }

    void dump() {
        auto args = top();
        printf("%d\n", args.stringId);
        printf("%ld\n", args.intValue);
        printf("%f\n", args.doubleValue);
    }

    int32_t programCounter() { return programCounterStack.back(); }

    int32_t stackFrame() { return stackFramePointerStack.back(); }

    void push(DataHolder holder) {
        dataStack.push_back(holder);
    }

    void spush(int16_t value) {
        dataStack.push_back(DataHolder(value));
    }

    void ipush(int64_t value) {
        dataStack.push_back(DataHolder(value));
    }

    void dpush(double value) {
        dataStack.push_back(DataHolder(value));
    }

    void pop() {
        dataStack.pop_back();
    }

    DataHolder top() { return dataStack.back(); }

    HolderPair topPair() {
        HolderPair result;
        result.first = tos();
        result.second = tos();
        return result;
    }

    DataHolder tos() {
        auto result = top();
        pop();
        return result;
    }

    Bytecode *currentBytecode() {
        return static_cast<BytecodeFunction *>(currentFunction())->bytecode();
    }

    TranslatedFunction *currentFunction() { return functionStack.back(); }

    void newContext(int32_t localsNumber) {
        localStorage.newContext(localsNumber);
    }

    void popContext() {
        localStorage.popContext();
    }

    void addRelativeOffset(int16_t relativeOffset) {
        programCounterStack.back() += relativeOffset;
    }

    int16_t int16() {
        int16_t result = currentBytecode()->getInt16(programCounter());
        next2();
        return result;
    }

    void next(size_t i = 1) {
        programCounterStack.back() += i;
    }

    void next2() { next(2); }

    void next8() { next(8); }

    void newCounter() { programCounterStack.push_back(0); }

    const std::string topFunctionName = "<top>";

    CounterStack programCounterStack;
    // Maintains the offset of stack frames
    CounterStack stackFramePointerStack;
    FunctionStack functionStack;
    Code *code;
    DataStack dataStack;
    DataStack localVariableStack;
    const int16_t defaultStringId;
    LocalVariableStorage localStorage;
};
