#ifndef _MATHVM_AST_PRINTER
#define _MATHVM_AST_PRINTER

#include "ast.h"
#include <map>
#include <stack>
#include <utility>

using namespace mathvm;
using std::map;
using std::stack;
using std::pair;
using std::make_pair;

class AstToBytecodeTranslator: public AstVisitor {

    map< pair<TokenKind, VarType>, pair<Instruction, VarType> > binaryOpToBytecodeInsn;
    stack<VarType> types;
    map< VarType, map<TokenKind, Instruction> > arithmetic;
    map< TokenKind, Instruction > comparison;

    Bytecode* getBytecode();

    void removeConditionCheckingParams();
    void compareInts(Instruction insn);
    void compareDoubles(Instruction insn);

public:
    AstToBytecodeTranslator();
    virtual ~AstToBytecodeTranslator() {}
    
    
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
    virtual void visitFunctionNode(FunctionNode* node);
    virtual void visitReturnNode(ReturnNode* node);
    virtual void visitCallNode(CallNode* node);
    virtual void visitNativeCallNode(NativeCallNode* node);
    virtual void visitPrintNode(PrintNode* node);
};

#endif
