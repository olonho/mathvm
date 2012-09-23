/*
 * ast_printer.h
 *
 *  Created on: Sep 23, 2012
 *      Author: Alexander Opeykin (alexander.opeykin@gmail.com)
 */

#ifndef AST_PRINTER_H_
#define AST_PRINTER_H_


#include "ast.h"

namespace mathvm {

class AstPrinter : public AstVisitor {
public:
	AstPrinter(ostream& ostrm) : _ostrm(ostrm) {
	}

	~AstPrinter();

#define VISITOR_FUNCTION_DECL(type, name) \
	virtual void visit##type(type* node);

	FOR_NODES(VISITOR_FUNCTION_DECL)
#undef VISITOR_FUNCTION_DECL

private:
	void printSubNodes(BlockNode* node);
	void printFuncDecl(Scope* scope);
	void printVarDecl(Scope* scope);

private:
    ostream& _ostrm;
};


} // namespace mathvm


#endif /* AST_PRINTER_H_ */
