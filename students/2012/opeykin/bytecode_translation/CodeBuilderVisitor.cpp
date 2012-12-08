/*
 * CodeBuilderVisitor.cpp
 *
 *  Created on: Oct 24, 2012
 *      Author: alex
 */

#include "CodeBuilderVisitor.h"
#include "type_inference_visitor.h"
#include "mathvm.h"

namespace mathvm {

CodeBuilderVisitor::CodeBuilderVisitor(Code* code)
		: _code(code) {
	_varScopes.push(0);
}

CodeBuilderVisitor::~CodeBuilderVisitor() {
}

Status* CodeBuilderVisitor::start(AstFunction* top) {
	TypeInferenceVisitor typer(_types);
	typer.processFunction(top);
	processFunction(top);
	return 0;
}

void CodeBuilderVisitor::dummyCond(AstNode* cond, Label& label) {
	if (cond->isLoadNode()) {
		LoadNode* loadNode = static_cast<LoadNode*>(cond);
		assert(loadNode->var()->type() == VT_INT);
		addInsn(BC_ILOAD0);
		pushToStack(loadNode->var());
		bytecode()->addBranch(BC_IFICMPNE, label);
	} else if (cond->isIntLiteralNode()) {
		int64_t value = ((IntLiteralNode*)cond)->literal();
		if (value != 0) {
			bytecode()->addBranch(BC_JA, label);
		}
	} else if (cond->isBinaryOpNode()) {
		BinaryOpNode* binaryOpNode = static_cast<BinaryOpNode*>(cond);
		binaryOpNode->left()->visit(this);
		binaryOpNode->right()->visit(this);
		switch(binaryOpNode->kind()) {
			case tEQ: bytecode()->addBranch(BC_IFICMPE, label); break;
			case tNEQ: bytecode()->addBranch(BC_IFICMPNE, label); break;
			case tGT: bytecode()->addBranch(BC_IFICMPG, label); break;
			case tGE: bytecode()->addBranch(BC_IFICMPGE, label); break;
			case tLT: bytecode()->addBranch(BC_IFICMPL, label); break;
			case tLE: bytecode()->addBranch(BC_IFICMPLE, label); break;
			default: assert(false); break;
		}
	} else {
		assert(false);
	}
}

void CodeBuilderVisitor::processFunction(AstFunction* ast_function) {
	BytecodeFunction* function = new BytecodeFunction(ast_function);
	uint16_t id = _code->addFunction(function);

	_functions.push(function);
	VarScopeMap* varScope = new VarScopeMap(function->id(), _varScopes.top());
	_varScopes.push(varScope);

	Scope::VarIterator arg_it(ast_function->scope());
	while (arg_it.hasNext()) {
		AstVar* var = arg_it.next();
		uint16_t id = varScope->add(var);
		storeLocalVar(var->type(), id);
	};

	ast_function->node()->visit(this);

	delete _varScopes.top();
	_varScopes.pop();
	_functions.pop();

	if(id == 0) {
		function->bytecode()->addInsn(BC_STOP);
	}
}

void CodeBuilderVisitor::storeLocalVar(VarType type, uint16_t id) {
	Instruction int_codes 		[] = {BC_STOREIVAR0, BC_STOREIVAR1, BC_STOREIVAR2, BC_STOREIVAR3};
	Instruction double_codes 	[] = {BC_STOREDVAR0, BC_STOREDVAR1, BC_STOREDVAR2, BC_STOREDVAR3};
	Instruction string_codes 	[] = {BC_STORESVAR0, BC_STORESVAR1, BC_STORESVAR2, BC_STORESVAR3};

	if (id < 4) {
		switch (type) {
			case VT_DOUBLE:		addInsn(double_codes[id]); break;
			case VT_INT:		addInsn(int_codes[id]); break;
			case VT_STRING:		addInsn(string_codes[id]); break;
			case VT_INVALID:
			case VT_VOID:
			default:			assert(false); break;
		}
	} else {
		switch (type) {
			case VT_DOUBLE:		addInsn(BC_STOREDVAR); break;
			case VT_INT:		addInsn(BC_STOREIVAR); break;
			case VT_STRING:		addInsn(BC_STORESVAR); break;
			case VT_INVALID:
			case VT_VOID:
			default:			assert(false); break;
		}
		addUInt16(id);
	}
}

void CodeBuilderVisitor::loadLocalVar(VarType type, uint16_t id) {
	Instruction int_codes 		[] = {BC_LOADIVAR0, BC_LOADIVAR1, BC_LOADIVAR2, BC_LOADIVAR3};
	Instruction double_codes 	[] = {BC_LOADDVAR0, BC_LOADDVAR1, BC_LOADDVAR2, BC_LOADDVAR3};
	Instruction string_codes 	[] = {BC_LOADSVAR0, BC_LOADSVAR1, BC_LOADSVAR2, BC_LOADSVAR3};

	if (id < 4) {
		switch (type) {
			case VT_DOUBLE:		addInsn(double_codes[id]); break;
			case VT_INT:		addInsn(int_codes[id]); break;
			case VT_STRING:		addInsn(string_codes[id]); break;
			case VT_INVALID:
			case VT_VOID:
			default:			assert(false); break;
		}
	} else {
		switch (type) {
			case VT_DOUBLE:		addInsn(BC_LOADDVAR); break;
			case VT_INT:		addInsn(BC_LOADIVAR); break;
			case VT_STRING:		addInsn(BC_LOADSVAR); break;
			case VT_INVALID:
			case VT_VOID:
			default:			assert(false); break;
		}
		addUInt16(id);
	}
}

void CodeBuilderVisitor::visitBinaryOpNode(BinaryOpNode* node) {


	VarType common_type = VT_INT;

	typedef pair<TokenKind, VarType> Key;
	map<Key, Instruction> operations;
	TokenKind kind = node->kind();

	operations[Key(tADD, VT_INT)] = BC_IADD;
	operations[Key(tSUB, VT_INT)] = BC_ISUB;
	operations[Key(tMUL, VT_INT)] = BC_IMUL;
	operations[Key(tDIV, VT_INT)] = BC_IDIV;
	operations[Key(tMOD, VT_INT)] = BC_IMOD;

	operations[Key(tADD, VT_DOUBLE)] = BC_DADD;
	operations[Key(tSUB, VT_DOUBLE)] = BC_DSUB;
	operations[Key(tMUL, VT_DOUBLE)] = BC_DMUL;
	operations[Key(tDIV, VT_DOUBLE)] = BC_DDIV;

	map<Key, Instruction>::iterator it =
			operations.find(Key(kind, common_type));

	if (it != operations.end()) {
		node->right()->visit(this);
		node->left()->visit(this);
		addInsn(it->second);
	} else {
		Label right(bytecode());
		JumpLocation or_loc (_jmp_loc->first, &right);
		JumpLocation and_loc (&right, _jmp_loc->second);
		JumpLocation* old_loc = _jmp_loc;
		switch (kind) {
			case tOR:
				_jmp_loc = &or_loc;
				node->left()->visit(this);
				right.bind(current());
				_jmp_loc = old_loc;
				node->right()->visit(this);
				break;
			case tAND:
				_jmp_loc = &and_loc;
				node->left()->visit(this);
				right.bind(current());
				_jmp_loc = old_loc;
				node->right()->visit(this);
				break;
			default: break;
		}



#define CASE(TYPE, INSN) \
		case TYPE: \
			node->right()->visit(this); \
			node->left()->visit(this); \
			bytecode()->addBranch(INSN, *_jmp_loc->first); \
			bytecode()->addBranch(BC_JA, *_jmp_loc->second); \
			break;

		switch (kind) {
			CASE(tEQ, BC_IFICMPE);
			CASE(tNEQ, BC_IFICMPNE);
			CASE(tGT, BC_IFICMPG);
			CASE(tGE, BC_IFICMPGE);
			CASE(tLT, BC_IFICMPL);
			CASE(tLE, BC_IFICMPLE);
			default: break;
		}
#undef CASE

	}
}

void CodeBuilderVisitor::addInsn(Instruction instruction) {
	bytecode()->addInsn(instruction);
}

void CodeBuilderVisitor::addUInt16(uint16_t value) {
	bytecode()->addUInt16(value);
}

void CodeBuilderVisitor::visitUnaryOpNode(UnaryOpNode* node) {
}

void CodeBuilderVisitor::visitStringLiteralNode(StringLiteralNode* node) {
	uint16_t id = _code->makeStringConstant(node->literal());
	addInsn(BC_SLOAD);
	addUInt16(id);
}

void CodeBuilderVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
	addInsn(BC_DLOAD);
	bytecode()->addDouble(node->literal());
}

