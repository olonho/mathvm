#ifndef TRANSLATOR_H_
#define TRANSLATOR_H_

#include "visitors.h"
#include "ast.h"
#include "mathvm.h"
#include "codeimpl.h"




using namespace mathvm;

class BytecodeTranslator : public AstVisitor {

public:
    BytecodeTranslator(AstFunction* ast_top);
    virtual ~BytecodeTranslator();

    CodeImpl* run();

    virtual void visitBinaryOpNode(BinaryOpNode *node);
    virtual void visitUnaryOpNode(UnaryOpNode *node);
    virtual void visitStringLiteralNode(StringLiteralNode *node);
    virtual void visitDoubleLiteralNode(DoubleLiteralNode *node);
    virtual void visitIntLiteralNode(IntLiteralNode *node);
    virtual void visitLoadNode(LoadNode *node);
    virtual void visitStoreNode(StoreNode *node);
    virtual void visitForNode(ForNode *node);
    virtual void visitWhileNode(WhileNode *node);
    virtual void visitIfNode(IfNode *node);
    virtual void visitBlockNode(BlockNode *node);
    virtual void visitFunctionNode(FunctionNode *node);
    virtual void visitReturnNode(ReturnNode *node);
    virtual void visitCallNode(CallNode *node);
    virtual void visitNativeCallNode(NativeCallNode *node);
    virtual void visitPrintNode(PrintNode *node);


private:


    void castStackToInt();
    void castStackToDouble();
    void castOperands(bool toInt);

    AstFunction* _ast_top;
    CodeImpl _code;
    Bytecode* _bytecode; //for access convenience

    std::map<std::string, uint16_t> vars;
    vector<VarType> stackTypes;

    unsigned int lastId;

};



#endif /* TRANSLATOR_H_ */
