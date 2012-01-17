#include <iostream>
#include <stdio.h>

#include "mathvm.h"
#include "ast.h"
#include "mytranslator.h"

using namespace mathvm;

void GeneratingVisitor :: getIfResult(Label& endLabel, Instruction insn, Instruction ifinsn) {
	Label curLabel(currentBytecode);
	if(previousType == VT_INT) {
			currentBytecode->addInsn(BC_ICMP);
			currentBytecode->addInsn(insn);
			currentBytecode->addBranch(ifinsn, curLabel);
			currentBytecode->addInsn(BC_ILOAD0);
			currentBytecode->addBranch(BC_JA, endLabel);
			currentBytecode->bind(curLabel);
			currentBytecode->addInsn(BC_ILOAD1);
		}
	if(previousType == VT_DOUBLE) {
			currentBytecode->addInsn(BC_DCMP);
			currentBytecode->addInsn(insn);
			currentBytecode->addBranch(ifinsn, curLabel);
			currentBytecode->addInsn(BC_DLOAD0);
			currentBytecode->addBranch(BC_JA, endLabel);
			currentBytecode->bind(curLabel);
			currentBytecode->addInsn(BC_DLOAD1);
	}
}

void GeneratingVisitor :: visitBinaryOpNode(BinaryOpNode* node) {
	Label endLabel(currentBytecode);
	Label trueLabel(currentBytecode);
	Label falseLabel(currentBytecode);
	TokenKind kind = node->kind();
	node->right()->visit(this);
	if(previousType == VT_STRING || previousType == VT_INVALID) {
		throw GeneratingException("Wrong type");
	}
	Instruction result;
	if(kind == tOR) {
		if(previousType == VT_INT) {
			currentBytecode->addInsn(BC_ILOAD1);
			currentBytecode->addBranch(BC_IFICMPE, trueLabel);
		}
	}
	
	if(kind == tAND ) {
		if(previousType == VT_INT) {
			currentBytecode->addInsn(BC_ILOAD0);
			currentBytecode->addBranch(BC_IFICMPE, falseLabel);
		}
	}
	int leftType = previousType;
	node->left()->visit(this);
	if(previousType == VT_STRING || previousType == VT_INVALID ) {
		throw GeneratingException("Wrong type");
	}
	
	if(leftType != previousType) {
		if(leftType == VT_INT && previousType == VT_DOUBLE) {
			currentBytecode->addInsn(BC_D2I);
			previousType = VT_INT;
		}
		
		if(previousType == VT_INT && leftType == VT_DOUBLE) {
			currentBytecode->addInsn(BC_I2D);
			previousType = VT_DOUBLE;
		}
			
	}

	if(kind == tADD) {
		if(previousType == VT_INT) {
			currentBytecode->addInsn(BC_IADD);
		}
		if(previousType == VT_DOUBLE) {
			currentBytecode->addInsn(BC_DADD);
		}
	}
	if(kind == tSUB) {
		if(previousType == VT_INT) {
			currentBytecode->addInsn(BC_ISUB);
		}
		if(previousType == VT_DOUBLE) {
			currentBytecode->addInsn(BC_DSUB);
		}
	}
	if(kind == tMUL) {
		if(previousType == VT_INT) {
			currentBytecode->addInsn(BC_IMUL);
		}
		if(previousType == VT_DOUBLE) {
			currentBytecode->addInsn(BC_DMUL);
		}
	}
	if(kind == tDIV) {
		if(previousType == VT_INT) {
			currentBytecode->addInsn(BC_IDIV);
		}
		if(previousType == VT_DOUBLE) {
			currentBytecode->addInsn(BC_DDIV);
		}
	}
	
	if(kind == tEQ) {
		getIfResult(endLabel, BC_ILOAD0, BC_IFICMPE);
	}

	if(kind == tNEQ) {
		getIfResult(endLabel, BC_ILOAD0, BC_IFICMPNE);
	}

	if(kind == tLE) {
		getIfResult(endLabel, BC_ILOADM1, BC_IFICMPNE);
	}

	if(kind == tLT) {
		getIfResult(endLabel, BC_ILOAD1, BC_IFICMPE);
	}

	if(kind == tGE) {
		getIfResult(endLabel, BC_ILOAD1, BC_IFICMPNE);
	}
	if(kind == tGT) {
		getIfResult(endLabel, BC_ILOADM1, BC_IFICMPE);
	}
	currentBytecode->addBranch(BC_JA, endLabel);
	currentBytecode->bind(trueLabel);
	currentBytecode->addInsn(BC_ILOAD1);
	currentBytecode->addBranch(BC_JA, endLabel);
	currentBytecode->bind(falseLabel);
	currentBytecode->addInsn(BC_ILOAD0);
	currentBytecode->bind(endLabel);
}

