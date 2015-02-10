#pragma once

#include <map>
#include <set>
#include "identity.h"

namespace mathvm {
    namespace IR {
        struct PhiFiller : public Transformation<> {
            virtual ~PhiFiller() {
            }

            virtual void operator()() {
                Transformation::visit(&_oldIr);
            }

            PhiFiller(SimpleIr const &source, SimpleIr &dest, std::ostream &debug = std::cerr)
                    : Transformation(source, dest, "phi filler", debug) {
            }

            virtual IrElement *visit(Phi const *const expr);

        };

    }
}
