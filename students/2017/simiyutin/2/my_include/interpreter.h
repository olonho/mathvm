#pragma once
#include "includes.h"
#include <vector>
#include <map>
#include "stack_frame.h"
#include "identity.h"
#include "native_caller.h"
#include "stack.h"
#include <dlfcn.h>

namespace detail {
    template <typename T>
    struct image;

    template <>
    struct image<int64_t> {
        using type = int64_t;
    };

    template <>
    struct image<double> {
        using type = double;
    };

    template <>
    struct image<std::string> {
        using type = uint16_t;
    };
}

struct Interpreter : mathvm::Code {

    Interpreter(const mathvm::Bytecode & bytecode,
             const std::map<std::string, int> & topMostVars,
             const std::vector<std::string> & stringConstants,
             const std::map<uint16_t, uint32_t> & functionOffsets,
             const std::map<uint16_t, std::pair<std::string, std::vector<mathvm::VarType>>> & nativeFunctions
    ) :
            bytecode(bytecode),
            topMostVars(topMostVars),
            stringConstants(stringConstants),
            functionOffsets(functionOffsets),
            nativeFunctions(nativeFunctions),
            nativeCaller(nativeFunctions),
            callStack({stack_frame(0, 0)}),
            contexts({{0, {0}}})
    {}

    mathvm::Status* execute(std::vector<mathvm::Var*>& vars) override;

    void disassemble(std::ostream& out = std::cout, mathvm::FunctionFilter* filter = 0) override;

private:

    template<typename T>
    void handleLoad() {
        handleLoad(identity<T>());
    }

    template <typename T>
    void handleLoad(identity<T>) {
        T val = bytecode.getTyped<T>(callStack.back().executionPoint);
        callStack.back().executionPoint += sizeof(T);
        stack.addTyped(val);
    }

    void handleLoad(identity<std::string>) {
        uint16_t stringTableIndex = bytecode.getInt16(callStack.back().executionPoint);
        callStack.back().executionPoint += sizeof(uint16_t);
        stack.addUInt16(stringTableIndex);
    }

    template <typename T>
    void handleLoad1() {
        T val = 1;
        stack.addTyped(val);
    }

    template <typename T>
    void handleLoad0() {
        T val = 0;
        stack.addTyped(val);
    }


    template <typename T>
    void handleAdd() {
        T fst = stack.getTyped<T>();
        T snd = stack.getTyped<T>();
        stack.addTyped(fst + snd);
    }

    template <typename T>
    void handleSub() {
        T fst = stack.getTyped<T>();
        T snd = stack.getTyped<T>();
        stack.addTyped(fst - snd);
    }

    template <typename T>
    void handleMul() {
        T fst = stack.getTyped<T>();
        T snd = stack.getTyped<T>();
        stack.addTyped(fst * snd);
    }

    template <typename T>
    void handleDiv() {
        T fst = stack.getTyped<T>();
        T snd = stack.getTyped<T>();
        stack.addTyped(fst / snd);
    }

    template <typename T>
    void handleMod() {
        T fst = stack.getTyped<T>();
        T snd = stack.getTyped<T>();
        stack.addTyped(fst % snd);
    }

    template <typename T>
    void handleAnd() {
        T fst = stack.getTyped<T>();
        T snd = stack.getTyped<T>();
        stack.addTyped(fst & snd);
    }

    template <typename T>
    void handleOr() {
        T fst = stack.getTyped<T>();
        T snd = stack.getTyped<T>();
        stack.addTyped(fst | snd);
    }

    template <typename T>
    void handleXor() {
        T fst = stack.getTyped<T>();
        T snd = stack.getTyped<T>();
        stack.addTyped(fst ^ snd);
    }

    template <typename T>
    void handleNeg() {
        T fst = stack.getTyped<T>();
        stack.addTyped(-fst);
    }

    template <typename FROM, typename TO>
    void handleCast() {
        handleCast(identity<FROM>(), identity<TO>());
    };

    template <typename FROM, typename TO>
    void handleCast(identity<FROM>, identity<TO>) {
        FROM val = stack.getTyped<FROM>();
        stack.addTyped(static_cast<TO>(val));
    };

    void handleCast(identity<std::string>, identity<int64_t>) {
        uint16_t string_id = stack.getTyped<uint16_t>();
        // bom bom, only int, not int64
        int64_t val = std::atoi(getString(string_id));
        stack.addTyped(val);
    };
    
