#include "bytecoder.h"
#include <iostream>
#include <iomanip>
#include <dlfcn.h>

using namespace mathvm;
using std::cout;
using std::endl;
using std::string;

// auxiliary functions
void Bytecoder::placeVar(std::string const& name) {
	bytecode_->addInt16(varMap_.getData(name));
}

void Bytecoder::placeFunc(std::string const& name) {
	bytecode_->addInt16(funcMap_.getData(name));
}

// for handling results after BC_DCMP and BC_ICMP
void Bytecoder::cmpHandler(mathvm::TokenKind tokenKind) {
	Instruction insn = BC_IFICMPE;
	if (tokenKind == tNEQ || tokenKind == tLE || tokenKind == tGE)
		insn = BC_IFICMPNE;

	switch (tokenKind) {
		case tEQ  : bytecode_->addInsn(BC_ILOAD0); break;
		case tNEQ : bytecode_->addInsn(BC_ILOAD0); break;
		case tGT  : bytecode_->addInsn(BC_ILOADM1); break;
		case tGE  : bytecode_->addInsn(BC_ILOAD1); break;
		case tLT  : bytecode_->addInsn(BC_ILOAD1); break;
		case tLE  : bytecode_->addInsn(BC_ILOADM1); break;
		default :
			assert(false);
			break;
	}

	Label lTrue(bytecode_), lEnd(bytecode_);
	bytecode_->addBranch(insn, lTrue);
	bytecode_->addInsn(BC_ILOAD0);
	bytecode_->addBranch(BC_JA, lEnd);
	bytecode_->bind(lTrue);
	bytecode_->addInsn(BC_ILOAD1);
	bytecode_->bind(lEnd);
}

void Bytecoder::convertTypes(VarType from, VarType to) {
	if (from != to) {
		// we can convert only DOUBLE and INT
		assert(to   == VT_DOUBLE || to   == VT_INT);
		assert(from == VT_DOUBLE || from == VT_INT);

		if (from == VT_DOUBLE)
			bytecode_->addInsn(BC_D2I);
		else
			bytecode_->addInsn(BC_I2D);
	}
	topType_ = to;
}

void Bytecoder::convertTypes(VarType from_upper, VarType from_lower, VarType to) {
	if (from_lower != to) {
		bytecode_->addInsn(BC_SWAP);
		convertTypes(from_lower, to);
		bytecode_->addInsn(BC_SWAP);
	}
	if (from_upper != to) {
		convertTypes(from_upper, to);
	}
	topType_ = to;
}


