#include "validating_visitor.h"
#include <algorithm>

namespace mathvm {

Status* ValidatingVisitor::checkProgram(AstFunction* top) {
    status = Status::Ok();
    checkAstFunction(top);
    return status;
}

bool ValidatingVisitor::isBinaryOk(VarType left, VarType right, TokenKind op) {
    static const TokenKind intOnly[7] = {tOR, tAND, tAAND, tAOR, tAXOR, tRANGE, tMOD};
    static const TokenKind numericOnly[10] = {tEQ, tNEQ, tGT, tGE, tLT, tLE, tADD, tSUB, tMUL, tDIV};

    if(left != right) {
        return false;
    }

    if(std::find(numericOnly, numericOnly + 10, op) != numericOnly + 10) {
        return (left == VT_DOUBLE || left == VT_INT);
    } 

    if(std::find(intOnly, intOnly + 7, op) != intOnly + 7) {
        return left == VT_INT;
    }

    return false;
}

void ValidatingVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    node->visitChildren(this); 
    if(status->isError()) return;

    NodeData* left = (NodeData*)node->left()->info();
    NodeData* right = (NodeData*)node->right()->info();

    if(!isBinaryOk(left->type, right->type, node->kind())) {
        fail(string("Operation \'") + tokenOp(node->kind()) + "\' is not allowed for " + 
                typeToName(left->type) + " and " + typeToName(right->type), node->position());
    }

    VarType resType;
    if(node->kind() == tEQ || node->kind() == tNEQ || node->kind() == tGT ||
            node->kind() == tGE || node->kind() == tLT || node->kind() == tLE) {
        resType = VT_INT;
    }
    else {
        if(node->kind() == tRANGE) {
            resType = VT_INVALID;
        }
        else {
            resType = left->type;
        }
    }

    node->setInfo(NodeData::getTyped(resType));
}

bool ValidatingVisitor::isUnaryOk(VarType type, TokenKind token) {
    switch(token) {
        case tNOT:
            return type == VT_INT;
        case tSUB:
            return type == VT_INT || type == VT_DOUBLE;
        default:
            return false;
    }
}

void ValidatingVisitor::visitUnaryOpNode(UnaryOpNode* node) {
    node->visitChildren(this);
    if(status->isError()) return;

    NodeData* exprData = (NodeData*) node->operand()->info(); 
    if(!isUnaryOk(exprData->type, node->kind())) {
        fail(string("Operation \'") + tokenOp(node->kind()) + "\' is not allowed for " +
                typeToName(exprData->type), node->position());
    }

    node->setInfo(exprData);
}

void ValidatingVisitor::visitStringLiteralNode(StringLiteralNode* node) {
    node->setInfo(NodeData::getTyped(VT_STRING));
}
void ValidatingVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    node->setInfo(NodeData::getTyped(VT_DOUBLE));
}
void ValidatingVisitor::visitIntLiteralNode(IntLiteralNode* node) {
    node->setInfo(NodeData::getTyped(VT_INT));
}

void ValidatingVisitor::visitLoadNode(LoadNode* node) {
    node->visitChildren(this);
    if(status->isError()) return;
    node->setInfo(NodeData::getTyped(node->var()->type()));
}

void ValidatingVisitor::visitStoreNode(StoreNode* node) {
    node->visitChildren(this);
    if(status->isError()) return;
    node->setInfo(NodeData::getTyped());

    NodeData* valData = (NodeData*) node->value()->info();
    if(valData->type != node->var()->type()) {
        typeFail(node->var()->type(), valData->type, node->value()->position());    
    }

    if(valData->type == VT_STRING && node->op() != tASSIGN) {
        fail("Wrong operation with strings", node->position());
    }
}

void ValidatingVisitor::visitForNode(ForNode* node) {
    node->visitChildren(this);
    if(status->isError()) return;
    node->setInfo(NodeData::getTyped());

    if(node->var()->type() != VT_INT) {
        fail("For index variable should be int", node->position());
    }

    if(!node->inExpr()->isBinaryOpNode() ||
            node->inExpr()->asBinaryOpNode()->kind() != tRANGE) {

        fail("For can iterate only over range", node->inExpr()->position());
    }
}

void ValidatingVisitor::visitWhileNode(WhileNode* node) {
    node->visitChildren(this);
    if(status->isError()) return;
    node->setInfo(NodeData::getTyped());

    expectType(node->whileExpr(), VT_INT);
}

void ValidatingVisitor::visitIfNode(IfNode* node) {
    node->visitChildren(this);
    if(status->isError()) return;
    node->setInfo(NodeData::getTyped());

    expectType(node->ifExpr(), VT_INT); 
}