void GeneratingVisitor ::  visitUnaryOpNode(UnaryOpNode* node) {
	node->operand()->visit(this);
	if(node->kind() == tSUB) {
		if(previousType == VT_INT) {
			currentBytecode->addInsn(BC_INEG);
		}
    if(previousType == VT_DOUBLE) {
      currentBytecode->addInsn(BC_DNEG);
    }
		if(previousType == VT_STRING || previousType == VT_INVALID){
			throw GeneratingException("Wrong type");
		}
	}
	if(node->kind() == tNOT) {
		if(previousType == VT_INT) { 
			Label thenLabel(currentBytecode);
			Label endLabel(currentBytecode);
			currentBytecode->addInsn(BC_ILOAD0);
			currentBytecode->addBranch(BC_IFICMPE, thenLabel);
			currentBytecode->addInsn(BC_ILOAD1);
			currentBytecode->addBranch(BC_JA, endLabel);
			currentBytecode->bind(thenLabel);
			currentBytecode->addInsn(BC_ILOAD1);
			currentBytecode->bind(endLabel);
		}
		if(previousType == VT_STRING || previousType == VT_INVALID
		 || previousType == VT_DOUBLE){
			throw GeneratingException("Wrong type");
		}
	}
}

void GeneratingVisitor ::  visitStringLiteralNode(StringLiteralNode* node) {
	currentBytecode->addInsn(BC_SLOAD);
	currentBytecode->addInt16(code.makeStringConstant(node->literal()));
	previousType = VT_STRING;
}

void GeneratingVisitor ::  visitDoubleLiteralNode(DoubleLiteralNode* node) {
	currentBytecode->addInsn(BC_DLOAD);
	currentBytecode->addDouble(node->literal());
	previousType = VT_DOUBLE;
}

void GeneratingVisitor ::  visitIntLiteralNode(IntLiteralNode* node) {
	currentBytecode->addInsn(BC_ILOAD);
	currentBytecode->addInt64(node->literal());
	previousType = VT_INT;
}

bool GeneratingVisitor :: getVarId(string const & name, InfoId& id_out) {
	
	Scope * localScope = currentScope;
	uint16_t currentFunctionId = _ids[localScope]._function_id;
	while(localScope) {
		int count = 0;
		InfoId id = _ids[localScope];
		id_out = id;
		if(id_out.variables.find(name) != id_out.variables.end()) {
			id_out._var_counter = id_out.variables[name];
			return currentFunctionId == id._function_id;
		}
		localScope = localScope->parent();
	}
	throw GeneratingException("Variable not found.");
}


void GeneratingVisitor ::  visitLoadNode(LoadNode* node) {
	const AstVar* var = node->var();
	VarType type = var->type();
	InfoId id;
	bool isClosure = !getVarId(var->name(), id);
	Instruction a;
 	switch (type) {
		case VT_DOUBLE:		a = (isClosure)?BC_LOADCTXDVAR:BC_LOADDVAR; 
							previousType = VT_DOUBLE;
							break;
		case VT_INT:		a = (isClosure)?BC_LOADCTXIVAR:BC_LOADIVAR; 
							previousType = VT_INT;
							break;
		case VT_STRING:		a = (isClosure)?BC_LOADCTXSVAR:BC_LOADSVAR; 
							previousType = VT_STRING;
							break;
		default: throw GeneratingException("Wrong type");
	}
	currentBytecode->addInsn(a);
	if(isClosure)
		currentBytecode->addUInt16((uint16_t)id._function_id);  
	currentBytecode->addUInt16((uint16_t)id._var_counter);  
}

