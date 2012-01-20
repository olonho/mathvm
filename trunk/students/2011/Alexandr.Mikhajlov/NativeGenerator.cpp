#include "NativeGenerator.h"
#include <iostream>
#include <string>
#include <sstream>

#include "asmjit\Build.h"
#include "AsmJit/Assembler.h"
#include "AsmJit/Compiler.h"
#include "AsmJit/MemoryManager.h"

using namespace AsmJit;
using namespace mathvm;
using namespace std;

const int DEFAULT_STACK_SIZE = 1024;

extern bool silentMode;

FileLogger logger(stderr);


void PrintInt(int64_t value) {
	std::cout << value;
}

void PrintMemory(int64_t address) {
	int64_t * p = (int64_t*)address;
	int64_t sz = *(p-1);
	std::cout << "ID: " << *(p - 2) << " Size: " << sz << "(";

	for (int64_t * ip = p + (sz - 2 * sizeof(int64_t)) / sizeof(int64_t) ; p != ip; ++p ) {
		cout << *p << ", ";
	}
	cout << ")\n";
}

void PrintString(int64_t address) {
	std::string * s = (std::string*)address;
	printf("%s", s->c_str());
}


void PrintDouble( double value )
{
	std::stringstream stream;
	stream << value;
	std::string s = stream.str();
	int i = s.find("e+0");
	if (i != -1) {
		s.replace(i, 3, "e+");
	}

	i = s.find("e-0");
	if (i != -1) {
		s.replace(i, 3, "e-");
	}

  std::cout << s;
}


int64_t idiv(int64_t a, int64_t b) {
	return a / b;
}


void NativeGenerator::Compile( mathvm::AstFunction * rootNode)
{
	myFirstPassVisitor.visit(rootNode);
	myResultVar.Integer = NULL;

	myCompiler = new Compiler;
  if (!silentMode)myCompiler->setLogger(&logger);

	rootNode->node()->visit(this);

	auto top = function_cast<void (*)()>(myFunctions[0]);

	myLocalsPointer = new int64_t[DEFAULT_STACK_SIZE];
	myLocalsPointerOrigin = myLocalsPointer;
	memset(myLocalsPointer, 0, DEFAULT_STACK_SIZE * sizeof(int64_t));
	myLocalsPointer[0] = -1;
	myLocalsPointer[1] = sizeof(int64_t) * 2;
	myLocalsPointer = &myLocalsPointer[2]; // It should point directly to vars block. Access size and id with negative offset


	top();

	//myCompiler = new Compiler;
	//if (!silentMode)myCompiler->setLogger(&logger);
	//	
	//myCompiler->newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<void>());
	//myCompiler->getFunction()->setHint(FUNCTION_HINT_NAKED, true);

	//myLocalsPtr = GPVar(myCompiler->newGP());
	//myCompiler->mov(myLocalsPtr, imm((sysint_t)myLocalsPointer));
	//	
	//ECall * ctx = myCompiler->call(top);
	//ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder0<void>());

	//myCompiler->ret();
	//myCompiler->endFunction();

	//void * pt = myCompiler->make();

	//auto fun = function_cast<void (*)()>(pt);
	//fun();
	// 
	int x = 0; x++;
}

uint32_t ToAsmJitType(mathvm::VarType type) {
	switch (type) {
	case mathvm::VT_INT:
	case mathvm::VT_STRING:
		return AsmJit::VARIABLE_TYPE::VARIABLE_TYPE_INT64;
	case mathvm::VT_DOUBLE:
		return AsmJit::VARIABLE_TYPE::VARIABLE_TYPE_DOUBLE;
	}
}

