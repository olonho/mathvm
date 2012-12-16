#include "BytecodeGenerationVisitor.h"
#include <iostream>

BytecodeGenerationVisitor::BytecodeGenerationVisitor(InterpreterCodeImpl *code): code(code) {
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
	insnByToken[VT_DOUBLE][tADD] = BC_IADD;
	insnByToken[VT_DOUBLE][tSUB] = BC_ISUB;
	insnByToken[VT_DOUBLE][tMUL] = BC_IMUL;
	insnByToken[VT_DOUBLE][tDIV] = BC_IDIV;


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

	insnByToken[VT_INT][tQE] = BC_IFICMPE;
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

void BytecodeGenerationVisitor::visitBinaryOpNode(BinaryOpNode* node) { // add AND, OR
	Bytecode *bytecode = getBytecode();

	node -> right() -> visit(this);
	node -> left() -> visit(this);

	VarType firstType = types.top();
	types.pop();
	VarType secondType = types.top();
	types.pop();

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
		throw std::exception("Invalid operands types");
	}

	TokenKind op = node -> kind();
	switch (op) {
		case tRANGE:
			if (commonType == VT_INT) {
				types.push(secondType);
				types.push(firstType);
				return;
			} else {
				throw std::exception("Range bounds must integer");
			}

		case tADD:
		case tSUB:
		case tMUL:
		case tDIV:
		case tMOD:
			if (insnByToken[commonType].find(op) == insnByToken[commonType].end()) {
				throw std::exception("Can't apply given operation to operands");
			}
			bytecode -> addInsn(insnByToken[commonType][op]);
			types.push(commonType);
			break;

		case tAND:  // TODO
		case tOR:   // TODO
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
			types.push(VT_INT);
			break;
	}
}

void BytecodeGenerationVisitor::visitUnaryOpNode(UnaryOpNode* node) { // DONE
	Bytecode *bytecode = getBytecode();

	node -> operand() -> visit(this);

	switch (node -> kind()) {
		case tSUB:
			if (insnByUntypedInsn[types.top()].find(UT_NEG) == insnByUntypedInsn[types.top()].end()) {
				throw std::exception("SUB can be applied only to INT or DOUBLE");
			} else {
				bytecode -> addInsn(insnByUntypedInsn[types.top()][UT_NEG]);
			}
			break;
		case tNOT:
			if (types.top() == VT_INT) {
				notInt();
			} else if (types.top() == VT_DOUBLE) {
				notDouble();
				types.pop();
				types.push(VT_INT);
			} else {
				throw std::exception("NOT can be applied only to INT or DOUBLE");
			}
			break;
		default:
			throw std::exception("unknown unary operation");
	}
}

void BytecodeGenerationVisitor::notInt() {
	Bytecode *bytecode = getBytecode();

	bytecode -> addInsn(BC_ILODA0);
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

void BytecodeGenerationVisitor::notDouble() {
	Bytecode *bytecode = getBytecode();

	bytecode -> addInsn(BC_DLODA0);
	bytecode -> addInsn(BC_DCMP);
	bytecode -> addInsn(BC_ILODA0);
	bytecode -> addInsn(BC_IFICMPE);
	bytecode -> addInt16(7);
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_ILOAD0);
	bytecode -> addInsn(BC_JA);
	bytecode -> addInt16(4);
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_ILOAD1);
}

void BytecodeGenerationVisitor::visitStringLiteralNode(StringLiteralNode* node) {   // DONE
	Bytecode *bytecode = getBytecode();

	uint16_t strId = code -> makeStringConstant(node -> literal());
	bytecode -> addInsn(BC_SLOAD);
	bytecode -> addUInt16(strID);
	types.push(VT_STRING);
}

void BytecodeGenerationVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) { // DONE
	Bytecode *bytecode = getBytecode();
	bytecode -> addInsn(BC_DLOAD);
	bytecode -> addDouble(node -> literal());
	types.push(VT_DOUBLE);
}

void BytecodeGenerationVisitor::visitIntLiteralNode(IntLiteralNode* node) { // DONE
	Bytecode *bytecode = getBytecode();
	bytecode -> addInsn(BC_ILOAD);
	bytecode -> addInt64(node -> literal());
	types.push(VT_INT);
}

void BytecodeGenerationVisitor::visitLoadNode(LoadNode* node) { // DONE
	Bytecode *bytecode = getBytecode();
	uint16_t varId = getVarId(node -> var() -> name());
	VarType type = node -> var() -> type();

	bytecode -> addInsn(insnByUntypedInsn[type][UT_LOAD]);
	bytecode -> addUInt16(varId);

	types.push(type);
}

