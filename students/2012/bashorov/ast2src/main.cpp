#include <iostream>

#include "ast.h"
#include "mathvm.h"
#include "parser.h"

#include "Ast2SrcVisitor.h"

enum EReturnCode {
    OK,
    WRONG_ARG_COUNT,
    CANNOT_READ_SOURCE,
    PARCER_ERROR,
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

        std::cerr << sourceFile << ":" << line << ":" << offset << ":"
                  << "Parser error:" << status->getError()
                  << std::endl;

        return PARCER_ERROR;
    }

    mathvm::Ast2SrcVisitor ast2src;

    mathvm::FunctionNode* node = parser.top()->node();
    if (node->name() == mathvm::AstFunction::top_name)
        node->visitChildren(&ast2src);
    else
        return -1;

	std::cout << "!";
    return OK;
}