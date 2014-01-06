#include <mathvm.h>
#include <visitors.h>
#include <parser.h>
#include <ast.h>

#include <stdexcept>
#include <memory>

#include "InterpreterCodeImpl.h"


namespace mathvm {

uint16_t ID_MAX = 0xFFFF;

class BytecodeScope {
  typedef std::map<std::string, uint16_t> FunctionMap;
  typedef std::map<std::string, uint16_t> VarMap;
  typedef std::pair<uint16_t, uint16_t>   VarId; // (scopeId, varId);

public:
  BytecodeScope(Code* code) : _id(0), _parent(0), _code(code), _all_scopes(new std::vector<BytecodeScope *>(ID_MAX)) {
    _all_scopes->push_back(this);
  }
  
  uint16_t addFunction(BytecodeFunction* function) {
    uint16_t id = _code->addFunction(function);
    if (id >= ID_MAX || _functions.find(function->name()) != _functions.end()) return ID_MAX;
    function->setScopeId(_id);
    _functions[function->name()] = id;
    return id;
  }
  
  uint16_t addVariable(std::string const & name, VarType type) {
    if (_variables_map.find(name) != _variables_map.end() || _variables.size() >= ID_MAX) return ID_MAX;
    uint16_t newId = _variables.size();
    _variables.push_back(new Var(type, name));
    _variables_map[name] = newId;
    return newId;
  }

  uint16_t id() const {
    return _id;
  }

  VarId resolveVariable(std::string const & varName) const {
    VarMap::const_iterator i = _variables_map.find(varName);
    if (i == _variables_map.end()) {
      return _parent ? _parent->resolveVariable(varName) : std::make_pair(ID_MAX, ID_MAX);
    }
    return std::make_pair(_id, i->second);
  }
  
  Var * getVariable(VarId varId) {
    return getVariable(varId.first, varId.second);
  }

  Var * getVariable(uint16_t scopeId, uint16_t varId) {
    if (scopeId >= _all_scopes->size()) return NULL;
    BytecodeScope * scope = (*_all_scopes)[scopeId];
    if (scope->_variables.size() >= varId) return NULL;
    return scope->_variables[varId];
  }
  
  uint16_t resolveFunction(std::string const & functionName) {
    BytecodeFunction * function = getFunction(functionName);
    return function ? function->id() : ID_MAX;
  }

  BytecodeFunction * getFunction(std::string const & functionName) {
    FunctionMap::iterator i = _functions.find(functionName);
    if (i == _functions.end()) {
      return _parent ? _parent->getFunction(functionName) : NULL;
    }
    return dynamic_cast<BytecodeFunction *>(_code->functionById(i->second));
  }

  BytecodeScope * createChildScope() {
    uint16_t newId = _all_scopes->size();
    if (newId >= ID_MAX) return NULL;
    BytecodeScope * newScope = new BytecodeScope(this, newId);
    _all_scopes->push_back(newScope);
    return newScope;
  }
  
  ~BytecodeScope() {
    if (!_parent) { 
      for (std::vector<BytecodeScope *>::iterator i = _all_scopes->begin(); i != _all_scopes->end(); ++i) {
        delete *i;
      }
      delete _all_scopes;
    }
    for (std::vector<Var *>::iterator i = _variables.begin(); i != _variables.end(); ++i) {
      delete *i;
    }
  }
  
  static bool isValidVarId(VarId varId) {
    return !(varId.first == ID_MAX || varId.second == ID_MAX);
  }

private:
  BytecodeScope(BytecodeScope* parent, uint16_t id) : _id(id),  _parent(parent), _code(parent->_code), _all_scopes(parent->_all_scopes) {
  }
  
  BytecodeScope(BytecodeScope &);
  
