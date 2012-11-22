
#include "ASTtoByteCodeTranslator.h"

namespace mathvm {

ASTtoByteCodeTranslator::ASTtoByteCodeTranslator(Code* code) : code(code){}

ASTtoByteCodeTranslator::~ASTtoByteCodeTranslator() {}

void ASTtoByteCodeTranslator::performTranslation(AstFunction* top) {
	createFunMap(top->scope());
}

void ASTtoByteCodeTranslator::createFunMap(Scope* scope){
    Scope::FunctionIterator funIt(scope);

    while(funIt.hasNext()) {
        AstFunction* fun = funIt.next();
        if (fun->node()->body()->nodeAt(0)->isNativeCallNode())
            fun->node()->visit(this);
        else{
        	BytecodeFunction* bcFun = new BytecodeFunction(fun);
        	code->addFunction(bcFun);
        	funMap.insert(std::make_pair(fun, bcFun));
        	funStack.push(bcFun);
        	fun->node()->visit(this);
        	funStack.pop();
        }
    }
}

uint16_t  ASTtoByteCodeTranslator::getNextVarID(){
	static uint16_t  id = 0;
	return id++;
}

void ASTtoByteCodeTranslator::createVarMap(Scope* scope){
	Scope::VarIterator varIt(scope);
	while(varIt.hasNext()) {
        AstVar* var = varIt.next();
        varMap[var] = getNextVarID();
    }
}

void ASTtoByteCodeTranslator::subWorkOnBinaryOp(Bytecode *bytecode){
	uint32_t mem1, mem2;
	mem1 = bytecode->current();
	bytecode->addInt16(0);
	bytecode->addInsn(BC_JA);
	mem2 = bytecode->current();
	bytecode->addInt16(0);
	bytecode->setInt16(mem1, bytecode->current() - mem1);
	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_ILOAD1);
	bytecode->setInt16(mem2, bytecode->current() - mem1);
	bytecode->addInsn(BC_JA);
	mem1 = bytecode->current();
	bytecode->addInt16(0);
	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_ILOAD0);
	bytecode->setInt16(mem1, bytecode->current() - mem1);
}

void ASTtoByteCodeTranslator::visitBinaryOpNode(BinaryOpNode* node) {
	node->right()->visit(this);
	node->left()->visit(this);

	uint32_t mem1, mem2;

	Bytecode *bytecode = funStack.top()->bytecode();

	switch(node->kind()) {
		case tEQ:
			bytecode->addInsn(BC_IFICMPE);
			subWorkOnBinaryOp(bytecode);
			break;
		case tNEQ:
			bytecode->addInsn(BC_IFICMPNE);
			subWorkOnBinaryOp(bytecode);
			break;
		case tGT:
			bytecode->addInsn(BC_IFICMPG);
			subWorkOnBinaryOp(bytecode);
			break;
		case tGE:
			bytecode->addInsn(BC_IFICMPGE);
			subWorkOnBinaryOp(bytecode);
			break;
		case tLT:
			bytecode->addInsn(BC_IFICMPL);
			subWorkOnBinaryOp(bytecode);
			break;
		case tLE:
			bytecode->addInsn(BC_IFICMPLE);
			subWorkOnBinaryOp(bytecode);
			break;
		case tADD:
			switch(varType){
				case VT_INT:
					bytecode->addInsn(BC_IADD);
					break;
				case VT_DOUBLE:
					bytecode->addInsn(BC_DADD);
					break;
				default:
					break;
			}
			break;
		case tSUB:
			switch(varType){
				case VT_INT:
					bytecode->addInsn(BC_ISUB);
					break;
				case VT_DOUBLE:
					bytecode->addInsn(BC_DSUB);
					break;
				default:
					break;
			}
			break;
		case tMUL:
			switch(varType){
				case VT_INT:
					bytecode->addInsn(BC_IMUL);
					break;
				case VT_DOUBLE:
					bytecode->addInsn(BC_DMUL);
					break;
				default:
					break;
			}
			break;
		case tDIV:
			switch(varType){
				case VT_INT:
					bytecode->addInsn(BC_IDIV);
					break;
				case VT_DOUBLE:
					bytecode->addInsn(BC_DDIV);
					break;
				default:
					break;
			}
			break;
		case tMOD:
			bytecode->addInsn(BC_IMOD);
			break;
		case tAND:
			bytecode->addInsn(BC_IMUL);
			bytecode->addInsn(BC_ILOAD0);
			bytecode->addInsn(BC_IFICMPNE); mem1 = bytecode->current();
			bytecode->addInt16(0);
			bytecode->addInsn(BC_POP);
			bytecode->addInsn(BC_POP);
			bytecode->addInsn(BC_ILOAD0);
			bytecode->addInsn(BC_JA);       mem2 = bytecode->current();
			bytecode->addInt16(0);
			bytecode->setInt16(mem1, bytecode->current() - mem1);
			bytecode->addInsn(BC_POP);
			bytecode->addInsn(BC_POP);
			bytecode->addInsn(BC_ILOAD1);
			bytecode->setInt16(mem2, bytecode->current() - mem2);
			break;
		case tOR:

			break;

		default:
			break;
	}
}

