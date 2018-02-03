#include "../../../../include/mathvm.h"
#include "../../../../vm/parser.h"
#include "../../../../include/visitors.h"

#include "include/ast_printer_impl.h"
#include "include/ast_to_bytecode_impl.h"

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
        // std::cout << "was: " << endl << program << endl;
        // std::cout << "became: " << endl << visitor->program() << endl;
        std::cout << visitor->program() << endl;
        return Status::Ok();
    }
};

Status* BytecodeTranslatorImpl::translate(const string &program, Code* *code) {
    Parser* parser = new Parser();

    Status* status = parser->parseProgram(program);
    if (status->isError()) {
        std::cerr << "[BytecodeTranslator] parser errored with message "
                  << status->getMsg() << std::endl;
        *code = 0;
        return status;
    }

    AstFunction* top = parser->top();
    (*code) = new BytecodeCode();
    ((BytecodeCode*)(*code))->add_scope(top->scope());
    BytecodeTranslateVisitor visitor((BytecodeCode*)(*code));
    StackFrame* sf = ((BytecodeCode*)(*code))->get_top_function();
    visitor.setTopFunction(sf);
    top->node()->visit(&visitor);
    visitor.unsetTopFunction();

    status = visitor.get_status();

    delete parser;

    return Status::Ok();
}

Translator* Translator::create(const string& impl) {
    if (impl == "printer") {
        return new PrinterTranslator();
    }
    if (impl == "" || impl == "interpreter") {
        return new BytecodeTranslatorImpl();
    }
    if (impl == "jit") {
        //return new MachCodeTranslatorImpl();
        return 0;
    }
    return new PrinterTranslator();
}

}
