/*
 * CodeBuilderVisitor.cpp
 *
 *  Created on: Oct 24, 2012
 *      Author: alex
 */

#include "CodeBuilderVisitor.h"
#include "mathvm.h"

namespace mathvm {

CodeBuilderVisitor::CodeBuilderVisitor(Code* code)
		: _code(code) {
}

CodeBuilderVisitor::~CodeBuilderVisitor() {
}

void CodeBuilderVisitor::processFunction(AstFunction* ast_function) {
	BytecodeFunction* bytecode_function = new BytecodeFunction(ast_function);
	_code->addFunction(bytecode_function);
	_bytecodes.push(bytecode_function->bytecode());
	ast_function->node()->visit(this);
	_bytecodes.pop();
}

void CodeBuilderVisitor::visitBinaryOpNode(BinaryOpNode* node) {
	node->left()->visit(this);
	node->right()->visit(this);
	Bytecode* bytecode = _bytecodes.top();
	//TODO: assumed that integers on stack
	switch (node->kind()) {
	case tADD:
		bytecode->addInsn(BC_IADD);
		break;
	case tSUB:
		bytecode->addInsn(BC_ISUB);
		break;
	case tMUL:
		bytecode->addInsn(BC_IMUL);
		break;
	case tDIV:
		bytecode->addInsn(BC_IDIV);
		break;
	default:
		assert(false);
		break;
	}
}

void CodeBuilderVisitor::visitUnaryOpNode(UnaryOpNode* node) {
}

void CodeBuilderVisitor::visitStringLiteralNode(StringLiteralNode* node) {
}

void CodeBuilderVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
}

void CodeBuilderVisitor::visitIntLiteralNode(IntLiteralNode* node) {
	Bytecode* bytecode = _bytecodes.top();
	bytecode->addInsn(BC_ILOAD);
	bytecode->addInt64(node->literal());
}

void CodeBuilderVisitor::visitLoadNode(LoadNode* node) {
}

void CodeBuilderVisitor::visitStoreNode(StoreNode* node) {
}

void CodeBuilderVisitor::visitForNode(ForNode* node) {
}

void CodeBuilderVisitor::visitWhileNode(WhileNode* node) {
}

void CodeBuilderVisitor::visitIfNode(IfNode* node) {
}

void CodeBuilderVisitor::visitBlockNode(BlockNode* node) {
	Scope* scope = node->scope();
	Scope::FunctionIterator it(scope);
	while (it.hasNext()) {
		processFunction(it.next());
	}
	node->visitChildren(this);
}

void CodeBuilderVisitor::visitFunctionNode(FunctionNode* node) {
	node->body()->visit(this);
//	BytecodeFunction* bf = new BytecodeFunction(node);
}

void CodeBuilderVisitor::visitReturnNode(ReturnNode* node) {
}

void CodeBuilderVisitor::visitCallNode(CallNode* node) {
}

void CodeBuilderVisitor::visitNativeCallNode(NativeCallNode* node) {
}

void CodeBuilderVisitor::visitPrintNode(PrintNode* node) {
}

} /* namespace mathvm */
