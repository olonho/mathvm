//
// Created by svloyso on 20.11.16.
//

#include "translator_visitor.h"

namespace mathvm {

void TranslatorVisitor::visitStoreNode(StoreNode *node) {
	const AstVar* var = node->var();
	AstNode* val = node->value();

	int var_id = ctx.varIdByName(var->name());
	if(var_id == -1) {
		throw Status::Error(("Using of undeclared variable " + var->name()).c_str(), node->position());
	}
	val->visit(this);

	Instruction insn;
	Status* err = Status::Error(("Invalid assign operator for store into variable " + var->name() + ": " + tokenStr(node->op())).c_str(), node->position());
	switch(node->op()) {
		case tASSIGN:
			break;
		case tINCRSET:
			loadVariable(var, node->position());
			OPFORINTEGRALTYPE(var->type(), BC_, ADD, insn, err);
			ctx.bytecode()->addInsn(insn);
			break;
		case tDECRSET:
			loadVariable(var, node->position());
			OPFORINTEGRALTYPE(var->type(), BC_, SUB, insn, err);
			ctx.bytecode()->addInsn(BC_SWAP);
			ctx.bytecode()->addInsn(insn);
			break;
		default:
			throw err;
	}
	OPFORTYPE(var->type(), BC_STORECTX, VAR, insn, Status::Error(("Invalid type of variable " + var->name()).c_str(), node->position()));
	ctx.bytecode()->addInsn(insn);
	ctx.bytecode()->addUInt16(ctx.functions.top()->id());
	ctx.bytecode()->addUInt16(var_id);

	lastResult = VT_VOID;
}

void TranslatorVisitor::visitLoadNode(LoadNode *node) {
	const AstVar* var = node->var();
	loadVariable(var, node->position());
	lastResult = var->type();
}

void TranslatorVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
	if(node->literal() == 0.0) {
		ctx.bytecode()->addInsn(BC_DLOAD0);
	} else if(node->literal() == 1) {
		ctx.bytecode()->addInsn(BC_DLOAD1);
	} else if(node->literal() == -1) {
		ctx.bytecode()->addInsn(BC_DLOADM1);
	} else {
		ctx.bytecode()->addInsn(BC_DLOAD);
		ctx.bytecode()->addDouble(node->literal());
	}
	lastResult = VT_DOUBLE;
}

void TranslatorVisitor::visitIntLiteralNode(IntLiteralNode *node) {
	loadInt(node->literal());
	lastResult = VT_INT;
}

void TranslatorVisitor::visitStringLiteralNode(StringLiteralNode *node) {
	if(node->literal().empty()) {
		ctx.bytecode()->addInsn(BC_SLOAD0);
	} else {
		int16_t id = code->makeStringConstant(node->literal());
		ctx.bytecode()->addInsn(BC_SLOAD);
		ctx.bytecode()->addInt16(id);
	}
	lastResult = VT_STRING;
}

void TranslatorVisitor::visitUnaryOpNode(UnaryOpNode *node) {
	Instruction insn;
	node->operand()->visit(this);
	switch(node->kind()) {
		case tNOT:
			if(lastResult != VT_INT) {
				throw Status::Error("Invalid operand type", node->position());
			}
			ctx.bytecode()->addInsn(BC_ILOAD0);
			cmpAndLoad(VT_INT, tEQ);
			break;
		case tSUB:
			if(lastResult != VT_INT && lastResult != VT_DOUBLE) {
				throw Status::Error("Invalid operand type", node->position());
			}
			OPFORINTEGRALTYPE(lastResult, BC_, NEG, insn, nullptr);
			ctx.bytecode()->addInsn(insn);
			break;
		default:
			throw Status::Error((std::string("Invalid token ") + tokenOp(node->kind())).c_str(), node->position());
	}
}

