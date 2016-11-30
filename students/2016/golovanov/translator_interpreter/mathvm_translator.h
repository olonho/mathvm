#ifndef MATHVM_TRANSLATOR_H
#define MATHVM_TRANSLATOR_H

#include <vector>
#include <map>
#include <stack>
#include <string>
#include <utility>
#include <cstddef>

#include "ast.h"
#include "mathvm.h"

namespace mathvm
{

namespace
{

// Context class
///////////////////////////////////////////////////////////////////////////
class translator_context
{
public:
    translator_context(BytecodeFunction* top)
        : id_(0)
        , function_(top)
        , parent_(nullptr)
    {}

    BytecodeFunction* function();
    Bytecode* bc();
    uint16_t id();
    translator_context* parent();
    translator_context* up(BytecodeFunction* func);
    translator_context* down();

    bool contains(const std::string& var_name);
    void add_var(const std::string& var_name);
    void add_all_vars(Scope* scope);
    std::pair<uint16_t, uint16_t> get_var_id(const std::string& var_name);

private:
    translator_context(uint16_t id, BytecodeFunction* func, translator_context* parent)
        : id_(id)
        , function_(func)
        , parent_(parent)
    {}

private:
    uint16_t id_;
    BytecodeFunction* function_;
    translator_context* parent_;
    std::map<std::string, uint16_t> local_vars_;
};


// Visitor class
///////////////////////////////////////////////////////////////////////////
class BytecodeVisitor : private AstVisitor
{
public:
    BytecodeVisitor(Code* code)
        : context_(nullptr)
        , code_(code)
    {}

    Code* make_visit(AstFunction* top);

private:
    VarType get_tos_type();
    void set_tos_type(VarType type);
    void cast_tos_to(VarType type);
    void process_arithmetic_operation(BinaryOpNode* node);
    void process_logic_operation(BinaryOpNode* node);
    void process_compare_operation(BinaryOpNode* node);
    void process_bit_operation(BinaryOpNode* node);
    void load_var(const AstVar* var);
    void store_var(const AstVar* var);

    void visitAstFunction(AstFunction* func);
    void visitBinaryOpNode(BinaryOpNode* node);
    void visitUnaryOpNode(UnaryOpNode* node);
    void visitStringLiteralNode(StringLiteralNode* node);
    void visitDoubleLiteralNode(DoubleLiteralNode* node);
    void visitIntLiteralNode(IntLiteralNode* node);
    void visitLoadNode(LoadNode* node);
    void visitStoreNode(StoreNode* node);
    void visitForNode(ForNode* node);
    void visitWhileNode(WhileNode* node);
    void visitIfNode(IfNode* node);
    void visitBlockNode(BlockNode* node);
    void visitFunctionNode(FunctionNode* node);
    void visitReturnNode(ReturnNode* node);
    void visitCallNode(CallNode* node);
    void visitNativeCallNode(NativeCallNode* node);
    void visitPrintNode(PrintNode* node);

private:
    translator_context* context_;
    Code* code_;
    std::stack<VarType> tos_types_;
};

} //anonymous namespace


// Translator class
///////////////////////////////////////////////////////////////////////////
// See mathvm.h

} // mathvm namespacce

#endif // MATHVM_TRANSLATOR_H
