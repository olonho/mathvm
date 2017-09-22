#pragma once

#include "visitors.h"
#include "mathvm.h"
#include "parser.h"


namespace mathvm {

struct AstOffsetDumper : AstDumper {
#define VISITOR_FUNCTION(type, name)            \
void visit##type(type* node);

FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
private: 
  int offset = 0;
};


struct AstPrettyPrinter : AstDumper {
#define VISITOR_FUNCTION(type, name)            \
void visit##type(type* node);

FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
private: 
  static const int offsetSingleInc = 4;
  int offset = 0;
  char lastPrintedChar = 0;

  void increaseOffset() {
    offset += offsetSingleInc;
  }
  void decreaseOffset() {
    offset -= offsetSingleInc;
  }
  void printOffset();

  std::string escapeCharacters(const std::string&);

  void printScope(Scope*);

  void writeSemicolonAndNewline();
  // Abstract out the specific method of writing,
  // so we can write to file or to console at will.
  void write(const char*);
  void write(const string&);
  
  template<class T>
  void write(T);

  template<class T>
  void writeLn(T);

  const char* tokenToText(const TokenKind) const;
  const char* varTypeToText(const VarType) const;
};


struct AstPrinterTranslator : Translator {
  Status* translate(const string& program, Code* *code);
};

}
