#ifndef BYTECODE_GENERATOR_HPP
#define BYTECODE_GENERATOR_HPP

#include "exceptions.hpp"
#include "visitors.h"
#include "ast.h"

#include <vector>


namespace mathvm
{

class BytecodeGenerator: public AstVisitor
{
public:
    BytecodeGenerator():
        m_code(0)
    {
    }

    Status *generate(InterpreterCodeImpl *code, AstFunction *top);

#define VISITOR_FUNCTION(type, name) \
    void visit##type(type* node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
    void registerFunction(AstFunction *func);
    void translateFunction(AstFunction *top);
    void findVariable(AstNode const *node, std::string const &name,
                      uint16_t &ctx, uint16_t &id, VarType &type);
    void loadVariable(AstNode const *node,
                      VarType type, uint16_t ctx, uint16_t id);
    void storeVariable(AstNode const *node,
                       VarType type, uint16_t ctx, uint16_t id,
                       char const *errMsg = 0);
    void addStoreBytecode(AstNode const *node,
                          VarType type, uint16_t ctx, uint16_t id,
                          char const *errMsg = 0);
    void makeLazyLogicalOp(BinaryOpNode *node);
    VarType makeIBinaryOp(AstNode const *node, Instruction insn);
    VarType makeIDBinaryOp(AstNode const *node,
                           Instruction ibc, Instruction dbc);
    VarType makeCmpOp(AstNode const *node, int64_t mask, bool swap);
    VarType castIntDouble(AstNode const *node);
    void castTypes(AstNode const *node, VarType from, VarType to);

    void pushScope(FunctionNode *fn, Scope *scope);

    void popScope()
    {
        m_scopes.pop_back();
        m_vars.pop_back();
    }

    void registerFunctions(Scope::FunctionIterator fi)
    {
        while (fi.hasNext()) {
            registerFunction(fi.next());
        }
    }

    void translateFunctions(Scope::FunctionIterator fi)
    {
        while (fi.hasNext()) {
            translateFunction(fi.next());
        }
    }

    VarType tosType() const
    {
        if (m_types.empty())
            return VT_INVALID;
        return m_types.back();
    }

    VarType ptosType() const
    {
        if (m_types.size() < 2)
            return VT_INVALID;
        return m_types[m_types.size() - 2];
    }

    void pushType(VarType type)
    {
        //std::cerr << "push " << typeToName(type) << "\n";
        m_types.push_back(type);
    }

    void popType()
    {
        //std::cerr << "pop " << typeToName(m_types.back()) << "\n";
        m_types.pop_back();
    }

    uint16_t currentContext()
    {
        return m_fids[m_funcs.back()->name()];
    }

    uint16_t nextContext() const
    {
        return 0;
    }

    Bytecode *bc()
    {
        assert(!m_bcs.empty());
        return m_bcs.back();
    }

    int &returnCount()
    {
        assert(!m_returnCounts.empty());
        return m_returnCounts.back();
    }

    VarType returnType()
    {
        assert(!m_returnTypes.empty());
        return m_returnTypes.back();
    }

    Label &returnLabel()
    {
        assert(!m_returnLabels.empty());
        return m_returnLabels.back();
    }

    Scope *scope()
    {
        return m_scopes.back().second;
    }

    FunctionNode *function()
    {
        return m_scopes.back().first;
    }

    bool hasNoReturn(AstNode *node) const
    {
        return node->isForNode() || node->isWhileNode()
                || node->isIfNode() || node->isPrintNode();
    }

private:
    std::vector<VarType> m_types;
    std::vector< std::pair<FunctionNode *, Scope *> > m_scopes;
    std::vector< std::vector<AstVar *> > m_vars;
    std::vector<FunctionNode *> m_funcs;
    std::map<std::string, uint16_t> m_fids;

    std::vector<int> m_returnCounts;
    std::vector<VarType> m_returnTypes;
    std::vector<Label> m_returnLabels;
    std::vector<Bytecode *> m_bcs;

    InterpreterCodeImpl *m_code;
};

}


#endif // BYTECODE_GENERATOR_HPP
