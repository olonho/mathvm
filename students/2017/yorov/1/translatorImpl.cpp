#include "parser.h"
#include "astPrinter.h"

namespace mathvm {
    Translator* Translator::create(const string &impl) {
        if (impl == "printer") {
            return new AstPrinterTranslator();
        }
        return nullptr;
    }

    Status* AstPrinterTranslator::translate(const string &program, Code **code) {
        Parser parser{};
        Status* status = parser.parseProgram(program);
        if (status->isError()) {
            return status;
        }
        AstVisitor* visitor = new AstPrinterVisitor{};
        parser.top()->node()->visitChildren(visitor);
        delete visitor;
        return status;
    }
}

