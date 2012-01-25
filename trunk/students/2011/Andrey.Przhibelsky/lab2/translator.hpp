#ifndef _TRANSLATOR_HPP_
#define _TRANSLATOR_HPP_

#include <map>

#include "ast.h"

using namespace mathvm;

class TranslatorException {

private:
	std::string msg;


public:
	TranslatorException(const std::string& m): msg(m) {
	}

	TranslatorException(const std::string& m1, const std::string& m2) {
		msg = m1 + m2;
	}

	TranslatorException(const std::string& m1, const std::string& m2, const std::string& m3) {
		msg = m1 + m2 + m3;
	}

	std::string what() {
		return msg;
	}
};	



struct VarDescriptor {

	VarDescriptor(uint16_t ctxId, uint16_t vId): contextId(ctxId), varId(vId) {
	}

	VarDescriptor(): contextId(-1), varId(-1) {
	}

	uint16_t contextId;
	uint16_t varId;

};



class AstToBytecode : public AstVisitor {

private:
	Code * code;
	Bytecode * bytecode;

	VarType typeOfTOS;
	VarType returnType;

	std::vector <std::map <std::string, uint16_t> > scopeStack;
	std::vector <uint16_t> contextStack;
	std::map <uint16_t, uint16_t> contextVarIds;

	uint16_t contextId;

	VarDescriptor getVarDscriptor(const std::string& varName) {
		for (int i = scopeStack.size() - 1; i >= 0; --i) {
			std::map <std::string, uint16_t>::iterator varIt = scopeStack[i].find(varName);
	
			if (varIt != scopeStack[i].end()) {
				return VarDescriptor(contextStack[i], varIt->second);
			}
		}

		throw TranslatorException("Cannot find variable ", varName);
		return VarDescriptor();
	}


public:
	AstToBytecode(Code * c): code(c), bytecode(((BytecodeFunction *) (code->functionByName(AstFunction::top_name)))->bytecode()),
				typeOfTOS(VT_INVALID), returnType(VT_VOID), scopeStack(), contextStack(), contextVarIds() ,contextId(0) {
	}


