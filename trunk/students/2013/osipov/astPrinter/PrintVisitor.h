/* 
 * File:   PrintVisitor.h
 * Author: stasstels
 *
 * Created on September 23, 2013, 3:20 PM
 */

#ifndef PRINTVISITOR_H
#define	PRINTVISITOR_H

#include "mathvm.h"
#include "ast.h"


namespace mathvm {

class PrintVisitor : public AstVisitor {
private:
    
    
    uint32_t tab_level;
    std::ostream& out;

    void printFunctions(Scope* scope) {
        for(auto it = Scope::FunctionIterator(scope); it.hasNext();) {
            it.next() -> node() -> visit(this);
        }
    }
    
    void printVariableDeclarations(Scope* scope) {
        std::string tab(tab_level * TAB_SIZE, SPACE);
        for(auto it = Scope::VarIterator(scope); it.hasNext();) {
            out << tab;
            printVariableDeclaration(it.next());
        }
        out << std::endl;
    }
    
    void printVariableDeclaration(AstVar* variable) {
        out << typeToName(variable -> type()) 
        << " " 
        << variable -> name() 
        << ";" 
        << std::endl;
    }
    
    static bool hasParens(TokenKind currentOperator, AstNode* expr);
    
    static const uint32_t TAB_SIZE;
    static const char SPACE;
    
    
public:
    
    explicit PrintVisitor(std::ostream& out);
    
    ~PrintVisitor() {}
    
    PrintVisitor(const PrintVisitor& orig) : out(orig.out) {}
    
    void visitBlockNodeInside(BlockNode* node);
    
    #define VISITOR_FUNCTION(type, name)            \
        virtual void visit##type(type* node);

        FOR_NODES(VISITOR_FUNCTION)
    #undef VISITOR_FUNCTION


};

}
#endif	/* PRINTVISITOR_H */

