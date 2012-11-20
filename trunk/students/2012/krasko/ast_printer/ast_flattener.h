/*
 * ast_flattener.h
 *
 *  Created on: 17.11.2012
 *      Author: Evgeniy Krasko
 */
#ifndef AST_FLATTENER_H_
#define AST_FLATTENER_H_

#include <string>
#include "parser.h"
#include <sstream>
#include "visitors.h"

using namespace mathvm;
using namespace std;

class AstFlattener: public AstVisitor {

	string dToS(double d) {
		std::ostringstream out;
		out << d;
		return out.str();
	}

	string iToS(int64_t i) {
		std::ostringstream out;
		out << i;
		return out.str();
	}

	void visitBinaryOpNode(BinaryOpNode * node) {
		s += "(";
		node->left()->visit(this);
		s += " ";
		s += tokenOp(node->kind());
		s += " ";
		node->right()->visit(this);
		s += ")";
	}

	void visitBlockNode(BlockNode * node) {
		s += "{\n";
		flattenBlockContents(node);
		s += "}";
	}

	void visitCallNode(CallNode * node) {
		s += node->name();
		s += "(";
		for (unsigned i = 0; i < node->parametersNumber(); ++i) {
			if (i != 0)
				s += ", ";
			node->parameterAt(i)->visit(this);
		}
		s += ")";
	}

	void visitDoubleLiteralNode(DoubleLiteralNode * node) {
		s += dToS(node->literal());
	}

	void visitForNode(ForNode * node) {
		s += "for (";
		s += node->var()->name();
		s += " in ";
		node->inExpr()->visit(this);
		s += ") ";
		visitBlockNode(node->body());
	}

	void visitFunctionNode(FunctionNode * node) {
		s += "function ";
		s += typeToName(node->returnType());
		s += " ";
		s += node->name();
		s += "(";
		for (unsigned i = 0; i < node->parametersNumber(); ++i) {
			if (i != 0)
				s += ", ";
			s += typeToName(node->parameterType(i));
			s += " ";
			s += node->parameterName(i);
		}
		s += ") ";
		if(node->body()->nodeAt(0)->isNativeCallNode()) {
			visitNativeCallNode(node->body()->nodeAt(0)->asNativeCallNode());
		} else {
			visitBlockNode(node->body());
		}
	}
	void visitIfNode(IfNode * node) {
		s += "if(";
		node->ifExpr()->visit(this);
		s += ") ";
		visitBlockNode(node->thenBlock());
		if (node->elseBlock() != NULL) {
			s += " else ";
			visitBlockNode(node->elseBlock());
		}
	}
	void visitIntLiteralNode(IntLiteralNode * node) {
		s += iToS(node->literal());
	}
	void visitLoadNode(LoadNode * node) {
		s += node->var()->name();
	}
	void visitNativeCallNode(NativeCallNode * node) {
		s += " native '";
		s += node->nativeName();
		s += "';";
	}
	void visitPrintNode(PrintNode * node) {
		s += "print(";
		for (unsigned i = 0; i < node->operands(); ++i) {
			if (i != 0)
				s += ", ";
			node->operandAt(i)->visit(this);
		}
		s += ")";
	}

	void visitReturnNode(ReturnNode * node) {
		s += "return ";
		node->visitChildren(this);
	}

	void visitStoreNode(StoreNode * node) {
		s += node->var()->name();
		s += " ";
		s += tokenOp(node->op());
		s += " ";
		node->value()->visit(this);
	}

	string escape(const string & s) {
		string res;
		for (unsigned i = 0; i < s.length(); ++i) {
			switch (s[i]) {
			case '\n':
				res += "\\n";
				break;
			case '\t':
				res += "\\t";
				break;
			case '\r':
				res += "\\r";
				break;
			case '\\':
				res += "\\\\";
				break;
			default:
				res += s[i];
				break;
			}
		}
		return res;
	}

	void visitStringLiteralNode(StringLiteralNode * node) {
		s += "'";
		s += escape(node->literal());
		s += "'";
	}

	void visitUnaryOpNode(UnaryOpNode * node) {
		s += tokenOp(node->kind());
		node->operand()->visit(this);
	}

	void visitWhileNode(WhileNode * node) {
		s += "while(";
		node->whileExpr()->visit(this);
		s += ") ";
		visitBlockNode(node->loopBlock());
	}

	void declareVariable(AstVar * var) {
		s += typeToName(var->type());
		s += " ";
		s += var->name();
	}

	void declareVariables(Scope * scope) {
		Scope::VarIterator iter(scope);
		while (iter.hasNext()) {
			AstVar * var = iter.next();
			declareVariable(var);
			s += ";\n";
		}
	}

	void declareFunctions(Scope * scope) {
		Scope::FunctionIterator iter(scope);
		while (iter.hasNext()) {
			AstFunction * func = iter.next();
			visitFunctionNode(func->node());
			s += "\n";
		}
	}

	void flattenBlockContents(BlockNode *node) {
		if(node == NULL) return;

		declareVariables(node->scope());
		declareFunctions(node->scope());

		for (unsigned i = 0; i < node->nodes(); ++i) {
			AstNode * nodeAtI = node->nodeAt(i);
			nodeAtI->visit(this);
			if (!nodeAtI->isWhileNode() && !nodeAtI->isBlockNode()
					&& !nodeAtI->isFunctionNode() && !nodeAtI->isForNode()
					&& !nodeAtI->isIfNode()) {
				s += ";\n";
			} else {
				s += "\n";
			}
		}

	}

	string s;

public:
	string flatten(AstFunction * ast) {
		s = "";
		flattenBlockContents(ast->node()->body());
		return s;
	}
};

#endif /* AST_FLATTENER_H_ */