void NativeGenerator::visitFunctionNode( mathvm::FunctionNode* node )
{
	uint16_t id = myFirstPassVisitor.GetFunctionId(node->name());
	NodeInfo const & nodeInfo = GetNodeInfo(node->body());
	uint16_t varsNum = nodeInfo.scopeInfo->GetTotalVariablesNum();
	AstFunction * astFunction = nodeInfo.scopeInfo->GetAstFunction();
	

	BlockNode * block = node->body();
	Scope::FunctionIterator it(block->scope());
	while (it.hasNext()) {
		AstFunction * astFun = it.next();
		astFun->node()->visit(this);
	}

	Compiler * old = myCompiler;
	myCompiler = new Compiler;
	if (!silentMode)myCompiler->setLogger(&logger);

	uint32_t argsTypes[256] = {0};
	for (int i = 0; i < astFunction->parametersNumber(); ++i) {
		argsTypes[i] = ToAsmJitType(astFunction->parameterType(i));
	}

	EFunction* fun = myCompiler->newFunction_(CALL_CONV_DEFAULT, argsTypes, astFunction->parametersNumber(), ToAsmJitType(astFunction->returnType()));
	myCompiler->getFunction()->setHint(FUNCTION_HINT_NAKED, true);
	myCompiler->comment("Function: %s", astFunction->name().c_str());

	GPVar  oldPtr = myLocalsPtr;
	myLocalsPtr = GPVar(myCompiler->newGP());

	
	// Prolog
	GPVar sz(myCompiler->newGP()); // Size of previous stack frame
	GPVar localsPtr(myCompiler->newGP()); // Pointer to local vars

	myCompiler->comment("My Prolog");

	myCompiler->mov(localsPtr, imm((sysint_t)&myLocalsPointer));
	myCompiler->mov(myLocalsPtr, qword_ptr(localsPtr));
	//
	myCompiler->mov(sz, qword_ptr(myLocalsPtr, -sizeof(int64_t)));

	myCompiler->add(myLocalsPtr, sz);

	myCompiler->mov(qword_ptr(myLocalsPtr, - 2 * sizeof(int64_t)), imm(id)); // Write id to stack frame
	myCompiler->mov(qword_ptr(myLocalsPtr, - sizeof(int64_t)), imm((2 + varsNum) *sizeof(int64_t))); // Write total frame size

	

	myCompiler->mov(qword_ptr(localsPtr), myLocalsPtr);

	myCompiler->comment("Parameters loading");

	//call = myCompiler->call(PrintInt);
	//call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, int64_t>());
	//call->setArgument(0, myLocalsPtr);

	// Loading parameters:
	for (int i = 0; i < nodeInfo.scopeInfo->GetAstFunction()->parametersNumber(); ++i) {
		if (astFunction->parameterType(i) == VT_DOUBLE) 
			myCompiler->movq(qword_ptr(myLocalsPtr, i * sizeof(int64_t)), myCompiler->argXMM(i));
		else 
			myCompiler->mov(qword_ptr(myLocalsPtr, i * sizeof(int64_t)), myCompiler->argGP(i));
	}

	myCompiler->comment("My Body");

	// Body
	block->visit(this);

	//ECall * call = myCompiler->call(PrintMemory);
	//call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, int64_t>());
	//call->setArgument(0, myLocalsPtr);

	myCompiler->comment("My Epilog");

	// Epilog
	myCompiler->sub(myLocalsPtr, sz);
	myCompiler->mov(qword_ptr(localsPtr), myLocalsPtr);

	myCompiler->ret();
	myCompiler->endFunction();

	myFunctions[id] = myCompiler->make();

	myCompiler = old;
	myLocalsPtr = oldPtr;
}


