//
// Created by andy on 11/22/16.
//

#ifndef PROJECT_INTERPRETER_H
#define PROJECT_INTERPRETER_H

#include <unordered_map>
#include <memory>
#include <parser.h>
#include "mathvm.h"
#include "MetaInfo.h"

namespace mathvm
{
class InterpreterCodeImpl : public Code {
public:
    InterpreterCodeImpl(){}

    virtual Status* execute(vector<Var*>& vars) override;

    MetaInfo& getMeta() {
        return _info;
    }
private:
    MetaInfo _info;
};

}

#endif //PROJECT_INTERPRETER_H
