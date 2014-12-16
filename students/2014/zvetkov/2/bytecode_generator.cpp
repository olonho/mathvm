#include "bytecode_generator.hpp"
#include "errors.hpp"
#include "info.hpp"
#include "translation_utils.hpp"
#include "utils.hpp"

#include <iostream>
#include <utility>

#include <cstdlib>
#include <cassert>

namespace mathvm {

Status* BytecodeGenerator::generate() {
  ctx()->addFunction(top_);
  visit(top_);
  return Status::Ok();
}

void BytecodeGenerator::visit(AstFunction* function) {
  ctx()->enterFunction(function);
  
  if (!isTopLevel(function)) { 
    parameters(function);
  } 

  visit(function->node());
  ctx()->exitFunction();
}

void BytecodeGenerator::parameters(AstFunction* function) {
  Scope* scope = function->scope();
  ctx()->enterScope(scope);
  visit(scope);
  
  AstNode* node = function->node();
  int64_t parametersNumber = function->parametersNumber();

  for (int64_t i = parametersNumber - 1; i >= 0; --i) {
    const std::string& name = function->parameterName(i);
    AstVar* param = findVariable(name, scope, node);
    VarInfo* info = getInfo<VarInfo>(param);
    
    Instruction insn;
    switch (param->type()) {
      case VT_INT:    insn = BC_STOREIVAR; break;
      case VT_DOUBLE: insn = BC_STOREDVAR; break;
      default:
        throw TranslationException(node, "Function %s has parameter with Illegal type: %s", 
                                   function->name().c_str(), typeToName(param->type()));
    }

    bc()->addInsn(insn);
    bc()->addUInt16(info->localId());
  }

  ctx()->exitScope();
}

void BytecodeGenerator::visit(Scope* scope) {
  Scope::VarIterator varIt(scope);
  while (varIt.hasNext()) {
    AstVar* var = varIt.next();
    ctx()->declare(var);
  }

  // Functions can call other functions defined
  // later in same scope, so add them before visit
  Scope::FunctionIterator addFunIt(scope);
  while (addFunIt.hasNext()) {
    ctx()->addFunction(addFunIt.next()); 
  }

  Scope::FunctionIterator funIt(scope);
  while (funIt.hasNext()) {
    visit(funIt.next()); 
  }
}

void BytecodeGenerator::visit(FunctionNode* function) {
  function->body()->visit(this);
  // If reached from top-level fun, it's OK.
  // Otherwise it means that there's no return in fun,
  // so stop execution anyway.
  bc()->addInsn(BC_STOP); 
}

void BytecodeGenerator::visit(BlockNode* block) {
  Scope* scope = block->scope();
  ctx()->enterScope(scope);
  visit(scope);
  
  for (uint32_t i = 0; i < block->nodes(); ++i) {
    AstNode* statement = block->nodeAt(i);
    statement->visit(this);
    
    if (hasNonEmptyStack(statement)) {
      bc()->addInsn(BC_POP);
    } 
  }

  ctx()->exitScope();
}

void BytecodeGenerator::visit(NativeCallNode* node) { 
  uint16_t id = ctx()->addNativeFunction(node->nativeName(), node->nativeSignature(), 0);
  bc()->addInsn(BC_CALLNATIVE);
  bc()->addUInt16(id);
}

void BytecodeGenerator::storeInt(AstNode* expr, uint16_t localId, uint16_t localContext) {
  expr->visit(this);
  cast(expr, VT_INT, bc());
  storeVar(VT_INT, localId, localContext, tASSIGN, bc());
}

void BytecodeGenerator::visit(ForNode* node) { 
  const AstVar* var = node->var();
  AstNode* inExpr = node->inExpr();
  BinaryOpNode* range;

  if (var->type() != VT_INT) {
    throw TranslationException(node, "Illegal for iteration variable type: %s", 
                               typeToName(var->type()));
  }

  if (!inExpr->isBinaryOpNode() || static_cast<BinaryOpNode*>(inExpr)->kind() != tRANGE) {
    throw TranslationException(node, "For statement expects range");
  } else {
    range = static_cast<BinaryOpNode*>(inExpr);
  }

  uint16_t varId;
  uint16_t varContext;
  uint16_t endId = ctx()->declareTemporary();
  Label begin(bc());
  Label end(bc());

  readVarInfo(var, varId, varContext, ctx());
  storeInt(range->left(), varId, varContext);
  storeInt(range->right(), endId, 0);
  
  bc()->bind(begin);
  loadVar(VT_INT, varId, varContext, bc());
  loadVar(VT_INT, endId, 0, bc());
  bc()->addBranch(BC_IFICMPL, end);
  node->body()->visit(this);

  bc()->addInsn(BC_ILOAD1);
  loadVar(VT_INT, varId, varContext, bc());
  storeVar(VT_INT, varId, varContext, tINCRSET, bc());
  bc()->addBranch(BC_JA, begin);
  bc()->bind(end);
}

void BytecodeGenerator::visit(IfNode* node) { 
  Label otherwise(bc());
  Label end(bc());

  node->ifExpr()->visit(this);
  bc()->addInsn(BC_ILOAD0);
  bc()->addBranch(BC_IFICMPE, otherwise);
  node->thenBlock()->visit(this);
  bc()->addBranch(BC_JA, end);
  
  bc()->bind(otherwise);
  if (node->elseBlock()) {
    node->elseBlock()->visit(this);
  }
  
  bc()->bind(end);
}

void BytecodeGenerator::visit(WhileNode* node) { 
  Label begin(bc());
  Label end(bc());
  bc()->bind(begin);
  node->whileExpr()->visit(this);

  bc()->addInsn(BC_ILOAD0);
  bc()->addBranch(BC_IFICMPE, end);
  node->loopBlock()->visit(this);

  bc()->addBranch(BC_JA, begin);
  bc()->bind(end);
}

void BytecodeGenerator::visit(LoadNode* node) { 
  uint16_t localId;
  uint16_t context;
  readVarInfo(node->var(), localId, context, ctx());
  loadVar(node, localId, context, bc());
  setType(node, node->var()->type());
}

void BytecodeGenerator::visit(StoreNode* node) { 
  const AstVar* var = node->var();
  AstNode* value = node->value();
  TokenKind op = node->op();

  value->visit(this); 

  uint16_t localId;
  uint16_t localContext;
  readVarInfo(var, localId, localContext, ctx());
  cast(value, var->type(), bc());

  if (op == tINCRSET || op == tDECRSET) {
    loadVar(node, localId, localContext, bc());
  }

  storeVar(var->type(), localId, localContext, op, bc());
}

void BytecodeGenerator::visit(PrintNode* node) { 
  for (uint32_t i = 0; i < node->operands(); ++i) {
    AstNode* operand = node->operandAt(i);
    operand->visit(this);
    
    Instruction insn;
    switch (typeOf(operand)) {
      case VT_INT:    insn = BC_IPRINT; break;
      case VT_DOUBLE: insn = BC_DPRINT; break;
      case VT_STRING: insn = BC_SPRINT; break;
      default:
        throw TranslationException(node, "Print is only applicable to int, double, string");
    }
    bc()->addInsn(insn);
  }
}

void BytecodeGenerator::visit(ReturnNode* node) { 
  AstNode* returnExpr = node->returnExpr();
  
  if (returnExpr) {
    returnExpr->visit(this);
    cast(returnExpr, ctx()->currentFunction()->returnType(), bc());
  } else {
    bc()->addInsn(BC_ILOAD0);
  }

  bc()->addInsn(BC_RETURN); 
}

void BytecodeGenerator::visit(CallNode* node) { 
  AstFunction* function = findFunction(node->name(), ctx()->currentScope(), node);
  
  if (node->parametersNumber() != function->parametersNumber()) {
    throw TranslationException(node, "Invocation has wrong argument number");
  }
  
  for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
    AstNode* argument = node->parameterAt(i);
    argument->visit(this);
    cast(argument, function->parameterType(i), bc());
  }  

