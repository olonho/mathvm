//
// Created by svloyso on 20.11.16.
//

#ifndef MATHVM_TRANSLATOR_VISITOR_H
#define MATHVM_TRANSLATOR_VISITOR_H

#include "visitors.h"
#include "Context.h"
#include <exception>

namespace mathvm {
class TranslatorVisitor : public AstVisitor {
	Code *code;
	Context ctx;
	VarType lastResult = VT_INVALID;
public:
	TranslatorVisitor(Code *code) : code(code), lastResult(VT_INVALID) { /* EMPTY */ }

	void loadVariable(const AstVar* var, uint32_t pos = Status::INVALID_POSITION);
	void loadInt(int64_t val);
	void compare(VarType type, uint64_t eqThen, uint64_t lessThen, uint64_t greaterThen, uint32_t pos);
	void convertCmp(VarType& t1, VarType& t2, uint32_t pos);
	void convertAriphmetic(VarType& t1, VarType& t2, uint32_t pos);
	void convertLogic(VarType& t1, VarType& t2, uint32_t pos);

	void declareScope(Scope *scope, bool isGlobal = false);


#define VISITOR_FUNCTION(type, name) \
    void visit##type(type* node) override;

	FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

};

#define OPFORINTEGRALTYPE(type, op_prefix, op_postfix, var, ...) \
	do { \
	switch(type) { \
		case VT_DOUBLE: \
			var = op_prefix ## D ## op_postfix; \
			break; \
		case VT_INT: \
			var = op_prefix ## I ## op_postfix; \
			break; \
		default: \
			throw __VA_ARGS__; \
	} } while (0)

#define OPFORTYPE(type, op_prefix, op_postfix, var, ...) \
	do { \
	switch(type) { \
		case VT_DOUBLE: \
			var = op_prefix ## D ## op_postfix; \
			break; \
		case VT_INT: \
			var = op_prefix ## I ## op_postfix; \
			break; \
		case VT_STRING: \
			var = op_prefix ## S ## op_postfix; \
			break; \
		default: \
			throw __VA_ARGS__; \
	} } while (0)
} // namespace mathvm

#endif //MATHVM_TRANSLATOR_VISITOR_H