void TranslatorVisitor::cmpAndLoad(VarType type, TokenKind kind) {
	Bytecode* bc = ctx.bytecode();
	Label then(bc), end(bc);
	Instruction insn;

	OPFORINTEGRALTYPE(type, BC_, CMP, insn, Status::Error("Invalid type"));
	bc->addInsn(insn);

	Instruction cmpInsn;
	Instruction doThen;
	Instruction doElse;
	switch(kind) {
		case tEQ:
			cmpInsn = BC_ILOAD0;
			doThen = BC_ILOAD1;
			doElse = BC_ILOAD0;
			break;
		case tNEQ:
			cmpInsn = BC_ILOAD0;
			doThen = BC_ILOAD0;
			doElse = BC_ILOAD1;
			break;
		case tGE:
			cmpInsn = BC_ILOADM1;
			doThen = BC_ILOAD0;
			doElse = BC_ILOAD1;
			break;
		case tGT:
			cmpInsn = BC_ILOAD1;
			doThen = BC_ILOAD1;
			doElse = BC_ILOAD0;
			break;
		case tLE:
			cmpInsn = BC_ILOAD1;
			doThen = BC_ILOAD0;
			doElse = BC_ILOAD1;
			break;
		case tLT:
			cmpInsn = BC_ILOADM1;
			doThen = BC_ILOAD1;
			doElse = BC_ILOAD0;
			break;
		default:
			throw Status::Error("Invalid comparator");
	}
	bc->addInsn(cmpInsn);
	bc->addBranch(BC_IFICMPE, then);
	bc->addInsn(doElse);
	bc->addBranch(BC_JA, end);
	bc->bind(then);
	bc->addInsn(doThen);
	bc->bind(end);
}

void TranslatorVisitor::visitBinaryOpNode(BinaryOpNode *node) {
	Bytecode* bc = ctx.bytecode();
	Instruction insn;
	VarType t1;
	VarType t2;
	node->left()->visit(this);
	t1 = lastResult;
	node->right()->visit(this);
	t2 = lastResult;

	switch(node->kind()) {
		case tOR:
		case tAOR:
			convertLogic(t1, t2, node->position());
			bc->addInsn(BC_IAOR);
			break;
		case tAND:
		case tAAND:
			convertLogic(t1, t2, node->position());
			bc->addInsn(BC_IAAND);
			break;
		case tAXOR:
			convertLogic(t1, t2, node->position());
			bc->addInsn(BC_IAXOR);
			break;
		case tEQ:
			convertCmp(t1, t2, node->position());
			cmpAndLoad(t1, node->kind());
			break;
		case tNEQ:
			convertCmp(t1, t2, node->position());
			cmpAndLoad(t1, node->kind());
			break;
		case tGT:
			convertCmp(t1, t2, node->position());
			cmpAndLoad(t1, node->kind());
			break;
		case tGE:
			convertCmp(t1, t2, node->position());
			cmpAndLoad(t1, node->kind());
			break;
		case tLT:
			convertCmp(t1, t2, node->position());
			cmpAndLoad(t1, node->kind());
			break;
		case tLE:
			convertCmp(t1, t2, node->position());
			cmpAndLoad(t1, node->kind());
			break;
		case tADD:
			convertAriphmetic(t1, t2, node->position());
			OPFORINTEGRALTYPE(t1, BC_, ADD, insn, nullptr);
			bc->addInsn(insn);
			break;
		case tSUB:
			convertAriphmetic(t1, t2, node->position());
			OPFORINTEGRALTYPE(t1, BC_, SUB, insn, nullptr);
			bc->addInsn(insn);
			break;
		case tMUL:
			convertAriphmetic(t1, t2, node->position());
			OPFORINTEGRALTYPE(t1, BC_, MUL, insn, nullptr);
			bc->addInsn(insn);
			break;
		case tDIV:
			convertAriphmetic(t1, t2, node->position());
			OPFORINTEGRALTYPE(t1, BC_, DIV, insn, nullptr);
			bc->addInsn(insn);
			break;
		case tMOD:
			convertLogic(t1, t2, node->position());
			bc->addInsn(BC_IMOD);
			break;
		default:
			throw Status::Error((std::string("Invalid token ") + tokenOp(node->kind())).c_str(), node->position());
	}
}

void TranslatorVisitor::visitFunctionNode(FunctionNode *node) {
	BlockNode* body = node->body();
	if (node->body()->nodes() > 0 &&
            node->body()->nodeAt(0)->isNativeCallNode()) {
        code->makeNativeFunction(node->name(), node->signature(), nullptr);
        return;
    }
	body->visit(this);
	if(lastResult != VT_INVALID && node->returnType() == VT_VOID) {
		ctx.bytecode()->addInsn(BC_RETURN); //just in case
	}
}

