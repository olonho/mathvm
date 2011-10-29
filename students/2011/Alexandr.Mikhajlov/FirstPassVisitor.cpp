#include "mathvm.h"
#include "ast.h"
#include "GeneratorCommon.h"
#include "FirstPassVisitor.h"

using namespace mathvm;


void FirstPassVisitor::visit( mathvm::AstFunction * main )
{
  FunctionID fid(main, 0);
  myFunctionDeclarations[main->name()] = fid;
  main->node()->visit(this);
}

void FirstPassVisitor::visitUnaryOpNode( mathvm::UnaryOpNode* node )
{
  node->operand()->visit(this);
  VarType operandType = GetNodeType(node->operand());
  if (operandType == VT_STRING) throw TranslationException("String unary operations not supported");
  if (node->kind() == tNOT && operandType == VT_DOUBLE) throw TranslationException(node, "Invalid argument type for NOT command");

  SetNodeType(node, operandType);
}

VarType DeduceBinaryOpType(VarType leftType, VarType rightType, mathvm::BinaryOpNode* node) {
  if (leftType == VT_STRING || rightType == VT_STRING) {
    throw TranslationException(node, "Binary operations with strings not supported");
  } 
  if (leftType == VT_INVALID || rightType == VT_INVALID)
    throw TranslationException(node, "Invalid operation exception");
  if (leftType == rightType) return leftType;
  if (node->kind() == tAND || node->kind() == tOR) return VT_INT;
  return VT_DOUBLE;
}

void FirstPassVisitor::visitBinaryOpNode( mathvm::BinaryOpNode* node )
{
  if (node->kind() == tRANGE) throw TranslationException(node, "'range' statement can be placed only inside 'for'");
  node->left()->visit(this);
  node->right()->visit(this);

  VarType leftType = GetNodeType(node->left());
  VarType rightType = GetNodeType(node->right());

  if (leftType == VT_VOID || rightType == VT_VOID)
    throw TranslationException(node, "'%s' : illegal operand of type 'void'", tokenOp(node->kind()));


  SetNodeType(node, DeduceBinaryOpType(leftType, rightType, node));
}


void FirstPassVisitor::visitStringLiteralNode( mathvm::StringLiteralNode* node )
{
  SetNodeType(node, VT_STRING);
}

void FirstPassVisitor::visitDoubleLiteralNode( mathvm::DoubleLiteralNode* node )
{
  SetNodeType(node, VT_DOUBLE);
}

void FirstPassVisitor::visitIntLiteralNode( mathvm::IntLiteralNode* node )
{
  SetNodeType(node, VT_INT);
}

void FirstPassVisitor::visitLoadNode( mathvm::LoadNode* node )
{
  SetNodeType(node, node->var()->type());
}

void FirstPassVisitor::visitStoreNode( mathvm::StoreNode* node )
{
  node->visitChildren(this);  
  CheckConversion(tokenOp(node->op()), GetNodeType(node->value()), node->var()->type(), node);

  SetNodeType(node, VT_INVALID);
}



void FirstPassVisitor::visitForNode( mathvm::ForNode* node )
{
  BinaryOpNode * range = node->inExpr()->asBinaryOpNode();
  if (range->kind() != tRANGE) throw TranslationException(node, "'for' : 'range' statement contains unsupported operation");
  range->left()->visit(this);
  range->right()->visit(this);

  node->body()->visit(this);

  VarType v = node->var()->type();
  if (v == VT_STRING || v == VT_DOUBLE ) 
    throw TranslationException(node, "'for' : iterating variable can not be of type '%s'", typeToName(v));
  SetNodeType(node, VT_INVALID);  
}

void FirstPassVisitor::visitWhileNode( mathvm::WhileNode* node )
{
  node->whileExpr()->visit(this);
  node->loopBlock()->visit(this);
  SetNodeType(node, VT_INVALID);
}

void FirstPassVisitor::visitIfNode( mathvm::IfNode* node )
{
  node->ifExpr()->visit(this);
  node->thenBlock()->visit(this);
  if (node->elseBlock()) node->elseBlock()->visit(this);
  SetNodeType(node, VT_INVALID);  
}

void FirstPassVisitor::visitBlockNode( mathvm::BlockNode* node )
{
  ScopeInfo * lastScopeInfo = myCurrentScopeInfo;
  myCurrentScopeInfo = new ScopeInfo(node->scope(), lastScopeInfo);
  myScopeInfos.push_back(myCurrentScopeInfo); // GC

  DeclareFunctions(node->scope());
  node->visitChildren(this);
  VisitFunctions(node->scope());
  SetNodeType(node, VT_INVALID);

  myCurrentScopeInfo->UpdateTotalVars();
  myCurrentScopeInfo = lastScopeInfo;
}

void FirstPassVisitor::visitPrintNode( mathvm::PrintNode* node )
{
  for (unsigned int i = 0; i < node->operands(); ++i) {
    AstNode* op = node->operandAt(i);
    op->visit(this);
  }
  SetNodeType(node, VT_INVALID);
}