  BytecodeScope & operator=(BytecodeScope const &);
   
private:
  uint16_t _id;
  BytecodeScope* _parent;
  Code* _code;
  std::vector<BytecodeScope *>* _all_scopes;

private:
  FunctionMap _functions;
  VarMap _variables_map;
  std::vector<Var *> _variables;
};


class ScopedBytecodeGenerator : public AstVisitor {
public:
ScopedBytecodeGenerator(BytecodeScope * scope, BytecodeFunction * function) : _scope(scope), _current_function(function) {
}

void visitBinaryOpNode(BinaryOpNode * binaryOpNode) {
  throw std::logic_error("NOT IMPLEMENTED");
}

void visitUnaryOpNode(UnaryOpNode * unaryOpNode) {
  throw std::logic_error("NOT IMPLEMENTED");
}

void visitStringLiteralNode(StringLiteralNode * stringLiteralNode) {
 throw std::logic_error("NOT IMPLEMENTED");
}

void visitDoubleLiteralNode(DoubleLiteralNode * doubleLiteralNode) {
  throw std::logic_error("NOT IMPLEMENTED");
} 

void visitIntLiteralNode(IntLiteralNode * intLiteralNode) {
  throw std::logic_error("NOT IMPLEMENTED");
}

void visitLoadNode(LoadNode * loadNode) {
  throw std::logic_error("NOT IMPLEMENTED");
}

void visitStoreNode(StoreNode * storeNode) {
  throw std::logic_error("NOT IMPLEMENTED");
}

void visitForNode(ForNode * forNode) {
  throw std::logic_error("NOT IMPLEMENTED");
}

void visitWhileNode(WhileNode * whileNode) {
  throw std::logic_error("NOT IMPLEMENTED");
}

void visitIfNode(IfNode * ifNode) {
  throw std::logic_error("NOT IMPLEMENTED");
}

void visitBlockNode(BlockNode * blockNode) {
  for (Scope::VarIterator i(blockNode->scope()); i.hasNext();) {
    AstVar* var = i.next();
    _scope->addVariable(var->name(), var->type());
  }
  for (Scope::FunctionIterator i(blockNode->scope()); i.hasNext();) {
    _scope->addFunction(new BytecodeFunction(i.next()));
  }

  blockNode->visitChildren(this);

  for (Scope::FunctionIterator i(blockNode->scope()); i.hasNext();) {
    visitFunctionNode(i.next()->node());
  }
}

void visitFunctionNode(FunctionNode * functionNode) {
  BytecodeScope * childScope = _scope->createChildScope();
  for (uint32_t i = 0;  i != functionNode->parametersNumber(); ++i) {
    childScope->addVariable(functionNode->parameterName(i), functionNode->parameterType(i));
  }
  ScopedBytecodeGenerator(childScope, _scope->getFunction(functionNode->name())).visitBlockNode(functionNode->body());
  //TODO validate generated code ?
}

void visitReturnNode(ReturnNode * returnNode) {
  if (returnNode->returnExpr()) {
    throw std::logic_error("NOT IMPLEMENTED");
  }
  addInstruction(BC_RETURN);
}

void visitCallNode(CallNode * callNode) {
  throw std::logic_error("NOT IMPLEMENTED");
}

void visitNativeCallNode(NativeCallNode * NativeCallNode) {
  throw std::logic_error("NOT IMPLEMENTED");
}

void visitPrintNode(PrintNode * printNode) {
  throw std::logic_error("NOT IMPLEMENTED");
}

private:
void addInstruction(Instruction instruction) {
  _current_function->bytecode()->addInsn(instruction);
}

private:
  BytecodeScope * _scope;
  BytecodeFunction * _current_function;
};

Status* BytecodeTranslatorImpl::translate(std::string const & program, Code** code) {
  Parser parser;
  std::auto_ptr<Status> status(parser.parseProgram(program));
  
  if (status.get() != NULL && status->isError()) {
    std::cerr << "Parser reports error at " << status->getPosition() << ":" << std::endl;
    std::cerr << status->getError() << std::endl;
    return 0;
  }
  
  Code * translatedCode = new InterpreterCodeImpl;
  BytecodeScope rootScope(translatedCode); 
  rootScope.addFunction(new BytecodeFunction(parser.top()));
  ScopedBytecodeGenerator(&rootScope, NULL).visitFunctionNode(parser.top()->node());
  
  //TODO check if everything's fine before setting code
  *code = translatedCode;
  return new Status;
}

}
