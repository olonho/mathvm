#ifndef TYPER_H
#define TYPER_H
#include "../../../../include/visitors.h"
namespace mathvm {
    class Typer : public AstVisitor {
        Scope* currentScope;
        FunctionNode* currentFunction;
        
        
        VarType get_type(AstNode* node);
        bool is_number(AstNode* node);
        bool is_number(VarType node);
        bool two_are_integers(AstNode* nodeOne, AstNode* nodeTwo);
        bool two_are_numbers(AstNode* nodeOne, AstNode* nodeTwo);
        bool equal(AstNode* nodeOne, AstNode* nodeTwo);
        
        VarType onlyIntegers(AstNode* nodeOne, AstNode* nodeTwo);
        VarType onlyNumbers(AstNode* nodeOne, AstNode* nodeTwo);
        VarType onlyEqual(AstNode* nodeOne, AstNode* nodeTwo);
        
    public:
        
        Typer(Scope* scope, FunctionNode* func): currentScope(scope),
                currentFunction(func) {}
        
        void visitIntLiteralNode(IntLiteralNode* node);

        void visitStringLiteralNode(StringLiteralNode* node);
        
        void visitAstFunction(AstFunction* node);

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



#endif /* TYPER_H */

