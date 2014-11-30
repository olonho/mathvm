#ifndef TRANSLATION_UTILS_HPP
#define TRANSLATION_UTILS_HPP 

#include "ast.h"
#include "mathvm.h"
#include "context.hpp"

#include <string>

namespace mathvm {

AstVar* findVariable(const std::string& name, Scope* scope, AstNode* at);
AstFunction* findFunction(const std::string& name, Scope* scope, AstNode* at);

bool isTopLevel(AstFunction* function);
bool isTopLevel(FunctionNode* function);
bool isNumeric(VarType type);
bool hasNonEmptyStack(const AstNode* node);

void readVarInfo(const AstVar* var, uint16_t& localId, uint16_t& localContext, Context* ctx);
void loadVar(VarType type, uint16_t localId, uint16_t context, Bytecode* bc);
void loadVar(LoadNode* node, uint16_t localId, uint16_t context, Bytecode* bc);
void loadVar(StoreNode* node, uint16_t localId, uint16_t context, Bytecode* bc);
void storeVar(VarType type, uint16_t localId, uint16_t localContext, TokenKind op, Bytecode* bc);
void cast(AstNode* expr, VarType to, Bytecode* bc);

} // namespace mathvm

#endif