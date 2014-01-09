#include "CompilerVisitor.h"

#include <stdexcept>

#include "VarsUtil.h"

#define PUSH_IEAX BC_LOADIVAR0

#define PUSH_EBP  BC_LOADIVAR1
#define POP_EBP   BC_STOREIVAR1

#define PUSH_ESP  BC_LOADIVAR2
#define POP_ESP   BC_STOREIVAR2

// only int kinds
#define FOR_BIN_INT_OPNODE_KINDS(DO) \
	DO(AOR)  \
	DO(AAND) \
	DO(AXOR) \
	DO(MOD)

// common kinds
#define FOR_BIN_OPNODE_KINDS(DO) \
	DO(ADD)  \
	DO(SUB)  \
	DO(MUL)  \
	DO(DIV)

#define FOR_IF_OPNODE_KINDS(DO) \
	DO(EQ,  BC_IFICMPE,  BC_IFICMPNE) \
	DO(NEQ, BC_IFICMPNE, BC_IFICMPE)  \
	DO(GT,  BC_IFICMPG,  BC_IFICMPLE) \
	DO(GE,  BC_IFICMPGE, BC_IFICMPL)  \
	DO(LT,  BC_IFICMPL,  BC_IFICMPGE) \
	DO(LE,  BC_IFICMPLE, BC_IFICMPG)

#define FOR_IF_OPNODE_AND_OR(DO) \
	DO(AND) \
	DO(OR)  


CompilerVisitor::CompilerVisitor():elseLabel_(0), thenLabel_(0), parent_(0)
{
	bytecodes_.reserve(4096);
	callStacks_.reserve(4096);
}


void CompilerVisitor::visitStartFunction(AstFunction* f, const map<AstFunction*, set<pair<VarType, string> > >& _captured)
{
	captured_ = _captured;
	lastAstFunction_ = f;
	visitFunctionNode(f->node());
}


const vector<Bytecode>& CompilerVisitor::bytecodes() const
{
	return bytecodes_;
}


const vector<string>& CompilerVisitor::literals() const
{
	return stringLiterals;
}


void CompilerVisitor::visitFunctionNode(FunctionNode* function)
{
	int tmpId;
	lastFunction_ = bytecodes_.size();
	tmpId = lastFunction_;
	functionId_[function->name()] = lastFunction_;
	//cout << lastFunction_ << ":" << function->name() << endl;

	bytecodes_.push_back(Bytecode());
	callStacks_.push_back(StackLayout(function->signature(), function->name(), lastFunction_, captured_[lastAstFunction_]));
	parent_ = &callStacks_.back();

	Bytecode& bc = bytecodes_[lastFunction_];
	// save context
	bc.add(PUSH_EBP);	// push(ebp) - save ebp
	bc.add(PUSH_ESP);	// mov(ebp, esp)
	bc.add(POP_EBP);

	// compile body
	function->body()->visit(this);

	if (tmpId == 0)
		bc.add(BC_STOP);
}


void CompilerVisitor::visitBlockNode(BlockNode* block)
{
	int tmpLastFunction = lastFunction_;
	Bytecode&    bc = bytecodes_[lastFunction_];
	StackLayout& sl = callStacks_[lastFunction_];

	scopeStack_.push_back(block->scope());

	// push local variables to stack
	int parametersSize = 0;
	Scope::VarIterator varIt(block->scope());
	while(varIt.hasNext())
	{
		AstVar* var = varIt.next();
		load0(&bc, var->type());

		parametersSize += sizeOfType(var->type());
	}
	// add local variable to stack layout
	sl.addLocalVars(block->scope());

	// process body
	lastFunction_ = tmpLastFunction;
	bc = bytecodes_[lastFunction_];
	sl = callStacks_[lastFunction_];

	block->visitChildren(this);

	// remove all local variables from stack
	//varIt = block->scope();
	//while(varIt.hasNext())
	//{
	//	AstVar* var = varIt.next();
	//	bc.add(BC_POP);
	//}
	// remove all local variables from layout

	// hm =/
	bc.add(PUSH_ESP);
	bc.add(BC_ILOAD);
	bc.addInt64(parametersSize);
	bc.add(BC_ISUB);
	bc.add(POP_ESP);
	sl.remLocalVars(block->scope()->variablesCount());

	scopeStack_.pop_back();
}


