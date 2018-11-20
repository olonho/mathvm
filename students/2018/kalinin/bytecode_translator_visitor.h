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

    class BytecodeTranslator : public AstBaseVisitor {
        Context *ctx{};
        Bytecode *bytecode = new Bytecode;
        Code *code{};

    public:
        explicit BytecodeTranslator(Code *code) : code(code) {};

        void visitFunctionNode(FunctionNode *node) override;

        void visitBlockNode(BlockNode *node) override;

        void visitIfNode(IfNode *node) override;

        void visitWhileNode(WhileNode *node) override;

        void visitStoreNode(StoreNode *node) override;

        void visitLoadNode(LoadNode *node) override;

        void visitBinaryOpNode(BinaryOpNode *node) override;

        void visitUnaryOpNode(UnaryOpNode *node) override;

        void visitDoubleLiteralNode(DoubleLiteralNode *node) override;

        void visitIntLiteralNode(IntLiteralNode *node) override;

        void visitStringLiteralNode(StringLiteralNode *node) override;

        void visitForNode(ForNode *node) override;

        void visitReturnNode(ReturnNode *node) override;

        void visitPrintNode(PrintNode *node) override;

        void visitCallNode(CallNode *node) override;

//        void visitNativeCallNode(NativeCallNode *node) override;

        Bytecode *getBytecode();

    private :
        VarType getType(AstNode *node);

        void translateBooleanOperation(BinaryOpNode *node, TokenKind op);

        void translateCompareOperation(AstNode *left, AstNode *right, TokenKind op);

        void translateArithmeticOperation(BinaryOpNode *node, TokenKind op);

        void translateNegateNumber(UnaryOpNode *node);

        void translateInverseBoolean(UnaryOpNode *node);

        void translateLoadVariable(const AstVar *var);

        void translateStoreVariable(const AstVar *var);

        void translateCastTypes(VarType sourse, VarType target);

        void translateFunctionsBody(Scope *scope);

        bool isExpressionNode(AstNode *node);
    };

    class TypeEvaluter : public AstBaseVisitor {
        Context *ctx;
        VarType returnType = VT_VOID;

    public:
        explicit TypeEvaluter(Context *ctx) : ctx(ctx) {}

        void visitFunctionNode(FunctionNode *node) override;

        void visitBlockNode(BlockNode *node) override;

        void visitIfNode(IfNode *node) override;

        void visitWhileNode(WhileNode *node) override;

        void visitStoreNode(StoreNode *node) override;

        void visitLoadNode(LoadNode *node) override;

        void visitBinaryOpNode(BinaryOpNode *node) override;

        void visitUnaryOpNode(UnaryOpNode *node) override;

        void visitDoubleLiteralNode(DoubleLiteralNode *node) override;

        void visitIntLiteralNode(IntLiteralNode *node) override;

        void visitStringLiteralNode(StringLiteralNode *node) override;

        void visitForNode(ForNode *node) override;

        void visitReturnNode(ReturnNode *node) override;

        void visitPrintNode(PrintNode *node) override;

        void visitCallNode(CallNode *node) override;

//        void visitNativeCallNode(NativeCallNode *node) override;

    private:
        void fillContext(Scope *scope);

        VarType checkCompareOperation(VarType left, VarType right);

        VarType checkEqualsOperation(VarType left, VarType right);

        VarType checkArithmeticOperation(VarType left, VarType right);

        VarType checkIntegerOperation(VarType left, VarType right);

        VarType checkRangeOperation(VarType left, VarType right);

        VarType checkFunctionCallParameter(VarType expected, VarType actual);

        void checkFunctionParameters(AstFunction *func);

        void setType(AstNode *node, VarType type);

        VarType getType(AstNode *node);

        bool containsFunction(string name);

        void visitFunctions(Scope *scope);
    };

    //TODO add general context list
    class Context {
        class ChildsIterator;

        static vector<BytecodeFunction *> functionList;
        vector<Var *> varList{};
        unordered_map<string, uint16_t> variablesById{};
        unordered_map<string, uint16_t> functionsById{};

        static vector<Context *> contextList;
        Context *parent{};
        vector<Context *> childs{};

        uint16_t id{};
        ChildsIterator *iter{};
        static Context *instanse;

    private:
        Context() {
            init(nullptr);
        }

        explicit Context(Context *parentContext) {
            init(parentContext);
        }

        void init(Context *parentContext);

    public:
        static Context *getRoot();

        Context *addChild();

        Context *getLastChildren();

        uint16_t getId();

        Context *getVarContext(string name);

        void addVar(Var *var);

        uint16_t VarNumber();

        uint16_t getVarId(string name);

        BytecodeFunction *getFunction(string name);

        void addFun(AstFunction *func);

        Context *getParentContext();

        ChildsIterator *childsIterator();

//        FunctionsIterator* functionsIterator();

    private:
        class ChildsIterator {
            friend Context;
            vector<Context *> *childs{};
            uint32_t count = 0;

        private:
            explicit ChildsIterator(vector<Context *> *childs) : childs(childs) {};

        public:
            bool hasNext() {
                return count < childs->size() - 1;
            }

            Context *next() {
                Context *res = (*childs)[count];
                count++;
                return res;
            }
        };

    public:
        class FunctionsIterator {
            unordered_map<string, uint16_t> *functions{};
            unordered_map<string, uint16_t>::iterator iter{};
            vector<BytecodeFunction *> *functionList{};

        public:
            explicit FunctionsIterator(Context *ctx)
                    : functions(&ctx->functionsById), iter(functions->begin()), functionList(&ctx->functionList) {}

            bool hasNext() {
                return iter != functions->end();
            }

            BytecodeFunction *next() {
                uint32_t res = (*iter).second;
                iter++;
                return (*functionList)[res];
            }
        };
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