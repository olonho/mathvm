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

    RVT_ARG,
    RVT_LOCAL
  };

  class RTVar {
  public:
    RTVar() {
      _t = RVT_INVALID;
    }

    RTVar(RTVarType t) {
      _t = t;
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

    uint16_t getRef() const {
      assert(_t == RVT_ARG || _t == RVT_LOCAL);

      return _u.ref;
    }

  private:
    RTVarType _t;

    union {
      int64_t  i;
      double   d;
      char*    s;
      uint16_t ref;
    } _u;
  };

  // --------------------------------------------------------------------------------

  class BCIFunction : public BytecodeFunction {
  public:
    BCIFunction(AstFunction* af)
      : BytecodeFunction(af) { 
      //_retType = af->returnType();
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

    /*
    VarType returnType() const {
      return _retType;
    }
    */

  private:
    uint16_t _firstArg;
    uint16_t _firstLocal;
    VarType _retType;
  };

  // --------------------------------------------------------------------------------

  class BytecodeInterpreter : public Code {
    struct Function {
      Bytecode code;
      FunArgs args;
    };


    /* The stack model that resembles a hardware stack */
#define STACK_SIZE 1024*1024

    class Stack {
    public:
      Stack() {
        _top = 0;
        _data = new unsigned char[STACK_SIZE]; 
      }

      void init() {
        _top = 0;
        _data = new unsigned char[STACK_SIZE]; 
      }

      ~Stack() {
        delete [] _data;
      }

      template<typename T>
      void push(T v) {
        assert(_top + sizeof(T) < STACK_SIZE);

        *(T*)(_data + _top) = v;
        _top += sizeof(T);
      }

      template<typename T>
      T pop() {
        T v = *((T*)&_data[_top - sizeof(T)]);
        _top -= sizeof(T);
        return v;
      }

      RTVar pop() {
        return pop<RTVar>();
      }

      template<typename T>
      T& get(size_t offset) {
        //assert(_top - offset > 0);

        return *(T*)(_data + offset);
      }

      template<typename T>
      void set(size_t offset, T v) {
        *(T*)(_data + offset) = v;
      }

      void advance(size_t size) {
        _top += size;
      }

      void reduce(size_t size) {
        assert(_top >= size);

        _top -= size;
      }

      template<typename T>
      T& top() {
        return *(T*)(_data + _top - sizeof(T));
      }

      RTVar& top() {
        return top<RTVar>();
      }

      size_t pos() const {
        return _top;
      }

    private:
      size_t _top;
      unsigned char* _data;
    };


    struct CallStackEntry {
      CallStackEntry(BCIFunction* f, uint32_t ip, size_t frame) {
        oldFunction = f;
        oldIP = ip;
        oldFrame = frame;
      }
      
      void restore(BCIFunction** f, uint32_t* ip, size_t* frame) {
        *f = oldFunction;
        *ip = oldIP;
        *frame = oldFrame;
      }

      void restore(uint32_t* ip) {
      }

      const BCIFunction* function() const {
        return oldFunction;
      }

      size_t framePtr() const {
        return oldFrame;
      }

      BCIFunction* oldFunction;
      uint32_t oldIP;
      size_t oldFrame;
    };

    typedef std::vector<CallStackEntry> CallStack;


  public:
    BytecodeInterpreter() 
      : _stack() { }

    void setVarPoolSize(unsigned int);

    Status* execute(vector<Var*> vars) { return NULL; }
    
    Bytecode* bytecode();
    StringPool* strings();

    void createFunction(Bytecode** code, uint16_t* id, FunArgs** args);

    void createFunction(BCIFunction** function, AstFunction* fNode);

    Status* execute(std::vector<mathvm::Var*, std::allocator<Var*> >&);
    
  private:
    void createStackFrame();
    void leaveStackFrame();

    RTVar& frameVar(uint16_t idx, const CallStackEntry&);
    RTVar& arg(uint16_t idx, const CallStackEntry&);
    RTVar& var(uint16_t idx);

    //std::vector<RTVar> _varPool;
    std::vector<BCIFunction*> _functions;
    StringPool _stringPool;
    //Bytecode _code;
    //size_t _framePos;
    //BCIFunction* _curFun;

    Stack _stack;
    CallStack _callStack;
  };
}
