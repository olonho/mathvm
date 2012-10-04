/* 
 * File:   PrinterVisitor.h
 * Author: yarik
 *
 * Created on October 2, 2012, 6:18 PM
 */

#ifndef PRINTERVISITOR_H
#define	PRINTERVISITOR_H

#include <visitors.h>
#include <parser.h>

using namespace mathvm;

class PrinterVisitor: public AstVisitor {
public:
    PrinterVisitor();
    virtual ~PrinterVisitor();
    void print(const string&);
    
#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node );
FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
    
private:
    void visitBlockNodeBody(BlockNode* node);
    void visitScopeVars(Scope* scope);
    void visitScopeFuns(Scope* scope);
    void visitScopeAttr(Scope* scope);
    
    void init(const string&);
    
    Parser _parser;
};

#endif	/* PRINTERVISITOR_H */