  uint16_t calledFunctionId = ctx()->getId(function);
  int32_t currentContext = ctx()->currentFunction()->deepness();
  int32_t targetContext = ctx()->functionById(calledFunctionId)->deepness();
  bc()->addInsn(BC_ILOAD);
  bc()->addInt64(currentContext - targetContext);

  bc()->addInsn(BC_CALL);
  bc()->addUInt16(calledFunctionId);
  setType(node, function->returnType());
}

void BytecodeGenerator::visit(BinaryOpNode* op) { 
  switch (op->kind()) {
    case tOR:
    case tAND:
      logicalOp(op);
      break;

    case tAOR:
    case tAAND:
    case tAXOR:
      bitwiseOp(op);
      break;

    case tEQ:
    case tNEQ:
    case tGT:
    case tGE:
    case tLT:
    case tLE:
      comparisonOp(op);
      break;

    case tADD:
    case tSUB:
    case tDIV:
    case tMUL:
    case tMOD:
      arithmeticOp(op);
      break;

    default:
      throw TranslationException(op, "Unknown binary operator");
  }
}

void BytecodeGenerator::visit(UnaryOpNode* op) { 
  op->visitChildren(this);
  
  switch (op->kind()) {
    case tSUB: negOp(op); break;
    case tNOT: notOp(op); break;
    default:
      throw TranslationException(op, "Unknown unary operator");
  }
}

