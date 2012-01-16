#ifndef MYTRANSLATOR_H
#define MYTRANSLATOR_H

#include "ast.h"
#include "MyCode.h"
#include "TranslationException.h"
#include "VariableMap.h"

class MyTranslator: public mathvm::AstVisitor {
public:
    virtual void visitBinaryOpNode(mathvm::BinaryOpNode* node);
    mathvm::VarType DeduceBinaryOperationType( mathvm::VarType leftType, mathvm::VarType rightType);

    virtual void visitUnaryOpNode(mathvm::UnaryOpNode* node);
    virtual void visitStringLiteralNode(mathvm::StringLiteralNode* node);
    virtual void visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node);
    virtual void visitIntLiteralNode(mathvm::IntLiteralNode* node);
    virtual void visitLoadNode(mathvm::LoadNode* node);
    virtual void visitStoreNode(mathvm::StoreNode* node);
    virtual void visitForNode(mathvm::ForNode* node);
    virtual void visitWhileNode(mathvm::WhileNode* node);
    virtual void visitIfNode(mathvm::IfNode* node);
    virtual void visitBlockNode(mathvm::BlockNode* node);
    virtual void visitFunctionNode(mathvm::FunctionNode* node);
    virtual void visitPrintNode(mathvm::PrintNode* node);

    void visit(mathvm::AstFunction * rootNode);

    void Dump();
    mathvm::Bytecode* GetBytecode();
    std::vector<std::string> GetStringsVector();

private:
    void bytecodeAdd(mathvm::VarType expectedType);
    void bytecodeSub(mathvm::VarType expectedType);
    void bytecodeMul(mathvm::VarType expectedType);
    void bytecodeDiv(mathvm::VarType expectedType);
    void bytecodePrint(mathvm::VarType expectedType);
    void bytecodeNeg(mathvm::VarType expectedType);

    bool tryDoArithmetics(mathvm::BinaryOpNode * node, mathvm::VarType expectedType);
    bool tryDoIntegerLogic( mathvm::BinaryOpNode*  node );
    bool tryDoFloatingLogic( mathvm::BinaryOpNode* node );
    void doIFICMP(mathvm::Instruction operation );
    void loadVar( mathvm::AstVar const * var );
    void storeVar( mathvm::AstVar const * var );

    MyCode myCode;
    VariableMap myVariables;
    mathvm::Bytecode myBytecode;
    std::map<mathvm::AstNode const*, mathvm::VarType> myNodeTypes;
};


#endif // MYTRANSLATOR_H
