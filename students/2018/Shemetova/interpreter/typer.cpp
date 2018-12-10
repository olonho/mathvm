#include "typer.h"

namespace mathvm {

    void Typer::visitIntLiteralNode(IntLiteralNode* node) {
        node->setInfo(new VarType(VT_INT));
    }

    void Typer::visitDoubleLiteralNode(DoubleLiteralNode* node) {
        node->setInfo(new VarType(VT_DOUBLE));
    }

    void Typer::visitStringLiteralNode(StringLiteralNode* node) {
        node->setInfo(new VarType(VT_STRING));
    }

    VarType Typer::get_type(AstNode* node) {
        return *((VarType *) node->info());
    }

    bool Typer::is_number(AstNode* node) {
        VarType type = get_type(node);
        return (type == VT_INT || type == VT_DOUBLE);
    }
    
    bool Typer::is_number(VarType type) {
        return (type == VT_INT || type == VT_DOUBLE);
    }

    bool Typer::two_are_integers(AstNode* nodeOne, AstNode* nodeTwo) {
        VarType typeOne = get_type(nodeOne);
        VarType typeTwo = get_type(nodeTwo);
        return (typeOne == VT_INT && typeTwo == VT_INT);
    }

    bool Typer::two_are_numbers(AstNode* nodeOne, AstNode* nodeTwo) {
        return (is_number(nodeOne) && is_number(nodeTwo));
    }

    bool Typer::equal(AstNode* nodeOne, AstNode* nodeTwo) {
        return nodeOne == nodeTwo;
    }

    VarType Typer::onlyIntegers(AstNode* nodeOne, AstNode* nodeTwo) {
        VarType result = VT_INVALID;
        if (two_are_integers(nodeOne, nodeTwo)) {
            return VT_INT;
        }
        return result;
    }

    VarType Typer::onlyNumbers(AstNode* nodeOne, AstNode* nodeTwo) {
        VarType result = VT_INVALID;
        if (two_are_numbers(nodeOne, nodeTwo)) {
            if (two_are_integers(nodeOne, nodeTwo)) {
                return VT_INT;
            } else {
                return VT_DOUBLE;
            }
        }
        return result;
    }

    VarType Typer::onlyEqual(AstNode* nodeOne, AstNode* nodeTwo) {
        VarType result = VT_INVALID;
        if (equal(nodeOne, nodeTwo)) {
            return get_type(nodeOne);
        }
        return result;
    }

    void Typer::visitUnaryOpNode(UnaryOpNode* node) {
        node->operand()->visit(this);
        VarType type = get_type(node->operand());
        switch (node->kind()) {
            case tSUB:
                if (is_number(node->operand())) {
                    node->setInfo(new VarType(type));
                } else {
                    throw std::runtime_error("Typechecking error: NEG");
                }
                break;
            case tNOT:
                if (type == VT_INT) {
                    node->setInfo(new VarType(type));
                } else {
                    throw std::runtime_error("Typechecking error: NOT");
                }
                break;
            default: assert(0);
        }
    }

    void Typer::visitBinaryOpNode(BinaryOpNode* node) {
        AstNode* left = node->left();
        AstNode* right = node->right();
        left->visit(this);
        right->visit(this);
        TokenKind token = node->kind();
        VarType type = VT_INVALID;
        switch (token) {
            case tADD:
            case tSUB:
            case tMUL:
            case tDIV:
                type = onlyNumbers(left, right);
                if (type == VT_INVALID) {
                    throw std::runtime_error("Typechecking error: Arithmetic not numbers");
                }
                break;
            case tMOD:
                type = onlyIntegers(left, right);
                if (type == VT_INVALID) {
                    throw std::runtime_error("Typechecking error: MOD not integers");
                }
                break;
            case tAND:
            case tOR:
                type = onlyIntegers(left, right);
                if (type == VT_INVALID) {
                    throw std::runtime_error("Typechecking error: OR, AND not integers");
                }
                break;
            case tEQ:
            case tNEQ:
                type = onlyIntegers(left, right);
                if (type == VT_INVALID) {
                    type = onlyEqual(left, right);
                }
                if (type == VT_INVALID) {
                    throw std::runtime_error("Typechecking error: EQ, NEQ");
                }
                break;
            case tGT:
            case tGE:
            case tLT:
            case tLE:
                type = onlyNumbers(left, right);
                if (type == VT_INVALID) {
                    throw std::runtime_error("Typechecking error: Compare not numbers");
                }
                break;
            case tAOR:
            case tAAND:
            case tAXOR:
                type = onlyIntegers(left, right);
                if (type == VT_INVALID) {
                    throw std::runtime_error("Typechecking error: Bitwise not integers");
                }
                break;
            case tRANGE:
                type = onlyIntegers(left, right);
                if (type == VT_INVALID) {
                    throw std::runtime_error("Typechecking error: RANGE not integers");
                }
                type = VT_VOID;
                break;
            default: assert(0);
        }
        node->setInfo(new VarType(type));
    }

