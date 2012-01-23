#include "AstToAsm.h"
#include "Exceptions.h"
#include <string.h>
#include "VarsSearcherVisitor.h"

using namespace mathvm;
using namespace AsmJit;

CompiledFunc AstToAsm::_functions[FUNC_LIMIT];
AstToAsm::FuncInfo AstToAsm::_funcInfos[FUNC_LIMIT];
int AstToAsm::_funcCount = 0;
Value *AstToAsm::_stack = new Value[DATA_STACK_SIZE];
int64_t *AstToAsm::_framePtr = (int64_t *)_stack;



CompiledFunc AstToAsm::compile(mathvm::AstFunction *root) {
	_rootFunc = root;
	FileLogger logger(stderr);				
	_c.setLogger(&logger);		
	_c.newFunction(AsmJit::CALL_CONV_DEFAULT, FunctionBuilder0<void>());
	_c.getFunction()->setHint(FUNCTION_HINT_NAKED, true);
	_frame = new GPVar(_c.newGP(1U, "_frame"));
	GPVar frameAddr(_c.newGP());
	_c.mov(frameAddr, imm((sysint_t)&_framePtr));
	_c.mov(*_frame, qword_ptr(frameAddr, 0));
	//_locals = new GPVar(_c.newGP());
	//_tos = new GPVar(_c.newGP());
	//_c.mov(*_tos, imm((sysint_t)_stack));
	//_c.mov(*_tos, imm((sysint_t)_stack));
	//_c.mov(*_locals, imm((sysint_t)_stack));
	root->node()->body()->visit(this);
	//_c.ret();
	_c.endFunction();
	CompiledFunc func = (CompiledFunc)_c.make();
	return func;
}

