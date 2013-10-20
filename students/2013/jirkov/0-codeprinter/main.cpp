#include "parser.h"
#include "AstPrinter.h"

#include <iostream>
#include <fstream>
#include <streambuf>
#include <string>
#include <memory>

void printAst(std::string const & programText) {
  mathvm::Parser parser;
  std::auto_ptr<mathvm::Status> status(parser.parseProgram(programText));
  
  if (status.get() != NULL && status->isError()) {
    std::cerr << "Error at " << status->getPosition() << ":" << std::endl;
    std::cerr << status->getError() << std::endl;
    return;
  }
  
  std::auto_ptr<mathvm::AstPrinter> printer(new mathvm::AstPrinter);
  parser.top()->node()->visit(printer.get());
}

int main( int argc, char **argv )
{
  if (argc != 2)
  {
    std::cout << "usage: " << argv[0] << " <filename>" << std::endl;
    return 1;
  }

  std::ifstream stream(argv[1]);
  if (!stream.is_open()) {
    std::cerr << "failed to open file: " << argv[1] << std::endl;
    return 1;
  }

  std::string programText((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  printAst(programText);

  return 0;
}
