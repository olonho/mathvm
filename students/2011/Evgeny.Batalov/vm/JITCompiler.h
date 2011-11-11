#pragma once
#include <stdlib.h>
#include "Executable.h"
#include <asmjit/Compiler.h>
#include <asmjit/Logger.h>
#include <asmjit/MemoryManager.h>

typedef int (*MainFunc)(void*);

class JITCompiler {
  Executable& executable;
  void compileFunc(size_t funcId);
  void* *cFuncPtrs;
  char* copyToStringPull(const char* str);
  std::vector<char*> stringPull;
public:
  JITCompiler(Executable& executable)
    : executable(executable)
  {
    cFuncPtrs = (void**)malloc(sizeof(void*) * executable.funcCount());
  }
  void compileAll();
  void* *getCompFuncPtrs() { return cFuncPtrs; }
  std::vector<char*> getStringPull() { return stringPull; }
  
  virtual ~JITCompiler() {
    //MemoryManager::getGlobal()->free((void*)fn3);
  }
  typedef void    (*VoidFunc)(void*, void*);
  typedef int64_t (*IntFunc)(void*, void*);
  typedef double  (*DoubleFunc)(void*, void*);
};
