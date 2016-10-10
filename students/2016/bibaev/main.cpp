#include <iostream>
#include <fstream>
#include <sstream>
#include <streambuf>

#include "parser.h"
#include "printer.h"

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

  mathvm::Parser parser;
  mathvm::Status* parsingStatus = parser.parseProgram(content);
  if (!parsingStatus->isOk()) {
    std::cerr << "Parsing failed. Message: " << std::endl;
    std::cerr << parsingStatus->getPosition() << " : " <<  parsingStatus->getError();

    return 3;
  }

  std::stringstream sstream;
  mathvm::Printer printer(sstream);
  parser.top()->node()->visitChildren(&printer);

  if (argc == 3) {
    std::ofstream filestream(argv[2], std::ofstream::out);
    if (filestream.fail()) {
      std::cerr << "Could not open output file " << argv[2] << std::endl;
      return 4;
    }

    filestream << sstream.str() << std::endl;
  }
  else {
    std::cout << sstream.str() << std::endl;
  }

  return 0;
}
