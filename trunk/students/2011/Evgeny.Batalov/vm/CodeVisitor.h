#pragma once
#include <ostream>
#include <ast.h>
#include <map>
#include <vector>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <mathvm.h>
#include <visitors.h>
#include "Executable.h"
#include "TranslationException.h"
#include "TranslationUtils.h"
#include "SymbolStack.h"

class CodeVisitor: public mathvm::AstVisitor {

    Executable* executable;
    mathvm::AstFunction* topAstFunc;
    
    FunctionNodeToIndex funcNodeToIndex;
    IndexToFunctionNode indexToFuncNode;
    NodeInfos nodeInfo;
    SymbolStack<size_t> funcId;

    size_t curFuncId;
    MyBytecode  *curBytecode;
    mathvm::BlockNode *curBlock;
    MyBytecode&  cCode()  { return *curBytecode; }
    mathvm::BlockNode& cBlock() { return *curBlock;    }

    void transError(std::string str = "", mathvm::AstNode *node = 0);
    void procBinNode(mathvm::BinaryOpNode* node, mathvm::VarType resType);
    void putLazyLogic(mathvm::TokenKind op, mathvm::Label& lbl);
    void cast(mathvm::AstNode* node); 
public:
    CodeVisitor(mathvm::AstFunction* top, 
                const FunctionContexts& funcContexts, 
                const FunctionNodeToIndex& funcNodeToIndex, 
                const IndexToFunctionNode& indexToFuncNode,
                const NodeInfos& nodeInfo);
    void  visit();
    Executable* getExecutable() { return executable; }

#define VISITOR_FUNCTION(type, name) \
    void visit##type(mathvm::type* node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};
