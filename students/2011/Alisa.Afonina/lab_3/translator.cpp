#include <iostream>
#include <stdio.h>

#include "mathvm.h"
#include "ast.h"
#include "translator.h"

using namespace mathvm;

void GeneratingVisitor :: getIfResult(Label& endLabel, Instruction insn, Instruction ifinsn) {
	Label curLabel(&bytecode);
	if(previousType == VT_INT) {
			bytecode.addInsn(BC_ICMP);
			bytecode.addInsn(insn);
			bytecode.addBranch(ifinsn, curLabel);
			bytecode.addInsn(BC_ILOAD0);
			bytecode.addBranch(BC_JA, endLabel);
			bytecode.bind(curLabel);
			bytecode.addInsn(BC_ILOAD1);
		}
	if(previousType == VT_DOUBLE) {
			bytecode.addInsn(BC_DCMP);
			bytecode.addInsn(insn);
			bytecode.addBranch(ifinsn, curLabel);
			bytecode.addInsn(BC_DLOAD0);
			bytecode.addBranch(BC_JA, endLabel);
			bytecode.bind(curLabel);
			bytecode.addInsn(BC_DLOAD1);
	}
}

void GeneratingVisitor :: visitBinaryOpNode(BinaryOpNode* node) {
	Label endLabel(&bytecode);
	TokenKind kind = node->kind();
	node->left()->visit(this);
	if(previousType == VT_STRING || previousType == VT_INVALID) {
		//bytecode.addInsn(BC_INVALID);
		bytecode.addBranch(BC_JA, endLabel);
	}
	if(kind == tOR) {
		if(previousType == VT_INT) {
			bytecode.addInsn(BC_ILOAD1);
			bytecode.addBranch(BC_IFICMPE, endLabel);
		}
	}
	
	if(kind == tAND ) {
		if(previousType == VT_INT) {
			bytecode.addInsn(BC_ILOAD0);
			bytecode.addBranch(BC_IFICMPE, endLabel);
		}
	}
	int leftType = previousType;
	node->right()->visit(this);
	if(previousType == VT_STRING || previousType == VT_INVALID ) {
		//bytecode.addInsn(BC_INVALID);
		bytecode.addBranch(BC_JA, endLabel);
	}
	
	if(leftType != previousType) {
		if(leftType == VT_INT && previousType == VT_DOUBLE) {
			bytecode.addInsn(BC_D2I);
			previousType = VT_INT;
		}
		
		if(previousType == VT_INT && leftType == VT_DOUBLE) {
			bytecode.addInsn(BC_I2D);
			previousType = VT_DOUBLE;
		}
			
	}

	if(kind == tADD) {
		if(previousType == VT_INT) {
			bytecode.addInsn(BC_IADD);
		}
		if(previousType == VT_DOUBLE) {
			bytecode.addInsn(BC_DADD);
		}
	}
	if(kind == tSUB) {
		if(previousType == VT_INT) {
			bytecode.addInsn(BC_ISUB);
		}
		if(previousType == VT_DOUBLE) {
			bytecode.addInsn(BC_DSUB);
		}
	}
	if(kind == tMUL) {
		if(previousType == VT_INT) {
			bytecode.addInsn(BC_IMUL);
		}
		if(previousType == VT_DOUBLE) {
			bytecode.addInsn(BC_DMUL);
		}
	}
	if(kind == tDIV) {
		if(previousType == VT_INT) {
			bytecode.addInsn(BC_IDIV);
		}
		if(previousType == VT_DOUBLE) {
			bytecode.addInsn(BC_DDIV);
		}
	}
	
	if(kind == tEQ) {
		getIfResult(endLabel, BC_ILOAD0, BC_IFICMPE);
	}

	if(kind == tNEQ) {
		getIfResult(endLabel, BC_ILOAD0, BC_IFICMPNE);
	}

	if(kind == tLE) {
		getIfResult(endLabel, BC_ILOAD0, BC_IFICMPGE);
	}

	if(kind == tLT) {
		getIfResult(endLabel, BC_ILOAD1, BC_IFICMPE);
	}

	if(kind == tGE) {
		getIfResult(endLabel, BC_ILOAD0, BC_IFICMPLE);
	}
	if(kind == tGT) {
		getIfResult(endLabel, BC_ILOADM1, BC_IFICMPE);
	}
	bytecode.bind(endLabel);
}

