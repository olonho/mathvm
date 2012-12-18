#include "Ast2BytecodeVisitor.h"
#include <sstream>

namespace mathvm
{

Ast2BytecodeVisitor::Ast2BytecodeVisitor(Code* code)
: _code(code)
, _bytecode(0)
, _bcHelper(_code)
, _currentFunc(0)
{}

bool generateCompare(const TokenKind& kind, BytecodeHelper& bc) {
	Label ELSE;
	Label END;

	switch(kind) {
		case tEQ:  bc.ifcmpE(ELSE); break;
		case tLT:  bc.ifcmpL(ELSE); break;
		case tLE:  bc.ifcmpLE(ELSE); break;
		case tGT:  bc.ifcmpG(ELSE); break;
		case tGE:  bc.ifcmpGE(ELSE); break;
		case tNEQ: bc.cmp(); return true;
		default: return false;
	}

	bc
	.load(false)
	.jmp(END)

	.setLabel(ELSE)
	.load(true)

	.setLabel(END);

	return true;
}

void generateLogical(BytecodeHelper& bc, const TokenKind& kind, AstVisitor* visitor, AstNode* left, AstNode* right) {
	Label END;
	Label LAZY_END;
	left->visit(visitor);
	bc
	.load(false);
	if (kind == tOR)
		bc.ifcmpNE(LAZY_END);
	else
		bc.ifcmpE(LAZY_END);

	right->visit(visitor);
	bc
	.jmp(END)
	.setLabel(LAZY_END)
	.load(kind == tOR)
	.setLabel(END);
}

void Ast2BytecodeVisitor::visitBinaryOpNode(BinaryOpNode* node) {
	if (node->kind() == tAND || node->kind() == tOR) {
	    return generateLogical(bc(), node->kind(), this, node->left(), node->right());
	}

    node->left()->visit(this);
    node->right()->visit(this);
    if (node->kind() == tADD) {
    	bc().add();
    } else if (node->kind() == tSUB) {
    	bc().sub();
    } else if (node->kind() == tMUL) {
    	bc().mul();
    } else if (node->kind() == tDIV) {
    	bc().div();
    } else if (!generateCompare(node->kind(), bc())) {
    	assert("Unknown operation" == 0);
    }
}

void Ast2BytecodeVisitor::visitUnaryOpNode(UnaryOpNode* node) {
	if (node->kind() == tSUB ) {
        if (node->operand()->isIntLiteralNode()) {
            int64_t value = - ((IntLiteralNode*) node->operand())->literal();
            bc().load(value);
            return;
        } else if (node->operand()->isDoubleLiteralNode()) {
            double value = - ((DoubleLiteralNode*) node->operand())->literal();
            bc().load(value);
            return;
        } else {
            node->operand()->visit(this);
			bc().neg();
        }
    } else {
    	node->operand()->visit(this);
    	if (node->kind() == tNOT) {
    		bc().inot();
    	}
    }
}

void Ast2BytecodeVisitor::visitStringLiteralNode(StringLiteralNode* node) {
    bc().load(node->literal());
}
void Ast2BytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    bc().load(node->literal());
}
void Ast2BytecodeVisitor::visitIntLiteralNode(IntLiteralNode* node) {
    bc().load(node->literal());
}

void Ast2BytecodeVisitor::visitLoadNode(LoadNode* node) {
     bc().loadvar(node->var());
}
void Ast2BytecodeVisitor::visitStoreNode(StoreNode* node) {
	node->visitChildren(this);
	if (node->op() != tASSIGN) {
		bc().loadvar(node->var());
		if (node->op() == tINCRSET) {
			bc().add();
		} else if (node->op() == tDECRSET) {
			bc().sub();
		} else {
			assert("Bad operation" == 0);
		}
	}
    bc().storevar(node->var());
}

//Ast2BytecodeVisitor::scopeId_t Ast2BytecodeVisitor::scope2id(Scope* scope) {
//	scope2id_t::iterator it = _scope2id.find(scope);
//	if (it == _scope2id.end()) {
//		it = _scope2id.insert(std::make_pair(scope, (Ast2BytecodeVisitor::scopeId_t) _scope2id.size())).first;
//	}
//	return it->second;
//}

