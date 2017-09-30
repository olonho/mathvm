#include "include/ast_printer_translator.h"
#include "include/ast_printer_visitor.h"
#include "parser.h"

using namespace mathvm;

AstPrinterTranslatorImpl::AstPrinterTranslatorImpl() {}

Status *AstPrinterTranslatorImpl::translate(const string &program, Code **code) {
    Parser parser;
    Status * status = parser.parseProgram(program);
    if (status->isError()) {
        return status;
    }

    AstPrinterVisitor visitor;
    FunctionNode * node = parser.top()->node();
    node->visitChildren(&visitor);
    cout << visitor.get_program();

    return status;
}

AstPrinterTranslatorImpl::~AstPrinterTranslatorImpl() {

}
