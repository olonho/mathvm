#include "bytecode_generator.hpp"
#include "utils.hpp"

#include <iostream>
#include <utility>

#include <cstdlib>
#include <cassert>

using namespace mathvm;

Status* BytecodeGenerator::generate() {
  visitAstFunction(top_);

  assert(functionIds_.empty());
  return status().release();
}

void BytecodeGenerator::visitFunctionNode(FunctionNode* node) {
  node->body()->visit(this);
  if (status().isError()) { return; }

  if (node->name() == AstFunction::top_name) {
    bc()->addInsn(BC_STOP);
  } else {
    bc()->addInsn(BC_RETURN);
  }
}

void BytecodeGenerator::visitBlockNode(BlockNode* node) {
  Scope* scope = node->scope();
  uint16_t id = currentFunctionId();
  functionIdByScope_.insert(std::make_pair(scope, id));
  
  visitScope(scope);
  for (uint32_t i = 0; i < node->nodes(); i++) {
    AstNode* statement = node->nodeAt(i);
    statement->visit(this);
    if (status().isError()) { return; }

    if (statement->isCallNode()) {
      CallNode* callNode = dynamic_cast<CallNode*>(statement);
      TranslatedFunction* calledFun = code_->functionByName(callNode->name());
      if (calledFun->returnType() != VT_VOID) {
        bc()->addInsn(BC_POP);
      }

      expr().pop();
    }
    expr().assertEmpty(statement);
    if (status().isError()) { return; }
  }
}

void BytecodeGenerator::visitIfNode(IfNode* node) {
  Label otherwise(bc());
  Label end(bc());

  AstNode* ifExpr = node->ifExpr();
  ifExpr->visit(this);
  if (status().isError()) { return; }

  expr().consume(VT_INT, ifExpr);
  expr().assertEmpty(ifExpr);
  if (status().isError()) { return; }

  bc()->addInsn(BC_ILOAD0);
  bc()->addBranch(BC_IFICMPE, otherwise);

  node->thenBlock()->visit(this);
  if (status().isError()) { return; }
  bc()->addBranch(BC_JA, end);

  bc()->bind(otherwise);
  if (node->elseBlock() != 0) {
    node->elseBlock()->visit(this);
    if (status().isError()) { return; }
  }

  bc()->bind(end);
}

void BytecodeGenerator::visitWhileNode(WhileNode* node) {
  AstNode* whileExpr = node->whileExpr();
  
  Label begin(bc());
  Label end(bc());

  bc()->bind(begin);
  whileExpr->visit(this);
  if (status().isError()) { return; }

  expr().consume(VT_INT, whileExpr);
  expr().assertEmpty(whileExpr);
  if (status().isError()) { return; }

  bc()->addInsn(BC_ILOAD0);
  bc()->addBranch(BC_IFICMPE, end);

  node->loopBlock()->visit(this);
  if (status().isError()) { return; }
  bc()->addBranch(BC_JA, begin);
  bc()->bind(end);
}

void BytecodeGenerator::visitPrintNode(PrintNode* node) {
  for (uint32_t i = 0; i < node->operands(); i++) {
    AstNode* operand = node->operandAt(i);
    operand->visit(this);
    if (status().isError()) { return; }
    
    switch (expr().pop()) {
      case VT_INT:
        bc()->addInsn(BC_IPRINT);
        break;
      case VT_DOUBLE:
        bc()->addInsn(BC_DPRINT);
        break;
      case VT_STRING:
        bc()->addInsn(BC_SPRINT);
        break;
      default:
        status().error("Unsupported print operand type ", operand);
    }

    expr().assertEmpty(operand);
    if (status().isError()) { return; }
  }
}

void BytecodeGenerator::visitCallNode(CallNode* node) {
  TranslatedFunction* calledFun = code_->functionByName(node->name());
  assert(calledFun->parametersNumber() == node->parametersNumber());

  for (uint32_t i = 0; i < node->parametersNumber(); i++) {
    AstNode* arg = node->parameterAt(i);
    arg->visit(this);
    if (status().isError()) { return; }

    expr().consume(calledFun->parameterType(i), arg);
    if (status().isError()) { 
      status().error("Wrong argument type", arg);
      return; 
    }

    expr().assertEmpty(arg);
    if (status().isError()) { return; }
  }

  bc()->addInsn(BC_CALL);
  bc()->addUInt16(calledFun->id());

  VarType returnType = calledFun->returnType();
  expr().load(returnType);
}

