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

            virtual bool visit(const BinOp *const expr)  const {
                visitElement(expr->left);
                visitElement(expr->right);
                return false;
            }

            virtual bool visit(const UnOp *const expr) const  {
                visitElement(expr->operand);
                return false;
            }

            virtual bool visit(const Variable *const expr) const  {
                _status.insert(expr->id);
                return false;
            }

            virtual bool visit(const Return *const expr) const  {
                visitElement(expr->atom);
                return false;
            }

            virtual bool visit(const Phi *const expr) const  {
                for (auto p : expr->vars) visitElement(p);
                return false;
            }

            virtual bool visit(const Block *const expr)  const {
                if (visited.find(expr) != visited.end()) return false;
                for (auto st : expr->contents)
                    visitElement(st);

                visited.insert(expr);
                if (!expr->isLastBlock())
                    visitElement(expr->getTransition());
                return false;
            }

            virtual bool visit(const Assignment *const expr) const {
                //todo consider pure and non-pure functions here.
                if (expr->value->isCall()) _status.insert(expr->var->id);
                visitElement(expr->value);
                return false;
            }

            virtual bool visit(const Call *const expr)  const {
                for (auto p : expr->params) visitElement(p);
                for (auto p : expr->refParams)   _status.insert(p);
                return false;
            }

            virtual bool visit(const Print *const expr)  const {
                visitElement(expr->atom);
                return false;
            }

            virtual bool visit(const Function *const expr) const  {
                visitElement(expr->entry);
                return false;
            }

            virtual bool visit(const JumpAlways *const expr) const  {
                visitElement(expr->destination);
                return false;
            }

            virtual bool visit(const JumpCond *const expr)  const {
                visitElement(expr->condition);
                visitElement(expr->yes);
                visitElement(expr->no);
                return false;
            }

            virtual bool visit(const WriteRef *const expr)  const {
                _status.insert(expr->refId);
                visitElement(expr->atom);
                return false;
            }

            virtual bool visit(const ReadRef *const expr) const  {
                _status.insert(expr->refId);
                return false;
            }

            virtual void analyze(SimpleIr const *const ir)  const {
                for (auto f : ir->functions)
                    visitElement(f);
            }

        protected:
            virtual bool defaultAnswer() const  {
                return false;
            }
        };

        class Substitution : public Transformation<bool> {

        public:
            virtual IrElement *visit(BinOp const *const expr) override;

            virtual IrElement *visit(UnOp const *const expr) override;

            virtual IrElement *visit(Variable const *const expr);

            virtual IrElement *visit(Assignment const *const expr);

            virtual IrElement *visit(Phi const *const expr);

            virtual IrElement *visit(Return const* const expr);
            Substitution(SimpleIr const &source, SimpleIr & dest, std::ostream &_debug = std::cerr)
                    : Transformation(source, dest, "substitutions", _debug), used(_debug), _changed(false)  {
                used.analyze(&_oldIr);
            }

            bool isTrivial() {
                return !_changed;
            }


            virtual bool operator()();

        private:
            std::map<VarId, Atom const *> _substitutions;
            Referenced used;
            bool _changed;

            bool isUsed(VarId var) const { return used.status().find(var) != used.status().end(); }
            bool canSubstitute(VarId var) const { return _substitutions.find(var) != _substitutions.cend(); }
        };
    }
}
