#pragma once

#include "mathvm.h"
#include "visitors.h"

#include <cstdint>
#include <stack>
#include <unordered_map>

namespace mathvm {


namespace details {

struct VarLocation {
    uint16_t scope;
    uint16_t local;
};

struct TranslatorVisitor : public AstBaseVisitor {
    TranslatorVisitor(InterpreterCodeImpl &code);
    virtual ~TranslatorVisitor();

    void translate(AstFunction &top);

    #define VISITOR_FUNCTION(type, name) \
        virtual void visit##type(type* node);

        FOR_NODES(VISITOR_FUNCTION)
    #undef VISITOR_FUNCTION

private:
    InterpreterCodeImpl &code_;
    std::stack<BytecodeFunction*> function_scope_;
    std::stack<Scope*> scopes_;
    std::stack<VarType> stack_types_;
    std::map<std::string, std::stack<VarLocation>> locals_;
    std::stack<uint32_t> stack_size_;

    void translateFunction(AstFunction &function);
    VarType eval(AstNode &node);

    void instruction(Instruction ins, int stack_pop);
    void convert(VarType from, VarType to);

    void store(AstVar const& variable);
    void load(AstVar const& variable);

    void indexFunctions();
    void indexVariable(AstVar const& variable);
    void unindexVariable(AstVar const& variable);

    void generateLazyBinaryOp(BinaryOpNode *node);
    void generateCompare(BinaryOpNode *node);

    VarType unifyTop(VarType left, VarType right);
    AstVar newVar(VarType type);

    Bytecode& bytecode();
};

}

}
