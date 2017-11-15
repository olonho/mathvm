#pragma once

#include <map>
#include <unordered_map>
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

	class MyFrame {
		typedef std::unordered_map<uint16_t, Val> Scope;
		std::unordered_map<uint16_t, Scope> scopes;
		
	public:
		Val& val(uint16_t ctxId, uint16_t varId) {
			return scopes[ctxId][varId];
		}
	};

	class MyStack {
		size_t sz = 0;
		uint16_t topID;
		std::map<uint16_t, std::stack<MyFrame>> frames;
		std::stack<uint16_t> minIDs;
	public:
		void push(uint16_t id) {
			topID = id;
			minIDs.push(id);
			frames[id].push(MyFrame());
		}

		void pop() {
			minIDs.pop();
			frames[topID].pop();
			topID = minIDs.empty() ? 0 : minIDs.top();
		}

		MyFrame& findClosure(uint16_t idCtx) {
			auto it = --frames.lower_bound(idCtx);
			return it->second.top();
		}

		MyFrame& top() {
			return frames[topID].top();
		}

		bool topContainsScope(uint16_t scopeID) {
			return scopeID >= topID;
		}
	};

	MyStack memory;
	std::unordered_map<uint16_t, Val> vars;
	std::unordered_map<std::string, uint16_t> globalVars;
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
