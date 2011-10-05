#ifndef MYTRANSLATOR_H
#define MYTRANSLATOR_H

#pragma once
#include "ast.h"
#include "MyCode.h"
#include "VariableMap.h"
#include "TranslationException.h"
#include <stdint.h>

class MyTranslator : mathvm::AstVisitor {
private:
    MyCode myCode;
    VariableMap myVariables;
    mathvm::Bytecode myBytecode;
    std::map<mathvm::AstNode const*, mathvm::VarType> myNodeTypes;

    void BytecodeAdd(mathvm::VarType expectedType);
    void BytecodeSub(mathvm::VarType expectedType);
    void BytecodeMul(mathvm::VarType expectedType);
    void BytecodeDiv(mathvm::VarType expectedType);
    void BytecodePrint(mathvm::VarType expectedType);
    void BytecodeNeg(mathvm::VarType expectedType);

    bool TryDoArithmetics(mathvm::BinaryOpNode * node, mathvm::VarType expectedType);
    bool TryDoIntegerLogic( mathvm::BinaryOpNode*  node );
    bool TryDoFloatingLogic( mathvm::BinaryOpNode* node );
    void DoIFICMP(mathvm::Instruction operation );
    void LoadVar( mathvm::AstVar const * var );
    void StoreVar( mathvm::AstVar const * var );

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
    void visit( mathvm::BlockNode* rootNode );

    void Dump();
    mathvm::Bytecode* GetBytecode();
    std::vector<std::string> GetStringsVector();
};

#endif // MYTRANSLATOR_H
