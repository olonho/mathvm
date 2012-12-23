#include "BytecodeGenerationVisitor.h"
#include <iostream>


namespace mathvm {

BytecodeGenerationVisitor::BytecodeGenerationVisitor(InterpreterCodeImpl *code):
	currentScope(0), code(code) {
	fillInstructionsForInt();
	fillInstructionsForDouble();
	fillInstructionsForString();
}

void BytecodeGenerationVisitor::fillInstructionsForString() {
	insnByUntypedInsn[VT_STRING][UT_LOAD] = BC_SLOAD;
	insnByUntypedInsn[VT_STRING][UT_LOAD0] = BC_SLOAD0;
	insnByUntypedInsn[VT_STRING][UT_PRINT] = BC_SPRINT;

	insnByUntypedInsn[VT_STRING][UT_LOADVAR] = BC_LOADSVAR;
	insnByUntypedInsn[VT_STRING][UT_LOADVAR0] = BC_LOADSVAR0;
	insnByUntypedInsn[VT_STRING][UT_LOADVAR1] = BC_LOADSVAR1;
	insnByUntypedInsn[VT_STRING][UT_LOADVAR2] = BC_LOADSVAR2;
	insnByUntypedInsn[VT_STRING][UT_LOADVAR3] = BC_LOADSVAR3;
	insnByUntypedInsn[VT_STRING][UT_LOADCTXVAR] = BC_LOADCTXSVAR;

	insnByUntypedInsn[VT_STRING][UT_STOREVAR] = BC_STORESVAR;
	insnByUntypedInsn[VT_STRING][UT_STOREVAR0] = BC_STORESVAR0;
	insnByUntypedInsn[VT_STRING][UT_STOREVAR1] = BC_STORESVAR1;
	insnByUntypedInsn[VT_STRING][UT_STOREVAR2] = BC_STORESVAR2;
	insnByUntypedInsn[VT_STRING][UT_STOREVAR3] = BC_STORESVAR3;
	insnByUntypedInsn[VT_STRING][UT_STORECTXVAR] = BC_STORECTXSVAR;
}
void BytecodeGenerationVisitor::fillInstructionsForDouble() {
	insnByToken[VT_DOUBLE][tADD] = BC_DADD;
	insnByToken[VT_DOUBLE][tSUB] = BC_DSUB;
	insnByToken[VT_DOUBLE][tMUL] = BC_DMUL;
	insnByToken[VT_DOUBLE][tDIV] = BC_DDIV;


	insnByUntypedInsn[VT_DOUBLE][UT_LOAD] = BC_DLOAD;
	insnByUntypedInsn[VT_DOUBLE][UT_LOAD0] = BC_DLOAD0;
	insnByUntypedInsn[VT_DOUBLE][UT_LOAD1] = BC_DLOAD1;
	insnByUntypedInsn[VT_DOUBLE][UT_LOADM1] = BC_DLOADM1;
	insnByUntypedInsn[VT_DOUBLE][UT_ADD] = BC_DADD;
	insnByUntypedInsn[VT_DOUBLE][UT_SUB] = BC_DSUB;
	insnByUntypedInsn[VT_DOUBLE][UT_MUL] = BC_DMUL;
	insnByUntypedInsn[VT_DOUBLE][UT_DIV] = BC_DDIV;
	insnByUntypedInsn[VT_DOUBLE][UT_NEG] = BC_DNEG;
	insnByUntypedInsn[VT_DOUBLE][UT_PRINT] = BC_DPRINT;

	insnByUntypedInsn[VT_DOUBLE][UT_LOADVAR] = BC_LOADDVAR;
	insnByUntypedInsn[VT_DOUBLE][UT_LOADVAR0] = BC_LOADDVAR0;
	insnByUntypedInsn[VT_DOUBLE][UT_LOADVAR1] = BC_LOADDVAR1;
	insnByUntypedInsn[VT_DOUBLE][UT_LOADVAR2] = BC_LOADDVAR2;
	insnByUntypedInsn[VT_DOUBLE][UT_LOADVAR3] = BC_LOADDVAR3;
	insnByUntypedInsn[VT_DOUBLE][UT_LOADCTXVAR] = BC_LOADCTXDVAR;

	insnByUntypedInsn[VT_DOUBLE][UT_STOREVAR] = BC_STOREDVAR;
	insnByUntypedInsn[VT_DOUBLE][UT_STOREVAR0] = BC_STOREDVAR0;
	insnByUntypedInsn[VT_DOUBLE][UT_STOREVAR1] = BC_STOREDVAR1;
	insnByUntypedInsn[VT_DOUBLE][UT_STOREVAR2] = BC_STOREDVAR2;
	insnByUntypedInsn[VT_DOUBLE][UT_STOREVAR3] = BC_STOREDVAR3;
	insnByUntypedInsn[VT_DOUBLE][UT_STORECTXVAR] = BC_STORECTXDVAR;

	insnByUntypedInsn[VT_DOUBLE][UT_CMP] = BC_DCMP;
}

void BytecodeGenerationVisitor::fillInstructionsForInt() {
	insnByToken[VT_INT][tADD] = BC_IADD;
	insnByToken[VT_INT][tSUB] = BC_ISUB;
	insnByToken[VT_INT][tMUL] = BC_IMUL;
	insnByToken[VT_INT][tDIV] = BC_IDIV;
	insnByToken[VT_INT][tMOD] = BC_IMOD;

	insnByToken[VT_INT][tEQ] = BC_IFICMPE;
	insnByToken[VT_INT][tNEQ] = BC_IFICMPNE;
	insnByToken[VT_INT][tGT] = BC_IFICMPG;
	insnByToken[VT_INT][tGE] = BC_IFICMPGE;
	insnByToken[VT_INT][tLT] = BC_IFICMPL;
	insnByToken[VT_INT][tLE] = BC_IFICMPLE;

	insnByToken[VT_INT][tASSIGN] = BC_STOREIVAR;


	insnByUntypedInsn[VT_INT][UT_LOAD] = BC_ILOAD;
	insnByUntypedInsn[VT_INT][UT_LOAD0] = BC_ILOAD0;
	insnByUntypedInsn[VT_INT][UT_LOAD1] = BC_ILOAD1;
	insnByUntypedInsn[VT_INT][UT_LOADM1] = BC_ILOADM1;
	insnByUntypedInsn[VT_INT][UT_ADD] = BC_IADD;
	insnByUntypedInsn[VT_INT][UT_SUB] = BC_ISUB;
	insnByUntypedInsn[VT_INT][UT_MUL] = BC_IMUL;
	insnByUntypedInsn[VT_INT][UT_DIV] = BC_IDIV;
	insnByUntypedInsn[VT_INT][UT_MOD] = BC_IMOD;
	insnByUntypedInsn[VT_INT][UT_NEG] = BC_INEG;
	insnByUntypedInsn[VT_INT][UT_PRINT] = BC_IPRINT;

	insnByUntypedInsn[VT_INT][UT_LOADVAR] = BC_LOADIVAR;
	insnByUntypedInsn[VT_INT][UT_LOADVAR0] = BC_LOADIVAR0;
	insnByUntypedInsn[VT_INT][UT_LOADVAR1] = BC_LOADIVAR1;
	insnByUntypedInsn[VT_INT][UT_LOADVAR2] = BC_LOADIVAR2;
	insnByUntypedInsn[VT_INT][UT_LOADVAR3] = BC_LOADIVAR3;
	insnByUntypedInsn[VT_INT][UT_LOADCTXVAR] = BC_LOADCTXIVAR;

	insnByUntypedInsn[VT_INT][UT_STOREVAR] = BC_STOREIVAR;
	insnByUntypedInsn[VT_INT][UT_STOREVAR0] = BC_STOREIVAR0;
	insnByUntypedInsn[VT_INT][UT_STOREVAR1] = BC_STOREIVAR1;
	insnByUntypedInsn[VT_INT][UT_STOREVAR2] = BC_STOREIVAR2;
	insnByUntypedInsn[VT_INT][UT_STOREVAR3] = BC_STOREIVAR3;
	insnByUntypedInsn[VT_INT][UT_STORECTXVAR] = BC_STORECTXIVAR;

	insnByUntypedInsn[VT_INT][UT_CMP] = BC_ICMP;
}

void BytecodeGenerationVisitor::visitBinaryOpNode(BinaryOpNode* node) {
	node -> right() -> visit(this);
	VarType secondType = currentType;
	node -> left() -> visit(this);
	VarType firstType = currentType;

	VarType commonType;
	if (secondType == firstType && (firstType == VT_INT || firstType == VT_DOUBLE)) {
		commonType = firstType;
	} else if (firstType == VT_DOUBLE && secondType == VT_INT) {
		bytecode -> addInsn(BC_SWAP);
		bytecode -> addInsn(BC_I2D);
		bytecode -> addInsn(BC_SWAP);
		commonType = VT_DOUBLE;
	} else if (secondType == VT_DOUBLE && firstType == VT_INT) {
		bytecode -> addInsn(BC_I2D);
		commonType = VT_DOUBLE;
	} else {
		throw std::exception();
	}

	TokenKind op = node -> kind();
	switch (op) {
		case tRANGE:
			if (commonType == VT_INT) {
				currentType = VT_VOID;
				return;
			} else {
				throw std::exception();
			}

		case tADD:
		case tSUB:
		case tMUL:
		case tDIV:
		case tMOD: {
			if (insnByToken[commonType].find(op) == insnByToken[commonType].end()) {
				throw std::exception();
			}
			Instruction ins = insnByToken[commonType][op];
			bytecode -> addInsn(ins);
			currentType = commonType;
			break;
		}

		case tAND:
			if (commonType != VT_INT) {
				throw std::exception();
			}
			AND();
			currentType = VT_INT;
			break;
		case tOR:
			if (commonType != VT_INT) {
				throw std::exception();
			}
			OR();
			currentType = VT_INT;
			break;

		case tEQ:
		case tNEQ:
		case tGT:
		case tGE:
		case tLT:
		case tLE:
			if (commonType == VT_INT) {
				compareInts(insnByToken[VT_INT][op]);
			} else if (commonType == VT_DOUBLE) {
				compareDoubles(insnByToken[VT_INT][op]);
			}
			currentType = VT_INT;
			break;
		default:
			throw std::exception();
	}
}

void BytecodeGenerationVisitor::visitUnaryOpNode(UnaryOpNode* node) { // DONE
	node -> operand() -> visit(this);

	switch (node -> kind()) {
		case tSUB:
			if (insnByUntypedInsn[currentType].find(UT_NEG) == insnByUntypedInsn[currentType].end()) {
				throw std::exception();
			}
			bytecode -> addInsn(insnByUntypedInsn[currentType][UT_NEG]);
			break;
		case tNOT:
			if (currentType != VT_INT) {
				throw std::exception();
			}
			NOT();
			break;
		default:
			throw std::exception();
	}
}

void BytecodeGenerationVisitor::NOT() {
	bytecode -> addInsn(BC_ILOAD0);
	bytecode -> addInsn(BC_IFICMPE);
	bytecode -> addInt16(5);
	bytecode -> addInsn(BC_SWAP);
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_JA);
	bytecode -> addInt16(3);
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_ILOAD1);
}

