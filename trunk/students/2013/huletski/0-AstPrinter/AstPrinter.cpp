//
//  AstPrinter.cpp
//  VM_1
//
//  Created by Hatless Fox on 9/22/13.
//  Copyright (c) 2013 WooHoo. All rights reserved.
//

#include "AstPrinter.h"

void AstPrinter::visitBlockNode(BlockNode* node) {
	bool blockIsNotOuter = _exprDepthPerBlock.size();
	if (blockIsNotOuter) {
		log("{", true);
		_padding.push_back('\t'); //indent w tabs is lame but easier to impl
	}
	_exprDepthPerBlock.push(0);
	
	Scope::VarIterator vi(node->scope());
	while (vi.hasNext()) { printVariableDeclaration(vi.next()); }
	
	Scope::FunctionIterator fi(node->scope());
	while (fi.hasNext()) { printFunctionDeclaration(fi.next()); }
	
	AstBaseVisitor::visitBlockNode(node);
	
	_exprDepthPerBlock.pop();
	if (blockIsNotOuter) {
		//std::string::pop_back() is not defined till C++11 so use the hack
		_padding.erase(_padding.end() - 1, _padding.end());
		
		logPadding();
		log("}", true);
	}
}

void AstPrinter::printVariableDeclaration(AstVar* var) {
	enterStatementNode();
	
	log(typeToName(var->type()));
	logSpace();
	log(var->name());
	
	leaveStatementNode();
}

void AstPrinter::printFunctionDeclaration(AstFunction* func) {
	enterStatementNode();
	
	log("function ");
	log(typeToName(func->returnType()));
	logSpace();
	log(func->name());
	log(" (");
	uint32_t paramNum = func->parametersNumber();
	for (uint32_t paramIndex = 0; paramIndex < paramNum; ++paramIndex) {
		log(typeToName(func->parameterType(paramIndex)));
		logSpace();
		log(func->parameterName(paramIndex));
		if (paramIndex + 1 != paramNum) { log(", "); }
	}
	log(") ");
	
	bool firstNodeIsNC = func->node()->body()
	&& func->node()->body()->nodes()
	&& func->node()->body()->nodeAt(0)->isNativeCallNode();
	if (firstNodeIsNC) {
		func->node()->body()->nodeAt(0)->visit(this);
		logSeparator();
	} else {
		func->node()->body()->visit(this);
	}
	
	leaveStatementNode(false);
}

void AstPrinter::visitPrintNode(PrintNode* node) {
	enterStatementNode();
	
	log("print(");
	uint32_t ind = 0;
	while (node->operands()) {
		node->operandAt(ind)->visit(this);
		if (++ind >= node->operands()) { break;	}
		log(", ");
	}
	log(")");
	
	leaveStatementNode();
}

void AstPrinter::visitLoadNode(LoadNode* node) {
	enterStatementNode();
	
	log(node->var()->name());
	
	leaveStatementNode();
}

void AstPrinter::visitStoreNode(StoreNode* node) {
	enterStatementNode();
	
	log(node->var()->name());
	logSpace();
	log(tokenOp(node->op()));
	logSpace();
	AstBaseVisitor::visitStoreNode(node);
	
	leaveStatementNode();
}

void AstPrinter::visitReturnNode(ReturnNode* node) {
	enterStatementNode();
	
	log("return");
	if (node->returnExpr()) { logSpace(); }
	AstBaseVisitor::visitReturnNode(node);
	
	leaveStatementNode();
}

void AstPrinter::visitBinaryOpNode(BinaryOpNode* node) {
	enterStatementNode();
	int priority = tokenPrecedence(node->kind());
	int prevPriority = _binOpPriorities.size() > 0 ? _binOpPriorities.top() : 0;
	bool bracesShouldBePrinted = priority < prevPriority;
	_binOpPriorities.push(priority);
	
	if (bracesShouldBePrinted) { log("("); }
	node->left()->visit(this);
	if (node->kind() != tRANGE) { logSpace(); }
	log(tokenOp(node->kind()));
	if (node->kind() != tRANGE) { logSpace(); }
	node->right()->visit(this);
	if (bracesShouldBePrinted) { log(")"); }
	
	_binOpPriorities.pop();
	leaveStatementNode();
}

