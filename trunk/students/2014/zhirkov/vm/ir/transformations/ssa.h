#pragma once

#include <map>
#include <set>
#include "identity.h"
#include "../../../../../../include/mathvm.h"

namespace mathvm {
    namespace IR {

        /**
        * static single assignment
        * inserts empty phi functions which should be filled with phi_filler transformation after.
        */
        struct Ssa : public Transformation<> {
            virtual ~Ssa() {
            }

            virtual void operator()() {
                Transformation::visit(&_oldIr);
            }

            Ssa(SimpleIr const &source, SimpleIr &dest, std::ostream &debug = std::cerr)
                    : Transformation(source, dest, "ssa transformation", debug),
                      meta(_newIr.varMeta) {
            }

            bool shouldBeRenamed(VarId id) {
                return _latestVersion.find(id) != _latestVersion.end();
            }

            VarId newName(VarId id) {
                return _latestVersion[id];
            }

            virtual IrElement *visit(const Variable *const expr);


            virtual IrElement *visit(const Phi *const expr);

            virtual IrElement *visit(const Assignment *const expr);

            virtual IrElement *visit(const Call *const expr);

            virtual IrElement *visit(const Function *const expr);

            virtual IrElement *visit(const WriteRef *const expr);

            virtual IrElement *visit(const ReadRef *const expr);;

        private:
            std::vector<IR::SimpleIr::VarMeta> &meta;
            std::map<VarId, VarId> _latestVersion;

        };

    }
}
