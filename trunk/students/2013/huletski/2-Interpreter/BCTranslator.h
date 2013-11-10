//
//  BCTranslator.h
//  VM_2
//
//  Created by Hatless Fox on 10/25/13.
//  Copyright (c) 2013 WooHoo. All rights reserved.
//

#ifndef __VM_2__BCTranslator__
#define __VM_2__BCTranslator__

#include <iostream>
#include "mathvm.h"
#include "parser.h"
#include "FenrirInterpreter.h"

using namespace mathvm;

class BCTranslator : public Translator {
public: // methods
  virtual Status* translate(const string& program, Code* *code);
  FenrirInterpreter code;
private: //methods
  bool parseProgram(const string& program);
private: //members
  Parser m_parser;
};


  
#endif /* defined(__VM_2__BCTranslator__) */