void BytecodeGenerationVisitor::visitStringLiteralNode(StringLiteralNode* node) {   // DONE
	uint16_t strId = code -> makeStringConstant(node -> literal());
	bytecode -> addInsn(BC_SLOAD);
	bytecode -> addUInt16(strId);
	currentType = VT_STRING;
}

void BytecodeGenerationVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) { // DONE
	bytecode -> addInsn(BC_DLOAD);
	bytecode -> addDouble(node -> literal());
	currentType = VT_DOUBLE;
}

void BytecodeGenerationVisitor::visitIntLiteralNode(IntLiteralNode* node) { // DONE
	bytecode -> addInsn(BC_ILOAD);
	bytecode -> addInt64(node -> literal());
	currentType = VT_INT;
}

void BytecodeGenerationVisitor::visitLoadNode(LoadNode* node) { // DONE
	uint16_t varId = getVarId(node -> var());
	VarType type = node -> var() -> type();

	bytecode -> addInsn(insnByUntypedInsn[type][UT_LOADVAR]);
	bytecode -> addUInt16(varId);

	currentType = type;
}

void BytecodeGenerationVisitor::visitStoreNode(StoreNode* node) {   // DONE
	uint16_t varId = getVarId(node -> var());
	VarType type = node -> var() -> type();

	node -> value() -> visit(this);
	
	if (currentType == VT_INT && type == VT_DOUBLE) {
		bytecode->addInsn(BC_I2D);
	} else if (currentType == VT_DOUBLE && type == VT_INT) {
		bytecode->addInsn(BC_D2I);
	}

	TokenKind op = node -> op();
	switch (op) {
		case tASSIGN:
			bytecode -> addInsn(insnByUntypedInsn[type][UT_STOREVAR]);
			bytecode -> addUInt16(varId);
			break;
		case tINCRSET:
			bytecode -> addInsn(insnByUntypedInsn[type][UT_LOADVAR]);
			bytecode -> addUInt16(varId);
			bytecode -> addInsn(insnByUntypedInsn[type][UT_ADD]);
			bytecode -> addInsn(insnByUntypedInsn[type][UT_STOREVAR]);
			bytecode -> addUInt16(varId);
			break;
		case tDECRSET:
			bytecode -> addInsn(insnByUntypedInsn[type][UT_LOADVAR]);
			bytecode -> addUInt16(varId);
			bytecode -> addInsn(insnByUntypedInsn[type][UT_SUB]);
			bytecode -> addInsn(insnByUntypedInsn[type][UT_STOREVAR]);
			bytecode -> addUInt16(varId);
			break;
		default:
			throw std::exception();
	}
	
	currentType = VT_VOID;
}

