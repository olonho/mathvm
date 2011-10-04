#pragma once

#include <assert.h>

#include <vector>

#include "mathvm.h"

// ================================================================================

namespace mathvm {
  typedef std::vector<std::string> StringPool;

  struct FunArg {
    const std::string* name;
    int8_t varId;
  };

  typedef std::vector<FunArg> FunArgs;

  // --------------------------------------------------------------------------------

  enum RTVarType {
    RVT_INVALID,
    RVT_DOUBLE,
    RVT_INT,
    RVT_STRING,
    RVT_LOCAL
  };

  class RTVar {
  public:
    RTVar() {
      _t = RVT_INVALID;
    }

    RTVar(int i) {
      _t = RVT_INT;
      _u.i = i;
    }

    RTVar(int64_t i) {
      _t = RVT_INT;
      _u.i = i;
    }

    RTVar(double d) {
      _t = RVT_DOUBLE;
      _u.d = d;
    }

    RTVar(char *s) {
      _t = RVT_STRING;
      _u.s = s;
    }

    RTVarType type() const { return _t; }

    int64_t getInt() const {
      assert(_t == RVT_INT);

      return _u.i;
    }

    double getDouble() const {
      assert(_t == RVT_DOUBLE);

      return _u.d;
    }

    const char *getString() const {
      assert(_t == RVT_STRING);

      return _u.s;
    }

  private:
    RTVarType _t;

    union {
      int64_t  i;
      double   d;
      char*    s;
      uint16_t localId;
    } _u;
  };


  class BytecodeInterpreter : public Code {
    struct Function {
      Bytecode code;
      FunArgs args;
    };

  public:
    void setVarPoolSize(unsigned int);

    Status* execute(vector<Var*> vars);
    
    Bytecode* bytecode();
    StringPool* strings();

    void createFunction(Bytecode** code, uint16_t* id, FunArgs** args);
    
  private:
    std::vector<RTVar> _varPool;
    std::vector<Function> _functions;
    StringPool string_pool;
    Bytecode _code;
  };
}