void BytecodeGenerationVisitor::visitStoreNode(StoreNode* node) {   // DONE
	Bytecode *bytecode = getBytecode();
	uint16_t varId = getVarId(node -> var() -> name());
	VarType type = node -> var() -> type();

	node -> value() -> visit(this);

	TokenKind op = node -> op();
	switch (op) {
		case tASSIGN:
			bytecode -> addInsn(insnByUntypedInsn[type][UT_STOREVAR]);
			bytecode -> addUInt16(varId);
		case tINCRSET:
			bytecode -> addInsn(insnByUntypedInsn[type][UT_LOADVAR]);
			bytecode -> addUInt16(varId);
			bytecode -> addInsn(insnByUntypedInsn[type][UT_ADD]);
			bytecode -> addInsn(insnByUntypedInsn[type][UT_STOREVAR]);
			bytecode -> addUInt16(varId);
		case tDECRSET:
			bytecode -> addInsn(insnByUntypedInsn[type][UT_LOADVAR]);
			bytecode -> addUInt16(varId);
			bytecode -> addInsn(insnByUntypedInsn[type][UT_SUB]);
			bytecode -> addInsn(insnByUntypedInsn[type][UT_STOREVAR]);
			bytecode -> addUInt16(varId);
	}
}

void BytecodeGenerationVisitor::visitForNode(ForNode* node) { // DONE
	Bytecode *bytecode = getBytecode();

	uint16_t varId = getVarId(node -> var() -> name());
	node -> inExpr() -> visit(this);

	// checking cycle condition
	int jumpIndex = bytecode() -> length();
	bytecode -> addInsn(BC_IFICMPG);
	int jumpOffsetIndex = bytecode() -> length();
	bytecode -> addInt16(0);

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
	bytecode -> addInt16(jumpIndex - (bytecode() -> length() - 1));

	// setting offset to exit from cycle
	int endIndex = bytecode -> length();
	bytecode -> setInt16(jumpOffsetIndex, endIndex - jumpIndex);

	// deleting cycle bounds parameters
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_POP);
	types.pop();
	types.pop();
}

void BytecodeGenerationVisitor::visitWhileNode(WhileNode* node) { // DONE
	Bytecode *bytecode = getBytecode();

	int startIndex = bytecode -> length();
	node -> whileExpr() -> visit(this);
	bytecode -> addInsn(BC_ILOAD0);
	int jumpIndex = bytecode -> length();
	bytecode -> addInsn(BC_IFICMPE);
	int jumpOffsetIndex = bytecode -> length();
	bytecode -> addInt16(0);

	removeConditionCheckingParams();

	node->loopBlock()->visit(this);
	int returnIndex = bytecode -> length();
	bytecode -> addInsn(BC_JA);
	bytecode -> addInt16(startIndex - returnIndex);

	int endIndex = bytecode -> length();
	bytecode -> setInt16(jumpOffsetIndex, endIndex - jumpIndex);

	removeConditionCheckingParams();
}

void BytecodeGenerationVisitor::visitIfNode(IfNode* node) {   // DONE
	Bytecode *bytecode = getBytecode();

	node -> ifExpr() -> visit(this);
	bytecode -> addInsn(BC_ILOAD0);
	int jumpFromIf = bytecode -> length();
	bytecode -> addInsn(BC_IFICMPE);
	int jumpFromIfOffset = bytecode -> length();
	bytecode -> addInt16(0);

	removeConditionCheckingParams();

	int thenBlockStartIndex = bytecode -> length();
	node->thenBlock()->visit(this);

	if ((node -> elseBlock()) != 0) {
		int jumpFromThen = bytecode -> length();
		bytecode -> addInsn(BC_JA);
		int jumpFromThenOffset = bytecode -> length();
		bytecode -> addInt16(0);

		bytecode -> setInt16(jumpFromIfOffset, (bytecode -> length()) - jumpFromIf);

		node -> elseBlock() -> visit(this);
		int elseBlockEndIndex = bytecode -> length();
		bytecode -> setInt16(jumpFromThenOffset, elseBlockEndIndex - jumpFromThen);
	} else {
		bytecode -> setInt16(jumpFromIfOffset, (bytecode -> length()) - jumpFromIf);
	}

	removeConditionCheckingParams();
}

