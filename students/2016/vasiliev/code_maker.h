#ifndef MATHVM_CODE_MAKER_H
#define MATHVM_CODE_MAKER_H

#include "ast.h"
#include "mathvm.h"

using namespace mathvm;

class code_maker: public mathvm::AstVisitor {

    mathvm::Code* code;

    mathvm::VarType top_type;

    mathvm::Bytecode* bc;

    vector<vector<map<string, pair<uint16_t, VarType>>>> variables;
    vector<uint16_t> locals;
    vector<uint16_t> scopes;


    void clear_context() {
        variables.clear();
        locals.clear();
        scopes.clear();
        top_type = mathvm::VT_INVALID;
    }

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


private:


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


    void load(const std::string& name) {
        auto pair = resolve_name(name);
        VarType type = pair.second;
        switch (type) {
            case VT_INT: bc->addInsn(BC_LOADIVAR); break;
            case VT_DOUBLE: bc->addInsn(BC_LOADDVAR); break;
            case VT_STRING: bc->addInsn(BC_LOADSVAR); break;
            default: throw "Not valid type (load)";
        }
        bc->addUInt16(pair.first);
        top_type = type;
    }

    void store(const std::string& name) {
        auto pair = resolve_name(name);
        VarType type = pair.second;
        if (top_type != type) throw "Wrong type (store)";
        switch (top_type) {
            case VT_INT: bc->addInsn(BC_STOREIVAR); break;
            case VT_DOUBLE: bc->addInsn(BC_STOREDVAR); break;
            case VT_STRING: bc->addInsn(BC_STORESVAR); break;
            default: throw "Not valid type (store)";
        }
        bc->addUInt16(pair.first);
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