void NativeGenerator::visitCallNode( mathvm::CallNode* node )
{
	uint16_t id = myFirstPassVisitor.GetFunctionId(node->name());
	ECall* ctx = myCompiler->call(myFunctions[id]);

	uint32_t argsTypes[256] = {0};
	for (int i = 0; i < node->parametersNumber(); ++i) {
		argsTypes[i] = ToAsmJitType(GetNodeType(node->parameterAt(i)));
	}

	ctx->_setPrototype(CALL_CONV_DEFAULT, argsTypes, node->parametersNumber(), VARIABLE_TYPE_INT64);

	AsmVarPtr old = myResultVar;

	for (int i = 0; i < node->parametersNumber(); ++i) {
		VarType type = GetNodeType(node->parameterAt(i));
		myResultVar = CreateAsmVar(type);
		node->parameterAt(i)->visit(this);
		if (type == VT_DOUBLE) ctx->setArgument(i, *myResultVar.Double);
		else ctx->setArgument(i, *myResultVar.Integer);
	}

	myResultVar = old;

}


void NativeGenerator::visitPrintNode(mathvm::PrintNode* node) 
{
	AsmVarPtr old = myResultVar;
	for (unsigned int i = 0; i < node->operands(); ++i) {
    AstNode* op = node->operandAt(i);
    
		myResultVar = CreateAsmVar(GetNodeType(op));
		op->visit(this);

		ECall* ctx = NULL;

		switch (GetNodeType(op)) {
		case mathvm::VT_INT: 
			ctx = myCompiler->call(PrintInt);
			ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, int64_t>());
			ctx->setArgument(0, *myResultVar.Integer);
			break;

		case mathvm::VT_STRING: 
			ctx = myCompiler->call(PrintString);
			ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, int64_t>());
			ctx->setArgument(0, *myResultVar.Integer);
			break;

		case mathvm::VT_DOUBLE: 
			ctx = myCompiler->call(PrintDouble);
			ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, double>());
			ctx->setArgument(0, *myResultVar.Double);
			break;
		}
  }

	myResultVar = old;
}

AsmVarPtr NativeGenerator::CreateAsmVar( mathvm::VarType type )
{
	AsmVarPtr p;
	switch(type) {
	case VT_DOUBLE: p.Double = new XMMVar(myCompiler->newXMM()); break;
	default:
		p.Integer = new GPVar(myCompiler->newGP());
	}
	return p;
}


void NativeGenerator::visitIntLiteralNode(mathvm::IntLiteralNode* node) 
{
	myCompiler->mov(*myResultVar.Integer, node->literal());
}

void NativeGenerator::visitBlockNode(mathvm::BlockNode* node)
{
	node->visitChildren(this);
}

void NativeGenerator::visitStringLiteralNode(mathvm::StringLiteralNode* node)
{
	int64_t p = (int64_t)new std::string(node->literal());
	myCompiler->mov(*myResultVar.Integer, p);
}

void NativeGenerator::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node)
{
	GPVar v(myCompiler->newGP());
	double value = node->literal();
	int64_t nvalue = *((int64_t*) &value);
	
	myCompiler->mov(v, imm(nvalue));
	myCompiler->movq(*myResultVar.Double, v);
	myCompiler->unuse(v);
}

void NativeGenerator::visitBinaryOpNode(mathvm::BinaryOpNode* node)
{
  AsmVarPtr old = myResultVar;

	VarType expectedType = GetNodeType(node);
	
	myResultVar = CreateAsmVar(expectedType);
	AsmVarPtr left = VisitWithTypeControl(node->left(), expectedType);
	
	myResultVar = CreateAsmVar(expectedType);
	AsmVarPtr right = VisitWithTypeControl(node->right(), expectedType);

	myResultVar = old;
	if (myResultVar.Integer == NULL) return;

	if (TryDoArithmetics(node, left, right, expectedType)) return;
	if (expectedType == mathvm::VT_INT) {
		TryDoIntegerLogic(node, left, right);
		return;
	}
}

VarId NativeGenerator::GetVariableId( mathvm::AstNode* currentNode, std::string const& varName, bool* isClosure_out /*= NULL*/ )
{
	ScopeInfo * info = GetNodeInfo(currentNode).scopeInfo;
	bool isClosure = false;
	uint16_t id = 0;
	while (info) {
		if (info->TryFindVariableId(varName, id)) break;
		if (info->IsFunction()) isClosure = true;
		info = info->GetParent();
	}

	if (info == NULL) throw TranslationException("Undefined variable: " + varName);

	VarId result;
	result.id = id;
	result.ownerFunction = info->GetFunctionId();

	if (isClosure_out) *isClosure_out = isClosure;
	return result;
}

