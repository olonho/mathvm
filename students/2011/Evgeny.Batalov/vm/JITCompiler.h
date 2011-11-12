#pragma once
#include <stdlib.h>
#include <asmjit/Compiler.h>
#include <asmjit/Logger.h>
#include <asmjit/MemoryManager.h>
#include "Executable.h"
#include "ByteCodeAnnotations.h"

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
  #define BP_ARG 0
  #define SP_ARG 1
  typedef void    (*VoidFunc)(void* bp, void* sp);
  typedef int64_t (*IntFunc)(void*, void*);
  typedef double  (*DoubleFunc)(void*, void*);
};
