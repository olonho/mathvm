//
//  BCTranslator.cpp
//  VM_2
//
//  Created by Hatless Fox on 10/25/13.
//  Copyright (c) 2013 WooHoo. All rights reserved.
//

#include "BCTranslator.h"
#include "AstToBcTranslator.h"

using namespace mathvm;

Status* BCTranslator::translate(const string& program, Code* *code) {
  Status* status = m_parser.parseProgram(program);
  if (status && status->isError()) { return status; }
  AstToBCTranslator converter;
  converter.convert(m_parser.top());
  
  *code = converter.code();
  
  
  return converter.errorStatus();
}