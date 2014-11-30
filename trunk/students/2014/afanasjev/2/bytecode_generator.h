#pragma once

#include "detailed_binary_visitor.h"
#include "validating_visitor.h"

namespace mathvm {
class BytecodeGenerator : public DetailedBinaryVisitor {
public:
    BytecodeGenerator();
    virtual ~BytecodeGenerator() {}

    Status* generateCode(AstFunction* top, Code* code);
    
    virtual void visitBooleanBinOpNode(BinaryOpNode* node);
    virtual void visitArithmeticBinOpNode(BinaryOpNode* node);
    virtual void visitCmpBinOpNode(BinaryOpNode* node);
    virtual void visitRangeBinOpNode(BinaryOpNode* node);

    virtual void visitUnaryOpNode(UnaryOpNode* node);

    virtual void visitStringLiteralNode(StringLiteralNode* node);
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node);
    virtual void visitIntLiteralNode(IntLiteralNode* node);

    virtual void visitLoadNode(LoadNode* node);
    void visitChangeStoreNode(StoreNode* node);
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

    Scope* startScope(Scope* scope);
    void endScope(Scope* scope, Scope* old);
    void visitAstFunction(AstFunction* function);

    void loadVar(AstVar const * var);
    void storeVar(AstVar const * var);
    void convertIfNecessary(VarType have, VarType expect);
    void generateDuplicateIntTOS();
    
    void declareVar(AstVar* var);
    void undeclareVar(AstVar* var);

private:
    int valsLeftOnStack(AstNode* node);
    
    void fail(string const & msg, uint32_t position);
    VarType getType(AstNode* node);
    void initTables();

    struct VarLocation {
        VarLocation() : funId(0), idx(0) {}
        VarLocation(uint16_t funId, uint16_t idx)
            : funId(funId), idx(idx) 
        {}

        uint16_t funId;
        uint16_t idx;
    };


    Instruction arithmeticCommands[tTokenCount][5];
    Instruction cmpCommands[tTokenCount];
    Instruction convertCommands[5][5];
    Instruction storeCommands[2][5];
    Instruction loadCommands[2][5];

    map<AstVar const *, VarLocation> vars;
    uint16_t locals; 
    Scope* scope;
    BytecodeFunction* currentFunction;
    Bytecode* bytecode;

    Status* status;
    Code* code;

    static const std::string tmpVarName;
};
}
