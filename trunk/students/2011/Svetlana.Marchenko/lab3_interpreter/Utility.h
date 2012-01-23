#pragma once

#include "ast.h"
#include "Utility.h"

#include <algorithm>

mathvm::AstFunction* extractAstFunction(mathvm::FunctionNode *funcNode);
uint16_t calculateScopeMaxLocals(mathvm::Scope *scope);
uint16_t calculateFuncMaxLocals(mathvm::FunctionNode *functNode);

mathvm::AstFunction* extractAstFunction(mathvm::FunctionNode *funcNode) {
	mathvm::Scope *searchScope = funcNode->body()->scope();
	return searchScope->lookupFunction(funcNode->name());
}

uint16_t calculateFuncMaxLocals(mathvm::FunctionNode *funcNode) {
	mathvm::Scope *searchScope = funcNode->body()->scope();
	
	int count = funcNode->parametersNumber() + searchScope->variablesCount();
	int max = 0;
	for (uint16_t i = 0; i < searchScope->childScopeNumber(); ++i) {
		int scopeMaxCount = calculateScopeMaxLocals(searchScope->childScopeAt(i));
		max = std::max(max, scopeMaxCount);
	}
	return count + max;
}

uint16_t calculateScopeMaxLocals(mathvm::Scope *scope) {
	int count = scope->variablesCount();
	int maxChildCount = 0;
	for (uint16_t i = 0; i < scope->childScopeNumber(); ++i) {
		int count = calculateScopeMaxLocals(scope->childScopeAt(i));
		maxChildCount = std::max (maxChildCount, count);
	}
	return count + maxChildCount;
}
