#ifndef CONVERTER_H_
#define CONVERTER_H_
#include "mathvm.h"
#include "ast.h"

namespace mathvm {

struct Converter: AstVisitor{
public:
	Converter(ostream& os = std::cout): out(os) {}
	virtual ~Converter();

	void printSource(AstFunction * top);
	void visitBlockBodyNode(BlockNode * node);

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
	ostream& out;
};
}
#endif /* CONVERTER_H_ */
