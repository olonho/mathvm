#include "ByteCodeGenerator.h"
#include <iostream>
#include "FirstPassVisitor.h"

using namespace mathvm;
using namespace std;

void ByteCodeGenerator::visitBinaryOpNode( mathvm::BinaryOpNode* node )
{
  VarType expectedType = GetNodeType(node);
  if (node->kind() == tAND || node->kind() == tOR) {
    Label lEnd(myBytecode);
    Label lFirst(myBytecode);
    VisitWithTypeControl(node->left(), expectedType); 
    myBytecode->add(BC_ILOAD0);
    
    if (node->kind() == tAND) myBytecode->addBranch(BC_IFICMPE, lFirst);
    if (node->kind() == tOR) myBytecode->addBranch(BC_IFICMPNE, lFirst);
    
    VisitWithTypeControl(node->right(), expectedType);
    myBytecode->addBranch(BC_JA, lEnd);
    myBytecode->bind(lFirst);
    
    if (node->kind() == tAND) myBytecode->add(BC_ILOAD0);
    if (node->kind() == tOR) myBytecode->add(BC_ILOAD1);
    
    myBytecode->bind(lEnd);
    return;
  } 
  
  VisitWithTypeControl(node->left(), expectedType);
  VisitWithTypeControl(node->right(), expectedType);

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
  VarType operandType = GetNodeType(node->operand());

  switch (node->kind()) {
  case tSUB: 
    BytecodeNeg(operandType); break;
  case tNOT:
    myBytecode->addInsn(BC_ILOAD0);
    DoIFICMP(BC_IFICMPE);
  default:
    break;
  }

  myLastNodeType = operandType;
}

void ByteCodeGenerator::visitStringLiteralNode( mathvm::StringLiteralNode* node )
{
  uint16_t id = myCode.makeStringConstant(node->literal());
  myBytecode->addInsn(BC_SLOAD);
  myBytecode->addInt16(id);
  myLastNodeType = VT_STRING;
}

void ByteCodeGenerator::visitDoubleLiteralNode( mathvm::DoubleLiteralNode* node )
{
  myBytecode->addInsn(BC_DLOAD);
  myBytecode->addDouble(node->literal());
  myLastNodeType = VT_DOUBLE;
}

void ByteCodeGenerator::visitIntLiteralNode( mathvm::IntLiteralNode* node )
{
  myBytecode->addInsn(BC_ILOAD);
  myBytecode->addInt64(node->literal());
  myLastNodeType = VT_INT;
}

void ByteCodeGenerator::visitLoadNode( mathvm::LoadNode* node )
{
  bool isClosure = false;
  VarId id = GetVariableId(node, node->var()->name(), &isClosure);
  LoadVarCommand(node->var()->type(), isClosure, id);
  myLastNodeType = node->var()->type();
}

void ByteCodeGenerator::visitStoreNode( mathvm::StoreNode* node )
{
	node->value()->visit(this);
  bool isClosure = false;
  VarId id = GetVariableId(node, node->var()->name(), &isClosure);
  VarType expectedType = node->var()->type();

  if (node->op() != tASSIGN) {
    if (node->var()->type() == VT_STRING) throw TranslationException("Strings can not mutate");
    LoadVarCommand(expectedType, isClosure, id);
    VisitWithTypeControl(node->value(), expectedType);
    
    if (node->op() == tDECRSET) BytecodeSub(expectedType);
    else if (node->op() == tINCRSET) BytecodeAdd(expectedType);
    else throw TranslationException((std::string)"Unsupported operation: " + tokenOp(node->op()));
  } 
  else {
    VisitWithTypeControl(node->value(), expectedType);
  }
  StoreVarCommand(node->var()->type(), isClosure, id);
}

