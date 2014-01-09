#pragma once

#include <vector>
using std::vector;
#include <list>
using std::list;
#include <string>
using std::string;

#include "ast.h"
using namespace mathvm;

#include "StackLayout.h"

class Bytecode_ : public Bytecode {
public:
    uint8_t * getData() {
        return _data.data();
    }
};

class CompilerVisitor : public AstVisitor
{
public:
	CompilerVisitor();

	void visitStartFunction(AstFunction* f, const map<AstFunction*, set<pair<VarType, string> > >& captured_);
	const vector<Bytecode_>& bytecodes() const;
	const vector<string>& literals() const;

private:
	virtual void visitFunctionNode(FunctionNode* function);
	virtual void visitBlockNode(BlockNode* block);
	virtual void visitBinaryOpNode(BinaryOpNode* opNode);
	virtual void visitCallNode(CallNode* callNode);
	virtual void visitDoubleLiteralNode(DoubleLiteralNode* doubleLiteral);
	virtual void visitForNode(ForNode* forNode);
	virtual void visitIfNode(IfNode* ifNode);
	virtual void visitIntLiteralNode(IntLiteralNode* intLIteral);
	virtual void visitLoadNode(LoadNode* loadNode);
	virtual void visitNativeCallNode(NativeCallNode* nativeCall);
	virtual void visitPrintNode(PrintNode* printNode);
	virtual void visitReturnNode(ReturnNode* returnNode);
	virtual void visitStoreNode(StoreNode* storeNode);
	virtual void visitStringLiteralNode(StringLiteralNode* stringLiteral);
	virtual void visitUnaryOpNode(UnaryOpNode* opNode);
	virtual void visitWhileNode(WhileNode* whileNode);

private:
	vector<Bytecode_>    bytecodes_;
	vector<StackLayout> callStacks_;
	vector<string>      stringLiterals;

	int  lastFunction_;
	bool isNot_;
	Label* elseLabel_;
	Label* thenLabel_;
	bool lastAnd_;

	StackLayout* parent_;
	vector<Scope*> scopeStack_;

	void visitIntBinOpNode(BinaryOpNode* opNode);
	void visitDoubleBinOpNode(BinaryOpNode* opNode);

	set<AstFunction*> already_;
	map<string, int> functionId_;
	AstFunction* lastAstFunction_;
	map<AstFunction*, set<pair<VarType, string> > > captured_;
};

/*
it can't work per function, only per call (!)

processAllCall example:
function C()
{
	int c;
	...
	A();
	C();
	function A()
	{
		int a = c;
		...
		A();
		B();
		C();
		function B()
		{
			int b = a;
			...

			A();
			B();
			C();
			D();
			function D()
			{
				...
				int d = b;
				...

				A();
				B();
				C();
				D();
			}
		}
	}
}

C - A C
A - A B C and c
B - A B C D and a
D - A B C D and b

remove self call
C - A
A - B C and c
B - A C D and a
D - A B C and b

substitue all and remove self call and duplicate from top to bottom
C - A
A - B and c
B - and c D and a
D - and c and a and b

remove local variable
C - (c)
A - (a), c
B - a, (b), c
D - a, b, c

C -
A - c
B - a,c
D - a,b,c

reverse substiture and remove duplicate
D - a,b,c
B - a,b,c
A - a,b,c
C - a,b,c

remove self call and duplicate
C - A
A - B C and c
B - A C D and a and c
D - A 

C - can't capture
A - A B and c
B - A B D and a
D - A B D and b

i can skip self call
C - can't capture
A - B and c
B - A D and a
D - A B and b

i can substitute
C - can't capture
A - B and c
B - B and c D and a
D - B and c B and b

skip self call and merge eq call
C - can't capture
A - B and c
B - D and c and a
D - B and c and b

substitute
C -
A - B and c
B - D and c and a
D - D and c and a and b

skip self call
C -
D - c,a,b
B - c,a,b
A - c,a,b
*/

/*
i use bad parse strategy

start parse top while first call.
on first call - resolve 
*/