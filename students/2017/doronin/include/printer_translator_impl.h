#ifndef _PRINTER_TRANSLATOR_IMPL_H
#define _PRINTER_TRANSLATOR_IMPL_H

#include "visitors.h"
#include "parser.h"
#include <ostream>

namespace mathvm {

class PrinterTranslatorImpl : public Translator {
  public:
    PrinterTranslatorImpl() = default;
    virtual ~PrinterTranslatorImpl() = default;

    virtual Status* translate(const string& program, Code* *code);
};

class PrinterVisitor : public AstVisitor {
    static constexpr uint32_t _indent_size = 4;
    uint32_t _expr_counter;
    uint32_t _indent;
    std::ostream& _strm;
  public:
    PrinterVisitor(std::ostream& strm);
    virtual ~PrinterVisitor() = default;

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

}

#endif // _PRINTER_TRANSLATOR_IMPL_H
