#pragma once

#include <set>
#include "identity.h"
#include "../ir_printer.h"
#include "../ir_analyzer.h"
#include "../reg_allocator.h"

namespace mathvm {
    namespace IR {


        struct RegSpiller : public Transformation<> {

            const GlobalRegAllocInfo allocations;
            const VarId gpAcc;
            const VarId doubleAcc;

        private:
            bool hasMemoryCell(uint64_t id) {
                for (auto v : _currentResultFunction->memoryCells) if (id == v) return true;
                return false;
            }

        public:
            RegSpiller(SimpleIr const &source,
                    SimpleIr &dest,
                    GlobalRegAllocInfo const &alloc,
                    std::ostream &_debug)
                    : Transformation(source, dest, "stack allocator", _debug),
                      allocations(alloc),
                      gpAcc(source.varMeta.size()),
                      doubleAcc(source.varMeta.size() + 1) {
                _debug << "Double acc id is " << doubleAcc << "\nGP acc id is " << gpAcc << "\n";
            }

            bool isStackAllocated(uint64_t id) const;

            bool isRegAllocated(uint64_t id) const {
                return !isStackAllocated(id);
            }


            virtual IrElement *visit(const Assignment *const expr);

            virtual IrElement *visit(const Variable *const expr);

            virtual IrElement *visit(const Function *const expr);

            virtual void operator()() {
                Transformation::visit(&_oldIr);
            }

            virtual ~RegSpiller() {
            }
        };


    }
}
