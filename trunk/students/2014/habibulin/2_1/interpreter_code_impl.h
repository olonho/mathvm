#ifndef INTERPRETER_CODE_IMPL_H
#define INTERPRETER_CODE_IMPL_H

#include "mathvm.h"
#include "my_utils.h"

#include <memory>

using std::shared_ptr;

namespace mathvm {

class InterpreterCodeImpl : public Code {
private:
    // maps function id to it's context
//    map<uint16_t, shared_ptr<Context> > _ctxMap;

public:
    InterpreterCodeImpl() {}

    virtual Status* execute(vector<Var*>& vars) {
        return Status::Error("Code execution is not implemented");
    }

//    shared_ptr<Context> ctxById(uint16_t ctxId) {
//         auto it = _ctxMap.find(ctxId);
//         if(it == _ctxMap.end()) {
//             return shared_ptr<Context>();
//         }
//         return it->second;
//    }

//    shared_ptr<Context> createCtx(uint16_t functionId, uint16_t parentId) {
//        shared_ptr<Context> newCtx(new Context(functionId, parentId));
//        auto res = _ctxMap.insert(make_pair(functionId, newCtx));
//        return res.first->second;
//    }

//    shared_ptr<Context> createTopmostCtx(uint16_t functionId) {
//        shared_ptr<Context> newCtx(new Context(functionId));
//        auto res = _ctxMap.insert(make_pair(functionId, newCtx));
//        return res.first->second;
//    }
};

}

#endif // INTERPRETER_CODE_IMPL_H
