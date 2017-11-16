#include "ast.h"
#include "mathvm.h"
#include "parser.h"
#include "bytecode_translator.h"

#include <dlfcn.h>

#define USING_NODES(type, name) using mathvm::type;
FOR_NODES(USING_NODES)
#undef USING_NODES

using mathvm::AstVar;
using mathvm::AstFunction;
using my::BytecodeTranslator;

mathvm::Status * BytecodeTranslator::translate(const std::string& program, mathvm::Code ** code)
{
	mathvm::Parser parser;
	mathvm::Status * status = parser.parseProgram(program);

	if (!status->isOk()) {
		std::cerr << status->getError() << '\n';
		return status;
	}

	hd = dlopen(nullptr, RTLD_LAZY | RTLD_NODELETE);
	this->code = new my::Code();
	*code = this->code;

	registerAll(parser.top()->scope());
	registerAllFunctions(parser.top()->scope());

	for (auto f : functions) {
		bytecode = dynamic_cast<BytecodeFunctionE*>(this->code->functionByName(f->name()))->bytecode();
		translateFunction(f);
	}

	// add global functions
	mathvm::Scope * scope = parser.top()->node()->body()->scope();
	mathvm::Scope::VarIterator iv(scope);
	while (iv.hasNext()) {
		AstVar * var = iv.next();
		this->code->addGlobalVar(var->name(), varsIDs[var]);
	}

	dlclose(hd);

	return status;
}

using I = mathvm::Instruction;

void BytecodeTranslator::registerAll(mathvm::Scope * scope)
{	
	scopes.push_back(scope);
	uint16_t scopeID = scopes.size() - 1;
	scopesIDs[scope] = scopeID;

	for (size_t i = 0; i < scope->childScopeNumber(); ++i) {
		mathvm::Scope * child = scope->childScopeAt(i);
		registerAll(child);
	}

	mathvm::Scope::VarIterator ivar(scope);
	while (ivar.hasNext()) {
		AstVar * var = ivar.next();
		assert(scope == var->owner());
		vars[scopeID].push_back(var);
		uint16_t varID = vars[scopeID].size() - 1;
		varsIDs[var] = varID;
	}
}

void BytecodeTranslator::registerAllFunctions(mathvm::Scope * scope)
{
	mathvm::Scope::FunctionIterator ifun(scope);
	while (ifun.hasNext()) {
		AstFunction * fun = ifun.next();
		functions.push_back(fun);
		code->addFunction(new BytecodeFunctionE(scopesIDs[fun->scope()], fun));
	}

	for (size_t i = 0; i < scope->childScopeNumber(); ++i) {
		mathvm::Scope * child = scope->childScopeAt(i);
		registerAllFunctions(child);
	}
}

void BytecodeTranslator::translateFunction(mathvm::AstFunction * fun)
{
	mathvm::Scope * scope = fun->scope();
	uint16_t scopeID = scopesIDs[scope];

	nativescope = scope; // ugly

	for (size_t i = 0; i < fun->parametersNumber(); ++i) {
		mathvm::VarType aType = fun->parameterType(i);
		if (aType == mathvm::VT_INT) {
			bytecode->addInsn(I::BC_STORECTXIVAR);
		} else if (aType == mathvm::VT_DOUBLE) {
			bytecode->addInsn(I::BC_STORECTXDVAR);
		} else {
			bytecode->addInsn(I::BC_STORECTXSVAR);
		}

		AstVar * var = scope->lookupVariable(fun->parameterName(i));
		uint16_t varID = varsIDs[var];

		bytecode->addTyped(scopeID);
		bytecode->addTyped(varID);
	}

	returnType = fun->returnType();
	fun->node()->visit(this);

	assert(typeStack.empty());
}

void BytecodeTranslator::range(BinaryOpNode * range)
{
	// PASS
}

