#include <fstream>
#include "../../../../include/mathvm.h"
#include "translator/closure_analyzer.h"
#include "ast_printer.h"
#include "translator/translator.h"
#include "typechecker.h"

using namespace mathvm;
using namespace std;

const char *programTextOrFailure(int argc, char **argv) {
    if (argc < 2 || argv[1] == NULL) {
        std::cerr << "First argument should be script name!" << std::endl;
        exit(EXIT_FAILURE);
    }
    const char *filename = argv[1];
    const char *text = loadFile(filename);
    if (text == NULL) {
        std::cerr << "Can't read file: " << filename << std::endl;
        exit(EXIT_FAILURE);
    }
    return text;
}

int main(int argc, char **argv) {

    const char *program = programTextOrFailure(argc, argv);

    Parser parser;
    Status *status = parser.parseProgram(program);

    ClosureAnalyzer ca;
    ca.visitAstFunction(parser.top());
    SsaIrBuilder translator(parser.top(), ca, std::cout);
    translator.start();
    IR::SimpleSsaIr* ir = translator.getResult();

//    std::ofstream irRepr;
//    irRepr.open("irRepr.txt", ios_base::openmode::_S_out);
//    IR::IrPrinter printer(std::cout);
//    printer.print(*ir);

    IR::IrPrinter printer(std::cout);
    printer.print(*ir);
    delete ir;
    AstMetadataEraser eraser;
    eraser.visitFunctionNode(parser.top()->node());

//    irRepr.close();
    delete status;
    delete[] program;

    return EXIT_SUCCESS;
}
