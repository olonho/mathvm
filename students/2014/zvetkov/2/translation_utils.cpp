#include "translation_utils.hpp"
#include "errors.hpp"
#include "info.hpp"

namespace mathvm {

AstVar* findVariable(const std::string& name, Scope* scope, AstNode* at) {
  AstVar* var = scope->lookupVariable(name);

  if (!var) {
    throw TranslationException(at, "Variable '%s' is not defined", name.c_str());
  }

  return var;
}

AstFunction* findFunction(const std::string& name, Scope* scope, AstNode* at) {
  AstFunction* function = scope->lookupFunction(name);

  if (!function) {
    throw TranslationException(at, "Function '%s' is not defined", name.c_str());
  }

  return function;
}

bool isTopLevel(AstFunction* function) {
  return function->name() == AstFunction::top_name;
}

bool isTopLevel(FunctionNode* function) {
  return function->name() == AstFunction::top_name;
}

bool isNumeric(VarType type) {
  return type == VT_INT || type == VT_DOUBLE;
}

bool hasNonEmptyStack(const AstNode* node) {
  return node->isBinaryOpNode() 
         || node->isUnaryOpNode()
         || node->isStringLiteralNode()
         || node->isDoubleLiteralNode()
         || node->isIntLiteralNode()
         || node->isLoadNode()
         || node->isCallNode();
}

void readVarInfo(const AstVar* var, uint16_t& localId, uint16_t& localContext, Context* ctx) {
  VarInfo* info = getInfo<VarInfo>(var);
  uint16_t varFunctionId = info->functionId();
  uint16_t curFunctionId = ctx->currentFunctionId();
  localId = info->localId();
  localContext = 0;

  if (varFunctionId != curFunctionId) {
    InterpreterFunction* varFunction = ctx->functionById(varFunctionId);
    InterpreterFunction* curFunction = ctx->functionById(curFunctionId);

    int32_t varFunctionDeep = varFunction->deepness();
    int32_t curFunctionDeep = curFunction->deepness();
    // var function deepness always >= current function deepness
    localContext = static_cast<uint16_t>(curFunctionDeep - varFunctionDeep);
  } 
}

void loadVar(VarType type, uint16_t localId, uint16_t context, Bytecode* bc) {
  assert(isNumeric(type));
  bool isInt = type == VT_INT;

  if (context != 0) {
    bc->addInsn(isInt ? BC_LOADCTXIVAR : BC_LOADCTXDVAR);
    bc->addUInt16(context);
  } else {
    bc->addInsn(isInt ? BC_LOADIVAR : BC_LOADDVAR);
  }
  
  bc->addUInt16(localId);
}

static void loadVarImpl(const AstVar* var, uint16_t localId, uint16_t context, Bytecode* bc, AstNode* at) {
  VarType type = var->type();

  if (!isNumeric(type)) {
    throw TranslationException(at, "Wrong var reference type (only numbers are supported)");
  }

  loadVar(type, localId, context, bc);
}

void loadVar(LoadNode* node, uint16_t localId, uint16_t context, Bytecode* bc) {
  loadVarImpl(node->var(), localId, context, bc, node);
}

void loadVar(StoreNode* node, uint16_t localId, uint16_t context, Bytecode* bc) {
  loadVarImpl(node->var(), localId, context, bc, node);
}

void storeVar(VarType type, uint16_t localId, uint16_t localContext, TokenKind op, Bytecode* bc) {
  bool isInt = type == VT_INT;

  switch (op) {
    case tINCRSET: bc->addInsn(isInt ? BC_IADD : BC_DADD); break;
    case tDECRSET: bc->addInsn(isInt ? BC_ISUB : BC_DSUB); break;
    default: break;
  }

  if (localContext != 0) {
    bc->addInsn(isInt ? BC_STORECTXIVAR : BC_STORECTXDVAR);
    bc->addUInt16(localContext);
  } else {
    bc->addInsn(isInt ? BC_STOREIVAR : BC_STOREDVAR);
  }

  bc->addUInt16(localId);
}

static void castImpl(VarType from, VarType to, Bytecode* bc, AstNode* node);

void cast(AstNode* expr, VarType to, Bytecode* bc) {
  VarType from = typeOf(expr);
  castImpl(from, to, bc, expr);
}

void castImpl(VarType from, VarType to, Bytecode* bc, AstNode* node) {
  if (from == VT_DOUBLE && to == VT_INT) {
    bc->addInsn(BC_D2I);
  } else if (from == VT_INT && to == VT_DOUBLE) {
    bc->addInsn(BC_I2D);
  } else if (from != to) {
    throw TranslationException(node, "Illegal type: expected -- %s, actual -- %s", 
                               typeToName(to), typeToName(from));
  }
}

} // namespace mathvm