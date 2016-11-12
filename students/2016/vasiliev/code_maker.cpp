#include "code_maker.h"
#include "ast.h"

using namespace mathvm;

code_maker::code_maker(Code* code) : code(code), top_type(VT_INVALID) {}

code_maker::code_maker(Code* code, Bytecode* bc) : code(code), top_type(VT_INVALID), bc(bc) {}

code_maker::~code_maker() {}

void code_maker::visitForNode(mathvm::ForNode *node) {
    BinaryOpNode *range = node->inExpr()->asBinaryOpNode();
    if (!range || range->kind() != tRANGE) throw "Range (visitForNode)";

    range->left()->visit(this);
    store(node->var()->name());

    Label start(bc);
    Label end(bc);

    start.bind(bc->current());

    load(node->var()->name());
    range->right()->visit(this);
    bc->addBranch(BC_IFICMPG, end);

    node->body()->visit(this);
    load(node->var()->name());
    bc->addInsn(BC_ILOAD1);
    bc->addInsn(BC_IADD);
    store(node->var()->name());

    bc->addBranch(BC_JA, start);

    end.bind(bc->current());

    top_type = VT_VOID;
}

void code_maker::visitPrintNode(mathvm::PrintNode *node) {
    for (uint32_t i = 0; i < node->operands(); ++i) {
        print(node->operandAt(i));
    }
}

void code_maker::visitLoadNode(mathvm::LoadNode *node) {
    load(node->var()->name());
}

void code_maker::visitIfNode(mathvm::IfNode *node) {
    Label else_l(bc);
    Label end_l(bc);
    node->ifExpr()->visit(this);
    if (top_type != VT_INT) throw "Condition should be int (visitIfNode)";
    bc->add(BC_ILOAD0);
    bc->addBranch(BC_IFICMPE, else_l);
    node->thenBlock()->visit(this);
    bc->addBranch(BC_JA, end_l);
    else_l.bind(bc->current());
    if (node->elseBlock()) node->elseBlock()->visit(this);
    end_l.bind(bc->current());
    top_type = VT_VOID;
}

void code_maker::visitCallNode(mathvm::CallNode *node) {
    for (size_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
    }
    bc->addInsn(BC_CALL);
    bc->addUInt16(code->functionByName(node->name())->id());

}

void code_maker::visitDoubleLiteralNode(mathvm::DoubleLiteralNode *node) {
    bc->addInsn(BC_DLOAD);
    bc->addDouble(node->literal());
    top_type = VT_DOUBLE;
}

void code_maker::visitStoreNode(mathvm::StoreNode *node) {
    if (node->op() == tINCRSET) {
        load(node->var()->name());
        VarType type = top_type;
        node->value()->visit(this);
        if (top_type != type) throw "Types not same (visitStoreNode)";
        switch (type) {
            case VT_INT: bc->addInsn(BC_IADD); break;
            case VT_DOUBLE: bc->addInsn(BC_DADD); break;
            default: throw "Type should be int or double in increment (visitStoreNode)";
        }
    } else if (node->op() == tDECRSET) {
        load(node->var()->name());
        VarType type = top_type;
        node->value()->visit(this);
        if (top_type != type) throw "Types not same (visitStoreNode)";
        switch (type) {
            case VT_INT: bc->addInsn(BC_ISUB); break;
            case VT_DOUBLE: bc->addInsn(BC_DSUB); break;
            default: throw "Type should be int or double in increment (visitStoreNode)";
        }
    } else {
        node->value()->visit(this);
    }
    store(node->var()->name());
}

void code_maker::visitStringLiteralNode(mathvm::StringLiteralNode *node) {
    bc->addInsn(BC_SLOAD);
    uint16_t cid = code->makeStringConstant(node->literal());
    bc->addUInt16(cid);
    top_type = VT_STRING;
}

void code_maker::visitWhileNode(mathvm::WhileNode *node) {
    Label start(bc);
    Label end(bc);
    start.bind(bc->current());
    node->whileExpr()->visit(this);
    if (top_type != VT_INT) throw "While should be integer (visitWhileNode)";
    bc->addInsn(BC_ILOAD0);
    bc->addBranch(BC_IFICMPE, end);
    node->loopBlock()->visit(this);
    bc->addBranch(BC_JA, start);
    end.bind(bc->current());
    top_type = VT_VOID;
}

void code_maker::visitIntLiteralNode(mathvm::IntLiteralNode *node) {
    bc->addInsn(BC_ILOAD);
    bc->addInt64(node->literal());
    top_type = VT_INT;
}

void code_maker::visitBlockNode(mathvm::BlockNode *node) {

    add_small_scope();
    Scope::VarIterator varIterator(node->scope());
    while (varIterator.hasNext()) {
        auto nextVar = varIterator.next();
        add_name(nextVar->name(), nextVar->type());
    }

    Scope::FunctionIterator functionIterator(node->scope());
    while (functionIterator.hasNext()) {

        Bytecode* bytecode = bc;
        auto function = functionIterator.next();
        auto tFunction = new BytecodeFunction(function);
        add_scope();
        code->addFunction(tFunction);
        tFunction->setLocalsNumber(locals_number());
        pop_scope();
        bc = tFunction->bytecode();
        function->node()->visit(this);
        bc = bytecode;
    }

    node->visitChildren(this);

    pop_small_scope();
}

