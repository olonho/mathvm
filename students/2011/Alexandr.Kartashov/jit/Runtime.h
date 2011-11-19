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

#include <deque>

// ================================================================================

namespace mathvm {

  // --------------------------------------------------------------------------------

  class NativeFunction : public TranslatedFunction {
  public:
    struct Ref {
      Ref(NativeFunction* f, uint32_t p) {
        _fun = f;
        _pos = p;
      }

      NativeFunction* _fun;
      uint32_t _pos;
    };

    typedef std::deque<Ref> Refs;

    NativeFunction(AstFunction* af) 
      : TranslatedFunction(af),
        _linked(false){ }

    NativeCode* code() {
      return &_code;
    }

    void disassemble(std::ostream& out) const { /* objdump -d goes here :) */ }

    void addRef(uint32_t pos, NativeFunction* to) {
      if (to != this) {
        to->_inRefs.push_back(Ref(this, pos));
      }

      _outRefs.push_back(Ref(to, pos));
    }

    const Refs& inRefs() const {
      return _inRefs;
    }

    const Refs& outRefs() const {
      return _outRefs;
    }

    void setStart(char* p) {
      _start = p;
      _linked = true;
    }

    char* start() {
      return _start;
    }

    bool linked() const {
      return _linked;
    }

    void setCallStorageSize(size_t size) {
      _callStor = size;
    }

    size_t callStorageSize() const {
      return _callStor;
    }

  private:
    VarType _retType;    
    NativeCode _code;
    Refs _inRefs;   // who references us
    Refs _outRefs;  // those we reference

    size_t _callStor;

    char* _start;   // function entry point address
    bool _linked;

    //std::multimap<X86Code*, LazyLabel*> _refs;
  };

  // --------------------------------------------------------------------------------

  class Runtime : public Code {
    typedef std::vector<std::string> Strings;
    typedef std::deque<NativeFunction*> Functions;

  public:
    NativeFunction* createFunction(AstFunction* fNode);

    Status* execute(std::vector<mathvm::Var*, std::allocator<Var*> >& args);

    const char* addString(const std::string& s) {
      _strings.push_back(s);
      return _strings.back().c_str();
    }

    void link();

    ~Runtime();

  private:
    Functions _functions;
    Strings _strings;

    NativeFunction* _topf;
    std::map<std::string, size_t> _topArgMap;

    char* _exec;
    size_t _codeSize;
  };
}
