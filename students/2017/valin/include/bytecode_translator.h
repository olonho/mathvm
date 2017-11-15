#pragma once

#include <map>
#include <string>
#include <iostream>
#include <stack>
#include <vector>

#include "ast.h"
#include "mathvm.h"
#include "visitors.h"

namespace my {

class Code;

class BytecodeTranslator : public mathvm::AstVisitor, public mathvm::Translator
{
public:
	// BytecodeTranslator(bool executable)
	virtual mathvm::Status * translate(const std::string& program, mathvm::Code ** code) override;
	void processFunction(mathvm::AstFunction * fun);

#define VISITOR_FUNCTION(type, name) \
	virtual void visit##type(mathvm::type* node);

	FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
	Code * code = nullptr;
	mathvm::Bytecode * bytecode = nullptr;
	std::stack<mathvm::VarType> typeStack;

	std::vector<mathvm::AstFunction*> functions;
	
	std::vector<mathvm::Scope*> scopes;
	std::map<mathvm::Scope*, uint16_t> scopesIDs;

	std::map<uint16_t, std::vector<mathvm::AstVar*>> vars;
	std::map<const mathvm::AstVar*, uint16_t> varsIDs;

	mathvm::VarType returnType;

	void * hd = nullptr;

	void registerAll(mathvm::Scope * scope);
	void registerAllFunctions(mathvm::Scope * scope);
	void translateFunction(mathvm::AstFunction * fun);
	void range(mathvm::BinaryOpNode * range);
	void unifyNumTypes();
	void toBool();
	void binCmp(mathvm::BinaryOpNode * node);
	void binMath(mathvm::BinaryOpNode * node);
	void binLogic(mathvm::BinaryOpNode * node);
	void unLogic(mathvm::UnaryOpNode * node);
	void unMath(mathvm::UnaryOpNode * node);
	void castTo(mathvm::VarType t);
};

class BytecodeFunctionE : public mathvm::BytecodeFunction
{
	uint16_t minid;
public:
	BytecodeFunctionE(uint16_t id , mathvm::AstFunction * function) : BytecodeFunction(function), minid(id) {}

	uint16_t minID() { return minid; }
};

class Code : public mathvm::Code
{
	union Val {
		double D;
        int64_t I;
        const char* S;
	};

	std::vector<uint16_t> minID;
	std::vector<std::map<uint16_t, std::map<uint16_t, Val>>> memory;
	std::map<uint16_t, Val> vars;
	std::map<std::string, uint16_t> globalVars;
	std::stack<Val> stack;
	std::stack<std::pair<mathvm::Bytecode*, size_t>> instructions;
	mathvm::Bytecode * bytecode;
	size_t * IP = 0;

	mathvm::Instruction fetch();
	bool hasNextInstruction();
	Val& getVal(uint16_t scopeID, uint16_t varID);

	#define INS(b, s, l) void b();
		FOR_BYTECODES(INS)
	#undef INS

public:
	virtual ~Code() { }

	virtual void disassemble(std::ostream& out = std::cout, mathvm::FunctionFilter* filter = nullptr) override;

	virtual mathvm::Status* execute(std::vector<mathvm::Var*>& vars) override;
	
	void addGlobalVar(const std::string& var, uint16_t id) { globalVars[var] = id; }
};

}
