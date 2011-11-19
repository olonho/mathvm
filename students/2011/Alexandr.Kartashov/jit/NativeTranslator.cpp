#include <iostream>
#include <map>
#include <vector>

#include <cstdio>
#include <cstring>
#include <assert.h>

#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

#include "VarNum.h"
#include "VarCollector.h"
#include "CallCollector.h"
#include "FunctionCollector.h"
#include "Flow.h"
#include "Runtime.h"
#include "NativeGenerator2.h"

// ================================================================================

namespace mathvm {

  class NativeGenerator : public AstVisitor {
    /**
     *  There's no way to use RSP and probably RBP...
     *  Nevertheless, we have 8 x87 registers and 8 128 bit XMM registers.
     *  In SSE2 the latter can store 2 64-bit integers and doubles, so we have
     *  32 integer registers and 24 floating-point registers.
     *  We shall consider using it...
     */

  private:
    Runtime* _runtime;
    NativeCode* _code;
    char _retReg;

  public:
    NativeGenerator(AstFunction* root) { 
      CompilerPool pool;
      
      _runtime = new Runtime;
      
      FunctionCollector fc(root, _runtime, &pool);
      
      for (FunctionCollector::Functions::const_iterator fit = fc.functions().begin();
           fit != fc.functions().end();
           ++fit) {
        VarCollector vc(*fit, _runtime, &pool);
        CallCollector cc(*fit, &pool);
        Flow flow(*fit, &pool);
        NativeGenerator2 ng(*fit);
      }

      _runtime->link();

      //info(root->node())->funRef->code()->link();
      //root->node()->visit(this);
    }

    Code *getCode() {
      return _runtime;
    }
  };

  // --------------------------------------------------------------------------------  

  class NativeTranslator : public Translator {
  public:
    Status* translate(const string& program, Code** code) {
      Parser parser;

      Status *status = parser.parseProgram(program);
      if (!status) {
        NativeGenerator ng(parser.top());

        *code = ng.getCode();
      }

      return status;
    }
  };

  // --------------------------------------------------------------------------------

  Translator* Translator::create(const string& impl) {
    return new NativeTranslator;
  }  
}