void BytecodeTranslator::unifyNumTypes()
{
	mathvm::VarType lType = typeStack.top();
	typeStack.pop();

	mathvm::VarType rType = typeStack.top();
	typeStack.pop();

	if (lType != rType) {
		if (rType == mathvm::VT_INT) {
			bytecode->addInsn(I::BC_SWAP);
			bytecode->addInsn(I::BC_I2D);
			bytecode->addInsn(I::BC_SWAP);
		} else {
			bytecode->addInsn(I::BC_I2D);
		}

		rType = lType = mathvm::VT_DOUBLE;
	}

	typeStack.push(rType);
	typeStack.push(lType);
}

void BytecodeTranslator::toBool()
{
	mathvm::Label L0(bytecode);
	mathvm::Label L1(bytecode);
	bytecode->addInsn(I::BC_ILOAD1);
	bytecode->addBranch(I::BC_IFICMPE, L0);
	bytecode->addInsn(I::BC_ILOAD0);
	bytecode->addBranch(I::BC_JA, L1);
	bytecode->bind(L0);
	bytecode->addInsn(I::BC_ILOAD1);
	bytecode->bind(L1);
}

void BytecodeTranslator::binCmp(mathvm::BinaryOpNode * node)
{
	unifyNumTypes();
	
	mathvm::VarType resType = typeStack.top();
	typeStack.pop();
	typeStack.pop();
	typeStack.push(mathvm::VT_INT);

	mathvm::TokenKind kind = node->kind();

	if (resType == mathvm::VT_INT) {
		bytecode->addInsn(I::BC_ICMP);
	} else {
		bytecode->addInsn(I::BC_DCMP);
	}

	mathvm::Label oL(bytecode);
	mathvm::Label L0(bytecode);

	if (kind == mathvm::tEQ) {
		bytecode->addInsn(I::BC_ILOAD0);
		bytecode->addBranch(I::BC_IFICMPNE, L0);
	} else if (kind == mathvm::tNEQ) {
		bytecode->addInsn(I::BC_ILOAD0);
		bytecode->addBranch(I::BC_IFICMPE, L0);
	} else if (kind == mathvm::tLT) {
		bytecode->addInsn(I::BC_ILOADM1);
		bytecode->addBranch(I::BC_IFICMPNE, L0);
	} else if (kind == mathvm::tLE) {
		bytecode->addInsn(I::BC_ILOAD1);
		bytecode->addBranch(I::BC_IFICMPE, L0);
	} else if (kind == mathvm::tGT) {
		bytecode->addInsn(I::BC_ILOAD1);
		bytecode->addBranch(I::BC_IFICMPNE, L0);
	} else if (kind == mathvm::tGE) {
		bytecode->addInsn(I::BC_ILOADM1);
		bytecode->addBranch(I::BC_IFICMPE, L0);
	}

	bytecode->addInsn(I::BC_ILOAD1);
	bytecode->addBranch(I::BC_JA, oL);
	bytecode->bind(L0);
	bytecode->addInsn(I::BC_ILOAD0);
	bytecode->bind(oL);
}

void BytecodeTranslator::binMath(mathvm::BinaryOpNode * node)
{
	unifyNumTypes();
	
	mathvm::VarType resType = typeStack.top();
	typeStack.pop();

	mathvm::TokenKind kind = node->kind();

	if (resType == mathvm::VT_INT) {
		if (kind == mathvm::tADD) {
			bytecode->addInsn(I::BC_IADD);
		} else if (kind == mathvm::tSUB) {
			bytecode->addInsn(I::BC_ISUB);
		} else if (kind == mathvm::tMUL) {
			bytecode->addInsn(I::BC_IMUL);
		} else if (kind == mathvm::tDIV) {
			bytecode->addInsn(I::BC_IDIV);
		} else if (kind == mathvm::tMOD) {
			bytecode->addInsn(I::BC_IMOD);
		} else if (kind == mathvm::tAOR) {
			bytecode->addInsn(I::BC_IAOR);
		} else if (kind == mathvm::tAAND) {
			bytecode->addInsn(I::BC_IAAND);
		} else if (kind == mathvm::tAXOR) {
			bytecode->addInsn(I::BC_IAXOR);
		}
	} else if (resType == mathvm::VT_DOUBLE) {
		if (kind == mathvm::tADD) {
			bytecode->addInsn(I::BC_DADD);
		} else if (kind == mathvm::tSUB) {
			bytecode->addInsn(I::BC_DSUB);
		} else if (kind == mathvm::tMUL) {
			bytecode->addInsn(I::BC_DMUL);
		} else if (kind == mathvm::tDIV) {
			bytecode->addInsn(I::BC_DDIV);
		}
	}
}

