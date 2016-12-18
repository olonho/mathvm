#ifndef _BYTECODE_TRANSLATE_H
#define _BYTECODE_TRANSLATE_H

#include <unordered_map>

#include "ast.h"
#include "mathvm.h"
#include "translator_context.h"
#include "bytecode_metadata..h"

namespace mathvm {
    /**
     * Bytecode generating visitor
     */
    class BytecodeGenVisitor : public AstVisitor {
    public:
        BytecodeGenVisitor(InterpreterCodeImpl *code) : code(code) {
        }

        void translateToBytecode(AstFunction *root);

#define VISITOR_FUNCTION(type, name)            \
    void visit##type(type* node) override;

        FOR_NODES(VISITOR_FUNCTION)

#undef VISITOR_FUNCTION
    private:
        InterpreterCodeImpl *code;
        TranslatorContext ctx;

        void translateFunction(AstFunction *f);

        /**
         * Does not pushes loaded type on stack
         */
        void loadVar(VarData var_data, uint32_t position);

        void storeVar(VarData var_data, uint32_t position);

        void genStoreVarBytecode(VarData var_data, uint32_t position);

        void castTos(VarType to, uint32_t position);

        /**
         * Cast int-double operands to one type
         */
        VarType castBinOpOperands(uint32_t position);

        VarType getBinOpType(uint32_t position);

        /**
         * generate bytecode for binary operation with DOUBLEs or INTs
         */
        void doIDBinOp(Instruction bin_op, uint32_t position);

        /**
         * generate bytecode for logical operation node
         */
        void processLogicalOpNode(BinaryOpNode *node);

        /**
         * generate bytecode for one of compare binary operations
         * if_cmp_insn must be one of IF_CMP... instructions, with which
         * help compare can be implemented: consider we comparing 2
         * numbers (numerical type) a and b with operator OP, so they may
         * be comapred as: `cmp(a, b) OP 0`. OP is encoded in IF_CMP... instruction
         */
        void doCompareOp(Instruction if_cmp_insn, uint32_t position);
    };


    class BytecodeGenTranslator : public Translator {

    public:
        BytecodeGenTranslator() {}

        Status *translate(const std::string &program, Code **code) override;
    };
}

#endif
