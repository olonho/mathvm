#ifndef MAIN_CONTEXT2_H
#define MAIN_CONTEXT2_H


#include <map>
#include <string>
#include "mathvm.h"
#include "ast.h"
#include "visitor_ctx.hpp"
#include <stack>

namespace mathvm {

    struct Variable {
        int64_t _integer;
        double _double;
        string _string;

        VarType _type;

        Variable(VarType t) {
            _type = t;
        }

        void set_int(int64_t x) {
            _integer = x;
        }

        void set_double(double x) {
            _double = x;
        }

        void set_string(const string &x) {
            _string = x;
        }

        int64_t get_int() {
            return _integer;
        }

        double get_double() {
            return _double;
        }

        string get_string() {
            return _string;
        }

        VarType get_type() {
            return _type;
        }

    };


    class InterpreterCtx {

    public:
        BytecodeFunction* func = nullptr;
        map<uint16_t, Variable> vars;

        InterpreterCtx(BytecodeFunction* f): func(f) {}

        void set(uint16_t id, Variable var) {
            vars.erase(id);
            vars.insert(make_pair(id, var));
        }

        Variable get(uint16_t id) {
            if (vars.count(id) == 0) {
                Variable var(VT_INT);
                var.set_int(0);
                vars.insert(make_pair(id, var));
            }
            return vars.at(id);
        }

    };

}

#endif //MAIN_Context2_H
