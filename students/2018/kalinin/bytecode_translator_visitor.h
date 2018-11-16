// Created by Владислав Калинин on 09/11/2018.
//

#ifndef MATHVM_BYTECODE_TRANSLATOR_VISITOR_H
#define MATHVM_BYTECODE_TRANSLATOR_VISITOR_H

#include "../../../include/visitors.h"
#include <unordered_map>
#include <stdexcept>

using namespace std;

namespace mathvm {
    class Context;

    class Bytecode_translator_visitor : public AstBaseVisitor {
        Context *ctx{};
        Bytecode *bytecode{};
        map<uint32_t, VarType> nodeTypes{};
        map<uint16_t, Context *> contextList{};
        map<uint32_t, Context *> nodeContext{};

    public:
        void visitFunctionNode(FunctionNode *node) override;

        void visitBlockNode(BlockNode *node) override;

        void visitIfNode(IfNode *node) override;
//
//        void visitWhileNode(WhileNode *node) override;
//
//        void visitStoreNode(StoreNode *node) override;
//
//        void visitLoadNode(LoadNode *node) override;
//
//        void visitBinaryOpNode(BinaryOpNode *node) override;
//
//        void visitUnaryOpNode(UnaryOpNode *node) override;
//
//        void visitDoubleLiteralNode(DoubleLiteralNode *node) override;
//
//        void visitIntLiteralNode(IntLiteralNode *node) override;
//
//        void visitStringLiteralNode(StringLiteralNode *node) override;
//
//        void visitForNode(ForNode *node) override;
//
//        void visitReturnNode(ReturnNode *node) override;
//
//        void visitPrintNode(PrintNode *node) override;
//
//        void visitCallNode(CallNode *node) override;
//
//        void visitNativeCallNode(NativeCallNode *node) override;

    private :
//        void fillContext(Scope *scope);
    };

    class TypeEvaluter : public AstBaseVisitor {
        Context *ctx;
        map<uint32_t, VarType> *nodeTypes{};
        map<uint16_t, Context *> *contextList{};
        map<uint32_t, Context *> *nodeContext{};
        VarType returnType;

    public:
        TypeEvaluter(Context *ctx, map<uint32_t, VarType> *nodeTypes,
                     map<uint16_t, Context *> *contextList, map<uint32_t, Context *> *nodeContext,
                     VarType returnType) :
                ctx(ctx), nodeTypes(nodeTypes), contextList(contextList), nodeContext(nodeContext),
                returnType(returnType) {}

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node) { matchNodeAndContext(node); register##type(node); }

        FOR_NODES(VISITOR_FUNCTION)

#undef VISITOR_FUNCTION

    private:
        void registerFunctionNode(FunctionNode *node);

        void registerBlockNode(BlockNode *node);

        void registerIfNode(IfNode *node);

        void registerWhileNode(WhileNode *node);

        void registerStoreNode(StoreNode *node);

        void registerLoadNode(LoadNode *node);

        void registerBinaryOpNode(BinaryOpNode *node);

        void registerUnaryOpNode(UnaryOpNode *node);

        void registerDoubleLiteralNode(DoubleLiteralNode *node);

        void registerIntLiteralNode(IntLiteralNode *node);

        void registerStringLiteralNode(StringLiteralNode *node);

        void registerForNode(ForNode *node);

        void registerReturnNode(ReturnNode *node);

        void registerPrintNode(PrintNode *node);

        void registerCallNode(CallNode *node);

        void registerNativeCallNode(NativeCallNode *node);

        void fillContext(Scope *scope);

        VarType checkBooleanOperation(VarType left, VarType right, TokenKind op);

        VarType checkEqualsOperation(VarType left, VarType right, TokenKind op);

        VarType checkArithmeticOperationWithoutPlus(VarType left, VarType right, TokenKind op);

        VarType checkIntegerModOperation(VarType left, VarType right, TokenKind op);

        VarType checkPlusOperation(VarType left, VarType right, TokenKind op);

        void matchNodeAndContext(AstNode *node);

        void registerContext();

        void setType(AstNode *node, VarType type);

        VarType getType(AstNode *node);
    };

    //TODO add general context list
    class Context {
        unordered_map<string, uint16_t> variables{};
        unordered_map<string, uint16_t> functions{};
        vector<Var *> var_buffer{};
        vector<BytecodeFunction *> fun_buffer{};
        Context *parent;
        vector<Context *> childs{};
        static uint16_t count;
        uint16_t id;

        Context(Scope *currentScope, Context *parentContext) {
            parent = parentContext;
            id = count + (uint16_t) 1;
            count++;
        }

    public:
        Context() {
            id = count;
            parent = nullptr;
            count++;
        }

        Context *addChild(Scope *scope) {
            auto *child = new Context(scope, this);
            childs.push_back(child);
            return child;
        }

        uint16_t getId() {
            return id;
        }

        Var *getLocalVar(string name) {
            return var_buffer[variables[name]];
        }

        Context *getVarContext(string name);

        Context *getParentContext() {
            return parent;
        }

        void addVar(Var *var);

        void addFun(BytecodeFunction *func);
    };

    class CompileError : std::exception {
        const char *msg;
        uint32_t position;

    public:
        CompileError(const char *msg, uint32_t position) : msg(msg), position(position) {}

        const char *getMsg() {
            return msg;
        }

        uint32_t getPosition() {
            return position;
        }
    };

}//mathvm

#endif //MATHVM_BYTECODE_TRANSLATOR_VISITOR_H