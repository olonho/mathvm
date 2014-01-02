#include <iostream>
#include <fstream>
#include <streambuf>
#include <string>
#include <memory>

#include <parser.h>
#include <mathvm.h>

using namespace mathvm;

void runTranslator(std::string const & programText) {
  std::auto_ptr<Translator> translator(new BytecodeTranslatorImpl);
  Code * code = NULL;
  
  std::auto_ptr<Status> status(translator->translate(programText, &code));

  if (status.get() != NULL && status->isError()) {
    std::cerr << "Error at " << status->getPosition() << ":" << std::endl;
    std::cerr << status->getError() << std::endl;
    goto cleanup;
  }
  
  //code->execute(//TODO add params);
  code->disassemble();

  cleanup:
  delete code;
}
 
int main(int argc, char** argv) {
 if (argc != 2) {
    std::cout << "usage: " << argv[0] << " <filename>" << std::endl;
    return 0;
  }

  std::ifstream stream(argv[1]);
  if (!stream.is_open()) {
    std::cerr << "failed to open file: " << argv[1] << std::endl;
    return 1;
  }

  std::string programText((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
 
  runTranslator(programText);

  return 0;
}
