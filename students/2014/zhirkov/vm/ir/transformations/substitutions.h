#pragma once


#include "identity.h"
#include "../ir_printer.h"
#include "../ir_analyzer.h"

namespace mathvm {
    namespace IR {

class Referenced : public IrAnalyzer<bool, std::set<VarId>> {

public:
    virtual bool visit(const BinOp *const expr);

    virtual bool visit(const UnOp *const expr);

    virtual bool visit(const Variable *const expr);

    virtual bool visit(const Return *const expr);

    virtual bool visit(const Phi *const expr);

    virtual bool visit(const Int *const expr);

    virtual bool visit(const Double *const expr);

    virtual bool visit(const Ptr *const expr);

    virtual bool visit(const Block *const expr);

    virtual bool visit(const Assignment *const expr);

    virtual bool visit(const Call *const expr);

    virtual bool visit(const Print *const expr);

    virtual bool visit(const FunctionRecord *const expr);

    virtual bool visit(const JumpAlways *const expr);

    virtual bool visit(const JumpCond *const expr);

    virtual bool visit(const WriteRef *const expr);

    virtual bool visit(const ReadRef *const expr);

    Referenced(std::ostream &debug, char const *name) : IrAnalyzer(debug, name) {
    }

protected:
    virtual bool defaultAnswer() { return false; }
};

        class Substitution : public Transformation {

        public:
            virtual IrElement *visit(BinOp const *const expr) override;

            virtual IrElement *visit(UnOp const *const expr) override;

            virtual IrElement *visit(Variable const *const expr);
            virtual IrElement *visit(Assignment const *const expr);

            Substitution(SimpleIr const *old, std::ostream &_debug = std::cerr)
                    : Transformation(old, "substitutions", _debug), _changed(false) {
            }

            bool isTrivial() {
                return !_changed;
            }

        private:
            std::map<VarId, Atom const *> _substitutions;
            bool _changed;

        public:

        };
    }
}
