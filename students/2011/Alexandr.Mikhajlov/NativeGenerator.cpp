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


void PrintInt(int64_t value) {
	printf("%d", value);
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


void NativeGenerator::Compile( mathvm::AstFunction * rootNode)
{
	myFirstPassVisitor.visit(rootNode);
	myResultVar.Integer = NULL;

	myCompiler = new Compiler;
	
	FileLogger logger(stderr);
  myCompiler->setLogger(&logger);

	myCompiler->newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<void>());
	myCompiler->getFunction()->setHint(FUNCTION_HINT_NAKED, true);

	CreateAsmVar(mathvm::VT_INT);

	rootNode->node()->body()->visit(this);
	
	myCompiler->ret();
	myCompiler->endFunction();

	void * pt = myCompiler->make();

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

int64_t idiv(int64_t a, int64_t b) {
	return a / b;

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

