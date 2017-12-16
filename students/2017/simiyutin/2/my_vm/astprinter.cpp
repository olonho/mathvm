#include "../my_include/astprinter.h"
#include "../my_include/astprinter_visitor.h"

using namespace mathvm;

AstPrinter::AstPrinter() {}

Status *AstPrinter::translate(const string &program, Code **code) {
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

AstPrinter::~AstPrinter() {

}