void ASTtoByteCodeTranslator::visitUnaryOpNode(UnaryOpNode* node) {
	Bytecode *bytecode = funStack.top()->bytecode();
	node->operand()->visit(this);
	switch(node->kind()) {
		case tSUB:
			switch(varType){
				case VT_INT:
					bytecode->addInsn(BC_INEG);
					break;
				case VT_DOUBLE:
					bytecode->addInsn(BC_DNEG);
					break;
				default:
					break;
			}
			break;
		case tNOT:

			break;
	    default:
	    	bytecode->addInsn(BC_INVALID);
	    	break;
	}
}

void ASTtoByteCodeTranslator::visitStringLiteralNode(StringLiteralNode* node) {
	Bytecode *bytecode = funStack.top()->bytecode();
	bytecode->addInsn(BC_SLOAD);
	bytecode->addInt16(code->makeStringConstant(node->literal()));
	varType = VT_STRING;
}

void ASTtoByteCodeTranslator::visitDoubleLiteralNode(DoubleLiteralNode* node) { // double number 2.3
	Bytecode *bytecode = funStack.top()->bytecode();
	bytecode->addInsn(BC_DLOAD);
	bytecode->addDouble(node->literal());
	varType = VT_DOUBLE;
}

void ASTtoByteCodeTranslator::visitIntLiteralNode(IntLiteralNode* node) {
	Bytecode *bytecode = funStack.top()->bytecode();
	bytecode->addInsn(BC_ILOAD);
	bytecode->addInt64(node->literal());
	varType = VT_INT;
}

void ASTtoByteCodeTranslator::visitLoadNode(LoadNode* node) {   // get value for var a into stack
	Bytecode *bytecode = funStack.top()->bytecode();

	varType = node->var()->type();
	switch(varType){
		case VT_INT:
			bytecode->addInsn(BC_LOADIVAR);
			break;
		case VT_DOUBLE:
			bytecode->addInsn(BC_LOADDVAR);
			break;
		default:
			break;
	}
	bytecode->addInt16(varMap[node->var()]);
}

void ASTtoByteCodeTranslator::visitStoreNode(StoreNode* node) {  // a = 3;
	node->visitChildren(this);
	Bytecode *bytecode = funStack.top()->bytecode();

	TokenKind op = node->op();
	varType = node->var()->type();

	switch(op){
		case tASSIGN:
			switch(varType){
				case VT_INT:
					bytecode->addInsn(BC_STOREIVAR);
					break;
				case VT_DOUBLE:
					bytecode->addInsn(BC_STOREDVAR);
					break;
				default:
					break;
			}
			bytecode->addUInt16(varMap[node->var()]);
			break;
		case tINCRSET:
			switch(varType){
				case VT_INT:
					bytecode->addInsn(BC_LOADIVAR);
					bytecode->addInt16(varMap[node->var()]);
					bytecode->addInsn(BC_IADD);
					bytecode->addInsn(BC_STOREIVAR);
					bytecode->addInt16(varMap[node->var()]);
					break;

				case VT_DOUBLE:
					bytecode->addInsn(BC_LOADDVAR);
					bytecode->addInt16(varMap[node->var()]);
					bytecode->addInsn(BC_DADD);
					bytecode->addInsn(BC_STOREDVAR);
					bytecode->addInt16(varMap[node->var()]);
					break;
				default:
					break;
			}
			break;
		case tDECRSET:
			switch(varType){
				case VT_INT:
					bytecode->addInsn(BC_LOADIVAR);
					bytecode->addInt16(varMap[node->var()]);
					bytecode->addInsn(BC_ISUB);
					bytecode->addInsn(BC_STOREIVAR);
					bytecode->addInt16(varMap[node->var()]);
					break;
				case VT_DOUBLE:
					bytecode->addInsn(BC_LOADDVAR);
					bytecode->addInt16(varMap[node->var()]);
					bytecode->addInsn(BC_DSUB);
					bytecode->addInsn(BC_STOREDVAR);
					bytecode->addInt16(varMap[node->var()]);
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}

void ASTtoByteCodeTranslator::visitForNode(ForNode* node) {
	Bytecode *bytecode = funStack.top()->bytecode();
	uint32_t mem1;

	BinaryOpNode *range = (BinaryOpNode*) node->inExpr();
	range->right()->visit(this);
	range->left()->visit(this);

	mem1 = bytecode->current();
	bytecode->addInsn(BC_STOREIVAR);
	bytecode->addInt16(varMap[node->var()]);

	node->body()->visit(this);

	bytecode->addInsn(BC_LOADIVAR);
	bytecode->addInt16(varMap[node->var()]);
	bytecode->addInsn(BC_ILOAD1);
	bytecode->addInsn(BC_IADD);
	bytecode->addInsn(BC_IFICMPLE);
	bytecode->addInt16(mem1 - bytecode->current());
}

void ASTtoByteCodeTranslator::visitWhileNode(WhileNode* node) {
	Bytecode *bytecode = funStack.top()->bytecode();
	uint32_t mem1, mem2, mem3;
	mem1 = bytecode->current();

	node->whileExpr()->visit(this);
	bytecode->addInsn(BC_ILOAD0);
	bytecode->addInsn(BC_IFICMPNE); mem2 = bytecode->current();
	bytecode->addInt16(0);
	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_JA);       mem3 = bytecode->current();
	bytecode->addInt16(0);          bytecode->setInt16(mem2, bytecode->current() - mem2);
	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_POP);
	node->loopBlock()->visit(this);
	bytecode->addInsn(BC_JA);       bytecode->setInt16(mem3, bytecode->current() - mem3);
	bytecode->addInt16(mem1 - bytecode->current());
}

