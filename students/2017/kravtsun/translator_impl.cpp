#include <ostream>
#include <numeric>

#include "ast_printer.h"
#include "translator_impl.h"
#include "translator_visitor.h"
#include "bytecode_impl.h"

namespace mathvm {

static Parser *parseRoutine(const string &program) {
    auto parser = new Parser;
    Status *parseStatus = parser->parseProgram(program);
    bool isOk = parseStatus->isOk();
    delete parseStatus;
    if (!isOk) {
        auto errorMessage = "parser failed: " + parseStatus->getError();
        throw std::logic_error(errorMessage);
    }
    return parser;
}

Status *TranslatorForPrinterImpl::translate(const string &program, Code **code) {
    Parser *parser = parseRoutine(program);
    auto astPrinter = new AstPrinter();
    astPrinter->dump(parser->top()->node());
    delete parser;
    return Status::Ok();
}

Status *MyBytecodeTranslator::translate(const string &program, Code **code) {
    Parser *parser = parseRoutine(program);
    parser->top()->node();
    
    try {
        auto interpreter = new BytecodeImpl();
        auto main_function = new BytecodeFunction(parser->top());
        interpreter->addFunction(main_function);
        auto bc = main_function->bytecode();
        auto bc_visitor = new TranslatorVisitor(interpreter, bc, main_function);
        parser->top()->node()->visit(bc_visitor);
        if (print_) {
            bc->dump(std::cout);
        }
        delete bc_visitor;
        delete parser;
        *code = interpreter;
    }
    catch(std::exception &e) {
        return Status::Error(e.what());
    }
    return Status::Ok();
}

Translator* Translator::create(const string &impl) {
    if (impl == "printer") {
        return new TranslatorForPrinterImpl;
    } else if (impl == "jit") {
        return nullptr;
    } else {
        return new MyBytecodeTranslator(impl == "translator");
    }
}

}