void CodeBuilderVisitor::visitIntLiteralNode(IntLiteralNode* node) {
	addInsn(BC_ILOAD);
	bytecode()->addInt64(node->literal());
}

void CodeBuilderVisitor::visitLoadNode(LoadNode* node) {
	pushToStack(node->var());
}

void CodeBuilderVisitor::visitStoreNode(StoreNode* node) {
	node->value()->visit(this);

	// TODO: add -= += etc. support
	assert(node->op() == tASSIGN);

	// TODO: add type conversion
	const AstVar* var = node->var();
	VarInfo varInfo = getVarInfo(var);

	if (varInfo.context == _functions.top()->id()) {
		storeLocalVar(var->type(), varInfo.id);
	} else {
		switch (var->type()) {
			case VT_DOUBLE:		addInsn(BC_STORECTXDVAR); break;
			case VT_INT:		addInsn(BC_STORECTXIVAR); break;
			case VT_STRING:		addInsn(BC_STORECTXSVAR); break;
			case VT_INVALID:
			case VT_VOID:
			default:			assert(false); break;
		}
		bytecode()->addInt16(varInfo.context);
		addUInt16(varInfo.id);
	}

}

void CodeBuilderVisitor::visitForNode(ForNode* node) {
	node->inExpr()->visit(this);
	addInsn(BC_INVALID);
	node->body()->visit(this);
//
//	bytecode->addInsn(BC_JA);
//	uint32_t jump_to_cond = bytecode->current();
//	bytecode->addInt16(0);
//	uint32_t body_begin = bytecode->current();
//		node->loopBlock()->visit(this);
//		bytecode->setInt16(jump_to_cond, bytecode->current() - jump_to_cond);
//		node->whileExpr()->visit(this);
//		bytecode->addInsn(BC_ILOAD0);
//		bytecode->addInsn(BC_IFICMPNE);
//		bytecode->addInt16(body_begin - bytecode->current());
}



