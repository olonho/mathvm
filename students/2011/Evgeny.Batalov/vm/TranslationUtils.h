#pragma once 
#include <map>
#include <vector>
#include <string>
#include <sys/time.h>
#include <ctime>
#include <ast.h>
#include <mathvm.h>

//#define VERBOSE
#ifdef VERBOSE
#define DEBUG(str) std::cerr << str
#else
#define DEBUG(str)
#endif

#include "TranslationException.h"
#include "SymbolStack.h"

mathvm::VarType upType(mathvm::VarType t1, mathvm::VarType t2);

struct NodeInfo {
  mathvm::VarType type;
  mathvm::VarType convertTo;
};

class NodeInfos {
  typedef std::map<mathvm::AstNode*, NodeInfo> NodeInfoMap;
  typedef NodeInfoMap::iterator MapIt;
  NodeInfoMap map;
public:

  void setNodeInfo(mathvm::AstNode* node, mathvm::VarType type, mathvm::VarType convertTo = mathvm::VT_INVALID) {
    NodeInfo& n = map[node];
    n.type = type;
    n.convertTo = convertTo;
  }

  NodeInfo& getNodeInfo(mathvm::AstNode* node) {
    MapIt it = map.find(node);
    if (it != map.end()) {
        return it->second;
    }
    throw new TranslationException("NodeInfo for AstNode not found", node);
    return *(NodeInfo*)0;
  }
};

typedef std::vector<std::string> Strings;
typedef std::pair<std::string, size_t> SymbolUse; //symbol used, function_id
typedef std::vector<SymbolUse> SymbolsUse;

bool symUsed(const SymbolsUse& a, const std::string& str, size_t user);
//bool symUsed(const SymbolsUse& a, const std::string& str);
bool symUsed(const Strings& a, const std::string& str);

struct FunctionContext {
  size_t id;
  std::string funcName;
  Strings parameters;
  Strings locals; //includes vars in child scopes
  //track use of external symbols only
  SymbolsUse iUsedSymbols;
  SymbolsUse mySymbolsUsed; //I have to put following symbols on stack when I call functions
  //caller have to put following symbols on stack when call me
  SymbolsUse needForClosure;//= (needForClosure of all children + iUsedSymbols) - mySymbolsUsed
  std::vector<size_t> calledFuncs;

  void useMySymbol(std::string sym, size_t userFuncId) {
    if (symUsed(mySymbolsUsed, sym, userFuncId))
      return;
    if (symUsed(parameters, sym) || symUsed(locals, sym)) {
      mySymbolsUsed.push_back(SymbolUse(sym, userFuncId));
    } else {
      throw new TranslationException("use of symbol " + sym  + "  not defined in my scope", 0);
    }
  }  
};

void useSymbol(size_t userFuncId, std::string sym, size_t symFuncId, std::vector<FunctionContext>& contexts);
typedef std::vector<FunctionContext> FunctionContexts;
typedef std::map<mathvm::FunctionNode*, size_t> FunctionNodeToIndex;
typedef std::map<size_t, mathvm::FunctionNode*> IndexToFunctionNode;

//function which can allocate stack for its variables relative to begin of frame
//frame: parameters, closures, locals | computational stack
class TranslatableFunction {
  
  struct VarAddresses {
    VarAddresses() 
      : curAddr() {}
    void advance() {  //uint16_t oldAddr = curAddr;
                      //if (oldAddr > ++curAddr) { throw new TranslationException("Var count reached maximum", 0); }
                      if (++curAddr == addresses.size()) throw new TranslationException("VarAdresses:advance out of bounds", 0);}
    void goback()  {  --curAddr; }
    uint16_t curAddr;
    std::vector<uint16_t> addresses;
  };

  typedef std::map<std::string, VarAddresses> Addresses;
  Addresses addresses;
  FunctionContext proto;
  size_t frameSize;
public:
  
  const FunctionContext& getProto() const {
    return proto;
  }

  TranslatableFunction(const FunctionContext& proto)
    : proto(proto) 
  {
    Strings::const_iterator it = proto.parameters.begin();
    uint16_t frameOffset = 0;
    DEBUG( "Creating translatable function " << proto.funcName << ":" << proto.id  << std::endl);
    for(; it != proto.parameters.end(); ++it) {
      addresses[*it].addresses.push_back(frameOffset);
      addresses[*it].curAddr = 0;
      DEBUG(*it << ":" << frameOffset << std::endl);
      frameOffset++;
      if (frameOffset == 0) throw new TranslationException("Var count reached maximum", 0); 
    }
    SymbolsUse::const_iterator it1 = proto.needForClosure.begin(); 
    for(;it1 != proto.needForClosure.end(); ++it1) {
      addresses[it1->first].addresses.push_back(frameOffset);
      addresses[it1->first].curAddr = 0;
      DEBUG(it1->first << ":" << frameOffset << std::endl);
      frameOffset++;
      if (frameOffset == 0) throw new TranslationException("Var count reached maximum", 0); 
    }
    for(it = proto.locals.begin(); it != proto.locals.end(); ++it) {
      addresses[*it].addresses.push_back(frameOffset);
      addresses[*it].curAddr = -1;
      DEBUG(*it << ":" << frameOffset << std::endl);
      frameOffset++;
      if (frameOffset == 0) throw new TranslationException("Var count reached maximum", 0); 
    }
    frameSize = frameOffset;
    DEBUG("Stack frame size = " << frameSize << std::endl);
  }

  size_t getFrameSize() {
    return frameSize;
  }

  void pushSymbols(const Strings& symbols) {
    Strings::const_iterator it = symbols.begin();
    for(; it != symbols.end(); ++it) {
      //DEBUG(proto.id << " Advance: " << *it  << std::endl);
      addresses[*it].advance();
    }
  }

