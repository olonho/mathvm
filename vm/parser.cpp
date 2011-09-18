#include "ast.h"
#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

namespace mathvm {

Parser::Parser() {
}

Parser::~Parser() {
    DeleteVisitor terminator;
    terminator.performDelete(_top);

    delete _topmostScope;
}

Status* Parser::parseProgram(const string& code) {
    Scanner scanner;
    Status* s = scanner.scan(code, _tokens);
    if (s) {
        return s;
    }
    return parseTopLevel();
}

BlockNode* Parser::top() const {
    return _top;
}

TokenKind Parser::currentToken() {
  if (_currentToken == tUNDEF) {
      _currentToken = _tokens.kindAt(_currentTokenIndex);
      if (_currentToken == tERROR) {
          error("parse error");
      }
    }
  return _currentToken;
}

string Parser::currentTokenValue() {
    TokenKind kind = currentToken();
    if (kind == tIDENT || kind == tDOUBLE || kind == tINT || kind == tSTRING) {
        return _tokens.valueAt(_currentTokenIndex);
    }
    return "";
}

void Parser::consumeToken() {
    _currentToken = tUNDEF;
    _currentTokenIndex++;
}


void Parser::ensureToken(TokenKind token) {
  if (currentToken() != token) {
      error("'%s' expected, seen %s", tokenStr(token), tokenStr(currentToken()));
  }
  consumeToken();
}

void  Parser::ensureKeyword(const string& keyword) {
    if (currentToken() != tIDENT || currentTokenValue() != keyword) {
        error("keyword '%s' expected", currentTokenValue().c_str());
    }
    consumeToken();
}

void Parser::error(const char* format, ...) {
    int pos = 0;
    va_list args;
    va_start(args, format);
    
    vsnprintf(_msgBuffer + pos, sizeof(_msgBuffer) - pos, format, args);
    throw _msgBuffer;
}


void Parser::pushScope() {
    Scope* newScope = new Scope(_currentScope);
    _currentScope = newScope;
}


void Parser::popScope() {
    Scope* parentScope = _currentScope->parent();
    assert(parentScope);
    _currentScope = parentScope;
}

Status* Parser::parseTopLevel() {
    _currentToken = tUNDEF;
    _currentTokenIndex = 0;
    _topmostScope = _currentScope = new Scope(0);
    _top = 0;
    try {
        _top = parseBlock(false);
    } catch (char* error) {
        return new Status(error, _tokens.positionOf(_currentTokenIndex));
    }

    //AstDumper dumper; dumper.dump(top());

    return 0;
}

StoreNode* Parser::parseAssignment() {
    assert(currentToken() == tIDENT);
    AstVar* lhs = _currentScope->lookup(currentTokenValue());
    if (lhs == 0) {
        error("undeclared variable: %s", currentTokenValue().c_str());
    }
    consumeToken();

    TokenKind op = currentToken();
    if (op == tASSIGN ||
        op == tINCRSET ||
        op == tDECRSET) {
        consumeToken();
    } else {
        error("assignment expected");
    }

    StoreNode* result = new StoreNode(_currentTokenIndex,
                                      lhs,
                                      parseExpression(),
                                      op);

    if (currentToken() == tSEMICOLON) {
        consumeToken();
    }
    return result;
}

AstNode* Parser::parseArgList() {
    ensureToken(tLBRACE);
    while (!currentToken() == tRBRACE) {
        consumeToken();
    }
    ensureToken(tRBRACE);
    return 0;
}


PrintNode* Parser::parsePrint() {
    uint32_t token = _currentToken;
    ensureKeyword("print");
    ensureToken(tLPAREN);
    
    PrintNode* result = new PrintNode(token);
    while (currentToken() != tRPAREN) {
        AstNode* operand = parseExpression();
        result->add(operand);
        if (currentToken() == tCOMMA) {
            consumeToken();
        }
    }
    ensureToken(tRPAREN);
    ensureToken(tSEMICOLON);
    return result;
}

AstNode* Parser::parseFunction() {
    ensureKeyword("function");

    if (currentToken() != tIDENT) {
        error("identifier expected");
    }
    const string& name = currentTokenValue();
    AstNode* params = parseArgList();
    BlockNode* body = parseBlock(true);

    return new FunctionNode(_currentTokenIndex,
                            name, params, body);
}

ForNode* Parser::parseFor() {
    uint32_t token = _currentTokenIndex;
    ensureKeyword("for");
    ensureToken(tLPAREN);

    if (currentToken() != tIDENT) {
        error("identifier expected");
    }

    const string& varName = currentTokenValue();
    consumeToken();

    ensureKeyword("in");

    AstNode* inExpr = parseExpression();
    ensureToken(tRPAREN);

    BlockNode* forBody = parseBlock(true);
    AstVar* forVar = forBody->scope()->lookup(varName);

    return new ForNode(token, forVar, inExpr, forBody);
}

IfNode* Parser::parseIf() {
    uint32_t token = _currentTokenIndex;
    ensureKeyword("if");
    ensureToken(tLPAREN);
    AstNode* ifExpr = parseExpression();
    ensureToken(tRPAREN);

    BlockNode* thenBlock = parseBlock(true);
    BlockNode* elseBlock = 0;
    if (currentTokenValue() == "else") {
        consumeToken();
        elseBlock = parseBlock(true);
    }

    return new IfNode(token, ifExpr, thenBlock, elseBlock);
}

BlockNode* Parser::parseBlock(bool needBraces) {
    if (needBraces) {
        ensureToken(tLBRACE);
    }

    pushScope();

    BlockNode* block = new BlockNode(_currentTokenIndex,
                                     _currentScope);
    TokenKind sentinel = needBraces ? tRBRACE : tEOF;

     while (currentToken() != sentinel) {
         if (currentToken() == tSEMICOLON) {
             consumeToken();
             continue;
         }
         if (currentToken() != tIDENT) {
             throw "identifier expected";
         }
         if (isKeyword(currentTokenValue())) {
             if (currentTokenValue() == "function") {
                 block->add(parseFunction());
             } else if (currentTokenValue() == "for") {
                 block->add(parseFor());
             } else if (currentTokenValue() == "if") {
                 block->add(parseIf());
             } else if (currentTokenValue() == "print") {
                 block->add(parsePrint());
             } else if (currentTokenValue() == "int") {
                 parseDeclaration(VT_INT);
                 continue;
             } else if (currentTokenValue() == "double") {
                 parseDeclaration(VT_DOUBLE);
                 continue;
             } else if (currentTokenValue() == "string") {
                 parseDeclaration(VT_STRING);
                 continue;
             } else {
                 cout << "unhandled keyword " << currentTokenValue() << endl;
                 assert(false);
             }
         } else {
             block->add(parseAssignment());
         }
     }

     popScope();

     if (needBraces) {
         ensureToken(tRBRACE);
     }

     return block;
}

void Parser::parseDeclaration(VarType type) {
    ensureToken(tIDENT);
    _currentScope->declare(currentTokenValue(), type);
    ensureToken(tIDENT);
    ensureToken(tSEMICOLON);
}

static inline bool isUnaryOp(TokenKind token) {
    return (token == tSUB) || (token == tNOT);
}

static inline bool isBinaryOp(TokenKind token) {
    return (token >= tADD && token <= tDIV) ||
           (token == tRANGE) ||
           (token >= tEQ && token <= tLE) ||
           (token == tAND || token <= tOR);
}

AstNode* Parser::parseUnary() {
    if (isUnaryOp(currentToken())) {
        TokenKind op = currentToken();
        consumeToken();
        return new UnaryOpNode(_currentTokenIndex, op, parseUnary());
    } else if (currentToken() == tIDENT) {
        AstVar* var = _currentScope->lookup(currentTokenValue());
        if (var == 0) {
            error("undeclared variable: %s", currentTokenValue().c_str());
        }
        LoadNode* result = new LoadNode(_currentTokenIndex, var);
        consumeToken();
        return result;
    } else if (currentToken() == tDOUBLE) {
        DoubleLiteralNode* result =
            new DoubleLiteralNode(_currentTokenIndex,
                                  parseDouble(currentTokenValue()));
        consumeToken();
        return result;
    } else if (currentToken() == tINT) {
        IntLiteralNode* result =
            new IntLiteralNode(_currentTokenIndex,
                               parseInt(currentTokenValue()));
        consumeToken();
        return result;
    } else if (currentToken() == tSTRING) {
        StringLiteralNode* result =
            new StringLiteralNode(_currentTokenIndex,
                                  currentTokenValue());
        consumeToken();
        return result;
    } else if (currentToken() == tLPAREN) {
        consumeToken();
        AstNode* expr = parseExpression();
        ensureToken(tRPAREN);
        return expr;
    } else {
        assert(false);
        return 0;
    }
}

AstNode* Parser::parseBinary(int minPrecedence) {
    AstNode* left = parseUnary();
    int precedence = tokenPrecedence(currentToken());

    while (precedence >= minPrecedence) {
        while (tokenPrecedence(currentToken()) == precedence) {
            TokenKind op = currentToken();
            uint32_t op_pos = _currentTokenIndex;
            consumeToken();
            AstNode* right = parseBinary(precedence + 1);
            left = new BinaryOpNode(op_pos, op, left, right);
        }
        precedence--;
    }
    return left;
}

AstNode* Parser::parseExpression() {
    return parseBinary(tokenPrecedence(tOR));
}

int64_t Parser::parseInt(const string& str) {
    char* p;
    int64_t result = strtoll(str.c_str(), &p, 10);
    assert(*p == '\0');
    return result;
}

double Parser::parseDouble(const string& str) {
    char* p;
    double result = strtod(str.c_str(), &p);
    assert(*p == '\0');
    return result;
}

}
