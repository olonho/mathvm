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

#include <dlfcn.h>
#include <fstream>

#include "asmjit/asmjit.h"

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
	mathvm::Scope * nativescope = nullptr;

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

	class Function
	{
	    const mathvm::Signature * sign;
	    const void * addr;
	    std::vector<Val> val;
		void (*foo)(void *, void *, void *);
	public:
	    Function(const mathvm::Signature * sign, const void * addr, std::vector<Val> val) : sign(sign), addr(addr), val(val) {}

	    std::string strType(std::pair<mathvm::VarType, std::string> type) {
	        if (type.first == mathvm::VarType::VT_DOUBLE) {
	            return "double";
	        }

	        return "int";
	    }

	    Val call() {
			using namespace asmjit;
			using namespace asmjit::x86;

	        JitRuntime runtime;
			X86Assembler as(&runtime);
			X86Compiler c(&as);

			c.addFunc(FuncBuilder3<void, void*, void*, void*>(kCallConvX64Unix));

			c.emit(kX86InstIdPush, rbx);
			// store fun pointer
			c.emit(kX86InstIdPush, rdi);
			// move res pointer
			c.emit(kX86InstIdMov, rbx, rsi);
			// move args pointer
			c.emit(kX86InstIdMov, rax, rdx);

			constexpr int iCntMax = 6;
			int iCnt = 0;

			constexpr int dCntMax = 8;
			int dCnt = 0;

			const asmjit::X86GpReg* regs[] = { &rdi, &rsi, &rdx, &rcx, &r8, &r9 };
			std::vector<X86Mem> mem;

			for (size_t i = 1; i < sign->size(); ++i) {
				auto p = c.intptr_ptr(rax, (i - 1) * 8);
				mem.push_back(p);

				if (dCnt < dCntMax && sign->at(i).first == mathvm::VarType::VT_DOUBLE) {
					c.emit(kX86InstIdMovsd, xmm(dCnt), p);
					dCnt++;
				} else if (iCnt < iCntMax) {
					c.emit(kX86InstIdMov, *regs[iCnt], p);
					iCnt++;
				} else {
					assert(false);
				}
			}

			// pop fun pointer
			c.emit(kX86InstIdPop, rax);
			c.emit(kX86InstIdCall, rax);

			auto p = c.intptr_ptr(rbx);
			if (sign->at(0).first == mathvm::VarType::VT_DOUBLE) {
				c.emit(kX86InstIdMovsd, p, xmm(0));
			} else {
				c.emit(kX86InstIdMov, p, rax);
			}

			c.emit(kX86InstIdPop, rbx);
			c.ret();
			c.endFunc();
			c.finalize();

			/////////////

			foo = asmjit_cast<void (*)(void *, void *, void *)>(as.make());

			Val res;
			foo(const_cast<void*>(addr), &res, &val[0]);
			return res;
		}
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
			auto it = --frames.upper_bound(idCtx);
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
