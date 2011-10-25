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
#include "MyCode.h"
#include "TranslationException.h"
#include "TranslationUtils.h"

class CodeVisitor: public mathvm::AstVisitor {

    MyCode code;
    mathvm::AstFunction* topAstFunc;
    
    TranslatableFunctions tranFuncs;
    FunctionContexts funcContexts;
    FunctionNodeToIndex funcNodeToIndex;
    IndexToFunctionNode indexToFuncNode;
    NodeInfos nodeInfo;
    SymbolStack<size_t> funcId;

    size_t curFuncId;
    mathvm::Bytecode  *curBytecode;
    mathvm::BlockNode *curBlock;
    mathvm::Bytecode&  cCode()  { return *curBytecode; }
    mathvm::BlockNode& cBlock() { return *curBlock;    }

    void transError(std::string str = "");

    void procBinNode(const NodeInfo &a, const NodeInfo &b, mathvm::TokenKind op, mathvm::VarType& resType);
    void putLazyLogic(mathvm::TokenKind op, mathvm::Label& lbl);
public:
    CodeVisitor(mathvm::AstFunction* top, const FunctionContexts& funcContexts, 
                const FunctionNodeToIndex& funcNodeToIndex, 
                const IndexToFunctionNode& indexToFuncNode,
                const NodeInfos& nodeInfo);
    void  visit();
    MyCode& getCode() { return code; }

#define VISITOR_FUNCTION(type, name) \
    void visit##type(mathvm::type* node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};