    void handleIcmp() {
        int64_t top = stack.getInt64();
        int64_t bottom = stack.getInt64();
        int64_t less = top < bottom;
        stack.addTyped(less);
    }

//    template <typename T>
//    void handleStoreVar() {
//        handleStoreVar(identity<T>());
//    }
//
//    template <typename T>
//    void handleStoreVar(identity<T>) {
//        uint16_t varId = bytecode.getUInt16(callStack.back().executionPoint);
//        callStack.back().executionPoint += sizeof(uint16_t);
//        T val = stack.getTyped<T>();
////        std::cout << "--store ctx var @"
////                  << varId
////                  << "val="
////                  << val
////                  << std::endl;
//        getVarMap<T>()[varId] = val;
//    }
//
//    void handleStoreVar(identity<std::string>) {
//        uint16_t varId = bytecode.getUInt16(callStack.back().executionPoint);
//        callStack.back().executionPoint += sizeof(uint16_t);
//        uint16_t stringId = stack.getTyped<uint16_t>();
//        getVarMap<std::string>()[varId] = stringId;
//    }

    template <typename T>
    void handleStoreCtxVar() {
        handleStoreCtxVar(identity<T>());
    }

    template <typename T>
    void handleStoreCtxVar(identity<T>) {
        uint16_t contextId = bytecode.getUInt16(callStack.back().executionPoint);
        callStack.back().executionPoint += sizeof(uint16_t);
        uint16_t varId = bytecode.getUInt16(callStack.back().executionPoint);
        callStack.back().executionPoint += sizeof(uint16_t);
        T val = stack.getTyped<T>();
        getVarMap<T>(contextId)[varId] = val;
    }

    void handleStoreCtxVar(identity<std::string>) {
        uint16_t contextId = bytecode.getUInt16(callStack.back().executionPoint);
        callStack.back().executionPoint += sizeof(uint16_t);
        uint16_t varId = bytecode.getUInt16(callStack.back().executionPoint);
        callStack.back().executionPoint += sizeof(uint16_t);
        uint16_t stringId = stack.getTyped<uint16_t>();
        getVarMap<std::string>(contextId)[varId] = stringId;
    }

    template <typename T>
    void handleStoreVar0() {
        handleStoreRegister(identity<T>(), 0);
    }

    template <typename T>
    void handleStoreVar1() {
        handleStoreRegister(identity<T>(), 1);
    }

    template <typename T>
    void handleStoreRegister(identity<T>, uint8_t reg_id) {
        using typeImage = typename detail::image<T>::type;
        auto val = stack.getTyped<typeImage>();
        getRegister<typeImage>()[reg_id] = val;
    }

    template <typename T>
    void handleLoadVar0() {
        handleLoadRegister(identity<T>(), 0);
    }

    template <typename T>
    void handleLoadVar1() {
        handleLoadRegister(identity<T>(), 1);
    }

    template <typename T>
    void handleLoadRegister(identity<T>, uint8_t reg_id) {
        using typeImage = typename detail::image<T>::type;
        typeImage val = getRegister<typeImage>()[reg_id];
        stack.addTyped(val);
    }




//    template <typename T>
//    void handleLoadVar() {
//        handleLoadVar(identity<T>());
//    }
//
//    template <typename T>
//    void handleLoadVar(identity<T>) {
//        uint16_t varId = bytecode.getUInt16(callStack.back().executionPoint);
//        callStack.back().executionPoint += sizeof(uint16_t);
//        auto& varMap = getVarMap<T>();
//        auto it = varMap.find(varId);
//        if (it == varMap.end()) {
//            std::cout << callStack.back().executionPoint << ": ERROR: no such variable found: id = " << varId << std::endl;
//            exit(42);
//        }
//        T val = it->second;
//        stack.addTyped(val);
//    }
//
//    void handleLoadVar(identity<std::string>) {
//        uint16_t varId = bytecode.getUInt16(callStack.back().executionPoint);
//        callStack.back().executionPoint += sizeof(uint16_t);
//        auto & varMap = getVarMap<std::string>();
//        auto it = varMap.find(varId);
//        if (it == varMap.end()) {
//            std::cout << callStack.back().executionPoint <<  ": ERROR: no such variable found: id = " << varId << std::endl;
//            exit(42);
//        }
//        uint16_t stringId = it->second;
//        stack.addUInt16(stringId);
//    }

    template <typename T>
    void handleLoadCtxVar() {
        uint16_t contextId = bytecode.getUInt16(callStack.back().executionPoint);
        callStack.back().executionPoint += sizeof(uint16_t);
        uint16_t varId = bytecode.getUInt16(callStack.back().executionPoint);
        callStack.back().executionPoint += sizeof(uint16_t);
        // переменная неявно инициализируется дефолтным значением, см. вызов bar в тесте complex
        typename detail::image<T>::type val = getVarMap<T>(contextId)[varId];
        stack.addTyped(val);
    }