void AstToAsm::visitBinaryOpNode( mathvm::BinaryOpNode* node )
{
	TokenKind opKind = node->kind();
	if (opKind == tOR || opKind == tAND) {		
		switch(opKind) {
			case tAND : {
				AsmJit::Label lAndFalse(_c.newLabel());
				AsmJit::Label lAndEnd(_c.newLabel());
				AsmJit::Label lAndTrue(_c.newLabel());
				GPVar *result = new GPVar(_c.newGP());
				//_bytecode->add(BC_ILOAD0);
				node->left()->visit(this);
				checkCurrentType(mathvm::VT_INT);
				//_bytecode->addBranch(BC_IFICMPE, lAndFalse);
				_c.cmp(static_cast<GPVar &>(*_lastVar), imm(0));
				_c.je(lAndFalse);
				node->right()->visit(this);
				checkCurrentType(mathvm::VT_INT);
				//_bytecode->addBranch(BC_JA, lAndEnd);
				_c.cmp(static_cast<GPVar &>(*_lastVar), imm(0));
				_c.jne(lAndTrue);
				_c.bind(lAndFalse);
				//_bytecode->add(BC_ILOAD0);
				_c.mov(*result, imm(0));
				_c.jmp(lAndEnd);
				_c.bind(lAndTrue);
				_c.mov(*result, imm(1));
				_c.bind(lAndEnd);
				_lastVar = result;
				_lastType = mathvm::VT_INT;}
				break;
			case tOR: {
				AsmJit::Label lOrTrue(_c.newLabel());
				AsmJit::Label lOrEnd(_c.newLabel());
				AsmJit::Label lOrFalse(_c.newLabel());
				GPVar *result = new GPVar(_c.newGP());
				//_bytecode->add(BC_ILOAD1);
				node->left()->visit(this);
				checkCurrentType(mathvm::VT_INT);
				//_bytecode->addBranch(BC_IFICMPE, lOrTrue);
				_c.cmp(static_cast<GPVar &>(*_lastVar), imm(0));
				_c.jne(lOrTrue);
				node->right()->visit(this);
				checkCurrentType(mathvm::VT_INT);
				//_bytecode->addBranch(BC_JA, lOrEnd);
				_c.cmp(static_cast<GPVar &>(*_lastVar), imm(0));
				_c.je(lOrFalse);
				_c.bind(lOrTrue);
				_c.mov(*result, imm(1));
				_c.jmp(lOrEnd);
				_c.bind(lOrFalse);
				_c.mov(*result, imm(0));
				_c.bind(lOrEnd); 
				_lastVar = result;
				_lastType = mathvm::VT_INT;}
				break;
			default: 
				throwException("Invalid logic operation");
		}
		return;
	}
	node->right()->visit(this);
	BaseVar *right = _lastVar;
	VarType rightType = _lastType;
	_lastType = VT_INVALID;
	node->left()->visit(this);
	VarType leftType = _lastType;
	BaseVar *left = _lastVar;
	switch(opKind) {
		case tADD :
		case tSUB :
		case tMUL :
		case tDIV :
		case tEQ :
		case tNEQ :
		case tGT :
		case tLT :
		case tGE :
		case tLE :
			if (leftType != mathvm::VT_INT && leftType != VT_DOUBLE) {
				throwException("Invalid type of left operand");
			}
			if (rightType != mathvm::VT_INT && rightType != VT_DOUBLE) {
				throwException("Invalid type of right operand");
			}
			if (leftType != rightType) {				
				if (leftType == mathvm::VT_INT) {
					XMMVar *temp = new XMMVar(_c.newXMM());
					_c.cvtsi2sd(*temp, static_cast<GPVar &>(*left));					
					delete left;
					left = temp;
					leftType = VT_DOUBLE;
				}
				else {
					XMMVar *temp = new XMMVar(_c.newXMM());
					_c.cvtsi2sd(*temp, static_cast<GPVar &>(*right));					
					delete right;
					right = temp;					
					rightType = VT_DOUBLE;					
				}
				_lastType = VT_DOUBLE;
			} break;
		default: throwException("Invalid binary operation");
	}
	if (_lastType == VT_DOUBLE) {
		//switch (opKind) {
		//	case tEQ : _c.cmpsd((XMMVar &)*left, (XMMVar &)right, imm(0)); _c.movq(
		//	case tNEQ : _c.cmpsd((XMMVar &)*left, (XMMVar &)right, imm(4)); checkIfInsn(C_NOT_EQUAL); break;
		//	case tGT : _c.cmpsd((XMMVar &)*left, (XMMVar &)right, imm(6)); checkIfInsn(C_NOT_EQUAL);
		//	case tLT : _c.cmpsd((XMMVar &)*left, (XMMVar &)right, imm(1)); checkIfInsn(C_NOT_EQUAL);
		//	case tGE : _c.cmpsd((XMMVar &)*left, (XMMVar &)right, imm(5)); checkIfInsn(C_NOT_EQUAL);
		//	case tLE : _c.cmpsd((XMMVar &)*left, (XMMVar &)right, imm(2)); checkIfInsn(C_NOT_EQUAL);
		//		_bytecode->addInsn(BC_DCMP);
		//		_lastType = mathvm::VT_INT; break;
		//	default: ;
		//}

		// CMPSD man - http://www.jaist.ac.jp/iscenter-new/mpc/altix/altixdata/opt/intel/vtune/doc/users_guide/mergedProjects/analyzer_ec/mergedProjects/reference_olh/mergedProjects/instructions/instruct32_hh/vc40.htm
		int predicate = -1;
		switch (opKind) {
			case tNEQ : predicate = 4; break;
			case tEQ : predicate = 0; break;
			case tGT: predicate = 6; break;
			case tLT : predicate = 1; break;
			case tGE : predicate = 5; break;
			case tLE : predicate = 2; break;
			default : ;
		}
		
		switch (opKind) {
			case tNEQ : 
			case tEQ : 
			case tGT : 				
			case tLT : 
			case tGE : 
			case tLE : {
					_c.cmpsd(static_cast<XMMVar &>(*left), static_cast<XMMVar &>(*right), imm(predicate));
					GPVar *res = new GPVar(_c.newGP());
					_c.movq(*res, (XMMVar &)*left);
					_c.neg(*res);
					_lastVar = res;
					_lastType = mathvm::VT_INT;
					break;
				}
			case tADD : _c.addsd(static_cast<XMMVar &>(*left), static_cast<XMMVar &>(*right)); _lastVar = left; break;
			case tSUB : _c.subsd(static_cast<XMMVar &>(*left), static_cast<XMMVar &>(*right)); _lastVar = left; break;
			case tMUL : _c.mulsd(static_cast<XMMVar &>(*left), static_cast<XMMVar &>(*right)); _lastVar = left; break;
			case tDIV : _c.divsd(static_cast<XMMVar &>(*left), static_cast<XMMVar &>(*right)); _lastVar = left; break;
			default : ; 
		}
	}
	else {
		switch (opKind) {
			case tNEQ : _c.cmp(static_cast<GPVar &>(*left), static_cast<GPVar &>(*right)); checkIfInsn(C_NOT_EQUAL); break;
			case tEQ : _c.cmp(static_cast<GPVar &>(*left), static_cast<GPVar &>(*right)); checkIfInsn(C_EQUAL); break;
			case tGT : _c.cmp(static_cast<GPVar &>(*left), static_cast<GPVar &>(*right)); checkIfInsn(C_GREATER); break;
			case tLT : _c.cmp(static_cast<GPVar &>(*left), static_cast<GPVar &>(*right)); checkIfInsn(C_LESS); break;
			case tGE : _c.cmp(static_cast<GPVar &>(*left), static_cast<GPVar &>(*right)); checkIfInsn(C_GREATER_EQUAL); break;
			case tLE : _c.cmp(static_cast<GPVar &>(*left), static_cast<GPVar &>(*right)); checkIfInsn(C_LESS_EQUAL); break;
			case tADD : _c.add(static_cast<GPVar &>(*left), static_cast<GPVar &>(*right)); _lastVar = left; break;
			case tSUB : _c.sub(static_cast<GPVar &>(*left), static_cast<GPVar &>(*right)); _lastVar = left; break;
			case tMUL : _c.imul(static_cast<GPVar &>(*left), static_cast<GPVar &>(*right)); _lastVar = left; break;
			case tDIV : {
				ECall *ctx = _c.call(div);
				ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<int64_t, int64_t, int64_t>());
				ctx->setArgument(0, static_cast<GPVar &>(*left));
				ctx->setArgument(1, static_cast<GPVar &>(*right));
				GPVar *result = new GPVar(_c.newGP());
				ctx->setReturn(*result);
				_lastVar = result;
				break;}
			default : ; 
		}
	}
	
}

