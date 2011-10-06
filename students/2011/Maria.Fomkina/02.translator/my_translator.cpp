#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "my_translator.h"
#include "bytecode_visitor.h"

namespace mathvm {

Translator* Translator::create(const string& impl) {
  if ((impl == "") || (impl == "intepreter")) {
    return new BytecodeTranslator();
  }
  assert(false);
  return 0;
}

Status* BytecodeTranslator::translate(
    const string& program, Code* *code) {
  Parser* parser = new Parser();
  Status* parser_status = parser->parseProgram(program);
  BlockNode* top = parser->top();
  BytecodeFunction* function = new BytecodeFunction(
      new AstFunction(NULL, top->scope()));
  (*code)->addFunction(function);
  BytecodeVisitor* visitor = new BytecodeVisitor(*code);
  visitor->visitBlockNode(top);
  function->bytecode()->add(BC_STOP);
  function->bytecode()->dump();
  delete parser;
  delete parser_status;
  delete visitor;
  return new Status();
}

}
