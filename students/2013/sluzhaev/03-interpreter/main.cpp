#include <iostream>
#include <vector>
#include "TranslatorVisitor.h"

int main(int argc, char** argv) {
   if (argc != 2) {
      std::cerr << "Usage: " << argv[0] << " <input>" << std::endl;
      return 1;
   }

   const char* input_code = mathvm::loadFile(argv[1]);

   mathvm::Translator* translator = new mathvm::BytecodeTranslatorImpl();
   mathvm::Code* code;
   mathvm::Status* status = translator->translate(input_code, &code);
   if (status && status->isError()) {
      std::cerr << "Translation error: " << status->getError() << std::endl;
      delete status;
      return -1;
   }
   std::vector<mathvm::Var*> vars;
   
   status = code->execute(vars);
   if (status && status->isError()) {
      std::cerr << "Interpretation error: " << status->getError();
      delete status;
      return -1;
   }
   delete translator;
   delete code;
   return 0;
}
