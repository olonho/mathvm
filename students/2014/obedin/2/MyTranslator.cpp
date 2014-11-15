#include "MyTranslator.hpp"


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
    m_curScope = new TScope(new BytecodeFunction(top));
    std::stack<uint32_t> empty;
    m_sourcePos.swap(empty);

    try {
        Scope *topScope = top->scope()->childScopeAt(0);
        initVars(topScope);
        initFunctions(topScope);
        genBlock(top->node()->body());

        bc()->addInsn(BC_STOP);

        std::cout << "<main>:" << std::endl;
        bc()->dump(std::cout);

        return Status::Ok();

    } catch (std::runtime_error &err) {
        return m_sourcePos.empty()
               ? Status::Error(err.what())
               : Status::Error(err.what(), m_sourcePos.top());
    }
}

#include "MyTranslator_helpers.cpp"
#include "MyTranslator_visitors.cpp"
