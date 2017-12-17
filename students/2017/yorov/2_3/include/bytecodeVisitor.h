#ifndef BYTECODE_VISITOR_H
#define BYTECODE_VISITOR_H

#include "ast.h"
#include "visitors.h"
#include "mathvm.h"
#include "translatorUtil.h"

namespace mathvm {
    struct BytecodeVisitor : public AstVisitor {
        BytecodeVisitor(Code* code);
        ~BytecodeVisitor();
        Status* start(AstFunction* astFunction);

#define VISITOR_FUNCTION(type, name) \
        void visit##type(type* node) override;

        FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
    private:
        static double epsilon;
        void visitArithmeticOp(BinaryOpNode* node);
        void visitLogicalOp(BinaryOpNode* node);
        void visitBitwiseOp(BinaryOpNode* node);
        void visitComparisonOp(BinaryOpNode* node);
        void visitAstFunction(AstFunction* func);
        void storeTOS(const AstVar* var);
        void loadToTOS(const AstVar* var);
        void visitScope(Scope* scope);
        void convert(VarType from, VarType to);

        uint16_t getNativeId(NativeCallNode* node);
        std::unordered_map<uint16_t, bool> _isNativeFunc;
        std::unordered_map<std::string, uint16_t> _nativeToId;
        Code* _code;
        utils::Context* _context;
        VarType _TOSType;
    };
}
#endif //BYTECODE_VISITOR_H
