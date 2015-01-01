#pragma once

#include <sstream>
#include <stack>
#include <map>
#include "../../../../../vm/parser.h"
#include "../ir/ir.h" 
#include "../ir/ir_printer.h"
#include "ssa_utils.h"
#include "closure_analyzer.h"


namespace mathvm {
    struct AstVarMetadata;

    struct AstFunctionMetadata {
        AstFunctionMetadata(const AstFunction *f, const uint16_t id) : fun(f), id(id) {
        }

        const AstFunction *const fun;
        const uint16_t id;

        std::vector<const AstVarMetadata *> args;
        std::vector<const AstVarMetadata *> capturedArgs;
        std::vector<const AstVarMetadata *> capturedRefArgs;
    };

    struct AstVarMetadata {
        AstVarMetadata(AstVar const *const var, uint64_t const id, bool const isRef = false)
                : isTemp(false), var(var), id(id), isRef(isRef) {
        }

        AstVarMetadata(uint64_t const id, bool const isRef = false) : isTemp(true), var(NULL), id(id), isRef(isRef) {
        }

        const bool isTemp;
        const AstVar *const var;
        const uint64_t id;
        const bool isRef;
    };

    
    struct TranslationContext : public AstAnalyzerContext {

        TranslationContext() :
                block(NULL),
                function(NULL),
                nVars(0),
                nBlocks(0) {
        }

        IR::Block *block;
        AstFunction const *function;

        uint64_t nVars;
        uint64_t nBlocks;

        std::map<AstVar const *, AstVarMetadata *> astVarMeta;
        std::map<uint64_t, AstVarMetadata *> allVarMeta;
        std::map<AstFunction const *, AstFunctionMetadata *> funMeta;
    };

    class SimpleIrBuilder : public AstAnalyzer<IR::SimpleSsaIr, TranslationContext> {
        ClosureInfo const *const _closureInfo;
        std::stack<IR::Atom const *> _lastAtoms;

    public:
        virtual void visitPrintNode(PrintNode *node) override;

        virtual void visitLoadNode(LoadNode *node) override;

        virtual void visitDoubleLiteralNode(DoubleLiteralNode *node) override;

        virtual void visitStoreNode(StoreNode *node) override;

        virtual void visitCallNode(CallNode *node) override;

        virtual void visitStringLiteralNode(StringLiteralNode *node) override;

        virtual void visitWhileNode(WhileNode *node) override;

        virtual void visitIntLiteralNode(IntLiteralNode *node) override;

        virtual void visitBlockNode(BlockNode *node) override;

        virtual void visitBinaryOpNode(BinaryOpNode *node) override;

        virtual void visitUnaryOpNode(UnaryOpNode *node) override;

        virtual void visitNativeCallNode(NativeCallNode *node) override;

        virtual void visitIfNode(IfNode *node) override;

        virtual void visitReturnNode(ReturnNode *node) override;

        virtual void visitFunctionNode(FunctionNode *node) override;

        virtual void visitForNode(ForNode *node) override;

        IR::Atom const *_popAtom() { const IR::Atom *a = _lastAtoms.top(); _lastAtoms.pop(); return a; }

        void _pushAtom(IR::Atom const *atom) { _lastAtoms.push(atom); }
        void _pushVar(uint64_t v) { _lastAtoms.push(new IR::Variable(v)); }

        void visitAstFunction(AstFunction const *function);

        SimpleIrBuilder(AstFunction const *top, ClosureInfo const *closureInfo, std::ostream &debug)
                : AstAnalyzer(top, "translator", debug),
                  _closureInfo(closureInfo) {
        }

        ClosureInfo::Function const& closureFunMeta(AstFunction const* f) const {
            return *(_closureInfo->functions.at(f));
        }
        ClosureInfo::Variable const& closureVarMeta(AstVar const* v) const {
            return _closureInfo->vars.at(v);
        }
        AstVarMetadata & varMeta(AstVar const * const var)  {
            return *(ctx.astVarMeta.at(var));
        }


        AstVarMetadata const& varMetaById(uint64_t var) const {
            return *(ctx.allVarMeta.at(var));
        }

        AstFunctionMetadata & funMeta(AstFunction const *f) {
            return *(ctx.funMeta.at(f));
        }

        virtual void start();


    private:

        IR::Atom* readVar(uint64_t varId);
        void embraceArgs(AstFunction const * const);
        void argsForCaptured(AstFunction const *const);
        void createMemoryCells(AstFunction const *const);
        void prologue(AstFunction const* const f);
        void epilogue(AstFunction const* const f);

        void embraceVars(Scope * const);

        std::string nextBlockName() {
            std::ostringstream str;
            str << ctx.function->name() << '[' << ctx.nBlocks++ << ']';
            return str.str();
        }

        IR::Block *newBlock() {
            std::string name = nextBlockName();
            return new IR::Block(name);
        }

        void emit(IR::Statement const *const statement) {
            ctx.block->contents.push_back(statement);
        }

        uint64_t makeAstVar(AstVar const *var);

        uint64_t makeTempVar();

        virtual void declareFunction(AstFunction const *fun);

        void insertPhi();
    public:
        virtual ~SimpleIrBuilder() {
            for (auto v : ctx.allVarMeta)
                delete v.second;
            for (auto f : ctx.funMeta)
                delete f.second;
            while (!_lastAtoms.empty()) {
                delete _lastAtoms.top();
                _lastAtoms.pop();
            }
        }


    };
}