void GeneratingVisitor ::  visitStoreNode(StoreNode* node) {
	node->value()->visit(this);
	const AstVar* var = node->var();
	VarType type = var->type();
	InfoId id;
	bool isClosure = !getVarId(var->name(), id);
	Instruction store;
	Instruction load;
	switch (type) {
		case VT_INVALID:throw GeneratingException("Wrong type"); 
		case VT_DOUBLE:  
			if(previousType == VT_STRING || previousType == VT_INVALID){
				throw GeneratingException("Wrong type");
			}
			
			if(previousType == VT_INT) {
				currentBytecode->addInsn(BC_I2D);
			}
			store = (isClosure)?BC_STORECTXDVAR:BC_STOREDVAR;
			load = (isClosure)?BC_LOADCTXDVAR:BC_LOADDVAR;
			if(node->op() == tINCRSET) {
				currentBytecode->addInsn(load);
				if(isClosure) currentBytecode->addUInt16((uint16_t)id._function_id);  
				currentBytecode->addUInt16(id._var_counter);
				currentBytecode->addInsn(BC_DADD);
				currentBytecode->addInsn(store);
				if(isClosure) currentBytecode->addUInt16((uint16_t)id._function_id);  
				currentBytecode->addUInt16(id._var_counter);
				break;
			}
			
			if(node->op() == tDECRSET) {
				currentBytecode->addInsn(load);
				if(isClosure) currentBytecode->addUInt16((uint16_t)id._function_id);  
				currentBytecode->addUInt16(id._var_counter);
				currentBytecode->addInsn(BC_SWAP);
				currentBytecode->addInsn(BC_DSUB);
				currentBytecode->addInsn(store);
				if(isClosure) currentBytecode->addUInt16((uint16_t)id._function_id);  
				currentBytecode->addUInt16(id._var_counter);
				break;
			}
			
			if(node->op() == tASSIGN) {
				currentBytecode->addInsn(store);
				if(isClosure) currentBytecode->addUInt16((uint16_t)id._function_id);  
				currentBytecode->addUInt16(id._var_counter);
				break;
			}
			
			throw GeneratingException("Wrong op");
			
		case VT_INT:     
			if(previousType == VT_STRING || previousType == VT_INVALID){
				throw GeneratingException("Wrong type");
			}
			
			if(previousType == VT_DOUBLE) {
				currentBytecode->addInsn(BC_D2I);
			}
			store = (isClosure)?BC_STORECTXIVAR:BC_STOREIVAR;
			load = (isClosure)?BC_LOADCTXIVAR:BC_LOADIVAR;
			if(node->op() == tINCRSET) {
				currentBytecode->addInsn(load);
				if(isClosure) currentBytecode->addUInt16((uint16_t)id._function_id);  
				currentBytecode->addUInt16(id._var_counter);
				currentBytecode->addInsn(BC_IADD);
				currentBytecode->addInsn(store);
				if(isClosure) currentBytecode->addUInt16((uint16_t)id._function_id);  
				currentBytecode->addUInt16(id._var_counter);

				break;
			}
			
			if(node->op() == tDECRSET) {
				currentBytecode->addInsn(load);
				if(isClosure) currentBytecode->addUInt16((uint16_t)id._function_id);  
				currentBytecode->addUInt16(id._var_counter);
				currentBytecode->addInsn(BC_SWAP);
				currentBytecode->addInsn(BC_ISUB);
				currentBytecode->addInsn(store);
				if(isClosure) currentBytecode->addUInt16((uint16_t)id._function_id);  
				currentBytecode->addUInt16(id._var_counter);
				break;
			}
			
			if(node->op() == tASSIGN) {
				currentBytecode->addInsn(store);
				if(isClosure) currentBytecode->addUInt16((uint16_t)id._function_id);  
				currentBytecode->addUInt16(id._var_counter);
				break;
			}
			throw GeneratingException("Wrong op");
			
		case VT_STRING:  
			if(previousType == VT_INT || previousType == VT_INVALID
			   || previousType == VT_DOUBLE){
				throw GeneratingException("Wrong type");
			}
			if(previousType == VT_STRING) {
				store = (isClosure)?BC_STORECTXSVAR:BC_STORESVAR;
				if(node->op() == tASSIGN) {
					currentBytecode->addInsn(store);
					if(isClosure) currentBytecode->addUInt16((uint16_t)id._function_id);  
					currentBytecode->addUInt16(id._var_counter);
					break;
				}
			}
		case VT_VOID: throw GeneratingException("Wrong type");
	}
}

