#pragma once

#include <cstdio>
#include <stdint.h>

#include <deque>
#include <map>

#include <sys/mman.h>
#include <string.h>

#include "common.h"
#include "mathvm.h"
#include "X86Code.h"

// ================================================================================

namespace mathvm {

  /*
  template<typename T>
  class LazyLabel {
    LazyLabel() { }

    LazyLabel(X86Code* src, X86Code* dst) {
      _label = bc->current();
      bc->addTyped<T>(0);
      _bc = bc;
    }

    void bind() {
      int32_t d = (int32_t)offset - (int32_t)_label - sizeof(T);
      _bc->setTyped<int32_t>(_label, d);
    }

  private:
    uint32_t _label;
  };
  */

  // --------------------------------------------------------------------------------

  class NativeFunction : public TranslatedFunction {
  public:
    NativeFunction(AstFunction* af) 
      : TranslatedFunction(af) { }

    NativeCode* code() {
      return &_code;
    }

    NativeCode* createBlock() {
      return new NativeCode;
    }

    void setFirstArg(uint16_t idx) {
      _firstArg = idx;
    }

    void setFirstLocal(uint16_t idx) {
      _firstLocal = idx;
    }

    uint16_t firstLocal() const {
      return _firstLocal;
    }

    uint16_t firstArg() const {
      return _firstArg;
    }

    void disassemble(std::ostream& out) const { }

    void addRef(X86Code* src, X86Code* dst) {
    }

  private:
    uint16_t _firstArg;
    uint16_t _firstLocal;
    VarType _retType;    

    NativeCode _code;

    //std::multimap<X86Code*, LazyLabel*> _refs;
  };

  // --------------------------------------------------------------------------------

  class Runtime : public Code {
    typedef std::vector<std::string> Strings;

  public:
    NativeFunction* createFunction(AstFunction* fNode);

    Status* execute(std::vector<mathvm::Var*, std::allocator<Var*> >& args);

    const char* addString(const std::string& s) {
      _strings.push_back(s);
      return _strings.back().c_str();
    }

  private:
    std::deque<NativeFunction*> _functions;
    Strings _strings;

    NativeFunction* _topf;
    std::map<std::string, size_t> _topArgMap;
  };
}
