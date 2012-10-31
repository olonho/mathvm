#include "bytecode_visitor.h"
#include "ast.h"

using namespace std;
using namespace mathvm;

void ByteCodeVisitor::visitBlockNode(BlockNode *node) {
    Scope::VarIterator var(node->scope());
    while(var.hasNext()) {
			AstVar* cur_var = var.next();
			vars[cur_var->name()] = last_id++;
    }
    Scope::FunctionIterator func(node->scope());
    while(func.hasNext())
		{
			AstFunction* function= func.next();
      BytecodeFunction* bytecodeFunction = new BytecodeFunction(function);
      code->addFunction(bytecodeFunction);
			function->node()->visit(this);
		}
    for (uint32_t i = 0; i < node->nodes(); ++i)
    {
    	node->nodeAt(i)->visit(this);
    }
}

void ByteCodeVisitor::visitForNode(ForNode *node) {
}

void ByteCodeVisitor::visitPrintNode(PrintNode *node) {
    for (uint32_t i = 0; i < node->operands(); i++) {
        AstNode *pNode = node->operandAt(i);
        pNode->visit(this);
				switch (TOStype)
				{
					case VT_INT:
					{
						bytecode->addInsn(BC_IPRINT);
						break;
					}
					case VT_DOUBLE:
					{
						bytecode->addInsn(BC_DPRINT);
						break;
					}
					case VT_STRING:
					{
						bytecode->addInsn(BC_SPRINT);
						break;
					}
					default:
						std::cout << "Can't print properly!\n";
				}
    }
}

void ByteCodeVisitor::visitLoadNode(LoadNode *node) {
		switch (node->var()->type())
		{
			case VT_INT:
			{
				TOStype = VT_INT;
				bytecode->addInsn(BC_LOADIVAR);
				bytecode->addUInt16(vars[node->var()->name()]);
				break;
			}
			case VT_DOUBLE:
			{
				TOStype = VT_DOUBLE;
				bytecode->addInsn(BC_LOADDVAR);
				bytecode->addUInt16(vars[node->var()->name()]);
				break;
			}
			case VT_STRING:
			{
				TOStype = VT_STRING;
				bytecode->addInsn(BC_LOADSVAR);
				bytecode->addUInt16(vars[node->var()->name()]);
				break;
			}
			default:
			{
				std::cout << "Can't load variable!\n";
			}
		}
    node->var()->name();
}

void ByteCodeVisitor::visitIfNode(IfNode *node) {
    node->ifExpr()->visit(this);
		Label else_label(bytecode);
		Label end(bytecode);
		bytecode->addInsn(BC_ILOAD1);
		bytecode->addBranch(BC_IFICMPNE, else_label);
    node->thenBlock()->visit(this);
		bytecode->addBranch(BC_JA,end);
		bytecode->bind(else_label);
    node->elseBlock()->visit(this);
		bytecode->bind(end);
}

void ByteCodeVisitor::visitCallNode(CallNode *node) {
    for (uint32_t i = 0; i < node->parametersNumber(); i++) {
        node->parameterAt(i)->visit(this);	
				switch (code->functionByName(node->name())->parameterType(i))
				{
					case VT_INT:
					{
						bytecode->addInsn(BC_STOREIVAR);
						bytecode->addUInt16(vars[code->functionByName(node->name())->parameterName(i)]);
						break;
					}
					case VT_DOUBLE:
					{
						bytecode->addInsn(BC_STOREDVAR);
						bytecode->addUInt16(vars[code->functionByName(node->name())->parameterName(i)]);
						break;
					}
					case VT_STRING:
					{
						bytecode->addInsn(BC_STORESVAR);
						bytecode->addUInt16(vars[code->functionByName(node->name())->parameterName(i)]);
						break;
					}
					default:
					{
						std::cout << "Can't call function!\n";
					}
				}	
    }
    TOStype = code->functionByName(node->name())->returnType();
		bytecode->addInsn(BC_CALL);
		bytecode->addUInt16(code->functionByName(node->name())->id());
}

void ByteCodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
	bytecode->addInsn(BC_DLOAD);
	bytecode->addInt16(node->literal());
	TOStype = VT_DOUBLE;
}

