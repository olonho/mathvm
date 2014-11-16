#include "my_translator.hpp"


TVisitor::TVisitor()
    : m_code(NULL)
    , m_tosType(VT_INVALID)
    , m_curScope(NULL)
{
}

Status *
TVisitor::visit(Code *code, AstFunction *top)
{
    m_code = code;

    std::stack<uint32_t> empty;
    m_sourcePos.swap(empty);

    BytecodeFunction *bcFn = new BytecodeFunction(top);
    m_code->addFunction(bcFn);
    m_curScope = new TScope(bcFn);

    try {
        Scope *topScope = top->scope()->childScopeAt(0);
        initVars(topScope);
        initFunctions(topScope);
        genBlock(top->node()->body());
        bc()->addInsn(BC_STOP);
        return Status::Ok();

    } catch (std::runtime_error &err) {
        return m_sourcePos.empty()
               ? Status::Error(err.what())
               : Status::Error(err.what(), m_sourcePos.top());
    }
}

#include "my_translator-helpers.cpp"
#include "my_translator-visitors.cpp"