void Ast2BytecodeVisitor::initScope(Scope* scope) {
	assert(scope);
//     Scope::VarIterator varIt(scope);
//     while (varIt.hasNext()) {
//         AstVar* var = varIt.next();
//     }

    Scope::FunctionIterator funcIt(scope);
    while (funcIt.hasNext()) {
    	AstFunction* prevFunc = _currentFunc;
    	_currentFunc = funcIt.next();

//    	scopeId_t scopeId = scope2id(scope);

        BytecodeFunction *bytecodeFunction = new BytecodeFunction(_currentFunc);
//        bytecodeFunction->setScopeId(scopeId);

        _code->addFunction(bytecodeFunction);


        Bytecode* prevBC = _bytecode;
        _bytecode = bytecodeFunction->bytecode();
        _currentFunc->node()->visit(this);
        _bytecode = prevBC;
        _currentFunc = prevFunc;
    }
}

void Ast2BytecodeVisitor::start(AstFunction* top) {
	_currentFunc = top;
    BytecodeFunction *bytecodeFunction = new BytecodeFunction(top);
    _code->addFunction(bytecodeFunction);
    _bytecode = bytecodeFunction->bytecode();

    top->node()->visit(this);
}

void Ast2BytecodeVisitor::visitBlockNode(BlockNode* node) {
	bc().enterScope(node->scope());

    initScope(node->scope());
    node->visitChildren(this);

    bc().exitScope();
}

void Ast2BytecodeVisitor::visitForNode(ForNode* node) {
	Label LOOP;
	Label END;

	BinaryOpNode* range = (BinaryOpNode*) node->inExpr();
	assert(range->isBinaryOpNode() && range->kind() == tRANGE);

	range->right()->visit(this);
	range->left()->visit(this);
	bc()
	.storevar(node->var())
	.loadvar(node->var())
	.ifcmpL(END)
	.setLabel(LOOP);

	node->body()->visit(this);

	range->right()->visit(this);
	bc()
	.loadvar(node->var())
	.ifcmpGE(LOOP)
	.setLabel(END);
}

void Ast2BytecodeVisitor::visitWhileNode(WhileNode* node) {
	Label LOOP;
	Label END;

	node->whileExpr()->visit(this);

	bc()
	.load(false)
	.ifcmpE(END)
	.setLabel(LOOP);

    node->loopBlock()->visit(this);

	node->whileExpr()->visit(this);
    bc()
	.load(false)
	.ifcmpE(LOOP)
	.setLabel(END);
}

void Ast2BytecodeVisitor::visitIfNode(IfNode* node) {
	Label ELSE;
	Label END;

	node->ifExpr()->visit(this);
	bc()
	.load(false)
	.ifcmpE(ELSE);

	node->thenBlock()->visit(this);
	bc()
	.jmp(END)
	.setLabel(ELSE);

    if (node->elseBlock()) {
       node->elseBlock()->visit(this);
    }

    bc().setLabel(END);
}

void Ast2BytecodeVisitor::visitFunctionNode(FunctionNode* node) {
	assert(node);
	uint16_t functionId = _code->functionByName(node->name())->id();
	bc().enterContext(functionId);

	for (size_t i = node->parametersNumber(); i > 0; --i) {
			bc().pushType(node->parameterType(i - 1));
	}

	Scope* scope = node->body()->scope();
	assert(scope);

	//fixme ???
	bc().enterScope(scope);
	for (size_t i = 0; i < node->parametersNumber(); ++i) {
		scope->declareVariable(node->parameterName(i), node->parameterType(i));
		AstVar* param = scope->lookupVariable(node->parameterName(i), false);
		bc().storevar(param);
	}

	node->visitChildren(this);

	bc().exitContext(functionId);
}

void Ast2BytecodeVisitor::visitReturnNode(ReturnNode* node) {
	if (node->returnExpr())
		node->returnExpr()->visit(this);
    bc().ret(_currentFunc->returnType());
}

void Ast2BytecodeVisitor::visitNativeCallNode(NativeCallNode* node) {
    // _out << "native '" << node->nativeName() << "';";
}

void Ast2BytecodeVisitor::visitCallNode(CallNode* node) {
	node->visitChildren(this);
	TranslatedFunction* func = _code->functionByName(node->name());
	assert(func != 0);
	bc().call(func->id());
}

void Ast2BytecodeVisitor::visitPrintNode(PrintNode* node) {
    for (size_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        bc().print();
    }
}

}
