//
// Created by svloyso on 17.10.16.
//

#include "ast_to_code.h"
#include "parser.h"
#include "CodeImpl.h"

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

int printer(int argc, char** argv) {
    if(argc < 2) {
        std::cout << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    std::string filename(argv[1]);
    std::ifstream file(filename);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());


    mathvm::Parser parser;
    mathvm::Status* parsingStatus = parser.parseProgram(content);
    if (!parsingStatus->isOk()) {
        std::cout << "Parsing failed. Message: " << std::endl;
        std::cout << parsingStatus->getPosition() << " : " << parsingStatus->getError();
        return 1;
    }

    std::stringstream result;
    mathvm::AstToCode astToCode(result);
    astToCode.dumpCode(parser.top()->node()->asFunctionNode()->body());

    std::cout << result.str() << std::endl;
    return 0;
}

int translator(int argc, char** argv) {
    if(argc < 3) {
        std::cout << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
        return 1;
    }

    std::string filename(argv[1]);
    std::ifstream file(filename);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

	mathvm::Translator* translator = mathvm::Translator::create();
    mathvm::Code* code = new mathvm::CodeImpl();
    translator->translate(content, &code);

    std::ofstream out(argv[2]);
    code->disassemble(out);
    return 0;
}

int main(int argc, char** argv) {
    return translator(argc, argv);
}