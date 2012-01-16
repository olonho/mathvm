#include "MyVisitor.h"
#include <stdio.h>
#include "parser.h"

using namespace std;
using namespace mathvm;

int main(int argc, char** argv) 
{
  if (argc < 2) {
    std::cerr << "Specify file to process\n";
    return 1;
  }
  mathvm::Parser *parser = new mathvm::Parser;
  char* code = mathvm::loadFile(argv[1]);
  Status* status = parser->parseProgram(code);
  if (status == 0)   {
      MyVisitor *visitor = new MyVisitor(std::cout);
      parser->top()->node()->visit(visitor);
  }
  else {
    if (status->isError()) {
      uint32_t position = status->getPosition();
      uint32_t line = 0, offset = 0;
      positionToLineOffset(code, position, line, offset);
      printf("Cannot translate expression: expression at %d,%d; "
        "error '%s'\n",
        line, offset,
        status->getError().c_str());
    }
  }
  return 0;
}
