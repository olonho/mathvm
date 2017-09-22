#include "mathvm.h"
#include "ast_printing.h"


namespace mathvm {

Translator* Translator::create(const string& impl) {
    if (impl == "printer") {
      return new AstPrinterTranslator(); 
    }

    return nullptr;
}

}
