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
#include "ir/live_analyzer.h"
#include "ir/transformations/minimizer.h"
#include "ir/transformations/unssa.h"
#include "ir/transformations/phi_remover.h"
#include "ir/reg_allocator.h"
#include "ir/transformations/reg_spiller.h"
#include "ir/transformations/optypes_normalizer.h"
#include "translator/machcode_generator.h"

void test();

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

static void printIr(IR::SimpleIr const &ir, std::ostream &str = std::cerr) {
    IR::IrTypePrinter printer(ir.varMeta, str);
    printer.print(ir);
}


int main(int argc, char **argv) {

    const char *program = programTextOrFailure(argc, argv);

    std::ofstream irRepr;
    irRepr.open("irRepr.txt", std::ofstream::out);

    Parser parser;
    Status *status = parser.parseProgram(program);
    ClosureInfo const &closureInfo = ClosureAnalyzer(parser.top(), irRepr)();

    IR::IrPrinter printer(irRepr);

    //will have to refactor AstAnalyzer
    auto &initial = SimpleIrBuilder(parser.top(), closureInfo, irRepr)();
    printIr(initial, irRepr);

    IR::SimpleIr ssa;
    IR::Ssa(initial, ssa, irRepr)();
    printIr(ssa, irRepr);

    IR::SimpleIr ssaWithPhi;
    IR::PhiFiller(ssa, ssaWithPhi, irRepr)();
    printIr(ssaWithPhi, irRepr);

    IR::SimpleIr emitCasts;
    IR::EmitCasts(ssaWithPhi, emitCasts, irRepr)();

    IR::SimpleIr normalized;
    IR::OperationTypesNormalizer(emitCasts, normalized, irRepr)();
    printIr(normalized, irRepr);


    std::vector<IR::SimpleIr *> substituted;
    substituted.push_back(new IR::SimpleIr());
    IR::Substitution(normalized, *substituted.back(), irRepr)();

    size_t i;
    for (i = 1; i < 1; i++) {
        IR::SimpleIr &prev = *substituted.back();
        substituted.push_back(new IR::SimpleIr());
        IR::SimpleIr &next = *substituted.back();
        bool trivial = IR::Substitution(prev, next, irRepr)();
        if (trivial) break;
        printIr(*substituted.back(), irRepr);
    }

    irRepr << "\n\n     " << i - 1 << " substitutions were performed in total. After that the result is: \n\n";

    printIr(*substituted.back(), irRepr);

    IR::SimpleIr minimized;
    IR::Minimizer(*substituted.back(), minimized, irRepr)();
    printIr(minimized, irRepr);


    IR::SimpleIr unssa, nophi;
    IR::UnSSA(minimized, unssa, irRepr)();
    IR::PhiRemover(unssa, nophi, irRepr)();
    printIr(nophi, irRepr);

    IR::LiveAnalyzer liveAnalyzer(irRepr);
    IR::LiveInfo *liveInfo = liveAnalyzer.start(nophi);

    printLiveInfo(*liveInfo, irRepr);

    auto gallocInfo = IR::regAlloc(nophi, *liveInfo, 12, irRepr);

    IR::regAllocDump(gallocInfo, irRepr);

    IR::SimpleIr spoiled;
    IR::RegSpiller(nophi, spoiled, gallocInfo, irRepr, 42)();
    printIr(spoiled, irRepr);


    IR::LiveAnalyzer secondLiveAnalyzer(irRepr);
    IR::LiveInfo *secondLiveInfo = secondLiveAnalyzer.start(spoiled);
    auto finalGallocInfo = IR::regAlloc(spoiled, *secondLiveInfo, 12, irRepr);

    printLiveInfo(*secondLiveInfo, irRepr);
    IR::regAllocDump(finalGallocInfo, irRepr);

    JitRuntime runtime;
    MachCodeGenerator generator(spoiled, finalGallocInfo, runtime, std::cerr);
    typedef double RetType;
    typedef RetType (*Program)(void);

    Program translated = (Program) generator.translate();
    RetType result = translated();
    std::cerr << result << std::endl;

//test();


    runtime.release((void *) translated);
    delete &closureInfo;

    for (auto ir : substituted) delete ir;
    delete liveInfo;
    delete secondLiveInfo;
    delete &initial;

    irRepr.close();
    delete status;
    delete[] program;

    return EXIT_SUCCESS;
}

//void test() {
//    JitRuntime runtime;
//    X86Assembler assembler(&runtime);
//    double value = 42.0;
//    uint64_t ivalue = *((uint64_t *) (&value));
//    assembler.mov(rax, ivalue);
//    assembler.movq(xmm0, rax);
//    assembler.emms();
//    assembler.ret();
//    void *prgrm = assembler.make();
//
//    typedef uint64_t (*Program)(void);
//    uint64_t result = ((Program) prgrm)();
//    std::cerr << result << std::endl;
//    runtime.release(prgrm);
//}
