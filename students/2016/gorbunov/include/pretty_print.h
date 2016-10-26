#ifndef _MATHVM_PRETTY_PRINT_H
#define _MATHVM_PRETTY_PRINT_H

#include <ostream>

#include "ast.h"
#include "mathvm.h"

namespace mathvm 
{

class PPrintVisitor : public AstVisitor {
	static const int SPACE_NUM;
	static const char SPACE;
	static const char BLOCK_OPEN;
	static const char BLOCK_END;
	static const char LPAREN;
	static const char RPAREN;
	static const char SEMICOLON;
	static const char COMMA;

	static std::string prepareStringLiteral(std::string str);
	
	std::ostream& _out;
	int _level;
public:
    PPrintVisitor(std::ostream& out): _out(out), _level(0) {}

    void print(AstNode* root);

#define VISITOR_FUNCTION(type, name)            \
    void visit##type(type* node) override;

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

class PPrintTranslator : public Translator {
	std::ostream& _out;

  public:
    PPrintTranslator(std::ostream& out): _out(out) {
    }

    virtual ~PPrintTranslator() {
    }

    Status* translate(const std::string& program, Code* *code) override;
};

}
#endif