void GeneratingVisitor ::  visitForNode(ForNode* node) {
	Label endLabel(currentBytecode);
	Label bodyLabel(currentBytecode);
	int id = varTable.getVarId(node->var());
	
	if(!(node->inExpr()->isBinaryOpNode() && ((BinaryOpNode*)node->inExpr())->kind() == tRANGE)) {
		throw GeneratingException("Wrong for declaration");
	}
	BinaryOpNode* expr = (BinaryOpNode*)node->inExpr();
	expr->left()->visit(this);
	currentBytecode->addInsn(BC_STOREIVAR);
	currentBytecode->addUInt16(id);
	if(previousType != VT_INT) {throw GeneratingException("Wrong type");}
	currentBytecode->bind(bodyLabel);
	currentBytecode->addInsn(BC_LOADIVAR);
	currentBytecode->addUInt16(id);
	expr->right()->visit(this);
	if(previousType != VT_INT) {throw GeneratingException("Wrong type");}
	currentBytecode->addBranch(BC_IFICMPL, endLabel);
	node->body()->visit(this);

	currentBytecode->addInsn(BC_LOADIVAR);
	currentBytecode->addUInt16(id);
	currentBytecode->addInsn(BC_ILOAD1);
	currentBytecode->addInsn(BC_IADD);
	currentBytecode->addInsn(BC_STOREIVAR);
	currentBytecode->addUInt16(id);
	currentBytecode->addBranch(BC_JA, bodyLabel);
	currentBytecode->bind(endLabel);
}

void GeneratingVisitor ::  visitWhileNode(WhileNode* node) {
	Label endLabel(currentBytecode);
	Label loopLabel(currentBytecode);
	
	currentBytecode->bind(loopLabel);
	node->whileExpr()->visit(this);
	currentBytecode->addInsn(BC_ILOAD0);
	currentBytecode->addBranch(BC_IFICMPE, endLabel);
	node->loopBlock()->visit(this);
	currentBytecode->addBranch(BC_JA, loopLabel);
	currentBytecode->bind(endLabel);
}

void GeneratingVisitor ::  visitIfNode(IfNode* node) {
	Label thenLabel(currentBytecode);
	Label endLabel(currentBytecode);
	
	node->ifExpr()->visit(this);
	currentBytecode->addInsn(BC_ILOAD1);
	currentBytecode->addBranch(BC_IFICMPE, thenLabel);
	
	if(node->elseBlock()) {
		node->elseBlock()->visit(this);
	}
    currentBytecode->addBranch(BC_JA, endLabel);		 
	currentBytecode->bind(thenLabel);
	node->thenBlock()->visit(this);
	currentBytecode->bind(endLabel);
}

