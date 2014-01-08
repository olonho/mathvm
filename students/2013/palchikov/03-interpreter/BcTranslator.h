#pragma once

#include "ast.h"
#include "CodeImpl.h"

using namespace mathvm;
using namespace std;

class BcTranslator : public Translator, private AstVisitor
{
public:
	BcTranslator();
	virtual ~BcTranslator();

	// inherited from Translator
	virtual Status* translate(const string& program, Code** code);

private:
	// inherited from Visitor
#define VISITOR_FUNCTION(type, name) \
	virtual void visit##type(type* node);

	FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
	
	// Trick: use AstVar's _info field
	// instead of creating own class hierarchy
	// to deal with ids and scopes
	struct CustomData {
		CustomData(uint16_t id, uint16_t scopeId) : id(id), scopeId(scopeId) {}
		uint16_t id;
		uint16_t scopeId;
	};

	void visitFunctionDeclaration(AstFunction* node);
	void visitVariableDeclaration(AstVar* node);
	
	// help methods
	bool isNumType(VarType t) { return t == VT_INT || t == VT_DOUBLE; }
	void convertNum(VarType from, VarType to);
	void visitIntBinOp(BinaryOpNode* node);
	void visitNumBinOp(BinaryOpNode* node);
	TokenKind invertCmpOp(TokenKind op);

	// Code implementation returned by translate()
	CodeImpl* resultCode;
	// Bytecode of currently translated function
	Bytecode* currBytecode;
	// Return type of the current function
	VarType retType;
	// Result type of the last translated expression
	VarType resType;
	// Context manage
	Scope* currScope;
	vector<CustomData*> customDataStorage;
	uint16_t lastVarId;
	uint16_t lastScopeId;
};
