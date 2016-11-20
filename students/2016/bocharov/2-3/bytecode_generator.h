#ifndef BYTECODE_GENERATOR_H
#define BYTECODE_GENERATOR_H

#include "scope_ctx.h"

#include "mathvm.h"
#include "ast.h"

#include <stack>
#include <memory>

namespace mathvm
{

struct CodeGenerationError : public std::runtime_error {
public:
    CodeGenerationError(const std::string& msg, uint32_t pos)
        : std::runtime_error(msg)
        , m_pos(pos)
    {}

    uint32_t position() const
    {
        return m_pos;
    }

private:
    uint32_t m_pos;
};


class BytecodeGeneratorVisitor : public AstVisitor {
public:
    BytecodeGeneratorVisitor(Code * code);

#define VISITOR_FUNCTION(type, name) \
    void visit##type(type* node) override;

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
    Bytecode * bytecode();

    void generateIfElse(IfNode * node);
    void generateWhile(WhileNode * node);
    void generateFor(AstVar const * var, BinaryOpNode * range, BlockNode * body);

    void generateLoad(const AstVar * var);
    void generateStore(const AstVar * var);

    void generateReturn(VarType type);

    template<typename T>
    void generateLiteral(const T& val);

    void generateArithmeticOp(BinaryOpNode * node);
    void generateBitOp(BinaryOpNode * node);
    void generateCompareOp(BinaryOpNode * node);
    void generateLogicOp(BinaryOpNode * node);
    void generateNotOp(UnaryOpNode * node);
    void generateNegateOp(UnaryOpNode * node);

    void generateCast(VarType type);

    void generateCall(uint16_t fid);
    void generateNativeCall(uint16_t fid);

    void generatePrint();

    VarType lastExprType() const;
    void setLastExprType(VarType type);

    ScopeContextPtr topCtx();

    void pushContext(FunctionNode * fn = nullptr);
    void popContext();

private:
    static std::map<VarType, std::map<TokenKind, Instruction>> instructions;

private:
    Code * m_code;

private:
    std::stack<ScopeContextPtr> m_contexts;
    VarType m_lastExprType;
};

}   // namespace mathvm

#endif  // BYTECODE_GENERATOR_H