void AstToAsm::visitUnaryOpNode( mathvm::UnaryOpNode* node )
{
	/*node->operand()->visit(this);
	switch(node->kind()) {
		case tSUB :
			switch(_lastType) {
				case mathvm::VT_INT : _bytecode->addInsn(BC_INEG); break;
				case VT_DOUBLE : _bytecode->addInsn(BC_DNEG); break;
				default:throwException(string("Type mismatch: expected double or int, but ") + typeToString(_lastType) + " got");
			} break;
		case tNOT : 	
			checkCurrentType(VT_INT);
			_bytecode->add(BC_ILOAD0);
			checkIfInsn(BC_IFICMPE);
			break;
		default : 
			throwException("Invalid unary operation");
	}*/
	node->operand()->visit(this);
	switch(node->kind()) {
		case tSUB :
			switch(_lastType) {
				case mathvm::VT_INT : _c.neg((GPVar &)*_lastVar); break;
				case VT_DOUBLE : {
					XMMVar zero(_c.newXMM());
					_c.subsd(zero, (XMMVar &)*_lastVar);
					_c.movq((XMMVar &)*_lastVar, zero);
					break;
					}
				default: throwException(string("Type mismatch: expected double or int, but ") + typeToString(_lastType) + " got");
			} break;
		case tNOT : {	
			checkCurrentType(mathvm::VT_INT);
			//_bytecode->add(BC_ILOAD0);
			//checkIfInsn(BC_IFICMPE);
			AsmJit::Label ifZero(_c.newLabel());
			AsmJit::Label end(_c.newLabel());
			GPVar zero(_c.newGP());
			_c.mov(zero, imm(0));
			_c.cmp(zero, (GPVar &)*_lastVar);			
			checkIfInsn(C_EQUAL);
			break;
			}
		default : 
			throwException("Invalid unary operation");
	}
}

void AstToAsm::visitStringLiteralNode( mathvm::StringLiteralNode* node )
{
	//_bytecode->addInsn(BC_SLOAD);
	//uint16_t strConstantId = _code->makeStringConstant(node->literal());
	//string str = _code->constantById(strConstantId);
	//_bytecode->addUInt16(strConstantId);
	char *str = new char[node->literal().size() + 1];
	strncpy(str, node->literal().c_str(), node->literal().size());
	str[node->literal().size()] = '\0';
	_lastVar = new GPVar(_c.newGP());
	_c.mov(static_cast<GPVar &>(*_lastVar), imm((sysint_t)str));	
	_lastType = VT_STRING;
}

 void AstToAsm::visitDoubleLiteralNode( mathvm::DoubleLiteralNode* node )
{
	/*_bytecode->addInsn(BC_DLOAD);
	_bytecode->addDouble(node->literal());*/
	double val = node->literal();
	GPVar temp(_c.newGP());
	_c.mov(temp, imm(*(int64_t *)&val));
	XMMVar *xmm = new XMMVar(_c.newXMM());
	_c.movq(*xmm, temp);
	_lastVar = xmm;
	_lastType = VT_DOUBLE;
}

