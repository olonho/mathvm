#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "my_translator.h"
#include "mvm_code.h"
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
  AstFunction* top = parser->top();
  BytecodeFunction* function = new BytecodeFunction(top);
  MvmCode* mvm_code = new MvmCode();
  mvm_code->addFunction(function);
  mvm_code->makeStringConstant("");
  BytecodeVisitor* visitor = new BytecodeVisitor(mvm_code);
  visitor->visitBlockNode(top->node()->body());
  function->bytecode()->add(BC_STOP);
  // function->bytecode()->dump(std::cout);
  mvm_code->set_bytecode(function->bytecode());
  *code = (Code *)mvm_code;
  delete parser;
  delete parser_status;
  delete visitor;
  return new Status();
}

}
