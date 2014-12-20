#include <fstream>
#include "../../../../include/mathvm.h"
#include "translator/closure_analyzer.h"
#include "ast_printer.h"
#include "translator/translator.h"
#include "ir/transformations/ssa.h"
#include "ir/transformations/typecasts.h"

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

static void printIr(IR::SimpleIr& ir, std::ostream& str = std::cerr) {
    IR::IrTypePrinter printer(ir.varMeta, str);
    printer.print(ir);
}

int main(int argc, char **argv) {

    const char *program = programTextOrFailure(argc, argv);

    std::ofstream irRepr;
    irRepr.open("irRepr.txt", ios_base::openmode::_S_out);


    Parser parser;
    Status *status = parser.parseProgram(program);

    ClosureAnalyzer ca(parser.top(), irRepr);
    ca.start();
    ca.debug();

    IR::IrPrinter printer(irRepr);

    SimpleIrBuilder translator(parser.top(), ca.getResult(), irRepr);
    translator.start();

    printIr(*translator.getResult(), irRepr);

    IR::SsaTransformation ssaTransformation(*translator.getResult(), irRepr);
    ssaTransformation.start();

    printIr(*ssaTransformation.result(), irRepr);

    IR::EmitCasts typecheckerTransformation(*ssaTransformation.result(), irRepr);
    typecheckerTransformation.start();

    printIr(*typecheckerTransformation.result(), irRepr);


    delete translator.getResult();
    delete ssaTransformation.result();

//    irRepr.close();
    delete ca.getResult();
    delete status;
    delete[] program;

    return EXIT_SUCCESS;
}
