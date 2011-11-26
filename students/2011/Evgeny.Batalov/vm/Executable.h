#pragma once
#include <mathvm.h>
#include <ast.h>
#include <vector>
#include <map>
#include "TranslationUtils.h"

class Executable {
    typedef std::map<uint16_t, MyBytecodeFunction*> IdToFunction;
    typedef std::map<MyBytecodeFunction*, uint16_t> FunctionToId;
    typedef std::map<uint16_t, std::string> IdToStringConstant;
    IdToFunction idToFunction;
    FunctionToId functionToId;
    IdToStringConstant idToStringConstant;
    uint16_t stringConstantCounter;
    TranslatableFunctions metaData;
public:
    Executable();
    virtual ~Executable();
    virtual mathvm::Status* execute(std::vector<mathvm::Var*, std::allocator<mathvm::Var*> >& vars);

    void addFunc(uint16_t id, MyBytecodeFunction* func) { idToFunction[id] = func; functionToId[func] = id;}
    
    MyBytecodeFunction* funcById(uint16_t id) { return idToFunction[id]; }
    
    uint16_t idByFunc(mathvm::TranslatedFunction* func) { return functionToId[static_cast<MyBytecodeFunction*>(func)]; }
    
    MyBytecodeFunction* getMain() { return idToFunction.begin()->second; }
    
    size_t funcCount() { return idToFunction.size(); }
    
    uint16_t makeStringConstant(const std::string& str) { idToStringConstant[stringConstantCounter] = str; return stringConstantCounter++; }
    
    virtual void disassemble(std::ostream& out = std::cout, mathvm::FunctionFilter *f = 0) const;
    
    std::string const& sConstById(uint16_t id) { return idToStringConstant[id]; } 

    TranslatableFunctions& getMetaData() { return metaData; }
};
