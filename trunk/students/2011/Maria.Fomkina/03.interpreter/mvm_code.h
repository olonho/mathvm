#ifndef _MVM_CODE_H
#define _MVM_CODE_H

#include "mathvm.h"

#include <cstdio>
#include <string>
#include <iostream>
#include <map>

namespace mathvm {
  
class MvmCode : public Code {
 public:
    MvmCode() {
      _functionids.insert(std::make_pair("<top>", 0));
    }

    ~MvmCode() {}

    void set_bytecode(Bytecode* bytecode) {
      bytecode_ = bytecode;
    }

    Status* execute(vector<Var*>& vars);

 private:
    Bytecode* bytecode_;
 public:
    std::map<string, uint16_t> _functionids;
  };

}

#endif /* _MVM_CODE_H */
