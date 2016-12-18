#ifndef VM_AF_3_JTRANSLATOR_H
#define VM_AF_3_JTRANSLATOR_H

#include <cstring>
#include "../../../../include/mathvm.h"
#include "../../../../include/visitors.h"
#include "../../../../libs/asmjit/asmjit.h"
#include "ScopeData.h"
#include "JScopeData.h"
#include <set>
#include <stack>
#include <queue>
#include <bits/shared_ptr.h>

namespace mathvm {

    extern asmjit::Label jitedFunctions[UINT16_MAX + 12];

    class JTranslator : public BytecodeTranslatorImpl {

    public:
        virtual ~JTranslator() override;

        virtual Status *translate(const string &program, mathvm::Code **code);
    };

    struct FunctionAnalysData {
        AstFunction *function;
        shared_ptr<JScopeData> scope;

        FunctionAnalysData(AstFunction *function, shared_ptr<JScopeData> scope);
    };

    extern asmjit::Label doubleZero;

    class JVisitor : private AstVisitor {
    private:
        std::shared_ptr<JScopeData> currentSd;
        asmjit::JitRuntime runtime;
        asmjit::X86Assembler assembler;
        asmjit::X86Compiler *topCompiler = nullptr;
        std::stack<asmjit::X86Var *> jStack;
        std::stack<FunctionAnalysData> waitedFunctions;
        asmjit::FuncBuilderX printDouble = asmjit::FuncBuilderX(asmjit::kCallConvHost);
        asmjit::FuncBuilderX printInt = asmjit::FuncBuilderX(asmjit::kCallConvHost);
        asmjit::FuncBuilderX printString = asmjit::FuncBuilderX(asmjit::kCallConvHost);;
        asmjit::FuncBuilderX signatureIII = asmjit::FuncBuilderX(asmjit::kCallConvHost);;
        asmjit::FuncBuilderX signatureIDD = asmjit::FuncBuilderX(asmjit::kCallConvHost);;
        asmjit::FuncBuilderX signatureLongCallI = asmjit::FuncBuilderX(asmjit::kCallConvHost);;
        asmjit::FuncBuilderX signatureLongCallD = asmjit::FuncBuilderX(asmjit::kCallConvHost);;
        asmjit::FuncBuilderX signatureLongCallS = asmjit::FuncBuilderX(asmjit::kCallConvHost);;
        asmjit::FuncBuilderX signatureLongCallV = asmjit::FuncBuilderX(asmjit::kCallConvHost);;
    public:
        JVisitor();

        virtual ~JVisitor() override;

        Status *runTranslate(Code *code, AstFunction *function);

        void translateFunction(AstFunction *function);

        void dumpByteCode(ostream &out);

    private:

        inline void bindAll();

        void scopeEvaluator(Scope *scope);

        void storeVariable(const AstVar *var);

        void loadVariable(const AstVar *var);

        void prepareTopType(VarType param);

        virtual void visitForNode(ForNode *node) override;

        virtual void visitPrintNode(PrintNode *node) override;

        virtual void visitLoadNode(LoadNode *node) override;

        virtual void visitIfNode(IfNode *node) override;

        virtual void visitBinaryOpNode(BinaryOpNode *node) override;

        virtual void visitDoubleLiteralNode(DoubleLiteralNode *node) override;

        virtual void visitStoreNode(StoreNode *node) override;

        virtual void visitStringLiteralNode(StringLiteralNode *node) override;

        virtual void visitWhileNode(WhileNode *node) override;

        virtual void visitIntLiteralNode(IntLiteralNode *node) override;

        virtual void visitUnaryOpNode(UnaryOpNode *node) override;

        virtual void visitNativeCallNode(NativeCallNode *node) override;

        virtual void visitBlockNode(BlockNode *node) override;

        virtual void visitFunctionNode(FunctionNode *node) override;

        virtual void visitReturnNode(ReturnNode *node) override;

        virtual void visitCallNode(CallNode *node) override;

        void processArithmeticOperation(BinaryOpNode *node);

        void processLogicOperation(BinaryOpNode *node);

        void processBitwiseOperation(BinaryOpNode *node);

        void processComparisonOperation(BinaryOpNode *node);

        void registerFunction(AstFunction *function);

        void processIntegerComparisonOperation(BinaryOpNode *node);

        void processDoubleComparisonOperation(BinaryOpNode *node);

        void saveCapturedVariables(NodeScopeData *nsd, const std::vector<asmjit::X86Mem *> &stackPointers);

        void setXmmDataForLastCall(const int &xmmCnt);

        void setupArgumentsForTraditionalCall(AstFunction *bf,
                                                        CallNode *node,
                                                        asmjit::X86CallNode *&callNode,
                                                        asmjit::FuncPrototype &prototype,
                                                        const NodeScopeData *nsd,
                                                        const std::vector<asmjit::X86Var *> &closureVariables,
                                                        const int64_t &function_id,
                                                        const int &xmmCnt,
                                                        bool &requireSetXmmCnt,
                                                        bool isNativeCall
        );

        void setupArgumentsForLongCall(AstFunction *bf,
                                                        CallNode *node,
                                                        asmjit::X86CallNode *&callNode,
                                                        asmjit::FuncPrototype &prototype,
                                                        const NodeScopeData *nsd,
                                                        const std::vector<asmjit::X86Var *> &closureVariables,
                                                        const int64_t &function_id,
                                                        const int &xmmCnt,
                                                        bool &requireSetXmmCnt,
                                                        bool isNativeCall
        );

        void makeInline(CallNode *node, AstFunction *bf, NodeScopeData *nsd, int64_t function_id);
    };
}


#endif //VM_AF_3_JTRANSLATOR_H
