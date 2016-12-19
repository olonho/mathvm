#pragma once

#include "visitors.h"

namespace mathvm {

class AstPrinter: public AstBaseVisitor {
public:
    AstPrinter(std::ostream& out);

#define DECLARE_OVERRIDE(type, name)\
    void visit##type(type* node) override;

    FOR_NODES(DECLARE_OVERRIDE)
#undef DECLARE_OVERRIDE

private:
    void printEscaped(std::string const& str);
    void printNewline();

private:
    std::ostream& m_out;
    size_t m_depth;
};

}   // namespace mathvm
