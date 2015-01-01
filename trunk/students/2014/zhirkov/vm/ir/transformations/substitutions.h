#pragma once

#include <set>
#include "identity.h"
#include "../ir_printer.h"
#include "../ir_analyzer.h"

namespace mathvm {
    namespace IR {


        class Referenced : public IrAnalyzer<bool, std::set<VarId>> {

        public:

            Referenced(std::ostream &debug) : IrAnalyzer(debug, "referenced variables") {
            }

            virtual bool visit(const BinOp *const expr) {
                visitElement(expr->left);
                visitElement(expr->right);
                return false;
            }

            virtual bool visit(const UnOp *const expr) {
                visitElement(expr->operand);
                return false;
            }

            virtual bool visit(const Variable *const expr) {
                _status.insert(expr->id);
                return false;
            }

            virtual bool visit(const Return *const expr) {
                visitElement(expr->atom);
                return false;
            }

            virtual bool visit(const Phi *const expr) {
                for (auto p : expr->vars) visitElement(p);
                return false;
            }

            virtual bool visit(const Block *const expr) {
                if (visited.find(expr) != visited.end()) return false;
                for (auto st : expr->contents)
                    visitElement(st);

                visited.insert(expr);
                auto tr = expr->getTransition();
                if (tr)
                    visitElement(tr);
                return false;
            }

            virtual bool visit(const Assignment *const expr) {
                visitElement(expr->value);
                return false;
            }

            virtual bool visit(const Call *const expr) {
                for (auto p : expr->params) visitElement(p);
                return false;
            }

            virtual bool visit(const Print *const expr) {
                visitElement(expr->atom);
                return false;
            }

            virtual bool visit(const FunctionRecord *const expr) {
                visitElement(expr->entry);
                return false;
            }

            virtual bool visit(const JumpAlways *const expr) {
                visitElement(expr->destination);
                return false;
            }

            virtual bool visit(const JumpCond *const expr) {
                visitElement(expr->condition);
                visitElement(expr->yes);
                visitElement(expr->no);
                return false;
            }

            virtual bool visit(const WriteRef *const expr) {
                visitElement(expr->atom);
                return false;
            }

            virtual bool visit(const ReadRef *const expr) {
                _status.insert(expr->refId);
                return false;
            }

            virtual void analyze(SimpleIr const *const ir) {
                for (auto f : ir->functions)
                    visitElement(f);
            }

        protected:
            virtual bool defaultAnswer() {
                return false;
            }
        };

        class Substitution : public Transformation {

        public:
            virtual IrElement *visit(BinOp const *const expr) override;

            virtual IrElement *visit(UnOp const *const expr) override;

            virtual IrElement *visit(Variable const *const expr);

            virtual IrElement *visit(Assignment const *const expr);

            virtual IrElement *visit(Phi const *const expr);

            Substitution(SimpleIr const *old, std::ostream &_debug = std::cerr)
                    : Transformation(old, "substitutions", _debug), used(_debug), _changed(false) {
            }

            bool isTrivial() {
                return !_changed;
            }

            virtual void start();

        private:
            std::map<VarId, Atom const *> _substitutions;
            Referenced used;
            bool _changed;
        };
    }
}
