//
//  ast_analyzer.h
//
//  Created by Dmitriy on 9/27/12.
//  Copyright (c) 2012 Dmitriy. All rights reserved.
//

#ifndef __vm__ast_analyzer__
#define __vm__ast_analyzer__

#include <ostream>
#include <string>

#include "mathvm.h"
#include "ast.h"

using namespace mathvm;

using std::cout;
using std::endl;
using std::string;
using std::ostream;

class ASTAnalyzer : public AstVisitor {
    
public:
    ASTAnalyzer(ostream& output) : output(output) {
    }
    virtual ~ASTAnalyzer() {
    }
    
#define VISITOR_FUNCTION(type, name) \
virtual void visit##type(type* node);
    
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
    
private:
    ostream& output;
    
    void printScopeDeclarations (Scope* scope);
    void printBlock (BlockNode* node);
    string escape(const string& s) const;
    
};

#endif /* defined(__vm__ast_analyzer__) */