void GeneratingVisitor ::  visitUnaryOpNode(UnaryOpNode* node) {
	node->operand()->visit(this);
	if(node->kind() == tSUB) {
		if(previousType == VT_INT) {
			bytecode.addInsn(BC_INEG);
		}
    if(previousType == VT_DOUBLE) {
      bytecode.addInsn(BC_DNEG);
    }
		if(previousType == VT_STRING || previousType == VT_INVALID){
			//bytecode.addInsn(BC_INVALID);
		}
	}
	if(node->kind() == tNOT) {
		if(previousType == VT_INT) { 
			Label thenLabel(&bytecode);
			Label endLabel(&bytecode);
			bytecode.addInsn(BC_ILOAD1);
			bytecode.addBranch(BC_IFICMPE, thenLabel);
			bytecode.addInsn(BC_ILOAD1);
			bytecode.addBranch(BC_JA, endLabel);
			bytecode.bind(thenLabel);
			bytecode.addInsn(BC_ILOAD0);
			bytecode.bind(endLabel);
		}
		if(previousType == VT_STRING || previousType == VT_INVALID
		 || previousType == VT_DOUBLE){
			//bytecode.addInsn(BC_INVALID);
		}
	}
}

void GeneratingVisitor ::  visitStringLiteralNode(StringLiteralNode* node) {
	bytecode.addInsn(BC_SLOAD);
	bytecode.addInt16(code.makeStringConstant(node->literal()));
	previousType = VT_STRING;
}

void GeneratingVisitor ::  visitDoubleLiteralNode(DoubleLiteralNode* node) {
	bytecode.addInsn(BC_DLOAD);
	bytecode.addDouble(node->literal());
	previousType = VT_DOUBLE;
}

void GeneratingVisitor ::  visitIntLiteralNode(IntLiteralNode* node) {
	bytecode.addInsn(BC_ILOAD);
	bytecode.addInt64(node->literal());
	previousType = VT_INT;
}

void GeneratingVisitor ::  visitLoadNode(LoadNode* node) {
  VarType type = node->var()->type();
	int id = varTable.getVarId(node->var());
 	switch (type) {
		case VT_INVALID: 
						 //bytecode.addInsn(BC_INVALID);
						 break; 
		case VT_DOUBLE:  bytecode.addInsn(BC_LOADDVAR);
					     previousType = VT_DOUBLE;
						 break;
		case VT_INT:     bytecode.addInsn(BC_LOADIVAR);
						 previousType = VT_INT;
					     break;
		case VT_STRING:  bytecode.addInsn(BC_LOADSVAR);
						 previousType = VT_STRING;
						 break;
	  }
	bytecode.addByte((uint8_t)id);  
}

void GeneratingVisitor ::  visitStoreNode(StoreNode* node) {
  node->value()->visit(this);
	VarType type = node->var()->type();
	int id = varTable.getVarId(node->var());
	switch (type) {
		case VT_INVALID: 
						//bytecode.addInsn(BC_INVALID);
						break;
		case VT_DOUBLE:  
			if(previousType == VT_STRING || previousType == VT_INVALID){
				//bytecode.addInsn(BC_INVALID);
				break;
			}
			
			if(previousType == VT_INT) {
				bytecode.addInsn(BC_I2D);
			}
			if(node->op() == tINCRSET) {
				bytecode.addInsn(BC_LOADDVAR);
				bytecode.addByte(id);
				bytecode.addInsn(BC_DADD);
				bytecode.addInsn(BC_STOREDVAR);
				bytecode.addByte(id);
        
				break;
			}
			
			if(node->op() == tDECRSET) {
				bytecode.addInsn(BC_LOADDVAR);
				bytecode.addByte(id);
				bytecode.addInsn(BC_SWAP);
				bytecode.addInsn(BC_DSUB);
				bytecode.addInsn(BC_STOREDVAR);
				bytecode.addByte(id);
				break;
			}
			
			if(node->op() == tASSIGN) {
				bytecode.addInsn(BC_STOREDVAR);
				bytecode.addByte(id);
				break;
			}
			
			//bytecode.addInsn(BC_INVALID);
			
		case VT_INT:     
			if(previousType == VT_STRING || previousType == VT_INVALID){
				//bytecode.addInsn(BC_INVALID);
				break;
			}
			
			if(previousType == VT_DOUBLE) {
				bytecode.addInsn(BC_D2I);
			}
			if(node->op() == tINCRSET) {
				bytecode.addInsn(BC_LOADIVAR);
				bytecode.addByte(id);
				bytecode.addInsn(BC_IADD);
				bytecode.addInsn(BC_STOREIVAR);
        bytecode.addByte(id);

				break;
			}
			
			if(node->op() == tDECRSET) {
				bytecode.addInsn(BC_LOADIVAR);
				bytecode.addByte(id);
				bytecode.addInsn(BC_SWAP);
				bytecode.addInsn(BC_ISUB);
				bytecode.addInsn(BC_STOREIVAR);
				bytecode.addByte(id);
				break;
			}
			
			if(node->op() == tASSIGN) {
				bytecode.addInsn(BC_STOREIVAR);
        bytecode.addByte(id);

				break;
			}
			//bytecode.addInsn(BC_INVALID);
			
		case VT_STRING:  
			if(previousType == VT_INT || previousType == VT_INVALID
			   || previousType == VT_DOUBLE){
				//bytecode.addInsn(BC_INVALID);
			}
			if(previousType == VT_STRING) {
				if(node->op() == tASSIGN) {
					bytecode.addInsn(BC_STORESVAR);
          bytecode.addByte(id);

					break;
				}
			}
	}
}

