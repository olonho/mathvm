/* 
 * File:   type_calculator.h
 * Author: griver
 *
 * Created on 26 Январь 2013 г., 17:10
 */

#ifndef TYPE_CALCULATOR_H
#define	TYPE_CALCULATOR_H

#include <map> 
#include <deque>
#include <stack>
#include "ast.h"
#include "parser.h"
#include "Utilities.h"

using std::stack;


namespace mathvm {
 
    class TypeCalculator : public AstVisitor {

    private: // private fields
        AstTypeMap& myMap;
        stack<Scope*> myScopes;
        ostream& myErr;

    private: // private methods       
        VarType getBinaryOpType(VarType typeA, VarType typeB, TokenKind kind);
        void visitAstFunction(AstFunction *function);

    public: // public methods

        TypeCalculator(AstTypeMap &typeMap, ostream &errorStream);	

        //!!
        #define VISITOR_FUNCTION(type, name) \
        virtual void visit##type(type *node);

        FOR_NODES(VISITOR_FUNCTION)
        #undef VISITOR_FUNCTION

        void calculateTypes(Parser &parser);

        virtual ~TypeCalculator();

    };

}
#endif	/* TYPE_CALCULATOR_H */

