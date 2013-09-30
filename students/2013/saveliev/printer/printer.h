#ifndef PRINTER_VISITOR
#define PRINTER_VISITOR

#include <ostream>

#include "mathvm.h"
#include "visitors.h"
#include "ast.h"

namespace mathvm {

class Printer: AstVisitor {
public:
    Printer(std::ostream& out = std::cout):
        out(out), level(-1) {}
    
    void print(AstFunction* root) {        
        assert(root);                
        root->node()->body()->visit(this);
    } 
    
private:
    std::ostream& out;   
    int level;
      
    virtual void visitBinaryOpNode(BinaryOpNode* node);
    virtual void visitUnaryOpNode(UnaryOpNode* node);    
    virtual void visitStringLiteralNode(StringLiteralNode* node);
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node);
    virtual void visitIntLiteralNode(IntLiteralNode* node);
    virtual void visitLoadNode(LoadNode* node);
    virtual void visitStoreNode(StoreNode* node);
    virtual void visitForNode(ForNode* node);
    virtual void visitWhileNode(WhileNode* node);
    virtual void visitIfNode(IfNode* node);  
    virtual void visitBlockNode(BlockNode* node);
    void blockNodeVariableDeclarations(BlockNode* node);
    void blockNodeFunctionDeclarations(BlockNode* node);
    void blockNodeInnerNodes(BlockNode* node);
    virtual void visitFunctionNode(FunctionNode* node);
    virtual void visitReturnNode(ReturnNode* node);
    virtual void visitCallNode(CallNode* node);
    virtual void visitNativeCallNode(NativeCallNode* node);
    virtual void visitPrintNode(PrintNode* node);

    void indent() {
        for (int i = 0; i < level; i++)
            out << "    ";
    }
};

}

#endif