void BytecodeGenerationVisitor::visitForNode(ForNode* node) { // DONE
	uint16_t varId = getVarId(node -> var());
	node -> inExpr() -> visit(this);

	// checking cycle condition
	int returnIndex = bytecode -> length();
	bytecode -> addInsn(BC_IFICMPG);
	int jumpOffsetIndex = bytecode -> length();
	bytecode -> addInt16(0);
	int jumpIndex = bytecode->length();

	// setting cycle variable value
	bytecode -> addInsn(BC_STOREIVAR);
	bytecode -> addUInt16(varId);
	bytecode -> addInsn(BC_LOADIVAR);
	bytecode -> addUInt16(varId);

	node -> body() -> visit(this);

	// updating cycle variable
	bytecode -> addInsn(BC_ILOAD1);
	bytecode -> addInsn(BC_IADD);

	// returning to the cycle begin
	bytecode -> addInsn(BC_JA);
	bytecode -> addInt16(returnIndex - (bytecode -> length() + 2));

	// setting offset to exit from cycle
	int endIndex = bytecode -> length();
	bytecode -> setInt16(jumpOffsetIndex, endIndex - jumpIndex);

	// deleting cycle bounds parameters
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_POP);
	currentType = VT_VOID;
}

void BytecodeGenerationVisitor::visitWhileNode(WhileNode* node) { // DONE
	int startIndex = bytecode -> length();
	node -> whileExpr() -> visit(this);
	bytecode -> addInsn(BC_ILOAD0);
	bytecode -> addInsn(BC_IFICMPE);
	int jumpOffsetIndex = bytecode -> length();
	bytecode -> addInt16(0);
	int jumpIndex = bytecode -> length();

	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_POP);

	node->loopBlock()->visit(this);
	bytecode -> addInsn(BC_JA);
	bytecode -> addInt16(startIndex - (bytecode->length() + 2));

	int endIndex = bytecode -> length();
	bytecode -> setInt16(jumpOffsetIndex, endIndex - jumpIndex);

	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_POP);
	currentType = VT_VOID;
}

