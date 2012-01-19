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

extern bool silentMode;

void PrintInt(int64_t value) {
	std::cout << value;
}

void PrintString(int64_t address) {
	std::string * s = (std::string*)address;
	printf("%s", s->c_str());
}


StackFrame* MyNativeCode::AllocateFrame( uint16_t functionId )
{
	BytecodeFunction * fun = (BytecodeFunction*) this->functionById(functionId);
	StackFrame * frame = AddFrame(fun->localsNumber(), functionId);
	frame->prevFrame = myCurrentFrame;
	if (myCurrentFrame) {
		if (myCurrentFrame->functionId != functionId) frame->prevDifferentFrame = myCurrentFrame;
		else frame->prevDifferentFrame = myCurrentFrame->prevDifferentFrame;
	} 
	assert(fun);
	myCurrentFrame = frame;

	return frame;
}

void MyNativeCode::Init()
{
	myFrameStackPoolIP = 0;
	myCurrentFrame = NULL;

	AllocateFrameStack(50*1024);
	AllocateFrame(0);
}

void MyNativeCode::AllocateFrameStack( int stackSizeInKb )
{
	myFrameStackPoolSize = stackSizeInKb * 1024;
	myFrameStackPool = new char[myFrameStackPoolSize];
}

StackFrame * MyNativeCode::AddFrame( uint16_t localsNumber, uint16_t functionId )
{
	StackFrame * result = new (&myFrameStackPool[myFrameStackPoolIP]) StackFrame(localsNumber, functionId);
	myFrameStackPoolIP += result->size;
	return result;
}

void* MyNativeCode::GetCurrentLocalsPtr( void* codeAddr )
{
	MyNativeCode * code = (MyNativeCode*)codeAddr;
	return code->myCurrentFrame->vars;
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
	
	FileLogger logger(stderr);
  if (!silentMode)myCompiler->setLogger(&logger);

	myCompiler->newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<void>());
	myCompiler->getFunction()->setHint(FUNCTION_HINT_NAKED, true);

	myLocalsPtr = GPVar(myCompiler->newGP());
	myCompiler->mov(myLocalsPtr, imm((sysint_t)new int[256]));

	rootNode->node()->visit(this);
	
	myCompiler->ret();
	myCompiler->endFunction();

	void * pt = myCompiler->make();

	myCode.Init();

	auto fun = function_cast<void (*)()>(pt);
	fun();
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
	uint16_t id = myCode.makeStringConstant(node->literal());
	std::string const & s = myCode.constantById(id);
	int64_t p = (int64_t)(void*)&s;
	p = (int64_t)new std::string(node->literal());
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


void NativeGenerator::visitFunctionNode( mathvm::FunctionNode* node )
{
	NodeInfo const & nodeInfo = GetNodeInfo(node->body());
	BytecodeFunction *bfun = new BytecodeFunction(nodeInfo.scopeInfo->GetAstFunction());
	bfun->setLocalsNumber(nodeInfo.scopeInfo->GetTotalVariablesNum());
	myCode.addFunction(bfun);

	//ECall * ctx = myCompiler->call(imm((size_t)idiv));
	//ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<int64_t,int64_t,int64_t>());
	//ctx->setArgument(0, imm(5));
	//ctx->setArgument(1, imm(6));
	//ctx->setReturn(myLocalsPtr);

	//ECall * call = myCompiler->call(imm((sysint_t) MyNativeCode::GetCurrentLocalsPtr));
	//call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, int>() );
	//call->setArgument(0, imm((sysint_t) &myCode));
	//call->setReturn(myLocalsPtr);

	node->visitChildren(this);
	
	//myCode.addFunction()
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




