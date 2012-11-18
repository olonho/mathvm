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
	_varScopes.push(0);
}

CodeBuilderVisitor::~CodeBuilderVisitor() {
}

void CodeBuilderVisitor::processFunction(AstFunction* ast_function) {
	BytecodeFunction* function = new BytecodeFunction(ast_function);
	_code->addFunction(function);

	_functions.push(function);
	_varScopes.push(new VarScopeMap(function->id(), _varScopes.top()));

	ast_function->node()->visit(this);

	delete _varScopes.top();
	_varScopes.pop();
	_functions.pop();
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
	pushToStack(node->var());
}

void CodeBuilderVisitor::visitStoreNode(StoreNode* node) {
	node->value()->visit(this);

	// TODO: add -= += etc. support
	assert(node->op() == tASSIGN);

	Bytecode* bytecode = curBytecode();
	// TODO: add type conversion
	const AstVar* var = node->var();
	VarInfo varInfo = getVarInfo(var);

	if (varInfo.context == _functions.top()->id()) {
		switch (var->type()) {
			case VT_DOUBLE:		bytecode->addInsn(BC_STOREDVAR); break;
			case VT_INT:		bytecode->addInsn(BC_STOREIVAR); break;
			case VT_STRING:		bytecode->addInsn(BC_STORESVAR); break;
			case VT_INVALID:
			case VT_VOID:
			default:			assert(false); break;
		}
	} else {
		switch (var->type()) {
			case VT_DOUBLE:		bytecode->addInsn(BC_STORECTXDVAR); break;
			case VT_INT:		bytecode->addInsn(BC_STORECTXIVAR); break;
			case VT_STRING:		bytecode->addInsn(BC_STORECTXSVAR); break;
			case VT_INVALID:
			case VT_VOID:
			default:			assert(false); break;
		}
		curBytecode()->addInt16(varInfo.context);
	}

	bytecode->addUInt16(varInfo.id);
}

void CodeBuilderVisitor::visitForNode(ForNode* node) {
}

void CodeBuilderVisitor::visitWhileNode(WhileNode* node) {
	Bytecode* bytecode = curBytecode();

	bytecode->addInsn(BC_JA);
	uint32_t initial_jump = bytecode->current();
	bytecode->addInt16(0);

	uint32_t body_begin = bytecode->current();
	node->loopBlock()->visit(this);
	bytecode->setInt16(initial_jump, bytecode->current() - initial_jump);


	node->whileExpr()->visit(this);

	bytecode->addInsn(BC_ILOAD0);
	bytecode->addInsn(BC_IFICMPNE);
	bytecode->addInt16(body_begin - bytecode->current());
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

//	const uint16_t context = _functions.top()->id();
//	VarScopeMap* parentScope = _varScopes.top();
//	_varScopes.push(new VarScopeMap(context, parentScope));
	BytecodeFunction* function = _functions.top();
	Scope::VarIterator var_it(scope);
	while (var_it.hasNext()) {
		function->setLocalsNumber(function->localsNumber() + 1);
		_varScopes.top()->add(var_it.next());
	};

	Scope::FunctionIterator func_it(scope);
	while (func_it.hasNext()) {
		processFunction(func_it.next());
	}

	node->visitChildren(this);
//	delete _varScopes.top();
//	_varScopes.pop();
}

void CodeBuilderVisitor::visitFunctionNode(FunctionNode* node) {
	node->body()->visit(this);
//	curBytecode()->addInsn(BC_INVALID);
//	processFunction(node);

}

void CodeBuilderVisitor::visitReturnNode(ReturnNode* node) {
	node->visitChildren(this);
	curBytecode()->addInsn(BC_RETURN);
}

void CodeBuilderVisitor::visitCallNode(CallNode* node) {
	TranslatedFunction* function = _code->functionByName(node->name());

	for (size_t i = 1; i <= node->parametersNumber(); ++i) {
		size_t index = node->parametersNumber() - i;
		node->parameterAt(index)->visit(this);
	}

	Bytecode* bytecode = curBytecode();
	bytecode->addInsn(BC_CALL);
	bytecode->addUInt16(function->id());
}

void CodeBuilderVisitor::visitNativeCallNode(NativeCallNode* node) {
//	curBytecode()->addInsn(BC_CALLNATIVE);
//	curBytecode()->addUInt16(node->)
}

void CodeBuilderVisitor::visitPrintNode(PrintNode* node) {

}

void CodeBuilderVisitor::pushToStack(const AstVar* var) {
	Bytecode* bytecode = curBytecode();

	VarInfo varInfo = getVarInfo(var);

	if (varInfo.context == _functions.top()->id()) {
		switch (var->type()) {
			case VT_DOUBLE:		bytecode->addInsn(BC_LOADDVAR); break;
			case VT_INT:		bytecode->addInsn(BC_LOADIVAR); break;
			case VT_STRING:		bytecode->addInsn(BC_LOADSVAR); break;
			case VT_INVALID:
			case VT_VOID:
			default:			assert(false); break;
		}
	} else {
		switch (var->type()) {
			case VT_DOUBLE:		bytecode->addInsn(BC_LOADCTXDVAR); break;
			case VT_INT:		bytecode->addInsn(BC_LOADCTXIVAR); break;
			case VT_STRING:		bytecode->addInsn(BC_LOADCTXSVAR); break;
			case VT_INVALID:
			case VT_VOID:
			default:			assert(false); break;
		}
		curBytecode()->addInt16(varInfo.context);
	}

	curBytecode()->addUInt16(varInfo.id);
}

} /* namespace mathvm */
