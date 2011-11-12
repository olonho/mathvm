#pragma once 
#include <ostream>
#include <map>
#include <vector>
#include <string>
#include <utility>
#include <sys/time.h>
#include <ctime>
#include <ast.h>
#include <mathvm.h>

#define VERBOSE
#ifdef VERBOSE
#define DEBUG(str) std::cerr << str
#else
#define DEBUG(str)
#endif

#include "TranslationException.h"
#include "SymbolStack.h"
#include "ByteCodeAnnotations.h"
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
struct SymbolUse {
  SymbolUse(const std::string& symName, size_t funcId)//, mathvm::VarType type)
    : first(symName)
    , second(funcId)
    //, type(type)
  {}
  std::string first;
  size_t second;
  //mathvm::VarType type;
};
//typedef std::pair<std::string, size_t> SymbolUse; //symbol used, function_id
typedef std::vector<SymbolUse> SymbolsUse;
typedef std::pair<std::string, uint16_t> ClosureVarIdentity; //Closure var identity: name, owner id

struct FuncTypeInfo {
  typedef std::vector<mathvm::VarType> LocalsType;
  typedef std::map<ClosureVarIdentity, mathvm::VarType> ClosuresType;
  typedef std::map<std::string, mathvm::VarType> ParametersType;

  LocalsType      localsType;
  ClosuresType    closuresType;
  ParametersType  parametersType;
  mathvm::VarType returnType;
};
typedef std::vector<FuncTypeInfo> FuncTypeInfos;