void AstToAsm::visitIntLiteralNode( mathvm::IntLiteralNode* node )
{
	/*_bytecode->addInsn(BC_ILOAD);
	_bytecode->addInt64(node->literal());*/
	_lastVar = new GPVar(_c.newGP());
	_c.mov(static_cast<GPVar &>(*_lastVar), imm(node->literal()));	
	_lastType =mathvm::VT_INT;
}

void AstToAsm::visitLoadNode( mathvm::LoadNode* node )
{
	//switch (node->var()->type()) {
	//	case mathvm::VT_INT : _bytecode->addInsn(BC_LOADIVAR); break;
	//	case VT_DOUBLE : _bytecode->addInsn(BC_LOADDVAR); break;
	//	case VT_STRING : _bytecode->addInsn(BC_LOADSVAR); break;
	//	default: throwException("Unknown type of variable " + node->var()->name());
	//}
	//insertVarId(node->var()->name());
	//_lastType = node->var()->type();

	switch (node->var()->type()) {
		case VT_STRING : 	
		case mathvm::VT_INT : {
				GPVar *result = new GPVar(_c.newGP());
				_c.mov(*result, qword_ptr(*_frame, getVarOffset(node->var()->name()) * sizeof (int64_t)));				
				_lastVar = result;
				break;
			}
		case VT_DOUBLE : {
				XMMVar *result = new XMMVar(_c.newXMM());				
				_c.movq(*result, qword_ptr(*_frame, getVarOffset(node->var()->name()) * sizeof (int64_t)));
				_lastVar = result;
				break;
			}		
		default: throwException("Unknown type of variable " + node->var()->name());
	}
	//insertVarId(node->var()->name());
	_lastType = node->var()->type();
}

void AstToAsm::visitStoreNode( mathvm::StoreNode* node )
{
	if (node->op() == tASSIGN) {			
		node->value()->visit(this);	
		if (_lastType != node->var()->type()) {
			if (_lastType == VT_DOUBLE && node->var()->type() == mathvm::VT_INT) {
				GPVar *newVar = insertD2I(static_cast<XMMVar*>(_lastVar));
				_c.unuse(*_lastVar);
				delete _lastVar;
				_lastVar = newVar;
				_lastType = mathvm::VT_INT;				
			}
			else if (_lastType == mathvm::VT_INT && node->var()->type() == VT_DOUBLE) {
				XMMVar *newVar = insertI2D(static_cast<GPVar*>(_lastVar));
				_c.unuse(*_lastVar);
				delete _lastVar;
				_lastVar = newVar;
				_lastType = VT_DOUBLE;				
			}
			else {
				throwException("Incompatible types in assigning");
			}
		}
	}
	uint16_t varOffset = getVarOffset(node->var()->name());
	TokenKind binaryOp = tUNDEF;	
	switch (node->op()) {
		case tASSIGN : break;
		case tINCRSET : binaryOp = tADD; break;
		case tDECRSET : binaryOp = tSUB; break;
		default : throwException("Invalid assignment operation");
	}
		
	if (node->op() == tINCRSET || node->op() == tDECRSET) {
		LoadNode varNode(0, node->var());
		BinaryOpNode binaryOpNode(0, binaryOp, &varNode, node->value());
		binaryOpNode.visit(this);
	}

	//switch (node->var()->type()) {
	//	case mathvm::VT_INT : _bytecode->addInsn(BC_STOREIVAR); break;
	//	case VT_DOUBLE : _bytecode->addInsn(BC_STOREDVAR); break;
	//	case VT_STRING : _bytecode->addInsn(BC_STORESVAR); break;
	//	default : throwException("Invalid variable type");
	//}
	//insertVarId(node->var()->name());
	//_lastType = VT_INVALID;
	//node->value()->visit(this);
	switch (node->var()->type()) {
		case VT_STRING :
		case mathvm::VT_INT : {
			_c.mov(qword_ptr(*_frame, getVarOffset(node->var()->name()) * sizeof (int64_t)), (GPVar &)*_lastVar);			
			break;
		}
		case VT_DOUBLE : {
			_c.movq(qword_ptr(*_frame, getVarOffset(node->var()->name()) * sizeof (int64_t)), (XMMVar &)*_lastVar);
			break;
		}
		//case VT_STRING : _bytecode->addInsn(BC_STORESVAR); break;
		default : throwException("Invalid variable type");
	}
	//insertVarId(node->var()->name());
	_lastType = VT_INVALID;
}

