#include "translator_impl.h"

#include "mathvm.h"

#include "source_print_translator.h"

namespace mathvm {


Translator* Translator::create(const string& impl) {
    if (impl == TRANSLATOR_TASK_PRINT) {
        return new SourcePrintTranslator(std::cout);
    } else {
        return nullptr;
    }
}

}