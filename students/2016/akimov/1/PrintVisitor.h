#pragma once

#include <mathvm.h>
#include <visitors.h>

namespace mathvm {

class PrintVisitor : public AstVisitor {
    ostream& _out;
    uint32_t _indentLevel = 0;

    void printIndent();

public:
    PrintVisitor(ostream& out)
            : _out(out)
    {}

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node) override;

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

}