void AstToAsm::visitForNode( mathvm::ForNode* node )
{
	BinaryOpNode *inBinaryNode = node->inExpr()->asBinaryOpNode();
	if (inBinaryNode == NULL) {
		throwException("\"In\" expression is invalid");
	}
	inBinaryNode->left()->visit(this);
	checkCurrentType(mathvm::VT_INT);
	//_bytecode->addInsn(BC_STOREIVAR);
	uint16_t varOffset = getVarOffset(node->var()->name());
	_c.mov(qword_ptr(*_frame, varOffset * sizeof (uint64_t)), (GPVar &)*_lastVar);
	//insertVarId(node->var()->name());
	AsmJit::Label lStart(_c.newLabel()), lEnd(_c.newLabel());
	_c.bind(lStart);
	inBinaryNode->right()->visit(this);
	_c.cmp(qword_ptr(*_frame, varOffset * sizeof (uint64_t)), static_cast<GPVar &>(*_lastVar));
	_c.jg(lEnd);
	//_bytecode->addInsn(BC_LOADIVAR);
	//insertVarId(node->var()->name());
	//_bytecode->addBranch(BC_IFICMPG, lEnd);
	node->body()->visit(this);
	//IntLiteralNode unit(0, 1);
	//StoreNode incrNode(0, node->var(), &unit, tINCRSET);
	//incrNode.visit(this);
	_c.add(qword_ptr(*_frame, varOffset * sizeof (uint64_t)), imm(1));
	//_bytecode->addBranch(BC_JA, lStart);
	_c.jmp(lStart);
	_c.bind(lEnd);
}

void AstToAsm::visitWhileNode( mathvm::WhileNode* node )
{
	AsmJit::Label lCond(_c.newLabel()), lEnd(_c.newLabel());
	_c.bind(lCond);
	node->whileExpr()->visit(this);
	if (_lastType != mathvm::VT_INT) {
		throwException("\"While\" condition must have int type");
	}
	//_bytecode->addInsn(BC_ILOAD0);
	//_bytecode->addBranch(BC_IFICMPE, lEnd);
	_c.cmp(static_cast<GPVar &>(*_lastVar), imm(0));
	_c.je(lEnd);
	node->loopBlock()->visit(this);
	_c.jmp(lCond);
	//_bytecode->addBranch(BC_JA, lCond);
	_c.bind(lEnd);
}

void AstToAsm::visitIfNode( mathvm::IfNode* node )
{
	node->ifExpr()->visit(this);
	if (_lastType != mathvm::VT_INT) {
		throwException("\"If\" condition must have int type");
	}
	AsmJit::Label lEnd(_c.newLabel()), lElse(_c.newLabel());
	//_bytecode->addInsn(BC_ILOAD0);
	_c.cmp(static_cast<GPVar &>(*_lastVar), imm(0)); 
	_c.je(lElse);
	//_bytecode->addBranch(BC_IFICMPE, lElse);
	node->thenBlock()->visit(this);		
	if (node->elseBlock() != NULL) {
		//_bytecode->addBranch(BC_JA, lEnd);
		_c.jmp(lEnd);
		_c.bind(lElse);
		node->elseBlock()->visit(this);
	}
	else {
		_c.bind(lElse);
	}
	_c.bind(lEnd);
}