void ByteCodeVisitor::visitStoreNode(StoreNode *node) {
    node->value()->visit(this);
		switch (node->op())
		{
			case tINCRSET:
			{	
				switch (node->var()->type())
				{
					case VT_INT:
					{
						bytecode->addInsn(BC_LOADIVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						bytecode->addInsn(BC_SWAP);
						bytecode->addInsn(BC_IADD);
						bytecode->addInsn(BC_STOREIVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						break;
					}
					case VT_DOUBLE:
					{
						bytecode->addInsn(BC_LOADDVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						bytecode->addInsn(BC_SWAP);
						bytecode->addInsn(BC_DADD);
						bytecode->addInsn(BC_STOREDVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						break;
					}
					default:
					{
						std::cout << "Can't store variable!\n";
					}
				}
				break;
			}
			case tDECRSET:
			{				
				switch (node->var()->type())
				{
					case VT_INT:
					{
						bytecode->addInsn(BC_LOADIVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						bytecode->addInsn(BC_SWAP);
						bytecode->addInsn(BC_ISUB);
						bytecode->addInsn(BC_STOREIVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						break;
					}
					case VT_DOUBLE:
					{
						bytecode->addInsn(BC_LOADDVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						bytecode->addInsn(BC_SWAP);
						bytecode->addInsn(BC_DSUB);
						bytecode->addInsn(BC_STOREDVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						break;
					}
					default:
					{
						std::cout << "Can't store variable!\n";
					}
				break;
				}
			}
			case tASSIGN:
			{	
				switch (node->var()->type())
				{
					case VT_INT:
					{
						bytecode->addInsn(BC_STOREIVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						break;
					}
					case VT_DOUBLE:
					{
						bytecode->addInsn(BC_STOREDVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						break;
					}
					case VT_STRING:
					{
						bytecode->addInsn(BC_STORESVAR);
						bytecode->addUInt16(vars[node->var()->name()]);
						break;
					}
					default:
					{
						std::cout << "Can't store variable!\n";
					}
				}
				break;
			}
			default:
			{
				std::cout << "Can't store variable!\n";
			}
		}
}

void ByteCodeVisitor::visitStringLiteralNode(StringLiteralNode *node) {
	bytecode->addInsn(BC_SLOAD);
	bytecode->addInt16(code->makeStringConstant(node->literal()));
	TOStype = VT_STRING;
}

void ByteCodeVisitor::visitWhileNode(WhileNode *node) {
		Label while_label(bytecode);
		Label end(bytecode);
   		node->whileExpr()->visit(this);
			bytecode->bind(while_label);
			bytecode->addInsn(BC_ILOAD1);
			bytecode->addBranch(BC_IFICMPNE, end);
   		node->loopBlock()->visit(this);
			bytecode->addBranch(BC_JA, while_label);
			bytecode->bind(end);
}

void ByteCodeVisitor::visitIntLiteralNode(IntLiteralNode *node) {
	bytecode->addInsn(BC_ILOAD);
	bytecode->addInt16(node->literal());
	TOStype = VT_INT;
}

void ByteCodeVisitor::visitFunctionNode(FunctionNode *node) {
    if(node->name() == "<top>")
    {
			node->body()->visit(this);
			return;
    }
    for (uint32_t j = 0; j < node->parametersNumber(); j++) {
				vars[node->parameterName(j)] = node->parameterType(j);
    }
    if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode())
    {
				node->body()->nodeAt(0)->visit(this);
    }
    else
    {
    	node->body()->visit(this);
    }
		code->addFunction(code->functionByName(node->name()));
		TOStype = node->returnType();
}

void ByteCodeVisitor::visitBinaryOpNode(BinaryOpNode *node) {
    node->left()->visit(this);
    node->right()->visit(this);
		bytecode->add(BC_SWAP);
		switch(node->kind())
		{
			case tAND:
			{
				Label right(bytecode);
				Label true_end(bytecode);
				Label end(bytecode);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_IFICMPNE, right);
				bytecode->addInsn(BC_POP);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->bind(right);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_IFICMPNE, true_end);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_JA, end);
				bytecode->bind(true_end);
				bytecode->addInsn(BC_ILOAD1);
				bytecode->bind(end);
				break;
			}
			case tOR:
			{
				Label true_end_pop(bytecode);
				Label true_end(bytecode);
				Label end(bytecode);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_IFICMPNE, true_end_pop);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_IFICMPNE, true_end);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_JA, end);
				bytecode->bind(true_end_pop);
				bytecode->addInsn(BC_POP);
				bytecode->bind(true_end);
				bytecode->addInsn(BC_ILOAD1);
				bytecode->bind(end);
				break;
			}
			case tEQ:
			{
				Label true_end(bytecode);
				Label end(bytecode);
				bytecode->addBranch(BC_IFICMPE, true_end);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_JA, end);
				bytecode->bind(true_end);
				bytecode->addInsn(BC_ILOAD1);
				bytecode->bind(end);
				break;
			}
			case tNEQ:
			{
				Label true_end(bytecode);
				Label end(bytecode);
				bytecode->addBranch(BC_IFICMPNE, true_end);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_JA, end);
				bytecode->bind(true_end);
				bytecode->addInsn(BC_ILOAD1);
				bytecode->bind(end);
				break;
			}
			case tGT:
			{
				Label true_end(bytecode);
				Label end(bytecode);
				bytecode->addBranch(BC_IFICMPG, true_end);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_JA, end);
				bytecode->bind(true_end);
				bytecode->addInsn(BC_ILOAD1);
				bytecode->bind(end);
				break;
			}
			case tGE:
			{
				Label true_end(bytecode);
				Label end(bytecode);
				bytecode->addBranch(BC_IFICMPGE, true_end);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_JA, end);
				bytecode->bind(true_end);
				bytecode->addInsn(BC_ILOAD1);
				bytecode->bind(end);
				break;
			}
			case tLT:
			{
				Label true_end(bytecode);
				Label end(bytecode);
				bytecode->addBranch(BC_IFICMPL, true_end);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_JA, end);
				bytecode->bind(true_end);
				bytecode->addInsn(BC_ILOAD1);
				bytecode->bind(end);
				break;
			}
			case tLE:
			{
				Label true_end(bytecode);
				Label end(bytecode);
				bytecode->addBranch(BC_IFICMPLE, true_end);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_JA, end);
				bytecode->bind(true_end);
				bytecode->addInsn(BC_ILOAD1);
				bytecode->bind(end);
				break;
			}

			case tADD:
			{
				if (TOStype == VT_DOUBLE)
				{
					bytecode->addInsn(BC_DADD);
				}
				else if (TOStype == VT_INT)
				{
					bytecode->addInsn(BC_IADD);
				}
				else
				{
					std::cerr << "Impossible adding!\n";
				}
				break;
			}			
			case tSUB:
			{
				if (TOStype == VT_DOUBLE)
				{
					bytecode->addInsn(BC_DSUB);
				}
				else if (TOStype == VT_INT)
				{
					bytecode->addInsn(BC_ISUB);
				}
				else
				{
					std::cerr << "Impossible subtraction!\n";
				}
				break;
			}
			case tMUL:
			{
				if (TOStype == VT_DOUBLE)
				{
					bytecode->addInsn(BC_DMUL);
				}
				else if (TOStype == VT_INT)
				{
					bytecode->addInsn(BC_IMUL);
				}
				else
				{
					std::cerr << "Impossible multiplication!\n";
				}
				break;
			}			
			case tDIV:
			{
				if (TOStype == VT_DOUBLE)
				{
					bytecode->addInsn(BC_DDIV);
				}
				else if (TOStype == VT_INT)
				{
					bytecode->addInsn(BC_IDIV);
				}
				else
				{
					std::cerr << "Impossible dividing!\n";
				}
				break;
			}			
			case tMOD:
			{
				if (TOStype == VT_INT)
				{
					bytecode->addInsn(BC_IMOD);
				}
				else
				{
					std::cerr << "Impossible dividing by mod!\n";
				}
				break;
			}
			default:
			{
					std::cerr << "Impossible binary operation!\n";
			}
		}
}

void ByteCodeVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    node->operand()->visit(this);
		switch(node->kind())
		{
			case tNOT:
			{
				Label true_end(bytecode);
				Label end(bytecode);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->addBranch(BC_IFICMPNE, true_end);
				bytecode->addInsn(BC_ILOAD1);
				bytecode->addBranch(BC_JA, end);
				bytecode->bind(true_end);
				bytecode->addInsn(BC_ILOAD0);
				bytecode->bind(end);
				break;
			}
			case tSUB:
			{
				if (TOStype == VT_DOUBLE)
				{
					bytecode->addInsn(BC_DNEG);
				}
				else if (TOStype == VT_INT)
				{
					bytecode->addInsn(BC_INEG);
				}
				else
				{
					std::cerr << "Impossible negating!\n";
				}
				break;
			}
			default:
			{
					std::cerr << "Impossible unary operation!\n";
			}
		}
}

void ByteCodeVisitor::visitReturnNode(ReturnNode *node) {
		node->returnExpr()->visit(this);
    bytecode->addInsn(BC_RETURN);
}

void ByteCodeVisitor::visitNativeCallNode(NativeCallNode *node) {
    bytecode->addInsn(BC_CALLNATIVE);
    bytecode->addUInt16(code->makeNativeFunction(node->nativeName(), node->nativeSignature(), 0));
}
