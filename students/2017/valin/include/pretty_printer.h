#pragma once

#include <map>
#include <string>
#include <iostream>

#include "ast.h"

namespace my {

class AstPrinter : public mathvm::AstVisitor, public mathvm::Translator {
	mathvm::AstFunction * program;
	mathvm::FunctionNode * top;
	std::ostream& code;
	int tabs = 0;
	bool in_expression = false;

	class Braces {
		AstPrinter * printer;
	public:
		Braces(AstPrinter * printer) {
			this->printer = printer;
			printer->code << "(";
		}

		~Braces() {
			printer->code << ")";
		}
	};

	class CurveBraces {
		AstPrinter * printer;
	public:
		CurveBraces(AstPrinter * printer) {
			this->printer = printer;
			printer->code << "{\n";
		}

		~CurveBraces() {
			for (int i = 0; i < printer->tabs; ++i) {
				printer->code << "    ";
			}

			printer->code << "}";
		}
	};

	class Expression {
		AstPrinter * printer;
	public:
		Expression(AstPrinter * printer) {
			this->printer = printer;
			this->printer->in_expression = true;
		}

		~Expression() {
			this->printer->in_expression = false;
		}
	};

	class Tab {
		AstPrinter * printer;
	public:
		Tab(AstPrinter * printer) {
			this->printer = printer;
			printer->tabs += 1;
		}

		~Tab() {
			printer->tabs -= 1;
		}
	};

	class Statement {
		AstPrinter * printer;
	public:
		Statement(AstPrinter * printer) {
			this->printer = printer;
			for (int i = 0; i < printer->tabs; ++i) {
				printer->code << "    ";
			}
		}

		~Statement() {
			printer->code << "\n";
		}
	};

	void define(mathvm::AstFunction * fun);
	void define(mathvm::AstVar * var);
public:
	AstPrinter(std::ostream& code = std::cout) : code(code) {}

#define VISITOR_FUNCTION(type, name) \
	virtual void visit##type(mathvm::type* node);

	FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
	

	virtual mathvm::Status * translate(const std::string& source, mathvm::Code ** program) override;
};

}