void ByteCodeGenerator::visitForNode( mathvm::ForNode* node )
{
  Label lCheck(myBytecode);
  Label lEnd(myBytecode);

  BinaryOpNode * range = node->inExpr()->asBinaryOpNode();
  if (range == NULL || range->kind() != tRANGE) throw TranslationException("Range not specified in for statement");
  //if (!myScopeManager.IsVarOnStack(node->var())) throw TranslationException("Undefined variable " + node->var()->name());
  uint16_t varId = GetVariableId(node, node->var()->name()).id;

  // init counter
  range->left()->visit(this);
  myBytecode->addInsn(BC_STOREIVAR);
  myBytecode->addInt16(varId);

  myBytecode->bind(lCheck);
    
  // counter >= right
  myBytecode->addInsn(BC_LOADIVAR);
  myBytecode->addInt16(varId);
  range->right()->visit(this);
  myBytecode->addBranch(BC_IFICMPG, lEnd);

  node->body()->visit(this);

  // increment counter
  myBytecode->addInsn(BC_LOADIVAR);
  myBytecode->addInt16(varId);
  myBytecode->addInsn(BC_ILOAD1);
  myBytecode->addInsn(BC_IADD);
  myBytecode->addInsn(BC_STOREIVAR);
  myBytecode->addInt16(varId);
  
  myBytecode->addBranch(BC_JA, lCheck);


  myBytecode->bind(lEnd);
}

void ByteCodeGenerator::visitWhileNode( mathvm::WhileNode* node )
{
  Label lEnd(myBytecode);
  Label lCheck(myBytecode);  
  
  myBytecode->bind(lCheck);
  node->whileExpr()->visit(this);
  myBytecode->addInsn(BC_ILOAD0);
  myBytecode->addBranch(BC_IFICMPE, lEnd);

  node->loopBlock()->visit(this);
  myBytecode->addBranch(BC_JA, lCheck);

  myBytecode->bind(lEnd);
}

void ByteCodeGenerator::visitIfNode( mathvm::IfNode* node )
{
  Label lEnd(myBytecode);

  node->ifExpr()->visit(this);
  myBytecode->addInsn(BC_ILOAD0);

  if (node->elseBlock()) {
    Label lFalse(myBytecode);
    myBytecode->addBranch(BC_IFICMPE, lFalse);
    node->thenBlock()->visit(this);
    myBytecode->addBranch(BC_JA, lEnd);
    myBytecode->bind(lFalse);
    node->elseBlock()->visit(this);
  } 
  else {
    myBytecode->addBranch(BC_IFICMPE, lEnd);
    node->thenBlock()->visit(this);
  }

  myBytecode->bind(lEnd);
}

void ByteCodeGenerator::visitBlockNode( mathvm::BlockNode* node )
{
  node->visitChildren(this);
  Scope::FunctionIterator it(node->scope());
  while(it.hasNext()) {
    AstFunction* f = it.next();
    f->node()->visit(this);
  }
}

void ByteCodeGenerator::visitFunctionNode( mathvm::FunctionNode* node )
{
  Bytecode * prev = myBytecode;
  myBytecode = new Bytecode;

  NodeInfo const & nodeInfo = GetNodeInfo(node->body());
  BytecodeFunction *bfun = new BytecodeFunction(nodeInfo.scopeInfo->GetAstFunction());
  bfun->setLocalsNumber(nodeInfo.scopeInfo->GetTotalVariablesNum());
  myCode.addFunction(bfun);

  node->body()->visit(this);
  
  if (prev == NULL) myBytecode->addInsn(BC_STOP);
  *bfun->bytecode() = *myBytecode;
  
  delete myBytecode;
  myBytecode = prev;
}

void ByteCodeGenerator::visitPrintNode( mathvm::PrintNode* node )
{
  for (unsigned int i = 0; i < node->operands(); ++i) {
    AstNode* op = node->operandAt(i);
    op->visit(this);
    BytecodePrint(GetNodeType(op));
  }
}

void ByteCodeGenerator::BytecodeAdd( VarType expectedType )
{
  if (expectedType == VT_DOUBLE) myBytecode->addInsn(BC_DADD);
  else if (expectedType == VT_INT) myBytecode->addInsn(BC_IADD);
  else throw TranslationException("Invalid operation");
}

