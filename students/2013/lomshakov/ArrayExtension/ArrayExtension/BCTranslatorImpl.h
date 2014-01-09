//
// Created by Vadim Lomshakov on 13/11/13.
// Copyright (c) 2013 spbau. All rights reserved.
//


#ifndef __BcTranslatorImpl_H_
#define __BcTranslatorImpl_H_

#include "mathvm.h"
#include "parser.h"
#include "BytecodeEmitter.h"
#include "InterpreterImpl.h"

namespace mathvm {
  class BCTranslatorImpl : public Translator {
    Status* translateBytecode(const string& program,
        InterpreterImpl * *code);

  public:
    BCTranslatorImpl() {
    }

    virtual ~BCTranslatorImpl() {
    }

    virtual Status* translate(const string& program, Code* *code);
  };
}


#endif //__BcTranslatorImpl_H_
