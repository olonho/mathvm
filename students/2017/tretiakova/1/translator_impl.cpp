#include "../../../../include/mathvm.h"
#include "../../../../vm/parser.h"
#include "../../../../include/visitors.h"

#include "include/ast_printer_impl.h"

#include <iostream>

namespace mathvm {

class PrinterTranslator : public Translator {
  public:
    PrinterTranslator() {
    }

    virtual ~PrinterTranslator() {
    }

    virtual Status* translate(const string& program, Code* *code) {
        Parser* parser = new Parser();

        Status* status = parser->parseProgram(program);
        if (status->isError()) {
            std::cerr << "[PrinterTranslator] parser errored with message "
                      << status->getMsg() << std::endl;
            *code = 0;
            return status;
        }

        AstFunction* top = parser->top();
        FunctionNode* ast = top->node();
        BlockNode* body = ast->body();
        AstPrinter* visitor = new AstPrinter();
        body->visit(visitor);
        std::cout << "was: " << program << endl;
        std::cout << "became: " << visitor->program() << endl;
        return Status::Ok();
    }
};

Translator* Translator::create(const string& impl) {
    if (impl == "printer") {
        return new PrinterTranslator();
    }
    if (impl == "" || impl == "intepreter") {
        //return new BytecodeTranslatorImpl();
        return 0;
    }
    if (impl == "jit") {
        //return new MachCodeTranslatorImpl();
        return 0;
    }
    return 0;
}

}
