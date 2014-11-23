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

        TypedVariable loadVariable(uint16_t id) {
            return loadVariable(contextID.back(), id);
        }

    private:
        typedef vector<TypedVariable> scope;
        typedef int64_t signedIntType;
        typedef uint64_t unsignedIntType;
        typedef uint32_t indexType;

        scope programStack;
        std::vector<Bytecode *> bytecodes;
        std::vector<indexType> indices;
        vector<vector<scope>> vars; // contextID, recursiveID, variableID
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

        TypedVariable popVariable() {
            TypedVariable back = programStack.back();
            programStack.pop_back();
            return back;
        }

        void pushVariable(double value) {
            TypedVariable var(VT_DOUBLE);
            var.setDoubleValue(value);
            programStack.push_back(var);
        }

        void pushVariable(signedIntType value) {
            TypedVariable var(VT_INT);
            var.setIntValue(value);
            programStack.push_back(var);
        }

        void pushVariable(char const* value) {
            TypedVariable var(VT_STRING);
            var.setStringValue(value);
            programStack.push_back(var);
        }

        TypedVariable loadVariable(unsignedIntType contextID, unsignedIntType variableID) {
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
                vars.push_back(vector<scope>());
            }
            while(callsCounter.size() <= contextID) {
                callsCounter.push_back(0);
            }

            vector<scope> &currentRecursiveScope = vars[contextID];
            while (currentRecursiveScope.size() <= callsCounter[contextID]) {
                currentRecursiveScope.push_back(scope());
            }

            scope &local_vars = currentRecursiveScope[callsCounter[contextID]];
            while (local_vars.size() <= id) {
                local_vars.push_back(TypedVariable(VT_INT));
            }
            local_vars[id] = programStack.back();
            programStack.pop_back();
        }

        template<class T, class R = T>
        void binary_operation(VarType type, R (*binaryFunction)(T const &, T const &)) {
            binary_operation<T, R>(type, binaryFunction, type);
        }

        template<class T, class R = T>
        void binary_operation(VarType type, R (*binaryFunction)(T const &, T const &), VarType resultType) {
            TypedVariable left = popVariable();
            TypedVariable right = popVariable();

            T leftValue = (type == VT_DOUBLE) ? (T) left.getDoubleValue() : (T) left.getIntValue();
            T rightValue = (type == VT_DOUBLE) ? (T) right.getDoubleValue() : (T) right.getIntValue();

            TypedVariable result(resultType);
            R resultValue = binaryFunction(leftValue, rightValue);
            resultType == VT_DOUBLE ? result.setDoubleValue(resultValue) : result.setIntValue(resultValue);
            programStack.push_back(result);
        }

        template<class T>
        bool check_condition(bool (*comparer)(T const &, T const &)) {
            TypedVariable left = popVariable();
            TypedVariable right = popVariable();
            return comparer(right.getIntValue(), left.getIntValue());
        }

        template<class T>
        void unary_operation(VarType type, T (*unaryFunction)(T const &)) {
            TypedVariable var = popVariable();
            TypedVariable result(type);

            T value = type == (VT_DOUBLE) ? (T) var.getDoubleValue() : (T) var.getIntValue();
            T resultValue = unaryFunction(value);
            type == VT_DOUBLE ? result.setDoubleValue(resultValue) : result.setIntValue(resultValue);
            programStack.push_back(result);
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
            // bc_swap should be
            if (a < b) {
                return 1;
            } else if (a > b) {
                return -1;
            } else return 0;
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