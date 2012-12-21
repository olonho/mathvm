#include "BytecodeEmittingAstVisitor.h"

namespace mathvm_ext {

BytecodeEmittingAstVisitor::BytecodeEmittingAstVisitor(Code* output)
    : m_bytecode(0), m_code(output), m_scope(0) {
      m_var_storage.push_back("");
}

BytecodeEmittingAstVisitor::~BytecodeEmittingAstVisitor() {
}

Code* BytecodeEmittingAstVisitor::operator()(AstFunction* main_func) {
    main_func->node()->visit(this);   
    return m_code;
}

Code* BytecodeEmittingAstVisitor::operator()(AstNode* ast) {
    ast->visit(this);
    return m_code;
}

void BytecodeEmittingAstVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    node->right()->visit(this);
    VarType right_type = m_latest_type;
    node->left()->visit(this);
    VarType left_type = m_latest_type;

    switch (node->kind()) {
        case tADD:  // "+"
            m_latest_type = m_primitives.Add(m_bytecode, left_type, right_type);
            break;
        case tSUB:  // "-"
            m_latest_type = m_primitives.Sub(m_bytecode, left_type, right_type);
            break;
        case tMUL:  // "*"
            m_latest_type = m_primitives.Mul(m_bytecode, left_type, right_type);
            break;
        case tDIV:  // "/"
            m_latest_type = m_primitives.Div(m_bytecode, left_type, right_type);
            break;
        case tMOD:  // "%"
            m_latest_type = m_primitives.Mod(m_bytecode, left_type, right_type);
            break;
       case tAND:   // &&
            m_latest_type = m_primitives.And(m_bytecode, left_type, right_type);
            break;
        case tOR:    // "||"
            m_latest_type = m_primitives.Or(m_bytecode, left_type, right_type);
            break;
        case tEQ:   // "=="
            m_latest_type = m_primitives.CmpEq(m_bytecode, left_type, right_type);
            break;
        case tNEQ:  // "!=";
            m_latest_type = m_primitives.CmpNe(m_bytecode, left_type, right_type);
            break;
        case tGT:   // ">";
            m_latest_type = m_primitives.CmpGt(m_bytecode, left_type, right_type);
            break;
        case tGE:   // ">=";
            m_latest_type = m_primitives.CmpGe(m_bytecode, left_type, right_type);
            break;
        case tLT:   // "<";
            m_latest_type = m_primitives.CmpLt(m_bytecode, left_type, right_type);
            break;
        case tLE:   // "<=";
            m_latest_type = m_primitives.CmpLe(m_bytecode, left_type, right_type);
            break;
        case tRANGE:    // ".."
            std::cerr << "Error: Ranges are supported only as a FOR loop condition"
                      << std::endl;
            m_latest_type = m_primitives.Invalid(m_bytecode);
            break;
        default:
            m_latest_type = m_primitives.Invalid(m_bytecode);
            break;
    }
}

void BytecodeEmittingAstVisitor::visitUnaryOpNode(UnaryOpNode* node) {
    node->operand()->visit(this);

    switch (node->kind()) {
        case tNOT:  // "!"
            m_latest_type = m_primitives.Not(m_bytecode, m_latest_type);
            break;
        case tSUB:  // "-"
            m_latest_type = m_primitives.Neg(m_bytecode, m_latest_type);
            break;
        default:
            std::cerr << "Error: Unknown AST node kind '"
                      << node->kind()
                      << "'"
                      << std::endl;
            m_latest_type = m_primitives.Invalid(m_bytecode);
    }
}

void BytecodeEmittingAstVisitor::visitStringLiteralNode(StringLiteralNode* node) {
    uint16_t string_id = m_code->makeStringConstant(node->literal());
    m_latest_type = m_primitives.Load(m_bytecode, string_id, VT_STRING);
}

void BytecodeEmittingAstVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    m_latest_type = m_primitives.Load(m_bytecode, node->literal(), VT_DOUBLE);
}

void BytecodeEmittingAstVisitor::visitIntLiteralNode(IntLiteralNode* node) {
    m_latest_type = m_primitives.Load(m_bytecode, node->literal(), VT_INT);
}

void BytecodeEmittingAstVisitor::visitWhileNode(WhileNode* node) {
    Label entryLoopLabel(m_bytecode, m_bytecode->current());
    Label exitLoopLabel(m_bytecode);

    m_bytecode->addInsn(BC_ILOAD0);
    node->whileExpr()->visit(this);

    m_primitives.JmpEq(m_bytecode, exitLoopLabel);

    node->loopBlock()->visit(this);
    m_primitives.Jmp(m_bytecode, entryLoopLabel);

    m_bytecode->bind(exitLoopLabel);
}

void BytecodeEmittingAstVisitor::visitIfNode(IfNode* node) {
    m_bytecode->addInsn(BC_ILOAD0);
    node->ifExpr()->visit(this);

    Label elseLabel(m_bytecode);
    Label ifExitLabel(m_bytecode);
    //jump if the condition evaluates to false
    m_primitives.JmpEq(m_bytecode, elseLabel);

    node->thenBlock()->visit(this);
    m_primitives.Jmp(m_bytecode, ifExitLabel);

    m_bytecode->bind(elseLabel);
    if (node->elseBlock()) {
        node->elseBlock()->visit(this);
    }

    m_bytecode->bind(ifExitLabel);
}

void BytecodeEmittingAstVisitor::visitReturnNode(ReturnNode* node) {
    if (node->returnExpr()) {
        node->returnExpr()->visit(this);
    } else {
        m_latest_type = VT_VOID;
    }

    m_primitives.Return(m_bytecode, m_latest_type);
}

void BytecodeEmittingAstVisitor::visitPrintNode(PrintNode* node) {
    uint32_t parameters_number = node->operands();
    uint32_t last_parameter = parameters_number - 1;

    std::vector<VarType> parameter_types;
    parameter_types.reserve(parameters_number);

    // push arguments on the top of the stack in the right-to-left order
    for (uint32_t i = 0; i < parameters_number; ++i) {
        node->operandAt(last_parameter - i)->visit(this);
        parameter_types.push_back(m_latest_type);
    }

    // emit needed number of print instructions
    for (uint32_t i = 0; i < parameters_number; ++i) {
        m_primitives.Print(m_bytecode, parameter_types[last_parameter - i]);
    }
}

}