void NativeGenerator::visitStoreNode( mathvm::StoreNode* node )
{
	AsmVarPtr old = myResultVar;
	myResultVar = CreateAsmVar(node->var()->type());
	
	node->value()->visit(this);
	bool isClosure = false;
	VarId id = GetVariableId(node, node->var()->name(), &isClosure);
	VarType expectedType = node->var()->type();

	if (isClosure == false) {
		if (node->op() == tASSIGN) SetVariable(expectedType, id);
		else if (node->op() == tINCRSET) IncrSetVariable(myLocalsPtr, expectedType, id.id);
		else if (node->op() == tDECRSET) DecrSetVariable(myLocalsPtr, expectedType, id.id);
	}
}

void NativeGenerator::visitLoadNode( mathvm::LoadNode* node )
{
	bool isClosure = false;
	VarId id = GetVariableId(node, node->var()->name(), &isClosure);
	switch(node->var()->type()) {
	case mathvm::VT_INT:
	case mathvm::VT_STRING:
		myCompiler->mov(*myResultVar.Integer, dword_ptr(myLocalsPtr, id.id * sizeof(uint64_t)));
		break;
	case mathvm::VT_DOUBLE:
		myCompiler->movq(*myResultVar.Double, dword_ptr(myLocalsPtr, id.id * sizeof(uint64_t)));
		break;
	}
}




AsmVarPtr NativeGenerator::VisitWithTypeControl( mathvm::AstNode * node, mathvm::VarType expectedType )
{
	mathvm::VarType type = GetNodeType(node);
	if (expectedType == VT_STRING || type == VT_STRING) throw new std::exception("Binary operations for strings not supported");
	
	node->visit(this);

	if (expectedType != type) {
		AsmVarPtr nw = AsmVarPtr();

		if (expectedType == VT_DOUBLE) {
			nw.Double = new XMMVar(myCompiler->newXMM());

			myCompiler->cvtsi2sd(*nw.Double, *myResultVar.Integer );
			return nw;
		}
		if (expectedType == mathvm::VT_INT) {
			nw.Integer = new GPVar(myCompiler->newGP());
			myCompiler->cvtsd2si(*nw.Integer, *myResultVar.Double);
		}
	}
	return myResultVar;
}


bool NativeGenerator::TryDoArithmetics( mathvm::BinaryOpNode* node, AsmVarPtr left, AsmVarPtr right, mathvm::VarType expectedType )
{
	switch(node->kind()) {
	case tADD: 
		if (expectedType == mathvm::VT_INT) {
			myCompiler->mov(*myResultVar.Integer, *left.Integer);
			myCompiler->add(*myResultVar.Integer, *right.Integer);
			return true;
		}
		else if (expectedType == mathvm::VT_DOUBLE) {
			myCompiler->movq(*myResultVar.Double, *left.Double);
			myCompiler->addsd(*myResultVar.Double, *right.Double);
			return true;
		}
		break;
	case tMUL:
		if (expectedType == mathvm::VT_INT) {
			myCompiler->mov(*myResultVar.Integer, *left.Integer);
			myCompiler->imul(*myResultVar.Integer, *right.Integer);
			return true;
		}
		else if (expectedType == mathvm::VT_DOUBLE) {
			myCompiler->movq(*myResultVar.Double, *left.Double);
			myCompiler->mulsd(*myResultVar.Double, *right.Double);
			return true;
		}
		break;
	case tSUB:
		if (expectedType == mathvm::VT_INT) {
			myCompiler->mov(*myResultVar.Integer, *left.Integer);
			myCompiler->sub(*myResultVar.Integer, *right.Integer);
			return true;
		}
		else if (expectedType == mathvm::VT_DOUBLE) {
			myCompiler->movq(*myResultVar.Double, *left.Double);
			myCompiler->subsd(*myResultVar.Double, *right.Double);
			return true;
		}
		break;
	case tDIV:
		if (expectedType == mathvm::VT_INT) {
			ECall * ctx = myCompiler->call(imm((size_t)idiv));
			ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<int64_t,int64_t,int64_t>());
			ctx->setArgument(0, *left.Integer);
			ctx->setArgument(1, *right.Integer);
			ctx->setReturn(*myResultVar.Integer);
			return true;
		}
		else if (expectedType == mathvm::VT_DOUBLE) {
			myCompiler->movq(*myResultVar.Double, *left.Double);
			myCompiler->divsd(*myResultVar.Double, *right.Double);
			return true;
		}
		break;
	}
	return false;
}

