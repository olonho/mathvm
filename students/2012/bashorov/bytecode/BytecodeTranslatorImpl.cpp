#include "mathvm.h"

#include "parser.h"

#include "Ast2BytecodeVisitor.h"
#include "CodeImpl.h"

namespace mathvm {

Status* BytecodeTranslatorImpl::translate(const string& program, Code* *code){
	mathvm::Parser parser;

    mathvm::Status* status = parser.parseProgram(program);
    if (status && status->isError()) {
        uint32_t line = -1;
        uint32_t offset = -1;
        mathvm::positionToLineOffset(program, status->getPosition(), line, offset);

        std::cerr << "<program>" << ":" << line << ":" << offset << ": "
                  << "Parser error:" << status->getError()
                  << std::endl;

        return 0;
    }

    *code = new CodeImpl();
    mathvm::Ast2BytecodeVisitor ast2bytecode(*code);

    // mathvm::FunctionNode* node = parser.top()->node();
    ast2bytecode.start(parser.top());
    // if (node->isFunctionNode() && node->name() == mathvm::AstFunction::top_name) {
    //     ast2bytecode.printBlock(node->body());
    // } else {
    //     std::cerr << "Top level function not found!" << std::endl;
    //     return 0;
    // }

    return new Status();
}


}

