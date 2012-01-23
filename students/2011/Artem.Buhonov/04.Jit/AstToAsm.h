#pragma once
#include "ast.h"
#include "mathvm.h"
#include "VarsSearcherVisitor.h"
#include "AsmJit/Compiler.h"
#include "AsmJit/Logger.h"
#include <vector>

typedef void (*CompiledFunc)();

struct AsmVar {
	AsmVar(AsmJit::BaseVar *var, mathvm::VarType type) : Var(var), Type(type) {}
	AsmJit::BaseVar *Var;
	mathvm::VarType Type;
};

union Value {
	double Double;
	int64_t Int;
	char *Str;
};

class AstToAsm : public mathvm::AstVisitor
{
public:	
	AstToAsm(int paramsCount = 0) : _currentFreeVarId(1), _currentFreeFuncId(0), _paramsCount(paramsCount), _localsCount(0) {}
	virtual ~AstToAsm(void) {};
	

	CompiledFunc compile(mathvm::AstFunction *root);

	virtual void visitBinaryOpNode(mathvm::BinaryOpNode* node);
	virtual void visitUnaryOpNode(mathvm::UnaryOpNode* node);
	virtual void visitStringLiteralNode(mathvm::StringLiteralNode* node);
	virtual void visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node);
	virtual void visitIntLiteralNode(mathvm::IntLiteralNode* node);
	virtual void visitLoadNode(mathvm::LoadNode* node);
	virtual void visitStoreNode(mathvm::StoreNode* node);
	virtual void visitForNode(mathvm::ForNode* node);
	virtual void visitWhileNode(mathvm::WhileNode* node);
	virtual void visitIfNode(mathvm::IfNode* node);
	virtual void visitBlockNode(mathvm::BlockNode* node);
	virtual void visitPrintNode(mathvm::PrintNode* node);
	virtual void visitFunctionNode(mathvm::FunctionNode* node);
	virtual void visitCallNode(mathvm::CallNode* node);
	virtual void visitReturnNode(mathvm::ReturnNode* node);
	virtual void visitNativeCallNode( mathvm::NativeCallNode *node);

	void pushVar(const std::string &name, const mathvm::VarType type);
	void popVar(const std::string &name);


private:
	static const int VARS_LIMIT = 256 * 256;
	static const int FUNC_LIMIT = 256 * 256;
	static const size_t DATA_STACK_SIZE = 1024 * 1024 * 16;
	
	AsmJit::Compiler _c;
	
	struct FuncInfo {
		std::string name;
		//mathvm::VarType returnType;
		VarsSearcherVisitor *vsv;
		mathvm::AstFunction *funcNode;
	};

	static CompiledFunc _functions[FUNC_LIMIT];
	static FuncInfo _funcInfos[FUNC_LIMIT];	
	static int _funcCount;
	static Value *_stack;
	static int64_t *_framePtr;
	//static int64_t *_localsPtr;
	//static int64_t *_tosPtr;
	AsmJit::GPVar *_frame;
	//AsmJit::GPVar *_locals;
	//AsmJit::GPVar *_tos;
	

	static void throwException(const std::string &what);

	static void callProxy(uint64_t funcId) {
		_functions[funcId]();
	}

	static int64_t getFuncId(const std::string &name) {
		for (int i = 0; i < _funcCount; ++i) {
			if (_funcInfos[i].name == name) {
				return i;
			}
		}
		if (_funcCount + 1 == FUNC_LIMIT) {
			throwException("Function limit exceeded");
		}
		_funcInfos[_funcCount].name = name;
		_funcCount++;
		return _funcCount - 1;
	}

	static mathvm::AstFunction * getFuncNode(int64_t funcId) {
		return _funcInfos[funcId].funcNode;
	}

	static void setFuncNode(int64_t funcId, mathvm::AstFunction *funcNode) {
		_funcInfos[funcId].funcNode = funcNode;
	}

	/*static mathvm::VarType getFuncRetType(int64_t id) {
		return _funcInfos[id].returnType;
	}*/

	static VarsSearcherVisitor * getFuncVarsVisitor(int64_t id) {
		return _funcInfos[id].vsv;
	}

	static void setFuncVarsVisitor(int64_t funcId, VarsSearcherVisitor *vsv) {
		_funcInfos[funcId].vsv = vsv;
	}

	/*static void setFuncRetType(int64_t funcId, mathvm::VarType retType) {
		_funcInfos[funcId].returnType = retType;
	}*/

	static void linkFunc(uint64_t funcId, CompiledFunc ptr) {
		_functions[funcId] = ptr;
	}

	static void printInt(int64_t val) {
#ifdef _WIN32			
		printf("%lld", val);
#else
		printf("%ld", val);
#endif
	}

	static void printDouble(double val) {
		printf("%f", val);
	}

	static void printStr(char *val) {
		printf("%s", val);
	}

	static int64_t div(int64_t val, int64_t div) {
		return val / div;
	}

	AsmJit::XMMVar * insertI2D(AsmJit::GPVar *src);
	AsmJit::GPVar * insertD2I(AsmJit::XMMVar *src);
	void convertLastVarTo(mathvm::VarType type);

	void insertData(const void *data, size_t size);
	void insertVarId(const std::string &name);
	uint16_t getVarId(const std::string &name);
	uint32_t getVarOffset(const std::string &name);
	uint32_t getParamOffset(int index);
	void checkCurrentType(mathvm::VarType excpectedType);
	void checkIfInsn(AsmJit::CONDITION cond);
	std::string typeToString(mathvm::VarType type);
	
	std::vector<AsmVar> _asmVars;
	mathvm::VarType _lastType;
	typedef uint16_t VarInt;
	//std::map<std::string, std::vector<AsmVar> > _vars;
	std::map<std::string, std::vector<VarInt> > _vars;
	int _currentFreeVarId;
	int _currentFreeFuncId;
	//mathvm::Bytecode *_bytecode;		
	mathvm::Code *_code;	
	AsmJit::BaseVar *_lastVar;	
	int _paramsCount;
	int _localsCount;
	mathvm::AstFunction *_rootFunc;
};
