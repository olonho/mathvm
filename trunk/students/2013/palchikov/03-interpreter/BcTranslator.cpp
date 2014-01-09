#include "parser.h"
#include "BcTranslator.h"
#include "Errors.h"

BcTranslator::BcTranslator()
: Translator()
, AstVisitor()
, resultCode(0)
, retType(VT_VOID)
, resType(VT_VOID)
, currScope(0)
, customDataStorage()
, lastVarId(0)
, lastScopeId(0)
{}

BcTranslator::~BcTranslator()
{
	size_t count = customDataStorage.size();
	for (size_t i = 0; i < count; ++i) {
		delete customDataStorage[i];
	}
}

Status* BcTranslator::translate(const string& program, Code** code)
{
	Parser parser;
	Status* s = parser.parseProgram(program);
	if (s && s->isError()) {
		return s;
	}
	delete s;

	resultCode = new CodeImpl();

	try {
		visitFunctionDeclaration(parser.top());
	} catch (TranslateError& e) {
		delete resultCode;
		return new Status(e.what(), e.position());
	}
	
	*code = resultCode;

	return 0;
}

void BcTranslator::visitFunctionDeclaration(AstFunction* node)
{
	BytecodeFunction* bcFunction = new BytecodeFunction(node);
	resultCode->addFunction(bcFunction);

	Bytecode* saveBytecode = currBytecode;
	currBytecode = bcFunction->bytecode();

	CustomData* d = new CustomData(bcFunction->id(), 0);
	node->setInfo((void*)d);
	customDataStorage.push_back(d);

	lastVarId = 0;
	lastScopeId = bcFunction->id();

	currScope = node->scope();
	Scope::VarIterator params(currScope);
	while (params.hasNext()) {
		visitVariableDeclaration(params.next());
	}

	retType = node->returnType();

	node->node()->visit(this);

	bcFunction->setLocalsNumber(lastVarId);

	currBytecode = saveBytecode;
}

void BcTranslator::visitVariableDeclaration(AstVar* node)
{
	CustomData* d = new CustomData(lastVarId++, lastScopeId);
	node->setInfo((void*)d);
	customDataStorage.push_back(d);
}

// compare int @ TOS with 0 using cmp
#define IFNOT(cmp, elseLabel, endLabel) \
	Label elseLabel(currBytecode); \
	Label endLabel(currBytecode); \
	currBytecode->addInsn(BC_ILOAD0); \
	currBytecode->addBranch(cmp, elseLabel);
// add bytecode instructions (cmp false)
#define ELSE(elseLabel, endLabel) \
	currBytecode->addBranch(BC_JA, endLabel); \
	currBytecode->bind(elseLabel);
// add bytecode instructions (cmp true)
#define ENDIF(endLabel) \
	currBytecode->bind(endLabel);

#define IFLOGIC(node, err, elseLabel, endLabel) \
	node->visit(this); \
	if (!isNumType(resType)) { \
		throw TranslateError(err, node->position()); \
	} \
	if (resType == VT_DOUBLE) { \
		currBytecode->addInsn(BC_DLOAD0); \
		currBytecode->addInsn(BC_SWAP); \
		currBytecode->addInsn(BC_DCMP); \
		resType = VT_INT; \
	} \
	IFNOT(BC_IFICMPE, elseLabel, endLabel)

#define COMPARE(cmp) \
	visitNumBinOp(node); \
	if (resType == VT_DOUBLE) { \
		currBytecode->addInsn(BC_DLOAD0); \
		currBytecode->addInsn(BC_SWAP); \
		currBytecode->addInsn(BC_DCMP); \
		resType = VT_INT; \
	} \
	Label ltrue(currBytecode); \
	Label lend(currBytecode); \
	currBytecode->addBranch(cmp, ltrue); \
	currBytecode->addInsn(BC_ILOAD0); \
	ELSE(ltrue, lend) \
	currBytecode->addInsn(BC_ILOAD1); \
	ENDIF(lend) \
	resType = VT_INT;