void GeneratingVisitor ::  visitBlockNode(BlockNode* node) {
	
	Scope::FunctionIterator fit(node->scope());
	
	InfoId &info_id = _ids[currentScope];
	Scope* scope = currentScope;
	currentScope = node->scope();
	Scope::VarIterator it(currentScope);
	int count = 0;

	while (it.hasNext()) {
		AstVar* var = it.next();
		info_id.variables[var->name()] = info_id._var_counter + count;
		count++;
	}
	info_id._var_counter += currentScope->variablesCount();


	while (fit.hasNext()) {
		AstFunction* function = fit.next();
		FunctionInfo fid;
		fid._ast_function = function;
		fid._function_id = _functions.size();
		_functions[function->name()] = fid;
		function->node()->visit(this);
	}

	
	node->visitChildren(this);	
	currentScope = scope;

	InfoId &parent_id = _ids[currentScope];
	parent_id._vars_inside_scope_count = std::max<uint16_t>(parent_id._vars_inside_scope_count,
															currentScope->parent()->variablesCount()
															+ info_id._vars_inside_scope_count);
}


void GeneratingVisitor ::  visitFunctionNode(FunctionNode* node) {
	Bytecode* localBytecode = new Bytecode;
	Bytecode * oldBytecode = currentBytecode;
	currentBytecode = localBytecode;

	BlockNode * block = node->body();
	Scope* scope = currentScope;
	currentScope = block->scope();
	

	FunctionInfo id = _functions[node->name()];

	InfoId info_id;
	info_id._function_id = id._function_id;
	info_id._var_counter = node->parametersNumber();
	info_id._ast_function = id._ast_function;
	info_id._is_function = true;
	
	for(int i = 0; i < node->parametersNumber(); ++i) {
		info_id.variables[node->parameterName(i)] = i;
	}
	
	_ids[currentScope] = info_id;	
	BytecodeFunction * bytecode_function = new BytecodeFunction(id._ast_function);
	code.addFunction(bytecode_function);	
	block->visit(this);
	

	InfoId parent_id = _ids[currentScope->parent()];
	parent_id._vars_inside_scope_count = std::max<uint16_t>(parent_id._vars_inside_scope_count,
															currentScope->parent()->variablesCount()
															+ info_id._vars_inside_scope_count);
	if(!oldBytecode)
		currentBytecode->addInsn(BC_STOP);
	*bytecode_function->bytecode() = *currentBytecode;
	if(oldBytecode) {
		currentBytecode = oldBytecode;
	}
	
	currentScope = scope;
}


void GeneratingVisitor ::  visitReturnNode(ReturnNode* node) {
	AstNode * returnExpr = node->returnExpr();
	if(returnExpr)returnExpr->visit(this);
	currentBytecode->addInsn(BC_RETURN);
}
void GeneratingVisitor ::  visitCallNode(CallNode* node) {	\
	node->visitChildren(this);
	currentBytecode->addInsn(BC_CALL);
	currentBytecode->addUInt16(_functions[node->name()]._function_id);
}

void GeneratingVisitor ::  visitPrintNode(PrintNode* node) {
	for (unsigned int i = 0; i < node->operands(); ++i) {
		AstNode* operand = node->operandAt(i);
		operand->visit(this);	
		if(previousType == VT_STRING) {
			currentBytecode->addInsn(BC_SPRINT);
		}
		if(previousType == VT_DOUBLE) {
			currentBytecode->addInsn(BC_DPRINT);
		}
		if(previousType == VT_INT) {
			currentBytecode->addInsn(BC_IPRINT);
		}
	}	
}

void GeneratingVisitor :: visit(AstFunction* node) {
	FunctionInfo cur_info;
	currentBytecode = NULL;
	cur_info._function_id = 0;
	cur_info._ast_function = node;
	_functions[node->name()] = cur_info;
	node->node()->visit(this);
	
	code.setBytecode(currentBytecode);
	//BytecodeFunction * bytecode_function = new BytecodeFunction(cur_info._ast_function);
	//*bytecode_function->bytecode() = *currentBytecode;
	//code.addFunction(bytecode_function);
}

void GeneratingVisitor :: dump() {
	code.disassemble(cout);
}


std::vector<std::string> GeneratingVisitor::getStringsVector() {
  vector<string> result;
  for (uint16_t i = 0; i < 256; ++i) {
    string s = code.constantById(i);
    result.push_back(s);
  }
  return result;
}
