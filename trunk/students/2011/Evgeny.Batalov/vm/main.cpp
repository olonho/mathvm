#include <iostream> 
#include <fstream>
#include <streambuf>
#include "Translator.h"
#include "Interpreter.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Please supply source file" << std::endl;
    return 1;
  }

  std::ifstream f(argv[1]);
  std::string program((std::istreambuf_iterator<char>(f)),
      (std::istreambuf_iterator<char>()));

  MyTranslator trans;
  Executable* executable;
  mathvm::Status* status;
  status = trans.translate(program, &executable);
  if (status->isOk()) {

    Interpreter interpreter(*executable);
    interpreter.interpret();
    return 0;
  } else {
    uint32_t offset, line;
    mathvm::positionToLineOffset(program, status->getPosition(), line, offset);
    std::cerr << "ERROR: " << std::endl;
    std::cerr << status->getError() << std::endl;
    std::cerr << "At line: " << line  << " offset: " << offset << std::endl;
    return -1;
  }
}
