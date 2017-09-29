#ifndef _MATHVM_PRINTER_H
#define _MATHVM_PRINTER_H

#include <ostream>

#include "../include/mathvm.h"

namespace mathvm {

class AstPrinterTranslator : public Translator {
public:
    AstPrinterTranslator() {}
    ~AstPrinterTranslator() {}

    virtual Status* translate(const string& program, Code* *code);
};

class AstPrinter : public AstVisitor {
public:
    AstPrinter(std::ostream& out, uint8_t tab = 4)
    : out(out)
    , tab(tab)
    , indent(0) {}

#define VISITOR_FUNCTION(type, name) \
    void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

    void printScope(Scope* scope);
    void increaseIndent();
    void decreaseIndent();
private:
    std::ostream& out;
    const uint8_t tab;
    uint16_t indent;
};

}

#endif //_MATHVM_PRINTER_H