void AstToAsm::visitBlockNode( mathvm::BlockNode* node )
{
	//node->visitChildren(this);
	Scope::FunctionIterator fnIt1(node->scope());
	while (fnIt1.hasNext()) {
		/*if (_currentFreeFuncId == FUNC_LIMIT) {
			throwException("Functions limit was reached");
		}*/
		AstFunction *func = fnIt1.next();
		//FreeVarsFunction *fvf = new FreeVarsFunction(func);
		//_code->addFunction(fvf);

		VarsSearcherVisitor *vsv = new VarsSearcherVisitor;
		func->node()->visit(vsv);
		
		int64_t funcId = getFuncId(func->name());
		//setFuncRetType(funcId, func->returnType());
		setFuncNode(funcId, func);
		setFuncVarsVisitor(funcId, vsv);
		AstToAsm codeGenerator(func->parametersNumber());
		
		for (int i = 0; i < (int)vsv->freeVars().size(); ++i) {
			codeGenerator.pushVar(vsv->freeVars()[i]->name(), vsv->freeVars()[i]->type());
		}

		for (int i = 0; i < (int)func->parametersNumber(); ++i) {
			codeGenerator.pushVar(func->parameterName(i), func->parameterType(i));
		}
		
		//codeGenerator._bytecode = fvf->bytecode();	
		//func->node()->body()->visit(&codeGenerator);	
		CompiledFunc compiledFunc = codeGenerator.compile(func);
		linkFunc(funcId, compiledFunc);
	}
	Scope::VarIterator it1(node->scope());
	while (it1.hasNext()) {
		AstVar *var = it1.next();
		pushVar(var->name(), var->type());
		_localsCount++;
	}
	for (int i = 0; i < (int)node->nodes(); ++i) {
		AstNode *currNode = node->nodeAt(i);
		currNode->visit(this);
		/*if (currNode->isCallNode()) {
			_bytecode->addInsn(BC_POP);
		}*/
	}	
	Scope::VarIterator it2(node->scope());
	while (it2.hasNext()) {
		popVar(it2.next()->name());
		_localsCount--;
	}
}

void AstToAsm::visitPrintNode( mathvm::PrintNode* node )
{
	uint32_t count = node->operands();
	for (uint32_t i = 0; i < count; ++i) {
		node->operandAt(i)->visit(this);
		/*switch(_lastType) {
			case mathvm::VT_INT : _bytecode->addInsn(BC_IPRINT);  break;
			case VT_DOUBLE : _bytecode->addInsn(BC_DPRINT);  break;
			case VT_STRING : _bytecode->addInsn(BC_SPRINT);  break;
			default: ;
		}*/		

		switch(_lastType) {
			case mathvm::VT_INT : {
					ECall *ctx = _c.call(printInt);
					ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, int64_t>());
					ctx->setArgument(0, static_cast<GPVar &>(*_lastVar));
					break;
				}
			case VT_DOUBLE : {
					ECall *ctx = _c.call(printDouble);
					ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, double>());
					ctx->setArgument(0, static_cast<XMMVar &>(*_lastVar));
					break;
				}
			case VT_STRING : {
					ECall *ctx = _c.call(printStr);
					ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, char *>());
					ctx->setArgument(0, static_cast<GPVar &>(*_lastVar));
					break;
				}
			default: ;
		}
	}
}

void AstToAsm::insertData( const void *data, size_t size )
{
	//const uint8_t *pData = (const uint8_t *)data;
	//while (size--) {
	//	_bytecode->add(*pData++);
	//}
}

void AstToAsm::insertVarId( const std::string &name )
{
	//vector<VarInt> &v = _vars[name];
	//if (v.empty()) {
	//	throwException("Undefined variable " + name);
	//}
	//_bytecode->addUInt16(v.back());	
}

uint16_t AstToAsm::getVarId(const std::string &name) 
{
	vector<VarInt> &v = _vars[name];
	if (v.empty()) {
		throwException("Undefined variable " + name);
	}
	return v.back();
}

uint32_t AstToAsm::getVarOffset(const std::string &name) 
{
	return getVarId(name);
}

uint32_t AstToAsm::getParamOffset(int index) {
	return index + 1;
}

void AstToAsm::throwException( const std::string &what )
{
	throw Exception(what);
}

void AstToAsm::checkCurrentType(mathvm::VarType excpectedType)
{
	if (_lastType != excpectedType) {
		throwException(string("Type mismatch: expected") + typeToString(excpectedType) + ", but got " + typeToString(_lastType));
	}
}

std::string AstToAsm::typeToString( mathvm::VarType type )
{
	string result;
	switch(type) {
		case mathvm::VT_INT : result = "int"; break;
		case VT_DOUBLE : result = "double"; break;
		case VT_STRING : result = "string"; break;
		default : result = "unknown";
	}
	return result;
}

