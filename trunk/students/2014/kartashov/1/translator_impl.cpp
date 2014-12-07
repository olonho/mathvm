#include "mathvm.h"
#include "ast_printer.h"

using namespace mathvm;

Translator* Translator::create(const string& impl) {
  if (impl == "printer") {
    return new AstPrinter();
  } else {
    return NULL;
  }
}