void ByteCodeGenerator::BytecodeSub( mathvm::VarType expectedType )
{
  if (expectedType == VT_DOUBLE) myBytecode->addInsn(BC_DSUB);
  else if (expectedType == VT_INT) myBytecode->addInsn(BC_ISUB);
  else throw TranslationException("Invalid operation");
}

void ByteCodeGenerator::BytecodeMul( mathvm::VarType expectedType )
{
  if (expectedType == VT_DOUBLE) myBytecode->addInsn(BC_DMUL);
  else if (expectedType == VT_INT) myBytecode->addInsn(BC_IMUL);
  else throw TranslationException("Invalid operation");
}

void ByteCodeGenerator::BytecodeDiv( mathvm::VarType expectedType )
{
  if (expectedType == VT_DOUBLE) myBytecode->addInsn(BC_DDIV);
  else if (expectedType == VT_INT) myBytecode->addInsn(BC_IDIV);
  else throw TranslationException("Invalid operation");
}

void ByteCodeGenerator::BytecodeNeg( mathvm::VarType expectedType )
{
  if (expectedType == VT_DOUBLE) myBytecode->addInsn(BC_DNEG);
  else if (expectedType == VT_INT) myBytecode->addInsn(BC_INEG);
  else throw TranslationException("Invalid operation");
}

void ByteCodeGenerator::BytecodePrint( mathvm::VarType expectedType )
{
  switch (expectedType) {
    case VT_INT:
      myBytecode->addInsn(BC_IPRINT);
      break;
    case VT_DOUBLE:
      myBytecode->addInsn(BC_DPRINT);
      break;
    case VT_STRING:
      myBytecode->addInsn(BC_SPRINT);
    default:
      break;
  }
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
  DoIFICMP(ifInstruction);
  return true;
}

bool ByteCodeGenerator::TryDoFloatingLogic( mathvm::BinaryOpNode* node )
{
  myBytecode->addInsn(BC_DCMP);
  switch (node->kind()) {
    case tEQ:
      myBytecode->addInsn(BC_ILOAD0);
      DoIFICMP(BC_IFICMPE);
      return true;
    case tNEQ:
      myBytecode->addInsn(BC_ILOAD0);
      DoIFICMP(BC_IFICMPNE);
      return true;
    case tGT:
      myBytecode->addInsn(BC_ILOAD1);
      DoIFICMP(BC_IFICMPG);
      return true;
    case tGE:
      myBytecode->addInsn(BC_ILOAD1);
      DoIFICMP(BC_IFICMPGE);
      return true;
    case tLT:
      myBytecode->addInsn(BC_ILOADM1);
      DoIFICMP(BC_IFICMPL);
      return true;
    case tLE:
      myBytecode->addInsn(BC_ILOADM1);
      DoIFICMP(BC_IFICMPLE);
      return true;
    default:
      return false;
  }
}

void ByteCodeGenerator::DoIFICMP( mathvm::Instruction operation )
{
  Label lTrue(myBytecode);
  Label lEnd(myBytecode);
  myBytecode->addBranch(operation, lTrue);
  myBytecode->addInsn(BC_ILOAD0);
  myBytecode->addBranch(BC_JA, lEnd);
  myBytecode->bind(lTrue);
  myBytecode->addInsn(BC_ILOAD1);
  myBytecode->bind(lEnd);
}


void ByteCodeGenerator::VisitWithTypeControl( AstNode* node, mathvm::VarType expectedType )
{
  node->visit(this);
  if (myLastNodeType == VT_DOUBLE && expectedType == VT_INT) 
    myBytecode->addInsn(BC_D2I);
  if (myLastNodeType == VT_INT && expectedType == VT_DOUBLE)
    myBytecode->addInsn(BC_I2D);
}

mathvm::Code* ByteCodeGenerator::GetCode()
{
  return &myCode;
}

