#pragma once
#include <stdlib.h>
#include <string.h>
#include <mathvm.h>
#include <ast.h>
#include <vector>
#include <map>
#include "TranslationUtils.h"

class Executable {
    typedef std::map<uint16_t, MyBytecodeFunction*> IdToFunction;
    typedef std::map<MyBytecodeFunction*, uint16_t> FunctionToId;
    typedef std::map<uint16_t, std::string> IdToStringConstant;
    typedef std::map<uint16_t, char*> IdToCharPtr;
    typedef std::map<uint16_t, MyNativeFunction*> IdToNativeFunction;
    IdToFunction idToFunction;
    FunctionToId functionToId;
    IdToStringConstant idToStringConstant;
    IdToCharPtr idToCharPtr;
    IdToNativeFunction idToNativeFunction;
    uint16_t stringConstantCounter;
    TranslatableFunctions metaData;
public:
    Executable();
    virtual ~Executable();
    virtual mathvm::Status* execute(std::vector<mathvm::Var*, std::allocator<mathvm::Var*> >& vars);

    void addFunc(uint16_t id, MyBytecodeFunction* func) { idToFunction[id] = func; functionToId[func] = id;}
    
    void addNativeFunc(uint16_t id, MyNativeFunction* func) { idToNativeFunction[id] = func; }
    
    MyBytecodeFunction* funcById(uint16_t id) { 
      IdToFunction::iterator it = idToFunction.find(id); 
      if (it != idToFunction.end()) {
        return idToFunction[id]; 
      } else {
        return NULL;
      }
    }
    
    MyNativeFunction* nativeFuncById(uint16_t id) { 
      IdToNativeFunction::iterator it = idToNativeFunction.find(id); 
      if (it != idToNativeFunction.end()) {
        return idToNativeFunction[id]; 
      } else {
        return NULL;
      }
    }
    
    uint16_t idByFunc(mathvm::TranslatedFunction* func) { return functionToId[static_cast<MyBytecodeFunction*>(func)]; }
    
    MyBytecodeFunction* getMain() { return idToFunction.begin()->second; }
    
    size_t funcCount() { return idToFunction.size() + idToNativeFunction.size(); }
    
    uint16_t makeStringConstant(const std::string& str) {
      idToStringConstant[stringConstantCounter] = str; 
      char* ch = (char*)malloc(str.size() + 1);
      strcpy(ch, str.c_str());
      idToCharPtr[stringConstantCounter] = ch;
      return stringConstantCounter++; 
    }
    
    virtual void disassemble(std::ostream& out = std::cout, mathvm::FunctionFilter *f = 0) const;
    
    std::string const& sConstById(uint16_t id) { return idToStringConstant[id]; } 
    
    char* chConstById(uint16_t id) { return idToCharPtr[id]; } 

    TranslatableFunctions& getMetaData() { return metaData; }
};