void NativeGenerator::visitUnaryOpNode( mathvm::UnaryOpNode* node )
{
	node->operand()->visit(this);
	VarType operandType = GetNodeType(node->operand());

	switch (node->kind()) {
	case tSUB: 
		if (operandType == mathvm::VT_INT) myCompiler->neg(*myResultVar.Integer);
		else {
			GPVar itemp(myCompiler->newGP());
			XMMVar dtemp(myCompiler->newXMM());
			myCompiler->mov(itemp, imm(0));
			myCompiler->movq(dtemp, *myResultVar.Double);
			myCompiler->movq(*myResultVar.Double, itemp);
			myCompiler->subsd(*myResultVar.Double, dtemp);
		}
		break;
	case tNOT:
		AsmJit::Label lTrue = myCompiler->newLabel();
		AsmJit::Label lEnd  = myCompiler->newLabel();
		myCompiler->cmp(*myResultVar.Integer, imm(0));
		myCompiler->je(lTrue);
		myCompiler->mov(*myResultVar.Integer, imm(0));
		myCompiler->jmp(lEnd);
		myCompiler->bind(lTrue);
		myCompiler->mov(*myResultVar.Integer, imm(1));
		myCompiler->bind(lEnd);

	}
}

void NativeGenerator::visitIfNode( mathvm::IfNode* node )
{
	AsmVarPtr old = myResultVar;
	myResultVar = CreateAsmVar(mathvm::VT_INT);

	AsmJit::Label lEnd = myCompiler->newLabel();

	node->ifExpr()->visit(this);

	if (node->elseBlock()) {
		AsmJit::Label lFalse = myCompiler->newLabel();
		myCompiler->cmp(*myResultVar.Integer, imm(0));
		myCompiler->je(lFalse);
		node->thenBlock()->visit(this);
		myCompiler->ja(lEnd);
		myCompiler->bind(lFalse);
		node->elseBlock()->visit(this);
	} 
	else {
		myCompiler->cmp(*myResultVar.Integer, imm(0));
		myCompiler->je(lEnd);
		node->thenBlock()->visit(this);
	}

	myCompiler->bind(lEnd);
	myResultVar = old;
}

void NativeGenerator::TryDoIntegerLogic( mathvm::BinaryOpNode* node, AsmVarPtr left, AsmVarPtr right )
{
	myCompiler->cmp(*left.Integer, *right.Integer);

	AsmJit::Label lTrue = myCompiler->newLabel();
	AsmJit::Label lEnd = myCompiler->newLabel();

	switch (node->kind()) 
	{
	case tEQ:
	case tAND:
		myCompiler->je(lTrue); break;
	case tNEQ:
		myCompiler->jne(lTrue); break;
	case tGT:
		myCompiler->jg(lTrue); break;
	case tGE:
		myCompiler->je(lTrue); break;
	case tLT:
		myCompiler->jl(lTrue); break;
	case tLE:
		myCompiler->jle(lTrue); break;
	default: throw TranslationException("Invalid operation type");
	}

	myCompiler->mov(*myResultVar.Integer, imm(0));
	myCompiler->jmp(lEnd);
	myCompiler->bind(lTrue);
	myCompiler->mov(*myResultVar.Integer, imm(1));
	myCompiler->bind(lEnd);
}

