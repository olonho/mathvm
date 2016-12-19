#pragma once

#include <string>
#include <sstream>
#include "ast.h"
#include "visitors.h"

namespace mathvm {
class Formatter : public AstBaseVisitor {
  std::stringstream sstream;
  std::string currentIndent;
  static const std::string blockIndent;
  static const std::string separator;

  BlockNode *currentBlock;
  void reset();
  void rewindLastSeparator();
public:
  Formatter() { }

#define VISITOR_FUNCTION(type, name) \
  virtual void visit##type(type* node);

  FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

  Status* formatCode(std::string &outputSource, const std::string &inputSource);
};
}