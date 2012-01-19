#ifndef MYTRANSLATOR_H
#define MYTRANSLATOR_H

#include "VariableScopeManager.h"
#include <deque>
#include "MyInterpreter.h"
#include <iostream>
#include "TranslatorVisitor.h"
#include "TranslationException.h"

class MyTranslator: public mathvm::AstVisitor {
private:
    void bytecodeAdd(mathvm::VarType expectedType);
    void bytecodeSub(mathvm::VarType expectedType);
    void bytecodeMul(mathvm::VarType expectedType);
    void bytecodeDiv(mathvm::VarType expectedType);
    void bytecodePrint(mathvm::VarType expectedType);
    void bytecodeNeg(mathvm::VarType expectedType);

    bool tryDoArithmetics(mathvm::BinaryOpNode * node, mathvm::VarType expectedType);
    bool tryDoIntegerLogic(mathvm::BinaryOpNode*  node);
    bool tryDoFloatingLogic(mathvm::BinaryOpNode* node);
    void doIFICMP(mathvm::Instruction operation);
    void loadVar(mathvm::AstVar const * var);
    void storeVar(mathvm::AstVar const * var);
    void visitWithTypeControl(mathvm::AstNode* node, mathvm::VarType expectedType);
    void loadVarCommand(mathvm::VarType variableType, bool isClosure, VarId const& id);
    void storeVarCommand(mathvm::VarType variableType, bool isClosure, VarId id);

    MyInterpeter myCode;
    mathvm::Bytecode* myBytecode;
    std::map<mathvm::AstNode const*, mathvm::VarType> myNodeTypes;
    VariableScopeManager myScopeManager;
    mathvm::VarType myLastNodeType;

public:	
    virtual void visitBinaryOpNode(mathvm::BinaryOpNode* node);
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
    virtual void visitReturnNode(mathvm::ReturnNode* node);
    virtual void visitCallNode(mathvm::CallNode* node);

    void translate(mathvm::AstFunction * rootNode);
    mathvm::Code* getCode();

    MyTranslator();
    ~MyTranslator();
};
#endif

