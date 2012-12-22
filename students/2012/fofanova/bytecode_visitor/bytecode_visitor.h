#ifndef ____ByteCodeVisitor__
#define ____ByteCodeVisitor__

#include "visitors.h"
#include "ast.h"

#include <string>
#include <map>
#include <stack>

using namespace mathvm;

class ByteCodeVisitor : public AstVisitor {
		Code* code;
		Bytecode* bytecode;
        VarType TOStype;
        VarType UOStype;
		std::map<std::string, uint16_t> vars;
		uint16_t last_id;
public:
    ByteCodeVisitor(Code* c, Bytecode* b): code(c), bytecode(b), last_id(0) {}
    virtual ~ByteCodeVisitor() {}
    
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
    void castTwoOperands(bool onlyInt);
    void castTOSToInt();
    void castTOSToDouble();
};

#endif
