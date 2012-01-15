#include "NativeGenerator.h"
#include <iostream>

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

void NativeGenerator::Compile( mathvm::AstFunction * rootNode)
{
	myFirstPassVisitor.visit(rootNode);
	
	myCompiler = new Compiler;
	
	FileLogger logger(stderr);
  myCompiler->setLogger(&logger);

	myCompiler->newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<void>());
	myCompiler->getFunction()->setHint(FUNCTION_HINT_NAKED, true);

	rootNode->node()->body()->visit(this);
	
	myCompiler->ret();
	myCompiler->endFunction();

	void * pt = myCompiler->make();

	auto fun = function_cast<void (*)()>(pt);
	fun();
}

void NativeGenerator::visitPrintNode(mathvm::PrintNode* node) 
{
	GPVar v(myCompiler->newGP());
	myVars.push(v);
	for (unsigned int i = 0; i < node->operands(); ++i) {
    AstNode* op = node->operandAt(i);
    op->visit(this);
		void * fptr = NULL;
		switch (GetNodeType(op)) {
		case mathvm::VT_INT: fptr = PrintInt;break;
		case mathvm::VT_STRING: fptr = PrintString;break;
		}

		GPVar address(myCompiler->newGP());
		myCompiler->mov(address, imm((sysint_t)(void*)fptr));

		ECall* ctx = myCompiler->call(fptr);
    ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, int64_t>());
	  ctx->setArgument(0, v);
  }
}

void NativeGenerator::visitIntLiteralNode(mathvm::IntLiteralNode* node) 
{
	myCompiler->mov(myVars.top(), node->literal());
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
	myCompiler->mov(myVars.top(), p);
}




