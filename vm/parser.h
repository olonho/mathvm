#ifndef _MATHVM_PARSER_H
#define _MATHVM_PARSER_H

#include "mathvm.h"
#include "ast.h"
#include "scanner.h"

namespace mathvm {

// We implement simple top down parser.
class Parser {
    BlockNode* _top;
    Scope* _currentScope;
    Scope* _topmostScope;
    TokenList _tokens;
    TokenKind _currentToken;
    uint32_t _currentTokenIndex;
    char _msgBuffer[256];

    void error(const char* msg, ...);
    TokenKind currentToken();
    string currentTokenValue();
    void consumeToken();
    void ensureToken(TokenKind token);
    void ensureKeyword(const string& keyword);

    void pushScope();
    void popScope();

    Status* parseTopLevel();
    StoreNode* parseAssignment();
    AstNode* parseExpression();
    AstNode* parseUnary();
    AstNode* parseBinary(int minPrecedence);
    AstNode* parseFunction();
    PrintNode* parsePrint();
    AstNode* parseArgList();
    ForNode* parseFor();
    IfNode*  parseIf();
    BlockNode* parseBlock(bool needBraces);
    void parseDeclaration(VarType type);

    static int64_t parseInt(const string& str);
    static double  parseDouble(const string& str);
  public:
    Parser();
    ~Parser();

    Status* parseProgram(const string& code);
    BlockNode* top() const;
};

}

#endif // _MATHVM_PARSER_H
