#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "ast_translator_impl.h"

#include <iostream>

namespace mathvm {

Translator* Translator::create(const string& impl) {
    if (impl == "printer")
        return new AstTranslatorImpl();

    printf("Only AST translator supported for now, you asked for %s\n", impl.c_str());
    assert(false);

    return nullptr;
}


Status* AstTranslatorImpl::translate(const string& program, Code* *code)
{
    Status *res;
    Parser parser;
    res = parser.parseProgram(program);

    if (res->isError())
        return res;

    AstFunction *top = parser.top();

    AstPrinter printer;
    printer.visitFunctionNode(top->node());

    return res;
}

void AstPrinter::visitFunctionNode(FunctionNode *node)
{
    std::cout << node->name() << std::endl;
}

}
