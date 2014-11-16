#include "mathvm.h"
#include "parser.h"
#include "interpreter_code.hpp"
#include "bytecode_generator.hpp"
#include "utils.hpp"

#include <cstdlib>

using namespace mathvm;

Status* BytecodeTranslatorImpl::translate(
  const string& program, 
  Code** result
) {
  InterpreterCodeImpl* code = NULL;
  Status* status = translateBytecode(program, &code);

  if (status->isError()) {
    assert(code == NULL);
    *result = NULL;
    return status;
  }

  debug("\n", "BYTECODE:");
  code->disassemble();
  assert(code != NULL);
  *result = code;

  return status;
}


Status* BytecodeTranslatorImpl::translateBytecode(
  const string& program,
  InterpreterCodeImpl** result
) {
  InterpreterCodeImpl* code = NULL;
  Parser parser;

  Status* status = parser.parseProgram(program);
  
  if (status->isOk()) {
    delete status;
    code = new InterpreterCodeImpl();
    BytecodeGenerator codegen(parser.top(), code);
    status = codegen.generate();
  }

  if (status->isError()) {
    *result = NULL;
    delete code;
  } else {
    *result = code;
  }

  return status;
}