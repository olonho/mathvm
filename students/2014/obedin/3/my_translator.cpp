#include "my_translator.hpp"


TVisitor::TVisitor()
    : m_code(NULL)
    , m_curScope(NULL)
{
}

Status *
TVisitor::visit(Code *code, AstFunction *top)
{
    m_code = code;

    std::stack<uint32_t> emptyStack;
    m_sourcePos.swap(emptyStack);
    std::vector<VarType> emptyVector;
    m_stack.swap(emptyVector);

    BytecodeFunction *bcFn = new BytecodeFunction(top);
    m_code->addFunction(bcFn);
    m_curScope = new TScope(bcFn);

    try {
        Scope *topScope = top->scope()->childScopeAt(0);
        initVars(topScope);
        initFunctions(topScope);
        genBlock(top->node()->body());
        bc()->addInsn(BC_STOP);
        bcFn->setLocalsNumber(m_curScope->vars.size());
        return Status::Ok();

    } catch (std::runtime_error &err) {
        return m_sourcePos.empty()
               ? Status::Error(err.what())
               : Status::Error(err.what(), m_sourcePos.top());
    }
}

#include "my_translator-helpers.cpp"
#include "my_translator-visitors.cpp"
