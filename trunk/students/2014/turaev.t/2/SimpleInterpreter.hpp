#ifndef __SIMPLEINTERPRETER_HPP_
#define __SIMPLEINTERPRETER_HPP_

#include "mathvm.h"
#include "InterpreterCodeImpl.hpp"
#include "logger.hpp"
#include "TypedVariable.hpp"
#include "Errors.hpp"

namespace mathvm {
    namespace detail {
        template<class T>
        inline int64_t _cmp(T const &a, T const &b) {
            // bc_swap should be
            if (a < b) {
                return 1;
            } else if (a > b) {
                return -1;
            } else return 0;
        }

        template<>
        inline int64_t _cmp(int64_t const &a, int64_t const &b) {
            return (a == b);
        }
    }

    class SimpleInterpreter : public InterpreterCodeImpl {

    public:
        virtual Status *execute(vector<Var *> &vars) override;

        TypedVariable const& loadVariable(uint16_t id) {
            return loadVariable(contextID.back(), id);
        }

    private:
        typedef vector<TypedVariable> Variables;
        typedef int64_t signedIntType;
        typedef uint64_t unsignedIntType;
        typedef uint32_t indexType;

        size_t SP;
        Variables stack;
        std::vector<Bytecode *> bytecodes;
        std::vector<indexType> indices;
        vector<vector<Variables>> vars; // contextID, recursiveID, variableID
        std::vector<unsignedIntType> contextID;
        std::vector<unsignedIntType> callsCounter; //by contextID = functionID

        void run(ostream &out);

        size_t bytecodeLength(Instruction instruction) {
            static const struct {
                Instruction insn;
                size_t length;
            } instructions[] = {
            #define BC_NAME(b, d, l) {BC_##b, l},
                    FOR_BYTECODES(BC_NAME)
            };

            if (instruction >= BC_INVALID && instruction < BC_LAST) {
                return instructions[instruction].length;
            }
            assert(0);
            return 0;
        }

        void callNativeFunctionViaTemplateMagic(void *f, size_t params, VarType returnType);

        void callNativeFunctionViaAsmJit(void *f, const Signature *, VarType returnType);

        void callNative(uint16_t id) {
            const Signature *signature;
            const std::string *name;
            void *nativeFunctionAddress = (void *) nativeById(id, &signature, &name);
            if (!nativeFunctionAddress) {
                throw InterpretationError("Native function not found");
            }

            size_t paramsCount = signature->size() - 1;
            VarType returnType = signature->at(0).first;
            if (paramsCount <= 4) {
                callNativeFunctionViaTemplateMagic(nativeFunctionAddress, paramsCount, returnType);
            } else {
                callNativeFunctionViaAsmJit(nativeFunctionAddress, signature, returnType);
            }
        }

        void detectCallWithFunctionID(unsignedIntType functionID) {
            while (callsCounter.size() <= functionID) {
                callsCounter.push_back(0);
            }
            callsCounter[functionID]++;
        }

        TypedVariable const &popVariable() {
            return stack[--SP];
        }

        void increaseStack() {
            if (SP == stack.size()) {
                stack.resize(SP * 2);
            }
        }

        void pushVariable(double value) {
            increaseStack();
            TypedVariable &var = stack[SP];
            var._type = VT_DOUBLE;
            var.setDoubleValue(value);
            ++SP;
        }

        void pushVariable(signedIntType value) {
            increaseStack();
            TypedVariable &var = stack[SP];
            var._type = VT_INT;
            var.setIntValue(value);
            ++SP;
        }

        void pushVariable(char const* value) {
            increaseStack();
            TypedVariable &var = stack[SP];
            var._type = VT_STRING;
            var.setStringValue(value);
            ++SP;
        }

        void pushVariable(TypedVariable const &v) {
            increaseStack();
            stack[SP++] = v;
        }

