#include <iostream>
#include <fstream>
#include <sstream>
#include <streambuf>

#include "parser.h"
#include "printer.h"

int main(int argc, char** argv) {
    std::string filename(argv[1]);
    std::ifstream file(filename);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    mathvm::Parser parser;
    mathvm::Status* parsingStatus = parser.parseProgram(content);
    if (!parsingStatus->isOk()) {
        std::cerr << parsingStatus->getPosition() << " : " <<  parsingStatus->getError();
        return 1;
    }

    std::stringstream sstream;
    mathvm::AstPrinterVisitor printer(sstream);
    parser.top()->node()->visitChildren(&printer);

    if (argc == 3) {
        std::ofstream filestream(argv[2], std::ofstream::out);
        filestream << sstream.str() << std::endl;
        return 0;
    }
    std::cout << sstream.str() << std::endl;
    return 0;
}
