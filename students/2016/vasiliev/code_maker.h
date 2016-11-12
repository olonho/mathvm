#ifndef MATHVM_CODE_MAKER_H
#define MATHVM_CODE_MAKER_H

#include "ast.h"
#include "mathvm.h"

using namespace mathvm;

class code_maker: public mathvm::AstVisitor {

    mathvm::Code* code;

    mathvm::VarType top_type;

    mathvm::Bytecode* bc;

//    std::vector<std::map<std::string, uint16_t>> variables;
//    std::vector<uint16_t> scopes;
//    std::vector<std::vector<mathvm::VarType>> types;


//    typedef vector<map<string, pair<uint16_t, VarType>> small_scope;

    vector<vector<map<string, pair<uint16_t, VarType>>>> variables;
    vector<uint16_t> locals;
    vector<uint16_t> scopes;


    void clear_context() {
        variables.clear();
        locals.clear();
        scopes.clear();
        top_type = mathvm::VT_INVALID;
    }

//    void clear_context() {
//        variables.clear();
//        scopes.clear();
//        top_type = mathvm::VT_INVALID;
//    }

    void add_small_scope() {
        (*variables.rbegin()).push_back(map<string, pair<uint16_t, VarType>>());
    }

    void pop_small_scope() {
        (*variables.rbegin()).pop_back();
    }

public:
    void add_scope() {
        variables.push_back(vector<map<string, pair<uint16_t, VarType>>>());
        (*variables.rbegin()).push_back(map<string, pair<uint16_t, VarType>>());
        scopes.push_back(locals.size());
        locals.push_back(0);
    }

    uint16_t locals_number() {
        uint16_t sid = *scopes.rbegin();
        return locals[sid];
    }

    void pop_scope() {
        variables.pop_back();
        scopes.pop_back();
    }

//    void add_scope() {
//        variables.push_back(std::map<std::string, uint16_t>());
//        scopes.push_back(types.size());
//        types.push_back(std::vector<mathvm::VarType>());
//    }
//
//    uint16_t locals() {
//        uint16_t sid = *scopes.rbegin();
//        return (uint16_t) types[sid].size();
//    }
//
//    void pop_scope() {
//        variables.pop_back();
//        scopes.pop_back();
//    }

private:

//    std::pair<uint16_t, uint16_t> add_name(const std::string& name, mathvm::VarType type) {
//        map<std::string, uint16_t>& right = variables[variables.size() - 1];
//        if (right.find(name) != right.end()) throw "Variable exists";
//        uint16_t sid = *scopes.rbegin();
//        uint16_t vid = (uint16_t) types[sid].size();
//        types[sid].push_back(type);
//        right[name] = vid;
//        auto pair = std::make_pair(sid, vid);
//        return pair;
//    }

//    std::pair<uint16_t, uint16_t> resolve_name(const std::string& name) {
//        for (int i = variables.size() - 1; i >= (int) variables.size() - 1 /*no closure*/; --i) {
//            auto res = variables[i].find(name);
//            if (res != variables[i].end()) {
//                return std::make_pair(scopes[i], (*res).second);
//            }
//        }
//        throw "Variable not found (resolve_name)";
//    }

//    mathvm::VarType get_type(uint16_t sid, uint16_t vid) {
//        if (types.size() <= sid) throw "Scope not found (get_type)";
//        if (types[sid].size() <= vid) throw "Variable not fount (get_type)";
//        return types[sid][vid];
//    }

//    mathvm::VarType get_type(std::pair<uint16_t, uint16_t> id) {
//        return get_type(id.first, id.second);
//    }

    std::pair<uint16_t, VarType> add_name(const std::string& name, mathvm::VarType type) {
        vector<map<string, pair<uint16_t, VarType>>>& tmp = *variables.rbegin();
        map<string, pair<uint16_t, VarType>>& right = *tmp.rbegin();
        if (right.find(name) != right.end()) throw "Variable exists";
        uint16_t sid = *scopes.rbegin();
        uint16_t vid = locals[sid];
        auto pair = std::make_pair(vid, type);
        right[name] = pair;
        locals[sid] += 1;
        return pair;
    }

