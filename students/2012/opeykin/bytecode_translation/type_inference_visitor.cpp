/*
 * type_onference_visitor.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: alex
 */

#include "type_inference_visitor.h"

namespace mathvm {

TypeInferenceVisitor::TypeInferenceVisitor(TypeMap& typeMap)
		: _types(typeMap) {
}

TypeInferenceVisitor::~TypeInferenceVisitor() {
}

void TypeInferenceVisitor::processFunction(AstFunction* function) {
	Scope* scope = function->scope();
	Scope::VarIterator vit(scope);
	while(vit.hasNext()) {
		AstVar* var = vit.next();
		_types[var] = var->type();
	}
	function->node()->visit(this);
}

VarType TypeInferenceVisitor::maxType(VarType type1, VarType type2) {
	if (type1 == VT_INT 	&& type2 == VT_INT) 	return VT_INT;

	if (type1 == VT_DOUBLE 	&& type2 == VT_DOUBLE) 	return VT_DOUBLE;
	if (type1 == VT_DOUBLE 	&& type2 == VT_INT) 	return VT_DOUBLE;
	if (type1 == VT_INT 	&& type2 == VT_DOUBLE) 	return VT_DOUBLE;

	if (type1 == VT_STRING 	&& type2 == VT_STRING) 	return VT_STRING;
	if (type1 == VT_DOUBLE 	&& type2 == VT_STRING) 	return VT_DOUBLE;
	if (type1 == VT_STRING 	&& type2 == VT_DOUBLE) 	return VT_DOUBLE;
	if (type1 == VT_INT 	&& type2 == VT_STRING) 	return VT_INT;
	if (type1 == VT_STRING 	&& type2 == VT_INT) 	return VT_INT;

	return VT_INVALID;
}


void TypeInferenceVisitor::visitBinaryOpNode(BinaryOpNode* node) {
	AstNode* left = node->left();
	AstNode* right = node->right();
	left->visit(this);
	right->visit(this);
	_types[node] = maxType(_types[left], _types[right]);
}

void TypeInferenceVisitor::visitStringLiteralNode(StringLiteralNode* node) {
	_types[node] =VT_STRING;
}

void TypeInferenceVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
	_types[node] = VT_DOUBLE;
}

void TypeInferenceVisitor::visitIntLiteralNode(IntLiteralNode* node) {
	_types[node] = VT_INT;
}

void TypeInferenceVisitor::visitUnaryOpNode(UnaryOpNode* node) {
	node->visitChildren(this);
	_types[node] = _types[node->operand()];
}

void TypeInferenceVisitor::visitCallNode(CallNode* node) {
	node->visitChildren(this);
	AstFunction* function = _scopes.top()->lookupFunction(node->name());
	_types[node] = function->returnType();
}

void TypeInferenceVisitor::visitBlockNode(BlockNode* node) {
	_scopes.push(node->scope());
	Scope::FunctionIterator fit(node->scope());
	while(fit.hasNext()) {
		processFunction(fit.next());
	}
	node->visitChildren(this);
	_scopes.pop();
}

void TypeInferenceVisitor::visitLoadNode(LoadNode* node) {
	_types[node] = node->var()->type();
}


void TypeInferenceVisitor::visitStoreNode(StoreNode* node) {
	node->visitChildren(this);
}

void TypeInferenceVisitor::visitForNode(ForNode* node) {
	node->visitChildren(this);
}

void TypeInferenceVisitor::visitWhileNode(WhileNode* node) {
	node->visitChildren(this);
}

void TypeInferenceVisitor::visitIfNode(IfNode* node) {
	node->visitChildren(this);
}

void TypeInferenceVisitor::visitFunctionNode(FunctionNode* node) {
	node->visitChildren(this);
}

void TypeInferenceVisitor::visitReturnNode(ReturnNode* node) {
	node->visitChildren(this);
}

void TypeInferenceVisitor::visitNativeCallNode(NativeCallNode* node) { }
void TypeInferenceVisitor::visitPrintNode(PrintNode* node) {
	node->visitChildren(this);
}

} /* namespace mathvm */