void ValidatingVisitor::visitBlockNode(BlockNode* node) {
    if(status->isError()) return;
    node->setInfo(NodeData::getTyped());

    Scope* old = currentScope;
    currentScope = node->scope();

    checkScope(node->scope());
    node->visitChildren(this);

    currentScope = old;
}

void ValidatingVisitor::visitFunctionNode(FunctionNode* node) {
    if(status->isError()) return;
    for(uint32_t i = 0; i < node->parametersNumber(); ++i) {
        if(!isLvalueType(node->parameterType(i))) {
            fail("Wrong parameter type in function declaration", node->position());
            return;
        }
    }

    node->visitChildren(this);
    node->setInfo(NodeData::getTyped());
}

void ValidatingVisitor::visitReturnNode(ReturnNode* node) {
    node->visitChildren(this);
    if(status->isError()) return;
    node->setInfo(NodeData::getTyped());
    
    if(node->returnExpr() == 0) {
        if(currentFunction->returnType() != VT_VOID) {
            fail("Empty return statement in non-void function", node->position());
        }
    }
    else { 
        NodeData* exprData = (NodeData*)node->returnExpr()->info();
        if(!isLvalueType(exprData->type) ||
                currentFunction->returnType() != exprData->type) {

            fail("Attempt to return " + string(typeToName(exprData->type)) + " from " + 
                    typeToName(currentFunction->returnType()) + " function", node->position());
        }
    }
}

void ValidatingVisitor::visitCallNode(CallNode* node) {
    node->visitChildren(this);
    if(status->isError()) return;
    AstFunction* func = currentScope->lookupFunction(node->name());

    if(!func) {
        fail("Use of an undeclared function " + node->name(), node->position());
        return;
    }

    node->setInfo(NodeData::getTyped(func->returnType()));

    if(node->parametersNumber() != func->parametersNumber()) {
        fail("Wrong number of parameters for function " + func->name(), node->position());
        return;
    }

    for(uint32_t i = 0; i < node->parametersNumber(); ++i) {
        expectType(node->parameterAt(i), func->parameterType(i));
    }
}

void ValidatingVisitor::visitNativeCallNode(NativeCallNode* node) {
    node->visitChildren(this);
}

void ValidatingVisitor::visitPrintNode(PrintNode* node) {
    node->visitChildren(this);
    if(status->isError()) return;
    node->setInfo(NodeData::getTyped());

    for(uint32_t i = 0; i < node->operands(); ++i) {
        AstNode* operand = node->operandAt(i);
        NodeData* data = (NodeData*)operand->info();

        if(!isLvalueType(data->type)) {
            fail("Wrong operand for printing", operand->position());
        }
    }
}

void ValidatingVisitor::checkAstFunction(AstFunction* function) {
    if(status->isError()) return;
    AstFunction* old = currentFunction;
    currentFunction = function;
    function->node()->visit(this);
    currentFunction = old;
}

void ValidatingVisitor::checkAstVar(AstVar* var) {
    if(status->isError()) return;
    if(!isLvalueType(var->type())) {
        fail("Wrong type for variable " + var->name(), currentFunction->node()->position());
    }
}

void ValidatingVisitor::checkScope(Scope* scope) {
    if(status->isError()) return;
    Scope::VarIterator varIt(scope); 
    while(varIt.hasNext()) {
        checkAstVar(varIt.next());
    }

    Scope::FunctionIterator funIt(scope);
    while(funIt.hasNext()) {
        checkAstFunction(funIt.next());
    }
}

void ValidatingVisitor::expectType(AstNode* node, VarType type) {
    NodeData* data = (NodeData*)node->info();
    if(data->type != type) {
        typeFail(type, data->type, node->position());
    }
}

void ValidatingVisitor::fail(string const & msg, uint32_t position) {
    if(status->isOk()) {
        delete status;
        status = Status::Error(msg.c_str(), position);
    }
}

void ValidatingVisitor::typeFail(VarType expected, VarType found, uint32_t position) {
    string msg = string("Wrong type: ") + typeToName(expected) + 
        " expected but " + typeToName(found) + " found";

    fail(msg, position);
}

bool ValidatingVisitor::isLvalueType(VarType type) {
    return type == VT_INT || type == VT_STRING || type == VT_DOUBLE;
}

NodeData NodeData::node_int(VT_INT);
NodeData NodeData::node_double(VT_DOUBLE);
NodeData NodeData::node_string(VT_STRING);
NodeData NodeData::node_void(VT_VOID);
NodeData NodeData::node_invalid(VT_INVALID);
}
