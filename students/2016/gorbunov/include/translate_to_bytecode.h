#ifndef _BYTECODE_TRANSLATE_H
#define _BYTECODE_TRANSLATE_H

#include <unordered_map>

#include "ast.h"
#include "mathvm.h"
#include "translator_context.h"

namespace mathvm
{
    /**
     * Bytecode generating visitor
     */
    class BytecodeGenVisitor : public AstVisitor {
    public:
        BytecodeGenVisitor(Code* code): code(code) {
        }

        void translateToBytecode(AstFunction* root);

#define VISITOR_FUNCTION(type, name)            \
    void visit##type(type* node) override;
        FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
    private:
        Code* code;
        TranslatorContext ctx;

        void translateFunction(AstFunction *f);

        /**
         * Do not pushes loaded type on stack
         */
        void loadVar(VarData varData, uint32_t position);
        void storeVar(VarData varData, uint32_t position);
        void genStoreVarBytecode(VarData varData, uint32_t position);
        void castTos(VarType to, uint32_t position);

        /**
         * Cast int-double operands to one type
         */
        VarType castBinOpOperands(uint32_t position);
        VarType getBinOpType(uint32_t position);

        /**
         * generate bytecode for binary operation with DOUBLEs or INTs
         */
        void doIDBinOp(Instruction binOp, uint32_t position);

        /**
         * generate bytecode for logical operation node
         */
        void processLogicalOpNode(BinaryOpNode *node);

        /**
         * generate bytecode for one of compare binary operations
         * cmpOp must be one of IF_CMP... instructions, with which
         * help compare can be implemented: consider we comparing 2
         * numbers (numerical type) a and b with operator OP, so they may
         * be comapred as: `cmp(a, b) OP 0`. OP is encoded in IF_CMP... instruction
         */
        void doCompareOp(Instruction ifCmpInstruction, uint32_t position);
    };




    class BytecodeGenTranslator : public Translator {

    public:
        BytecodeGenTranslator() {}

        Status* translate(const std::string& program, Code* *code) override;
    };
}

#endif