void code_maker::visitBinaryOpNode(mathvm::BinaryOpNode *node) {
    node->left()->visit(this);
    VarType type1 = top_type;
    node->right()->visit(this);
    VarType type2 = top_type;

    if (type1 != type2) throw "Types must be same (visitBinaryNode)";
    if (type1 != VT_INT && type1 != VT_DOUBLE) throw "Integer or Double (visitBinaryNode)";
    switch (node->kind()) {
        case tADD:
            if (type1 == VT_INT) bc->addInsn(BC_IADD);
            else bc->addInsn(BC_DADD);
            break;
        case tSUB:
            if (type1 == VT_INT) bc->addInsn(BC_ISUB);
            else bc->addInsn(BC_DSUB);
            break;
        case tMUL:
            if (type1 == VT_INT) bc->addInsn(BC_IMUL);
            else bc->addInsn(BC_DMUL);
            break;
        case tDIV:
            if (type1 == VT_INT) bc->addInsn(BC_IDIV);
            else bc->addInsn(BC_DDIV);
            break;
        case tEQ:
            if (type1 == VT_INT) bc->addInsn(BC_ICMP);
            else bc->addInsn(BC_DCMP);
            compare(BC_IFICMPE);
            break;
        case tNEQ:
            if (type1 == VT_INT) bc->addInsn(BC_ICMP);
            else bc->addInsn(BC_DCMP);
            compare(BC_IFICMPNE);
            break;
        case tGE:
            if (type1 == VT_INT) bc->addInsn(BC_ICMP);
            else bc->addInsn(BC_DCMP);
            compare(BC_IFICMPGE);
            break;
        case tGT:
            if (type1 == VT_INT) bc->addInsn(BC_ICMP);
            else bc->addInsn(BC_DCMP);
            compare(BC_IFICMPG);
            break;
        case tLT:
            if (type1 == VT_INT) bc->addInsn(BC_ICMP);
            else bc->addInsn(BC_DCMP);
            compare(BC_IFICMPL);
            break;
        case tLE:
            if (type1 == VT_INT) bc->addInsn(BC_ICMP);
            else bc->addInsn(BC_DCMP);
            compare(BC_IFICMPLE);
            break;
        case tOR:
            if (type1 != VT_INT || type2 != VT_INT) throw "Only integers in and (visitBinaryOp)";
            bc->addInsn(BC_ILOAD0);
            bc->addInsn(BC_ICMP);
            compare(BC_IFICMPNE);
            bc->addInsn(BC_SWAP);
            bc->addInsn(BC_ILOAD0);
            bc->addInsn(BC_ICMP);
            compare(BC_IFICMPNE);
            bc->addInsn(BC_IADD);
            break;
        case tAND:
            if (type1 != VT_INT || type2 != VT_INT) throw "Only integers in and (visitBinaryOp)";
            bc->addInsn(BC_ILOAD0);
            bc->addInsn(BC_ICMP);
            compare(BC_IFICMPNE);
            bc->addInsn(BC_SWAP);
            bc->addInsn(BC_ILOAD0);
            bc->addInsn(BC_ICMP);
            compare(BC_IFICMPNE);
            bc->addInsn(BC_IMUL);
            break;
        case tAAND:
            if (type1 == VT_INT) bc->addInsn(BC_IAAND);
            else throw "Should be Integer (visitBinaryOp)";
            break;
        case tAOR:
            if (type1 == VT_INT) bc->addInsn(BC_IAOR);
            else throw "Should be Integer (visitBinaryOp)";
            break;
        case tAXOR:
            if (type1 == VT_INT) bc->addInsn(BC_IAXOR);
            else throw "Should be Integer (visitBinaryOp)";
            break;
        default: throw "Operation not found (visitBinaryNode)";
    }
}

void code_maker::visitUnaryOpNode(mathvm::UnaryOpNode *node) {
    node->operand()->visit(this);
    if (top_type != VT_INT && top_type != VT_DOUBLE) throw "In unary only Integer or Double (visitUnaryOperation)";
    switch (node->kind()) {
        case tSUB:
            if (top_type == VT_INT) bc->addInsn(BC_INEG);
            else bc->addInsn(BC_DNEG);
            break;
        case tNOT:
            if (top_type != VT_INT) throw "Only integer for not (visitUnaryOperation)";
            compare(BC_IFICMPE);
            break;
        default: throw "Operation not found (visitUnaryOperation)";
    }
}

void code_maker::visitNativeCallNode(mathvm::NativeCallNode *node) {
    // TODO
}

void code_maker::visitReturnNode(mathvm::ReturnNode *node) {
    if (node->returnExpr()) {
        node->returnExpr()->visit(this);
    }
    bc->addInsn(BC_RETURN);
}

void code_maker::visitFunctionNode(mathvm::FunctionNode *node) {
    for (size_t i = 1; i < node->signature().size(); ++i) {
        SignatureElement se = node->signature()[i];
        add_name(se.second, se.first);
    }

    for (size_t i = node->signature().size() - 1; i > 0; --i) {
        SignatureElement se = node->signature()[i];

        top_type = se.first;
        store(se.second);

    }

    node->body()->visit(this);
}

