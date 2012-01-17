#pragma once
#include <map>
#include <list>
#include "myinterpreter.h"
class GeneratingException {
	std::string _message;  
public:
	GeneratingException(std::string const& message) : _message(message) {}
	virtual std::string message() const {
		return _message;
	}
};

class VarTable {
	std :: map<mathvm :: AstVar const *, int> _map;
	public : 
		int getVarId(mathvm :: AstVar const * var) {return _map[var];}
		int registerVariable(mathvm :: AstVar* var) {
			_map[var] = _map.size();
			return _map.size() - 1;
		}
};
struct InfoId {
	uint16_t _function_id;
	uint16_t _var_counter;
	mathvm :: AstFunction* _ast_function;
	bool _is_function;
	int _vars_inside_scope_count;
	std :: map<std::string, uint16_t> variables;
	InfoId():_is_function(false), _vars_inside_scope_count(0) {};

};

struct FunctionInfo {
	uint16_t _function_id;
	mathvm :: AstFunction* _ast_function;
};



class GeneratingVisitor : public mathvm::AstVisitor {
    mathvm :: Bytecode * currentBytecode;
    VarTable varTable;
    mathvm :: VarType previousType; //last operation type
	mathvm :: Scope * currentScope; //currentScope
	GeneratedCode code;
	std :: map <std :: string, FunctionInfo> _functions;
	std :: map <mathvm :: Scope *, InfoId> _ids;

public:
	GeneratingVisitor(){} 
		
#define VISITOR_FUNCTION(type, name) \
	virtual void visit##type(mathvm::type* node);
	
	FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

	void visit(mathvm::AstFunction* node);
	void dump();
    mathvm :: Code* getCode() {
		
		return &code;
	}
	void getIfResult(mathvm :: Label& end,
					 mathvm :: Instruction insn,
					 mathvm :: Instruction ifinsn);
	bool getVarId(std :: string const & name, InfoId& id_out);
	//mathvm :: Bytecode * getBytecode();
	std::vector<std::string> getStringsVector();
};