void BcTranslator::visitBinaryOpNode(BinaryOpNode* node)
{
	TokenKind op = node->kind();

	switch (op) {
	case tAND: {
		IFLOGIC(node->left(), "invalid operand to && operator", else1, end1)
			IFLOGIC(node->right(), "invalid operand to && operator", else2, end2)
				currBytecode->addInsn(BC_ILOAD1);
			ELSE(else2, end2)
				currBytecode->addInsn(BC_ILOAD0);
			ENDIF(end2)
		ELSE(else1, end1)
			currBytecode->addInsn(BC_ILOAD0);
		ENDIF(end1)

		resType = VT_INT;

		break;
	}

	case tOR: {
		IFLOGIC(node->left(), "invalid operand to || operator", else1, end1)
			currBytecode->addInsn(BC_ILOAD1);
		ELSE(else1, end1)
			IFLOGIC(node->right(), "invalid operand to || operator", else2, end2)
				currBytecode->addInsn(BC_ILOAD1);
			ELSE(else2, end2)
				currBytecode->addInsn(BC_ILOAD0);
			ENDIF(end2)
		ENDIF(end1)

		resType = VT_INT;

		break;
	}

	case tAAND:
		visitIntBinOp(node);
		currBytecode->addInsn(BC_IAAND);
		break;
	case tAOR:
		visitIntBinOp(node);
		currBytecode->addInsn(BC_IAOR);
		break;
	case tAXOR:
		visitIntBinOp(node);
		currBytecode->addInsn(BC_IAXOR);
		break;

	case tEQ: {
		COMPARE(BC_IFICMPE)
		break;
	}
	case tNEQ: {
		COMPARE(BC_IFICMPNE)
		break;
	}
	case tLT: {
		COMPARE(BC_IFICMPL)
		break;
	}
	case tLE: {
		COMPARE(BC_IFICMPLE)
		break;
	}
	case tGT: {
		COMPARE(BC_IFICMPG)
		break;
	}
	case tGE: {
		COMPARE(BC_IFICMPGE)
		break;
	}

	case tADD:
		visitNumBinOp(node);
		if (resType == VT_INT) {
			currBytecode->addInsn(BC_IADD);
		} else {
			currBytecode->addInsn(BC_DADD);
		}
		break;
	case tSUB:
		visitNumBinOp(node);
		if (resType == VT_INT) {
			currBytecode->addInsn(BC_ISUB);
		} else {
			currBytecode->addInsn(BC_DSUB);
		}
		break;
	case tMUL:
		visitNumBinOp(node);
		if (resType == VT_INT) {
			currBytecode->addInsn(BC_IMUL);
		} else {
			currBytecode->addInsn(BC_DMUL);
		}
		break;
	case tDIV:
		visitNumBinOp(node);
		if (resType == VT_INT) {
			currBytecode->addInsn(BC_IDIV);
		} else {
			currBytecode->addInsn(BC_DDIV);
		}
		break;
	case tMOD:
		visitIntBinOp(node);
		currBytecode->addInsn(BC_IMOD);
		break;
	case tRANGE:
	default:
		assert(false);
	}
}

void BcTranslator::visitUnaryOpNode(UnaryOpNode* node)
{
	node->operand()->visit(this);

	switch (node->kind()) {
	case tNOT: {
		IFLOGIC(node->operand(), "invalid operand to ! operator", lelse, lend)
			currBytecode->addInsn(BC_ILOAD0);
		ELSE(lelse, lend)
			currBytecode->addInsn(BC_ILOAD1);
		ENDIF(lend)
		break;
	}
	case tSUB:
		if (resType == VT_INT) {
			currBytecode->addInsn(BC_INEG);
		} else if (resType == VT_DOUBLE) {
			currBytecode->addInsn(BC_DNEG);
		} else {
			throw TranslateError("invalid operand to negation operator", node->position());
		}
		break;

	default:
		assert(false);
	}
}

void BcTranslator::visitStringLiteralNode(StringLiteralNode* node)
{
	currBytecode->addInsn(BC_SLOAD);
	currBytecode->addUInt16(resultCode->makeStringConstant(node->literal()));
	resType = VT_STRING;
}

