//
//  LokiMachCodeTranslator.h
//  VM_3
//
//  Created by Hatless Fox on 12/11/13.
//  Copyright (c) 2013 WooHoo. All rights reserved.
//

#ifndef VM_3_LokiMachCodeTranslator_h
#define VM_3_LokiMachCodeTranslator_h

#include <iostream>
#include "MachCodeContainer.h"
#include "mathvm.h"
#include "parser.h"

#include "AstToMCConverter.h"
#include "AstInfoExtractor.h"

using namespace mathvm;

class LokiMachCodeTranslator : public Translator {
public: // methods
  virtual Status* translate(const string& program, Code ** code) {
    Status* status = m_parser.parseProgram(program);
    if (status && status->isError()) { return status; }
    
    AstFunction *top_function = m_parser.top();
    AstToMCConverter converter;
    
    AstInfoExtractor extractor(&converter);
    extractor.handle_function_definition(top_function);
    //std::cout << "------------------------------" << std::endl;
    if (converter.errorStatus()) { return converter.errorStatus(); }
    
    converter.convert(extractor.functions(), extractor.functions().size());
     
    *code = converter.code();
    return converter.errorStatus();
  }

private: //members
  Parser m_parser;
};

#endif