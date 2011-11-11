#ifndef _MATHVM_JIT_H
#define _MATHVM_JIT_H

#include "mathvm.h"

namespace mathvm {

class MachCodeImpl : public Code {
    void* _code;
  public:
    MachCodeImpl();
    virtual ~MachCodeImpl();

    virtual Status* execute(vector<Var*>& vars);

    MachCodeFunction* functionByName(const string& name);
    MachCodeFunction* functionById(uint16_t id);
    void error(const char* format, ...);

    void setCode(void* code) { _code = code; }
};


}

#endif