void BytecodeGenerator::visit(DoubleLiteralNode* floating) {
  bc()->addInsn(BC_DLOAD);
  bc()->addDouble(floating->literal());
  setType(floating, VT_DOUBLE);
}

void BytecodeGenerator::visit(IntLiteralNode* integer) {
  bc()->addInsn(BC_ILOAD);
  bc()->addInt64(integer->literal());
  setType(integer, VT_INT);
}

void BytecodeGenerator::visit(StringLiteralNode* string) {
  uint16_t id = ctx()->makeStringConstant(string->literal());
  bc()->addInsn(BC_SLOAD);
  bc()->addUInt16(id);
  setType(string, VT_STRING);
}

void BytecodeGenerator::negOp(UnaryOpNode* op) {
  switch (typeOf(op->operand())) {
    case VT_INT: 
      bc()->addInsn(BC_INEG);
      setType(op, VT_INT);
      break;
    case VT_DOUBLE: 
      bc()->addInsn(BC_DNEG);
      setType(op, VT_DOUBLE);
      break;
    default:
      throw TranslationException(op, "Unary sub (-) is only applicable to int/double");
  }
}

void BytecodeGenerator::notOp(UnaryOpNode* op) {
  if (typeOf(op->operand()) != VT_INT) {
    throw TranslationException(op, "Unary not (!) is only applicable to int");
  }

  Label setFalse(bc());
  Label end(bc());

  bc()->addInsn(BC_ILOAD0);
  bc()->addBranch(BC_IFICMPNE, setFalse);
  bc()->addInsn(BC_ILOAD1);
  bc()->addBranch(BC_JA, end);
  bc()->bind(setFalse);
  bc()->addInsn(BC_ILOAD0);
  bc()->bind(end);
  setType(op, VT_INT);
}

void BytecodeGenerator::logicalOp(BinaryOpNode* op) {
TokenKind token = op->kind();
  if (token != tAND && token != tOR) {
    throw TranslationException(op, "Unknown logical operator");
  }
  
  bool isAnd = (token == tAND);
  Label evaluateRight(bc());
  Label setTrue(bc());
  Label end(bc());

  op->left()->visit(this);
  
  bc()->addInsn(BC_ILOAD0);
  if (isAnd) {
    bc()->addBranch(BC_IFICMPNE, evaluateRight);
    bc()->addInsn(BC_ILOAD0);
    bc()->addBranch(BC_JA, end);
  } else {
    bc()->addBranch(BC_IFICMPNE, setTrue);
  }

  bc()->bind(evaluateRight);
  op->right()->visit(this);
    
  if (typeOf(op->left()) != VT_INT || typeOf(op->right()) != VT_INT) {
    throw TranslationException(op, "Logical operators are only applicable integer operands");
  }

  // only reachable in cases:
  // 1. true AND right
  // 2. false OR right
  // so if right is true, whole is true
  bc()->addInsn(BC_ILOAD0);
  bc()->addBranch(BC_IFICMPNE, setTrue);
  bc()->addInsn(BC_ILOAD0);
  bc()->addBranch(BC_JA, end);

  bc()->bind(setTrue);
  bc()->addInsn(BC_ILOAD1);
  bc()->bind(end);

  setType(op, VT_INT);
}

