//
// Created by user on 11/30/16.
//

#ifndef VM_AF_3_STACKITEM_H
#define VM_AF_3_STACKITEM_H

#include <cstdint>
#include <vector>

namespace mathvm {

    class StackItem {
    public:
        union {
            long intValue;
            double doubleValue;
            unsigned short stringIdValue;
        } value;

        StackItem() {
            value.intValue = 0;
        }

        StackItem(long arg) {
            value.intValue = arg;
        }

        StackItem(double arg) {
            value.doubleValue = arg;
        }

        StackItem(unsigned short arg) {
            value.stringIdValue = arg;
        }
    };

    static StackItem STACK_EMPTY = StackItem((int64_t) 0);
    extern std::vector<StackItem> variables;
}

#endif //VM_AF_3_STACKITEM_H