void BytecodeGenerationVisitor::visitIfNode(IfNode* node) {   // DONE
	node -> ifExpr() -> visit(this);
	bytecode -> addInsn(BC_ILOAD0);
	bytecode -> addInsn(BC_IFICMPE);
	int jumpFromIfOffset = bytecode -> length();
	bytecode -> addInt16(0);
	int jumpFromIf = bytecode -> length();

	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_POP);
	
	node->thenBlock()->visit(this);
	
	bytecode -> addInsn(BC_JA);
	int jumpFromThenOffset = bytecode -> length();
	bytecode -> addInt16(0);
	int jumpFromThen = bytecode -> length();

	if ((node -> elseBlock()) != 0) {

		bytecode -> setInt16(jumpFromIfOffset, (bytecode -> length()) - jumpFromIf);

		bytecode->addInsn(BC_POP);
		bytecode->addInsn(BC_POP);
		node -> elseBlock() -> visit(this);
		int elseBlockEndIndex = bytecode -> length();
		bytecode -> setInt16(jumpFromThenOffset, elseBlockEndIndex - jumpFromThen);
	} else {
		bytecode -> setInt16(jumpFromIfOffset, (bytecode -> length()) - jumpFromIf);
		bytecode->addInsn(BC_POP);
		bytecode->addInsn(BC_POP);
		bytecode -> setInt16(jumpFromThenOffset, (bytecode -> length()) - jumpFromThen);
	}

	currentType = VT_VOID;
}