void BytecodeTranslator::binLogic(mathvm::BinaryOpNode * node)
{
	unifyNumTypes();
	
	typeStack.pop();

	mathvm::TokenKind kind = node->kind();

	toBool();
	bytecode->addInsn(I::BC_SWAP);
	toBool();
	
	if (kind == mathvm::tAND) {
		bytecode->addInsn(I::BC_IAAND);
	} else if (kind == mathvm::tOR) {
		bytecode->addInsn(I::BC_IAOR);
	}
}

void BytecodeTranslator::visitBinaryOpNode(mathvm::BinaryOpNode * node)
{
	node->right()->visit(this);
	node->left()->visit(this);

	switch (node->kind()) {
		case mathvm::tRANGE:
			range(node);
			return;
		case mathvm::tEQ:
		case mathvm::tNEQ:
		case mathvm::tLT:
		case mathvm::tLE:
		case mathvm::tGT:
		case mathvm::tGE:
			binCmp(node);
			return;
		case mathvm::tOR:
		case mathvm::tAND:
			binLogic(node);
			return;
		default:
			binMath(node);
			return;
	}
}

void BytecodeTranslator::unLogic(UnaryOpNode * node)
{
	typeStack.pop();
	
	toBool();

	bytecode->addInsn(I::BC_ILOAD1);
	bytecode->addInsn(I::BC_IAXOR);

	typeStack.push(mathvm::VT_INT);
}

void BytecodeTranslator::unMath(UnaryOpNode * node)
{
	mathvm::VarType vType = typeStack.top();

	if (vType == mathvm::VT_INT) {
		bytecode->addInsn(I::BC_INEG);
	} else if (vType == mathvm::VT_DOUBLE) {
		bytecode->addInsn(I::BC_DNEG);
	}
}

void BytecodeTranslator::visitUnaryOpNode(UnaryOpNode * node)
{
	node->visitChildren(this);

	switch (node->kind()) {
		case mathvm::tNOT:
			unLogic(node);
			return;
		case mathvm::tSUB:
			 unMath(node);
			 return;
		default:
			std::cerr << "IMPOSSIBLE: " << tokenOp(node->kind()) << "\n";
			assert(false);
	}
}


void BytecodeTranslator::visitStringLiteralNode(StringLiteralNode * node)
{
	bytecode->addInsn(I::BC_SLOAD);
	uint16_t id = code->makeStringConstant(node->literal());
	bytecode->addTyped(id);
	typeStack.push(mathvm::VT_STRING);
}


void BytecodeTranslator::visitDoubleLiteralNode(DoubleLiteralNode * node)
{
	bytecode->addInsn(I::BC_DLOAD);
	bytecode->addTyped(node->literal());
	typeStack.push(mathvm::VT_DOUBLE);
}


void BytecodeTranslator::visitIntLiteralNode(IntLiteralNode * node)
{
	bytecode->addInsn(I::BC_ILOAD);
	bytecode->addTyped(node->literal());
	typeStack.push(mathvm::VT_INT);
}


void BytecodeTranslator::visitLoadNode(LoadNode * node)
{
	typeStack.push(node->var()->type());

	if (typeStack.top() == mathvm::VT_INT) {
		bytecode->addInsn(I::BC_LOADCTXIVAR);
	} else if (typeStack.top() == mathvm::VT_DOUBLE) {
		bytecode->addInsn(I::BC_LOADCTXDVAR);
	} else if (typeStack.top() == mathvm::VT_STRING) {
		bytecode->addInsn(I::BC_LOADCTXSVAR);
	}

	uint16_t scopeID = scopesIDs[node->var()->owner()];
	uint16_t varID = varsIDs[node->var()];

	bytecode->addTyped(scopeID);
	bytecode->addTyped(varID);
}

