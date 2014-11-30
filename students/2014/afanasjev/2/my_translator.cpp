#include "mathvm.h"
#include "parser.h"
#include "validating_visitor.h"
#include "bytecode_generator.h"
#include "ast_printer.h"
#include "interpreter_impl.h"

namespace mathvm {

class TranslatorImpl : public Translator {
    Status* translate(const string& program, Code* *code) {
        Parser parser;
        Status* status = parser.parseProgram(program);

        if(status->isError()) {
            return status;
        }
        delete status;

        ValidatingVisitor visitor;
        status = visitor.checkProgram(parser.top());        
        if(status->isError()) {
            return status;
        }
        delete status;
        
        Code* result = new InterpreterCodeImpl();
        *code = result;

        BytecodeGenerator generator;
        return generator.generateCode(parser.top(), result);
    }
};

Translator* Translator::create(const string& impl) {
    if(impl == "printer") {
        return new AstPrinter();
    }

    if (impl == "" || impl == "intepreter") {
        return new TranslatorImpl();
    }

    return 0;
}

}