void CompilerVisitor::visitBinaryOpNode(BinaryOpNode* opNode)
{
	Bytecode&    bc = bytecodes_[lastFunction_];
	StackLayout& sl = callStacks_[lastFunction_];

	if (opNode->kind() == tAND || opNode->kind() == tOR)
		lastAnd_ = isNot_ ? opNode->kind() != tAND : opNode->kind() == tAND; 
	// right on top
	opNode->left()->visit(this);

	if (opNode->kind() == tAND || opNode->kind() == tOR)
		lastAnd_ = isNot_ ? opNode->kind() != tAND : opNode->kind() == tAND; 

	opNode->right()->visit(this);

	if (opNode->kind() == tAND || opNode->kind() == tOR) return;

	const VarID* fst = sl.first();
	const VarID* snd = sl.second();
	if (!fst || !snd)
		throw std::invalid_argument("invalid args count in binary operator");
	// can convert only topmost value
	VarType sndType = snd->first;
	VarType fstType = fst->first;
	if (sndType == VT_STRING || fstType == VT_STRING)
		throw std::invalid_argument("no binary operator with strings is available");
	if (fstType != sndType)
	{
		convert(&bc, fstType, sndType);
		sl.popLocalVar();
		sl.pushLocalVar(make_pair(sndType, ""));
	}

	if (sndType == VT_INT)
		visitIntBinOpNode(opNode);
	else if (sndType == VT_DOUBLE)
		visitDoubleBinOpNode(opNode);
	else
		assert(true);
}


void CompilerVisitor::visitCallNode(CallNode* callNode)
{
	Bytecode&    bc = bytecodes_[lastFunction_];
	StackLayout& sl = callStacks_[lastFunction_];

	// compile function
	int tmpLastFunction = lastFunction_;
	StackLayout* tmpParent = parent_;
	
	AstFunction* f = scopeStack_.back()->lookupFunction(callNode->name());
	if (already_.find(f) == already_.end())
	{
		already_.insert(f);
		lastAstFunction_ = f;
		f->node()->visit(this);
	}
	
	parent_ = tmpParent;
	lastFunction_ = tmpLastFunction;

	int parametersSize = 0;
	// push captured pointers
	set<pair<VarType, string> >::reverse_iterator cit;
	for (cit = captured_[f].rbegin(); cit != captured_[f].rend(); ++cit)
		loadVar(&bc, cit->first, sl.getLoadOffsetAsPtr(*cit));
	parametersSize += PointerSize * captured_[f].size();

	// check parameters
	if (callNode->parametersNumber() != f->node()->signature().size() - 1)
		throw std::invalid_argument("bad parameters count");

	for (size_t i = 0; i < callNode->parametersNumber(); ++i)
	{
		callNode->parameterAt(i)->visit(this);
		VarType from = sl.first()->first;
		VarType to	 = f->node()->signature()[i + 1].first; 
		if(from != to)
		{
			convert(&bc, from, to); 
			sl.popLocalVar();
			sl.pushLocalVar(make_pair(to, ""));
		}
		parametersSize += sizeOfType(to);
	}

	bc.add(BC_CALL);
	bc.addInt16(functionId_[callNode->name()]);

	bc.add(PUSH_ESP);
	bc.add(BC_ILOAD);
	bc.addInt64(parametersSize);
	bc.add(BC_ISUB);
	bc.add(POP_ESP);

	VarType returnType = f->node()->signature().front().first;
	if (returnType != VT_VOID)
	{
		pushFromVar0(&bc, returnType);
		sl.pushLocalVar(make_pair(returnType, ""));
	}
}


void CompilerVisitor::visitDoubleLiteralNode(DoubleLiteralNode* doubleLiteral)
{
	Bytecode&    bc = bytecodes_[lastFunction_];
	StackLayout& sl = callStacks_[lastFunction_];

	bc.add(BC_DLOAD);
	bc.addDouble(doubleLiteral->literal());

	sl.pushLocalVar(make_pair(VT_DOUBLE, ""));
}