        TypedVariable const& loadVariable(unsignedIntType contextID, unsignedIntType variableID) {
            assert(vars.size() > contextID);
            assert(callsCounter.size() > contextID);
            assert(vars[contextID].size() > callsCounter[contextID]);
            assert(vars[contextID][callsCounter[contextID]].size() > variableID);
            return vars[contextID][callsCounter[contextID]][variableID];
        }

        void storeVariable(unsignedIntType id) {
            storeVariable(contextID.back(), id);
        }

        void storeVariable(unsignedIntType contextID, unsignedIntType id) {
            while (vars.size() <= contextID){
                vars.push_back(vector<Variables>());
            }
            while(callsCounter.size() <= contextID) {
                callsCounter.push_back(0);
            }

            vector<Variables> &currentRecursiveScope = vars[contextID];
            while (currentRecursiveScope.size() <= callsCounter[contextID]) {
                currentRecursiveScope.push_back(Variables());
            }

            Variables &local_vars = currentRecursiveScope[callsCounter[contextID]];
            while (local_vars.size() <= id) {
                local_vars.push_back(TypedVariable(VT_INT));
            }
            local_vars[id] = popVariable();
        }

        template<class T, class R = T>
        void binary_operation(VarType type, R (*binaryFunction)(T const &, T const &)) {
            auto left = popVariable();
            auto right = popVariable();

            T leftValue, rightValue;
            if (type == VT_DOUBLE) {
                leftValue = left.getDoubleValue();
                rightValue = right.getDoubleValue();
            }
            else {
                leftValue = left.getIntValue();
                rightValue = right.getIntValue();
            }

            pushVariable(binaryFunction(leftValue, rightValue));
        }

        template<class T>
        bool check_condition(bool (*comparer)(T const &, T const &)) {
            auto left = popVariable();
            auto right = popVariable();
            return comparer(right.getIntValue(), left.getIntValue());
        }

        template<class T>
        void unary_operation(VarType type, T (*unaryFunction)(T const &)) {
            auto var = popVariable();
            TypedVariable result(type);

            T value;
            if (type == (VT_DOUBLE)) {
                value = var.getDoubleValue();
            }
            else {
                value = var.getIntValue();
            }
            T resultValue = unaryFunction(value);
            if (type == VT_DOUBLE) {
                result.setDoubleValue(resultValue);
            }
            else {
                result.setIntValue(resultValue);
            }
            pushVariable(result);
        }

        template<class T>
        static T add(T const &a, T const &b) {
            return a + b;
        }

        template<class T>
        static T sub(T const &a, T const &b) {
            return a - b;
        }

        template<class T>
        static T mul(T const &a, T const &b) {
            return a * b;
        }

        template<class T>
        static T _div(T const &a, T const &b) {
            return a / b;
        }

        template<class T>
        static T mod(T const &a, T const &b) {
            return a % b;
        }

        template<class T>
        static T _or(T const &a, T const &b) {
            return a | b;
        }

        template<class T>
        static T _and(T const &a, T const &b) {
            return a & b;
        }

        template<class T>
        static T _xor(T const &a, T const &b) {
            return a ^ b;
        }

        template<class T>
        static T neg(T const &a) {
            return -a;
        }

        template<class T>
        static bool _eq(T const &a, T const &b) {
            return a == b;
        }

        template<class T>
        static signedIntType _cmp(T const &a, T const &b) {
            return detail::_cmp<T>(a, b);
        }

        template<class T>
        static bool _neq(T const &a, T const &b) {
            return a != b;
        }

        template<class T>
        static bool _g(T const &a, T const &b) {
            return a > b;
        }

        template<class T>
        static bool _ge(T const &a, T const &b) {
            return a >= b;
        }

        template<class T>
        static bool _l(T const &a, T const &b) {
            return a < b;
        }

        template<class T>
        static bool _le(T const &a, T const &b) {
            return a <= b;
        }
    };
};

#endif