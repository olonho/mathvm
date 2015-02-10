#pragma once

#include <map>
#include <set>
#include <queue>
#include "identity.h"

namespace mathvm {
    namespace IR {
        struct PhiRemover : public Transformation<> {
            virtual ~PhiRemover() {
            }

            PhiRemover(SimpleIr const &source, SimpleIr &dest, std::ostream &debug = std::cerr)
                    : Transformation(source, dest, "phi remover", debug) {
            }

            virtual IrElement *visit(Phi const *const expr) {
                return NULL;
            }

            virtual void operator()() {
                Transformation::visit(&_oldIr);
            }

        };
    }
}