void AstPrinter::visitUnaryOpNode(UnaryOpNode* node) {
	enterStatementNode();
	
	//NB: extra braces can be removed either
	//    with introducing separate unary minus or
	//    with addition of surrogate max_priority
	log(tokenOp(node->kind()));
	log("(");
	node->operand()->visit(this);
	log(")");
	
	leaveStatementNode();
}

void AstPrinter::visitStringLiteralNode(StringLiteralNode* node) {
	enterStatementNode();
	
	//escape string first
	std::string literal;
	for (string::const_iterator it = node->literal().begin();
		 it != node->literal().end(); it++) {
		switch (*it) {
			case '\n': literal += "\\n"; break;
			case '\t': literal += "\\t"; break;
			case '\r': literal += "\\r"; break;
			case '\\': literal += "\\\\"; break;
			case '\"': literal += "\\\""; break;
			case '\'': literal += "\\'"; break;
			default:
				literal.push_back(*it);
				break;
		}
	}
	log("\'");
	log(literal);
	log("\'");
	
	leaveStatementNode();
}

void AstPrinter::visitDoubleLiteralNode(DoubleLiteralNode* node) {
	enterStatementNode();
	
	double val = node->literal();
	_ostream << val;
	
	/*
	//emphasize doubleness for values w/o fractional part
	if (abs(val - round(val)) <= std::numeric_limits<double>::epsilon()) {
		_ostream << ".0";
	}
	*/
	leaveStatementNode();
}

void AstPrinter::visitIntLiteralNode(IntLiteralNode* node) {
	enterStatementNode();
	
	_ostream << node->literal();
	
	leaveStatementNode();
}

void AstPrinter::visitIfNode(IfNode* node) {
	enterStatementNode();
	
	log("if (");
	node->ifExpr()->visit(this);
	log(") ");
	node->thenBlock()->visit(this);
	if (node->elseBlock()) {
		logPadding();
		log("else ");
		node->elseBlock()->visit(this);
	}
	
	leaveStatementNode(false);
}

void AstPrinter::visitWhileNode(WhileNode* node) {
	enterStatementNode();
	
	log("while (");
	node->whileExpr()->visit(this);
	log(") ");
	node->loopBlock()->visit(this);
	
	leaveStatementNode(false);
}

void AstPrinter::visitForNode(ForNode* node) {
	enterStatementNode();
	
	log("for (");
	log(node->var()->name());
	log(" in ");
	node->inExpr()->visit(this);
	log(") ");
	node->body()->visit(this);
	
	leaveStatementNode(false);
}

void AstPrinter::visitCallNode(CallNode* node) {
	enterStatementNode();
	
	log(node->name());
	log("(");
	uint32_t paramIndex = 0;
	while (node->parametersNumber()) {
		node->parameterAt(paramIndex)->visit(this);
		if (++paramIndex >= node->parametersNumber()) { break; }
		log(",");
	}
	log(")");
	
	leaveStatementNode();
}

void AstPrinter::visitNativeCallNode(NativeCallNode* node) {
	enterStatementNode();
	
	log("native \'");
	log(node->nativeName());
	
	log("'");
	
	leaveStatementNode();
}

#pragma mark - Common Helpers

void AstPrinter::enterStatementNode() {
	if (_exprDepthPerBlock.top()++ == 0) { logPadding(); }
}
void AstPrinter::leaveStatementNode(bool separatorIsRequired) {
	assert(_exprDepthPerBlock.top() != 0);
	if (--_exprDepthPerBlock.top() == 0 && separatorIsRequired) {
		logSeparator();
	}
}

void AstPrinter::log(string const & msg, bool withLE) {
	log(msg.c_str(), withLE);
}

void AstPrinter::log(const char * msg, bool withLE) {
	_ostream << msg;
	if (withLE) { _ostream << std::endl; }
}