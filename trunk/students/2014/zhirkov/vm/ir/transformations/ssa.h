#pragma once

#include <map>
#include <set>
#include "identity.h"

namespace mathvm {
    namespace IR {
        struct Ssa : public Transformation {
            virtual ~Ssa() {
            }

            Ssa(SimpleIr* old, std::ostream& debug=std::cerr)
                    : Transformation(old, "ssa transformation", debug),
                      meta(_currentIr->varMeta) {
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

            virtual IrElement *visit(const FunctionRecord *const expr);

            virtual IrElement *visit(const WriteRef *const expr);

            virtual IrElement *visit(const ReadRef *const expr);;

        private:
            std::vector<IR::SimpleIr::VarMeta>& meta;
            std::map<VarId, VarId> _latestVersion;

        };

    }
}
