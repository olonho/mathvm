//
// Created by wimag on 20.11.16.
//

#ifndef MATHVM_BYTECODE_TRANSLATOR_OVERRIDES_H
#define MATHVM_BYTECODE_TRANSLATOR_OVERRIDES_H

//
// Created by wimag on 20.11.16.
//

#include "mathvm.h"
#include "../../../../vm/parser.h"
#include "ast_bytecode_generator.h"

namespace mathvm{
    class InterpreterCodeImpl: public Code{
    public:
        virtual Status *execute(vector<Var *> &vars) override;
    };

}


#endif //MATHVM_BYTECODE_TRANSLATOR_OVERRIDES_H
