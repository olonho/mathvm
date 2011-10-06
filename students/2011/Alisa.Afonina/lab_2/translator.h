#pragma once
#include <map>

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

class GeneratedCode : public mathvm :: Code {
	mathvm :: Bytecode _bytecode;
	public:
		virtual mathvm::Status* execute(std::vector<mathvm::Var*> vars) {return NULL;}
		mathvm :: Bytecode& getBytecode() {
			return _bytecode;
		}
		void setBytecode(mathvm :: Bytecode& bytecode) {
			_bytecode = bytecode;
		}		
};

class GeneratingVisitor : public mathvm::AstVisitor {
    mathvm :: Bytecode bytecode;
    VarTable varTable;
    mathvm :: VarType previousType; //last operation type
    GeneratedCode code;
	
public:
	GeneratingVisitor(){} 
		
#define VISITOR_FUNCTION(type, name) \
	virtual void visit##type(mathvm::type* node);
	
	FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

	void visit(mathvm::BlockNode* node);
	void dump();
    mathvm :: Code* getCode() {
		code.setBytecode(bytecode);
		return &code;
	}
	void getIfResult(mathvm :: Label& end,
					 mathvm :: Instruction insn,
					 mathvm :: Instruction ifinsn);
	
	mathvm :: Bytecode * getBytecode();
	std::vector<std::string> getStringsVector();
};
