#pragma once 
#include <map>
#include <vector>
#include <string>
#include <ast.h>
#include <mathvm.h>
#include "TranslationException.h"
#include <SymbolStack.h>

mathvm::VarType upType(mathvm::VarType t1, mathvm::VarType t2);

struct NodeInfo {
  mathvm::VarType type;
  mathvm::VarType convertTo;
};

class NodeInfos {
  typedef std::map<mathvm::AstNode*, NodeInfo> NodeInfoMap;
  typedef typename NodeInfoMap::iterator MapIt;
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
    throw new TranslationException("NodeInfo for AstNode not found")
    return *(NodeInfo*)0;
  }
};

typedef std::vector<std::string> Strings;
typedef std::pair<std::string, size_t> SymbolUse; //symbol used, function_id
typedef std::vector<SymbolUse> SymbolsUse;

bool symUsed(const SymbolsUse& a, const std::string& str);
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
      throw new TranslationException("use of symbol " + sym  + "  not defined in my scope");
    }
  }  
};

void useSymbol(size_t userFuncId, std::string sym, size_t symFuncId, std::vector<FunctionContext>& contexts);
typedef std::vector<FunctionContext> FunctionContexts;
void genClosures(FunctionContext& cont, FunctionContexts& conts);

typedef std::map<mathvm::FunctionNode*, size_t> FunctionNodeToIndex;
typedef std::map<size_t, mathvm::FunctionNode*> IndexToFunctionNode;

//function which can allocate stack for its variables relative to begin of frame
//frame: parameters, closures, locals | computational stack
class TranslatableFunction {
  struct VarAdresses {
    VarAdresses() 
      : curAddr(0) {}
    void advance() { ++curAddr; if (curAddr == addresses.size()) throw new TranslationException("VarAdresses:advance");}
    void goback()  { --curAddr; if (curAddr < 0) throw new TranslationException("VarAdresses:goback");}
    size_t curAddr;
    std::vector<int16_t> addresses;
  };
  typedef std::map<std::string, VarAddresses> Addresses;
  Addresses addresses;

  FunctionContext proto;
public:
  FunctionScope(const FunctionContext& proto)
    : proto(proto) 
  {
    Strings::const_iterator it = proto.parameters.begin();
    int16_t frameOffset = 0;
    for(; it != proto.parameters.end(); ++it) {
      addresses[*it].addresses.push_back(frameOffset);
      frameOffset++;
    }
    for(it = proto.needForClosure.begin(); it != proto.needForClosure.end(); ++it) {
      addresses[*it].addresses.push_back(frameOffset);
      frameOffset++;
    }
    for(it = proto.locals.begin(); it != proto.locals.end(); ++it) {
      addresses[*it].addresses.push_back(frameOffset);
      frameOffset++;
    }
    if (frameOffset != 0)
      throw new TranslationException("frameOffset != 0");
  }

  void pushSymbols(const Strings& symbols) {
    Strings::iterator it = symbols.begin();
    for(; it != symbols.end(); ++it) {
      addresses[*it].advance();
    }
  }

  void popSymbols(const Strings& symbols) {
    Strings::iterator it = symbols.begin();
    for(; it != symbols.end(); ++it) {
      addresses[*it].goback();
    }
  }

  uint16_t getAddress(const std::string& varName) const {
    return addresses[varName].addresses[addresses[varName].curAddr];
  }

  void genCallingCode(FunctionContext& callingFunc, mathvm:::AstVisitor* codeGenerator, 
                      mathvm::Bytecode& bcode, mathvm::CallNode* callNode) const {
    //push curent value of IVAR0 register on stack
    code.addByte(BC_LOADIVAR0);
    //push parameters
    Strings::const_iterator it = proto.parameters.begin();
    size_t i = 0;
    for(; it != proto.parameters.end(); ++it) {
      callNode->parameterAt(i)->visit(codeGenerator);
      ++i;
    }
    //FIXME: UNTYPED - better to make typed
    //push closures
    for(it = proto.needForClosure.begin(); it != proto.needForClosure.end(); ++it) {
      bcode.addByte(mathvm::BC_LOADIVAR);
      bcode.addTyped(callingFunc.getAddress(*it));
    }
    //push locals
    for(it = proto.locals.begin(); it != proto.locals.end(); ++it) {
      bcode.addByte(mathvm::BC_ILOAD0);
    }
    //call func
    bcode.addByte(BC_CALL);
    bcode.addTyped((uint16_t)proto.id);
    //store function result to IVAR0 register
    bcode.addByte(BC_STOREIVAR0);
    //pop locals
    for(it = proto.locals.begin(); it != proto.locals.end(); ++it) {
      bcode.addByte(mathvm::BC_POP);
    }
    //pop and save closures
    for(it = proto.needForClosure.rbegin(); it != proto.needForClosure.rend(); ++it) {
      bcode.addByte(BC_STOREIVAR);
      bcode.addTyped(callingFunc.getAdress(*it));
    }
    //pop parameters
    for(it = proto.parameters.begin(); it != proto.parameters.end(); ++it) {
      bcode.addByte(BC_POP);
    }
    //push function result back on stack
    code.addByte(BC_LOADIVAR0);
    //swap with old value of IVAR0 register
    code.addByte(BC_SWAP);
    //restore IVAR0 register to value before call
    code.addByte(BC_STOREIVAR0);
  }
};

typedef std::vector<TranslatableFunction> TranslatableFunctions;

class Params {
  typedef std::map<std::string, ParamInfo> ParamMap;
  typedef Params::iterator ParamIt;
  typedef Params::const_iterator ParamItConst;

  ParamMap map;
  public:
  mathvm::VarType returnType;
  ParamInfo& getParamInfo(size_t index) {
    for(ParamIt it = map.begin(); it != map.end(); ++it) {
      if (it->second.index == index) {
        return it->second;
      }
    }
    new TranslationException("parameter for function  not found");
    return *(ParamInfo*)0;
  }

  ParamInfo& getParamInfo(const std::string& pName) {
    ParamIt p = map.find(pName);
    if (p != map.end()) {
      return p->second;
    }
    throw new TranslationException("parameter " + pName  +  " for function " + fName  +  " not found");
    return *(ParamInfo*)0;
  }

  void setParamInfo(ParamInfo& info) {
    map[info.name] = info;
  }

  size_t size() const {
    return map.size();
  }
};