void CodeBuilderVisitor::visitWhileNode(WhileNode* node) {
//	Label true_label(bytecode());
//	Label false_label(bytecode());
//	processCondition(node->ifExpr(), &true_label, &false_label);
//	true_label.bind(current());
//	node->thenBlock()->visit(this);
//	if (node->elseBlock()) {
//		Label end_label(bytecode());
//		bytecode()->addBranch(BC_JA, end_label);
//		false_label.bind(current());
//		node->elseBlock()->visit(this);
//		end_label.bind(current());
//	} else {
//		false_label.bind(current());
//	}

	Label condition_label(bytecode());
	Label block_label(bytecode());
	Label end_label(bytecode());

	bytecode()->addBranch(BC_JA, condition_label);
	block_label.bind(current());
	node->loopBlock()->visit(this);

	condition_label.bind(current());
	processCondition(node->whileExpr(), &block_label, &end_label);
	end_label.bind(current());
}

void CodeBuilderVisitor::visitIfNode(IfNode* node) {
	Label true_label(bytecode());
	Label false_label(bytecode());
	processCondition(node->ifExpr(), &true_label, &false_label);
	true_label.bind(current());
	node->thenBlock()->visit(this);
	if (node->elseBlock()) {
		Label end_label(bytecode());
		bytecode()->addBranch(BC_JA, end_label);
		false_label.bind(current());
		node->elseBlock()->visit(this);
		end_label.bind(current());
	} else {
		false_label.bind(current());
	}
}

void CodeBuilderVisitor::visitBlockNode(BlockNode* node) {
	Scope* scope = node->scope();
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
}

void CodeBuilderVisitor::visitFunctionNode(FunctionNode* node) {
	node->body()->visit(this);

}

void CodeBuilderVisitor::visitReturnNode(ReturnNode* node) {
	node->visitChildren(this);
	addInsn(BC_RETURN);
}

void CodeBuilderVisitor::visitCallNode(CallNode* node) {
	TranslatedFunction* function = _code->functionByName(node->name());

	//TODO: throw here is name does not exist

	for (size_t i = 1; i <= node->parametersNumber(); ++i) {
		size_t index = node->parametersNumber() - i;
		node->parameterAt(index)->visit(this);
	}

	addInsn(BC_CALL);
	addUInt16(function->id());
}

void CodeBuilderVisitor::visitNativeCallNode(NativeCallNode* node) {
//	curBytecode()->addInsn(BC_CALLNATIVE);
//	curBytecode()->addUInt16(node->)
}

void CodeBuilderVisitor::visitPrintNode(PrintNode* node) {
	for (uint32_t i = 0; i < node->operands(); ++i) {
		AstNode* operand = node->operandAt(i);
		operand->visit(this);
		switch (_types[operand]) {
			case VT_DOUBLE:		addInsn(BC_DPRINT); break;
			case VT_INT:		addInsn(BC_IPRINT); break;
			case VT_STRING:		addInsn(BC_SPRINT); break;
			case VT_INVALID:
			case VT_VOID:
			default:			assert(false); break;
		}
	}

}

void CodeBuilderVisitor::pushToStack(const AstVar* var) {
	VarInfo varInfo = getVarInfo(var);

	if (varInfo.context == _functions.top()->id()) {
		loadLocalVar(var->type(), varInfo.id);
	} else {
		switch (var->type()) {
			case VT_DOUBLE:		addInsn(BC_LOADCTXDVAR); break;
			case VT_INT:		addInsn(BC_LOADCTXIVAR); break;
			case VT_STRING:		addInsn(BC_LOADCTXSVAR); break;
			case VT_INVALID:
			case VT_VOID:
			default:			assert(false); break;
		}
		bytecode()->addInt16(varInfo.context);
		addUInt16(varInfo.id);
	}
}

void CodeBuilderVisitor::processCondition(AstNode* node, Label* trueJump, Label* falseJump) {
	JumpLocation location (trueJump, falseJump);
	_jmp_loc = &location;
	node->visit(this);
}

} /* namespace mathvm */
