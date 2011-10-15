#ifndef _MVM_CODE_H
#define _MVM_CODE_H

#include "mathvm.h"

#include <cstdio>
#include <string>
#include <iostream>

namespace mathvm {
  
class MvmCode : public Code {
 public:
    MvmCode() {}

    ~MvmCode() {}

    void set_bytecode(Bytecode* bytecode) {
      bytecode_ = bytecode;
    }

    Status* execute(vector<Var*>& vars);

 private:
    Bytecode* bytecode_;
  };

}

#endif /* _MVM_CODE_H */
