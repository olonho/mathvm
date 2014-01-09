#pragma once

#include <map>
#include <string>
#include <vector>
#include <stack>
#include "mathvm.h"
#include "ast.h"
#include "BytecodeImpl.h"

namespace mathvm {

struct TranslationError {
    TranslationError(std::string message, uint32_t position) : message(message), position(position) {}
    
    std::string getMessage() const {
        return message;
    }
    
    uint32_t getPosition() const {
        return position;
    }
private:
    std::string message;
    uint32_t position;
};

struct TranslatorVisitor : AstVisitor {

    struct VariableScope {
        VariableScope(VariableScope* scope, uint16_t functionId) : parent(scope), functionId(functionId) {}

        void addVariable(AstVar* variable) {
            variables.insert(std::make_pair(variable, variables.size()));
        }

        std::pair<uint16_t, uint16_t> getVariable(const AstVar* variable) {
            std::map<const AstVar*, uint16_t>::iterator it = variables.find(variable);
            if (it == variables.end()) {
                if (!parent) {
                    throw TranslationError("Variable " + variable->name() + " not found", -1);
                }
                return parent->getVariable(variable);
            }
            return std::make_pair(it->second, functionId);
        }
    
        VariableScope* getParent() {
            return parent;
        }
        
    private:
        VariableScope* parent;
        uint16_t functionId;
        std::map<const AstVar*, uint16_t> variables;
    };
    
    TranslatorVisitor(BytecodeImpl*);
         
    void run(AstFunction*);

    virtual void visitIntLiteralNode(IntLiteralNode*);
    virtual void visitDoubleLiteralNode(DoubleLiteralNode*);
    virtual void visitStringLiteralNode(StringLiteralNode*);
    virtual void visitUnaryOpNode(UnaryOpNode*);
    virtual void visitBinaryOpNode(BinaryOpNode*);
    virtual void visitCallNode(CallNode*);
    virtual void visitLoadNode(LoadNode*);
    virtual void visitStoreNode(StoreNode*);
    virtual void visitBlockNode(BlockNode*);
    virtual void visitFunctionNode(FunctionNode*);
    virtual void visitReturnNode(ReturnNode*);
    virtual void visitNativeCallNode(NativeCallNode*);
    virtual void visitPrintNode(PrintNode*);
    virtual void visitForNode(ForNode*);
    virtual void visitWhileNode(WhileNode*);
    virtual void visitIfNode(IfNode*);

private:
    void loadVariable(const AstVar*);
    void storeVariable(const AstVar*);
    void convertToBool();
    void processArithmeticOperation(BinaryOpNode* node, VarType leftType, VarType rightType, TokenKind operation);
    void processLogicalOperation(BinaryOpNode* node, VarType leftType, VarType rightType, TokenKind operation);
    void processBitwiseOperation(BinaryOpNode* node, VarType leftType, VarType rightType, TokenKind operation);
    void processRelationalOperation(BinaryOpNode* node, VarType leftType, VarType rightType, TokenKind operation);
    
    BytecodeImpl* code;
    VariableScope* currentScope;
    VarType lastExpressionType;
    BytecodeFunction* currentFunction;
};

}
