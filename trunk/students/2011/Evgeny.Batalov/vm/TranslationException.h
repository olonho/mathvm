#pragma once 
#include <exception>
#include <string>
#include <ast.h>

class TranslationException : public std::exception {
    std::string error;
    mathvm::AstNode* node;
public:
    TranslationException(const std::string& err, mathvm::AstNode*  node)
        : error(err)
        , node(node)
        {}

    virtual const char* what() {
        return error.c_str();
    }

    virtual mathvm::AstNode* getNode() {
      return node;
    }

    virtual ~TranslationException() throw() {}
};

