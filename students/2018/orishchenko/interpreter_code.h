#ifndef MATHVM_INTERPRETER_CODE_H
#define MATHVM_INTERPRETER_CODE_H

#include "mathvm.h"
#include "ast.h"
#include "parser.h"
#include "visitor_ctx.hpp"
#include "interpreter_ctx.hpp"
#include <map>
#include <stack>


namespace mathvm {

    class InterpreterCode : public Code {
    public:
        InterpreterCode();

        ~InterpreterCode() override;

        Status *execute(vector<Var *> &vars) override;

        stack<Variable> varStack;
        stack<BytecodeFunction *> funStack;
        stack<map<uint16_t, InterpreterCtx *>> contextStack;
        stack<uint32_t> _pos;

        Bytecode *bytecode();

        uint32_t pos();

        void runInstr();

        void push_next(Instruction inst);

        void next_command();

        void next_int();

        void jump_to(uint32_t place);

        void push_int(int64_t value);

        void push_double(double value);

        void push_str(const string &value);

        double top_double();

        int64_t top_int();

        void print();

        InterpreterCtx *context(uint16_t ctx);

        Instruction cur_instr();

        void add_ctx(VisitorCtx *context, BytecodeFunction *owner);

        uint16_t get_int();
    };
}

#endif
