
#ifndef BYTECODEVISITOR_H_
#define BYTECODEVISITOR_H_

#include "ast.h"
#include <stack>
#include <map>

namespace mathvm {


class ASTtoByteCodeTranslator : public AstVisitor {

public:
	ASTtoByteCodeTranslator(Code* code);
	void performTranslation(AstFunction* top);
	virtual ~ASTtoByteCodeTranslator();

private:
	Code* code;
	std::stack<BytecodeFunction*> funStack;
	std::stack<Scope*> scopeStack;

	std::map<const AstFunction*, BytecodeFunction*> funMap;
	std::map<const AstVar*, uint16_t> varMap;

	VarType varType;

	void createFunMap(Scope* scope);
	void createVarMap(Scope* scope);

	uint16_t getNextVarID();

	void visitBinaryOpNode(BinaryOpNode *node);
	void subWorkOnBinaryOp(Bytecode *bytecode);

	void visitUnaryOpNode(UnaryOpNode *node);
	void visitStringLiteralNode(StringLiteralNode *node);
	void visitIntLiteralNode(IntLiteralNode *node);
	void visitDoubleLiteralNode(DoubleLiteralNode *node);
	void visitLoadNode(LoadNode *node);
	void visitStoreNode(StoreNode *node);
	void visitBlockNodeWithoutBraces(BlockNode *node);
	void visitBlockNode(BlockNode *node);
	void visitNativeCallNode(NativeCallNode *node);
	void visitForNode(ForNode *node);
	void visitWhileNode(WhileNode *node);
	void visitIfNode(IfNode *node);
	void visitReturnNode(ReturnNode *node);
	void visitFunctionNode(FunctionNode *node);
	void visitCallNode(CallNode *node);
	void visitPrintNode(PrintNode *node);

};

}

#endif /* BYTECODEVISITOR_H_ */
