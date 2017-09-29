#include "mathvm.h"
#include "printer_translator.h"

namespace mathvm
{

Translator* Translator::create(string const& impl)
{
    if (impl == "printer")
    {
        return new PrinterTranslator;
    }

    return nullptr;
}

}