    void Typer::visitLoadNode(LoadNode* node) {
        node->setInfo(new VarType(node->var()->type()));
    }

    void Typer::visitStoreNode(StoreNode* node) {
        VarType expected = node->var()->type();
        node->value()->visit(this);
        VarType actual = get_type(node->value());
        if (actual != expected) {
            if (is_number(expected) && is_number(actual)) {
                node->setInfo(new VarType(VT_VOID)); //we can cast it
                return;
            }
            throw std::runtime_error("Typechecking error: store var and value uncompatible!");
        } else {
            node->setInfo(new VarType(VT_VOID));
        }
    }

    void Typer::visitIfNode(IfNode* node) {
        node->ifExpr()->visit(this);
        if (get_type(node->ifExpr()) != VT_INT) {
            throw std::runtime_error("Typechecking error: if expr is not integer!");
        }
        node->thenBlock()->visit(this);
        if (node->elseBlock()) {
            node->elseBlock()->visit(this);
        }
        node->setInfo(new VarType(VT_VOID));
    }

    void Typer::visitWhileNode(WhileNode* node) {
        node->whileExpr()->visit(this);
        if (get_type(node->whileExpr()) != VT_INT) {
            throw std::runtime_error("Typechecking error: while expr is not integer!");
        }
        node->loopBlock()->visit(this);
        node->setInfo(new VarType(VT_VOID));
    }

    void Typer::visitForNode(ForNode* node) {
        node->inExpr()->visit(this);
        if (!node->inExpr()->isBinaryOpNode()) {
            throw std::runtime_error("Typechecking error: for in expr is not binaryop!");
        }
        if (node->inExpr()->asBinaryOpNode()->kind() != tRANGE) {
            throw std::runtime_error("Typechecking error: binaryOp should be range!");
        }
        node->body()->visit(this);
        node->setInfo(new VarType(VT_VOID));
    }

    void Typer::visitPrintNode(PrintNode* node) {
        node->visitChildren(this);
        for (uint32_t i = 0; i < node->operands(); ++i) {
            VarType type = get_type(node->operandAt(i));
            if (type == VT_INVALID || type == VT_VOID) {
                throw std::runtime_error("Typechecking error: uncompatible arg of print!");
            }
        }
        node->setInfo(new VarType(VT_VOID));
    }

    void Typer::visitCallNode(CallNode* node) {
        AstFunction* func = currentScope->lookupFunction(node->name());
        if (func) {
            uint32_t params = func->parametersNumber();
            if (params != node->parametersNumber()) {
                throw std::runtime_error("Typechecking error: uncompatible params number" + node->name());
            }
            node->visitChildren(this);
            for (uint32_t i = 0; i < params; ++i) {
                VarType actual = get_type(node->parameterAt(i));
                VarType expected = func->parameterType(i);
                if (actual != expected) {
                    if (is_number(expected) && is_number(actual)) {
                        continue;
                    } else {
                        throw std::runtime_error("Typechecking error: call params types uncompatible!");
                    }
                }
            }
        } else {
            throw std::runtime_error("Typechecking error: no such function in scope: " + node->name());
        }
        node->setInfo(new VarType(func->returnType()));
    }
    
    void Typer::visitFunctionNode(FunctionNode* node) {
        FunctionNode* prev = currentFunction;
        currentFunction = node;
        node->body()->visit(this);
        node->setInfo(new VarType(node->returnType()));
        currentFunction = prev;
    }
    
    void Typer::visitBlockNode(BlockNode* node) {
        Scope* prev = currentScope;
        currentScope = node->scope();
        Scope::VarIterator it = Scope::VarIterator(currentScope);
        while (it.hasNext()) {
            AstVar* var = it.next();
            var->setInfo(new VarType(var->type()));
        }
        Scope::FunctionIterator itf = Scope::FunctionIterator(currentScope);
        while (itf.hasNext()) {
            AstFunction* func = itf.next();
            func->node()->visit(this);
        }
        
        node->visitChildren(this);
        node->setInfo(new VarType(VT_VOID));
        currentScope = prev;
    }
    
    void Typer::visitReturnNode(ReturnNode* node) {
        VarType res = VT_INVALID;
        if (node->returnExpr()) {
            node->returnExpr()->visit(this);
            res = get_type(node->returnExpr());
        }
        else {
            res = VT_VOID;
        }
        if (currentFunction->returnType() != res) {
            if (is_number(res) && is_number(currentFunction->returnType())) {
                res = currentFunction->returnType();
            }
            else {          
            cout << "Typechecking error: returned result "
                    "uncompatible in function: " + currentFunction->name() << endl;
            throw std::runtime_error("Typechecking error: returned result "
                    "uncompatible in function: " + currentFunction->name());
            }
        }
        node->setInfo(new VarType(res));
    }

}
