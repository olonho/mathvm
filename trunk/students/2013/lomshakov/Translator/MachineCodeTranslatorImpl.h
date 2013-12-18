//
// Created by Vadim Lomshakov on 13/12/13.
// Copyright (c) 2013 spbau. All rights reserved.
//

#ifndef __MachineCodeTranslatorImpl_H_
#define __MachineCodeTranslatorImpl_H_


#include "mathvm.h"
#include "parser.h"
#include "MachCode.h"
#include "MachineCodeGenerator.h"

namespace mathvm {

  class MachineCodeTranslatorImpl : public Translator {
    Status* translateMachCode(const string& program,
        MachCode* *code);

  public:
    MachineCodeTranslatorImpl();
    virtual ~MachineCodeTranslatorImpl();

    virtual Status* translate(const string& program, Code* *result);
  };

}

#endif //__MachineCodeTranslatorImpl_H_
