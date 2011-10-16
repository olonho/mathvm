#include "ByteCodeGenerator.h"
#include <iostream>

using namespace mathvm;
using namespace std;

void ByteCodeGenerator::visitBinaryOpNode( mathvm::BinaryOpNode* node )
{
  if (node->kind() == tAND || node->kind() == tOR) {
    Label lEnd(&myBytecode);
    node->left()->visit(this);

    myBytecode.add(BC_ILOAD0);
    if (node->kind() == tAND) myBytecode.addBranch(BC_IFICMPE, lEnd);
    else myBytecode.addBranch(BC_IFICMPNE, lEnd);

    node->right()->visit(this);
    myBytecode.bind(lEnd);
  }
  else {
    node->left()->visit(this);
    node->right()->visit(this);
  }

  VarType leftType = myNodeTypes[node->left()];
  VarType rightType = myNodeTypes[node->right()];

  VarType expectedType = DeduceBinaryOperationType(leftType, rightType);
  myNodeTypes[node] = expectedType;

  if (TryDoArithmetics(node, expectedType)) return;

  if (expectedType == VT_DOUBLE) {
    TryDoFloatingLogic(node);
  } 
  else {
    TryDoIntegerLogic(node);
  }
}

void ByteCodeGenerator::visitUnaryOpNode( mathvm::UnaryOpNode* node )
{
  node->operand()->visit(this);
  VarType operandType = myNodeTypes[node->operand()];
  if (operandType == VT_STRING) throw TranslationException("String unary operations not supported");

  switch (node->kind()) {
  case tSUB: 
    BytecodeNeg(operandType); break;
  case tNOT:
    if (operandType == VT_DOUBLE) throw TranslationException("Invalid argument type for NOT command");
    myBytecode.addInsn(BC_ILOAD0);
    DoIFICMP(BC_IFICMPE);
  default:
    break;
  }

  myNodeTypes[node] = operandType;
}

void ByteCodeGenerator::visitStringLiteralNode( mathvm::StringLiteralNode* node )
{
  uint16_t id = myCode.makeStringConstant(node->literal());
  myBytecode.addInsn(BC_SLOAD);
  myBytecode.addInt16(id);
  myNodeTypes[node] = VT_STRING;
}

void ByteCodeGenerator::visitDoubleLiteralNode( mathvm::DoubleLiteralNode* node )
{
  myBytecode.addInsn(BC_DLOAD);
  myBytecode.addDouble(node->literal());
  myNodeTypes[node] = VT_DOUBLE;
}

void ByteCodeGenerator::visitIntLiteralNode( mathvm::IntLiteralNode* node )
{
  myBytecode.addInsn(BC_ILOAD);
  myBytecode.addInt64(node->literal());
  myNodeTypes[node] = VT_INT;
}

void ByteCodeGenerator::visitLoadNode( mathvm::LoadNode* node )
{
  int id = myVariables.GetId(node->var());
  switch (node->var()->type()) {
    case VT_DOUBLE:
      myBytecode.addInsn(BC_LOADDVAR);
      break;
    case VT_INT:
      myBytecode.addInsn(BC_LOADIVAR);
    default:
      break;
  }
  myBytecode.addInt16(id);
  myNodeTypes[node] = node->var()->type();
}

