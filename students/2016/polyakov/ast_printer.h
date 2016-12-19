#pragma once

#include <string>
#include <sstream>
#include "visitors.h"
#include "ast.h"

namespace mathvm {

class ASTPrinter : public AstBaseVisitor {
  static const std::string indent;
  static const std::string separator;
  static const std::string endline;

  std::stringstream& sstream;
  std::string currentIndent;
  bool isFunctionBlock;

public:
  ASTPrinter(std::stringstream& sstream):
     sstream(sstream) { 
    isFunctionBlock = false;
  }

#define VISITOR_FUNCTION(type, name) \
  virtual void visit##type(type* node);

  FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

};

}