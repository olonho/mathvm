#include "ast.h"
#include "mathvm.h"
#include "parser.h"

#include "bytecode_visitor.h"

namespace mathvm {

Status* BytecodeTranslatorImpl::translate(const string& program, Code* *code){
	Status* status;

    Parser parser;
    status = parser.parseProgram(program);

    if(status && status->isError()) {
            cerr << status->getError() << endl;
            return status;
    }

    *code = new BytecodeImpl();

    BytecodeVisitor visitor(parser.top(), *code);
    visitor.visit();

    return new Status();
}

}