void BytecodeGenerator::bitwiseOp(BinaryOpNode* op) {
  op->visitChildren(this);
  
  if (typeOf(op->left()) != VT_INT || typeOf(op->right()) != VT_INT) {
    throw TranslationException(op, "Bitwise operator is only applicable to int operands");
  }

  switch (op->kind()) {
    case tAOR:
      bc()->addInsn(BC_IAOR);
      break;
    case tAAND:
      bc()->addInsn(BC_IAAND);
      break;
    case tAXOR:
      bc()->addInsn(BC_IAXOR);
      break;
    default:
      throw TranslationException(op, "Unknown bitwise binary operator");
  } 

  setType(op, VT_INT);
}

void BytecodeGenerator::comparisonOp(BinaryOpNode* op) {
  op->visitChildren(this);
  
  VarType operandsCommonType = castOperandsNumeric(op);
  
  Instruction cmpi;
  switch (op->kind()) {
    case tEQ:  cmpi = BC_IFICMPE;  break;
    case tNEQ: cmpi = BC_IFICMPNE; break;
    case tGT:  cmpi = BC_IFICMPG;  break;
    case tGE:  cmpi = BC_IFICMPGE; break;
    case tLT:  cmpi = BC_IFICMPL;  break;
    case tLE:  cmpi = BC_IFICMPLE; break;
    default:
      throw TranslationException(op, "Unknown comparison operator");
  }

  Label setTrue(bc());
  Label end(bc());

  bc()->addInsn(operandsCommonType == VT_INT ? BC_ICMP : BC_DCMP);
  bc()->addInsn(BC_ILOAD0);
  bc()->addBranch(cmpi, setTrue);
  bc()->addInsn(BC_ILOAD0);
  bc()->addBranch(BC_JA, end);
  bc()->bind(setTrue);
  bc()->addInsn(BC_ILOAD1);
  bc()->bind(end);

  setType(op, VT_INT);
}

void BytecodeGenerator::arithmeticOp(BinaryOpNode* op) {
  op->visitChildren(this);
  
  VarType operandsCommonType = castOperandsNumeric(op);
  
  bool isInt = operandsCommonType == VT_INT;
  Instruction insn = BC_INVALID;

  if (op->kind() == tSUB || op->kind() == tMOD || op->kind() == tDIV) {
    bc()->addInsn(BC_SWAP);
  }

  switch (op->kind()) {
    case tADD: insn = isInt ? BC_IADD : BC_DADD; break;
    case tSUB: insn = isInt ? BC_ISUB : BC_DSUB; break;
    case tMUL: insn = isInt ? BC_IMUL : BC_DMUL; break;
    case tDIV: insn = isInt ? BC_IDIV : BC_DDIV; break;
    case tMOD:
      if (isInt) {
        insn = BC_IMOD;
      } else {
        throw TranslationException(op, "Modulo (%) is only applicable to integers");
      }
      break;
    default:
      throw TranslationException(op, "Unknown arithmetic binary operator");
  }

  bc()->addInsn(insn);
  setType(op, operandsCommonType);
}

VarType BytecodeGenerator::castOperandsNumeric(BinaryOpNode* op) {
  VarType tLower = typeOf(op->left());
  VarType tUpper = typeOf(op->right());
  
  if (!isNumeric(tLower) || !isNumeric(tUpper)) {
    throw TranslationException(op, "Operator is only applicable to numbers");
  }

  bool isInt = tLower == VT_INT && tUpper == VT_INT;

  if (!isInt && tLower == VT_INT) {
    bc()->addInsn(BC_SWAP);
    bc()->addInsn(BC_I2D);
    bc()->addInsn(BC_SWAP);
  }

  if (!isInt && tUpper == VT_INT) {
    bc()->addInsn(BC_I2D);
  }

  return isInt ? VT_INT : VT_DOUBLE;
}

} // namespace mathvm