void AstToAsm::checkIfInsn( AsmJit::CONDITION cond )
{
	AsmJit::Label lTrue(_c.newLabel());
	AsmJit::Label lEnd(_c.newLabel());
	_c.j(cond, lTrue);
	GPVar *result = new GPVar(_c.newGP());
	_c.mov(*result, imm(0));
	_c.jmp(lEnd);
	_c.bind(lTrue);
	_c.mov(*result, imm(1));
	_c.bind(lEnd);	
	_lastVar = result;
}

void AstToAsm::visitCallNode( mathvm::CallNode* node )
{
	uint64_t funcId = getFuncId(node->name());
	VarsSearcherVisitor *vsv = getFuncVarsVisitor(funcId);

	GPVar oldFrame(_c.newGP(1U, "oldFrame"));
	GPVar newFrame(_c.newGP(1U, "newFrame"));
	_c.mov(oldFrame, *_frame);
	_c.mov(newFrame, *_frame);
	

	int freeVarsCount = vsv->freeVars().size();
	_c.add(newFrame, imm((1 + _paramsCount + _localsCount + freeVarsCount) * sizeof (int64_t)));
	for (int i = 0; i < freeVarsCount; ++i) { //copy free vars from our frame into new frame
		uint16_t freeVarId = getVarId(vsv->freeVars()[i]->name());
		switch(vsv->freeVars()[i]->type()) {
			case mathvm::VT_STRING :
			case mathvm::VT_INT : {
					GPVar temp(_c.newGP());
					_c.mov(temp, qword_ptr(oldFrame, freeVarId * sizeof(int64_t)));
					_c.mov(qword_ptr(newFrame, (i + 1) * sizeof(int64_t)), temp); 
					_c.unuse(temp);
					break;
				}
			case mathvm::VT_DOUBLE : {
					XMMVar temp(_c.newXMM());
					_c.movq(temp, qword_ptr(oldFrame, freeVarId * sizeof(int64_t)));
					_c.movq(qword_ptr(newFrame, (i + 1) * sizeof(int64_t)), temp); 
					_c.unuse(temp);
					break;
				}
			default : throwException("Unknown type of free var");
		}
	}

	vector<BaseVar *> params;
	for (int i = 0; i < node->parametersNumber(); ++i) { // calculate and load parameter into new frame
		node->parameterAt(i)->visit(this);
		if (_lastType != getFuncNode(funcId)->parameterType(i)) {
			convertLastVarTo(getFuncNode(funcId)->parameterType(i));
		}
		switch(_lastType) {
			case mathvm::VT_STRING :
			case mathvm::VT_INT : {
					GPVar *param = new GPVar(_c.newGP());
					_c.mov(*param, static_cast<GPVar &>(*_lastVar)); 
					params.push_back(param);
					break;
				}
			case mathvm::VT_DOUBLE : {
					XMMVar *param = new XMMVar(_c.newXMM());
					_c.movq(*param, static_cast<XMMVar &>(*_lastVar)); 
					params.push_back(param);
					break;
				}
			default : throwException("Unknown type of parameter");
		}
		
	}
	for (int i = 0; i < node->parametersNumber(); ++i) {
		switch(_lastType) {
			case mathvm::VT_STRING :
			case mathvm::VT_INT : _c.mov(qword_ptr(newFrame, (1 + freeVarsCount + i) * sizeof(int64_t)), static_cast<GPVar &>(*params[i])); break;
			case mathvm::VT_DOUBLE : _c.movq(qword_ptr(newFrame, (1 + freeVarsCount + i) * sizeof(int64_t)), static_cast<XMMVar &>(*params[i])); break;
			default : throwException("Unknown type of parameter");
		}
	}
	//_c.mov(oldFrame, *_frame);
	//_c.add(*_frame, imm((1 + _paramsCount + _localsCount) * sizeof (int64_t)));
	_c.mov(*_frame, newFrame);
	
	GPVar frameAddr(_c.newGP(1U, "frameAddr"));
	_c.mov(frameAddr, imm((sysint_t)&_framePtr));
	_c.mov(qword_ptr(frameAddr, 0), *_frame);
	
	
	GPVar funcIdVar(_c.newGP(1U, "funcIdVar"));
	_c.mov(funcIdVar, imm(funcId));
	ECall *ctx = _c.call(callProxy);
	ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, CompiledFunc>());
	ctx->setArgument(0, funcIdVar);	

	for (int i = 0; i < freeVarsCount; ++i) { // copy free vars from new frame into our frame back
		uint16_t freeVarId = getVarId(vsv->freeVars()[i]->name());
		switch(vsv->freeVars()[i]->type()) {
			case mathvm::VT_STRING :
			case mathvm::VT_INT : {
					GPVar temp(_c.newGP());
					_c.mov(temp, qword_ptr(newFrame, (i + 1) * sizeof(int64_t)));
					_c.mov(qword_ptr(oldFrame, freeVarId * sizeof(int64_t)), temp); 
					_c.unuse(temp);
					break;
				}
			case mathvm::VT_DOUBLE : {
					XMMVar temp(_c.newXMM());
					_c.movq(temp, qword_ptr(newFrame, (i + 1) * sizeof(int64_t)));
					_c.movq(qword_ptr(oldFrame, freeVarId * sizeof(int64_t)), temp); 
					_c.unuse(temp);
					break;
				}
			default : throwException("Unknown type of free var");
		}
	}

	switch(getFuncNode(funcId)->returnType()) { // take return value
		case VT_STRING :
		case mathvm::VT_INT : {
				GPVar *result = new GPVar(_c.newGP());
				_c.mov(static_cast<GPVar &>(*result), qword_ptr(*_frame, 0));				
				_lastVar = result;
			} break;
		case VT_DOUBLE : {
				XMMVar *result = new XMMVar(_c.newXMM());
				_c.movq(static_cast<XMMVar &>(*result), qword_ptr(*_frame, 0));
				_lastVar = result;
			} break;
		case mathvm::VT_VOID : break;
		default : throwException("Unknown return type of function: " + node->name());		
	}
	_lastType = getFuncNode(funcId)->returnType();
	_c.mov(*_frame, oldFrame);
	_c.mov(qword_ptr(frameAddr, 0), *_frame);
}

