#include "AstToBytecode.h"
#include <exception>

using namespace std;
using namespace mathvm;

void AstToBytecode::visitBinaryOpNode( mathvm::BinaryOpNode* node )
{
	TokenKind opKind = node->kind();
	if (opKind == tOR || opKind == tAND) {
		checkTypeInt(node);
		switch(opKind) {
			case tAND : {
				Label lAndFalse(&_bytecode);
				Label lAndEnd(&_bytecode);
				_bytecode.add(BC_ILOAD0);
				_bytecode.addBranch(BC_IFICMPE, lAndFalse);
				node->right()->visit(this);
				_bytecode.addBranch(BC_JA, lAndEnd);
				_bytecode.bind(lAndFalse);
				_bytecode.add(BC_ILOAD0);
				_bytecode.bind(lAndEnd); }
				break;
			case tOR: {
				Label lOrTrue(&_bytecode);
				Label lOrEnd(&_bytecode);
				_bytecode.add(BC_ILOAD1);
				_bytecode.addBranch(BC_IFICMPE, lOrTrue);
				node->right()->visit(this);
				_bytecode.addBranch(BC_JA, lOrEnd);
				_bytecode.bind(lOrTrue);
				_bytecode.add(BC_ILOAD1);
				_bytecode.bind(lOrEnd); }
				break;
			default: 
				throwException("Invalid logic operation");
		}
		return;
	}
	node->left()->visit(this);
	VarType leftType = _lastType;
	_lastType = VT_INVALID;
	node->right()->visit(this);
	VarType rightType = _lastType;
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
			if (leftType != VT_INT && leftType != VT_DOUBLE) {
				throwException("Invalid type of left node");
			}
			if (rightType != VT_INT && rightType != VT_DOUBLE) {
				throwException("Invalid type of right node");
			}
			if (leftType != rightType) {				
				if (rightType == VT_INT) {
					_bytecode.addInsn(BC_I2D);
					rightType = VT_DOUBLE;
				}
				else {
					_bytecode.addInsn(BC_SWAP);
					_bytecode.addInsn(BC_I2D);
					leftType = VT_DOUBLE;
					_bytecode.addInsn(BC_SWAP);
				}
				_lastType = VT_DOUBLE;
			} break;
		default: throwException("Invalid binary operation");
	}
	if (_lastType == VT_DOUBLE) {
		switch(opKind) {
			case tEQ :
			case tNEQ :
			case tGT :
			case tLT :
			case tGE :
			case tLE :
				_bytecode.addInsn(BC_DCMP); break;
			default: ;
		}
		switch(opKind) {
			case tNEQ : break;
			case tEQ : _bytecode.addInsn(BC_ILOAD0); checkIfInsn(BC_IFICMPE); break;
				// check operand order. change left-right order in stack
			case tGT : _bytecode.addInsn(BC_ILOADM1); checkIfInsn(BC_IFICMPE); break;
			case tLT : _bytecode.addInsn(BC_ILOAD1); checkIfInsn(BC_IFICMPE); break;
			case tGE : _bytecode.addInsn(BC_ILOAD1); checkIfInsn(BC_IFICMPNE); break;
			case tLE : _bytecode.addInsn(BC_ILOADM1); checkIfInsn(BC_IFICMPNE); break;

		}

	}
	
}

void AstToBytecode::visitUnaryOpNode( mathvm::UnaryOpNode* node )
{
	node->operand()->visit(this);
	switch(node->kind()) {
		case tSUB :
			switch(_lastType) {
				case VT_INT : _bytecode.addInsn(BC_INEG); break;
				case VT_DOUBLE : _bytecode.addInsn(BC_DNEG); break;
				default:throwException(string("Type mismatch: expected double or int, but ") + typeToString(_lastType) + " got");
			} break;
		case tNOT : 
			checkTypeInt(node);
			_bytecode.add(BC_ILOAD0);
			checkIfInsn(BC_IFICMPE);
			break;
		default : 
			throwException("Invalid unary operation");
	}
}

void AstToBytecode::visitStringLiteralNode( mathvm::StringLiteralNode* node )
{
	_bytecode.addInsn(BC_SLOAD);
	uint16_t strConstantId = _code.makeStringConstant(node->literal());
	_bytecode.addInt16(strConstantId);
}

void AstToBytecode::visitDoubleLiteralNode( mathvm::DoubleLiteralNode* node )
{
	_bytecode.addInsn(BC_DLOAD);
	_bytecode.addDouble(node->literal());
}

void AstToBytecode::visitIntLiteralNode( mathvm::IntLiteralNode* node )
{
	_bytecode.addInsn(BC_ILOAD);
	_bytecode.addInt64(node->literal());
} 

void AstToBytecode::visitLoadNode( mathvm::LoadNode* node )
{

}

void AstToBytecode::visitStoreNode( mathvm::StoreNode* node )
{

}

void AstToBytecode::visitForNode( mathvm::ForNode* node )
{

}

void AstToBytecode::visitWhileNode( mathvm::WhileNode* node )
{

}

void AstToBytecode::visitIfNode( mathvm::IfNode* node )
{

}

void AstToBytecode::visitBlockNode( mathvm::BlockNode* node )
{
	Scope::VarIterator it(node->scope());
	while (it.hasNext()) {
		
	}
}

void AstToBytecode::visitFunctionNode( mathvm::FunctionNode* node )
{

}

void AstToBytecode::visitPrintNode( mathvm::PrintNode* node )
{
	uint32_t count = node->operands();
	for (uint32_t i = 0; i < count; ++i) {
		node->operandAt(i)->visit(this);
		switch(_lastType) {
			case VT_INT : _bytecode.addInsn(BC_IPRINT);  break;
			case VT_DOUBLE : _bytecode.addInsn(BC_DPRINT);  break;
			case VT_STRING : _bytecode.addInsn(BC_SPRINT);  break;
			default: ;
		}
	}
}

void AstToBytecode::insertData( const void *data, size_t size )
{
	const uint8_t *pData = (const uint8_t *)data;
	while (size--) {
		_bytecode.add(*pData++);
	}
}

void AstToBytecode::insertVar( const std::string &name )
{
	vector<VarInt> &v = _vars[name];
	if (v.empty()) {
		throwException("Undefined variable " + name);
	}
	insertData(&v.back(), sizeof VarInt);
}

void AstToBytecode::throwException( const std::string &what )
{
	throw std::exception(what.c_str());
}

void AstToBytecode::checkTypeInt( mathvm::AstNode *node )
{
	node->visit(this);
	if (_lastType != VT_INT) {
		throwException(string("Type mismatch: expected int, but got ") + typeToString(_lastType));
	}
	_lastType = VT_INVALID;
}

std::string AstToBytecode::typeToString( mathvm::VarType type )
{
	string result;
	switch(type) {
		case VT_INT : result = "int"; break;
		case VT_DOUBLE : result = "double"; break;
		case VT_STRING : result = "string"; break;
		default : result = "unknown";
	}
	return result;
}

void AstToBytecode::checkIfInsn( mathvm::Instruction insn )
{
	Label lTrue(&_bytecode);
	Label lEnd(&_bytecode);
	_bytecode.addBranch(insn, lTrue);
	_bytecode.add(BC_ILOAD0);
	_bytecode.addBranch(BC_JA, lEnd);
	_bytecode.bind(lTrue);
	_bytecode.add(BC_ILOAD1);
	_bytecode.bind(lEnd);
}
