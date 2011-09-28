#pragma once
#include <ostream>
#include <ast.h>
#include <map>
#include <ast.h>
class BCodeVisitor: public mathvm::AstVisitor {

    struct NodeInfo {
        size_t id;
        mathvm::VarType type;
    };

    std::ostream& stream;
    size_t id_counter;
    std::map<const void*, NodeInfo> nodeInfoMap;
    
    typedef std::pair<mathvm::AstNode*, NodeInfo> NodeInfoMapElem;

    NodeInfo& saveInfo(const void* node, size_t id, mathvm::VarType type);
    NodeInfo& getInfo(const void* node);
    size_t newId();
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