	virtual void visitBinaryOpNode(mathvm::BinaryOpNode * node) {
		if (node->kind() == tOR) {
			mathvm::Label checkSecond(bytecode), finish(bytecode);

			node->left()->visit(this);
			if (typeOfTOS != VT_INT) {
				throw TranslatorException("Unexpected type of result of boolean expresion: ", typeToName(typeOfTOS));
			}

			bytecode->addInsn(BC_ILOAD0);
			bytecode->addBranch(BC_IFICMPE, checkSecond);

			bytecode->addInsn(BC_ILOAD1);
			bytecode->addBranch(BC_JA, finish);
			
			bytecode->bind(checkSecond);
			node->right()->visit(this);
			if (typeOfTOS != VT_INT) {
				throw TranslatorException("Unexpected type of result of boolean expresion: ", typeToName(typeOfTOS));
			}

			bytecode->bind(finish);

			typeOfTOS= VT_INT;
		}
		else if (node->kind() == tAND) {
			mathvm::Label checkSecond(bytecode), finish(bytecode);

			node->left()->visit(this);
			if (typeOfTOS != VT_INT) {
				throw TranslatorException("Unexpected type of result of boolean expresion: ", typeToName(typeOfTOS));
			}

			bytecode->addInsn(BC_ILOAD0);
			bytecode->addBranch(BC_IFICMPNE, checkSecond);

			bytecode->addInsn(BC_ILOAD0);
			bytecode->addBranch(BC_JA, finish);
			
			bytecode->bind(checkSecond);
			node->right()->visit(this);
			if (typeOfTOS != VT_INT) {
				throw TranslatorException("Unexpected type of result in boolean expresion: ", typeToName(typeOfTOS));
			}

			bytecode->bind(finish);

			typeOfTOS= VT_INT;
		}
		else {
			node->right()->visit(this);
			if (typeOfTOS != VT_INT && typeOfTOS != VT_DOUBLE) {
				throw TranslatorException("Unexpected type of result in arithmetic expresion: ", typeToName(typeOfTOS));
			}
			VarType rightType = typeOfTOS;

			node->left()->visit(this);
			if (typeOfTOS != VT_INT && typeOfTOS != VT_DOUBLE) {
				throw TranslatorException("Unexpected type of result in arithmetic expresion: ", typeToName(typeOfTOS));
			}
			VarType leftType = typeOfTOS;


			if (leftType == VT_INT && rightType == VT_DOUBLE) {
				bytecode->addInsn(BC_I2D);
				leftType = VT_DOUBLE;
				typeOfTOS = VT_DOUBLE;
			}
			else if (leftType == VT_DOUBLE && rightType == VT_INT) {
				bytecode->addInsn(BC_SWAP);				
				bytecode->addInsn(BC_I2D);
				bytecode->addInsn(BC_SWAP);
				rightType = VT_DOUBLE;
			}

			if (leftType == VT_DOUBLE) {
				switch(node->kind()) {
					case tADD: {
						bytecode->addInsn(BC_DADD); 
						break;
					}
					case tSUB: {
						bytecode->addInsn(BC_DSUB);
						break;
					}
					case tMUL: {
						bytecode->addInsn(BC_DMUL);
						break;
					}
					case tDIV: {
						bytecode->addInsn(BC_DDIV);
						break;
					}
					case tEQ: {
						mathvm::Label equal(bytecode), finish(bytecode);

						bytecode->addInsn(BC_DCMP);

						bytecode->addInsn(BC_ILOAD0);
						bytecode->addBranch(BC_IFICMPE, equal);
					
						bytecode->addInsn(BC_ILOAD0);
						bytecode->addBranch(BC_JA, finish);

						bytecode->bind(equal);
						bytecode->addInsn(BC_ILOAD1);

						bytecode->bind(finish);
						typeOfTOS = VT_INT;
						break;
					}	
					case tNEQ: {
						bytecode->addInsn(BC_DCMP);
						typeOfTOS = VT_INT;
						break;
					}
					case tGT: {
						mathvm::Label gt(bytecode), finish(bytecode);

						bytecode->addInsn(BC_DCMP);

						bytecode->addInsn(BC_ILOAD1);
						bytecode->addBranch(BC_IFICMPE, gt);
					
						bytecode->addInsn(BC_ILOAD0);
						bytecode->addBranch(BC_JA, finish);

						bytecode->bind(gt);
						bytecode->addInsn(BC_ILOAD1);

						bytecode->bind(finish);					
						typeOfTOS = VT_INT;
						break;
					}
					case tLT: {
						mathvm::Label lt(bytecode), finish(bytecode);

						bytecode->addInsn(BC_DCMP);

						bytecode->addInsn(BC_ILOADM1);
						bytecode->addBranch(BC_IFICMPE, lt);
					
						bytecode->addInsn(BC_ILOAD0);
						bytecode->addBranch(BC_JA, finish);

						bytecode->bind(lt);
						bytecode->addInsn(BC_ILOAD1);

						bytecode->bind(finish);					
						typeOfTOS = VT_INT;
						break;
					}
					case tGE: {
						mathvm::Label ge(bytecode), finish(bytecode);

						bytecode->addInsn(BC_DCMP);

						bytecode->addInsn(BC_ILOADM1);
						bytecode->addBranch(BC_IFICMPNE, ge);
					
						bytecode->addInsn(BC_ILOAD0);
						bytecode->addBranch(BC_JA, finish);

						bytecode->bind(ge);
						bytecode->addInsn(BC_ILOAD1);

						bytecode->bind(finish);					
						typeOfTOS = VT_INT;
						break;
					}
					case tLE: {
						mathvm::Label le(bytecode), finish(bytecode);

						bytecode->addInsn(BC_DCMP);

						bytecode->addInsn(BC_ILOAD1);
						bytecode->addBranch(BC_IFICMPNE, le);
					
						bytecode->addInsn(BC_ILOAD0);
						bytecode->addBranch(BC_JA, finish);

						bytecode->bind(le);
						bytecode->addInsn(BC_ILOAD1);

						bytecode->bind(finish);					
						typeOfTOS = VT_INT;
						break;
					}
					default: {
						throw TranslatorException("Unexpected binary operator.");
					}
				}
			}
			else {
				switch(node->kind()) {
					case tADD: {
						bytecode->addInsn(BC_IADD); 
						break;
					}
					case tSUB: {
						bytecode->addInsn(BC_ISUB);
						break;
					}
					case tMUL: {
						bytecode->addInsn(BC_IMUL);
						break;
					}
					case tDIV: {
						bytecode->addInsn(BC_IDIV);
						break;
					}
					case tEQ: {
						mathvm::Label equal(bytecode), finish(bytecode);

						bytecode->addInsn(BC_ICMP);

						bytecode->addInsn(BC_ILOAD0);
						bytecode->addBranch(BC_IFICMPE, equal);
					
						bytecode->addInsn(BC_ILOAD0);
						bytecode->addBranch(BC_JA, finish);

						bytecode->bind(equal);
						bytecode->addInsn(BC_ILOAD1);

						bytecode->bind(finish);
						break;
					}	
					case tNEQ: {
						bytecode->addInsn(BC_ICMP);
						typeOfTOS = VT_INT;
						break;
					}
					case tGT: {
						mathvm::Label gt(bytecode), finish(bytecode);

						bytecode->addInsn(BC_ICMP);

						bytecode->addInsn(BC_ILOAD1);
						bytecode->addBranch(BC_IFICMPE, gt);
					
						bytecode->addInsn(BC_ILOAD0);
						bytecode->addBranch(BC_JA, finish);

						bytecode->bind(gt);
						bytecode->addInsn(BC_ILOAD1);

						bytecode->bind(finish);					
						break;
					}
					case tLT: {
						mathvm::Label lt(bytecode), finish(bytecode);

						bytecode->addInsn(BC_ICMP);

						bytecode->addInsn(BC_ILOADM1);
						bytecode->addBranch(BC_IFICMPE, lt);
					
						bytecode->addInsn(BC_ILOAD0);
						bytecode->addBranch(BC_JA, finish);

						bytecode->bind(lt);
						bytecode->addInsn(BC_ILOAD1);

						bytecode->bind(finish);					
						break;
					}
					case tGE: {
						mathvm::Label ge(bytecode), finish(bytecode);

						bytecode->addInsn(BC_ICMP);

						bytecode->addInsn(BC_ILOADM1);
						bytecode->addBranch(BC_IFICMPNE, ge);
					
						bytecode->addInsn(BC_ILOAD0);
						bytecode->addBranch(BC_JA, finish);

						bytecode->bind(ge);
						bytecode->addInsn(BC_ILOAD1);

						bytecode->bind(finish);					
						break;
					}
					case tLE: {
						mathvm::Label le(bytecode), finish(bytecode);

						bytecode->addInsn(BC_ICMP);

						bytecode->addInsn(BC_ILOAD1);
						bytecode->addBranch(BC_IFICMPNE, le);
					
						bytecode->addInsn(BC_ILOAD0);
						bytecode->addBranch(BC_JA, finish);

						bytecode->bind(le);
						bytecode->addInsn(BC_ILOAD1);

						bytecode->bind(finish);					
						break;
					}
					default: {
						throw TranslatorException("Unexpected binary operator.");
					}
				}
			}
		}
	}

	
	virtual void visitUnaryOpNode(mathvm::UnaryOpNode * node) {
		node->visitChildren(this);

		if (node->kind() == tSUB) {
			if (typeOfTOS == VT_INT) {
				bytecode->addInsn(BC_INEG);
			} 
			else if (typeOfTOS == VT_DOUBLE) {
				bytecode->addInsn(BC_DNEG);
			} else {
				throw TranslatorException("Unexpected type for unary minus: ", typeToName(typeOfTOS));
			}
		}
		else {
			if (typeOfTOS == VT_INT) {
				mathvm::Label zero(bytecode), finish(bytecode);

				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_IFICMPE, zero);
			
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_JA, finish);

				bytecode->bind(zero);
				bytecode->addInsn(BC_ILOAD1);

				bytecode->bind(finish);
			} 
			else {
				throw TranslatorException("Unexpected type for NOT: ", typeToName(typeOfTOS));
			}
		}
	}

	
	virtual void visitStringLiteralNode(mathvm::StringLiteralNode * node) {
		bytecode->addInsn(BC_SLOAD);
		uint16_t newId = code->makeStringConstant(node->literal());
		bytecode->addUInt16(newId);
		typeOfTOS = VT_STRING;
	}


	virtual void visitIntLiteralNode(mathvm::IntLiteralNode * node) {
		bytecode->addInsn(BC_DLOAD);
		bytecode->addInt64(node->literal());
		typeOfTOS = VT_DOUBLE;
	}


	virtual void visitDoubleLiteralNode(mathvm::DoubleLiteralNode * node) {
		bytecode->addInsn(BC_DLOAD);
		bytecode->addDouble(node->literal());
		typeOfTOS = VT_DOUBLE;
	}


	virtual void visitLoadNode(mathvm::LoadNode * node) {
		VarType thisType = node->var()->type();

		if (thisType == VT_INT) {
			bytecode->addInsn(BC_LOADCTXIVAR);
		}
		else if (thisType == VT_DOUBLE) {
			bytecode->addInsn(BC_LOADCTXDVAR);
		}
		else if (thisType == VT_STRING) {
			bytecode->addInsn(BC_LOADCTXSVAR);
		}
		else {
			throw TranslatorException("Unexpected type for load: ", typeToName(thisType));
		}
		
		VarDescriptor varDescriptor = getVarDscriptor(node->var()->name());
		bytecode->addUInt16(varDescriptor.contextId);
		bytecode->addUInt16(varDescriptor.varId);
		typeOfTOS = thisType;
	}


	virtual void visitStoreNode(mathvm::StoreNode * node) {
		node->value()->visit(this);

		VarType varType = node->var()->type();
		if (varType == VT_INT && typeOfTOS == VT_DOUBLE) {
			bytecode->addInsn(BC_D2I);
		}
		else if (varType == VT_DOUBLE && typeOfTOS == VT_INT) {
			bytecode->addInsn(BC_I2D);
		}
		else if (varType == VT_INT && typeOfTOS == VT_STRING) {
			bytecode->addInsn(BC_S2I);
		}
		else if (varType != typeOfTOS) {
			throw TranslatorException("Cannot cast following types in assignment: ", typeToName(typeOfTOS), typeToName(varType));
		}

		VarDescriptor varDescriptor = getVarDscriptor(node->var()->name());

		if (node->op() == tINCRSET) {
			if (varType == VT_INT) {
				bytecode->addInsn(BC_LOADCTXIVAR);
				bytecode->addUInt16(varDescriptor.contextId);
				bytecode->addUInt16(varDescriptor.varId);

				bytecode->addInsn(BC_IADD);
			}
			else if (varType == VT_DOUBLE) {
				bytecode->addInsn(BC_LOADCTXDVAR);
				bytecode->addUInt16(varDescriptor.contextId);
				bytecode->addUInt16(varDescriptor.varId);

				bytecode->addInsn(BC_DADD);
			}
			else {
				throw TranslatorException("Ivalid type in increment: ", typeToName(varType));
			}
		} else if (node->op() == tDECRSET) {
			if (varType == VT_INT) {
				bytecode->addInsn(BC_LOADCTXIVAR);
				bytecode->addUInt16(varDescriptor.contextId);
				bytecode->addUInt16(varDescriptor.varId);

				bytecode->addInsn(BC_ISUB);
			}
			else if (varType == VT_DOUBLE) {
				bytecode->addInsn(BC_LOADCTXDVAR);
				bytecode->addUInt16(varDescriptor.contextId);
				bytecode->addUInt16(varDescriptor.varId);

				bytecode->addInsn(BC_DSUB);
			}
			else {
				throw TranslatorException("Ivalid type in decrement: ", typeToName(varType));
			}
		}


		if (varType == VT_INT) {
			bytecode->addInsn(BC_STORECTXIVAR);
		}
		else if (varType == VT_DOUBLE) {
			bytecode->addInsn(BC_STORECTXDVAR);
		}
		else if (varType == VT_STRING) {
			bytecode->addInsn(BC_STORECTXSVAR);
		}
		else {
			throw TranslatorException("Ivalid type in assignment: ", typeToName(varType));
		}

		bytecode->addUInt16(varDescriptor.contextId);
		bytecode->addUInt16(varDescriptor.varId);
		typeOfTOS = VT_INVALID;
	}


	virtual void visitBlockNode(mathvm::BlockNode * node) {
		scopeStack.push_back(std::map <std::string, uint16_t>());
		contextStack.push_back(contextId);
		
		Scope::VarIterator varIterator(node->scope());
		while (varIterator.hasNext()) {
			AstVar * astVar = varIterator.next();
			uint16_t& currentVarId = contextVarIds[contextId];
			scopeStack.back().insert(std::make_pair(astVar->name(), currentVarId));
			currentVarId++;
		}

		Scope::FunctionIterator functionIteator(node->scope());
		while (functionIteator.hasNext()) {
			AstFunction * astFunction= functionIteator.next();
			BytecodeFunction* bytecodeFuncioin = new BytecodeFunction(astFunction);
			code->addFunction(bytecodeFuncioin);
		}

		functionIteator = Scope::FunctionIterator(node->scope());
		while (functionIteator.hasNext()) {
			AstFunction * astFunction= functionIteator.next();
			BytecodeFunction * bytecodeFunction = (BytecodeFunction *) code->functionByName(astFunction->name());
			uint16_t functionId = bytecodeFunction->id();

			VarType currentTypeOfTOs = typeOfTOS;
			VarType currentReturnType = returnType;
			Bytecode * currentBytecode = bytecode;
		
			returnType = astFunction->returnType();
			bytecode = bytecodeFunction->bytecode();
			contextId = functionId;

			scopeStack.push_back(std::map <std::string, uint16_t>());
			contextStack.push_back(contextId);

			for (uint32_t i = astFunction->node()->parametersNumber() - 1; i != (uint32_t) -1; --i) {
				uint16_t& currentVarId = contextVarIds[contextId];
				scopeStack.back().insert(std::make_pair(astFunction->parameterName(i), currentVarId));

				VarType paramType = astFunction->parameterType(i);
				if (paramType == VT_INT) {
					bytecode->addInsn(BC_STORECTXIVAR);
				}
				else if (paramType == VT_DOUBLE) {
					bytecode->addInsn(BC_STORECTXDVAR);
				}
				else if (paramType == VT_STRING) {
					bytecode->addInsn(BC_STORECTXSVAR);
				}
				else {
					throw TranslatorException("Ivalid type in function parameter: ", typeToName(paramType));
				}

				bytecode->addUInt16(contextId);
				bytecode->addUInt16(currentVarId);
				currentVarId++;
			}
			astFunction->node()->visit(this);

			//contextId = currentContextId;
			typeOfTOS = currentTypeOfTOs;
			returnType = currentReturnType;
			bytecode = currentBytecode;
		
			returnType = astFunction->returnType();
			bytecode = bytecodeFunction->bytecode();
			contextId = functionId;

			scopeStack.pop_back();
			contextStack.pop_back();
			contextId = contextStack.back();
		}
		
		node->visitChildren(this);
		scopeStack.pop_back();
		contextStack.pop_back();
	}


	virtual void visitForNode(mathvm::ForNode * node) {
		BinaryOpNode * loopExpresion = node->inExpr()->asBinaryOpNode();

		loopExpresion->left()->visit(this);

		VarDescriptor varDescriptor = getVarDscriptor(node->var()->name());
		bytecode->addInsn(BC_STORECTXIVAR);
		bytecode->addUInt16(varDescriptor.contextId);
		bytecode->addUInt16(varDescriptor.varId);

		Label loopCondition(bytecode), finish(bytecode);
		bytecode->bind(loopCondition);

		loopExpresion->right()->visit(this);
	
		bytecode->addInsn(BC_LOADCTXIVAR);
		bytecode->addUInt16(varDescriptor.contextId);
		bytecode->addUInt16(varDescriptor.varId);

		bytecode->addBranch(BC_IFICMPG, finish);
		node->body()->visit(this);

		bytecode->addInsn(BC_LOADCTXIVAR);
		bytecode->addUInt16(varDescriptor.contextId);
		bytecode->addUInt16(varDescriptor.varId);
		bytecode->addInsn(BC_ILOAD1);

		bytecode->addInsn(BC_IADD);
		bytecode->addInsn(BC_STORECTXIVAR);
		bytecode->addUInt16(varDescriptor.contextId);
		bytecode->addUInt16(varDescriptor.varId);

		bytecode->addBranch(BC_JA, loopCondition);
		bytecode->bind(finish);
	}


	virtual void visitWhileNode(mathvm::WhileNode * node) {
		Label loopCondition(bytecode), finish(bytecode);
		bytecode->bind(loopCondition);

		node->whileExpr()->visit(this);
		
		if (typeOfTOS != VT_INT) {
			throw TranslatorException("Invalid type in while expresion: ", typeToName(typeOfTOS));
		}
	
		bytecode->addInsn(BC_ILOAD0);
		bytecode->addBranch(BC_IFICMPE, finish);

		node->loopBlock()->visit(this);

		bytecode->addBranch(BC_JA, loopCondition);
		bytecode->bind(finish);
	}


	virtual void visitIfNode(mathvm::IfNode * node) {
		Label elseBody(bytecode), finish(bytecode);

		node->ifExpr()->visit(this);

		if (typeOfTOS != VT_INT) {
			throw TranslatorException("Invalid type in if expresion: ", typeToName(typeOfTOS));
		}
	
		bytecode->addInsn(BC_ILOAD0);
		bytecode->addBranch(BC_IFICMPE, elseBody);

		node->thenBlock()->visit(this);
		bytecode->addBranch(BC_JA, finish);

		bytecode->bind(elseBody);
		if (node->elseBlock()) {
			node->elseBlock()->visit(this);
		}
		bytecode->bind(finish);
	}


	virtual void visitReturnNode(mathvm::ReturnNode * node) {
		if (node->returnExpr()) {
			node->returnExpr()->visit(this);

			if (returnType == VT_INT && typeOfTOS == VT_DOUBLE) {
				bytecode->addInsn(BC_D2I);
			}
			else if (returnType == VT_DOUBLE && typeOfTOS == VT_INT) {
				bytecode->addInsn(BC_I2D);
			}
			else if (returnType == VT_INT && typeOfTOS == VT_STRING) {
				bytecode->addInsn(BC_S2I);
			}
			else if (returnType != typeOfTOS) {
				throw TranslatorException("Cannot cast following types in return statement: ", typeToName(typeOfTOS), typeToName(returnType));
			}
		}
		bytecode->addInsn(BC_RETURN);
	}


	virtual void visitFunctionNode(mathvm::FunctionNode * node) {
		node->body()->visit(this);
	}


	virtual void visitCallNode(mathvm::CallNode * node) {
		BytecodeFunction * bytecodeFunction = (BytecodeFunction *) code->functionByName(node->name());

		for (uint32_t i = 0; i < node->parametersNumber(); i++) {
			node->parameterAt(i)->visit(this);
			
			VarType paramType = bytecodeFunction->signature()[i + 1].first;
			if (paramType == VT_INT && typeOfTOS == VT_DOUBLE) {
				bytecode->addInsn(BC_D2I);
			}
			else if (paramType == VT_DOUBLE && typeOfTOS == VT_INT) {
				bytecode->addInsn(BC_I2D);
			}
			else if (paramType == VT_INT && typeOfTOS == VT_STRING) {
				bytecode->addInsn(BC_S2I);
			}
			else if (paramType != typeOfTOS) {
				throw TranslatorException("Cannot cast following types in function arguments: ", typeToName(typeOfTOS), typeToName(paramType));
			}
		}

		bytecode->addInsn(BC_CALL);
		bytecode->addUInt16(bytecodeFunction->id());

		typeOfTOS = bytecodeFunction->signature()[0].first;
	}


	virtual void visitNativeCallNode(mathvm::NativeCallNode * node) {
		bytecode->addInsn(BC_CALLNATIVE);
		bytecode->addUInt16(code->makeNativeFunction(node->nativeName(), node->nativeSignature(), 0));
	}


	virtual void visitPrintNode(mathvm::PrintNode * node) {
		for (uint32_t i = 0; i < node->operands(); ++i) {
			node->operandAt(i)->visit(this);

			if (typeOfTOS == VT_INT) {
				bytecode->addInsn(BC_IPRINT);
			}
			else if (typeOfTOS == VT_DOUBLE) {
				bytecode->addInsn(BC_DPRINT);
			}
			else if (typeOfTOS == VT_STRING) {
				bytecode->addInsn(BC_SPRINT);
			}
			else {
				throw TranslatorException("Unexpected type for print: ", typeToName(typeOfTOS));
			}
		}
	}

};


class BytecodeTranslator : public Translator {

public:
	Status * translate(const string& program, Code* *code) {
		Parser * parser = new Parser();
		Status * status = parser->parseProgram(program);

		if (status == 0) {
			BytecodeFunction * mainFunction = new BytecodeFunction(parser->top());
			(*code)->addFunction(mainFunction);
			
			AstToBytecode * toBytecode = new AstToBytecode(*code);
			parser->top()->node()->visit(toBytecode);
			mainFunction->bytecode()->add(BC_STOP);

			delete toBytecode;
		}

		delete parser;
		return status;
	}

};


class CodeExecutor: public Code {

public:
	Status * execute(vector <Var *> & vars) {
		return 0;
	}
};


Translator * Translator::create(const string& impl) {
	if (impl == "prj") {
		return new BytecodeTranslator();
	}

	return 0;
}



#endif /* _TRANSLATOR_HPP_ */
