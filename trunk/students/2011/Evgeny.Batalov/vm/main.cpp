#include <iostream> 
#include <fstream>
#include <streambuf>
#include "Translator.h"
#include "ExecutionEnv.h"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Please supply source file" << std::endl;
    return 1;
  }

  bool opt = argv[2] != 0;
  if (!opt) {
    std::cout << "Hint: type \"prog_name.mvm opt\" to force JIT" << std::endl;
  }

  std::ifstream f(argv[1]);
  std::string program((std::istreambuf_iterator<char>(f)),
      (std::istreambuf_iterator<char>()));
  uint64_t compStart = getTimeMs();
  MyTranslator trans;
  Executable* executable;
  mathvm::Status* status;
  status = trans.translate(program, &executable);
  uint64_t compFinish = getTimeMs();
  std::cout << "Compilation time: " << (compFinish - compStart)  << std::endl;

  std::vector<mathvm::Var*> vars;

  if (status->isOk()) {
    uint64_t interpStart = getTimeMs();
    ExecutionEnv env(*executable, opt);
    env.execute(vars);
    uint64_t interpFinish = getTimeMs();
    std::cout << "Execution time: " << (interpFinish - interpStart)  << std::endl;
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