void BytecodeGenerationVisitor::visitBlockNode(BlockNode* node) {   // DONE
	Scope* prevScope = currentScope;
	currentScope = node->scope();
	
	Scope::VarIterator varsToSave(currentScope);
	while (varsToSave.hasNext()) {
		AstVar* v = varsToSave.next();
		bytecode->addInsn(insnByUntypedInsn[v->type()][UT_LOADVAR]);
		bytecode->addInt16(getVarId(v));
		locals.push_back(v);
	}
	
	Scope::VarIterator varsToInit(currentScope);
	while (varsToInit.hasNext()) {
		AstVar* v = varsToInit.next();
		bytecode->addInsn(insnByUntypedInsn[v->type()][UT_LOAD0]);
		bytecode->addInsn(insnByUntypedInsn[v->type()][UT_STOREVAR]);
		bytecode->addInt16(getVarId(v));
	}
	
	Scope::FunctionIterator functionsToAdd(node -> scope());
	while (functionsToAdd.hasNext()) {
		AstFunction *astFunction = functionsToAdd.next();
		BytecodeFunction *bytecodeFunction = new BytecodeFunction(astFunction);
		code -> addFunction(bytecodeFunction);
	}
	
	Scope::FunctionIterator functionsToBuild(node -> scope());
	while (functionsToBuild.hasNext()) {
		AstFunction *astFunction = functionsToBuild.next();
		astFunction->node()->visit(this);
	}

	for (unsigned int i = 0; i < (node -> nodes()); i++) {
		node -> nodeAt(i) -> visit(this);
		if (currentType != VT_VOID && !(node->nodeAt(i)->isReturnNode())) {
			bytecode -> addInsn(BC_POP);
		}
	}
	
	Scope::VarIterator varsToRestore(currentScope);
	stack<AstVar*> variables;
	while (varsToRestore.hasNext()) {
		variables.push(varsToRestore.next());
	}
	
	while (!variables.empty()) {
		AstVar* v = variables.top();
		bytecode->addInsn(insnByUntypedInsn[v->type()][UT_STOREVAR]);
		bytecode->addInt16(getVarId(v));
		locals.pop_back();
		variables.pop();
	}
	
	currentScope = prevScope;
	currentType = VT_VOID;
}

void BytecodeGenerationVisitor::visitFunctionNode(FunctionNode* node) {
	Bytecode* currentBytecode = bytecode;
	vector<const AstVar*> prevLocals = locals;
	locals.clear();
	
	if (code->functionByName(node->name()) == 0) {
		AstFunction* astFunction = node->body()->scope()->lookupFunction(node->name());
		BytecodeFunction *bytecodeFunction = new BytecodeFunction(astFunction);
		code->addFunction(bytecodeFunction);
		bytecode = bytecodeFunction->bytecode();
	} else {
		bytecode = ((BytecodeFunction*)(code->functionByName(node->name())))->bytecode();
	}
	
	VarType prevReturnType = returnType;
	returnType = node -> returnType();
	
	if (node->body()->nodeAt(0)->isNativeCallNode()) {
		node->body()->nodeAt(0)->visit(this);	// TODO
	} else {
		node->body()->visit(this);
	}
	
	locals = prevLocals;
	returnType = prevReturnType;
	bytecode = currentBytecode;
}

void BytecodeGenerationVisitor::visitReturnNode(ReturnNode* node) { // DONE
	bool swap = false;
	if (node->returnExpr() != 0) {
		swap = true;
		node -> returnExpr() -> visit(this);
		if (currentType == VT_INT && returnType == VT_DOUBLE) {
			bytecode->addInsn(BC_I2D);
		} else if (currentType == VT_DOUBLE && returnType == VT_INT) {
			bytecode->addInsn(BC_D2I);
		} else if (currentType != returnType){
			throw std::exception();
		}
	}
	
	for (int i = locals.size() - 1; i >= 0; i--) {
		if (swap) {
			bytecode->addInsn(BC_SWAP);
		}
		const AstVar* v = locals[i];
		bytecode->addInsn(insnByUntypedInsn[v->type()][UT_STOREVAR]);
		bytecode->addInt16(getVarId(v));
	}
	
	bytecode -> addInsn(BC_RETURN);
	currentType = VT_VOID;
}