void BcTranslator::visitDoubleLiteralNode(DoubleLiteralNode* node)
{
	double val = node->literal();

	if (val == 0.0d) {
		currBytecode->addInsn(BC_DLOAD0);
	} else if (val == 1.0d) {
		currBytecode->addInsn(BC_DLOAD1);
	} else if (val == -1.0d) {
		currBytecode->addInsn(BC_DLOADM1);
	} else {
		currBytecode->addInsn(BC_DLOAD);
		currBytecode->addDouble(val);
	}

	resType = VT_DOUBLE;
}

void BcTranslator::visitIntLiteralNode(IntLiteralNode* node)
{
	int64_t val = node->literal();

	if (val == 0) {
		currBytecode->addInsn(BC_ILOAD0);
	} else if (val == 1) {
		currBytecode->addInsn(BC_ILOAD1);
	} else if (val == -1) {
		currBytecode->addInsn(BC_ILOADM1);
	} else {
		currBytecode->addInsn(BC_ILOAD);
		currBytecode->addInt64(val);
	}

	resType = VT_INT;
}

void BcTranslator::visitLoadNode(LoadNode* node)
{
	VarType type = node->var()->type();
	CustomData* data = (CustomData*)node->var()->info();
	uint16_t id = data->id;
	uint16_t scopeId = data->scopeId;

	if (type == VT_INT) {
		currBytecode->addInsn(BC_LOADCTXIVAR);
	} else if (type == VT_DOUBLE) {
		currBytecode->addInsn(BC_LOADCTXDVAR);
	} else if (type == VT_STRING) {
		currBytecode->addInsn(BC_LOADCTXSVAR);
	} else {
		assert(false);
	}
	currBytecode->addUInt16(scopeId);
	currBytecode->addUInt16(id);
	resType = type;
}

void BcTranslator::visitStoreNode(StoreNode* node)
{
	const AstVar* var = node->var();
	VarType varType = var->type();
	AstNode* value = node->value();
	TokenKind op = node->op();
	CustomData* data = (CustomData*)node->var()->info();
	uint16_t id = data->id;
	uint16_t scopeId = data->scopeId;

	value->visit(this);

	if (varType == VT_INT && resType == VT_DOUBLE) {
		convertNum(VT_DOUBLE, VT_INT);
	} else if (varType == VT_DOUBLE && resType == VT_INT) {
		convertNum(VT_INT, VT_DOUBLE);
	}

	switch (op) {
	case tASSIGN:
		if (varType == VT_INT && resType == VT_INT) {
			currBytecode->addInsn(BC_STORECTXIVAR);
		} else if (varType == VT_DOUBLE && resType == VT_DOUBLE) {
			currBytecode->addInsn(BC_STORECTXDVAR);
		} else if (varType == VT_STRING && resType == VT_STRING) {
			currBytecode->addInsn(BC_STORECTXSVAR);
		} else {
			throw TranslateError("assignment type mismatch", node->position());
		}
		currBytecode->addUInt16(scopeId);
		currBytecode->addUInt16(id);
		break;

	case tINCRSET:
	case tDECRSET:
		if (varType == VT_INT && resType == VT_INT) {
			currBytecode->addInsn(BC_LOADCTXIVAR);
		} else if (varType == VT_DOUBLE && resType == VT_DOUBLE) {
			currBytecode->addInsn(BC_LOADCTXDVAR);
		} else {
			throw TranslateError("assignment type mismatch", node->position());
		}
		currBytecode->addUInt16(scopeId);
		currBytecode->addUInt16(id);

		if (varType == VT_INT) {
			currBytecode->addInsn(op == tINCRSET ? BC_IADD : BC_ISUB);
			currBytecode->addInsn(BC_STORECTXIVAR);
		} else if (varType == VT_DOUBLE) {
			currBytecode->addInsn(op == tINCRSET ? BC_DADD : BC_DSUB);
			currBytecode->addInsn(BC_STORECTXDVAR);
		}
		currBytecode->addUInt16(scopeId);
		currBytecode->addUInt16(id);
		break;

	default:
		assert(false);
	}
}

