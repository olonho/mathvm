#include "parser.h"
#include "visitors.h"

#include <cstdio>
#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>
#include <cstdlib>

using namespace mathvm;

class AstPrinter : public AstBaseVisitor {
public:
	AstPrinter() : indent(0), needSeparator(true) {}
	virtual ~AstPrinter() {}

	void visitBinaryOpNode(BinaryOpNode* node) {
		std::cout << "(";
		node->left()->visit(this);
		std::cout << " " << tokenOp(node->kind()) << " ";
		node->right()->visit(this);
		std::cout << ")";
	}   

	void visitUnaryOpNode(UnaryOpNode* node) {
		std::cout << "(" << tokenOp(node->kind());
		node->operand()->visit(this);
		std::cout << ")";
	}

	void replace(std::string& str, const std::string& oldStr, const std::string& newStr) {
	  size_t pos = 0;
	  while((pos = str.find(oldStr, pos)) != std::string::npos) {
	     str.replace(pos, oldStr.length(), newStr);
	     pos += newStr.length();
	  }
	}

	void visitStringLiteralNode(StringLiteralNode* node) {
		string literal = node->literal();
		replace(literal, "\\", "\\\\");
		replace(literal, "\n", "\\n");
		replace(literal, "\t", "\\t");
		replace(literal, "\r", "\\r");
		std::cout << "\'" << literal << "\'";
	}

	void visitDoubleLiteralNode(DoubleLiteralNode* node) {
		std::cout << node->literal();
	}

	void visitIntLiteralNode(IntLiteralNode* node) {
		std::cout << node->literal();
	}

	void visitLoadNode(LoadNode* node) {
		std::cout << node->var()->name();
	}

	void visitStoreNode(StoreNode* node) {
		std::cout << node->var()->name() << " " << tokenOp(node->op()) << " ";
		node->value()->visit(this);
	}

	void visitForNode(ForNode* node) {
		std::cout << "for (" << node->var()->name() << " in ";
		node->inExpr()->visit(this);
		std::cout << ")" << std::endl;
		node->body()->visit(this);
		if (!node->body()->isBlockNode()) {
			needSeparator = false;
		}
	}   

	void visitWhileNode(WhileNode* node) {
		std::cout << "while (";
		node->whileExpr()->visit(this);
		std::cout << ")" << std::endl;
		node->loopBlock()->visit(this);
	}

	void visitIfNode(IfNode* node) {
		std::cout << "if (";
		node->ifExpr()->visit(this);
		std::cout << ")" << std::endl;
		node->thenBlock()->visit(this);
		if (node->elseBlock()) {
			std::cout << std::endl << "else" << std::endl;
			node->elseBlock()->visit(this);
			needSeparator = !node->elseBlock()->isBlockNode();
		} 
		else {
			needSeparator = !node->thenBlock()->isBlockNode();
		}
	}
	
	void visitBlockNode(BlockNode* node) {
		if (node->position() != 0) {
			std::cout << string(3*indent, ' ') << "{" << std::endl;
			indent++;
		}
	
		string indentStr(3*indent, ' ');

		Scope::VarIterator vit(node->scope());
		int variables = 0;
		while(vit.hasNext()) {
			AstVar* var = vit.next();
			std::cout << indentStr;
			std::cout << typeToName(var->type()) << " " << var->name() << ";" << std::endl;
			variables++;
		}
		if (variables != 0) std::cout << std::endl;

		Scope::FunctionIterator fit(node->scope());
		int functions = 0;
		while(fit.hasNext()) {
			AstFunction* func = fit.next();
			std::cout << indentStr;
			func->node()->visit(this);
			functions ++;
		}
		if (functions != 0) std::cout << std::endl;

		for (uint32_t i = 0; i < node->nodes(); ++i) {
			std::cout << indentStr;
			node->nodeAt(i)->visit(this);
			if (needSeparator) {
				std::cout << ";" << std::endl;
			}
			else {
				needSeparator = !needSeparator;
				std::cout << std::endl;
			}
		}

		if (node->position() != 0) {
			if (indent > 0) {
				indent--;
			}
		
			std::cout << string(3*indent, ' ') << "}" << std::endl;
		}
		//needSeparator = false;
	}

	void visitFunctionNode(FunctionNode* node) {
		if (node->name() != "<top>") 
		{
			std::cout << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
			for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
				std::cout << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
				if (i != node->parametersNumber() - 1) {
					std::cout << ", ";
				}
			}
			std::cout << ") ";
		}

		BlockNode* body = node->body();
		if (body != NULL) {
			if (body->nodes() != 0 && body->nodeAt(0) != NULL) {
				if (body->nodeAt(0)->isNativeCallNode()) {
					body->nodeAt(0)->visit(this);
					std::cout << ";" << std::endl;
				}
				else {
					body->visit(this);
				}
			}
		}

	}

	void visitReturnNode(ReturnNode* node) {
		std::cout << "return ";
		if(node->returnExpr() != NULL) {
			node->returnExpr()->visit(this);
		}
	}

	void visitCallNode(CallNode* node) {
		std::cout << node->name() << "(";
		for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
			node->parameterAt(i)->visit(this);
			if (i + 1 != node->parametersNumber()) {
				std::cout << ", ";
			}

		}
		std::cout << ")";
	}

	void visitNativeCallNode(NativeCallNode* node) {
		std::cout << "native \'" << node->nativeName() << "\'";
	}

	void visitPrintNode(PrintNode* node) {
		std::cout << "print(";
		for (uint32_t i = 0; i < node->operands(); ++i) {
			node->operandAt(i)->visit(this);
			if (i != node->operands() - 1) {
				std::cout << ", ";
			}
		}
		std::cout << ")";
	}

private:
	size_t indent;
	bool needSeparator;
};



int main(int argc, char const *argv[]) {

	if (argc != 2) {
		std::cerr << "Usage : " << argv[0] << " source" << std::endl;
		exit(1);
	}

	std::ifstream text_stream(argv[1]);
	std::string text((std::istreambuf_iterator<char>(text_stream)), std::istreambuf_iterator<char>());

	Parser parser;
	Status* status = parser.parseProgram(text);

	if (status && status->isError()) {
		std::cerr << "Error occured : " << status->getError() << std::endl;
	}
	else {
		AstPrinter printer;
		parser.top()->node()->visit(&printer);
	}

	delete status;

	return 0;
}