void ByteCodeGenerator::visitStoreNode( mathvm::StoreNode* node )
{
  node->visitChildren(this);
  if (node->op() == tASSIGN) {
    int id = myVariables.GetId(node->var());
    
    switch (node->var()->type()) {
      case VT_DOUBLE:
        myBytecode.addInsn(BC_STOREDVAR);
        break;
      case VT_INT:
        myBytecode.addInsn(BC_STOREIVAR);
        break;
      case VT_STRING:
        myBytecode.addInsn(BC_STORESVAR);
        break;
      default:
        break;
    }
    myBytecode.addInt16(id);
  } 
  else {
    if (node->var()->type() == VT_STRING) throw TranslationException("Strings can not mutate");
    VarType expectedType = node->var()->type();
    
    LoadVar(node->var());

    node->value()->visit(this);
    
    // make some conversion
    if (node->var()->type() == VT_INT && myNodeTypes[node->value()] == VT_DOUBLE) {
      myBytecode.addInsn(BC_D2I);
      expectedType = VT_INT;
    }
    else if (node->var()->type() == VT_DOUBLE && myNodeTypes[node->value()] == VT_INT) {
      myBytecode.addInsn(BC_I2D);
      expectedType = VT_DOUBLE;
    }

    if (node->op() == tDECRSET) BytecodeSub(expectedType);
    else if (node->op() == tINCRSET) BytecodeAdd(expectedType);

    StoreVar(node->var());
  }
}

void ByteCodeGenerator::visitForNode( mathvm::ForNode* node )
{
  Label lCheck(&myBytecode);
  Label lEnd(&myBytecode);

  BinaryOpNode * range = node->inExpr()->asBinaryOpNode();
  if (range == NULL || range->kind() != tRANGE) throw TranslationException("Range not specified in for statement");
  if (!myVariables.Exists(node->var())) throw TranslationException("Undefined variable " + node->var()->name());
  uint8_t varId = myVariables.GetId(node->var());

  // init counter
  range->left()->visit(this);
  myBytecode.addInsn(BC_STOREIVAR);
  myBytecode.addInt16(varId);

  myBytecode.bind(lCheck);
    
  // counter >= right
  myBytecode.addInsn(BC_LOADIVAR);
  myBytecode.addInt16(varId);
  range->right()->visit(this);
  myBytecode.addBranch(BC_IFICMPG, lEnd);

  node->body()->visit(this);

  // increment counter
  myBytecode.addInsn(BC_LOADIVAR);
  myBytecode.addInt16(varId);
  myBytecode.addInsn(BC_ILOAD1);
  myBytecode.addInsn(BC_IADD);
  myBytecode.addInsn(BC_STOREIVAR);
  myBytecode.addInt16(varId);
  
  myBytecode.addBranch(BC_JA, lCheck);


  myBytecode.bind(lEnd);
}

void ByteCodeGenerator::visitWhileNode( mathvm::WhileNode* node )
{
  Label lEnd(&myBytecode);
  Label lCheck(&myBytecode);  
  
  myBytecode.bind(lCheck);
  node->whileExpr()->visit(this);
  myBytecode.addInsn(BC_ILOAD1);
  myBytecode.addBranch(BC_IFICMPNE, lEnd);

  node->loopBlock()->visit(this);
  myBytecode.addBranch(BC_JA, lCheck);

  myBytecode.bind(lEnd);
}

void ByteCodeGenerator::visitIfNode( mathvm::IfNode* node )
{
  Label lFalse(&myBytecode);
  Label lEnd(&myBytecode);
 
  node->ifExpr()->visit(this);

  myBytecode.addInsn(BC_ILOAD1);
  myBytecode.addBranch(BC_IFICMPNE, lFalse);

  node->thenBlock()->visit(this);
  myBytecode.addBranch(BC_JA, lEnd);

  myBytecode.bind(lFalse);
  if (node->elseBlock()) {
    node->elseBlock()->visit(this);
  }

  myBytecode.bind(lEnd);
}

void ByteCodeGenerator::visitBlockNode( mathvm::BlockNode* node )
{
  Scope::VarIterator it(node->scope());
  
  while(it.hasNext()) {
    AstVar* var = it.next();
    myVariables.AddVariable(var);
  }
  node->visitChildren(this);
}

void ByteCodeGenerator::visitFunctionNode( mathvm::FunctionNode* node )
{
  node->body()->visit(this);
}

