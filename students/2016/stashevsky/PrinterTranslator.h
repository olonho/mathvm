#pragma once

#include "mathvm.h"
#include "visitors.h"

#include <cstdint>

namespace mathvm {

    struct PrinterTranslator : public Translator {
        virtual ~PrinterTranslator();

        virtual Status *translate(const string &program, Code **code);
    };

namespace details {

struct PrinterVisitor : public AstBaseVisitor {
    virtual ~PrinterVisitor();

    #define VISITOR_FUNCTION(type, name) \
        virtual void visit##type(type* node);

        FOR_NODES(VISITOR_FUNCTION)
    #undef VISITOR_FUNCTION

private:
    uint16_t level_ = 0;
    bool format_ = true;

    void printScope(Scope *scope);
    void printIndent();
    void printSeparator();

    string escape(char c) const;
};

}

}
