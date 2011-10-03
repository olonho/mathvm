#ifndef _BYTECODE_VISITOR_H
#define _BYTECODE_VISITOR_H

#include "mathvm.h"
#include "ast.h"
#include <map>
#include <string>
#include <stack>
#include <vector>

namespace mathvm {

class VarMap {
private:
  std::map<std::string, std::stack<uint8_t> > map_;
  std::stack<std::vector<std::string> > vars_;
  uint8_t cur_id_;
public:
  VarMap() : cur_id_(0) {}

  void AddScope() {
    vars_.push(std::vector<std::string>());
  }

  void DeleteScope() {
    std::vector<std::string> temp = vars_.top();
    vars_.pop();
    for (size_t i = 0; i < temp.size(); ++i) {
      map_[temp[i]].pop();
    }
    temp.clear();
  }

  uint8_t AddVar(std::string name) {
    map_[name].push(cur_id_);
    vars_.top().push_back(name);
    return cur_id_++;
  }

  const uint8_t GetVarId(std::string name) {
    return map_[name].top();
  }
};

class BytecodeVisitor : public AstVisitor {
 private:
    Code* code_;
    Bytecode* bcode_;
    VarMap var_map_;
    std::vector<VarType> var_types_;
 public:
    BytecodeVisitor(Code* code)
        : code_(code), bcode_(((BytecodeFunction *)(
            code->functionById(0)))->bytecode())  {
    }
    virtual ~BytecodeVisitor() {
    }
    
#define VISITOR_FUNCTION(type, name)            \
    virtual void visit##type(type* node);
    
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

}

#endif /* _BYTECODE_VISITOR_H */