    std::pair<uint16_t, VarType> resolve_name(const std::string& name) {
        vector<map<string, pair<uint16_t, VarType>>>& right = *variables.rbegin();
        for (int i = right.size() - 1; i >= 0; --i) {
            auto res = right[i].find(name);
            if (res != right[i].end()) {
                return (*res).second;
            }
        }
        throw "Variable not found (resolve_name)";
    }

//    mathvm::VarType get_type(uint16_t sid, uint16_t vid) {
//        if (types.size() <= sid) throw "Scope not found (get_type)";
//        if (types[sid].size() <= vid) throw "Variable not fount (get_type)";
//        return types[sid][vid];
//    }

//    mathvm::VarType get_type(std::pair<uint16_t, uint16_t> id) {
//        return get_type(id.first, id.second);
//    }

    void load(const std::string& name) {
        auto pair = resolve_name(name);
        VarType type = pair.second;
//        VarType type = get_type(pair);
        switch (type) {
            case VT_INT: bc->addInsn(BC_LOADIVAR); break;
            case VT_DOUBLE: bc->addInsn(BC_LOADDVAR); break;
            case VT_STRING: bc->addInsn(BC_LOADSVAR); break;
            default: throw "Not valid type (load)";
        }
        bc->addUInt16(pair.first);
//        bc->addUInt16(pair.second);
        top_type = type;
    }

    void store(const std::string& name) {
        auto pair = resolve_name(name);
        VarType type = pair.second;
//        VarType type = get_type(pair);
        if (top_type != type) throw "Wrong type (store)";
        switch (top_type) {
            case VT_INT: bc->addInsn(BC_STOREIVAR); break;
            case VT_DOUBLE: bc->addInsn(BC_STOREDVAR); break;
            case VT_STRING: bc->addInsn(BC_STORESVAR); break;
            default: throw "Not valid type (store)";
        }
        bc->addUInt16(pair.first);
//        bc->addUInt16(pair.second);
        top_type = VT_VOID;
    }

    void print(AstNode* node) {
        node->visit(this);
        switch (top_type) {
            case VT_INT: bc->addInsn(BC_IPRINT); break;
            case VT_DOUBLE: bc->addInsn(BC_DPRINT); break;
            case VT_STRING: bc->addInsn(BC_SPRINT); break;
            default: throw "No option (print)";
        }
        top_type = VT_VOID;
    }

    void compare(Instruction cnd) {
        bc->add(BC_ILOAD0);
        Label else_l(bc);
        Label end_l(bc);
        bc->addBranch(cnd, else_l);
        bc->add(BC_ILOAD0);
        bc->addBranch(BC_JA, end_l);
        else_l.bind(bc->current());
        bc->add(BC_ILOAD1);
        end_l.bind(bc->current());
        top_type = VT_INT;
    }


public:

    code_maker(mathvm::Code* code);

    code_maker(Code* code, Bytecode* bc);

    virtual ~code_maker() override;

    virtual void visitForNode(mathvm::ForNode *node) override;

    virtual void visitPrintNode(mathvm::PrintNode *node) override;

    virtual void visitLoadNode(mathvm::LoadNode *node) override;

    virtual void visitIfNode(mathvm::IfNode *node) override;

    virtual void visitCallNode(mathvm::CallNode *node) override;

    virtual void visitDoubleLiteralNode(mathvm::DoubleLiteralNode *node) override;

    virtual void visitStoreNode(mathvm::StoreNode *node) override;

    virtual void visitStringLiteralNode(mathvm::StringLiteralNode *node) override;

    virtual void visitWhileNode(mathvm::WhileNode *node) override;

    virtual void visitIntLiteralNode(mathvm::IntLiteralNode *node) override;

    virtual void visitBlockNode(mathvm::BlockNode *node) override;

    virtual void visitBinaryOpNode(mathvm::BinaryOpNode *node) override;

    virtual void visitUnaryOpNode(mathvm::UnaryOpNode *node) override;

    virtual void visitNativeCallNode(mathvm::NativeCallNode *node) override;

    virtual void visitReturnNode(mathvm::ReturnNode *node) override;

    virtual void visitFunctionNode(mathvm::FunctionNode *node) override;
};


#endif //MATHVM_CODE_MAKER_H
