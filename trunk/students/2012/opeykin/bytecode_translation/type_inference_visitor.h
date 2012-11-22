/*
 * type_inference_visitor.h
 *
 *  Created on: Nov 17, 2012
 *      Author: alex
 */

#ifndef TYPE_INFERENCE_VISITOR_H_
#define TYPE_INFERENCE_VISITOR_H_

#include "ast.h"
#include <stack>

namespace mathvm {

class TypeInferenceVisitor : public AstVisitor {
public:
	typedef map<CustomDataHolder*, VarType> TypeMap;
	TypeInferenceVisitor(TypeMap& typeMap);
	virtual ~TypeInferenceVisitor();

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

	VarType maxType(VarType type1, VarType type2);
    void processFunction(AstFunction* function);

private:
    TypeMap& _types;
	stack<Scope*> _scopes;
};

} /* namespace mathvm */
#endif /* TYPE_INFERENCE_VISITOR_H_ */