void CompilerVisitor::visitForNode(ForNode* forNode)
{
	Bytecode&    bc = bytecodes_[lastFunction_];

	// prepare variable
	const AstVar* var = forNode->var();

	const BinaryOpNode* binaryNode = forNode->inExpr()->asBinaryOpNode();
	if (binaryNode == 0 || binaryNode->kind() != tRANGE)
		throw std::logic_error("not range in for");

	// init counter
	StoreNode fakeStore(0, var, binaryNode->left(), tASSIGN);
	fakeStore.visit(this);

	LoadNode fakeLoad(0, var);
	BinaryOpNode fakeEndCheck(0, tLE, &fakeLoad, binaryNode->right());

	Label beforeCheck(&bc);
	Label elseLabel(&bc);
	Label thenLabel(&bc);
	
	elseLabel_ = &elseLabel;
	thenLabel_ = &thenLabel;
	
	isNot_   = false;
	lastAnd_ = true;

	bc.bind(beforeCheck);
	// if (var <= range.right)
	fakeEndCheck.visit(this);
	bc.bind(thenLabel);

	// for body
	forNode->body()->visit(this);

	// if (right > left) counter += 1 else counter -= 1;
	IntLiteralNode one(0, 1);
	StoreNode fakeInc(0, var, &one, tINCRSET);
	Scope empty(forNode->body()->scope());
	BlockNode blockInc(0, &empty);
	blockInc.add(&fakeInc);
	StoreNode fakeDec(0, var, &one, tDECRSET);
	BlockNode blockDec(0, &empty);
	blockDec.add(&fakeDec);
	BinaryOpNode checkDir(0, tLT, binaryNode->left(), binaryNode->right());
	IfNode fakeChangeCntr(0, static_cast<AstNode*>(&checkDir), &blockInc, &blockDec);
	fakeChangeCntr.visit(this);

	bc.addBranch(BC_JA, beforeCheck);

	bc.bind(elseLabel);
}


void CompilerVisitor::visitIfNode(IfNode* ifNode)
{
	Bytecode&    bc = bytecodes_[lastFunction_];

	Label elseLabel(&bc);
	Label thenLabel(&bc);
	Label afterLabel(&bc);
	elseLabel_ = &elseLabel;
	thenLabel_ = &thenLabel;
	
	isNot_   = false;
	lastAnd_ = true;

	ifNode->ifExpr()->visit(this);
	
	if (!lastAnd_)
		bc.addBranch(BC_JA, afterLabel);
	
	bc.bind(thenLabel);

	ifNode->thenBlock()->visit(this);
	bc.addBranch(BC_JA, afterLabel);

	bc.bind(elseLabel);
	if (ifNode->elseBlock())
		ifNode->elseBlock()->visit(this);
	
	bc.bind(afterLabel);

	elseLabel_ = 0;
	thenLabel_ = 0;
}


void CompilerVisitor::visitIntLiteralNode(IntLiteralNode* intLiteral)
{
	Bytecode&    bc = bytecodes_[lastFunction_];
	StackLayout& sl = callStacks_[lastFunction_];

	bc.add(BC_ILOAD);
	bc.addInt64(intLiteral->literal());

	sl.pushLocalVar(make_pair(VT_INT, ""));
}


void CompilerVisitor::visitLoadNode(LoadNode* loadNode)
{
	Bytecode&    bc = bytecodes_[lastFunction_];
	StackLayout& sl = callStacks_[lastFunction_];

	const AstVar* var = loadNode->var();
	uint32_t offset = sl.getLoadOffsetAsVal(make_pair(var->type(), var->name()));
	
	loadVar(&bc, var->type(), offset);
	sl.pushLocalVar(make_pair(var->type(), ""));
}


void CompilerVisitor::visitNativeCallNode(NativeCallNode* nativeCall)
{
	Bytecode&    bc = bytecodes_[lastFunction_];

	// todo: implement parameters cast
	// todo: implement more info in literal

	nativeCall->visitChildren(this);

	bc.add(BC_CALLNATIVE);

	int id = stringLiterals.size();
	stringLiterals.push_back(nativeCall->nativeName());

	bc.addInt16(static_cast<int16_t>(id));
}