void BytecodeGenerationVisitor::visitBlockNode(BlockNode* node) {   // DONE
	Scope::VarIterator varsToAdd(node -> scope());
	while (varsToAdd.hasNext()) {
		addVar(varsToAdd.next());
	}

	Scope::FunctionIterator functionsToAdd(node -> scope());
	while (functionsToAdd.hasNext()) {
		AstFunction *astFunction = functionsToAdd.next() -> node();
		BytecodeFunction *butecodeFunction = new BytecodeFunction(astFunction);
		code -> addFunction(bytecodeFunction);
	}

	Bytecode *bytecode = getBytecode();
	for (unsigned int i = 0; i < (node -> nodes()); i++) {
		int stackSize = types.size();
		node -> nodeAt(i) -> visit(this);

		assert(stackSize <= types.size());
		while (types.size() > stackSize) {
			bytecode -> addInsn(BC_POP);
			types.pop();
		}
	}

	// Scope::VarIterator varsToRemove(node -> scope());
	// while (varsToRemove.hasNext()) {
	//     removeVar();
	//     varsToRemove.next();
	// }
}

void BytecodeGenerationVisitor::visitFunctionNode(FunctionNode* node) {
	std::cout << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
	for (unsigned int i = 0; i < node->parametersNumber(); i++) {
		std::cout << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
		if (i != node->parametersNumber() - 1) {
			std::cout << ", ";
		}
	}
	std::cout << ") ";
	if (node->body()->nodeAt(0)->isNativeCallNode()) {
		node->body()->nodeAt(0)->visit(this);
		std::cout << ";" << std::endl;
		blockEnded = false;
	} else {
		std::cout << "{" << std::endl;
		node->body()->visit(this);
		std::cout << "}" << std::endl;
		blockEnded = true;
	}

	if (node -> returnType() != VT_VOID) {
		if (types.top() != (node -> returnType())) {
			if (types.top() == VT_INT && (node -> returnType()) == VT_DOUBLE) {
				bytecode -> addInsn(BC_I2D);
				types.pop();
				types.push(VT_DOUBLE);
			} else if (types.top() == VT_DOUBLE && (node -> returnType()) == VT_INT) {
				bytecode -> addInsn(BC_D2I);
				types.pop();
				types.push(VT_INT);
			} else {
				throw std::exception("Can't convert from type of return expression to function return type");
			}
		}
	}
}

void BytecodeGenerationVisitor::visitReturnNode(ReturnNode* node) { // DONE
	Bytecode *bytecode = getBytecode();

	if (node->returnExpr() != 0) {
		node -> returnExpr() -> visit(this);
	}
	bytecode -> addInsn(BC_RETURN);
}

void BytecodeGenerationVisitor::visitCallNode(CallNode* node) {
	std::cout << node->name() << "(";
	for (unsigned int i = 0; i < node->parametersNumber(); i++) {
		node->parameterAt(i)->visit(this);
		if (i != node->parametersNumber() - 1) {
			std::cout << ", ";
		}
	}
	std::cout << ")";
	blockEnded = false;
}

void BytecodeGenerationVisitor::visitNativeCallNode(NativeCallNode* node) {
	std::cout << "native '" << node->nativeName() << "'";
	blockEnded = false;
}

void BytecodeGenerationVisitor::visitPrintNode(PrintNode* node) {   // DONE
	for (unsigned int i = 0; i < node -> operands(); i++) {
		node -> operandAt(i) -> visit(this);
		bytecode -> addInsn(insnByUntypedInsn[types.top()][UT_PRINT]);
		types.pop();
	}
}


void BytecodeGenerationVisitor::removeConditionCheckingParams() {
	Bytecode *bytecode = getBytecode();
	// deleting zero constant and result of condition evaluation from stack
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_POP);
	// deleting type of condition evaluation result from types stack
	types.pop();
}

void BytecodeGenerationVisitor::compareInts(Instruction insn) {
	Bytecode *bytecode = getBytecode();

	bytecode -> addInsn(insn);
	bytecode -> addInt16(6);
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_ILOAD0);
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_ILOAD1);
}

void BytecodeGenerationVisitor::compareDoubles(Instruction insn) {
	Bytecode *bytecode = getBytecode();

	bytecode -> addInsn(BC_DCMP);
	bytecode -> addInsn(BC_SWAP);
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_SWAP);
	bytecode -> addInsn(BC_POP);
	bytecode -> addInsn(BC_ILOAD0);

	compareInts(insn);
}

Bytecode* BytecodeGenerationVisitor::getBytecode();

uint16_t BytecodeGenerationVisitor::getVarId(const string& name) {
	for (uint16_t i = vars.size() - 1; i >= 0; i--) {
		if (vars[i] -> name() == name) {
			return i;
		}
	}
	return -1;
}

void BytecodeGenerationVisitor::addVar(AstVar *var) {
	vars.push_back(var);
}

void BytecodeGenerationVisitor::removeVar() {
	vars.pop_back();
}
