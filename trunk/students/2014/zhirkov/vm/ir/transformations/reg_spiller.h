#pragma once

#include <set>
#include "identity.h"
#include "../ir_printer.h"
#include "../ir_analyzer.h"
#include "../reg_allocator.h"

namespace mathvm {
    namespace IR {


        class RegSpiller : public Transformation<> {

            const GlobalRegAllocInfo allocations;
            const uint64_t accReg;


        private:
            bool hasMemoryCell(uint64_t id) {
                for (auto v : _currentResultFunction->memoryCells) if (id == v) return true;
                return false;
            }

        public:
            RegSpiller(SimpleIr const &source, SimpleIr &dest, GlobalRegAllocInfo const &alloc, std::ostream &_debug, uint64_t const accReg)
                    : Transformation(source, dest, "stack allocator", _debug), allocations(alloc), accReg(accReg) {
            }

            bool isStackAllocated(uint64_t id) const;

            bool isRegAllocated(uint64_t id) const {
                return !isStackAllocated(id);
            }


            virtual IrElement *visit(const Assignment *const expr);

            virtual IrElement *visit(const Variable *const expr);

            virtual IrElement *visit(const FunctionRecord *const expr);

            virtual void operator()() {
                Transformation::visit(&_oldIr);
            }

            virtual ~RegSpiller() {
            }
        };


    }
}