void BytecodeGenerator::visitBinaryOpNode(BinaryOpNode* node) {
  switch (node->kind()) {
    case tOR:
    case tAND:
      logicalOp(node);
      break;

    case tAOR:
    case tAAND:
    case tAXOR:
      bitwiseOp(node);
      break;

    case tEQ:
    case tNEQ:
    case tGT:
    case tGE:
    case tLT:
    case tLE:
      comparisonOp(node);
      break;

    case tADD:
    case tSUB:
    case tDIV:
    case tMUL:
    case tMOD:
      arithmeticOp(node);
      break;

    default:
      status().error("Unknown binary operator", node);
  }

}

void BytecodeGenerator::visitUnaryOpNode(UnaryOpNode* node) {
  node->operand()->visit(this);
  if (status().isError()) { return; }

  switch (node->kind()) {
    case tSUB:
      negOp(node->operand());
      break;
    case tNOT:
      notOp(node->operand());
      break;
    default:
      status().error("Unknown unary operator", node);
  }
}

void BytecodeGenerator::visitDoubleLiteralNode(DoubleLiteralNode* node) {
  bc()->addInsn(BC_DLOAD);
  bc()->addDouble(node->literal());
  expr().load(VT_DOUBLE);
}

void BytecodeGenerator::visitIntLiteralNode(IntLiteralNode* node) {
  bc()->addInsn(BC_ILOAD);
  bc()->addInt64(node->literal());
  expr().load(VT_INT);
}

void BytecodeGenerator::visitStringLiteralNode(StringLiteralNode* node) {
  uint16_t id = code_->makeStringConstant(node->literal());
  bc()->addInsn(BC_SLOAD);
  bc()->addUInt16(id);
  expr().load(VT_STRING);
}

void BytecodeGenerator::visitAstFunction(AstFunction* function) {
  uint16_t id = code_->addFunction(new BytecodeFunction(function));
  Scope* scope = function->node()->body()->scope();
  scopeByFunctionId_.insert(std::make_pair(id, scope));
  
  functionIds_.push(id);
  function->node()->visit(this);
  functionIds_.pop();
}

void BytecodeGenerator::visitScope(Scope* scope) {
  Scope::VarIterator var_it(scope);
  while (var_it.hasNext()) {
    AstVar* var = var_it.next();
  }

  Scope::FunctionIterator func_it(scope);
  while (func_it.hasNext()) {
    visitAstFunction(func_it.next()); 
  }
}

void BytecodeGenerator::negOp(AstNode* operand) {
  expr().alternatives();

  if (expr().unOp(VT_INT, VT_INT)) {
    bc()->addInsn(BC_INEG);
  } 

  if (expr().unOp(VT_DOUBLE, VT_DOUBLE)) {
    bc()->addInsn(BC_DNEG);
  }
  
  expr().apply(operand);
}

void BytecodeGenerator::notOp(AstNode* operand) {
  expr().alternatives();
  
  if (expr().unOp(VT_INT, VT_INT)) {
    Label setZero(bc());
    Label end(bc());

    bc()->addInsn(BC_ILOAD0);
    bc()->addBranch(BC_IFICMPNE, setZero);
    bc()->addInsn(BC_ILOAD1);
    bc()->addBranch(BC_JA, end);
    bc()->bind(setZero);
    bc()->addInsn(BC_ILOAD0);
    bc()->bind(end);
  }

  expr().apply(operand);
}

void BytecodeGenerator::logicalOp(BinaryOpNode* node) {
  TokenKind token = node->kind();
  if (token != tAND && token != tOR) {
    status().error("Unknown logical operator", node);
    return;
  }
  
  bool isAnd = (token == tAND);
  Label evaluateRight(bc());
  Label setTrue(bc());
  Label end(bc());

  node->left()->visit(this);
  if (status().isError()) { return; }

  bc()->addInsn(BC_ILOAD0);
  if (isAnd) {
    bc()->addBranch(BC_IFICMPNE, evaluateRight);
    bc()->addInsn(BC_ILOAD0);
    bc()->addBranch(BC_JA, end);
  } else {
    bc()->addBranch(BC_IFICMPNE, setTrue);
  }

  bc()->bind(evaluateRight);
  node->right()->visit(this);
  if (status().isError()) { return; }

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

  expr().alternatives();
  expr().binOp(VT_INT, VT_INT, VT_INT);
  if (!expr().apply(node)) {
    status().error("Logical operators expect integer operands", node);
  }
}