// general functions
void Bytecoder::visitBinaryOpNode( mathvm::BinaryOpNode* node )
{
	if (node->kind() == tOR) {
		Label lTrueEnd(bytecode_), lEnd(bytecode_);
		node->left()->visit(this);
		assert(topType_ == VT_INT);
		bytecode_->addInsn(BC_ILOAD0);
		bytecode_->addBranch(BC_IFICMPNE, lTrueEnd);
		node->right()->visit(this);
		assert(topType_ == VT_INT);
		bytecode_->addBranch(BC_JA, lEnd);
		bytecode_->bind(lTrueEnd);
		bytecode_->addInsn(BC_ILOAD1);
		bytecode_->bind(lEnd);
		topType_ = VT_INT;
	}
	else if (node->kind() == tAND) {
		Label lFalseEnd(bytecode_), lEnd(bytecode_);
		node->left()->visit(this);
		assert(topType_ == VT_INT);
		bytecode_->addInsn(BC_ILOAD0);
		bytecode_->addBranch(BC_IFICMPE, lFalseEnd);
		node->right()->visit(this);
		assert(topType_ == VT_INT);
		bytecode_->addBranch(BC_JA, lEnd);
		bytecode_->bind(lFalseEnd);
		bytecode_->addInsn(BC_ILOAD0);
		bytecode_->bind(lEnd);
		topType_ = VT_INT;
	}
	else {
		node->right()->visit(this);
		VarType rightType = topType_;
		node->left()->visit(this);
		VarType leftType = topType_;
		assert(rightType == VT_INT || rightType == VT_DOUBLE);
		assert(leftType ==  VT_INT || leftType == VT_DOUBLE);
		VarType resultType = VT_INT;
		if (leftType == VT_DOUBLE || rightType == VT_DOUBLE) {
			resultType = VT_DOUBLE;
			convertTypes(leftType, rightType, resultType);
		}
		if (resultType == VT_DOUBLE) {
			switch(node->kind()) {
				case tADD : bytecode_->addInsn(BC_DADD); break;
				case tSUB : bytecode_->addInsn(BC_DSUB); break;
				case tMUL : bytecode_->addInsn(BC_DMUL); break;
				case tDIV : bytecode_->addInsn(BC_DDIV); break;
				case tEQ  :
				case tNEQ :
				case tGT  :
				case tLT  :
				case tGE  :
				case tLE  :
					bytecode_->addInsn(BC_DCMP);
					topType_ = VT_INT;
					cmpHandler(node->kind());
					break;
				default :
					assert(false);
					break;
			}
		}
		else { // VT_INT
			switch(node->kind()) {
				case tADD : bytecode_->addInsn(BC_IADD); break;
				case tSUB : bytecode_->addInsn(BC_ISUB); break;
				case tMUL : bytecode_->addInsn(BC_IMUL); break;
				case tDIV : bytecode_->addInsn(BC_IDIV); break;
				case tEQ  :
				case tNEQ :
				case tGT  :
				case tLT  :
				case tGE  :
				case tLE  :
					bytecode_->addInsn(BC_ICMP);
					cmpHandler(node->kind());
					break;
				default :
					assert(false);
					break;
			}
		}
	}
}

void Bytecoder::visitUnaryOpNode( mathvm::UnaryOpNode* node )
{
	node->visitChildren(this);
	if (node->kind() == tSUB) {
		switch(topType_) {
			case VT_INT : 	 bytecode_->addInsn(BC_INEG); break;
			case VT_DOUBLE : bytecode_->addInsn(BC_DNEG); break;
			default:
				// can't apply unary "-" not to INT and DOUBLE
				assert(false);
				break;
		}
	}
	else { // tNOT
		assert(topType_ == VT_INT);

		// we should change 0 to 1 and "not 0" to 0
		bytecode_->add(BC_ILOAD0);
		Label lTrue(bytecode_), lEnd(bytecode_);
		bytecode_->addBranch(BC_IFICMPE, lTrue);
		bytecode_->addInsn(BC_ILOAD0);
		bytecode_->addBranch(BC_JA, lEnd);
		bytecode_->bind(lTrue);
		bytecode_->addInsn(BC_ILOAD1);
		bytecode_->bind(lEnd);
	}
}

void Bytecoder::visitStringLiteralNode( mathvm::StringLiteralNode* node )
{
	bytecode_->addInsn(BC_SLOAD);
	uint16_t strConstantId = code_->makeStringConstant(node->literal());
	bytecode_->addUInt16(strConstantId);
	topType_ = VT_STRING;
}

void Bytecoder::visitDoubleLiteralNode( mathvm::DoubleLiteralNode* node )
{
	bytecode_->addInsn(BC_DLOAD);
	bytecode_->addDouble(node->literal());
	topType_ = VT_DOUBLE;
}

void Bytecoder::visitIntLiteralNode( mathvm::IntLiteralNode* node )
{
	bytecode_->addInsn(BC_ILOAD);
	bytecode_->addInt64(node->literal());
	topType_ = VT_INT;
}

void Bytecoder::visitLoadNode( mathvm::LoadNode* node )
{
	switch (node->var()->type()) {
		case VT_INT :
			bytecode_->addInsn(BC_LOADIVAR);
			break;
		case VT_DOUBLE :
			bytecode_->addInsn(BC_LOADDVAR);
			break;
		case VT_STRING :
			bytecode_->addInsn(BC_LOADSVAR);
			break;
		default:
			assert(false);
			break;
	}
	placeVar(node->var()->name());
	topType_ = node->var()->type();
}

