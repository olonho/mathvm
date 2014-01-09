//
// Created by Vadim Lomshakov on 13/11/13.
// Copyright (c) 2013 spbau. All rights reserved.
//


#include "BCTranslatorImpl.h"


namespace mathvm {

  Status* BCTranslatorImpl::translateBytecode(const string& program, InterpreterImpl * *code) {
    Parser parser = Parser();
    Status* parserStatus = parser.parseProgram(program);

    if (!(parserStatus == 0 || parserStatus->isOk()))
      return parserStatus;

    BytecodeEmitter::getInstance().emitCode(parser.top(), *code);

    (*code)->disassemble(std::cout);

    return new Status();
  }

  Status *BCTranslatorImpl::translate(const string &program, Code **code) {
    InterpreterImpl * interpreterCode = new InterpreterImpl();
    Status* statusTranslate = 0;

    try {
      statusTranslate = translateBytecode(program, &interpreterCode);
    } catch(std::exception &e) {
      statusTranslate = new Status(e.what(), Status::INVALID_POSITION);
    }

    if (statusTranslate->isOk()) {
      *code = interpreterCode;
    }

    return statusTranslate;
  }
}