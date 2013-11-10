
#include "CodeTranslator.h"
#include "mathvm.h"

namespace mathvm {

CodeTranslator::CodeTranslator(Code* code) : _code(code), myCurrentFunction(0), myCurrentContext(0) {
	_varScopes.push(0);
}

CodeTranslator::~CodeTranslator() {
}

void CodeTranslator::translate(AstFunction* top, AstTypeMap *typeMap) {
    if(typeMap == 0)
        throw ExceptionInfo("CodeTranslator.translate: typeMap pointer equals null");
    
    myTypeMap = typeMap;
    processFunction(top);
}


void CodeTranslator::processFunction(AstFunction* ast_function) {

    BytecodeFunction* parentFunction = myCurrentFunction;
	myCurrentFunction = new BytecodeFunction(ast_function);

	uint16_t id = _code->addFunction(myCurrentFunction);
    
    VarToContext* parentContext = myCurrentContext;
    myCurrentContext = new VarToContext(myCurrentFunction->id(), parentContext);
    
	Scope::VarIterator arg_it(ast_function->scope());
        
	while (arg_it.hasNext()) {
		AstVar* var = arg_it.next();
		uint16_t id = myCurrentContext->add(var);
		storeLocal(var->type(), id);
	};

	ast_function->node()->visit(this);
    
	delete myCurrentContext;
	myCurrentContext = parentContext;
    
	if(id == 0) myCurrentFunction->bytecode()->addInsn(BC_STOP);
    myCurrentFunction = parentFunction;
    //cout << "----END PROCESS_FUNC Node----" << endl;
}




void CodeTranslator::visitBinaryOpNode(BinaryOpNode* node) {    
    VarType type = getType(node);
    VarType left = getType(node->left());
    VarType right = getType(node->right());
    
    if(type == VT_INVALID)
        throw ExceptionInfo() << "Invalid binary operation: " << "\n" 
                              << "leftType: " << left << "\n" 
                              << "rightType: " << right << "\n"
                              << "operation: " << node->kind() << "\n";
    int opClass = 0;
    switch (node->kind()) {
        case tOR: 
        case tAND: 
        case tEQ:
        case tNEQ: 
        case tGT:
        case tGE:
        case tLT:
        case tLE:    
        case tMOD: opClass = 0; break;
        case tDIV:  
        case tADD:
        case tSUB:
        case tMUL: opClass = 1; break; 
        default: opClass = 2;
    } 
    //if(node->kind() == tDIV) 
    //    cout << "Division: left = " << left << " / right = " << right <<
    //           " = result ("<< type << ")\n";
                ; 
    if(opClass == 1) {    
       node->right()->visit(this);
       if(right!= left && right == VT_INT) bytecode()->addInsn(BC_I2D);          
       node->left()->visit(this);
       if(right!= left && left == VT_INT) bytecode()->addInsn(BC_I2D);
       
       visitCalculativeOp(node);          
    }
    
    if(opClass == 0) {    
       node->right()->visit(this);
       if(right != VT_INT) bytecode()->addInsn(BC_D2I);         
       node->left()->visit(this);
       if(left != VT_INT) bytecode()->addInsn(BC_D2I);
       
       if(node->kind() == tOR) visitOrOp(node);
       else if(node->kind() == tAND) visitAndOp(node);
       else visitComparativeOp(node);
       
    } 
}
//-------visitBinaryOpNode support methods------------
void CodeTranslator::visitAndOp(BinaryOpNode* node) {
    Label ifFalse(bytecode());
    Label lazyEnd(bytecode());
    Label ifTrue(bytecode());
    Label end(bytecode());

    bytecode()->addInsn(BC_ILOAD0);
    bytecode()->addBranch(BC_IFICMPE, lazyEnd); // если левый аргкмент 0
    bytecode()->addInsn(BC_ILOAD0);
    bytecode()->addBranch(BC_IFICMPE, ifFalse); // если правый 0
    
    bytecode()->addBranch(BC_JA, ifTrue); // 1 && 1

    bytecode()->bind(lazyEnd);
    bytecode()->addInsn(BC_POP); // удаляем со стека правый аргумент 
    bytecode()->bind(ifFalse);
    bytecode()->addInsn(BC_ILOAD0); // кладем на стек false
    bytecode()->addBranch(BC_JA, end);
    
    bytecode()->bind(ifTrue);
    bytecode()->addInsn(BC_ILOAD1); // кладем на стек true
    bytecode()->bind(end);

}

void CodeTranslator::visitOrOp(BinaryOpNode* node) {
    Label ifFalse(bytecode());
    Label lazyEnd(bytecode());
    Label ifTrue(bytecode());
    Label end(bytecode());

    bytecode()->addInsn(BC_ILOAD0);
    bytecode()->addBranch(BC_IFICMPNE, lazyEnd); // если левый аргкмент true
    bytecode()->addInsn(BC_ILOAD0);
    bytecode()->addBranch(BC_IFICMPNE, ifTrue); // если правый true
    
    bytecode()->addBranch(BC_JA, ifFalse); // 0 && 0

    bytecode()->bind(lazyEnd);
    bytecode()->addInsn(BC_POP); // удаляем со стека правый аргумент 
    bytecode()->bind(ifTrue);
    bytecode()->addInsn(BC_ILOAD1); // кладем на стек false
    bytecode()->addBranch(BC_JA, end);
    
    bytecode()->bind(ifFalse);
    bytecode()->addInsn(BC_ILOAD0); // кладем на стек true
    bytecode()->bind(end);    
}

void CodeTranslator::visitCalculativeOp(BinaryOpNode* node) {
    VarType type = getType(node);
    switch (node->kind()) {
        case tDIV: {type == VT_DOUBLE ? bytecode()->addInsn(BC_DDIV): bytecode()->addInsn(BC_IDIV); break;}
        case tADD: {type == VT_DOUBLE ? bytecode()->addInsn(BC_DADD): bytecode()->addInsn(BC_IADD); break;}
        case tSUB: {type == VT_DOUBLE ? bytecode()->addInsn(BC_DSUB): bytecode()->addInsn(BC_ISUB); break;}
        case tMUL: {type == VT_DOUBLE ? bytecode()->addInsn(BC_DMUL): bytecode()->addInsn(BC_IMUL); break;}
        default: throw ExceptionInfo("visitCalculativeOp called for kind: ") << node->kind();
    }
}

void CodeTranslator::visitComparativeOp(BinaryOpNode* node) {
    Instruction inst;
    Label ifTrue(bytecode());
    Label end(bytecode());
    
    switch (node->kind()) {
        case tEQ:   inst = BC_IFICMPE; break;
        case tNEQ:  inst = BC_IFICMPNE; break;
        case tGT:   inst = BC_IFICMPG; break;
        case tGE:   inst = BC_IFICMPGE; break;
        case tLT:   inst = BC_IFICMPL; break;
        case tLE:   inst = BC_IFICMPLE; break;
        
        case tMOD:  bytecode()->addInsn(BC_IMOD); return;
        default: throw ExceptionInfo("visitComparativeOp called for kind: ") << node->kind();
    };
    
    bytecode()->addBranch(inst, ifTrue);
    
    bytecode()->addInsn(BC_ILOAD0);    // otherwise
    bytecode()->addBranch(BC_JA, end);
    
    bytecode()->bind(ifTrue);
    bytecode()->addInsn(BC_ILOAD1);    // if true
    
    bytecode()->bind(end);
}
//-------/visitBinaryOpNode support methods------------



void CodeTranslator::visitUnaryOpNode(UnaryOpNode* node) {
    node->operand()->visit(this);
    
    if(getType(node) == VT_INVALID) 
        throw ExceptionInfo() << "UnaryOpNode exception:" 
                << "\nkind = " << node->kind() 
                << "\noperand type = " << getType(node->operand()) << "\n";
    
    
    if(node->kind() == tNOT) {     
        Label ifNotZero(bytecode());
        Label end(bytecode());

        bytecode()->addInsn(BC_ILOAD0);
        bytecode()->addBranch(BC_IFICMPNE, ifNotZero);
        bytecode()->addInsn(BC_ILOAD1);
        bytecode()->addBranch(BC_JA, end);

        bytecode()->bind(ifNotZero);
        bytecode()->addInsn(BC_ILOAD0);
        bytecode()->bind(end);
        
    }else if (node->kind() == tSUB) {
        switch (getType(node->operand())) {
			case VT_INT: bytecode()->addInsn(BC_INEG); break;
			case VT_DOUBLE: bytecode()->addInsn(BC_DNEG); break;
			default: break;
		}
    } else {
       throw ExceptionInfo() << "unexpected operation kind: " << node->kind(); 
    }
}


void CodeTranslator::visitStringLiteralNode(StringLiteralNode* node) {
	uint16_t id = _code->makeStringConstant(node->literal());
	bytecode()->addInsn(BC_SLOAD);
	bytecode()->addUInt16(id);
}

void CodeTranslator::visitDoubleLiteralNode(DoubleLiteralNode* node) {
	bytecode()->addInsn(BC_DLOAD);
	bytecode()->addDouble(node->literal());
}

void CodeTranslator::visitIntLiteralNode(IntLiteralNode* node) {
    //cout << "----Start IntLiteral Node----" << endl;
	bytecode()->addInsn(BC_ILOAD);
	bytecode()->addInt64(node->literal());
    //bytecode()->dump(cout);
    //cout << "----END IntLiteral Node----" << endl;
}

void CodeTranslator::visitLoadNode(LoadNode* node) {
	load(node->var());
}

void CodeTranslator::visitStoreNode(StoreNode* node) {
	VarType type = node->var()->type();
	if (type != getType(node->value())) 
		throw ExceptionInfo("StoreNode: casts not allowed");
    
	node->value()->visit(this);
    
	if (node->op() == tINCRSET ) {
        if(type != VT_INT && type!= VT_DOUBLE) throw ExceptionInfo("tINCREASE for type(") << type << ")"; 
		load(node->var());
		bytecode()->addInsn( type ==VT_INT ? BC_IADD : BC_DADD);
	} else if (node->op() == tDECRSET) {
        if(type != VT_INT && type!= VT_DOUBLE) throw ExceptionInfo("tDECRSET for type(") << type << ")";
		load(node->var());
		bytecode()->addInsn(type ==VT_INT ? BC_ISUB : BC_DSUB);
	}

	store(node->var());
}


void CodeTranslator::visitForNode(ForNode* node) {
    if(node->inExpr()->isBinaryOpNode() == false ) 
        throw ExceptionInfo("ForNode: range is ont a Binary Node\n");
    BinaryOpNode* range = (BinaryOpNode*) node->inExpr();
    if(range->kind() != tRANGE ) 
        throw ExceptionInfo("ForNode: invalid range kind\n");
    
    AstNode *left = range->left();
    AstNode *right = range->right();
    if(getType(left) != VT_INT || getType(right) != VT_INT)
        throw ExceptionInfo("ForNode: Range borders have invalid types: ") 
                << getType(left) << " : " << getType(right);
	
    // store for variable
	left->visit(this);
	const AstVar* var = node->var() ;
	store(var);

	Label forHead(bytecode());
    
    bytecode()->bind(forHead);
	right->visit(this); // считаем условие
	load(var);
    
    Label forEnd(bytecode()); 
	bytecode()->addBranch(BC_IFICMPG, forEnd); // проверяем на равенство

	node->body()->visit(this);
    
    // инкрементируем
	load(var);
	bytecode()->addInsn(BC_ILOAD1);
	bytecode()->addInsn(BC_IADD);
	store(var);
    
    // взращаемся к условию
	bytecode()->addBranch(BC_JA, forHead);
	bytecode()->bind(forEnd);
}


void CodeTranslator::visitWhileNode(WhileNode* node) {
    //cout << "Start while node" << endl;
    Label loopEntry(bytecode(), bytecode()->current());
    Label loopExit(bytecode());

    bytecode()->addInsn(BC_ILOAD0);
    node->whileExpr()->visit(this);
    
    VarType exprType = getType(node->whileExpr());
    if(!handleConditionExpression(exprType)){
        throw (ExceptionInfo() << "whileExpr() has incorrect type(" << exprType << ")");
    }
    
    bytecode()->addBranch(BC_IFICMPE, loopExit);
    

    node->loopBlock()->visit(this);
    bytecode()->addBranch(BC_JA, loopEntry);

    bytecode()->bind(loopExit);
    //cout << "End while node" << endl;
}

void CodeTranslator::visitIfNode(IfNode* node) {
   // cout << "----Start IF Node----" << endl;
    bytecode()->addInsn(BC_ILOAD0);
    node->ifExpr()->visit(this);
    
    VarType exprType = getType(node->ifExpr()); 
    if(!handleConditionExpression(exprType)){
        throw (ExceptionInfo() << "ifExpr() has incorrect type(" << exprType << ")");
    }

    Label elseBranch(bytecode());
    Label afterIf(bytecode());
    
    //if equals false jump to else;
    bytecode()->addBranch(BC_IFICMPE, elseBranch);

    node->thenBlock()->visit(this);
    bytecode()->addBranch(BC_JA, afterIf);
    

    bytecode()->bind(elseBranch);
    if (node->elseBlock()) {
        node->elseBlock()->visit(this);
    }

    bytecode()->bind(afterIf);
    //bytecode()->dump(cout);
   // cout << "------------End IF Node--------------" << endl;
}


void CodeTranslator::visitBlockNode(BlockNode* node) {
    //cout << "Start BLOCK Node" << endl;
	Scope* scope = node->scope();
	Scope::VarIterator var_it(scope);
    
	while (var_it.hasNext()) {
		myCurrentFunction->setLocalsNumber(myCurrentFunction->localsNumber() + 1);
		myCurrentContext->add(var_it.next());
	};

	Scope::FunctionIterator func_it(scope);
	while (func_it.hasNext()) {
		processFunction(func_it.next());
	}

	node->visitChildren(this);
    
    //cout << "END BLOCK Node" << endl;
}

void CodeTranslator::visitFunctionNode(FunctionNode* node) {
	node->body()->visit(this);

}

void CodeTranslator::visitReturnNode(ReturnNode* node) {
    //cout << "----Start RETURN Node----" << endl;

	node->visitChildren(this);
	bytecode()->addInsn(BC_RETURN);
    //cout << "----END RETURN Node----" << endl;
}

void CodeTranslator::visitCallNode(CallNode* node) {
	TranslatedFunction* function = _code->functionByName(node->name());
    
    if(function == 0) throw ExceptionInfo("CallNode: unknown function(") << node->name() << ") call";
	
	for (size_t i = 0; i < node->parametersNumber(); ++i) {
		size_t index = node->parametersNumber() - i;
		node->parameterAt(index - 1)->visit(this);
	}

	bytecode()->addInsn(BC_CALL);
	bytecode()->addUInt16(function->id());
}

void CodeTranslator::visitNativeCallNode(NativeCallNode* node) {
}

void CodeTranslator::visitPrintNode(PrintNode* node) {
    VarType currentType;
    AstNode* current;
    
	for (uint32_t i = 0; i < node->operands(); ++i) {
		current = node->operandAt(i);
        currentType = getType(current);
        current->visit(this);
        
		switch (currentType) {
			case VT_INT:
                bytecode()->addInsn(BC_IPRINT); break;
            case VT_DOUBLE:
                bytecode()->addInsn(BC_DPRINT); break;
			case VT_STRING:
                bytecode()->addInsn(BC_SPRINT); break;
			default:
                throw ExceptionInfo() <<"PrintNode: operand " << i << " has type: " << currentType;
		}
	}

}

void CodeTranslator::load(const AstVar* var) {
	ContextInfo  info (myCurrentContext->get(var));
   
    const uint16_t context = info.context;
    const uint16_t index = info.id;
    
	if (context == myCurrentFunction->id()) {
		loadLocal(var->type(), index);
        return;
	}
    
    switch (var->type()) {
        case VT_INT: 
            bytecode()->addInsn(BC_LOADCTXIVAR); break;
        case VT_DOUBLE:
            bytecode()->addInsn(BC_LOADCTXDVAR); break;
        case VT_STRING:		
            bytecode()->addInsn(BC_LOADCTXSVAR); break;
        default:
            throw ExceptionInfo("try to load var with type:") << var->type();
    }
    
    bytecode()->addInt16(context);
    bytecode()->addUInt16(index);

}

void CodeTranslator::loadLocal(VarType type, uint16_t id) {
    switch (type) {
        case VT_INT:
            bytecode()->addInsn(BC_LOADIVAR); break;
        case VT_DOUBLE:	
            bytecode()->addInsn(BC_LOADDVAR); break;    
        case VT_STRING:
            bytecode()->addInsn(BC_LOADSVAR); break;
        default:
             throw ExceptionInfo("try to load var with type:") << type;
    }
    bytecode()->addUInt16(id);
}

void CodeTranslator::storeLocal(VarType type, uint16_t id) {
		switch (type) {
			case VT_INT: bytecode()->addInsn(BC_STOREIVAR); break;
            case VT_DOUBLE:	bytecode()->addInsn(BC_STOREDVAR); break;
			case VT_STRING:	bytecode()->addInsn(BC_STORESVAR); break;
            default:
                throw ExceptionInfo("try to store var vis type:") << type; 
		}
        
		bytecode()->addUInt16(id);
}

void CodeTranslator::store(const AstVar* var) {
	ContextInfo info(myCurrentContext->get(var));
    uint16_t varContext = info.context;
    uint16_t varIndex = info.id;
    
	if (varContext == myCurrentFunction->id()) {
		storeLocal(var->type(), varIndex);
        return;
    }
	
    switch (var->type()) {    
        case VT_INT: bytecode()->addInsn(BC_STORECTXIVAR); break;
        case VT_DOUBLE: bytecode()->addInsn(BC_STORECTXDVAR); break;
        case VT_STRING: bytecode()->addInsn(BC_STORECTXSVAR); break;
        default: 
            throw ExceptionInfo("try to store var with type:") << var->type(); 
    }
    
    bytecode()->addInt16(varContext);
    bytecode()->addUInt16(varIndex);
}

}

