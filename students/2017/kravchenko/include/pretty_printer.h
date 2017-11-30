#ifndef __AST_TRANSLATOR_IMPL_H__
#define __AST_TRANSLATOR_IMPL_H__

#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

#include <iostream>

namespace mathvm {

class AstTranslatorImpl : public Translator
{
public:
    ~AstTranslatorImpl() {}
    Status* translate(const string& program, Code* *code);
};

class AstPrinter : public AstBaseVisitor
{
private:
    std::ostream &_out;
    int _indent;

    void print();
    void printIndent();
    bool isNative(FunctionNode *node);
    bool containsBlock(AstNode *node);
public:
    AstPrinter(std::ostream &out): _out(out) {
        _indent = -1;
    }

    virtual ~AstPrinter() {
    }

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

}

#endif // __AST_TRANSLATOR_IMPL_H__
