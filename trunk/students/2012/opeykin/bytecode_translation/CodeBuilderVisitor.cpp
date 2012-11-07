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
	_variables.push(VarScopeMap());

	ast_function->node()->visit(this);

	_variables.pop();
	_bytecodes.pop();
}

void CodeBuilderVisitor::visitBinaryOpNode(BinaryOpNode* node) {
	node->left()->visit(this);
	node->right()->visit(this);

	VarType common_type = VT_INT;

	typedef pair<TokenKind, VarType> Key;
	map<Key, Instruction> operations;

	operations[Key(tADD, VT_INT)] = BC_IADD;
	operations[Key(tSUB, VT_INT)] = BC_ISUB;
	operations[Key(tMUL, VT_INT)] = BC_IMUL;
	operations[Key(tDIV, VT_INT)] = BC_IDIV;

	operations[Key(tADD, VT_DOUBLE)] = BC_DADD;
	operations[Key(tSUB, VT_DOUBLE)] = BC_DSUB;
	operations[Key(tMUL, VT_DOUBLE)] = BC_DMUL;
	operations[Key(tDIV, VT_DOUBLE)] = BC_DDIV;

	map<Key, Instruction>::iterator it =
			operations.find(Key(node->kind(), common_type));

	if (it != operations.end()) {
		curBytecode()->addInsn(it->second);
	} else {
		assert(false);
//		curBytecode()->addInsn(BC_INVALID);
	}

//	Bytecode* bytecode = curBytecode();
//	//TODO: assumed that integers on stack
//	switch (node->kind()) {
//		case tADD:	bytecode->addInsn(BC_IADD); break;
//		case tSUB:	bytecode->addInsn(BC_ISUB);	break;
//		case tMUL:	bytecode->addInsn(BC_IMUL);	break;
//		case tDIV:	bytecode->addInsn(BC_IDIV);	break;
//		default:	assert(false);	break;
//	}
}

void CodeBuilderVisitor::visitUnaryOpNode(UnaryOpNode* node) {
}

void CodeBuilderVisitor::visitStringLiteralNode(StringLiteralNode* node) {
}

void CodeBuilderVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
	Bytecode* bytecode = curBytecode();
	bytecode->addInsn(BC_DLOAD);
	bytecode->addDouble(node->literal());
}

void CodeBuilderVisitor::visitIntLiteralNode(IntLiteralNode* node) {
	Bytecode* bytecode = curBytecode();
	bytecode->addInsn(BC_ILOAD);
	bytecode->addInt64(node->literal());
}

void CodeBuilderVisitor::visitLoadNode(LoadNode* node) {
	const AstVar* var = node->var();
	Bytecode* bytecode = curBytecode();

	switch (var->type()) {
		case VT_DOUBLE:		bytecode->addInsn(BC_LOADDVAR); break;
		case VT_INT:		bytecode->addInsn(BC_LOADIVAR); break;
		case VT_STRING:		bytecode->addInsn(BC_LOADSVAR); break;
		case VT_INVALID:
		case VT_VOID:
		default:			assert(false); break;
	}

	curBytecode()->addUInt16(getId(var));
}

void CodeBuilderVisitor::visitStoreNode(StoreNode* node) {
	node->value()->visit(this);

	// TODO: add -= += etc. support
	assert(node->op() == tASSIGN);

	Bytecode* bytecode = curBytecode();
	// TODO: add type conversion
	const AstVar* var = node->var();
	switch (var->type()) {
		case VT_DOUBLE:		bytecode->addInsn(BC_STOREDVAR); break;
		case VT_INT:		bytecode->addInsn(BC_STOREIVAR); break;
		case VT_STRING:		bytecode->addInsn(BC_STORESVAR); break;
		case VT_INVALID:
		case VT_VOID:
		default:			assert(false); break;
	}

	uint16_t id = curVarMap()[var->name()];
	bytecode->addUInt16(id);
}

void CodeBuilderVisitor::visitForNode(ForNode* node) {
}

void CodeBuilderVisitor::visitWhileNode(WhileNode* node) {
}

void CodeBuilderVisitor::visitIfNode(IfNode* node) {
	node->ifExpr()->visit(this);
	Bytecode* bytecode = curBytecode();
	bytecode->addInsn(BC_ILOAD0);
	bytecode->addInsn(BC_IFICMPE);
	uint32_t cond_fail_pos = bytecode->current();
	bytecode->addInt16(0);

	node->thenBlock()->visit(this);
	if (node->elseBlock()) {
		bytecode->addInsn(BC_JA);
		uint32_t end_pos = bytecode->current();
		bytecode->addInt16(0);

		bytecode->setInt16(cond_fail_pos, bytecode->current() - cond_fail_pos);
		node->elseBlock()->visit(this);

		bytecode->setInt16(end_pos, bytecode->current() - end_pos);
	} else {
		bytecode->setInt16(cond_fail_pos, bytecode->current() - cond_fail_pos);
	}
}

void CodeBuilderVisitor::visitBlockNode(BlockNode* node) {
	Scope* scope = node->scope();

	Scope::VarIterator var_it(scope);
	while (var_it.hasNext()) {
		AstVar* var = var_it.next();
		VarScopeMap& vars = curVarMap();
		// TODO: process name duplicate compile error
		assert(vars.find(var->name()) == vars.end());
		uint16_t id = vars.size();
		vars[var->name()] = id;
	};


	Scope::FunctionIterator func_it(scope);
	while (func_it.hasNext()) {
		processFunction(func_it.next());
	}

	node->visitChildren(this);
}

void CodeBuilderVisitor::visitFunctionNode(FunctionNode* node) {
	node->body()->visit(this);
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
