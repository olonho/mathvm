//
//  AstPrinter.h
//  VM_1
//
//  Created by Hatless Fox on 9/21/13.
//  Copyright (c) 2013 WooHoo. All rights reserved.
//

#ifndef __AstPrinter__
#define __AstPrinter__

#include <iostream>
#include "visitors.h"
#include "ast.h"
#include <cmath>
#include <stack>
#include <algorithm>

using namespace mathvm;

class AstPrinter : public AstBaseVisitor {
public:
	
	AstPrinter(std::ostream& ostream = std::cout):
		_padding(""), _ostream(ostream) {}
       	virtual ~AstPrinter() {}
	
	inline void print(AstNode *node) { node->visit(this); }
	
private: // methods
	
#pragma mark - Visitors
	
	virtual void visitBlockNode(BlockNode* node);
	void printVariableDeclaration(AstVar* var);
	void printFunctionDeclaration(AstFunction* func);
	
	virtual void visitPrintNode(PrintNode* node);
	virtual void visitLoadNode(LoadNode* node);
	virtual void visitStoreNode(StoreNode* node);
	virtual void visitReturnNode(ReturnNode* node);
	virtual void visitBinaryOpNode(BinaryOpNode* node);
	virtual void visitUnaryOpNode(UnaryOpNode* node);
	virtual void visitStringLiteralNode(StringLiteralNode* node);
	virtual void visitDoubleLiteralNode(DoubleLiteralNode* node);
	virtual void visitIntLiteralNode(IntLiteralNode* node);
	virtual void visitIfNode(IfNode* node);
	virtual void visitWhileNode(WhileNode* node);
	virtual void visitForNode(ForNode* node);
	virtual void visitCallNode(CallNode* node);
	virtual void visitNativeCallNode(NativeCallNode* node);
	
#pragma mark - Common Helpers
	
	//TODO: deny cp ctor and assignement oprator
	void enterStatementNode();
	void leaveStatementNode(bool separatorIsRequired = true);
	
	inline void logSpace() { log(" ");}
	inline void logSeparator() { log(";", true); }
	inline void logPadding() { log(_padding.c_str()); }
	
	void log(string const & msg, bool withLE = false);
	void log(const char * msg, bool withLE = false);

private: // variables
	std::string _padding;
	std::ostream& _ostream;
	
	std::stack<unsigned> _exprDepthPerBlock;
	std::stack<int> _binOpPriorities;
};

#endif /* defined(AstPrinter) */
