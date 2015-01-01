#pragma once

#include <map>
#include <set>
#include "identity.h"

namespace mathvm {
    namespace IR {
        struct PhiFiller : public Transformation {
            virtual ~PhiFiller() {
            }

            PhiFiller(SimpleIr* old, std::ostream& debug=std::cerr)
                    : Transformation(old, "phi filler", debug){
            }
            virtual IrElement *visit(Phi const *const expr);

        };

    }
}
