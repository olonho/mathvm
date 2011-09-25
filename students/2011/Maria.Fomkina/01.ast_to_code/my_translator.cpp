#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "my_translator.h"

namespace mathvm {

Translator* Translator::create(const string& impl) {
  if ((impl == "") || (impl == "intepreter")) {
    //return new BytecodeTranslatorImpl();
    return 0;
  } else if (impl == "asttocode") {
    return new CodeWriterTranslator();
  }
  assert(false);
  return 0;
}

Status* CodeWriterTranslator::translate(
    const string& program, Code* *code) {
  printf("The code of ast_to_code translator will be here!\n");
  return new Status();
}

}