void TranslatorVisitor::visitForNode(ForNode *node) {
	Bytecode* bc = ctx.bytecode();
	BinaryOpNode* range = node->inExpr()->asBinaryOpNode();
	if(range->kind() != tRANGE) {
		throw Status::Error("Not range expression in for node", node->position());
	}
	AstNode* left = range->left();
	AstNode* right = range->right();
	const AstVar* var = node->var();
	uint16_t varId = ctx.declareVar(var->name());

	left->visit(this);
	if(lastResult != VT_INT) {
		throw Status::Error("Only int allowed in range", node->position());
	}
	uint16_t fid = ctx.functions.top()->id();
	bc->addInsn(BC_STORECTXIVAR);
	bc->addUInt16(fid);
	bc->addUInt16(varId);

	Label start(bc), end(bc);

	bc->bind(start);
	bc->addInsn(BC_LOADCTXIVAR);
	bc->addUInt16(fid);
	bc->addUInt16(varId);
	right->visit(this);
	if(lastResult != VT_INT) {
		throw Status::Error("Only int allowed in range", node->position());
	}
	bc->addBranch(BC_IFICMPG, end);

	node->body()->visit(this);

	bc->addInsn(BC_LOADCTXIVAR);
	bc->addUInt16(fid);
	bc->addUInt16(varId);
	bc->addInsn(BC_ILOAD1);
	bc->addInsn(BC_IADD);
	bc->addInsn(BC_STORECTXIVAR);
	bc->addUInt16(fid);
	bc->addUInt16(varId);
	bc->addBranch(BC_JA, start);
	bc->bind(end);
}

void TranslatorVisitor::visitIfNode(IfNode *node) {
	Bytecode* bc = ctx.bytecode();

	node->ifExpr()->visit(this);

	Label elseLabel(bc);
	bc->addInsn(BC_ILOAD0);
	bc->addBranch(BC_IFICMPE, elseLabel);
	node->thenBlock()->visit(this);
	if(node->elseBlock()) {
		Label end(bc);
		bc->addBranch(BC_JA, end);
		bc->bind(elseLabel);
		node->elseBlock()->visit(this);
		bc->bind(end);
	} else {
		bc->bind(elseLabel);
	}
}

void TranslatorVisitor::visitWhileNode(WhileNode *node) {
	Bytecode* bc = ctx.bytecode();
	Label start(bc), end(bc);

	bc->bind(start);
	node->whileExpr()->visit(this);
	bc->addInsn(BC_ILOAD0);
	bc->addBranch(BC_IFICMPE, end);

	node->loopBlock()->visit(this);
	bc->addBranch(BC_JA, start);
	bc->bind(end);
}

void TranslatorVisitor::visitPrintNode(PrintNode *node) {
	for(uint32_t i = 0; i < node->operands(); ++i) {
		AstNode* operand = node->operandAt(i);
		Instruction insn;
		operand->visit(this);
		OPFORTYPE(lastResult, BC_, PRINT, insn, nullptr);
		ctx.bytecode()->addInsn(insn);
	}
}

void TranslatorVisitor::visitNativeCallNode(NativeCallNode *node) {
	assert(false);
}

void TranslatorVisitor::visitBlockNode(BlockNode *node) {
	Scope* scope = node->scope();

	declareScope(scope);

	for(uint32_t i = 0; i < node->nodes(); ++i) {
		AstNode* child = node->nodeAt(i);
		child->visit(this);
		if (child->isIntLiteralNode()
                    || child->isStringLiteralNode()
                    || child->isDoubleLiteralNode()
                    || child->isLoadNode()
                    || (child->isCallNode() && code->functionByName(child->asCallNode()->name())->returnType() != VT_VOID)
                    || child->isBinaryOpNode()
                    || child->isUnaryOpNode())
		{
			ctx.bytecode()->addInsn(BC_POP);
        }
	}

	ctx.scope = ctx.scope->parent();
}

void TranslatorVisitor::visitCallNode(CallNode *node) {
	TranslatedFunction* func = code->functionByName(node->name());
	if(func == nullptr) {
		throw Status::Error(("Using undeclared function " + node->name()).c_str(), node->position());
	}
	if(func->parametersNumber() != node->parametersNumber()) {
		throw Status::Error("Invalid parameters count", node->position());
	}
	for(uint32_t i = 0; i < node->parametersNumber(); ++i) {
		AstNode* param = node->parameterAt(i);
		param->visit(this);
		if(lastResult != func->parameterType(i)) {
			throw Status::Error("Invalid parameter type", node->position());
		}
	}
	uint16_t id = func->id();
	ctx.bytecode()->addInsn(BC_CALL);
	ctx.bytecode()->addUInt16(id);
}

void TranslatorVisitor::visitReturnNode(ReturnNode *node) {
	AstNode* expr = node->returnExpr();
	if(expr != nullptr) {
		expr->visit(this);
	}
	ctx.bytecode()->addInsn(BC_RETURN);
}

