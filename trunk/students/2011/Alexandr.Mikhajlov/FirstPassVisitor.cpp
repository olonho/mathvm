#include "mathvm.h"
#include "ast.h"
#include "GeneratorCommon.h"
#include "FirstPassVisitor.h"

using namespace mathvm;


void FirstPassVisitor::visit( mathvm::AstFunction * main )
{
  myScopeManager.AddTopFunction(main);
  main->node()->visit(this);
}

void FirstPassVisitor::visitUnaryOpNode( mathvm::UnaryOpNode* node )
{
  node->operand()->visit(this);
  VarType operandType = myNodeTypes[node->operand()];
  if (operandType == VT_STRING) throw TranslationException("ERROR: String unary operations not supported");
  if (node->kind() == tNOT && operandType == VT_DOUBLE) throw TranslationException("ERROR: Invalid argument type for NOT command");

  myNodeTypes[node] = operandType;
}

VarType DeduceBinaryOpType(VarType leftType, VarType rightType, mathvm::BinaryOpNode* node) {
  if (leftType == VT_STRING || rightType == VT_STRING) {
    throw TranslationException("ERROR: Binary operations with strings not supported");
  } 
  if (leftType == VT_INVALID || rightType == VT_INVALID)
    throw TranslationException("ERROR: Invalid operation exception");
  if (leftType == rightType) return leftType;
  if (node->kind() == tAND || node->kind() == tOR) return VT_INT;
  return VT_DOUBLE;
}

void FirstPassVisitor::visitBinaryOpNode( mathvm::BinaryOpNode* node )
{
  node->left()->visit(this);
  node->right()->visit(this);

  VarType leftType = myNodeTypes[node->left()];
  VarType rightType = myNodeTypes[node->right()];

  myNodeTypes[node] = DeduceBinaryOpType(leftType, rightType, node);
}


void FirstPassVisitor::visitStringLiteralNode( mathvm::StringLiteralNode* node )
{
  myNodeTypes[node] = VT_STRING;
}

void FirstPassVisitor::visitDoubleLiteralNode( mathvm::DoubleLiteralNode* node )
{
  myNodeTypes[node] = VT_DOUBLE;
}

void FirstPassVisitor::visitIntLiteralNode( mathvm::IntLiteralNode* node )
{
  myNodeTypes[node] = VT_INT;
}

void FirstPassVisitor::visitLoadNode( mathvm::LoadNode* node )
{
  if (!myScopeManager.IsVarOnStack(node->var())) throw TranslationException("Undefined variable: " + node->var()->name());
  myNodeTypes[node] = node->var()->type();
}

void FirstPassVisitor::visitStoreNode( mathvm::StoreNode* node )
{
  if (!myScopeManager.IsVarOnStack(node->var())) throw TranslationException("Undefined variable: " + node->var()->name());
  node->visitChildren(this);  
  myNodeTypes[node] = VT_INVALID;
}

void FirstPassVisitor::visitForNode( mathvm::ForNode* node )
{
  node->inExpr()->visit(this);
  node->body()->visit(this);
  myNodeTypes[node] = VT_INVALID;
}

void FirstPassVisitor::visitWhileNode( mathvm::WhileNode* node )
{
  node->whileExpr()->visit(this);
  node->loopBlock()->visit(this);
  myNodeTypes[node] = VT_INVALID;
}

void FirstPassVisitor::visitIfNode( mathvm::IfNode* node )
{
  node->ifExpr()->visit(this);
  node->thenBlock()->visit(this);
  if (node->elseBlock()) node->elseBlock()->visit(this);
  myNodeTypes[node] = VT_INVALID;
}

void FirstPassVisitor::visitBlockNode( mathvm::BlockNode* node )
{
  myScopeManager.CreateBlockScope(node->scope());
  node->visitChildren(this);
  Scope::FunctionIterator it(node->scope());
  while(it.hasNext()) {
    AstFunction* f = it.next();
    f->node()->visit(this);
  }
  myNodeTypes[node] = VT_INVALID;
  myScopeManager.PopScope();
}

void FirstPassVisitor::visitPrintNode( mathvm::PrintNode* node )
{
  for (unsigned int i = 0; i < node->operands(); ++i) {
    AstNode* op = node->operandAt(i);
    op->visit(this);
  }
  myNodeTypes[node] = VT_INVALID;
}

void FirstPassVisitor::visitFunctionNode( mathvm::FunctionNode* node )
{
  FunctionNode* lastFunction = myCurrentFunction;
  myCurrentFunction = node;

  BlockNode * block = node->body()->asBlockNode();
  assert(block);
  myScopeManager.CreateFunctionScope(node, block->scope());
  
  block->visitChildren(this);

  Scope::FunctionIterator it(block->scope());
  while(it.hasNext()) {
    AstFunction* f = it.next();
    f->node()->visit(this);
  }

  myNodeTypes[node] = myCurrentFunction->returnType(); 
  myCurrentFunction = lastFunction;

  myScopeManager.PopScope();
}

void FirstPassVisitor::visitReturnNode( ReturnNode* node )
{
  VarType returnType = VT_VOID;
  if (node->returnExpr()) {
    node->returnExpr()->visit(this);
    returnType = myNodeTypes[node->returnExpr()];
  } 

  myNodeTypes[node] = myCurrentFunction->returnType();

  if (returnType == VT_INVALID) throw TranslationException("Invalid return type");
  if (returnType != myCurrentFunction->returnType() && (returnType == VT_STRING || myCurrentFunction->returnType() == VT_STRING)) throw TranslationException((std::string)"Invalid return type: function has return type " + typeToName(myCurrentFunction->returnType()) + " while returning " + typeToName(returnType));
}

void FirstPassVisitor::visitCallNode( mathvm::CallNode* node )
{
  if (!myScopeManager.IsFunctionVisible(node->name())) throw TranslationException("Function not found: " + node->name());
  // TODO: validate argument types
  AstFunction const * fun = myScopeManager.GetFunctionDeclaration(node->name());
  if (fun->parametersNumber() != node->parametersNumber()) throw TranslationException("ERROR: function " + node->name() + ": invalid arguments number");
  
  myNodeTypes[node] = fun->returnType();
  node->visitChildren(this);

  for (unsigned int i = 0; i < node->parametersNumber(); ++i) {
    AstNode* n = node->parameterAt(i);
    if (myNodeTypes[n] != fun->parameterType(i)) {
      if (myNodeTypes[n] == VT_STRING || fun->parameterType(i) == VT_STRING) throw TranslationException("Can not convert function parameter");
      myNodeTypes[n] = fun->parameterType(i);
    }
  }
}

