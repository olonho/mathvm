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

  /*
class Test {
public:
  int testFun(int x) {
    return 2*x;
  }
};
  */


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

    // --------------------------------------------------------------------------------

    typedef std::deque<Ref> Refs;
    typedef std::map<const AstVar*, FlowVar*> Vars;

    // --------------------------------------------------------------------------------

    NativeFunction(AstFunction* af) 
      : TranslatedFunction(af),
        _linked(false),
        _extVars(0) { }

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

    size_t externVars() const {
      return _extVars;
    }

    void addExternVar(const AstVar* v, FlowVar* fv) {
      //fv->kind = FlowVar::FV_LOCAL;
      fv->_stor = FlowVar::STOR_EXTERN;
      fv->_storIdx = _extVars;

      _vars.insert(std::make_pair(v, fv));      
      _extVars++;
    }

    void addLocalVar(const AstVar* v, FlowVar* fv) {
      if (v) {
        _vars.insert(std::make_pair(v, fv));
      }
      setLocalsNumber(localsNumber() + 1);
    }

    FlowVar* fvar(const AstVar* v) {
      Vars::iterator it = _vars.find(v);
      if (it != _vars.end()) {
        return it->second;
      } else {
        return NULL;
      }
    }

    FlowVar* fvar2(const AstVar* v) {
      Vars::iterator it = _vars.find(v);
      if (it != _vars.end()) {
        return it->second;
      } else {
        return NULL;
      }
    }

    Vars& vars() {
      return _vars;
    }

  private:
    //typedef std::deque<AstVar*> ExternVars;

    VarType _retType;    
    NativeCode _code;
    Refs _inRefs;   // who references us
    Refs _outRefs;  // those we reference

    size_t _callStor;

    char* _start;   // function entry point address
    bool _linked;

    size_t _extVars;

    //ExternVars _externVars;
    Vars _vars;
  };

  // --------------------------------------------------------------------------------

  class Runtime : public Code {
    typedef std::deque<std::string> Strings;
    typedef std::deque<NativeFunction*> Functions;
    //typedef std::deque<size_t> NFs;
    typedef std::map<std::string, size_t> NFMap;

  public:
    Runtime();

    NativeFunction* createFunction(AstFunction* fNode);

    Status* execute(std::vector<mathvm::Var*, std::allocator<Var*> >& args);

    const char* addString(const std::string& s) {
      _strings.push_back(s);
      return _strings.back().c_str();
    }

    size_t addNativeFunction(const std::string& name);

    void link();

    ~Runtime();

  private:
    Functions _functions;
    Strings _strings;

    void* _mainHandle;
    //NFs _nfs;
    NFMap _nfMap;

    NativeFunction* _topf;
    std::map<std::string, size_t> _topArgMap;

    char* _exec;
    size_t _codeSize;
  };
}