void GeneratingVisitor ::  visitForNode(ForNode* node) {
	Label endLabel(&bytecode);
	Label bodyLabel(&bytecode);
	int id = varTable.getVarId(node->var());
	
	if(!(node->inExpr()->isBinaryOpNode() && ((BinaryOpNode*)node->inExpr())->kind() == tRANGE)) {
		//bytecode.addInsn(BC_INVALID);
		bytecode.addBranch(BC_JA, endLabel);
	}
	BinaryOpNode* expr = (BinaryOpNode*)node->inExpr();
	expr->left()->visit(this);
	bytecode.addInsn(BC_STOREIVAR);
	bytecode.addByte(id);
	if(previousType != VT_INT) {/*ERROR*/ return;}
	bytecode.bind(bodyLabel);
	bytecode.addInsn(BC_LOADIVAR);
	bytecode.addByte(id);
	expr->right()->visit(this);
	if(previousType != VT_INT) {/*ERROR*/ return;}
	bytecode.addBranch(BC_IFICMPG, endLabel);
	node->body()->visit(this);

	bytecode.addInsn(BC_LOADIVAR);
	bytecode.addByte(id);
	bytecode.addInsn(BC_ILOAD1);
	bytecode.addInsn(BC_IADD);
	bytecode.addInsn(BC_STOREIVAR);
	bytecode.addByte(id);
	bytecode.addBranch(BC_JA, bodyLabel);
	bytecode.bind(endLabel);
}

void GeneratingVisitor ::  visitWhileNode(WhileNode* node) {
	Label endLabel(&bytecode);
	Label loopLabel(&bytecode);
	
	bytecode.bind(loopLabel);
	node->whileExpr()->visit(this);
	bytecode.addInsn(BC_ILOAD1);
	bytecode.addBranch(BC_IFICMPNE, endLabel);
	node->loopBlock()->visit(this);
	bytecode.addBranch(BC_JA, loopLabel);
	bytecode.bind(endLabel);
}

void GeneratingVisitor ::  visitIfNode(IfNode* node) {
	Label thenLabel(&bytecode);
	Label endLabel(&bytecode);
	
	node->ifExpr()->visit(this);
	bytecode.addInsn(BC_ILOAD1);
	bytecode.addBranch(BC_IFICMPE, thenLabel);
	
	if(node->elseBlock()) {
		node->elseBlock()->visit(this);
	}
  bytecode.addBranch(BC_JA, endLabel);		 
	bytecode.bind(thenLabel);
	node->thenBlock()->visit(this);
	bytecode.bind(endLabel);
}

void GeneratingVisitor ::  visitBlockNode(BlockNode* node) {
	Scope::VarIterator it(node->scope());
	while (it.hasNext()) {
		AstVar* var = it.next();
		varTable.registerVariable(var);
	}
	node->visitChildren(this);	
}


void GeneratingVisitor ::  visitFunctionNode(FunctionNode* node) {}

void GeneratingVisitor ::  visitReturnNode(ReturnNode* node) {
	bytecode.addInsn(BC_RETURN);
}
void GeneratingVisitor ::  visitCallNode(CallNode* node) {	
}

void GeneratingVisitor ::  visitPrintNode(PrintNode* node) {
	for (int i = 0; i < node->operands(); ++i) {
		AstNode* operand = node->operandAt(i);
		operand->visit(this);	
		if(previousType == VT_STRING) {
			bytecode.addInsn(BC_SPRINT);
		}
		if(previousType == VT_DOUBLE) {
			bytecode.addInsn(BC_DPRINT);
		}
		if(previousType == VT_INT) {
			bytecode.addInsn(BC_IPRINT);
		}
	}	
}

void GeneratingVisitor :: visit(BlockNode* node) {
	node->visit(this);	
	bytecode.addInsn(BC_STOP);
}

void GeneratingVisitor :: dump() {
  bytecode.dump();
}

Bytecode * GeneratingVisitor::getBytecode() {
  return &bytecode;
}

std::vector<std::string> GeneratingVisitor::getStringsVector() {
  vector<string> result;
  for (uint16_t i = 0; i < 256; ++i) {
    string s = code.constantById(i);
    result.push_back(s);
  }
  return result;
}
