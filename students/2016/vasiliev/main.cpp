#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>

#include "parser.h"
#include "mathvm.h"
//#include "visitors.h"
#include "ast.h"

#include "pretty_print.h"

using namespace std;

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Not enough arguments." << std::endl;
        return 1;
    }

    ifstream ifs{argv[1]};
    stringstream ss;
    ss << ifs.rdbuf();
    string text = ss.str();

    mathvm::Parser parser{};
    mathvm::Status *status = parser.parseProgram(text);

    if (status->isError()) {
        std::cerr << "Not right syntax." << std::endl;
        return 1;
    }

    delete status;

    pretty_print pp;
    parser.top()->node()->visitChildren(&pp);

    std::cout << pp.get_text() << std::endl;
    return 0;
}