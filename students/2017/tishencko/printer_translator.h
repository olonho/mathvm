#ifndef PRINTER_TRANSLATOR_H
#define PRINTER_TRANSLATOR_H

#include "mathvm.h"
#include "visitors.h"
#include "ast.h"
#include "parser.h"
#include "iostream"
#include "string"
#include "map"

namespace mathvm {

class AstPrinterTranslatorImpl : public Translator {
public:
    virtual Status* translate(const string& program, Code* *code);
};

class PrettyPrintVisitor : public AstDumper {
    int indent = 0;

    void printIndent();
    void incIndent();
    void decIndent();
    const char* to_string(const VarType&) const;
    std::string correctStrLiteral(const std::string& str);
    bool needSemicolon(const AstNode*);

public:
#define VISITOR_FUNCTION(type, name)            \
    void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

}
#endif

