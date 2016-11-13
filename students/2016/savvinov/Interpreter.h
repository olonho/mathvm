//
// Created by dsavvinov on 13.11.16.
//

#ifndef MATHVM_INTERPRETER_H
#define MATHVM_INTERPRETER_H

#include <mathvm.h>
#include <stack>
#include <unordered_map>
#include "BytecodeCode.h"

namespace mathvm {

template <class T>
struct Data {
    T value;
};

class Interpreter {
    BytecodeCode * code;
    vector<Var*> & vars;
    stack<DataT> stck;
    typedef void * DataT;
    std::unordered_map<uint16_t , DataT> dataById {};
public:
    Interpreter(BytecodeCode * code, vector<Var*> & vars)
            : code(code)
            , vars(vars)
    { }

    Status * executeProgram();

    void executeFunction(BytecodeFunction *pFunction);

    DataT getData(uint16_t id) {
        auto it = dataById.find(id);
        if (it == dataById.end()) {
            return nullptr;
        }
        return it->second;
    }

    void addData(DataT data, uint16_t id) {
        dataById[id] = data;
    }
};

}
#endif //MATHVM_INTERPRETER_H