void Bytecoder::visitStoreNode( mathvm::StoreNode* node )
{
	node->value()->visit(this);
	// make type conversion if needed
	convertTypes(topType_, node->var()->type());

	if (node->op() == tINCRSET || node->op() == tDECRSET) {
		if (node->var()->type() == VT_INT) {
			bytecode_->addInsn(BC_LOADIVAR);
			placeVar(node->var()->name());
			if (node->op() == tINCRSET)
				bytecode_->addInsn(BC_IADD);
			else
				bytecode_->addInsn(BC_ISUB);
		}
		else if (node->var()->type() == VT_DOUBLE) {
			bytecode_->addInsn(BC_LOADDVAR);
			placeVar(node->var()->name());
			if (node->op() == tINCRSET)
				bytecode_->addInsn(BC_DADD);
			else
				bytecode_->addInsn(BC_DSUB);
		}
		else {
			// wrong type to Decrement/Increment
			assert(false);
		}
	}

	switch (node->var()->type()) {
		case VT_INT :
			bytecode_->addInsn(BC_STOREIVAR);
			break;
		case VT_DOUBLE :
			bytecode_->addInsn(BC_STOREDVAR);
			break;
		case VT_STRING :
			bytecode_->addInsn(BC_STORESVAR);
			break;
		default :
			assert(false);
			break;
	}
	placeVar(node->var()->name());
	topType_ = VT_INVALID;
}

void Bytecoder::visitForNode( mathvm::ForNode* node )
{
	BinaryOpNode* inExpr = node->inExpr()->asBinaryOpNode();
	assert(inExpr != NULL);
	// init
	inExpr->left()->visit(this);
	bytecode_->addInsn(BC_STOREIVAR);
	placeVar(node->var()->name());

	Label lCheckCondition(bytecode_), lEnd(bytecode_);
	// exit condition
	bytecode_->bind(lCheckCondition);
	inExpr->right()->visit(this);
	bytecode_->addInsn(BC_LOADIVAR);
	placeVar(node->var()->name());
	bytecode_->addBranch(BC_IFICMPG, lEnd);
	// body
	node->body()->visit(this);
	// increment
	bytecode_->addInsn(BC_ILOAD1);
	bytecode_->addInsn(BC_LOADIVAR);
	placeVar(node->var()->name());
	bytecode_->addInsn(BC_IADD);
	bytecode_->addInsn(BC_STOREIVAR);
	placeVar(node->var()->name());
	// back to condition
	bytecode_->addBranch(BC_JA, lCheckCondition);
	bytecode_->bind(lEnd);
}

void Bytecoder::visitWhileNode( mathvm::WhileNode* node )
{
	Label lCheckCondition(bytecode_), lEnd(bytecode_);
	// exit condition
	bytecode_->bind(lCheckCondition);
	node->whileExpr()->visit(this);
	assert(topType_ == VT_INT);
	bytecode_->addInsn(BC_ILOAD0);
	bytecode_->addBranch(BC_IFICMPE, lEnd);
	// body
	node->loopBlock()->visit(this);
	// back to condition
	bytecode_->addBranch(BC_JA, lCheckCondition);
	bytecode_->bind(lEnd);
}

void Bytecoder::visitIfNode( mathvm::IfNode* node )
{
	Label lElse(bytecode_), lEnd(bytecode_);
	// condition
	node->ifExpr()->visit(this);
	assert(topType_ == VT_INT);
	bytecode_->addInsn(BC_ILOAD0);
	bytecode_->addBranch(BC_IFICMPE, lElse);
	// then
	node->thenBlock()->visit(this);
	// else
	if (node->elseBlock() != NULL) {
		bytecode_->addBranch(BC_JA, lEnd);
		bytecode_->bind(lElse);
		node->elseBlock()->visit(this);
	}
	else {
		bytecode_->bind(lElse);
	}
	bytecode_->bind(lEnd);
}

