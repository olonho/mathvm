#ifndef INT_BYTECODE_GEN_H_
#define INT_BYTECODE_GEN_H_

#include <string>
#include <map>
#include <sstream>

#include "parser.h"
#include "ast.h"

class BytecodeGenerator : public mathvm::AstVisitor {
public:
  typedef int16_t address_t;
  /** Generates code from the given source file */
  void generate(const char *source, mathvm::Code *acode);
#define VISITOR_FUNCTION(type, name) \
    void visit##type(mathvm::type *node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
  void incFunLocals(size_t index) {
    if (index >= localsCounter.size()) {
      localsCounter.resize(index + 1);
    }
    ++localsCounter[index];
  }

  void initFunLocals(size_t index) {
    if (index >= localsCounter.size()) {
      localsCounter.resize(index + 1);
    }
    localsCounter[index] = 1;
  }

private:
  std::vector<int> localsCounter;
  int currentFun;
  mathvm::Code *code;
  // Pointer to currently generating function
  mathvm::Bytecode* fBC;  
  mathvm::VarType TOSType, returnType;

  void convert(const mathvm::VarType &from, const mathvm::VarType &to);
  void loadVar(const mathvm::AstVar *var);
  void storeVar(const mathvm::AstVar *var);
  void loadCTXVar(const mathvm::AstVar *var);
  void storeCTXVar(const mathvm::AstVar *var);
  std::map<const mathvm::AstVar*, std::pair<address_t, address_t> > varIds;
  
  void firstRun(mathvm::Scope *scope);
  void secondRun(mathvm::Scope *scope);

  mathvm::Parser parser;
};

#endif /* INT_BYTECODE_GEN_H_ */
