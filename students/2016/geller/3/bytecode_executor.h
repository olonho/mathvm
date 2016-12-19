//
// Created by wimag on 23.11.16.
//

#ifndef MATHVM_BYTECODE_EXECUTOR_H
#define MATHVM_BYTECODE_EXECUTOR_H

#include "../2/bytecode_translator_overrides.h"
#include "execution_storage.h"
namespace mathvm{
    class bytecode_executor {
    private:
        Code* code;
        execution_storage storage;
        void execute_instruction(Instruction insn);

        void execute_int_binop(Instruction insn);
        void execute_double_binop(Instruction insn);

        void execute_conditional_jump(Instruction insn);
    public:
        bytecode_executor(InterpreterCodeImpl* code);
        Status* execute();
    };
}



#endif //MATHVM_BYTECODE_EXECUTOR_H