void BcTranslator::visitForNode(ForNode* node)
{
	// initialization
	const AstVar* var = node->var();
	if (!var) {
		throw TranslateError("undeclared variable", node->position());
	}
	if (var->type() != VT_INT) {
		throw TranslateError("for loop counter must be integer", node->position());
	}

	BinaryOpNode* inExpr = node->inExpr()->asBinaryOpNode();

	inExpr->left()->visit(this);
	if (resType != VT_INT) {
		throw TranslateError("for range must have integer bounds", inExpr->left()->position());
	}

	CustomData* data = (CustomData*)var->info();
	uint16_t id = data->id;
	uint16_t scopeId = data->scopeId;

	currBytecode->addInsn(BC_STORECTXIVAR);
	currBytecode->addUInt16(scopeId);
	currBytecode->addUInt16(id);

	// loop
	Label loop(currBytecode);
	currBytecode->bind(loop);
	Label forBody(currBytecode);
	Label end(currBytecode);

	inExpr->right()->visit(this);
	if (resType != VT_INT) {
		throw TranslateError("for range must have integer bounds", inExpr->right()->position());
	}

	currBytecode->addInsn(BC_LOADCTXIVAR);
	currBytecode->addUInt16(scopeId);
	currBytecode->addUInt16(id);
	currBytecode->addBranch(BC_IFICMPLE, forBody);
	currBytecode->addBranch(BC_JA, end);
	currBytecode->bind(forBody);

	node->body()->visit(this);

	// increment
	currBytecode->addInsn(BC_LOADCTXIVAR);
	currBytecode->addUInt16(scopeId);
	currBytecode->addUInt16(id);
	currBytecode->addInsn(BC_ILOAD1);
	currBytecode->addInsn(BC_IADD);
	currBytecode->addInsn(BC_STORECTXIVAR);
	currBytecode->addUInt16(scopeId);
	currBytecode->addUInt16(id);

	currBytecode->addBranch(BC_JA, loop);
	currBytecode->bind(end);
}

void BcTranslator::visitWhileNode(WhileNode* node)
{
	Label loop(currBytecode);
	currBytecode->bind(loop);

	IFLOGIC(node->whileExpr(), "invalid while condition", lelse, lend)
		node->loopBlock()->visit(this);
		currBytecode->addBranch(BC_JA, loop);
	ELSE(lelse, lend)
	ENDIF(lend)
}

void BcTranslator::visitIfNode(IfNode* node)
{
	IFLOGIC(node->ifExpr(), "invalid if condition", lelse, lend)
		node->thenBlock()->visit(this);
	ELSE(lelse, lend)
		if (node->elseBlock()) {
			node->elseBlock()->visit(this);
		}
	ENDIF(lend)
}

void BcTranslator::visitBlockNode(BlockNode* node)
{
	currScope = node->scope();

	Scope::VarIterator vars(currScope);
	while (vars.hasNext()) {
		visitVariableDeclaration(vars.next());
	}

	Scope::FunctionIterator fns(currScope);
	while (fns.hasNext()) {
		visitFunctionDeclaration(fns.next());
	}

	uint32_t count = node->nodes();
	for (uint32_t i = 0; i < count; ++i) {
		node->nodeAt(i)->visit(this);
	}
}

void BcTranslator::visitFunctionNode(FunctionNode* node)
{
	node->body()->visit(this);
}

void BcTranslator::visitReturnNode(ReturnNode* node)
{
	if (node->returnExpr()) {
		node->returnExpr()->visit(this);
	}
	currBytecode->addInsn(BC_RETURN);
}