void TranslatorVisitor::loadVariable(const AstVar *var, uint32_t pos) {
	Instruction insn;

	int var_id = ctx.varIdByName(var->name());

	if(var_id == -1) {
		throw Status::Error(("Using of undeclared variable " + var->name()).c_str(), pos);
	}

	switch(var->type()) {
		case VT_INVALID: /* fall through */
		case VT_VOID:
			throw Status::Error(("Invalid type of variable " + var->name()).c_str(), pos);
		case VT_DOUBLE:
			insn = BC_LOADCTXDVAR;
			break;
		case VT_INT:
			insn = BC_LOADCTXIVAR;
			break;
		case VT_STRING:
			insn = BC_LOADCTXSVAR;
			break;
	}
	ctx.bytecode()->addInsn(insn);
	ctx.bytecode()->addUInt16(ctx.functions.top()->id());
	ctx.bytecode()->addUInt16(var_id);
}

void TranslatorVisitor::loadInt(int64_t val) {
	switch(val) {
		case 0:
			ctx.bytecode()->addInsn(BC_ILOAD0);
			break;
		case 1:
			ctx.bytecode()->addInsn(BC_ILOAD1);
			break;
		case -1:
			ctx.bytecode()->addInsn(BC_ILOADM1);
			break;
		default:
			ctx.bytecode()->addInsn(BC_ILOAD);
			ctx.bytecode()->addInt64(val);
	}
}

void TranslatorVisitor::convertCmp(VarType &t1, VarType &t2, uint32_t pos) {
	Bytecode* bc = ctx.bytecode();
	if(t1 != t2 || t1 == VT_STRING) {
		if(t1 == VT_STRING && t2 == VT_STRING) {
			bc->addInsn(BC_S2I);
			bc->addInsn(BC_SWAP);
			bc->addInsn(BC_S2I);
			bc->addInsn(BC_SWAP);
			t1 = VT_INT;
			t2 = VT_INT;
		} else if(t1 == VT_DOUBLE && t2 == VT_INT) {
			bc->addInsn(BC_I2D);
			t2 = VT_DOUBLE;
		} else if(t2 == VT_DOUBLE && t1 == VT_INT) {
			bc->addInsn(BC_SWAP);
			bc->addInsn(BC_I2D);
			bc->addInsn(BC_SWAP);
			t1 = VT_DOUBLE;
		} else {
			throw Status::Error("Invalid operand type", pos);
		}
	}
}

void TranslatorVisitor::convertAriphmetic(VarType &t1, VarType &t2, uint32_t pos) {
	Bytecode* bc = ctx.bytecode();
	if(t1 != t2 || t1 == VT_STRING) {
		if(t1 == VT_DOUBLE && t2 == VT_INT) {
			bc->addInsn(BC_I2D);
			t2 = VT_DOUBLE;
		} else if(t2 == VT_DOUBLE && t1 == VT_INT) {
			bc->addInsn(BC_SWAP);
			bc->addInsn(BC_I2D);
			bc->addInsn(BC_SWAP);
			t1 = VT_DOUBLE;
		} else {
			throw Status::Error("Invalid operand type", pos);
		}
	}
}

void TranslatorVisitor::convertLogic(VarType &t1, VarType &t2, uint32_t pos) {
	if(t1 != VT_INT || t2 != VT_INT) {
		throw Status::Error("Invalid operand type", pos);
	}
}

void TranslatorVisitor::declareScope(Scope *scope, bool isGlobal) {
	if(!isGlobal) {
		assert(scope->parent() == ctx.scope);
	}
	ctx.scope = scope;
	Scope::VarIterator vIt(scope);
	while(vIt.hasNext()) {
		AstVar* var = vIt.next();
		ctx.declareVar(var->name());
	}

	Scope::FunctionIterator fIt(scope);
	while(fIt.hasNext()) {
		AstFunction *func = fIt.next();
		BytecodeFunction *bFunc = new BytecodeFunction(func);
		code->addFunction(bFunc);
	}
	fIt = Scope::FunctionIterator(scope);
	while(fIt.hasNext()) {
		AstFunction *func = fIt.next();
		BytecodeFunction *bFunc = dynamic_cast<BytecodeFunction*>(code->functionByName(func->name()));
		assert(bFunc);
		Scope* fScope = func->scope();
		ctx.functions.push(bFunc);
		if(!isGlobal) {
			declareScope(fScope);
		}
		func->node()->visit(this);
		ctx.scope = ctx.scope->parent();
		ctx.functions.pop();
	}
}

} // namespace mathvm