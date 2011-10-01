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

class CodeVisitor: public mathvm::AstVisitor {

    enum NodeType {
        NT_VAR,
        NT_SCONST,
        NT_OTHER
    };

    struct NodeInfo {
        mathvm::VarType type;
        NodeType nodeType;
        size_t id;
        size_t index; //index for example for function parameter
    };
 
    std::map<const void*, NodeInfo> nodeInfoMap;   
    typedef  std::map<std::string, NodeInfo> ParameterNameInfoMap;
    typedef std::map<std::string, ParameterNameInfoMap > FuncVarsMap;
    FuncVarsMap funcVars;
    std::string curFuncName;
    MyCode code;
    mathvm::BlockNode *curBlock;
    mathvm::Bytecode  *curBytecode;

    mathvm::Bytecode&  cCode()  { return *curBytecode; }
    mathvm::BlockNode& cBlock() { return *curBlock; }

    void transError(std::string str = "");

    NodeInfo& saveNodeInfo(const void* node, mathvm::VarType type,
                           size_t id = 0, CodeVisitor::NodeType nodeType = CodeVisitor::NT_OTHER);
    NodeInfo& loadNodeInfo(const void* node);
    NodeInfo& loadParameterInfo(const std::string& fName, const std::string& pName);
    NodeInfo& loadParameterInfo(const std::string& fName, size_t pName);
    NodeInfo& saveParameterInfo(const std::string& fName, const std::string& pName, 
                                                          const NodeInfo& info);

    size_t newVarId();

    void procBinNode(const NodeInfo &a, const NodeInfo &b, mathvm::TokenKind op, mathvm::VarType& resType);
    void putLazyLogic(mathvm::TokenKind op, mathvm::Label& lbl);
public:
    CodeVisitor();
    void  translate(mathvm::AstNode* node) { node->visit(this); cCode().addByte(mathvm::BC_STOP); }
    const mathvm::Code& getCode() const { return code; }
    const MyCode& getMyCode()     const { return code; }

#define VISITOR_FUNCTION(type, name) \
    void visit##type(mathvm::type* node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};


