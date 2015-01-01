#include <fstream>
#include "../../../../include/mathvm.h"
#include "translator/closure_analyzer.h"
#include "ast_printer.h"
#include "ir/ir.h"
#include "translator/translator.h"
#include "ir/transformations/ssa.h"
#include "ir/transformations/typecasts.h"
#include "ir/transformations/substitutions.h"
#include "ir/transformations/phi_values.h"

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

    IR::IrPrinter printer(irRepr);

    SimpleIrBuilder translator(parser.top(), ca.result(), irRepr);
    translator.start();

    printIr(*translator.result(), irRepr);

    IR::Ssa ssaTransformation(translator.result(), irRepr);
    ssaTransformation.start();
    printIr(*ssaTransformation.result(), irRepr);

    IR::PhiFiller phis(ssaTransformation.result(), irRepr);
    phis.start();
    printIr(*phis.result(), irRepr);

    IR::EmitCasts emitCasts(phis.result(), irRepr);
    emitCasts.start();

    printIr(*emitCasts.result(), irRepr);


    int maxSubst = 100;
    std::vector<IR::Substitution *> substitutions;

    while(maxSubst--) {
        IR::SimpleIr const * last = ( substitutions.empty())?(emitCasts.result()): (substitutions.back()->result());
        substitutions.push_back(new IR::Substitution(last, irRepr));
        substitutions.back()->start();
        printIr(*substitutions.back()->result(), irRepr);
        if (substitutions.back()->isTrivial()) break;

    }
    irRepr << "\n\n     " << substitutions.size() << " substitutions were performed in total\n\n";

    for (auto subst : substitutions) {
        delete subst->result();
        delete subst;
    }

    delete translator.result();
    delete ssaTransformation.result();
    delete phis.result();
    delete emitCasts.result();
    irRepr.close();
    delete ca.result();
    delete status;
    delete[] program;

    return EXIT_SUCCESS;
}