void CompilerVisitor::visitPrintNode(PrintNode* printNode)
{
	Bytecode&    bc = bytecodes_[lastFunction_];
	StackLayout& sl = callStacks_[lastFunction_];

	for (size_t i = 0; i < printNode->operands(); ++i)
	{
		printNode->operandAt(i)->visit(this);
		const VarID* id = sl.first();
		if (!id)
			throw std::invalid_argument("empty stack");
		print(&bc, id->first);
		sl.popLocalVar();
	}
}


void CompilerVisitor::visitReturnNode(ReturnNode* returnNode)
{
	Bytecode&    bc = bytecodes_[lastFunction_];
	StackLayout& sl = callStacks_[lastFunction_];

	if (returnNode->returnExpr())
	{
		returnNode->returnExpr()->visit(this);
		const VarID* id = sl.first();
		if (!id)
			throw std::invalid_argument("empty stack");

		VarType returnType = sl.signature().front().first;
		if (returnType != id->first)
			convert(&bc, id->first, returnType);
		popToVar0(&bc, returnType);
		sl.popLocalVar();
	}
	bc.add(PUSH_EBP);	// mov(esp, ebp)
	bc.add(POP_ESP);
	bc.add(POP_EBP);	// pop(ebp) - restore ebp
	bc.add(BC_RETURN);
}


void CompilerVisitor::visitStoreNode(StoreNode* storeNode)
{
	Bytecode&    bc = bytecodes_[lastFunction_];
	StackLayout& sl = callStacks_[lastFunction_];

	if (storeNode->op() == tINCRSET || storeNode->op() == tDECRSET)
	{
		LoadNode fakeLoad(0, storeNode->var());
		BinaryOpNode fakeNode(0, storeNode->op() == tINCRSET ? tADD : tSUB, &fakeLoad, storeNode->value());
		fakeNode.visit(this);
	}
	else
		storeNode->value()->visit(this);

	const AstVar* var = storeNode->var();
	const VarID* fst = sl.first();
	if (!fst)
		throw std::invalid_argument("empty stack");
	if (fst->first != var->type())
		throw std::invalid_argument("bad topmost type");

	uint32_t offset = sl.getStoreOffset(make_pair(var->type(), var->name()));
	storeVar(&bc, var->type(), offset);

	sl.popLocalVar();
}


void CompilerVisitor::visitStringLiteralNode(StringLiteralNode* stringLiteral)
{
	Bytecode&    bc = bytecodes_[lastFunction_];
	StackLayout& sl = callStacks_[lastFunction_];

	int16_t id = static_cast<int16_t>(stringLiterals.size());
	stringLiterals.push_back(stringLiteral->literal());

	// in runtime: id -> real pointer
	bc.add(BC_SLOAD);
	bc.addInt16(id);

	sl.pushLocalVar(make_pair(VT_STRING, ""));
}


void CompilerVisitor::visitUnaryOpNode(UnaryOpNode* opNode)
{
	Bytecode&    bc = bytecodes_[lastFunction_];
	StackLayout& sl = callStacks_[lastFunction_];

	if (opNode->kind() == tNOT)
	{
		isNot_ = !isNot_;
		opNode->operand()->visit(this);
	}
	else
	{
		opNode->operand()->visit(this);

		const VarID* fst = sl.first();
		if (!fst)
			throw std::invalid_argument("empty stack");
		if (fst->first != VT_INT && fst->first != VT_DOUBLE)
			throw std::invalid_argument("bad type");

		if (opNode->kind() == tSUB)
		{
			if (fst->first == VT_INT)
			{
				bc.add(BC_ILOADM1);
				sl.pushLocalVar(make_pair(VT_INT, ""));
				bc.add(BC_IMUL);
				sl.popLocalVar();
				sl.popLocalVar();
				sl.pushLocalVar(make_pair(VT_INT, ""));
			}
			else
			{
				bc.add(BC_DLOADM1);
				sl.pushLocalVar(make_pair(VT_DOUBLE, ""));
				bc.add(BC_DMUL);
				sl.popLocalVar();
				sl.popLocalVar();
				sl.pushLocalVar(make_pair(VT_DOUBLE, ""));
			}
		}
	}
}