void AstToAsm::visitNativeCallNode( mathvm::NativeCallNode *node) {
	node->nativeSignature();
}

void AstToAsm::visitReturnNode( mathvm::ReturnNode* node )
{
	if (node->returnExpr()) {
		node->returnExpr()->visit(this);
		//_bytecode->addInsn(BC_STOREIVAR);
		//_bytecode->addUInt16(0);
		if (_rootFunc->returnType() != _lastType) {
			convertLastVarTo(_rootFunc->returnType());
		}
		switch(_lastType) {
			case VT_STRING :
			case mathvm::VT_INT : _c.mov(qword_ptr(*_frame, 0), static_cast<GPVar &>(*_lastVar)); break;
			case VT_DOUBLE : _c.movq(qword_ptr(*_frame, 0), static_cast<XMMVar &>(*_lastVar)); break;
			default : throwException("Invalid type in return node");
		}
	}
	//_bytecode->addInsn(BC_RETURN);
	_c.ret();
}

void AstToAsm::visitFunctionNode( mathvm::FunctionNode* node )
{		
}

void AstToAsm::pushVar( const std::string &name, const mathvm::VarType type )
{
	if (_currentFreeVarId == VARS_LIMIT) {
		throwException("Variables limit was reached");
	}
	_vars[name].push_back(_currentFreeVarId++);		
}

void AstToAsm::popVar( const std::string &name )
{
	_vars[name].pop_back();
	_currentFreeVarId--;
}

AsmJit::XMMVar * AstToAsm::insertI2D(AsmJit::GPVar *src) 
{
	XMMVar *result = new XMMVar(_c.newXMM());
	_c.cvtsi2sd(*result, *src);								
	return result;
}

AsmJit::GPVar * AstToAsm::insertD2I(AsmJit::XMMVar *src) 
{
	GPVar *result = new GPVar(_c.newGP());
	_c.cvtsd2si(*result, *src);								
	return result;
}

void AstToAsm::convertLastVarTo(mathvm::VarType type) 
{
	if (_lastType == mathvm::VT_INT && type == VT_DOUBLE) {
		XMMVar *var = insertI2D(static_cast<GPVar *>(_lastVar));
		_c.unuse(*_lastVar);
		delete _lastVar;
		_lastVar = var;
		_lastType = VT_DOUBLE;
	} 
	else if (_lastType == VT_DOUBLE && type == mathvm::VT_INT) {
		GPVar *var = insertD2I(static_cast<XMMVar *>(_lastVar));
		_c.unuse(*_lastVar);
		delete _lastVar;
		_lastVar = var;
		_lastType = mathvm::VT_INT;
	}
	else {
		throwException("Incompatible types in conversion");
	}
}
	
