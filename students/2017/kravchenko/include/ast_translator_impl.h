#ifndef __AST_TRANSLATOR_IMPL_H__
#define __AST_TRANSLATOR_IMPL_H__

#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

namespace mathvm {

class AstTranslatorImpl : public Translator
{
public:
    ~AstTranslatorImpl() {}
    Status* translate(const string& program, Code* *code);
};

class AstPrinter : public AstBaseVisitor
{
public:
    AstPrinter() {
    }

    virtual ~AstPrinter() {
    }

    virtual void visitFunctionNode(FunctionNode* node);
/*
#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
*/
};

}

#endif // __AST_TRANSLATOR_IMPL_H__