void CompilerVisitor::visitWhileNode(WhileNode* whileNode)
{
	Bytecode&    bc = bytecodes_[lastFunction_];

	Label beforeLabel(&bc);
	Label elseLabel(&bc);
	Label thenLabel(&bc);
	elseLabel_ = &elseLabel;
	thenLabel_ = &thenLabel;
	
	isNot_   = false;
	lastAnd_ = true;

	bc.bind(beforeLabel);

	whileNode->whileExpr()->visit(this);

	bc.bind(thenLabel);
	
	if (!lastAnd_)
		bc.addBranch(BC_JA, elseLabel);
	
	whileNode->loopBlock()->visit(this);
	bc.addBranch(BC_JA, beforeLabel);

	bc.bind(elseLabel);
}


void CompilerVisitor::visitIntBinOpNode(BinaryOpNode* opNode)
{
	Bytecode&    bc = bytecodes_[lastFunction_];
	StackLayout& sl = callStacks_[lastFunction_];

	switch(opNode->kind())
	{
#define PROCESS_KIND(type)	\
	case t##type:			\
		bc.add(BC_I##type); \
		sl.popLocalVar();   \
		sl.popLocalVar();   \
		sl.pushLocalVar(make_pair(VT_INT, "")); \
		break;
    FOR_BIN_INT_OPNODE_KINDS(PROCESS_KIND)
	FOR_BIN_OPNODE_KINDS(PROCESS_KIND);
#undef PROCESS_KIND
#define SKIP_KIND(type) \
	case t##type:	\
		break;
	FOR_IF_OPNODE_AND_OR(SKIP_KIND)
#undef SKIP_KIND
#define PROCESS_KIND(kind, THEN, ELSE) \
	case t##kind:	\
		bc.add(BC_ICMP);   \
		sl.popLocalVar();  \
		sl.popLocalVar();  \
		sl.pushLocalVar(make_pair(VT_INT, ""));	\
		bc.add(BC_ILOAD0); \
		sl.pushLocalVar(make_pair(VT_INT, ""));	\
		if (!lastAnd_) \
			bc.addBranch(isNot_ ? ELSE : THEN, *thenLabel_); \
		else \
			bc.addBranch(isNot_ ? THEN : ELSE, *elseLabel_); \
		sl.popLocalVar(); \
		sl.popLocalVar(); \
		break;
	FOR_IF_OPNODE_KINDS(PROCESS_KIND);
#undef PROCESS_KIND
	default:
		assert(true);
	}
}


void CompilerVisitor::visitDoubleBinOpNode(BinaryOpNode* opNode)
{
	Bytecode&    bc = bytecodes_[lastFunction_];
	StackLayout& sl = callStacks_[lastFunction_];
	
	switch(opNode->kind())
	{
#define PROCESS_KIND(type)	\
	case t##type:			\
		bc.add(BC_D##type); \
		sl.popLocalVar();   \
		sl.popLocalVar();   \
		sl.pushLocalVar(make_pair(VT_DOUBLE, "")); \
		break;
	FOR_BIN_OPNODE_KINDS(PROCESS_KIND);
#undef PROCESS_KIND
#define SKIP_KIND(type) \
	case t##type:	\
		break;
	FOR_IF_OPNODE_AND_OR(SKIP_KIND)
#undef SKIP_KIND
#define PROCESS_KIND(kind, THEN, ELSE) \
	case t##kind:	\
		bc.add(BC_DCMP);   \
		sl.popLocalVar();  \
		sl.popLocalVar();  \
		sl.pushLocalVar(make_pair(VT_INT, ""));	\
		bc.add(BC_ILOAD0); \
		sl.pushLocalVar(make_pair(VT_INT, ""));	\
		if (!lastAnd_) \
			bc.addBranch(isNot_ ? THEN : ELSE, *thenLabel_); \
		else \
			bc.addBranch(isNot_ ? ELSE : THEN, *elseLabel_); \
		sl.popLocalVar(); \
		sl.popLocalVar(); \
		break;
	FOR_IF_OPNODE_KINDS(PROCESS_KIND);
#undef PROCESS_KIND
	default:
		assert(true);
	}
}


#undef PUSH_IEAX
#undef PUSH_EBP
#undef POP_EBP
#undef PUSH_ESP
