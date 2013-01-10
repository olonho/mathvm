#ifndef __vm__bytecode_visitor__
#define __vm__bytecode_visitor__

#include <stack>
#include <map>

#include "ast.h"

namespace mathvm {

class BytecodeVisitor:public AstVisitor {
	AstFunction* top;
	Code* code;
    Scope* currentScope;
	VarType TOSType;
    VarType returnType;
    vector<const AstVar*> locals;
    vector<const AstVar*> blockLocals;

	stack<std::pair<BytecodeFunction*, AstFunction*> > functionsStack;
	map<const AstVar*, uint16_t > varsMap;
	
	void processFunctionsDeclarations(Scope* scope);
	void processVariablesDeclarations(Scope* scope);
    
    void processForvardFunctionsDeclarations(Scope* scope);

	Bytecode* getActualBytecode();
	uint16_t getVarID(const AstVar* var);
	void castToIntOnly(VarType type1, VarType type2);
	VarType castToSameType(VarType type1, VarType type2);
	void compareInts(Instruction instruction);
	void compareDoubles(Instruction instruction);
    void loadVar(const AstVar* var);
    void initVar(const AstVar* var);
    void storeVar(const AstVar* var);
    void pushVars(const vector<const AstVar*>& vars);
    void initVars(const vector<const AstVar*>& vars);
    void popVars(const vector<const AstVar*>& vars, bool swap);
    void findLocals(const FunctionNode* func);
    
    #define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node);
    
        FOR_NODES(VISITOR_FUNCTION)
    #undef VISITOR_FUNCTION
public:
	
	BytecodeVisitor(AstFunction* top, Code* code);
	virtual ~BytecodeVisitor() {}
	void visit();
};

}

#endif /* defined(__vm__bytecode_visitor__) */
