
#include "parser.h"
#include "MvmTranslator.h"

using namespace mathvm;

MvmTranslator::MvmTranslator() {}


Status* MvmTranslator::translate(const string& program, Code**code) {
    Parser parser;
    if (Status * s = parser.parseProgram(program)) {
        return s;
    }

    MvmBytecode* bcode = new MvmBytecode();
    *code = bcode;
//    varmap_t varmap;
//    variable_t context = introduce_var(parser.top()->node()->body(), varmap, 0,
//            "@context", VT_INT);
//    funmap_t funmap;
//    try {
//        BytecodeBlockTranslator().run(bcode, bcode->bytecode(), varmap, funmap,
//                VT_VOID, context.id, parser.top()->node()->body());
//    } catch (translator_exception e) {
//        return new Status(e.what(), e.position());
//    }
    return 0;
}