// typeStack 0
void BytecodeTranslator::castTo(mathvm::VarType t)
{
	mathvm::VarType e = typeStack.top();

	if (e != t) {
		if (e == mathvm::VT_INT) {
			bytecode->addInsn(I::BC_I2D);
		} else {
			bytecode->addInsn(I::BC_D2I);
		}

		typeStack.pop();
		typeStack.push(t);
	}
}

// typeStack 0
void BytecodeTranslator::visitStoreNode(StoreNode * node)
{
	node->value()->visit(this);
	castTo(node->var()->type());

	mathvm::VarType sType = typeStack.top();
	mathvm::TokenKind kind = node->op();

	uint16_t scopeID = scopesIDs[node->var()->owner()];
	uint16_t varID = varsIDs[node->var()];

	if (kind != mathvm::tASSIGN) {
		bytecode->addInsn(I::BC_LOADCTXDVAR);
		bytecode->addTyped(scopeID);
		bytecode->addTyped(varID);
	
		if (kind == mathvm::tINCRSET) {
			if (sType == mathvm::VT_INT) {
				bytecode->addInsn(I::BC_IADD);
			} else {
				bytecode->addInsn(I::BC_DADD);
			}
		} else {
			if (sType == mathvm::VT_INT) {
				bytecode->addInsn(I::BC_ISUB);
			} else {
				bytecode->addInsn(I::BC_DSUB);
			}
		}
	}
	
	if (sType == mathvm::VT_INT) {
		bytecode->addInsn(I::BC_STORECTXIVAR);
	} else if (sType == mathvm::VT_DOUBLE) {
		bytecode->addInsn(I::BC_STORECTXDVAR);
	} else {
		bytecode->addInsn(I::BC_STORECTXSVAR);
	}

	bytecode->addTyped(scopeID);
	bytecode->addTyped(varID);

	typeStack.pop();
}


void BytecodeTranslator::visitForNode(ForNode * node)
{
	mathvm::Label bodyLabel(bytecode);
	mathvm::Label condLabel(bytecode);
	mathvm::Label outLabel(bytecode);

	uint16_t scopeID = scopesIDs[node->var()->owner()];
	uint16_t varID = varsIDs[node->var()];

	BinaryOpNode * range = dynamic_cast<BinaryOpNode*>(node->inExpr());
	assert(range);
	assert(range->kind() == mathvm::tRANGE);

	range->right()->visit(this);
	typeStack.pop();
	range->left()->visit(this);
	typeStack.pop();

	bytecode->addBranch(I::BC_JA, condLabel);
	bytecode->bind(bodyLabel);

	bytecode->addInsn(I::BC_STOREIVAR0);
	bytecode->addInsn(I::BC_LOADIVAR0);
	bytecode->addInsn(I::BC_LOADIVAR0);
	bytecode->addInsn(I::BC_STORECTXIVAR);
	bytecode->addTyped(scopeID);
	bytecode->addTyped(varID);

	node->body()->visit(this);
	
	bytecode->addInsn(I::BC_ILOAD1);
	bytecode->addInsn(I::BC_IADD);

	bytecode->bind(condLabel);
	
	bytecode->addInsn(I::BC_STOREIVAR0);
	bytecode->addInsn(I::BC_STOREIVAR1);
	bytecode->addInsn(I::BC_LOADIVAR1);
	bytecode->addInsn(I::BC_LOADIVAR0);
	bytecode->addInsn(I::BC_LOADIVAR1);
	bytecode->addInsn(I::BC_LOADIVAR0);

	bytecode->addBranch(I::BC_IFICMPLE, bodyLabel);

	bytecode->addInsn(I::BC_POP);
	bytecode->addInsn(I::BC_POP);
}


void BytecodeTranslator::visitWhileNode(WhileNode * node)
{
	mathvm::Label conditionLabel(bytecode);
	mathvm::Label outLabel(bytecode);
	bytecode->bind(conditionLabel);

	node->whileExpr()->visit(this);

	bytecode->addInsn(I::BC_ILOAD0);
	bytecode->addBranch(I::BC_IFICMPE, outLabel);

	node->loopBlock()->visit(this);
	bytecode->addBranch(I::BC_JA, conditionLabel);

	bytecode->bind(outLabel);

	typeStack.pop();
}


