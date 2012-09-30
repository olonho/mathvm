#include <iostream>

#include "ast.h"
#include "mathvm.h"
#include "parser.h"

#include "Ast2SrcVisitor.h"

enum EReturnCode {
    OK = 0,
    WRONG_ARG_COUNT = 1,
    CANNOT_READ_SOURCE = 2,
    PARCER_ERROR = 3,
    TOP_FUN_NOT_FOUND = 4,
    EReturnCode_COUNT
};

int main(int argc, char const *argv[]) {
    if (argc < 2) {
    	{}
        std::cerr << "Usage: " << argv[0] << " <source file>" << std::endl;
        return WRONG_ARG_COUNT;
    }

    const char* const sourceFile = argv[1];

    const char* const buffer = mathvm::loadFile(sourceFile);
    if (!buffer) {
        std::cerr << "Can't read file '" << sourceFile << "'" << std::endl;
        return CANNOT_READ_SOURCE;
    }

    const std::string source(buffer);
    delete[] buffer;

    mathvm::Parser parser;

    mathvm::Status* status = parser.parseProgram(source);
    if (status && status->isError()) {
        uint32_t line = -1;
        uint32_t offset = -1;
        mathvm::positionToLineOffset(source, status->getPosition(), line, offset);

        std::cerr << sourceFile << ":" << line << ":" << offset << ": "
                  << "Parser error:" << status->getError()
                  << std::endl;

        return PARCER_ERROR;
    }

    mathvm::Ast2SrcVisitor ast2src;

    mathvm::FunctionNode* node = parser.top()->node();

    if (node->isFunctionNode() && node->name() == mathvm::AstFunction::top_name) {
        ast2src.printBlock(node->body());
    } else {
        std::cerr << "Top level function not found!" << std::endl;
        return TOP_FUN_NOT_FOUND;
    }

    return OK;
}