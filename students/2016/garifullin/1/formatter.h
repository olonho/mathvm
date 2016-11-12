#pragma once

#include <string>
#include <sstream>
#include "ast.h"
#include "visitors.h"

namespace mathvm {

    class FormatVisitor : public AstBaseVisitor {
        std::ostream &out;
        size_t currentIndent = 0;
        std::string indent();
    public:
        FormatVisitor(std::ostream &out) : out(out) {}

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node);

        FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
    };

    class Formatter {
        Formatter(){}
    public:
        static Status *formatCode(std::string &input, std::string &output);
    };
}
