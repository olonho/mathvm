#pragma once

#include <iostream>
#include "ast.h"
#include "mathvm.h"
#include "visitors.h"
#include "parser.h"

namespace mathvm {

class BytecodeTranslator {
  public:
  	BytecodeTranslator() {}
    virtual ~BytecodeTranslator() {}

    virtual Status * translate(string const & program, Code ** code);
};

}