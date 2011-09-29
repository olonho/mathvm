#pragma once
#include <ostream>
#include <ast.h>
#include <map>
#include <vector>
#include <sstream>
#include <iostream>

class BCodeVisitor: public mathvm::AstVisitor {

    enum NodeType {
        NT_VAR,
        NT_SCONST,
        NT_OTHER
    };

    struct NodeInfo {
        mathvm::VarType type;
        NodeType nodeType;
        size_t id;
    };

    std::ostream& stream;
    
    std::map<const void*, NodeInfo> nodeInfoMap;
    
    typedef std::pair<mathvm::AstNode*, NodeInfo> NodeInfoMapElem;

    NodeInfo& saveNodeInfo(const void* node, mathvm::VarType type,
                           size_t id = 0, BCodeVisitor::NodeType nodeType = BCodeVisitor::NT_OTHER);
    NodeInfo& loadNodeInfo(const void* node);

    size_t newVarId();
    int genInstrBinNode(const NodeInfo &a, const NodeInfo &b, mathvm::TokenKind op, 
                        mathvm::VarType& resType, std::string& instruction);
public:
    BCodeVisitor(std::ostream& o);
    void show(mathvm::AstNode* node) { node->visit(this); }

#define VISITOR_FUNCTION(type, name) \
    void visit##type(mathvm::type* node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};


