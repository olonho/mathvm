#ifndef _ASTPRINTER_HPP_
#define _ASTPRINTER_HPP_

#include <ostream>

#include "ast.h"

class AstPrinter : public mathvm::AstVisitor {

private:
	std::ostream& os;


public:
	AstPrinter(std::ostream& outs): os(outs) {
	}


	virtual void visitBinaryOpNode(mathvm::BinaryOpNode * node) {
		os << "(";
		node->left()->visit(this);
		os << tokenOp(node->kind());
		node->right()->visit(this);
		os << ")";
	}

	
	virtual void visitUnaryOpNode(mathvm::UnaryOpNode * node) {
		os << "(";
		os << tokenOp(node->kind());
		node->operand()->visit(this);
		os << ")";
	}

	
	virtual void visitStringLiteralNode(mathvm::StringLiteralNode * node) {
		os << "'";

		const std::string& literal = node->literal();
		for (size_t i = 0; i < literal.length(); ++i) {
			char c = literal[i];
			switch(c) {
				case '\n': {
					os << "\\n";
					break;
				}
				case '\t': {
					os << "\\t";
					break;
				}
				case '\r': {
					os << "\\r";
					break;
				}
				case '\\': {
					os << "\\\\";
					break;
				}
				default: {
					os << c;
				}
			}
		}
		
		os << "'";
	}


	virtual void visitIntLiteralNode(mathvm::IntLiteralNode * node) {
		os << node->literal();
	}


	virtual void visitDoubleLiteralNode(mathvm::DoubleLiteralNode * node) {
		os << node->literal();
	}


	virtual void visitLoadNode(mathvm::LoadNode * node) {
		os << node->var()->name();
	}


	virtual void visitStoreNode(mathvm::StoreNode * node) {
		os << node->var()->name() << tokenOp(node->op());
		node->value()->visit(this);
		os << ";" << std::endl;
	}


	virtual void visitBlockNode(mathvm::BlockNode * node) {

		mathvm::Scope::VarIterator varIterator(node->scope());
		while (varIterator.hasNext()) {
			mathvm::AstVar * astVar = varIterator.next();
			os << typeToName(astVar->type()) << " " << astVar->name() << ";" << std::endl;
		}

		mathvm::Scope::FunctionIterator functionIteator(node->scope());
		while (functionIteator.hasNext()) {
			mathvm::AstFunction * astFunction= functionIteator.next();
			os << "function " << typeToName(astFunction->node()->returnType()) << " " << astFunction->node()->name() << "(";

			for (uint32_t i = 0; i < astFunction->node()->parametersNumber(); ++i) {
				if (i != 0) {
					os << ", ";
				}

				os << typeToName(astFunction->node()->parameterType(i)) << " " << astFunction->node()->parameterName(i);
			}
			os << ")";

			if (astFunction->node()->body()->nodes() != 0 && astFunction->node()->body()->nodeAt(0)->isNativeCallNode()) {
				astFunction->node()->body()->nodeAt(0)->visit(this);
			}
			else {
				os << "{" << std::endl;
				astFunction->node()->body()->visit(this);
				os << "}"  << std::endl;
			}
		}

		for (uint32_t i = 0; i < node->nodes(); i++) {
			node->nodeAt(i)->visit(this);

			if (node->nodeAt(i)->isCallNode()) {
				os << ";" << std::endl;			
			}			
        	}
	}


	virtual void visitNativeCallNode(mathvm::NativeCallNode * node) {
		os << "native '" << node->nativeName() << "';" << std::endl;
	}


	virtual void visitForNode(mathvm::ForNode * node) {
		os << "for (" << node->var()->name() << " in ";
		node->inExpr()->visit(this);
		os << ") {" << std::endl;
		node->body()->visit(this);
		os << "}" << std::endl;
	}


	virtual void visitWhileNode(mathvm::WhileNode * node) {
		os << "while (";
		node->whileExpr()->visit(this);
		os << ") {" << std::endl;
		node->loopBlock()->visit(this);
		os << "}" << std::endl;
	}


	virtual void visitIfNode(mathvm::IfNode * node) {
		os << "if (";
		node->ifExpr()->visit(this);
		os << ") {" << std::endl;
		node->thenBlock()->visit(this);
		os << "}" << std::endl;

		if (node->elseBlock()) {
			os << std::endl << "else {" << std::endl;
			node->elseBlock()->visit(this);
			os << "}" << std::endl;
		}
	}


	virtual void visitReturnNode(mathvm::ReturnNode * node) {
		os << "return ";
		node->visitChildren(this);
		os << ";" << std::endl;
	}


	virtual void visitFunctionNode(mathvm::FunctionNode * node) {
		node->body()->visit(this);
	}


	virtual void visitCallNode(mathvm::CallNode * node) {
		os << node->name() << "(";

		for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
			if (i != 0) {
				os << ", ";
			}

			node->parameterAt(i)->visit(this);
		}

		os << ")";
	}


	virtual void visitPrintNode(mathvm::PrintNode * node) {
		os << "print (";

		for (uint32_t i = 0; i < node->operands(); ++i) {
			if (i != 0) {
				os << ", ";
			}

			node->operandAt(i)->visit(this);
		}

		os << ");" << std::endl;
	}

};


#endif /* _ASTPRINTER_HPP_ */
