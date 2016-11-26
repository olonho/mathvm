#include <iostream>
#include <fstream>
#include <sstream>
#include "my_interpreter.h"

using namespace mathvm;

static void usage(const char* path) {
  std::cout << "Usage: " << std::endl << '\t' << path << " input_file [output_file]" << std::endl;
}

int main(int argc, char** argv) {
  if (argc != 2 && argc != 3) {
    usage(argv[0]);
    return 1;
  }

  std::string filename(argv[1]);

  std::ifstream file(filename);
  if (file.fail()) {
    std::cerr << "Could not open input the file " << filename << std::endl;
    return 2;
  }

  // http://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());

  Code* code = nullptr;
  Translator* translator = Translator::create("");
  Status* status = translator->translate(content, &code);
  if (status->isError()) {
    std::cerr << "Translation failed: " << status->getError() << std::endl;
    std::cerr << "Position: " << status->getPosition() << std::endl;
    return 3;
  }

  if (argc == 3) {
    std::ofstream filestream(argv[2], std::ofstream::out);
    if (filestream.fail()) {
      std::cerr << "Could not open output file " << argv[2] << std::endl;
      return 4;
    }

//    code->disassemble(filestream);
  } else {
//    code->disassemble(std::cout);
  }

  std::vector<Var*> no_vars{};
  dynamic_cast<InterpreterCodeImpl*>(code)->execute(no_vars);

  return 0;
}
