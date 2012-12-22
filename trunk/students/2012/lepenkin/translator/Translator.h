/* 
 * File:   PrinterVisitor.h
 * Author: yarik
 *
 * Created on October 2, 2012, 6:18 PM
 */

#ifndef PRINTERVISITOR_H
#define	PRINTERVISITOR_H

#include <visitors.h>
#include <stack>
#include <map>
#include <stdlib.h>
#include <mathvm.h>
#include "CodeImpl.h"


using namespace mathvm;
using std::stack;




const bool DEBUG_MODE = true;

void WR_ERROR(const char*);
void WR_DEBUG(const char*);



class TranslatorVisitor: public AstVisitor {
public:
    TranslatorVisitor(Code* *code);
    virtual ~TranslatorVisitor();


    void translate(AstFunction*);

    
#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node );
FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
    
private:

    SmartCode* _code;

    stack<VarType> varStack;



    void storeVar(const AstVar*);

    void visitBlockNodeBody(BlockNode*);
    void visitScopeVars(Scope*);
    void visitScopeFuns(Scope*);
    void visitScopeAttr(Scope*);



};

#endif	/* PRINTERVISITOR_H */

