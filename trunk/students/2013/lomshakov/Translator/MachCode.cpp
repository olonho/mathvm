//
// Created by Vadim Lomshakov on 13/12/13.
// Copyright (c) 2013 spbau. All rights reserved.
//

#include "MachCode.h"
#include "ast.h"


namespace mathvm {


  MachCode::MachCode() {

  }

  MachCode::~MachCode() {

  }

  Status *MachCode::execute(vector<Var *> &vars) {
    void* code = *functionByName(AstFunction::top_name)->machcode();
    function_cast<void (*)()>(code)();
    return new Status();
  }

  MachCodeFunc *MachCode::functionByName(const string &name) {
    return dynamic_cast<MachCodeFunc*>(Code::functionByName(name));
  }

  MachCodeFunc *MachCode::functionById(uint16_t id) {
    return dynamic_cast<MachCodeFunc*>(Code::functionById(id));
  }




  MachCodeFunc::~MachCodeFunc() {
    MemoryManager::getGlobal()->free(_code);
  }

  void MachCodeFunc::disassemble(ostream &out) const {

  }

  char const *  MachCode::makeOrGetStringConstant(string const &str) {
    StringSetIt i = _stringConstants.find(str);
    if (i == _stringConstants.end()) {
      pair<StringSetIt, bool> res = _stringConstants.insert(str);
      assert(res.second);

      return (*res.first).c_str();
    }

    return (*i).c_str();
  }
}