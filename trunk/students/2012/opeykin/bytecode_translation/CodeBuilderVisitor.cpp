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
	switch (node->kind()) {
		case tOR:
		case tAND: visitBinaryLogic(node); break;
		case tEQ:
		case tNEQ:
		case tGT:
		case tGE:
		case tLT:
		case tLE: visitBinaryCondition(node); break;
		case tADD:
		case tSUB:
		case tMUL:
		case tDIV:
		case tMOD: visitBinaryCalc(node); break;
		case tINCRSET:
		case tDECRSET: break;
		default: assert(false); break;
	}
}

// || and &&
void CodeBuilderVisitor::visitBinaryLogic(BinaryOpNode* node) {
	Label right(bytecode());
	JumpLocation or_loc (_jmp_loc->first, &right);
	JumpLocation and_loc (&right, _jmp_loc->second);
	JumpLocation* old_loc = _jmp_loc;

	switch (node->kind()) {
		case tOR: 	_jmp_loc = &or_loc;	 break;
		case tAND: 	_jmp_loc = &and_loc; break;
		default: break;
	}

	node->left()->visit(this);
	right.bind(current());
	_jmp_loc = old_loc;
	node->right()->visit(this);
}

Instruction CodeBuilderVisitor::CondTokenToInstruction(TokenKind kind) {
	switch (kind) {
		case tEQ: 	return BC_IFICMPE;
		case tNEQ: 	return BC_IFICMPNE;
		case tGT: 	return BC_IFICMPG;
		case tGE: 	return BC_IFICMPGE;
		case tLT: 	return BC_IFICMPL;
		case tLE: 	return BC_IFICMPLE;
		default:
			ERROR("can not convert '" << kind << "' kind to token");
			return BC_INVALID;
	}
}

// >, <, != ...
void CodeBuilderVisitor::visitBinaryCondition(BinaryOpNode* node) {
	if (_types[node->right()] != VT_INT || _types[node->left()] != VT_INT) {
		ERROR("double comparison is too complex");
	}
	node->right()->visit(this);
	node->left()->visit(this);
	Instruction instruction = CondTokenToInstruction(node->kind());
	bytecode()->addBranch(instruction, *_jmp_loc->first);
	bytecode()->addBranch(BC_JA, *_jmp_loc->second);
}

Instruction CodeBuilderVisitor::CalcTokenToInstruction(TokenKind kind, VarType type) {
	if (type == VT_INT) {
		switch (kind) {
			case tADD: return BC_IADD;
			case tSUB: return BC_ISUB;
			case tMUL: return BC_IMUL;
			case tDIV: return BC_IDIV;
			case tMOD: return BC_IMOD;
			default:
				ERROR("unsupported kind: " << kind);
				return BC_INVALID;
		}
	} else if (type == VT_DOUBLE) {
		switch (kind) {
			case tADD: return BC_DADD;
			case tSUB: return BC_DSUB;
			case tMUL: return BC_DMUL;
			case tDIV: return BC_DDIV;
			default:
				ERROR("unsupported kind: " << kind);
				return BC_INVALID;
		}
	} else {
		ERROR("unsupported type for calculation TokenKind -> Instruction conversion");
		return BC_INVALID;
	}

}

// +, -, / ...
void CodeBuilderVisitor::visitBinaryCalc(BinaryOpNode* node) {
	VarType common_type = _types[node];
	assert(common_type == _types[node->left()]);
	assert(common_type == _types[node->right()]);
	node->right()->visit(this);
	node->left()->visit(this);
	addInsn(CalcTokenToInstruction(node->kind(), common_type));
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
