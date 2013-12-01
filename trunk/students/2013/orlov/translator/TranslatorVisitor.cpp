#include "TranslatorVisitor.h"

namespace mathvm {

#define INSN(bc, type, suff) (type == VT_INT ? bc##I##suff : \
		(type == VT_DOUBLE ? bc##D##suff : (type == VT_STRING ? \
				bc##S##suff : BC_INVALID)))
#define INSN_NUM(type, suff) (type == VT_INT ? BC_##I##suff \
		: (type == VT_DOUBLE ? BC_##D##suff : BC_INVALID))

void TranslatorVisitor::visitBinaryOpNode(BinaryOpNode * node) {
	node->right()->visit(this);
	VarType first = lastExpressionType;
	node->left()->visit(this);
	VarType second = lastExpressionType;
	if (!(first == VT_INT || first == VT_DOUBLE)
			|| !(second == VT_INT || second == VT_DOUBLE)) {
		throw std::string("Error in binary expression");
	}

	switch (node->kind()) {
	case tAND:
		convertToBool();
		currentFunction->bytecode()->addInsn(BC_SWAP);
		convertToBool();
		currentFunction->bytecode()->addInsn(BC_SWAP);
		currentFunction->bytecode()->addInsn(BC_IMUL);
		break;

	case tOR:
		convertToBool();
		currentFunction->bytecode()->addInsn(BC_SWAP);
		convertToBool();
		currentFunction->bytecode()->addInsn(BC_SWAP);
		currentFunction->bytecode()->addInsn(BC_IADD);
		break;

	case tAAND:
	case tAOR:
	case tAXOR:
		if (first == VT_DOUBLE) {
			currentFunction->bytecode()->addInsn(BC_D2I);
			lastExpressionType = VT_INT;
		}
		if (second == VT_DOUBLE) {
			currentFunction->bytecode()->addInsn(BC_SWAP);
			currentFunction->bytecode()->addInsn(BC_D2I);
			currentFunction->bytecode()->addInsn(BC_SWAP);
		}
		switch (node->kind()) {
		case tAAND:
			currentFunction->bytecode()->addInsn(BC_IAAND);
			break;
		case tAOR:
			currentFunction->bytecode()->addInsn(BC_IAOR);
			break;
		case tAXOR:
			currentFunction->bytecode()->addInsn(BC_IAXOR);
			break;
		default:
			throw Error("Unsupported operation in bin operation node",
					node->position());
		}
		break;

		case tEQ:
		case tNEQ:
		case tGT:
		case tGE:
		case tLT:
		case tLE: {
			if (first == VT_DOUBLE) {
				currentFunction->bytecode()->addInsn(BC_D2I);
				lastExpressionType = VT_INT;
			}
			if (second == VT_DOUBLE) {
				currentFunction->bytecode()->addInsn(BC_SWAP);
				currentFunction->bytecode()->addInsn(BC_D2I);
				currentFunction->bytecode()->addInsn(BC_SWAP);
			}
			Label setFalse (currentFunction->bytecode());
			Label setTrue (currentFunction->bytecode());
			switch (node->kind()) {
			case tEQ:
				currentFunction->bytecode()->addBranch(BC_IFICMPE, setTrue);
				break;
			case tNEQ:
				currentFunction->bytecode()->addBranch(BC_IFICMPNE, setTrue);
				break;
			case tGT:
				currentFunction->bytecode()->addBranch(BC_IFICMPG, setTrue);
				break;
			case tGE:
				currentFunction->bytecode()->addBranch(BC_IFICMPGE, setTrue);
				break;
			case tLT:
				currentFunction->bytecode()->addBranch(BC_IFICMPL, setTrue);
				break;
			case tLE:
				currentFunction->bytecode()->addBranch(BC_IFICMPLE, setTrue);
				break;
			default:
				throw Error("Unsupported operation in bin operation node",
						node->position());
			}
			currentFunction->bytecode()->addInsn(BC_ILOAD0);
			currentFunction->bytecode()->addBranch(BC_JA, setFalse);
			currentFunction->bytecode()->bind(setTrue);
			currentFunction->bytecode()->addInsn(BC_ILOAD1);
			currentFunction->bytecode()->bind(setFalse);
			break;
		}
		case tADD:
		case tSUB:
		case tMUL:
		case tDIV:
		case tMOD: {
			VarType widest = (first == VT_DOUBLE || second == VT_DOUBLE) ?
					VT_DOUBLE : VT_INT;
			if (first != widest) {
				currentFunction->bytecode()->addInsn(BC_I2D);
				lastExpressionType = widest;
			}
			if (second != widest) {
				currentFunction->bytecode()->addInsn(BC_SWAP);
				currentFunction->bytecode()->addInsn(BC_I2D);
				currentFunction->bytecode()->addInsn(BC_SWAP);
			}
			switch (node->kind()){
			case tADD:
				currentFunction->bytecode()->addInsn(INSN_NUM(lastExpressionType, ADD));
				break;
			case tSUB:
				currentFunction->bytecode()->addInsn(INSN_NUM(lastExpressionType, SUB));
				break;
			case tMUL:
				currentFunction->bytecode()->addInsn(INSN_NUM(lastExpressionType, MUL));
				break;
			case tDIV:
				currentFunction->bytecode()->addInsn(INSN_NUM(lastExpressionType, DIV));
				break;
			case tMOD:
				if (lastExpressionType != VT_INT)
					throw Error("Not integer type in integer operation",
						node->position());
				currentFunction->bytecode()->addInsn(BC_IMOD);
				break;
			default:
				throw Error("Unsupported operation in bin operation node",
						node->position());
			}
			break;
		}
		default:
			throw Error("Unsupported operation in bin operation node",
					node->position());
	}
}


void TranslatorVisitor::visitBlockNode(BlockNode * node) {
	currentFunction->setLocalsNumber(
		currentFunction->localsNumber() + node->scope()->variablesCount());

	Scope::VarIterator vars(node->scope());
	while(vars.hasNext())
		currentScope->addVar(vars.next());

	Scope::FunctionIterator funcs(node->scope());
	while (funcs.hasNext())
		code->addFunction(new BytecodeFunction (funcs.next()));
	funcs = Scope::FunctionIterator(node->scope());
	while (funcs.hasNext())
		processFunction(funcs.next());
	for (uint32_t i = 0; i < node->nodes(); ++i)
		node->nodeAt(i)->visit(this);
}

void TranslatorVisitor::visitCallNode(CallNode * node) {
	TranslatedFunction * f = code->functionByName(node->name());
	if (!f)
		throw std::string("ERROR: function " + node->name());
	for (int i = node->parametersNumber() - 1; i >= 0; --i) {
		node->parameterAt(i)->visit(this);
		VarType paramType = f->parameterType(i);
		if (lastExpressionType == VT_INT && paramType == VT_DOUBLE)
			currentFunction->bytecode()->addInsn(BC_I2D);
		else if (lastExpressionType == VT_DOUBLE && paramType == VT_INT)
			currentFunction->bytecode()->addInsn(BC_D2I);
		else if (lastExpressionType == VT_STRING && paramType == VT_INT)
			currentFunction->bytecode()->addInsn(BC_S2I);
	}

	currentFunction->bytecode()->addInsn(BC_CALL);
	currentFunction->bytecode()->addInt16(f->id());
	if (f->returnType() != VT_VOID) lastExpressionType = f->returnType();
}

void TranslatorVisitor::visitDoubleLiteralNode(DoubleLiteralNode * node) {
	currentFunction->bytecode()->addInsn(BC_DLOAD);
	currentFunction->bytecode()->addDouble(node->literal());
	lastExpressionType = VT_DOUBLE;
}

void TranslatorVisitor::visitForNode(ForNode * node) {
	Label start(currentFunction->bytecode());
	Label end(currentFunction->bytecode());
	if (node->var()->type() != VT_INT)
		throw Error("Not Range type in for node expression",
				node->position());
	if (!node->inExpr()->isBinaryOpNode())
		throw Error("Not Range type in for node expression",
				node->position());
	BinaryOpNode *inExpr = node->inExpr()->asBinaryOpNode();
	if (inExpr->kind() != tRANGE)
		throw Error("Not Range type in for node expression",
				node->position());

	inExpr->left()->visit(this);
	if (lastExpressionType != VT_INT)
		throw Error("Not Range type in for node expression",
				node->position());
	storeVar(node->var());

	currentFunction->bytecode()->bind(start);
	inExpr->right()->visit(this);
	if (lastExpressionType != VT_INT)
		throw Error("Not Range type in for node expression",
				node->position());

	loadVar(node->var());

	currentFunction->bytecode()->addBranch(BC_IFICMPG, end);
	node->body()->visit(this);

	loadVar(node->var());
	currentFunction->bytecode()->addInsn(BC_ILOAD1);
	currentFunction->bytecode()->addInsn(BC_IADD);
	storeVar(node->var());
	currentFunction->bytecode()->addBranch(BC_JA, start);
	currentFunction->bytecode()->bind(end);
}

void TranslatorVisitor::visitFunctionNode(FunctionNode * node) {
	if (node->body()->nodeAt(0)->isNativeCallNode()) {
		node->body()->nodeAt(0)->visit(this);
	} else {
		node->body()->visit(this);
	}
}

void TranslatorVisitor::visitIfNode(IfNode * node) {
	Label ifElse (currentFunction->bytecode());
	Label ifEnd (currentFunction->bytecode());
	node->ifExpr()->visit(this);
	currentFunction->bytecode()->addInsn(BC_ILOAD0);
	currentFunction->bytecode()->addBranch(BC_IFICMPE, ifElse);
	node->thenBlock()->visit(this);
	currentFunction->bytecode()->addBranch(BC_JA, ifEnd);
	currentFunction->bytecode()->bind(ifElse);
	if(node->elseBlock())
		node->elseBlock()->visit(this);
	currentFunction->bytecode()->bind(ifEnd);
}

void TranslatorVisitor::visitIntLiteralNode(IntLiteralNode * node) {
	currentFunction->bytecode()->addInsn(BC_ILOAD);
	currentFunction->bytecode()->addInt64(node->literal());
	lastExpressionType = VT_INT;
}

void TranslatorVisitor::visitLoadNode(LoadNode * node) {
	loadVar(node->var());
}

void TranslatorVisitor::visitNativeCallNode(NativeCallNode * node) {
	//TODO;
}

void TranslatorVisitor::visitPrintNode(PrintNode * node) {
	for (uint32_t i = 0; i < node->operands(); ++i) {
		node->operandAt(i)->visit(this);
		currentFunction->bytecode()->addInsn(INSN(BC_, lastExpressionType, PRINT));
	}
}

void TranslatorVisitor::visitReturnNode(ReturnNode * node) {
	if (node->returnExpr()) {
		node->returnExpr()->visit(this);
		VarType returnType = currentFunction->returnType();
		if (lastExpressionType == VT_INT && returnType == VT_DOUBLE)
			currentFunction->bytecode()->addInsn(BC_I2D);
		else if (lastExpressionType == VT_DOUBLE && returnType == VT_INT)
			currentFunction->bytecode()->addInsn(BC_D2I);
		else if (lastExpressionType == VT_STRING && returnType == VT_INT)
			currentFunction->bytecode()->addInsn(BC_S2I);

	}
	currentFunction->bytecode()->addInsn(BC_RETURN);
}

void TranslatorVisitor::visitStoreNode(StoreNode * node) {
	node->value()->visit(this);
	switch(node->op()) {
	case tASSIGN:
		break;
	case tINCRSET:
		loadVar(node->var());
		currentFunction->bytecode()->addInsn(INSN_NUM(node->var()->type(), ADD));
		break;
	case tDECRSET:
		loadVar(node->var());
		currentFunction->bytecode()->addInsn(INSN_NUM(node->var()->type(), SUB));
		break;
	default:
		throw Error("Unsupported operation in store node",
				node->position());
	}
	storeVar(node->var());
}

void TranslatorVisitor::visitStringLiteralNode(StringLiteralNode * node) {
	currentFunction->bytecode()->addInsn(BC_SLOAD);
	currentFunction->bytecode()->addUInt16(code->makeStringConstant(node->literal()));
	lastExpressionType = VT_STRING;
}

void TranslatorVisitor::visitUnaryOpNode(UnaryOpNode * node) {
	node->operand()->visit(this);
	Label setFalse(currentFunction->bytecode());
	Label convEnd (currentFunction->bytecode());
	switch (node->kind()) {
	case tNOT:
		convertToBool();
		currentFunction->bytecode()->addInsn(BC_ILOAD1);
		currentFunction->bytecode()->addInsn(BC_ISUB);
		break;
	case tSUB:
		if (lastExpressionType != VT_INT &&
				lastExpressionType != VT_DOUBLE)
			throw Error("Math operation with not numeric types",
					node->position());
		currentFunction->bytecode()->addInsn(INSN_NUM(lastExpressionType,NEG));
		break;
	default:
		throw Error("Unsupported operation in unary operation node",
				node->position());
	}
}

void TranslatorVisitor::visitWhileNode(WhileNode * node) {
	Label start(currentFunction->bytecode());
	Label end (currentFunction->bytecode());
	currentFunction->bytecode()->bind(start);
	node->whileExpr()->visit(this);
	currentFunction->bytecode()->addInsn(BC_ILOAD0);
	currentFunction->bytecode()->addBranch(BC_IFICMPE, end);
	node->loopBlock()->visit(this);
	currentFunction->bytecode()->addBranch(BC_JA, start);
	currentFunction->bytecode()->bind(end);
}

//----------------------------------------------
Status * TranslatorVisitor::translate(const string &program, CodeImpl ** code) {
	Parser p;
	Status *s = p.parseProgram(program);
	if (s && s->isError()) return s;

	this->code = new CodeImpl;
	* code = this->code;
	currentScope = 0;
	lastExpressionType = VT_INVALID;
	try {
		processFunction(p.top());
	} catch (Error e) {
		return new Status(e.msg, e.pos);;
	}
	return 0;
}

void TranslatorVisitor::processFunction (AstFunction * f) {
	BytecodeFunction * fun = (BytecodeFunction *) code->functionByName(f->name());
	if (!fun) {
		fun = new BytecodeFunction(f);
		code->addFunction(fun);
	}
	BytecodeFunction * prevFunc = currentFunction;
	currentFunction = fun;
	currentScope = new ScopeVar(fun->id(), currentScope);

	for (Signature::const_iterator i = fun->signature().begin() + 1;
			i != fun->signature().end(); ++i) {
		AstVar * v = f->scope()->lookupVariable(i->second);
		currentScope->addVar(v);
		storeVar(v);
	}
	f->node()->visit(this);

	if (currentScope->parent == 0) currentFunction->bytecode()->addInsn(BC_STOP);
	ScopeVar * parent = currentScope->parent;
	delete currentScope;
	currentScope = parent;
	currentFunction = prevFunc;
}

void TranslatorVisitor::convertToBool() {
	if (lastExpressionType == VT_DOUBLE)
		currentFunction->bytecode()->addInsn(BC_D2I);
	else if (lastExpressionType == VT_STRING)
		currentFunction->bytecode()->addInsn(BC_S2I);
	Label setFalse(currentFunction->bytecode());
	Label convEnd(currentFunction->bytecode());
	currentFunction->bytecode()->addInsn(BC_ILOAD0);
	currentFunction->bytecode()->addBranch(BC_IFICMPE, setFalse);
	currentFunction->bytecode()->addInsn(BC_ILOAD1);
	currentFunction->bytecode()->addBranch(BC_JA, convEnd);
	currentFunction->bytecode()->bind(setFalse);
	currentFunction->bytecode()->addInsn(BC_ILOAD0);
	currentFunction->bytecode()->bind(convEnd);
}

void TranslatorVisitor::loadVar(const AstVar * var) {
	std::pair<uint16_t, uint16_t> scopeVar = currentScope->getVar(var);

	if(scopeVar.second == currentFunction->id()) {
		switch(scopeVar.first) {
		case 0:
			currentFunction->bytecode()->addInsn
				(INSN(BC_LOAD, var->type(), VAR0));
			break;
		case 1:
			currentFunction->bytecode()->addInsn
				(INSN(BC_LOAD, var->type(), VAR1));
			break;
		case 2:
			currentFunction->bytecode()->addInsn
				(INSN(BC_LOAD, var->type(), VAR2));
			break;
		case 3:
			currentFunction->bytecode()->addInsn
				(INSN(BC_LOAD, var->type(), VAR3));
			break;
		default:
			currentFunction->bytecode()->addInsn
				(INSN(BC_LOAD, var->type(), VAR));
			currentFunction->bytecode()->addUInt16(scopeVar.first);
		}
	} else {
		currentFunction->bytecode()->addInsn
					(INSN(BC_LOADCTX, var->type(), VAR));
		currentFunction->bytecode()->addUInt16(scopeVar.second);
		currentFunction->bytecode()->addUInt16(scopeVar.first);
	}
	lastExpressionType = var->type();
}

void TranslatorVisitor::storeVar(const AstVar * var) {
	std::pair<uint16_t, uint16_t> scopeVar = currentScope->getVar(var);

	if(scopeVar.second == currentFunction->id()) {
		switch(scopeVar.first) {
		case 0:
			currentFunction->bytecode()->addInsn
				(INSN(BC_STORE, var->type(), VAR0));
			break;
		case 1:
			currentFunction->bytecode()->addInsn
				(INSN(BC_STORE, var->type(), VAR1));
			break;
		case 2:
			currentFunction->bytecode()->addInsn
				(INSN(BC_STORE, var->type(), VAR2));
			break;
		case 3:
			currentFunction->bytecode()->addInsn
				(INSN(BC_STORE, var->type(), VAR3));
			break;
		default:
			currentFunction->bytecode()->addInsn
				(INSN(BC_STORE, var->type(), VAR));
			currentFunction->bytecode()->addUInt16(scopeVar.first);
		}
	} else {
		currentFunction->bytecode()->addInsn
					(INSN(BC_STORECTX, var->type(), VAR));
		currentFunction->bytecode()->addUInt16(scopeVar.second);
		currentFunction->bytecode()->addUInt16(scopeVar.first);
	}
}
}

