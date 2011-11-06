#include "VarsSearcherVisitor.h"
#include <algorithm>

using namespace std;
using namespace mathvm;

VarsSearcherVisitor::VarsSearcherVisitor(void) : _localVarsCount(0)
{
}


VarsSearcherVisitor::~VarsSearcherVisitor(void)
{
}

void VarsSearcherVisitor::visitBinaryOpNode( mathvm::BinaryOpNode* node )
{
	node->visitChildren(this);
}

void VarsSearcherVisitor::visitUnaryOpNode( mathvm::UnaryOpNode* node )
{
	node->visitChildren(this);
}

void VarsSearcherVisitor::visitStringLiteralNode( mathvm::StringLiteralNode* node )
{
	node->visitChildren(this);
}

void VarsSearcherVisitor::visitDoubleLiteralNode( mathvm::DoubleLiteralNode* node )
{
	node->visitChildren(this);
}

void VarsSearcherVisitor::visitIntLiteralNode( mathvm::IntLiteralNode* node )
{
	node->visitChildren(this);
}

void VarsSearcherVisitor::visitLoadNode( mathvm::LoadNode* node )
{
	addVar(node->var());
}

void VarsSearcherVisitor::visitStoreNode( mathvm::StoreNode* node )
{
	addVar(node->var());
}

void VarsSearcherVisitor::visitForNode( mathvm::ForNode* node )
{
	addVar(node->var());
	node->visitChildren(this);
}

void VarsSearcherVisitor::visitWhileNode( mathvm::WhileNode* node )
{
	node->visitChildren(this);
}

void VarsSearcherVisitor::visitIfNode( mathvm::IfNode* node )
{
	node->visitChildren(this);
}

void VarsSearcherVisitor::visitBlockNode( mathvm::BlockNode* node )
{
	Scope::VarIterator varIt1(node->scope());
	while (varIt1.hasNext()) {
		AstVar *var = varIt1.next();
		_vars.push_back(var);
		_localVarsCount++;
	}
	uint16_t currlocalsCount = _localVarsCount;
	Scope::FunctionIterator fnIt1(node->scope());
	while (fnIt1.hasNext()) {
		AstFunction *func = fnIt1.next();
		func->node()->visit(this);
	}
	_localVarsCount = currlocalsCount;
	node->visitChildren(this);
	Scope::VarIterator varIt2(node->scope());
	while (varIt2.hasNext()) {		
		varIt2.next();
		_vars.pop_back();
	}
}

void VarsSearcherVisitor::visitPrintNode( mathvm::PrintNode* node )
{
	node->visitChildren(this);
}

void VarsSearcherVisitor::visitFunctionNode( mathvm::FunctionNode* node )
{
	for (int i = 0; i < (int)node->parametersNumber(); ++i) {
		_vars.push_back(node->body()->scope()->parent()->lookupVariable(node->parameterName(i)));
	}
	node->visitChildren(this);
	for (int i = 0; i < (int)node->parametersNumber(); ++i) {
		_vars.pop_back();
	}
}

void VarsSearcherVisitor::visitCallNode( mathvm::CallNode* node )
{
	node->visitChildren(this);
}

void VarsSearcherVisitor::visitReturnNode( mathvm::ReturnNode* node )
{
	node->visitChildren(this);
}

void VarsSearcherVisitor::addVar( const mathvm::AstVar *var )
{
	if (find(_vars.begin(), _vars.end(), var) == _vars.end()) {
		_freeVars.insert(var);
	}
}