void NativeGenerator::visitWhileNode( mathvm::WhileNode* node )
{
	AsmJit::Label lEnd = myCompiler->newLabel();
	AsmJit::Label lCheck = myCompiler->newLabel();

	myCompiler->bind(lCheck);
	node->whileExpr()->visit(this);
	myCompiler->cmp(*myResultVar.Integer, imm(0));
	myCompiler->je(lEnd);

	node->loopBlock()->visit(this);
	myCompiler->jmp(lCheck);

	myCompiler->bind(lEnd);
}

void NativeGenerator::SetVariable( mathvm::VarType expectedType, VarId &id )
{
	switch (expectedType) {
	case mathvm::VT_INT:
	case mathvm::VT_STRING:
		myCompiler->mov(dword_ptr(myLocalsPtr, id.id * sizeof(uint64_t)), *myResultVar.Integer);
		break;
	case mathvm::VT_DOUBLE:
		myCompiler->movq(dword_ptr(myLocalsPtr, id.id * sizeof(uint64_t)), *myResultVar.Double);
		break;
	}
}

void NativeGenerator::IncrSetVariable( AsmJit::GPVar myLocalsPtr, mathvm::VarType type, int16_t varId )
{
	auto ptr = dword_ptr(myLocalsPtr, varId * sizeof(uint64_t));

	switch (type) {
	case mathvm::VT_INT:
		myCompiler->add(ptr, *myResultVar.Integer);
		break;
	case mathvm::VT_DOUBLE:
		XMMVar temp(myCompiler->newXMM());
		myCompiler->movq(temp, ptr);
		myCompiler->addsd(temp, *myResultVar.Double);
		myCompiler->movq(*myResultVar.Double, temp);
		break;
	}
}

void NativeGenerator::DecrSetVariable( AsmJit::GPVar myLocalsPtr, mathvm::VarType type, int16_t varId )
{
	auto ptr = dword_ptr(myLocalsPtr, varId * sizeof(uint64_t));

	switch (type) {
	case mathvm::VT_INT:
		myCompiler->sub(ptr, *myResultVar.Integer);
		break;
	case mathvm::VT_DOUBLE:
		XMMVar temp(myCompiler->newXMM());
		myCompiler->movq(temp, ptr);
		myCompiler->subsd(temp, *myResultVar.Double);
		myCompiler->movq(*myResultVar.Double, temp);
		break;
	}
}

void NativeGenerator::visitForNode( mathvm::ForNode* node )
{
	AsmVarPtr old = myResultVar;
	myResultVar = CreateAsmVar(mathvm::VT_INT);
	
	AsmJit::Label lCheck = myCompiler->newLabel();
	AsmJit::Label lEnd = myCompiler->newLabel();

	BinaryOpNode * range = node->inExpr()->asBinaryOpNode();
	if (range == NULL || range->kind() != tRANGE) throw TranslationException("Range not specified in for statement");
	uint16_t varId = GetVariableId(node, node->var()->name()).id;
	auto ptr = qword_ptr(myLocalsPtr, varId * sizeof(uint64_t));
	
	// init counter
	range->left()->visit(this);
	myCompiler->mov(ptr, *myResultVar.Integer);
	
	myCompiler->bind(lCheck);

	// counter >= right
	range->right()->visit(this);
	myCompiler->cmp(ptr, *myResultVar.Integer);
	myCompiler->jg(lEnd);

	node->body()->visit(this);

	// increment counter
	myCompiler->add(ptr, imm(1));
	myCompiler->jmp(lCheck);

	myCompiler->bind(lEnd);

	myResultVar = old;
}