void Bytecoder::visitBlockNode( mathvm::BlockNode* node )
{
	varMap_.pushScope();
	funcMap_.pushScope();

	Scope::VarIterator varIt(node->scope());
	while(varIt.hasNext()) {
		AstVar *var = varIt.next();
		varMap_.addDataToScope(var->name());
	}

	Scope::FunctionIterator funcIt(node->scope());
	while (funcIt.hasNext()) {
		AstFunction* astFunc = funcIt.next();
		BytecodeFunction* bytecodeFunc = new BytecodeFunction(astFunc);
		funcMap_.addDataToScope(astFunc->name(), code_->addFunction(bytecodeFunc));

		Bytecode* main_bytecode_   = bytecode_;
		VarType   main_topType_    = topType_;
		VarType   main_returnType_ = returnType_;
		bytecode_ = bytecodeFunc->bytecode();
		varMap_.pushScope();
		funcMap_.pushScope();
		returnType_ = astFunc->returnType();

		// parameters are on TOS in reverse order
		for (uint32_t i = astFunc->parametersNumber(); i > 0; --i) {
			varMap_.addDataToScope(astFunc->parameterName(i - 1));
			switch (astFunc->parameterType(i - 1)) {
				case VT_INT    : bytecode_->addInsn(BC_STOREIVAR); break;
				case VT_DOUBLE : bytecode_->addInsn(BC_STOREDVAR); break;
				case VT_STRING : bytecode_->addInsn(BC_STORESVAR); break;
				default :
					// wrong type of parameter
					assert(false);
					break;
			}
			placeVar(astFunc->parameterName(i - 1));
		}
		astFunc->node()->visit(this);

		varMap_.popScope();
		funcMap_.popScope();
		bytecode_   = main_bytecode_;
		topType_    = main_topType_;
		returnType_ = main_returnType_;
	}

	node->visitChildren(this);

	varMap_.popScope();
	funcMap_.popScope();
}

void Bytecoder::visitFunctionNode( mathvm::FunctionNode* node )
{
	node->visitChildren(this);
}

void Bytecoder::visitNativeCallNode( mathvm::NativeCallNode* node )
{
	bytecode_->addInsn(BC_CALLNATIVE);
	uint16_t nativeFunctionId = code_->makeNativeFunction(node->nativeName(), node->nativeSignature(), 0);
	bytecode_->addUInt16(nativeFunctionId);
}

void Bytecoder::visitReturnNode( mathvm::ReturnNode* node )
{
	if (node->returnExpr()) {
		node->returnExpr()->visit(this);
		if (topType_ != returnType_) {
			assert(topType_    == VT_DOUBLE || topType_ == VT_INT);
			assert(returnType_ == VT_DOUBLE || returnType_ == VT_INT);
			convertTypes(topType_, returnType_);
		}
	}
	bytecode_->addInsn(BC_RETURN);
}

void Bytecoder::visitPrintNode( mathvm::PrintNode* node )
{
	for (uint32_t i = 0; i < node->operands(); ++i) {
		node->operandAt(i)->visit(this);
		switch(topType_) {
			case VT_INT    : bytecode_->addInsn(BC_IPRINT);  break;
			case VT_DOUBLE : bytecode_->addInsn(BC_DPRINT);  break;
			case VT_STRING : bytecode_->addInsn(BC_SPRINT);  break;
			default:
				assert(false);
				break;
		}
	}
}

void Bytecoder::visitCallNode( mathvm::CallNode* node )
{
	BytecodeFunction* bytecodeFunc = (BytecodeFunction*)code_->functionById(funcMap_.getData(node->name()));
	assert(bytecodeFunc != 0);

	// storing parameters on TOS
	node->visitChildren(this); // load parameters

	bytecode_->addInsn(BC_CALL);
	placeFunc(node->name());
	// return type
	topType_ = bytecodeFunc->signature()[0].first;
}