bool symUsed(const SymbolsUse& a, const std::string& str, size_t user);
bool symUsed(const Strings& a, const std::string& str);
const SymbolUse& findSymUse(const SymbolsUse& sUses , const std::string& sym);

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
  FuncTypeInfo typeInfo;

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
    void goback()  { --curAddr; }
    uint16_t curAddr;
    std::vector<uint16_t> addresses;
  };

  typedef std::map<ClosureVarIdentity, VarAddresses> Addresses;
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
      addresses[std::make_pair(*it, proto.id)].addresses.push_back(frameOffset);
      addresses[std::make_pair(*it, proto.id)].curAddr = 0;
      DEBUG(*it << ":" << frameOffset << std::endl);
      frameOffset++;
      if (frameOffset == 0) throw new TranslationException("Var count reached maximum", 0); 
    }
    SymbolsUse::const_iterator it1 = proto.needForClosure.begin(); 
    for(;it1 != proto.needForClosure.end(); ++it1) {
      addresses[std::make_pair(it1->first, it1->second)].addresses.push_back(frameOffset);
      addresses[std::make_pair(it1->first, it1->second)].curAddr = 0;
      DEBUG(it1->first << " of " << it1->second << ":" << frameOffset << std::endl);
      frameOffset++;
      if (frameOffset == 0) throw new TranslationException("Var count reached maximum", 0); 
    }
    for(it = proto.locals.begin(); it != proto.locals.end(); ++it) {
      addresses[std::make_pair(*it, proto.id)].addresses.push_back(frameOffset);
      addresses[std::make_pair(*it, proto.id)].curAddr = -1;
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
      addresses[std::make_pair(*it, proto.id)].advance();
    }
  }

  void popSymbols(const Strings& symbols) {
    Strings::const_iterator it = symbols.begin();
    for(; it != symbols.end(); ++it) {
      //DEBUG(proto.id << "Go back: " << *it  << std::endl);
      addresses[std::make_pair(*it, proto.id)].goback();
    }
  }
  //for local variables, parameters and symbols needed in context of this function 
  uint16_t getAddress(const std::string& varName) const {
    Addresses::const_iterator it = addresses.find(std::make_pair(varName, proto.id));
    if (it == addresses.end()) {
      //var is not local, lookup int iUsedSymbols
      const SymbolUse& sUse = findSymUse(proto.iUsedSymbols, varName);
      it = addresses.find(std::make_pair(sUse.first, sUse.second));
    }
    if (it == addresses.end()) {
      throw new TranslationException("Internal error", 0); 
    }
    DEBUG("Getting address in " << proto.funcName << " of " << 
        varName << " with value " <<  it->second.addresses[it->second.curAddr]
        << " and offset " << it->second.curAddr << std::endl);
    return (uint16_t)it->second.addresses[it->second.curAddr];
  }

  //if you want to find address of var of other func in stack of this func
  uint16_t getAddress(const std::string& varName, uint16_t ownerId) const {
    Addresses::const_iterator it = addresses.find(std::make_pair(varName, ownerId));
    if (it == addresses.end()) {
      throw new TranslationException("Internal error", 0); 
    }
    DEBUG("Getting address in " << proto.funcName << " of " << 
        varName << " owned by " << ownerId << " with value " <<  it->second.addresses[it->second.curAddr]
        << " and offset " << it->second.curAddr << std::endl);
    return (uint16_t)it->second.addresses[it->second.curAddr];
  }

  void genCallingCode(TranslatableFunction& callingFunc, mathvm::AstVisitor* codeGenerator, 
      mathvm::Bytecode& bcode, mathvm::CallNode* callNode, mathvm::VarType retType) {
    using namespace mathvm;
    bcode.addByte(BCA_FCALL_BEGIN);
    if (retType != VT_VOID) {
      //push curent value of IVAR0 register on stack
      bcode.addByte(BCA_VM_SPECIFIC);
      bcode.addByte(BC_LOADIVAR0);
    }
    //push parameters
    Strings::const_iterator strsIt = proto.parameters.begin();
    size_t i = 0;
    for(; strsIt != proto.parameters.end(); ++strsIt) {
      callNode->parameterAt(i)->visit(codeGenerator);
      bcode.addByte(BCA_FPARAM_COMPUTED);
      bcode.addByte(proto.typeInfo.parametersType[proto.parameters[i]]);
      ++i;
    }
    //push closures
    SymbolsUse::const_iterator symUseIt = proto.needForClosure.begin();
    for(;symUseIt != proto.needForClosure.end(); ++symUseIt) {
      switch(proto.typeInfo.closuresType[std::make_pair(symUseIt->first, symUseIt->second)]) {
        case VT_INT: case VT_STRING:
          bcode.addByte(BC_LOADIVAR);
          bcode.addByte(BCA_FPARAM_COMPUTED);
          bcode.addByte(VT_INT);
        break;
        case VT_DOUBLE:
          bcode.addByte(BC_LOADDVAR);
          bcode.addByte(BCA_FPARAM_COMPUTED);
          bcode.addByte(VT_DOUBLE);
        break;
        default:
        break;
      }
      bcode.addTyped(callingFunc.getAddress(symUseIt->first, symUseIt->second));
    }
    //push locals
    for(size_t i = 0; i != proto.locals.size(); ++i) {
      switch(proto.typeInfo.localsType[i]) {
        case VT_INT: case VT_STRING:
          bcode.addByte(BC_ILOAD0);
          bcode.addByte(BCA_FPARAM_COMPUTED);
          bcode.addByte(VT_INT);
          break;
        case VT_DOUBLE:
          bcode.addByte(BC_DLOAD0);
          bcode.addByte(BCA_FPARAM_COMPUTED);
          bcode.addByte(VT_DOUBLE);
          break;
        default:
          break;
      }
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
      bcode.addByte(BCA_FPARAM_CLEANUP);
      bcode.addByte(mathvm::BC_POP);
    }
    //pop and save closures
    SymbolsUse::const_reverse_iterator revSymUseIt = proto.needForClosure.rbegin(); 
    for(;revSymUseIt != proto.needForClosure.rend(); ++revSymUseIt) {
      bcode.addByte(BCA_FPARAM_CLOSURE_SAVE);
      switch(proto.typeInfo.closuresType[std::make_pair(revSymUseIt->first, revSymUseIt->second)]) {
        case VT_INT: case VT_STRING:
          bcode.addByte(BC_STOREIVAR);
          break;
        case VT_DOUBLE:
          bcode.addByte(BC_STOREDVAR);
          break;
        default:
          break;
      }
      bcode.addTyped(callingFunc.getAddress(revSymUseIt->first, revSymUseIt->second));
    }
    //pop parameters
    for(strsIt = proto.parameters.begin(); strsIt != proto.parameters.end(); ++strsIt) {
      bcode.addByte(BCA_FPARAM_CLEANUP);
      bcode.addByte(BC_POP);
    }
    if (retType != VT_VOID) {
      //push function result back on stack
      bcode.addByte(BCA_VM_SPECIFIC);
      bcode.addByte(BC_LOADIVAR0);
      //swap with old value of IVAR0 register
      bcode.addByte(BCA_VM_SPECIFIC);
      bcode.addByte(BC_SWAP);
      //restore IVAR0 register to value before call
      bcode.addByte(BCA_VM_SPECIFIC);
      bcode.addByte(BC_STOREIVAR0);
    }
    bcode.addByte(BCA_FCALL_END);
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

    static const char* bcName(mathvm::Instruction insn, size_t& length) {
      using namespace mathvm;
      static const struct {
        const char* name;
        Instruction insn;
        size_t length;
      } names[] = {
#define BC_NAME(b, d, l) {#b, BC_##b, l},
        FOR_BYTECODES(BC_NAME)
      };

      if (insn >= BC_INVALID && insn < BC_LAST) {
        length = names[insn].length;
        return names[insn].name;
      }
      BytecodeAnnotations anot = (BytecodeAnnotations)insn;
      if (anot == BCA_FCALL_BEGIN) {
        length = 1; return "BCA_FCALL_BEGIN"; }
      if (anot == BCA_FCALL_END) {
        length = 1; return "BCA_FCALL_END"; }
      if (anot == BCA_FPARAM_COMPUTED) {
        length = 2; return "BCA_FPARAM_COMPUTED"; }
      if (anot == BCA_FPARAM_CLEANUP) {
        length = 1; return "BCA_FPARAM_CLEANUP"; }
      if (anot == BCA_FPARAM_CLOSURE_SAVE) {
        length = 1; return "BCA_FPARAM_CLOSURE_SAVE"; }
      if (anot == BCA_VM_SPECIFIC) {
        return "BCA_VM_SPECIFIC"; }

      assert(false);
      return 0;
    }

    virtual void dump(std::ostream& out) const {
      using namespace mathvm;
      for (size_t bci = 0; bci < length();) {
        size_t length;
        Instruction insn = getInsn(bci);
        out << bci << ": ";
        const char* name = bcName(insn, length);
        switch (insn) {
          case BC_DLOAD:
            out << name << " " << getDouble(bci + 1);
            break;
          case BC_ILOAD:
            out << name << " " << getInt64(bci + 1);
            break;
          case BC_SLOAD:
            out << name << " @" << getUInt16(bci + 1);
            break;
          case BC_CALL:
            out << name << " *" << getUInt16(bci + 1);
            break;
          case BC_LOADDVAR:
          case BC_STOREDVAR:
          case BC_LOADIVAR:
          case BC_STOREIVAR:
          case BC_LOADSVAR:
          case BC_STORESVAR:
            out << name << " @" << getUInt16(bci + 1);
            break;
          case BC_LOADCTXDVAR:
          case BC_STORECTXDVAR:
          case BC_LOADCTXIVAR:
          case BC_STORECTXIVAR:
          case BC_LOADCTXSVAR:
          case BC_STORECTXSVAR:
            out << name << " @" << getUInt16(bci + 1)
              << ":" << getUInt16(bci + 3);
            break;
          case BC_IFICMPNE:
          case BC_IFICMPE:
          case BC_IFICMPG:
          case BC_IFICMPGE:
          case BC_IFICMPL:
          case BC_IFICMPLE:
          case BC_JA:
            out << name << " " << getInt16(bci + 1) + bci + 1;
            break;
          default:
            out << name;
        }
        out << std::endl;
        bci += length;
      }
    }
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

typedef union {
  double   *double_;
  int64_t  *int_;
  uint16_t *str_;
  uint8_t  *instr_;
  int16_t  *jmp_;
  uint16_t *var_;
  void*    *ptr_;
  uint8_t  *byte_;
} ByteCodeElem;

inline bool operator<(const ByteCodeElem& a, const ByteCodeElem& b) {
  return a.instr_ < b.instr_;
}

inline bool operator==(const ByteCodeElem& a, const ByteCodeElem& b) {
  return a.instr_ == b.instr_;
}

struct VMRegisters {
  uint64_t ir0;
  uint64_t ir1;
  uint64_t ir2;
  uint64_t ir3;
  double dr0;
  double dr1;
  double dr2;
  double dr3;
};
