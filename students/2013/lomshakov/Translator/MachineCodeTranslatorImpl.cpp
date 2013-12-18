//
// Created by Vadim Lomshakov on 13/12/13.
// Copyright (c) 2013 spbau. All rights reserved.
//

#include "MachineCodeTranslatorImpl.h"



namespace mathvm {

  Status *MachineCodeTranslatorImpl::translateMachCode(const string &program, MachCode **code) {
    Parser parser = Parser();
    Status* parserStatus = parser.parseProgram(program);

    if (!(parserStatus == 0 || parserStatus->isOk()))
      return parserStatus;

    MachineCodeGenerator codegen(parser.top(), *code);
    codegen.generate();

//    (*code)->disassemble(std::cout);

    return new Status();
  }

  Status *MachineCodeTranslatorImpl::translate(const string &program, Code **result) {
    MachCode * machCode = new MachCode();
    Status* statusTranslate = 0;

    try {
      statusTranslate = translateMachCode(program, &machCode);
    } catch(std::exception &e) {
      statusTranslate = new Status(e.what(), Status::INVALID_POSITION);
    }

    if (statusTranslate->isOk()) {
      *result = machCode;
    }

    return statusTranslate;
  }

  MachineCodeTranslatorImpl::MachineCodeTranslatorImpl() {
  }

  MachineCodeTranslatorImpl::~MachineCodeTranslatorImpl() {
  }
}