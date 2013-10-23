#include "printVisitor.h"

PrintVisitor::PrintVisitor(std::ostream& _out, int _indentSize): AstVisitor(), out(_out), indentSize(_indentSize), indent(0), needSemicolon(true) {}
PrintVisitor::PrintVisitor(std::ostream& _out): AstVisitor(), out(_out), indentSize(2), indent(0), needSemicolon(true){}

void PrintVisitor::printIndent() {
	for (int i = 0; i < indent; ++i) {
		for (int j = 0; j < indentSize; ++j) {
			out << " ";
		}
	}
}

void PrintVisitor::visitBinaryOpNode(BinaryOpNode* node) {
	out << "(";
	node->left()->visit(this);
	out << " " << tokenOp(node->kind()) << " ";
	node->right()->visit(this);
	out << ")";
}

void PrintVisitor::visitUnaryOpNode(UnaryOpNode* node) {
	out << "(" << tokenOp(node-> kind());
	node->operand()->visit(this);
	out << ")";
}

void replaceAll(std::string& src, const std::string& from, const std::string& to) {
	if(from.empty()) {
	        return;
	}
	size_t pos = 0;
	while((pos = src.find(from, pos)) != std::string::npos) {
		src.replace(pos, from.length(), to);
		pos += to.length();
	}
}

void PrintVisitor::visitStringLiteralNode(StringLiteralNode* node) {
	std::string literal = node->literal();
	replaceAll(literal, "\\", "\\\\");
	replaceAll(literal, "\n", "\\n");
	replaceAll(literal, "\t", "\\t");
	replaceAll(literal, "\r", "\\r");
	out << "\'" << literal << "\'";
}

void PrintVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
	out << node->literal();
}

void PrintVisitor::visitIntLiteralNode(IntLiteralNode* node) {
	out << node->literal();
}

void PrintVisitor::visitLoadNode(LoadNode* node) {
	out << node->var()->name();
}

void PrintVisitor::visitStoreNode(StoreNode* node) {
	out << node->var()->name() << " " << tokenOp(node->op()) << " ";
	node->value()->visit(this);
}

void PrintVisitor::visitForNode(ForNode* node) {
	out << "for (" << node->var()->name() << " in ";
	node->inExpr()->visit(this);
	out << ") {" << std::endl;
	++indent;
	node->body()->visit(this); //body should make new line of its own
	--indent;
	printIndent();
	out << "}";
	needSemicolon = false;
}

void PrintVisitor::visitWhileNode(WhileNode* node) {
	out << "while " << "(";
	node->whileExpr()->visit(this);	
	out << ") {" << std::endl;
	++indent;
	node->loopBlock()->visit(this); //body should make new line of its own
	--indent;
	printIndent();
	out << "}";
	needSemicolon = false;
}

void PrintVisitor::visitIfNode(IfNode* node) {
	out << "if " << "(";
	node->ifExpr()->visit(this);
	out << ") {" << std::endl;
	++indent;
	node->thenBlock()->visit(this); //body should make new line of its own
	--indent;
	printIndent();
	AstNode* elseBlock = node->elseBlock();	
	if (elseBlock) {
		out << "} else {" << std::endl;
		++indent;
		elseBlock->visit(this); //body should make new line of its own
		--indent;
		printIndent();
	}
	out << "}";
	needSemicolon = false;
}

void PrintVisitor::visitBlockNode(BlockNode* node) {
	Scope* scope = node->scope();
	//declare variables
	int counter = 0;
	for (Scope::VarIterator varIt = Scope::VarIterator(scope); varIt.hasNext();) {
	    AstVar* variable = varIt.next();
	    printIndent();
	    out << typeToName(variable->type()) << " " << variable->name() << ";" << std::endl;
	    counter++;
	}
	
	if (counter > 0) {
	    out << std::endl;
	}
	
	//define functions
	counter = 0;
	for (Scope::FunctionIterator funIt = Scope::FunctionIterator(scope); funIt.hasNext();) {
	    AstFunction* function = funIt.next();
	    printIndent();
	    function->node()->visit(this);
	    out << std::endl;
	    counter++;
	}
	
	if (counter > 0) {
		out << std::endl;
	}

	for (size_t i = 0; i < node->nodes(); ++i) {
		printIndent();
		node->nodeAt(i)->visit(this);
		if (needSemicolon) {
		    out << ";";
		} else {
		    needSemicolon = true;
		}
		out << std::endl;
	}
}

void PrintVisitor::visitFunctionNode(FunctionNode* node) {
	if (node->name() == "<top>") {
	    //this is entry point
	    node->body()->visit(this);
	    return;
	}
	printIndent();
	out << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
	for (size_t i = 0; i < node->parametersNumber(); ++i) {
		if (i != 0) {
			out << ", ";
		}
		out << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
	}
	out << ") ";
	BlockNode* body = node->body();
	if (body->nodeAt(0)->isNativeCallNode()) {
		//this is native function
		body->nodeAt(0)->visit(this);
		
	} else {
		//this is usual function
		out << "{" << std::endl;
		++indent;
		body->visit(this);
		--indent;
		printIndent();
		out << "}" << std::endl;
	}
}

void PrintVisitor::visitReturnNode(ReturnNode* node) {
	out << "return";
	AstNode* returnExpr = node->returnExpr();
	if (returnExpr) {
		out << " ";
		returnExpr->visit(this);
	}
}

void PrintVisitor::visitCallNode(CallNode* node) {
	out << node->name() << "(";
	for (size_t i = 0; i < node->parametersNumber(); ++i) {
		if (i != 0) {
			out << ", ";
		}
		node->parameterAt(i)->visit(this);
	}
	out << ")";
}

void PrintVisitor::visitNativeCallNode(NativeCallNode* node) {
	out << "native \'" << node->nativeName() << "\';" << std::endl;	
}

void PrintVisitor::visitPrintNode(PrintNode* node) {
	out << "print(";
	for (size_t i = 0; i < node->operands(); ++i) {
		if (i != 0) {
			out << ", ";
		}
		node->operandAt(i)->visit(this);
	}
	out << ")";
}