    template <typename T>
    void handlePrint() {
        handlePrint(identity<T>());
    }

    template <typename T>
    void handlePrint(identity<T>) {
        T el = stack.getTyped<T>();
        std::cout << el;
    }

    void handlePrint(identity<std::string>) {
        uint16_t id = stack.getTyped<uint16_t>();
        std::cout << getString(id);
    }

    void handleCmpge() {
        handleConditionalJump([](int upper, int lower){ return upper >= lower;});
    }

    void handleCmple() {
        handleConditionalJump([](int upper, int lower){ return upper <= lower;});
    }

    void handleCmpg() {
        handleConditionalJump([](int upper, int lower){ return upper > lower;});
    }

    void handleCmpl() {
        handleConditionalJump([](int upper, int lower){ return upper < lower;});
    }

    void handleCmpe() {
        handleConditionalJump([](int upper, int lower){ return upper == lower;});
    }

    void handleCmpne() {
        handleConditionalJump([](int upper, int lower){ return upper != lower;});
    }

    template <typename FUNCTOR>
    void handleConditionalJump(const FUNCTOR & f) {
        int16_t shift = bytecode.getInt16(callStack.back().executionPoint);

        int64_t upper = stack.getTyped<int64_t>();
        int64_t lower = stack.getTyped<int64_t>();
        if (f(upper, lower)) {
            callStack.back().executionPoint += shift;
        } else {
            callStack.back().executionPoint += sizeof(int16_t);
        }
    }

    void handleJa() {
        int16_t shift = bytecode.getInt16(callStack.back().executionPoint);
        callStack.back().executionPoint += shift;
    }

    //assume swapping integers
    void handleSwap() {
        int64_t upper = stack.getTyped<int64_t>();
        int64_t lower = stack.getTyped<int64_t>();
        stack.addInt64(upper);
        stack.addInt64(lower);
    }

    //assume popping integers
    void handlePop() {
        int64_t upper = stack.getTyped<int64_t>();
        (void) upper;
    }
    
    void handleCall() {
        uint16_t function_id = bytecode.getUInt16(callStack.back().executionPoint);
        callStack.back().executionPoint += sizeof(uint16_t);
        callStack.back().stack_size = stack.length();
        contexts[function_id].push_back(callStack.size());
        uint32_t new_execution_point = functionOffsets.find(function_id)->second;
        callStack.push_back(stack_frame(new_execution_point, int64_t(function_id)));
    }

    void handleCallNative() {
        uint16_t function_id = bytecode.getUInt16(callStack.back().executionPoint);
        callStack.back().executionPoint += sizeof(uint16_t);
        nativeCaller.call(function_id, stack, stringConstants, dynamicStrings);
    }

    void handleReturn() {
        int64_t function_id = callStack.back().function_id;
        contexts[function_id].pop_back();
        callStack.pop_back();

        int64_t new_stack_size = stack.length();
        int64_t old_stack_size = callStack.back().stack_size;
        if (new_stack_size - old_stack_size > (int) sizeof(int64_t)) {
            std::cout << "STACK SANITIZER ERROR: BEFORE: " << old_stack_size << " AFTER: " << new_stack_size
                      << std::endl;
            exit(42);
        }
    }

    template <typename T>
    std::map<int, typename detail::image<T>::type> & getVarMap(uint16_t context) {
        size_t stackframe_id = contexts[context].back();
        return callStack[stackframe_id].getVarMap(identity<T>());
    };

    template <typename T>
    T* getRegister() {
        static T registers[] = {0, 0, 0, 0};
        return registers;
    }

    const char * getString(uint16_t string_id) {
        if (string_id < stringConstants.size()) {
            const std::string & constant = stringConstants[string_id];
            const char * data = constant.c_str();
            return data;
        } else {
            char * data = dynamicStrings[string_id];
            return data;
        }
    }

    mathvm::Bytecode bytecode;
    mathvm::Stack stack;
    std::map<std::string, int> topMostVars;
    std::vector<std::string> stringConstants;
    std::map<uint16_t, char *> dynamicStrings;
    const std::map<uint16_t , uint32_t> functionOffsets;
    std::map<uint16_t, std::pair<std::string, std::vector<mathvm::VarType>>> nativeFunctions;
    mathvm::native_caller nativeCaller;
    std::vector<stack_frame> callStack;
    std::map<uint16_t, std::vector<size_t>> contexts;
};