void BytecodeGenerationVisitor::visitCallNode(CallNode* node) {
	const Signature& sign = code->functionByName(node->name())->signature();
	AstFunction* f = currentScope->lookupFunction(node->name());
	
	for (unsigned int i = 0; i < node->parametersNumber(); i++) {
		AstVar* v = f->scope()->lookupVariable(sign[i + 1].second);
		uint16_t varId = getVarId(v);
		bytecode->addInsn(insnByUntypedInsn[v->type()][UT_LOADVAR]);
		bytecode->addInt16(varId);
	}
	
	for (unsigned int i = 0; i < node->parametersNumber(); i++) {
		node->parameterAt(i)->visit(this);
		if (currentType == VT_INT && sign[i + 1].first == VT_DOUBLE) {
			bytecode->addInsn(BC_I2D);
		} else if (currentType == VT_DOUBLE && sign[i + 1].first == VT_INT) {
			bytecode->addInsn(BC_D2I);
		}
	}
	
	for (int i = node->parametersNumber(); i > 0; i--) {
		AstVar* v = f->scope()->lookupVariable(sign[i].second);
		uint16_t varId = getVarId(v);
		bytecode->addInsn(insnByUntypedInsn[sign[i].first][UT_STOREVAR]);
		bytecode->addInt16(varId);
	}
	
	bytecode->addInsn(BC_CALL);
	bytecode->addInt16(code->functionByName(node->name())->id());
	
	if (sign[0].first == VT_VOID) {
		for (unsigned int i = node->parametersNumber(); i > 0; i--) {
			AstVar* v = f->scope()->lookupVariable(sign[i].second);
			uint16_t varId = getVarId(v);
			bytecode->addInsn(insnByUntypedInsn[v->type()][UT_STOREVAR]);
			bytecode->addInt16(varId);
		}
	} else {
		for (unsigned int i = node->parametersNumber(); i > 0; i--) {
			AstVar* v = f->scope()->lookupVariable(sign[i].second);
			uint16_t varId = getVarId(v);
			bytecode->addInsn(BC_SWAP);
			bytecode->addInsn(insnByUntypedInsn[v->type()][UT_STOREVAR]);
			bytecode->addInt16(varId);
		}
	}
	
	currentType = sign[0].first;
}

void BytecodeGenerationVisitor::visitNativeCallNode(NativeCallNode* node) {
//	std::cout << "native '" << node->nativeName() << "'";
//	blockEnded = false;
}

void BytecodeGenerationVisitor::visitPrintNode(PrintNode* node) {   // DONE
	for (unsigned int i = 0; i < node -> operands(); i++) {
		node -> operandAt(i) -> visit(this);
		bytecode -> addInsn(insnByUntypedInsn[currentType][UT_PRINT]);
	}
	currentType = VT_VOID;
}

void BytecodeGenerationVisitor::compareInts(Instruction insn) {
	bytecode -> addInsn(insn);
	bytecode -> addInt16(6);
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_ILOAD0);
	bytecode -> addInsn(BC_JA);
	bytecode -> addInt16(3);
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_ILOAD1);
}

void BytecodeGenerationVisitor::compareDoubles(Instruction insn) {
	bytecode -> addInsn(BC_DCMP);
	bytecode -> addInsn(BC_ILOAD0);

	compareInts(insn);
}

uint16_t BytecodeGenerationVisitor::getVarId(const AstVar *var) {
	if (vars.find(var) == vars.end()) {
		uint16_t id = (uint16_t) vars.size();
		vars[var] = id;
	}
	return vars[var];
}

void BytecodeGenerationVisitor::OR() {
	bytecode->addInsn(BC_ILOAD0);
	bytecode->addInsn(BC_IFICMPNE);
	bytecode->addInt16(10);
	
	bytecode->addInsn(BC_SWAP);
	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_IFICMPNE);
	bytecode->addInt16(6);
	
	bytecode->addInsn(BC_SWAP);
	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_JA);
	bytecode->addInt16(4);
	
	
	bytecode->addInsn(BC_POP);

	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_ILOAD1);
}

void BytecodeGenerationVisitor::AND() {
	bytecode->addInsn(BC_ILOAD0);
	bytecode->addInsn(BC_IFICMPE);
	bytecode->addInt16(11);
	
	bytecode->addInsn(BC_SWAP);
	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_IFICMPE);
	bytecode->addInt16(7);
	
	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_ILOAD1);
	bytecode->addInsn(BC_JA);
	bytecode->addInt16(4);
	
	
	bytecode->addInsn(BC_POP);

	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_ILOAD0);
}

}

