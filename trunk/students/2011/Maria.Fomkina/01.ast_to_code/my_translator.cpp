#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "my_translator.h"
#include "print_visitor.h"

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
  Parser* parser = new Parser();
  Status* parser_status = parser->parseProgram(program);
  BlockNode* top = parser->top();
  PrintVisitor* visitor = new PrintVisitor();
  visitor->visitBlockNode(top);
  delete parser;
  delete parser_status;
  delete visitor;
  return new Status();
}

}
