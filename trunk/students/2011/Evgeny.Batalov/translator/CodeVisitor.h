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

class CodeVisitor: public mathvm::AstVisitor {

    MyCode code;
    mathvm::AstFunction* topAstFunc;

    struct NodeInfo {
        mathvm::VarType type;
    };

    struct VarInfo {
        std::string name;
        mathvm::VarType type;
        size_t id;
    };

    struct ParamInfo {
        std::string name;
        mathvm::VarType type;
        size_t id;
        size_t index;
    };
 
    typedef std::map<mathvm::AstNode*, NodeInfo> NodeInfoMap;
    typedef std::vector<VarInfo> VarDefs;
    typedef std::map<std::string, VarDefs> VarInfoMap;
    typedef std::map<std::string, ParamInfo> FuncParams;
    typedef std::map<std::string, FuncParams> FuncParamsMap;

    NodeInfoMap nodeInfo;
    VarInfoMap varInfo;
    FuncParamsMap funcParams;

    mathvm::Bytecode  *curBytecode;
    mathvm::BlockNode *curBlock;
    mathvm::Bytecode&  cCode()  { return *curBytecode; }
    mathvm::BlockNode& cBlock() { return *curBlock;    }

    void transError(std::string str = "");

    void setNodeInfo(mathvm::AstNode* node, mathvm::VarType type);
    NodeInfo& getNodeInfo(mathvm::AstNode* node);

    void setVarInfo(std::string name, size_t id, mathvm::VarType type);
    VarInfo& getVarInfo(std::string name);

    ParamInfo& getParamInfo(const std::string& fName, const std::string& pName);
    ParamInfo& getParamInfo(const std::string& fName, size_t index);
    void setParamInfo(const std::string& fName, ParamInfo& info);

    void pushFuncParams(const std::string& fName);
    void popFuncParams(const std::string& fName);

    size_t newVarId();
    void checkFunction(std::string fName);
    void procBinNode(const NodeInfo &a, const NodeInfo &b, mathvm::TokenKind op, mathvm::VarType& resType);
    void putLazyLogic(mathvm::TokenKind op, mathvm::Label& lbl);
public:
    CodeVisitor(mathvm::AstFunction* top);
    void  visit();
    MyCode& getCode() { return code; }

#define VISITOR_FUNCTION(type, name) \
    void visit##type(mathvm::type* node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};
