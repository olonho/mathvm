#ifndef __SIMPLEINTERPRETER_HPP_
#define __SIMPLEINTERPRETER_HPP_

#include "mathvm.h"
#include "InterpreterCodeImpl.hpp"

namespace mathvm {
    class SimpleInterpreter : public InterpreterCodeImpl {

    public:
        virtual Status *execute(vector<Var *> &vars) override;

    private:
        typedef vector<Var> stack_t;
        typedef vector<vector<Var>> vars_t;

        void run(ostream &out);

        void storeVariable(stack_t &stack, vars_t &vars, uint16_t id) {
            while (vars.size() <= id)
                vars.push_back(vector<Var>());
            vector<Var> &local_vars = vars[id];
            if (local_vars.empty()) {
                local_vars.push_back(stack.back());
            } else {local_vars.back() = stack.back();}
            stack.pop_back();
        }

        template<class T>
        void binary_operation(VarType type, vector<Var> &stack, T (*binaryFunction)(T const &, T const &)) {
            Var left = stack.back();
            stack.pop_back();
            Var right = stack.back();
            stack.pop_back();

            Var result(type, "");
            T leftValue = type == VT_DOUBLE ? left.getDoubleValue() : left.getIntValue();
            T rightValue = type == VT_DOUBLE ? right.getDoubleValue() : right.getIntValue();
            T resultValue = binaryFunction(leftValue, rightValue);
            type == VT_DOUBLE ? result.setDoubleValue(resultValue) : result.setIntValue(resultValue);
            stack.push_back(result);
        }

        template<class T>
        bool check_condition(vector<Var> &stack, bool (*comparer)(T const &, T const &)) {
            Var left = stack.back();
            stack.pop_back();
            Var right = stack.back();
            stack.pop_back();
            return comparer(right.getIntValue(), left.getIntValue());
        }

        template<class T>
        void unary_operation(VarType type, vector<Var> &stack, T (*unaryFunction)(T const &)) {
            Var var = stack.back();
            stack.pop_back();

            Var result(type, "");
            T value = type == VT_DOUBLE ? var.getDoubleValue() : var.getIntValue();
            T resultValue = unaryFunction(value);
            type == VT_DOUBLE ? result.setDoubleValue(resultValue) : result.setIntValue(resultValue);
            stack.push_back(result);
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
            if (a == 0)
                return 1;
            return -a;
        }

        template<class T>
        static bool _eq(T const &a, T const &b) {
            return a == b;
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