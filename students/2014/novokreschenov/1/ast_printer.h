#ifndef AST_PRINTER
#define AST_PRINTER

#include "visitors.h"
#include "mathvm.h"

namespace mathvm {

class AstPrinter : public Translator
{
public:
    virtual Status* translate(const string & program, Code* *code);
};

class AstPrinterVisitor: public AstVisitor
{
    ostream& _output;

    void printTokenKind(TokenKind kind);
    void printVarType(VarType type);
    void printSemicolon();
    bool isMainScope(Scope* scope);

public:
    AstPrinterVisitor(ostream& output = cout)
        : _output(output)
    {}

#define VISITOR_FUNCTION(type, name)    \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

}

#endif //AST_PRINTER