void BytecodeGenerator::bitwiseOp(BinaryOpNode* node) {
  if (!operands(node)) { return; }

  expr().alternatives();
  expr().binOp(VT_INT, VT_INT, VT_INT);
  if (!expr().apply(node)) { return; }

  switch (node->kind()) {
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
      status().error("Unknown bitwise binary operator", node);
  }
}

Instruction cmpInstruction(TokenKind token) {
  switch (token) {
    case tEQ:  return BC_IFICMPE;
    case tNEQ: return BC_IFICMPNE;
    case tGT:  return BC_IFICMPG;
    case tGE:  return BC_IFICMPGE;
    case tLT:  return BC_IFICMPL;
    case tLE:  return BC_IFICMPLE;
    default:   return BC_INVALID;
  }
}

void BytecodeGenerator::comparisonOp(BinaryOpNode* node) {
  if (!operands(node)) { return; }

  swap(); // left -> upper, right -> lower
  expr().alternatives();

  bool isInt = false;
  if (expr().binOp(VT_INT, VT_INT, VT_INT)) {
    isInt = true;
  } else if (expr().binOp(VT_INT, VT_DOUBLE, VT_INT)) {
    bc()->addInsn(BC_I2D);
  } else if (expr().binOp(VT_DOUBLE, VT_INT, VT_INT)) {
    bc()->addInsn(BC_SWAP);
    bc()->addInsn(BC_I2D);
    bc()->addInsn(BC_SWAP);
  } else {
    expr().binOp(VT_DOUBLE, VT_DOUBLE, VT_INT);
  }

  if (!expr().apply(node)) { return; }

  Instruction cmpi = cmpInstruction(node->kind());
  if (cmpi == BC_INVALID) {
    status().error("Unknown comparison operator", node);
    return;
  }

  Label setTrue(bc());
  Label end(bc());

  bc()->addInsn(isInt ? BC_DCMP : BC_ICMP);
  bc()->addInsn(BC_ILOAD0);
  bc()->addInsn(BC_SWAP);
  bc()->addBranch(cmpi, setTrue);
  bc()->addInsn(BC_ILOAD0);
  bc()->addBranch(BC_JA, end);
  bc()->bind(setTrue);
  bc()->addInsn(BC_ILOAD1);
  bc()->bind(end);
}

void BytecodeGenerator::arithmeticOp(BinaryOpNode* node) {
  if (!operands(node)) { return; }

  swap(); // left -> upper, right -> lower
  expr().alternatives();

  bool isInt = false;
  if (expr().binOp(VT_INT, VT_INT, VT_INT)) {
    isInt = true;
  } else if (expr().binOp(VT_INT, VT_DOUBLE, VT_DOUBLE)) {
    bc()->addInsn(BC_I2D);
  } else if (expr().binOp(VT_DOUBLE, VT_INT, VT_DOUBLE)) {
    bc()->addInsn(BC_SWAP);
    bc()->addInsn(BC_I2D);
    bc()->addInsn(BC_SWAP);
  } else {
    expr().binOp(VT_DOUBLE, VT_DOUBLE, VT_DOUBLE);
  }

  if (!expr().apply(node)) { return; }

  switch (node->kind()) {
    case tADD:
      bc()->addInsn(isInt ? BC_IADD : BC_DADD);
      break;
    case tSUB:
      bc()->addInsn(isInt ? BC_ISUB : BC_DSUB);
      break;
    case tMUL:
      bc()->addInsn(isInt ? BC_IMUL : BC_DMUL);
      break;
    case tDIV:
      bc()->addInsn(isInt ? BC_IDIV : BC_DDIV);
      break;
    case tMOD:
      if (isInt) {
        bc()->addInsn(BC_IMOD);
      } else {
        status().error("Double modulo is unsupported", node);
      }
      break;
    default:
      status().error("Unknown arithmetic binary operator", node);
  }
}

bool BytecodeGenerator::operands(BinaryOpNode* node) {
  node->left()->visit(this);
  if (status().isError()) { return false; }

  node->right()->visit(this);
  return !status().isError();
}

void BytecodeGenerator::swap() {
  expr().swap();
  bc()->addInsn(BC_SWAP);
}