void ByteCodeGenerator::visitPrintNode( mathvm::PrintNode* node )
{
  for (unsigned int i = 0; i < node->operands(); ++i) {
    AstNode* op = node->operandAt(i);
    op->visit(this);
    BytecodePrint(myNodeTypes[op]);
  }
}

void ByteCodeGenerator::Dump()
{
  myBytecode.dump(cout);
}

void ByteCodeGenerator::BytecodeAdd( VarType expectedType )
{
  if (expectedType == VT_DOUBLE) myBytecode.addInsn(BC_DADD);
  else if (expectedType == VT_INT) myBytecode.addInsn(BC_IADD);
  else throw TranslationException("Invalid operation");
}

void ByteCodeGenerator::BytecodeSub( mathvm::VarType expectedType )
{
  if (expectedType == VT_DOUBLE) myBytecode.addInsn(BC_DSUB);
  else if (expectedType == VT_INT) myBytecode.addInsn(BC_ISUB);
  else throw TranslationException("Invalid operation");
}

void ByteCodeGenerator::BytecodeMul( mathvm::VarType expectedType )
{
  if (expectedType == VT_DOUBLE) myBytecode.addInsn(BC_DMUL);
  else if (expectedType == VT_INT) myBytecode.addInsn(BC_IMUL);
  else throw TranslationException("Invalid operation");
}

void ByteCodeGenerator::BytecodeDiv( mathvm::VarType expectedType )
{
  if (expectedType == VT_DOUBLE) myBytecode.addInsn(BC_DDIV);
  else if (expectedType == VT_INT) myBytecode.addInsn(BC_IDIV);
  else throw TranslationException("Invalid operation");
}

void ByteCodeGenerator::BytecodeNeg( mathvm::VarType expectedType )
{
  if (expectedType == VT_DOUBLE) myBytecode.addInsn(BC_DNEG);
  else if (expectedType == VT_INT) myBytecode.addInsn(BC_INEG);
  else throw TranslationException("Invalid operation");
}

void ByteCodeGenerator::BytecodePrint( mathvm::VarType expectedType )
{
  switch (expectedType) {
    case VT_INT:
      myBytecode.addInsn(BC_IPRINT);
      break;
    case VT_DOUBLE:
      myBytecode.addInsn(BC_DPRINT);
      break;
    case VT_STRING:
      myBytecode.addInsn(BC_SPRINT);
    default:
      break;
  }
}

mathvm::VarType ByteCodeGenerator::DeduceBinaryOperationType( mathvm::VarType leftType, mathvm::VarType rightType )
{
  VarType result;
  if (leftType == VT_STRING || rightType == VT_STRING) {
    throw TranslationException("Binary operations with strings not supported");
  }
  else if (leftType == rightType) {
    result = leftType;
  }
  else if (leftType == VT_INT) {
    throw TranslationException("Binary operation types mismatch");
  }
  else if (rightType == VT_INT) {
    myBytecode.addInsn(BC_I2D);
    result = VT_DOUBLE;
  }	
  return result;
}

void ByteCodeGenerator::visit( mathvm::AstFunction * main )
{
  main->node()->visit(this);
  BytecodeFunction * bfun = new BytecodeFunction(main);
}

bool ByteCodeGenerator::TryDoArithmetics( mathvm::BinaryOpNode * node, mathvm::VarType expectedType )
{
  switch (node->kind()) {
  case tADD:
    BytecodeAdd(expectedType);
    return true;
  case tSUB:
    BytecodeSub(expectedType);
    return true;
  case tMUL:
    BytecodeMul(expectedType);
    return true;
  case tDIV:
    BytecodeDiv(expectedType);
    return true;
  default:
    return false;
  }
  return false;
}

