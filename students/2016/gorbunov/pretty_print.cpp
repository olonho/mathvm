#include "pretty_print.h"

#include <sstream>
#include <map>

#include "parser.h"

using namespace mathvm;

const int PPrintVisitor::SPACE_NUM = 4;
const char PPrintVisitor::SPACE = ' ';
const char PPrintVisitor::BLOCK_OPEN = '{';
const char PPrintVisitor::BLOCK_END = '}';
const char PPrintVisitor::LPAREN = '(';
const char PPrintVisitor::RPAREN = ')';
const char PPrintVisitor::SEMICOLON = ';';
const char PPrintVisitor::COMMA = ',';

std::string PPrintVisitor::prepareStringLiteral(std::string str) {
	static const std::map<char, string> escapeCharStrs = {
		{ '\'', "\\'" }, { '\n', "\\n" }, { '\t', "\\t" }, 
		{ '\v', "\\v" }, { '\f', "\\f" }, { '\r', "\\r" }		
	};
	std::stringstream ss;
	ss << "'";
	for (auto c : str) {
		auto it = escapeCharStrs.find(c);
		if (it != escapeCharStrs.end()) {
			ss << it->second;
		} else {
			ss << c;
		}
	}
	ss << "'";
	return ss.str();
}

void PPrintVisitor::visitBinaryOpNode(BinaryOpNode* node) {
	_out << LPAREN;
	node->left()->visit(this);
	_out << " " << tokenOp(node->kind()) << " ";
	node->right()->visit(this);
	_out << RPAREN;
}

void PPrintVisitor::visitUnaryOpNode(UnaryOpNode* node) {
	_out << tokenOp(node->kind());
	node->operand()->visit(this);
}

void PPrintVisitor::visitStringLiteralNode(StringLiteralNode* node) {
	_out << prepareStringLiteral(node->literal());
}

void PPrintVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
	_out << node->literal();
}

void PPrintVisitor::visitIntLiteralNode(IntLiteralNode* node) {
	_out << node->literal();
}

void PPrintVisitor::visitLoadNode(LoadNode* node) {
	_out << node->var()->name();
}

void PPrintVisitor::visitStoreNode(StoreNode* node) {
	_out << node->var()->name() << " " << tokenOp(node->op()) << " ";
	node->value()->visit(this);
}

void PPrintVisitor::visitBlockNode(BlockNode* node) {
	const std::string blockIndent(_level * SPACE_NUM, ' ');

	if (_level > 0) {
		_out << BLOCK_OPEN << std::endl;
	}
	_level += 1;

	// printing scope
	
	Scope::VarIterator varIt(node->scope());
	while (varIt.hasNext()) {
		auto var = varIt.next();
		_out << blockIndent << typeToName(var->type()) << SPACE << var->name();
		_out << SEMICOLON << std::endl;
	}

	Scope::FunctionIterator funIt(node->scope());
	while (funIt.hasNext()) {
		auto fun = funIt.next();
		_out << blockIndent;
		fun->node()->visit(this);
		_out << std::endl;
	}

	// printing body
	
	for (uint32_t i = 0; i < node->nodes(); ++i) {
		_out << blockIndent;
        auto st = node->nodeAt(i);
        st->visit(this);
		if (!st->isBlockNode() && !st->isWhileNode() && !st->isIfNode() && !st->isForNode()) {
			_out << SEMICOLON;
		}
		_out << std::endl;
	}
	_level -= 1;
	if (_level > 0) {
		_out << std::string((_level - 1) * SPACE_NUM, ' ') << BLOCK_END;
	}
}

void PPrintVisitor::visitNativeCallNode(NativeCallNode* node) {
	_out << "native" << SPACE << node->nativeName();
}

void PPrintVisitor::visitForNode(ForNode* node) {
	_out << "for" << SPACE << LPAREN;
	_out << node->var()->name() << SPACE << "in" << SPACE;
	node->inExpr()->visit(this);
	_out << RPAREN;
	node->body()->visit(this);
}

void PPrintVisitor::visitWhileNode(WhileNode* node) {
	_out << "while" << SPACE << LPAREN;
	node->whileExpr()->visit(this);
	_out << RPAREN;
	node->loopBlock()->visit(this);
}

void PPrintVisitor::visitIfNode(IfNode* node) {
	_out << "if" << SPACE << LPAREN;
	node->ifExpr()->visit(this);
	_out << RPAREN << SPACE;
	node->thenBlock()->visit(this);
	if (node->elseBlock()) {
		_out << SPACE << "else" << SPACE;
		node->elseBlock()->visit(this);
	}
}

void PPrintVisitor::visitReturnNode(ReturnNode* node) {
	_out << "return";
	if (node->returnExpr() != nullptr) {
        _out << SPACE;
		node->returnExpr()->visit(this);
	}
}

void PPrintVisitor::visitFunctionNode(FunctionNode* node) {
	_out << "function" << SPACE << typeToName(node->returnType()) << SPACE;
	_out << node->name() << LPAREN;
	for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
		_out << typeToName(node->parameterType(i));
		_out << SPACE;
		_out << node->parameterName(i);
		if (i != node->parametersNumber() - 1) {
			_out << COMMA << SPACE;
		}
	}
	_out << RPAREN << SPACE;
	node->body()->visit(this);
}


void PPrintVisitor::visitCallNode(CallNode* node) {
	_out << node->name() << LPAREN;
	for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
		node->parameterAt(i)->visit(this);
		if (i != node->parametersNumber() - 1) {
			_out << COMMA << SPACE;
		}
	}
	_out << RPAREN;
}

void PPrintVisitor::visitPrintNode(PrintNode* node) {
	_out << "print" << LPAREN;
	for (uint32_t i = 0; i < node->operands(); ++i) {
		node->operandAt(i)->visit(this);
		if (i != node->operands() - 1) {
			_out << COMMA << SPACE;
		}
	}
	_out << RPAREN;
}

Status* PPrintTranslator::translate(const std::string& program, Code* *code) {
	Parser parser;
	auto status = parser.parseProgram(program);

	if (status->isError()) {
		return status;
	}

	auto astTop = parser.top();
	PPrintVisitor prettyPrinter(_out);
	auto programAst = astTop->node()->body();
	programAst->visit(&prettyPrinter);

	return status;
}