void BytecodeTranslator::visitIfNode(IfNode * node)
{
	mathvm::Label elseLabel(bytecode);
	mathvm::Label outLabel(bytecode);

	node->ifExpr()->visit(this);
	typeStack.pop();
	bytecode->addInsn(I::BC_ILOAD0);

	if (node->elseBlock()) {
		bytecode->addBranch(I::BC_IFICMPE, elseLabel);
		node->thenBlock()->visit(this);
		bytecode->addBranch(I::BC_JA, outLabel);
		bytecode->bind(elseLabel);
		node->elseBlock()->visit(this);
		bytecode->bind(outLabel);
	} else {
		bytecode->addBranch(I::BC_IFICMPE, outLabel);
		node->thenBlock()->visit(this);
		bytecode->bind(outLabel);
	}

	// assert(typeStack.empty());
}


void BytecodeTranslator::visitBlockNode(BlockNode * node)
{
	for (size_t i = 0; i < node->nodes(); ++i) {
		node->nodeAt(i)->visit(this);
		if (dynamic_cast<LoadNode*>(node->nodeAt(i))
			|| (dynamic_cast<CallNode*>(node->nodeAt(i))
			&& this->code->functionByName(dynamic_cast<CallNode*>(node->nodeAt(i))->name())->returnType() != mathvm::VT_VOID)) {
			bytecode->addInsn(I::BC_POP);
			typeStack.pop();
		}
	}
}


void BytecodeTranslator::visitFunctionNode(FunctionNode * node)
{
	node->visitChildren(this);	
}


void BytecodeTranslator::visitReturnNode(ReturnNode * node)
{
	node->visitChildren(this);
	
	if (node->returnExpr()) {
		castTo(returnType);

		typeStack.pop();
	}

	bytecode->addInsn(I::BC_RETURN);
}


void BytecodeTranslator::visitCallNode(CallNode * node)
{
	size_t args_n = node->parametersNumber();
	mathvm::TranslatedFunction * fun = code->functionByName(node->name());
	assert(fun);

	for (size_t i = 0; i < args_n; ++i) {
		size_t ai = args_n - i - 1;
		node->parameterAt(ai)->visit(this);
		castTo(fun->parameterType(ai));
		typeStack.pop();
	}

	bytecode->addInsn(I::BC_CALL);
	uint16_t funID =fun->id();
	bytecode->addTyped(funID);
	if (fun->returnType() != mathvm::VT_VOID) {
		typeStack.push(fun->returnType());
	}
}


void BytecodeTranslator::visitNativeCallNode(NativeCallNode * node)
{
	const mathvm::Signature& sign = node->nativeSignature();

	for (size_t i = 1; i < sign.size(); ++i) {
		AstVar * var = nativescope->lookupVariable(sign[sign.size() - i].second);
		assert(var);

		uint16_t scopeID = scopesIDs[nativescope];
		uint16_t varID = varsIDs[var];

		bytecode->addInsn(I::BC_LOADCTXDVAR);
		bytecode->addTyped(scopeID);
		bytecode->addTyped(varID);
	}

	bytecode->addInsn(I::BC_CALLNATIVE);
	void * addr = dlsym(hd, node->nativeName().c_str());
	assert(addr);

	uint16_t id = code->makeNativeFunction(node->nativeName(), node->nativeSignature(), addr);
	bytecode->addTyped(id);
}


void BytecodeTranslator::visitPrintNode(PrintNode * node)
{
	for (size_t i = 0; i < node->operands(); ++i) {		
		node->operandAt(i)->visit(this);
		mathvm::VarType pType = typeStack.top();
		typeStack.pop();

		if (pType == mathvm::VT_INT) {
			bytecode->addInsn(I::BC_IPRINT);
		} else if (pType == mathvm::VT_DOUBLE) {
			bytecode->addInsn(I::BC_DPRINT);
		} else {
			bytecode->addInsn(I::BC_SPRINT);
		}	
	}
}