void ASTtoByteCodeTranslator::visitIfNode(IfNode* node) {
	Bytecode *bytecode = funStack.top()->bytecode();
	uint32_t mem1, mem2, mem3;

	node->ifExpr()->visit(this);
	bytecode->addInsn(BC_ILOAD0);
	bytecode->addInsn(BC_IFICMPNE); mem1 = bytecode->current();
	bytecode->addInt16(0);
	bytecode->addInsn(BC_JA);      mem2 = bytecode->current();
	bytecode->addInt16(0);         bytecode->setInt16(mem1, bytecode->current() - mem1);
	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_POP);
	node->thenBlock()->visit(this);
	bytecode->addInsn(BC_JA);      mem3 = bytecode->current();
	bytecode->addInt16(0);         bytecode->setInt16(mem2, bytecode->current() - mem2);
	bytecode->addInsn(BC_POP);
	bytecode->addInsn(BC_POP);
	if (node->elseBlock()) {
		node->elseBlock()->visit(this);
	}
	bytecode->setInt16(mem3, bytecode->current() - mem3);
}

void ASTtoByteCodeTranslator::visitBlockNode(BlockNode* node) {
	scopeStack.push(node->scope());

	createVarMap(node->scope());
	createFunMap(node->scope());

	int nodesNumber = node->nodes();
	for(int i = 0; i < nodesNumber; ++i) {
		node->nodeAt(i)->visit(this);
	}

	scopeStack.pop();
}

void ASTtoByteCodeTranslator::visitFunctionNode(FunctionNode* node) {
	node->body()->visit(this);
}

void ASTtoByteCodeTranslator::visitReturnNode(ReturnNode* node) {
	node->returnExpr()->visit(this);
	Bytecode *bytecode = funStack.top()->bytecode();
	bytecode->addInsn(BC_RETURN);
}

void ASTtoByteCodeTranslator::visitCallNode(CallNode* node) {
	for(size_t i = 0; i < node->parametersNumber(); ++i) {
		node->parameterAt(i)->visit(this);
	}
	Bytecode *bytecode = funStack.top()->bytecode();
	bytecode->addInsn(BC_CALL);
	bytecode->addInt16(funMap[scopeStack.top()->lookupFunction(node->name(), true)]->id());
}

void ASTtoByteCodeTranslator::visitNativeCallNode(NativeCallNode* node) {
}

void ASTtoByteCodeTranslator::visitPrintNode(PrintNode* node) {
	int operandsNumber = node->operands();
	for (int i = 0; i < operandsNumber; ++i) {
		node->operandAt(i)->visit(this);
		Bytecode *bytecode = funStack.top()->bytecode();

		switch(varType){
			case VT_INT:
				bytecode->addInsn(BC_IPRINT);
				break;
			case VT_DOUBLE:
				bytecode->addInsn(BC_DPRINT);
				break;
			case VT_STRING:
				bytecode->addInsn(BC_SPRINT);
				break;
			default:
				break;
		}
	}
}


}

