#pragma once

#include <string>
#include <sstream>
#include "ast.h"
#include "visitors.h"

namespace mathvm {

class ASTPrinter : public AstBaseVisitor {
  static const std::string indent;
  static const std::string separator;
  static const std::string endline;

  std::stringstream sstream;
  std::string currentIndent;
  bool isFunctionBlock;

public:
  ASTPrinter() { 
    isFunctionBlock = false;
  }

#define VISITOR_FUNCTION(type, name) \
  virtual void visit##type(type* node);

  FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

  Status* print_code(std::string &outputSource, 
    const std::string &inputSource);
};

}