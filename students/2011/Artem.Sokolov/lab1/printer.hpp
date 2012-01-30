#ifndef _PRINTER_HPP_
#define _PRINTER_HPP_

#include <ostream>
#include <ast.h>
#include <iomanip>

class Printer: public mathvm::AstVisitor {
	std::ostream &output_stream;
	unsigned int inception;
public:
	Printer(std::ostream &output_stream): output_stream(output_stream), inception(0) {}

	virtual void visitBinaryOpNode(mathvm::BinaryOpNode* node) {
		++inception;
		output_stream << "(";
		node->left()->visit(this);
		output_stream << tokenOp(node->kind());
		node->right()->visit(this);
		output_stream << ")";
		--inception;
	}

	virtual void visitUnaryOpNode(mathvm::UnaryOpNode* node) {
		++inception;
		output_stream << "(";
		output_stream << tokenOp(node->kind());
		node->visitChildren(this);
		output_stream << ")";
		--inception;
	}

	virtual void visitStringLiteralNode(mathvm::StringLiteralNode* node) {
		output_stream << "'";
		for (unsigned int i = 0; i < node->literal().length(); ++i) {
			switch (node->literal()[i]) {
			case '\n':
				output_stream << "\\n";
				break;
			case '\t':
				output_stream << "\\t";
				break;
			case '\\':
				output_stream << "\\\\";
				break;
			case '\'':
				output_stream << "\\\'";
				break;
			case '\"':
				output_stream << "\\\"";
				break;
			default:
				output_stream << node->literal()[i];
				break;
			}
		}
		output_stream << "'";
	}

	virtual void visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
		output_stream.setf(std::ios_base::scientific);
		//output_stream << std::setprecision(1) << node->literal();
		output_stream << node->literal();
		//output_stream.unsetf(std::ios_base::scientific);;
	}

	virtual void visitIntLiteralNode(mathvm::IntLiteralNode* node) {
		output_stream << node->literal();
	}

	virtual void visitLoadNode(mathvm::LoadNode* node) {
		output_stream << node->var()->name();
	}

	virtual void visitStoreNode(mathvm::StoreNode* node) {
		++inception;
		output_stream << node->var()->name();
		output_stream << tokenOp(node->op());
		node->visitChildren(this);
		output_stream << ";\n";
		--inception;
	}

	virtual void visitForNode(mathvm::ForNode* node) {
		++inception;
		output_stream << "for (" << node->var()->name() << " in ";
		node->inExpr()->visit(this);
		output_stream << ")";
		--inception;
		output_stream << " {\n";
		node->body()->visit(this);
		output_stream << "}\n";
	}

	virtual void visitWhileNode(mathvm::WhileNode* node) {
		++inception;
		output_stream << "while ";
		node->whileExpr()->visit(this);
		--inception;
		output_stream << " {\n";
		node->loopBlock()->visit(this);
		output_stream << "}\n";
	}

	virtual void visitIfNode(mathvm::IfNode* node) {
		++inception;
		output_stream << "if ";
		node->ifExpr()->visit(this);
		--inception;
		output_stream << " {\n";
		node->thenBlock()->visit(this);
		output_stream << "} ";
		if (node->elseBlock()) {
			output_stream << "else {\n";
			node->elseBlock()->visit(this);
			output_stream << "}";
		}
		output_stream << "\n";
	}

	virtual void visitBlockNode(mathvm::BlockNode* node) {
		mathvm::Scope::VarIterator variable_iterator(node->scope());
		while (variable_iterator.hasNext()) {
			mathvm::AstVar *ast_var = variable_iterator.next();
			output_stream << mathvm::typeToName(ast_var->type()) << " ";
			output_stream << ast_var->name() << ";\n";
		}

		mathvm::Scope::FunctionIterator function_iterator(node->scope());
		while (function_iterator.hasNext()) {
			mathvm::AstFunction *ast_function = function_iterator.next();
			output_stream << "function " << mathvm::typeToName(ast_function->returnType());
			output_stream << " " << ast_function->name() << "(";

			if (ast_function->parametersNumber() > 0) {
				for (unsigned int i = 0; i < ast_function->parametersNumber() - 1; ++i) {
					output_stream << mathvm::typeToName(ast_function->node()->signature()[i + 1].first) << " ";
					output_stream << ast_function->node()->signature()[i + 1].second;
					output_stream << ",";
				}
				output_stream << mathvm::typeToName(ast_function->node()->signature()[ast_function->parametersNumber()].first) << " ";
				output_stream << ast_function->node()->signature()[ast_function->parametersNumber()].second;
			}
			output_stream << ") ";

			if (ast_function->node()->body()->nodes() != 0 && ast_function->node()->body()->nodeAt(0)->isNativeCallNode())
			{
				ast_function->node()->body()->nodeAt(0)->visit(this);
			} else {
				output_stream << "{\n";
				ast_function->node()->visitChildren(this);
				output_stream << "}\n";
			}
		}

		node->visitChildren(this);
	}

	virtual void visitFunctionNode(mathvm::FunctionNode* node) {
		node->visitChildren(this);
	}

	virtual void visitReturnNode(mathvm::ReturnNode* node) {
		++inception;
		if (node->returnExpr()) {
			output_stream << "return ";
			node->returnExpr()->visit(this);
			output_stream << ";\n";
		}
		--inception;
	}

	virtual void visitCallNode(mathvm::CallNode* node) {
		++inception;
		output_stream << node->name() << "(";
		if (node->parametersNumber() > 0) {
			for (uint32_t i = 0; i < node->parametersNumber() - 1; i++) {
				node->parameterAt(i)->visit(this);
				output_stream << ",";
			}
			node->parameterAt(node->parametersNumber() - 1)->visit(this);
		}
		output_stream << ")";
		if (inception == 1) {
			output_stream << ";\n";
		}
		--inception;
	}

	virtual void visitNativeCallNode(mathvm::NativeCallNode* node) {
		output_stream << "native '" << node->nativeName() << "';" << std::endl;
	}

	virtual void visitPrintNode(mathvm::PrintNode* node) {
		++inception;
		output_stream << "print(";
		for (uint32_t i = 0; i < node->operands() - 1; ++i) {
			node->operandAt(i)->visit(this);
			output_stream << ",";
		}
		node->operandAt(node->operands() - 1)->visit(this);
		output_stream << ");\n";
		--inception;
	}
};

#endif // _PRINTER_HPP_
