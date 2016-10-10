#pragma once
#include <ostream>
#include <visitors.h>

namespace mathvm {
  class Printer : public AstBaseVisitor {
  public:

    Printer(std::ostream& output)
      : _out(output)
      , _scopeLevel(0) {

    }

#define VISITOR_FUNCTION(type, name)            \
    virtual void visit##type(type* node) override;

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

  private:
    static constexpr const char* SEPARATOR = " ";
    static constexpr const char* BLOCK_INDENT = "  ";
    static constexpr const char* LPAREN = "(";
    static constexpr const char* RPAREN = ")";
    static constexpr const char* LBRACE = "{";
    static constexpr const char* RBRACE = "}";
    static constexpr const char* SEMICOLON = ";";
    static constexpr const char* COMMA = ",";
    static constexpr const char* QUOTE = "\'";

    uint32_t scopeLevel(Scope const* scope);

    void printTokenOp(TokenKind op);
    void printAstVar(const AstVar* var);
    void newLine();
    bool isNativeCallNode(FunctionNode* node);

    std::ostream& _out;
    uint32_t _scopeLevel;
  }; // Printer
} // mathvm