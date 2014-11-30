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
  InterpreterCodeImpl* code = 0;
  Status* status = translateBytecode(program, &code);

  if (status->isError()) {
    assert(code == 0);
    *result = 0;
    return status;
  }

  //code->disassemble();
  assert(code != 0);
  *result = code;

  return status;
}


Status* BytecodeTranslatorImpl::translateBytecode(
  const string& program,
  InterpreterCodeImpl** result
) {
  InterpreterCodeImpl* code = 0;
  Parser parser;

  Status* status = parser.parseProgram(program);
  
  if (status->isOk()) {
    delete status;
    code = new InterpreterCodeImpl();
    BytecodeGenerator codegen(parser.top(), code);
    status = codegen.generate();
  }

  if (status->isError()) {
    *result = 0;
    delete code;
  } else {
    *result = code;
  }

  return status;
}