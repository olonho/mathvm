#ifndef AST_TO_BYTECODE_H
#define AST_TO_BYTECODE_H
#include "../../../../include/visitors.h"
#include "translator_helper.h"

namespace mathvm {

    class AstToBytecodeVisitor : public AstVisitor {
        Code* code;
        Bytecode* bytecode;
        Scope* currentScope;
        //BytecodeFunction* currentFunction;
        ScopeHelper helper;
        //uint16_t currentScopeId = 0;
        
        VarType get_type(AstNode* node);
        void boolean_converter();
        void typevar_converter(VarType first, VarType second);
        void makeNotExpr();
        void makeArithmetic(BinaryOpNode* node);
        void makeBitwise(BinaryOpNode* node);
        void makeBoolean(BinaryOpNode* node);
        void makeComparision(BinaryOpNode* node);
        void makeLoad(const AstVar* var);
        void makeStore(const AstVar* var);

    public:
        
        AstToBytecodeVisitor(Code* code, Scope* scope) : code(code), currentScope(scope) {
        bytecode = nullptr;
        helper = ScopeHelper();
        }
        void visitMain(const AstFunction* main);
                
        void visitIntLiteralNode(IntLiteralNode* node);

        void visitStringLiteralNode(StringLiteralNode* node);
        
        void visitAstVar(AstVar* node);

        void visitLoadNode(LoadNode* node);

        void visitBinaryOpNode(BinaryOpNode* node);

        void visitUnaryOpNode(UnaryOpNode* node);

        void visitDoubleLiteralNode(DoubleLiteralNode* node);

        void visitPrintNode(PrintNode* node);

        void visitCallNode(CallNode* node);

        void visitFunctionNode(FunctionNode* node) ;

        void visitBlockNode(BlockNode* node);

        void visitStoreNode(StoreNode* node);

        void visitForNode(ForNode* node);

        void visitIfNode(IfNode* node);

        void visitWhileNode(WhileNode* node);

        void visitReturnNode(ReturnNode* node);


    };
}

#endif /* AST_TO_BYTECODE_H */

