#include "Converter.h"
namespace mathvm {

Converter::~Converter() {

}

void Converter::printSource(AstFunction * top) {
	visitBlockBodyNode(top->node()->body());
}

void Converter::visitBlockBodyNode(BlockNode * node) {
	for (Scope::VarIterator iter(node->scope()); iter.hasNext();) {
		AstVar * var = iter.next();
		out << typeToName(var->type()) << " " << var->name() << ";" << endl;
	}

	for (Scope::FunctionIterator iter(node->scope());iter.hasNext();) {
		iter.next()->node()->visit(this);
		out << endl;
	}

	for (uint32_t i = 0; i != node->nodes(); ++i) {
		node->nodeAt(i)->visit(this);
	}
}

void Converter::visitBlockNode(BlockNode * node) {
	out << "{" << endl;
	visitBlockBodyNode(node);
	out << "}" << endl;
}

void Converter::visitBinaryOpNode(BinaryOpNode * node) {
	out << "(";
	node->left()->visit(this);
	out << " " << tokenOp(node->kind()) << " ";
	node->right()->visit(this);
	out << ")";
}

void Converter::visitUnaryOpNode(UnaryOpNode * node) {
	out << "(";
	out << " " << tokenOp(node->kind()) << " ";
	node->operand()->visit(this);
	out << ")";
}

void Converter::visitCallNode(CallNode * node) {
	out << node->name() << "(";
	for (uint32_t i = 0; i != node->parametersNumber(); ++i) {
		node->parameterAt(i)->visit(this);
		if (i < node->parametersNumber() - 1) {
			out <<", ";
		}
	}
	out << ")";
}

void Converter::visitDoubleLiteralNode(DoubleLiteralNode * node) {
	out << node->literal();
}

void Converter::visitForNode(ForNode * node) {
	out << "for (" << node->var()->name() << " in ";
	node->inExpr()->visit(this);
	out << ")";
	node->body()->visit(this);
}

void Converter::visitFunctionNode(FunctionNode * node) {
	out << "function " << typeToName(node->returnType()) << " "
			<< node->name() << "(";
	for (uint32_t i = 0; i != node->parametersNumber(); ++i) {
		out << typeToName(node->parameterType(i)) << " "
				<< node->parameterName(i);
		if (i < node->parametersNumber() - 1) {
			out << ", ";
		}
	}
	out <<")";
	if(node->body()->nodeAt(0)->isNativeCallNode()) {
		node->body()->nodeAt(0)->visit(this);
	} else {
		node->body()->visit(this);
	}
}

void Converter::visitIfNode(IfNode * node) {
	out << "if (";
	node->ifExpr()->visit(this);
	out << ")";
	node->thenBlock()->visit(this);
	if (node->elseBlock() != NULL) {
		out << "else";
		node->elseBlock()->visit(this);
	}
}

void Converter::visitIntLiteralNode(IntLiteralNode * node) {
	out << node->literal();
}

void Converter::visitLoadNode(LoadNode * node) {
	out << node->var()->name();
}

void Converter::visitNativeCallNode(NativeCallNode * node) {
	out << "native '" << node->nativeName() <<"';";
}

void Converter::visitPrintNode(PrintNode * node) {
	out << "print (";
	for (uint32_t i = 0; i != node->operands(); ++i) {
		node->operandAt(i)->visit(this);
		if (i < node->operands() - 1) {
			out <<", ";
		}
	}
	out << ");" << endl;
}

void Converter::visitReturnNode(ReturnNode * node) {
	out << "return ";
	if (node->returnExpr() != NULL) {
		node->returnExpr()->visit(this);
	}
	out << ";" << endl;
}

void Converter::visitStoreNode(StoreNode * node) {
	out << node->var()->name() << tokenOp(node->op());
	node->value()->visit(this);
	out << ";" << endl;
}

void Converter::visitStringLiteralNode(StringLiteralNode * node) {
	out << "\'";
	string result;
	for (string::const_iterator it = node->literal().begin(); it != node->literal().end(); ++it) {
		switch (*it) {
		case '\n':
			result += "\\n";
			break;
		case '\t':
			result += "\\t";
			break;
		case '\0':
			result += "\\0";
			break;
		case '\r':
			result += "\\r";
			break;
		case '\\':
			result += "\\\\";
			break;
		default:
			result += *it;
		}
	}
	out << result << "\'";
}

void Converter::visitWhileNode(WhileNode * node) {
	out << "while (";
	node->whileExpr()->visit(this);
	out << ") ";
	node->loopBlock()->visit(this);
}
}