void BcTranslator::visitCallNode(CallNode* node)
{
	AstFunction* callee = currScope->lookupFunction(node->name());

	if (node->parametersNumber() != callee->parametersNumber()) {
		throw TranslateError("incorrect number of arguments", node->position());
	}

	Scope::VarIterator args(callee->scope());
	for (uint16_t i = 0; i < node->parametersNumber(); ++i) {
		node->parameterAt(i)->visit(this);
	
		AstVar* arg = args.next();
		VarType argType = arg->type();
		CustomData* data = (CustomData*)arg->info();
		uint16_t id = data->id;
		uint16_t scopeId = data->scopeId;

		if (argType == VT_INT && resType == VT_DOUBLE) {
			convertNum(VT_DOUBLE, VT_INT);
		} else if (argType == VT_DOUBLE && resType == VT_INT) {
			convertNum(VT_INT, VT_DOUBLE);
		}

		if (argType == VT_INT && resType == VT_INT) {
			currBytecode->addInsn(BC_STORECTXIVAR);
		} else if (argType == VT_DOUBLE && resType == VT_DOUBLE) {
			currBytecode->addInsn(BC_STORECTXDVAR);
		} else if (argType == VT_STRING && resType == VT_STRING) {
			currBytecode->addInsn(BC_STORECTXSVAR);
		} else {
			throw TranslateError("argument type mismatch", node->position());
		}
		currBytecode->addUInt16(scopeId);
		currBytecode->addUInt16(id);
	}

	uint16_t id = ((CustomData*)callee->info())->id;
	currBytecode->addInsn(BC_CALL);
	currBytecode->addUInt16(id);

	resType = callee->returnType();
}

void BcTranslator::visitNativeCallNode(NativeCallNode* node)
{
	// not implemented
}

void BcTranslator::visitPrintNode(PrintNode* node)
{
	uint32_t count = node->operands();
	for (uint32_t i = 0; i < count; ++i) {
		node->operandAt(i)->visit(this);

		switch (resType) {
		case VT_INT:
			currBytecode->addInsn(BC_IPRINT);
			break;
		case VT_DOUBLE:
			currBytecode->addInsn(BC_DPRINT);
			break;
		case VT_STRING:
			currBytecode->addInsn(BC_SPRINT);
			break;
		case VT_VOID:
			throw TranslateError("invalid operand to print operator", node->operandAt(i)->position());
		default:
			assert(false);
		}
	}
}

// Help methods

void BcTranslator::convertNum(VarType from, VarType to) {
	assert(isNumType(from) && isNumType(to));

	if (from == VT_INT && to == VT_DOUBLE) {
		currBytecode->addInsn(BC_I2D);
		resType = VT_DOUBLE;
	} else if (from == VT_DOUBLE && to == VT_INT) {
		currBytecode->addInsn(BC_D2I);
		resType = VT_INT;
	}
}

void BcTranslator::visitIntBinOp(BinaryOpNode* node)
{
	TokenKind op = node->kind();

	string err("invalid operand to ");
	switch (op) {
#define OP_CASE(token, str, prior) \
	case token: \
		err += str; \
		break;

	FOR_TOKENS(OP_CASE)
#undef OP_CASE
	default:
		break;
	}
	err += " operator";

	node->left()->visit(this);
	if (resType != VT_INT) {
		throw TranslateError(err, node->left()->position());
	}
	node->right()->visit(this);
	if (resType != VT_INT) {
		throw TranslateError(err, node->right()->position());
	}
}

void BcTranslator::visitNumBinOp(BinaryOpNode* node)
{
	TokenKind op = node->kind();

	string err("invalid operand to ");
	switch (op) {
#define OP_CASE(token, str, prior) \
	case token: \
		err += str; \
		break;

	FOR_TOKENS(OP_CASE)
#undef OP_CASE
	default:
		break;
	}
	err += " operator";

	node->right()->visit(this);
	VarType rightType = resType;
	if (!isNumType(rightType)) {
		throw TranslateError(err, node->right()->position());
	}
	node->left()->visit(this);
	VarType leftType = resType;
	if (!isNumType(leftType)) {
		throw TranslateError(err, node->left()->position());
	}

	if (leftType == VT_INT && rightType == VT_DOUBLE) {
		currBytecode->addInsn(BC_SWAP);
		convertNum(VT_INT, VT_DOUBLE);
		currBytecode->addInsn(BC_SWAP);
		resType = VT_DOUBLE;
	} else if (leftType == VT_DOUBLE && rightType == VT_INT) {
		convertNum(VT_INT, VT_DOUBLE);
		resType = VT_DOUBLE;
	}
}
