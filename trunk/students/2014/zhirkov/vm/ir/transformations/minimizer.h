#pragma once

#include <set>
#include <assert.h>
#include <iostream>
#include "identity.h"
#include "../ir_printer.h"
#include "../ir_analyzer.h"

namespace mathvm {
    namespace IR {
        struct Minimizer : public Transformation<> {

            virtual void operator()();

            Minimizer(SimpleIr const &source, SimpleIr& dest, std::ostream &_debug)
                    : Transformation(source, dest, "empty blocks elimination", _debug) {
            }

            virtual ~Minimizer() {
            }

            virtual IrElement * visit(const Function *const expr);

        private:
            static void replaceSucc(Block *parent, Block *old) ;
        };


    }
}