  void popSymbols(const Strings& symbols) {
    Strings::const_iterator it = symbols.begin();
    for(; it != symbols.end(); ++it) {
      //DEBUG(proto.id << "Go back: " << *it  << std::endl);
      addresses[*it].goback();
    }
  }

  uint16_t getAddress(const std::string& varName) const {
    Addresses::const_iterator it = addresses.find(varName);
    DEBUG("Getting address in " << proto.funcName << " of " << 
    varName << " with value " <<  it->second.addresses[it->second.curAddr]
    << " and offset " << it->second.curAddr << std::endl);
    return (uint16_t)it->second.addresses[it->second.curAddr];
  }

  void genCallingCode(TranslatableFunction& callingFunc, mathvm::AstVisitor* codeGenerator, 
                      mathvm::Bytecode& bcode, mathvm::CallNode* callNode, mathvm::VarType retType) const {
    using namespace mathvm;
    if (retType != VT_VOID) {
      //push curent value of IVAR0 register on stack
      bcode.addByte(BC_LOADIVAR0);
    }
    //push parameters
    Strings::const_iterator strsIt = proto.parameters.begin();
    size_t i = 0;
    for(; strsIt != proto.parameters.end(); ++strsIt) {
      callNode->parameterAt(i)->visit(codeGenerator);
      ++i;
    }
    //FIXME: UNTYPED - better to make typed
    //push closures
    SymbolsUse::const_iterator symUseIt = proto.needForClosure.begin();
    for(;symUseIt != proto.needForClosure.end(); ++symUseIt) {
      bcode.addByte(mathvm::BC_LOADIVAR);
      bcode.addTyped(callingFunc.getAddress(symUseIt->first));
    }
    //push locals
    for(strsIt = proto.locals.begin(); strsIt != proto.locals.end(); ++strsIt) {
      bcode.addByte(mathvm::BC_ILOAD0);
    }
    //call func
    bcode.addByte(BC_CALL);
    bcode.addTyped((uint16_t)proto.id);
    if (retType != VT_VOID) {
      //store function result to IVAR0 register
      bcode.addByte(BC_STOREIVAR0);
    }
    //pop locals
    for(strsIt = proto.locals.begin(); strsIt != proto.locals.end(); ++strsIt) {
      bcode.addByte(mathvm::BC_POP);
    }
    //pop and save closures
    SymbolsUse::const_reverse_iterator revSymUseIt = proto.needForClosure.rbegin(); 
    for(;revSymUseIt != proto.needForClosure.rend(); ++revSymUseIt) {
      bcode.addByte(BC_STOREIVAR);
      bcode.addTyped(callingFunc.getAddress(revSymUseIt->first));
    }
    //pop parameters
    for(strsIt = proto.parameters.begin(); strsIt != proto.parameters.end(); ++strsIt) {
      bcode.addByte(BC_POP);
    }
    if (retType != VT_VOID) {
      //push function result back on stack
      bcode.addByte(BC_LOADIVAR0);
      //swap with old value of IVAR0 register
      bcode.addByte(BC_SWAP);
      //restore IVAR0 register to value before call
      bcode.addByte(BC_STOREIVAR0);
    }
  }
};

typedef std::vector<TranslatableFunction> TranslatableFunctions;

struct ParamInfo {
  size_t index;
  std::string name;
  mathvm::VarType type;
};

class Params {
  typedef std::map<std::string, ParamInfo> ParamMap;
  typedef ParamMap::iterator ParamIt;
  typedef ParamMap::const_iterator ParamItConst;

  ParamMap map;
  public:
  mathvm::VarType returnType;
  ParamInfo& getParamInfo(size_t index) {
    for(ParamIt it = map.begin(); it != map.end(); ++it) {
      if (it->second.index == index) {
        return it->second;
      }
    }
    new TranslationException("parameter for function  not found", 0);
    return *(ParamInfo*)0;
  }

  ParamInfo& getParamInfo(const std::string& pName) {
    ParamIt p = map.find(pName);
    if (p != map.end()) {
      return p->second;
    }
    throw new TranslationException("parameter " + pName  +  " not found", 0);
    return *(ParamInfo*)0;
  }

  void setParamInfo(ParamInfo& info) {
    map[info.name] = info;
  }

  size_t size() const {
    return map.size();
  }
};

class MyBytecode: public mathvm::Bytecode {
  public:
    uint8_t* raw() { return &_data.front(); }
    std::vector<uint8_t>& data() { return _data; }
};

class MyBytecodeFunction: public mathvm::TranslatedFunction {
  MyBytecode *_bytecode;
  TranslatableFunction* _translatableFunc;
  public:
  MyBytecodeFunction(mathvm::AstFunction* function, TranslatableFunction* translatableFunc) 
    : mathvm::TranslatedFunction(function)
      , _bytecode(new MyBytecode())
      , _translatableFunc(translatableFunc)
  {}

  ~MyBytecodeFunction() {
    delete _bytecode;
  }

  MyBytecode* bytecode() {
    return _bytecode;
  }

  TranslatableFunction* translatableFunc() {
    return _translatableFunc;
  }

  void setBytecode(MyBytecode* bytecode) {
    _bytecode = bytecode;
  }

  virtual void disassemble(std::ostream& out) const {
    _bytecode->dump(out);
  }
};

inline uint64_t getTimeMs() {
  struct timeval tv;
  uint64_t ret;
  gettimeofday(&tv, NULL);
  ret = tv.tv_usec;
  ret /= 1000;
  ret += (tv.tv_sec * 1000);
  return ret;
}
