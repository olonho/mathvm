#include "parser.h"
#include "BcTranslator.h"

BcTranslator::BcTranslator()
: mathvm::Translator()
, mathvm::AstVisitor()
, resultCode(0)
{}

mathvm::Status* BcTranslator::translate(const std::string& program, mathvm::Code** code)
{
	mathvm::Parser parser;
	mathvm::Status* s = parser.parseProgram(program);
	if (s && s->isError()) {
		return s;
	}

	resultCode = new InterpreterCodeImpl();

	// magic
	
	*code = resultCode;

	return s;
}

void BcTranslator::visitBinaryOpNode(mathvm::BinaryOpNode* node)
{
}
void BcTranslator::visitUnaryOpNode(mathvm::UnaryOpNode* node)
{
}
void BcTranslator::visitStringLiteralNode(mathvm::StringLiteralNode* node)
{
}
void BcTranslator::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node)
{
}
void BcTranslator::visitIntLiteralNode(mathvm::IntLiteralNode* node)
{
}
void BcTranslator::visitLoadNode(mathvm::LoadNode* node)
{
}
void BcTranslator::visitStoreNode(mathvm::StoreNode* node)
{
}
void BcTranslator::visitForNode(mathvm::ForNode* node)
{
}
void BcTranslator::visitWhileNode(mathvm::WhileNode* node)
{
}
void BcTranslator::visitIfNode(mathvm::IfNode* node)
{
}
void BcTranslator::visitBlockNode(mathvm::BlockNode* node)
{
}
void BcTranslator::visitFunctionNode(mathvm::FunctionNode* node)
{
}
void BcTranslator::visitReturnNode(mathvm::ReturnNode* node)
{
}
void BcTranslator::visitCallNode(mathvm::CallNode* node)
{
}
void BcTranslator::visitNativeCallNode(mathvm::NativeCallNode* node)
{
}
void BcTranslator::visitPrintNode(mathvm::PrintNode* node)
{
}
