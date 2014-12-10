#pragma once

#include <sstream>
#include <stack>
#include <map>
#include "../../../../../vm/parser.h"
#include "../ir/ir.h"
#include "../common.h"
#include "../ir/ir_printer.h"
#include "../ast_metadata.h"
#include "ssa_utils.h"
#include "closure_analyzer.h"
#include "../typechecker.h"

namespace mathvm {


    struct TranslationContext : public AstAnalyzerContext {

        TranslationContext() :
                block(NULL),
                function(NULL),
                nVars(0),
                nBlocks(0) {
        }

        IR::Block *block;
        AstFunction const* function;

        uint64_t nVars;
        uint64_t nBlocks;

    };

    class SsaIrBuilder : public AstAnalyzer<IR::SimpleSsaIr, TranslationContext> {
        struct AstVarMetadata {
            AstVarMetadata(AstVar const *const var, uint64_t const id) : isTemp(false), var(var), id(id) {
            }

            AstVarMetadata(uint64_t const id) : isTemp(true), var(NULL), id(id) {
            }

            const bool isTemp;
            const AstVar *const var;
            const uint64_t id;
            std::vector<AstFunction *> capturedBy;

        };

        struct AstFunctionMetadata {
            AstFunctionMetadata(const AstFunction *f, const uint16_t id) : fun(f), id(id) {
            }

            const AstFunction *const fun;
            const uint16_t id;

            std::vector<AstVarMetadata *> args;
            std::vector<AstVarMetadata *> locals;
        };

         //preprocessing results
        ClosureAnalyzer const &_closureAnalyzer;


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

        IR::Atom const *_popAtom() {
            const IR::Atom *a = _lastAtoms.top();
            _lastAtoms.pop();
            return a;
        }

        void _pushAtom(IR::Atom const *atom) {
            _lastAtoms.push(atom);
        }

        void visitAstFunction(AstFunction const*function);

    public:
        SsaIrBuilder(AstFunction const *top, ClosureAnalyzer const &closureAnalyzer, ostream &debug)
                : AstAnalyzer(top),
                  _closureAnalyzer(closureAnalyzer),
                  _out(debug) {
        }

        AstVarMetadata &varMeta(AstVar const *var) {
            return *(_astvarMeta[var]);
        }

        AstVarMetadata &varMetaById(uint64_t var) {
            return *(_allvarMeta[var]);
        }

        AstFunctionMetadata &funMeta(AstFunction const*f) {
            return *(_funMeta[f]);
        }

        virtual void start();


    private:

        std::ostream &_out;

        void embraceArgs(AstFunction const *);

        void embraceVars(Scope *);

        std::string nextBlockName() {
            std::ostringstream str;
            str << ctx.function->name() << '[' << ctx.nBlocks ++ << ']';
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

        void declareFunction(AstFunction const* fun);

        void insertPhi();

        std::map<AstVar const *, AstVarMetadata *> _astvarMeta;
        std::map<uint64_t, AstVarMetadata *> _allvarMeta;
        std::map<AstFunction const*, AstFunctionMetadata *> _funMeta;


        std::stack<IR::Atom const *> _lastAtoms;

    public:
        virtual ~SsaIrBuilder() {
            for (auto v : _allvarMeta)
                delete v.second;
            for (auto f : _funMeta)
                delete f.second;
            while (!_lastAtoms.empty()) {
                delete _lastAtoms.top();
                _lastAtoms.pop();
            }
        }
    };
}