void ByteCodeGenerator::Compile( mathvm::AstFunction * main )
{
  myFirstPassVisitor.visit(main);

  main->node()->visit(this);
}

void ByteCodeGenerator::visitReturnNode( mathvm::ReturnNode* node )
{
  if (node->returnExpr()) VisitWithTypeControl(node->returnExpr(), GetNodeType(node));
  myBytecode->addInsn(BC_RETURN);
}

void ByteCodeGenerator::visitCallNode( mathvm::CallNode* node )
{
  for (unsigned int i = 0; i < node->parametersNumber(); ++i) {
    AstNode* n = node->parameterAt(i);
    VisitWithTypeControl(n, GetNodeType(n));
  }
  myBytecode->addInsn(BC_CALL);
  uint16_t id = myFirstPassVisitor.GetFunctionId(node->name());
  myBytecode->addInt16(id);
}

void ByteCodeGenerator::LoadVarCommand( mathvm::VarType variableType, bool isClosure, VarId const& id )
{
  Instruction ins;
  switch(variableType) {
  case VT_DOUBLE:
    ins = isClosure ? BC_LOADCTXDVAR : BC_LOADDVAR; break;
  case VT_STRING:
    ins = isClosure ? BC_LOADCTXSVAR : BC_LOADSVAR; break;
  case VT_INT:
    ins = isClosure ? BC_LOADCTXIVAR : BC_LOADIVAR; break;
  default:
    throw TranslationException("Invalid variable type");
  }
  myBytecode->addInsn(ins);
  if (isClosure) myBytecode->addUInt16(id.ownerFunction);
  myBytecode->addUInt16(id.id);
}

void ByteCodeGenerator::StoreVarCommand( mathvm::VarType variableType, bool isClosure, VarId id )
{
  Instruction ins;
  switch(variableType) {
  case VT_DOUBLE:
    ins = isClosure ? BC_STORECTXDVAR : BC_STOREDVAR; break;
  case VT_STRING:
    ins = isClosure ? BC_STORECTXSVAR : BC_STORESVAR; break;
  case VT_INT:
    ins = isClosure ? BC_STORECTXIVAR : BC_STOREIVAR; break;
  default:
    throw TranslationException("Invalid variable type");
  }
  myBytecode->addInsn(ins);
  if (isClosure) myBytecode->addUInt16(id.ownerFunction);
  myBytecode->addUInt16(id.id);
}

ByteCodeGenerator::ByteCodeGenerator() : myBytecode(NULL)
{

}

ByteCodeGenerator::~ByteCodeGenerator()
{
}


VarId ByteCodeGenerator::GetVariableId( mathvm::AstNode* currentNode, std::string const& varName, bool* isClosure_out /*= NULL*/ )
{
  ScopeInfo * info = GetNodeInfo(currentNode).scopeInfo;
  bool isClosure = false;
  uint16_t id = 0;
  while (info) {
    if (info->TryFindVariableId(varName, id)) break;
    if (info->IsFunction()) isClosure = true;
    info = info->GetParent();
  }

  if (info == NULL) throw TranslationException("Undefined variable: " + varName);

  VarId result;
  result.id = id;
  result.ownerFunction = info->GetFunctionId();

  if (isClosure_out) *isClosure_out = isClosure;
  return result;
}


bool ByteCodeGenerator::TryFindVariable( ScopeInfo * info, uint16_t &id, std::string const& varName, bool &isClosure )
{
  if (info->TryFindVariableId(varName, id)) return true;
  Scope::VarIterator it (info->GetScope());
  while (it.hasNext()) {
    AstVar * var = it.next();
    if (var->name().compare(varName) == 0) {
      id += info->GetInitialIndex();
      return true;
    }
    ++id;
  }

  if (info->IsFunction()) {
    AstFunction * ast = info->GetAstFunction();
    for (id = 0; id < ast->parametersNumber(); ++id) {
      if (ast->parameterName(id) == varName) return true;
    }
    isClosure = true;
  }

  return false;
}

