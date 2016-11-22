//
// Created by andy on 11/23/16.
//

#ifndef PROJECT_BYTECODEOPTIMIZER_H
#define PROJECT_BYTECODEOPTIMIZER_H

#include "interpreter_impl.h"

namespace mathvm
{
class BytecodeOptimizer {
public:
    BytecodeOptimizer(InterpreterCodeImpl *code, MetaInfo &meta)
            : _code(code), _meta(meta) {}

    void removeEmptyBlocks();
private:
    InterpreterCodeImpl *_code;
    MetaInfo &_meta;
};
}

#endif //PROJECT_BYTECODEOPTIMIZER_H
