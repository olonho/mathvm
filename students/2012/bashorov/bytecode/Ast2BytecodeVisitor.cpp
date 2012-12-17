#include "Ast2BytecodeVisitor.h"
#include <functional>

namespace mathvm
{

Ast2BytecodeVisitor::Ast2BytecodeVisitor(Code* code)
: _code(code)
, _bytecode(0)
, _bcHelper(_code)
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

void Ast2BytecodeVisitor::initScope(Scope* scope) {
//     Scope::VarIterator varIt(scope);
//     while (varIt.hasNext()) {
//         AstVar* var = varIt.next();
//     }

    Scope::FunctionIterator funcIt(scope);
    while (funcIt.hasNext()) {
        AstFunction* func = funcIt.next();

        // BytecodeFunction *bytecodeFunction = new BytecodeFunction(func);
        // _code->addFunction(bytecodeFunction);
        // _bytecode = bytecodeFunction->bytecode();

        func->node()->visit(this);

        // _functions.insert(make_pair(fun, bytecode_function));
    }
}

void Ast2BytecodeVisitor::start(AstFunction* top) {
    BytecodeFunction *bytecodeFunction = new BytecodeFunction(top);
    _code->addFunction(bytecodeFunction);
    _bytecode = bytecodeFunction->bytecode();

    top->node()->visit(this);
}

void Ast2BytecodeVisitor::visitBlockNode(BlockNode* node) {
    initScope(node->scope());
    node->visitChildren(this);
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
//node->returnType()
//node->signature();
//node->name()

    // if (node->body()->nodes() && node->body()->nodeAt(0)->isNativeCallNode())
    //     node->body()->nodeAt(0)->visit(this);

       node->visitChildren(this);
}

void Ast2BytecodeVisitor::visitReturnNode(ReturnNode* node) {
    node->returnExpr()->visit(this);
    //todo convert return type
    bc().ret();
}

void Ast2BytecodeVisitor::visitNativeCallNode(NativeCallNode* node) {
    // _out << "native '" << node->nativeName() << "';";
}

void Ast2BytecodeVisitor::visitCallNode(CallNode* node) {
    // printSignature(node->name(), node->parametersNumber(),
    //     std::bind1st(std::mem_fun(&CallNode::parameterAt), node),
    //     std::bind2nd(std::mem_fun(&AstNode::visit), this));
}

void Ast2BytecodeVisitor::visitPrintNode(PrintNode* node) {
    for (size_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        bc().print();
    }
}

}