void FirstPassVisitor::visitFunctionNode( mathvm::FunctionNode* node )
{
  BlockNode * block = node->body();
  
  FunctionID fid = myFunctionDeclarations[node->name()];
  ScopeInfo* lastScopeInfo = myCurrentScopeInfo;

  myCurrentScopeInfo = new ScopeInfo(block->scope(), lastScopeInfo, fid);
  myScopeInfos.push_back(myCurrentScopeInfo); // GCs
  
  DeclareFunctions(block->scope());

  block->visitChildren(this);

  VisitFunctions(block->scope());

  SetNodeType(block, node->returnType());

  myCurrentScopeInfo = lastScopeInfo;
}

void FirstPassVisitor::visitReturnNode( ReturnNode* node )
{
  VarType returnType = VT_VOID;
  VarType currentReturnType = myCurrentScopeInfo->GetFunctionReturnType();
  if (node->returnExpr()) {
    node->returnExpr()->visit(this);
    returnType = GetNodeType(node->returnExpr());
  } 

  SetNodeType(node, currentReturnType);

  if (currentReturnType != returnType) {
    if (currentReturnType == VT_VOID) throw TranslationException(node, "'%s' : 'void' function returning a value", myCurrentScopeInfo->GetAstFunction()->name().c_str());
    if (returnType == VT_VOID) throw TranslationException(node, "'%s' : function must return a value", myCurrentScopeInfo->GetAstFunction()->name().c_str());
    CheckConversion("return", returnType, currentReturnType, node);
  }
}

void FirstPassVisitor::visitCallNode( mathvm::CallNode* node )
{
  FunctionDeclarationsMap::iterator it = myFunctionDeclarations.find(node->name());
  if (it == myFunctionDeclarations.end()) throw TranslationException("Function not found");
  AstFunction * fun = it->second.function;
  if (fun->parametersNumber() != node->parametersNumber()) throw TranslationException("ERROR: function " + node->name() + ": invalid arguments number");
  
  SetNodeType(node, fun->returnType());
  node->visitChildren(this);

  for (unsigned int i = 0; i < node->parametersNumber(); ++i) {
    AstNode* n = node->parameterAt(i);
    VarType v1 = GetNodeType(n);
    VarType v2 = fun->parameterType(i);
    if (v1 != v2) {
      if (v1 == VT_STRING || v2 == VT_STRING || v1 == VT_VOID || v2 == VT_VOID) throw TranslationException(node, "'foo' : cannot convert parameter %d from '%s' to '%s'", i, typeToName(v1), typeToName(v2));
      SetNodeType(n, fun->parameterType(i));
    }
  }
}





void FirstPassVisitor::SetNodeType( mathvm::AstNode* node, mathvm::VarType value )
{
  NodeInfo info(value, myCurrentScopeInfo);
  myNodeInfos.push_back(info);
  node->setInfo(&myNodeInfos.back());
}

mathvm::VarType FirstPassVisitor::GetNodeType( mathvm::AstNode* node )
{
  NodeInfo const & info = GetNodeInfo(node);
  return info.nodeType;
}

NodeInfo const & FirstPassVisitor::GetNodeInfo( mathvm::AstNode* node )
{
  void* p = node->info();
  if (p == NULL) throw TranslationException("Node info is missing");
  NodeInfo * info = (NodeInfo*)p;
  return *info;
}

void FirstPassVisitor::VisitFunctions( mathvm::Scope * scope )
{
  Scope::FunctionIterator it(scope);
  while(it.hasNext()) {
    AstFunction* f = it.next();
    f->node()->visit(this);
  }
}

void FirstPassVisitor::DeclareFunctions( mathvm::Scope * scope )
{
  Scope::FunctionIterator it(scope);
  while(it.hasNext()) {
    AstFunction* f = it.next();
    FunctionID fid(f, myFunctionDeclarations.size());
    myFunctionDeclarations[f->name()] = fid;
  }
}

FirstPassVisitor::FirstPassVisitor() : myCurrentScopeInfo(NULL)
{

}

FirstPassVisitor::~FirstPassVisitor()
{
  for (unsigned int i = 0; i < myScopeInfos.size(); ++i) {
    delete myScopeInfos[i];
  }
}

uint16_t FirstPassVisitor::GetFunctionId( std::string const & functionName )
{
  FunctionDeclarationsMap::iterator it = myFunctionDeclarations.find(functionName);
  if (it == myFunctionDeclarations.end()) throw TranslationException("Undeclared function: " + functionName);
  return it->second.id;
}

void FirstPassVisitor::CheckConversion( std::string const & where, mathvm::VarType fistType, mathvm::VarType secondType, mathvm::AstNode* node )
{
  if (fistType == VT_INVALID || secondType == VT_INVALID) {
    throw TranslationException(node, "Internal translator error");
  }

  if (secondType != fistType && (fistType == VT_STRING || secondType == VT_STRING || fistType == VT_VOID || secondType == VT_VOID)) {
    throw TranslationException(node, "'%s' : cannot convert from '%s' to '%s'", where.c_str(), typeToName(fistType), typeToName(secondType));
  }
}