bool ByteCodeGenerator::TryDoIntegerLogic( mathvm::BinaryOpNode* node )
{
  Instruction ifInstruction = BC_INVALID;
  
  switch (node->kind()) {
    case tEQ:
      ifInstruction = BC_IFICMPE; break;
    case tNEQ:
      ifInstruction = BC_IFICMPNE; break;
    case tGT:
      ifInstruction = BC_IFICMPG; break;
    case tGE:
      ifInstruction = BC_IFICMPGE; break;
    case tLT:
      ifInstruction = BC_IFICMPL; break;
    case tLE:
      ifInstruction = BC_IFICMPLE; break;
    default:
      return false;
  }
  
  if (ifInstruction != BC_INVALID) {
    DoIFICMP(ifInstruction);
  }
  return true;
}

bool ByteCodeGenerator::TryDoFloatingLogic( mathvm::BinaryOpNode* node )
{
  myBytecode.addInsn(BC_DCMP);
  switch (node->kind()) {
    case tEQ:
      myBytecode.addInsn(BC_ILOAD0);
      DoIFICMP(BC_IFICMPE);
      return true;
    case tNEQ:
      myBytecode.addInsn(BC_ILOAD0);
      DoIFICMP(BC_IFICMPNE);
      return true;
    case tGT:
      myBytecode.addInsn(BC_ILOAD1);
      DoIFICMP(BC_IFICMPG);
      return true;
    case tGE:
      myBytecode.addInsn(BC_ILOAD1);
      DoIFICMP(BC_IFICMPGE);
      return true;
    case tLT:
      myBytecode.addInsn(BC_ILOADM1);
      DoIFICMP(BC_IFICMPL);
      return true;
    case tLE:
      myBytecode.addInsn(BC_ILOADM1);
      DoIFICMP(BC_IFICMPLE);
      return true;
    default:
      return false;
  }
}

void ByteCodeGenerator::DoIFICMP( mathvm::Instruction operation )
{
  Label lTrue(&myBytecode);
  Label lEnd(&myBytecode);
  myBytecode.addBranch(operation, lTrue);
  myBytecode.addInsn(BC_ILOAD0);
  myBytecode.addBranch(BC_JA, lEnd);
  myBytecode.bind(lTrue);
  myBytecode.addInsn(BC_ILOAD1);
  myBytecode.bind(lEnd);
}

void ByteCodeGenerator::LoadVar( mathvm::AstVar const * var )
{
  if (!myVariables.Exists(var)) throw TranslationException("Undefined variable " + var->name());
  uint8_t varId = myVariables.GetId(var);
  if (var->type() == VT_STRING) {
    myBytecode.addInsn(BC_LOADSVAR);
  } 
  else if (var->type() == VT_INT) {
    myBytecode.addInsn(BC_LOADIVAR);
  }
  else if (var->type() == VT_DOUBLE) {
    myBytecode.addInsn(BC_LOADDVAR);
  }
  else {
    throw TranslationException("Unable to load variable: unsupported type");
  }
  myBytecode.addInt16(varId);
}

void ByteCodeGenerator::StoreVar( mathvm::AstVar const * var )
{
  if (!myVariables.Exists(var)) throw TranslationException("Undefined variable " + var->name());
  uint8_t varId = myVariables.GetId(var);
  if (var->type() == VT_STRING) {
    myBytecode.addInsn(BC_STORESVAR);
  } 
  else if (var->type() == VT_INT) {
    myBytecode.addInsn(BC_STOREIVAR);
  }
  else if (var->type() == VT_DOUBLE) {
    myBytecode.addInsn(BC_STOREDVAR);
  }
  else {
    throw TranslationException("Unable to store variable: unsupported type");
  }
  myBytecode.addInt16(varId);
}

mathvm::Bytecode* ByteCodeGenerator::GetBytecode()
{
  return &myBytecode;
}

std::vector<std::string> ByteCodeGenerator::GetStringsVector()
{
  vector<string> result;
  for (uint16_t i = 0; i < 256; ++i) {
    string s = myCode.constantById(i);
    result.push_back(s);
  }
  return result;
}

