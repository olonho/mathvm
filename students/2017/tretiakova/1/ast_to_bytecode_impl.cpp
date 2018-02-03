#include "../include/ast.h"
#include "../include/mathvm.h"
#include "../include/visitors.h"

#include "include/ast_to_bytecode_impl.h"

#include "bytecode_code.h"

#include <vector>
#include <stack>
#include <map>
#include <memory>
#include <string>
#include <dlfcn.h>

namespace mathvm {

using namespace std;

void BytecodeTranslateVisitor::invalidate(string msg, uint32_t pos) {
    cerr << "[Translator] invalidate INVALID" << endl;
    type_stack.push(VT_INVALID);
    status = Status::Error(msg.c_str(), pos);
}

VarType BytecodeTranslateVisitor::update_type_stack_un() { // check
    if(type_stack.size() < 1) {
        cerr << "[Translator] type stack too small (<1) INVALID" << endl;
        type_stack.push(VT_INVALID);
        return VT_INVALID;
    }
    return type_stack.top();
}

VarType BytecodeTranslateVisitor::update_type_stack() {
    if(type_stack.size() < 2) {
        cerr << "[Translator] type stack too small (<2) INVALID" << endl;
        type_stack.push(VT_INVALID);
        return VT_INVALID;
    }

    VarType t_left = type_stack.top();
    type_stack.pop();
    VarType t_right = type_stack.top();
    type_stack.pop();
    VarType result;

    switch(t_left) {
    case VT_STRING:
        result =
            t_right == t_left ? t_left : VT_INVALID;
        break;
    case VT_DOUBLE:
        if(t_right != VT_INT && t_right != VT_DOUBLE) {
            result = VT_INVALID;
            break;
        }
        result = VT_DOUBLE;
        break;
    case VT_INT:
        if(t_right != VT_INT && t_right != VT_DOUBLE) {
            result = VT_INVALID;
            break;
        }
        result = t_right;
        break;
    default:
        result = VT_INVALID;
        break;
    }

    cerr << "[Translator] update type stack, new "
         << typeToName(result) << endl;
    type_stack.push(result);
    return result;
}

void BytecodeTranslateVisitor::push_numeric(VarType type, Instruction i_bc, Instruction d_bc, uint32_t pos) {
    switch(type) {
    case VT_INT:
        fun_hierarchy.back()->bytecode()->addInsn(i_bc);
        break;
    case VT_DOUBLE:
        fun_hierarchy.back()->bytecode()->addInsn(d_bc);
        break;
    default:
        cerr << "Invalid operand type in numeric op" << endl;
        invalidate("Invalid operand type in numeric op", pos);
        break;
    }
}

void BytecodeTranslateVisitor::push_comparison(VarType type, Instruction i_bc, Instruction d_bc, Instruction s_bc, uint32_t pos) {
    type_stack.pop();
    type_stack.push(VT_INT);

    switch(type) {
    case VT_INT:
        fun_hierarchy.back()->bytecode()->addInsn(i_bc);
        break;
    case VT_DOUBLE:
        fun_hierarchy.back()->bytecode()->addInsn(d_bc);
        break;
    case VT_STRING:
        fun_hierarchy.back()->bytecode()->addInsn(s_bc);
        break;
    default:
        cerr << "Invalid type in comparison" << endl;
        invalidate("Invalid type in comparison", pos);
        break;
    }
}

void BytecodeTranslateVisitor::push_condition(VarType type, Instruction comp_insn, uint32_t pos) {
    type_stack.pop();
    type_stack.push(VT_INT);
    // what we leave after comparison

    if(type == VT_DOUBLE) {
        fun_hierarchy.back()->bytecode()->addInsn(BC_DCMP);
        fun_hierarchy.back()->bytecode()->addInsn(BC_ILOAD0);
        switch(comp_insn) {
        case BC_IFICMPG:
            comp_insn = BC_IFICMPL;
            break;
        case BC_IFICMPGE:
            comp_insn = BC_IFICMPLE;
            break;
        case BC_IFICMPL:
            comp_insn = BC_IFICMPG;
            break;
        case BC_IFICMPLE:
            comp_insn = BC_IFICMPGE;
            break;
        default:
            break;
        }
    }

    fun_hierarchy.back()->bytecode()->addInsn(comp_insn);
    fun_hierarchy.back()->bytecode()->addInt16(2 + 2); // if true, goto load1
    fun_hierarchy.back()->bytecode()->addInsn(BC_ILOAD0);
    fun_hierarchy.back()->bytecode()->addInsn(BC_JA);
    fun_hierarchy.back()->bytecode()->addInt16(1);
    fun_hierarchy.back()->bytecode()->addInsn(BC_ILOAD1);
}

void BytecodeTranslateVisitor::push_logic(VarType type, Instruction bc, uint32_t pos) {
    if(type != VT_INT) {
        cerr << "Non-int type in logic" << endl;
        invalidate("Non-int type in logic", pos);
        return;
    }
    fun_hierarchy.back()->bytecode()->addInsn(bc);
}

void BytecodeTranslateVisitor::push_store(VarType type, uint16_t scope_id, uint16_t var_id, uint32_t pos) {
    switch(type) {
    case VT_INT:
        fun_hierarchy.back()->bytecode()->addInsn(BC_STORECTXIVAR);
        break;
    case VT_DOUBLE:
        fun_hierarchy.back()->bytecode()->addInsn(BC_STORECTXDVAR);
        break;
    case VT_STRING:
        fun_hierarchy.back()->bytecode()->addInsn(BC_STORECTXSVAR);
        break;
    default:
        cerr << "Unexpected type " << typeToName(type) << " in STORE" << endl;
        invalidate("Unexpected type", pos);
        break;
    }

    fun_hierarchy.back()->bytecode()->addUInt16(scope_id);
    fun_hierarchy.back()->bytecode()->addUInt16(var_id);

    return;
}

// returns the position of the jmp's argument, 0 on error
uint32_t BytecodeTranslateVisitor::push_cond_jump(uint32_t pos) {
    if(type_stack.size() < 2) {
        cerr << "Too few entries on stack (<2)" << endl;
        invalidate("Too few entries on stack (<2)", pos);
        return 0;
    }

    VarType type_zero = type_stack.top();
    type_stack.pop();
    VarType type_expr = type_stack.top();
    type_stack.pop();
    if(type_zero == VT_INT && type_expr == VT_INT) {
        // if `cond == 0`, i.e. false -- jump to "after block"
        fun_hierarchy.back()->bytecode()->addInsn(BC_IFICMPE);
        fun_hierarchy.back()->bytecode()->addInt16(0); // temporarily
        uint32_t index = fun_hierarchy.back()->bytecode()->current();
        return index;
    }

    cerr << "Non-int operands (" << typeToName(type_zero)
         << ", " << typeToName(type_expr) << ") in CMP JUMP" << endl;
    invalidate("Non-int operands in CMP JUMP", pos);
    return 0;
}

void BytecodeTranslateVisitor::push_load_i(uint16_t scope_id, uint16_t var_id) {
    fun_hierarchy.back()->bytecode()->addInsn(BC_LOADCTXIVAR);
    fun_hierarchy.back()->bytecode()->addUInt16(scope_id);
    fun_hierarchy.back()->bytecode()->addUInt16(var_id);
    type_stack.push(VT_INT);
}

void BytecodeTranslateVisitor::push_ja(uint32_t to) {
    uint32_t from = fun_hierarchy.back()->bytecode()->current() + 3;
    uint32_t offset = to - from;
    fun_hierarchy.back()->bytecode()->addInsn(BC_JA);
    fun_hierarchy.back()->bytecode()->addInt16((int16_t)offset);
}

void BytecodeTranslateVisitor::update_jmp(uint32_t src) {
    uint32_t cur = fun_hierarchy.back()->bytecode()->current();
    uint32_t offset = cur - src;
    fun_hierarchy.back()->bytecode()->setInt16(src - 2, (int16_t)offset);
    // -2 because the offset
}

bool BytecodeTranslateVisitor::fix_type_mismatch(VarType expected_type, VarType got_type) {
    if(expected_type == VT_INT && got_type == VT_DOUBLE) {
        fun_hierarchy.back()->bytecode()->addInsn(BC_D2I);
        type_stack.pop();
        type_stack.push(VT_INT);
    } else if(expected_type == VT_INT && got_type == VT_STRING) {
        fun_hierarchy.back()->bytecode()->addInsn(BC_S2I);
        type_stack.pop();
        type_stack.push(VT_INT);
    } else if(expected_type == VT_DOUBLE && got_type == VT_INT) {
        fun_hierarchy.back()->bytecode()->addInsn(BC_I2D);
        type_stack.pop();
        type_stack.push(VT_DOUBLE);
    } else {
        return false;
    }
    return true;
}

Status* BytecodeTranslateVisitor::get_status() {
    return status;
}

void BytecodeTranslateVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    cerr << "[BinaryOp]" << endl;

    // Order!
    node->right()->visit(this);
    // store some info about it
    uint32_t after_right_pos = fun_hierarchy.back()->bytecode()->current();
    fun_hierarchy.back()->bytecode()->addInsn(BC_DUMP);
    VarType right_type = type_stack.top();
    // I don't get what the DUMP is: 'dump without removal'?
    // So it will be an idle placeholder
    node->left()->visit(this);
    VarType left_type = type_stack.top();

    VarType type = update_type_stack();
    if(type != VT_INT && type != VT_DOUBLE) {
        cerr << "Invalid type before BinaryOp, type "
             << typeToName(type)
             << ", position " << node->position() << endl;
        invalidate("Invalid type before BinaryOp", node->position());
        return;
    }
    if(right_type != type) {
        if(right_type == VT_INT && type == VT_DOUBLE) {
            fun_hierarchy.back()->bytecode()->set(after_right_pos, BC_I2D);
        } else if(right_type == VT_DOUBLE && type == VT_INT) {
            // ?!
            cerr << "[ALARM] double-to-int conversion right" << endl;
            fun_hierarchy.back()->bytecode()->set(after_right_pos, BC_D2I);
        } else {
            cerr << "Unknown conversion right: " << typeToName(right_type)
                 << " to " << typeToName(type) << endl;
            invalidate("Unknown conversion right", node->position());
            return;
        }
    }
    if(left_type != type) {
        if(left_type == VT_INT && type == VT_DOUBLE) {
            fun_hierarchy.back()->bytecode()->addInsn(BC_I2D);
        } else if(left_type == VT_DOUBLE && type == VT_INT) {
            // ?!
            cerr << "[ALARM] double-to-int conversion left" << endl;
            fun_hierarchy.back()->bytecode()->addInsn(BC_D2I);
        } else {
            cerr << "Unknown conversion left: " << typeToName(right_type)
                 << " to " << typeToName(type) << endl;
            invalidate("Unknown conversion left", node->position());
            return;
        }
    }

    switch(node->kind()) {
    case tOR:
    case tAOR:
        push_logic(type, BC_IAOR, node->position());
        break;
    case tAND:
    case tAAND:
        push_logic(type, BC_IAAND, node->position());
        break;
    case tAXOR:
        push_logic(type, BC_IAXOR, node->position());
        break;
    case tNEQ:
        push_condition(type, BC_IFICMPNE, node->position());
        break;
    case tEQ:
        // the logic might change but for now it seems that
        // this is the right version
        push_condition(type, BC_IFICMPE, node->position());
        break;
    case tGT:
        push_condition(type, BC_IFICMPG, node->position());
        break;
    case tGE:
        push_condition(type, BC_IFICMPGE, node->position());
        break;
    case tLT:
        push_condition(type, BC_IFICMPL, node->position());
        break;
    case tLE:
        push_condition(type, BC_IFICMPLE, node->position());
        break;
    case tADD:
        push_numeric(type, BC_IADD, BC_DADD, node->position());
        break;
    case tSUB:
        push_numeric(type, BC_ISUB, BC_DSUB, node->position());
        break;
    case tMUL:
        push_numeric(type, BC_IMUL, BC_DMUL, node->position());
        break;
    case tDIV:
        push_numeric(type, BC_IDIV, BC_DDIV, node->position());
        break;
    case tMOD:
        if(type == VT_INT) {
            fun_hierarchy.back()->bytecode()->addInsn(BC_IMOD);
            break;
        }
    default:
        cerr << "Unknown token op " << string(tokenOp(node->kind()))
             << " at position " << node->position() << endl;
        invalidate("Unknown token op", node->position());
        break;
    }
}

void BytecodeTranslateVisitor::visitUnaryOpNode(UnaryOpNode* node) {
    cerr << "[UnaryOp]" << endl;

    node->operand()->visit(this);

    VarType type = update_type_stack_un();
    if(type == VT_INVALID || type == VT_VOID || type == VT_STRING) {
        cerr << "Invalid operand type, position " << node->position() << endl;
        invalidate("Invalid operand type", node->position());
        return;
    }

    switch(node->kind()) {
    case tNOT:
        if(type != VT_INT) {
            cerr << "Non-int operand in tNOT, type "
                 << typeToName(type)
                 << ", position " << node->position() << endl;
            invalidate("Non-int operand in tNOT", node->position());
            break;
        }

        fun_hierarchy.back()->bytecode()->addInsn(BC_ILOAD0);
        // pushed int
        fun_hierarchy.back()->bytecode()->addInsn(BC_IFICMPE);
        // removed 2 x ints and pushed one -- bal 0
        fun_hierarchy.back()->bytecode()->addInt16(4);
        fun_hierarchy.back()->bytecode()->addInsn(BC_ILOAD0);
        fun_hierarchy.back()->bytecode()->addInsn(BC_JA);
        fun_hierarchy.back()->bytecode()->addInt16(1);
        fun_hierarchy.back()->bytecode()->addInsn(BC_ILOAD1);
        // after all, pushed one
        type_stack.push(VT_INT);

        break;
    case tSUB:
        push_numeric(type, BC_INEG, BC_DNEG, node->position());
        break;
    default:
        cerr << "Unknown unary op " << tokenOp(node->kind())
             << ", position " << node->position() << endl;
        invalidate("Unknown unary op", node->position());
        break;
    }
}

void BytecodeTranslateVisitor::visitStringLiteralNode(StringLiteralNode* node) {
    cerr << "[StringLiteral]" << endl;

    uint16_t const_id = bcode->makeStringConstant(node->literal());
    cerr << "const_id: " << const_id << " literal: " << node->literal() << endl;
    fun_hierarchy.back()->bytecode()->addInsn(BC_SLOAD);
    fun_hierarchy.back()->bytecode()->addUInt16(const_id);
    type_stack.push(VT_STRING);
}

void BytecodeTranslateVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    cerr << "[DoubleLiteral]" << endl;

    fun_hierarchy.back()->bytecode()->addInsn(BC_DLOAD);
    fun_hierarchy.back()->bytecode()->addDouble(node->literal());
    type_stack.push(VT_DOUBLE);
}

void BytecodeTranslateVisitor::visitIntLiteralNode(IntLiteralNode* node) {
    cerr << "[IntLiteral]" << endl;

    fun_hierarchy.back()->bytecode()->addInsn(BC_ILOAD);
    fun_hierarchy.back()->bytecode()->addInt64(node->literal());
    type_stack.push(VT_INT);
}

void BytecodeTranslateVisitor::visitLoadNode(LoadNode* node) {
    cerr << "[Load]" << " <- " << node->var()->owner() << endl;

    const AstVar* var = node->var();
    Scope* scope = var->owner();

    // get the IDs if present
    int scope_id = bcode->get_scope_id(scope);
    assert(scope_id >= 0);
    pair<int, int> var_id = bcode->get_var_id(scope, var->name());
    if(var_id.first < 0 || var_id.second < 0) {
        invalidate("Unexpected unregistered var in Load", node->position());
        return;
    }

    VarType type = var->type();
    switch(type) {
    case VT_INT:
        fun_hierarchy.back()->bytecode()->addInsn(BC_LOADCTXIVAR);
        type_stack.push(VT_INT);
        break;
    case VT_DOUBLE:
        fun_hierarchy.back()->bytecode()->addInsn(BC_LOADCTXDVAR);
        type_stack.push(VT_DOUBLE);
        break;
    case VT_STRING:
        fun_hierarchy.back()->bytecode()->addInsn(BC_LOADCTXSVAR);
        type_stack.push(VT_STRING);
        break;
    default:
        cerr << "Invalid LOAD type " << typeToName(type)
             << ", position " << node->position() << endl;
        invalidate("Invalid LOAD type", node->position());
        break;
    }

    fun_hierarchy.back()->bytecode()->addUInt16(var_id.first);
    fun_hierarchy.back()->bytecode()->addUInt16(var_id.second);
}

void BytecodeTranslateVisitor::visitStoreNode(StoreNode* node) {
    cerr << "[Store]" << " <- " << node->var()->owner() << endl;

    TokenKind op = node->op();
    const AstVar* var = node->var();
    Scope* scope = var->owner();
    VarType type = var->type();

    int scope_id = bcode->get_scope_id(scope);
    assert(scope_id >= 0);
    pair<int, int> var_id = bcode->get_var_id(scope, var->name());
    if(var_id.first < 0 || var_id.second < 0) {
        cerr << "Unexpected unrecognized var in Store" << endl;
        invalidate("Unrecognized var in Store", node->position());
        return;
    }

    if(op == tINCRSET || op == tDECRSET) {
        switch(type) {
        case VT_INT:
            fun_hierarchy.back()->bytecode()->addInsn(BC_LOADCTXIVAR);
            type_stack.push(VT_INT);
            break;
        case VT_DOUBLE:
            fun_hierarchy.back()->bytecode()->addInsn(BC_LOADCTXDVAR);
            type_stack.push(VT_DOUBLE);
            break;
        default:
            cerr << "Unknown STORE op " << tokenOp(op)
                 << ", position " << node->position() << endl;
            invalidate("Unknown STORE op", node->position());
            break;
        }

        fun_hierarchy.back()->bytecode()->addUInt16(var_id.first);
        fun_hierarchy.back()->bytecode()->addUInt16(var_id.second);
    }

    node->value()->visit(this);
    VarType value_type = update_type_stack_un();
    if(type != value_type) {
        if(!fix_type_mismatch(type, value_type)) {
            cerr << "Store type mismatch, expected "
                 << typeToName(type) << ", got "
                 << typeToName(value_type) << ", position"
                 << node->position() << endl;
            invalidate("Store type mismatch", node->position());
            return;
        }
    }

    if(op == tINCRSET) {
        push_numeric(type, BC_IADD, BC_DADD, node->position());
    } else if(op == tDECRSET) {
        switch(type) {
        case VT_INT:
            fun_hierarchy.back()->bytecode()->addInsn(BC_ILOAD0);
            type_stack.push(VT_INT);
            break;
        case VT_DOUBLE:
            fun_hierarchy.back()->bytecode()->addInsn(BC_DLOAD0);
            type_stack.push(VT_DOUBLE);
            break;
        default:
            break;
        }
        push_numeric(type, BC_ISUB, BC_DSUB, node->position());
        push_numeric(type, BC_IADD, BC_DADD, node->position());
    }

    push_store(type, var_id.first, var_id.second, node->position());
}

void BytecodeTranslateVisitor::visitForNode(ForNode* node) {
    cerr << "[For]" << " <- " << node->var()->owner() << endl;

    // get and check the condition
    BinaryOpNode* range = dynamic_cast<BinaryOpNode*>(node->inExpr());
    if(!range) {
        cerr << "non-BinaryOpNode range in for, position " << node->position() << std::endl;
        invalidate("non-BinaryOpNode range in for", node->position());
        return;
    }
    TokenKind op = range->kind();
    if(op != tRANGE) {
        cerr << "non-range op in for, position " << node->position() << std::endl;
        invalidate("non-range op in for", node->position());
        return;
    }

    // evaluate the condition start
    range->left()->visit(this);

    // load to var
    const AstVar* var = node->var();
    Scope* scope = var->owner();
    int scope_id = bcode->get_scope_id(scope);
    if(scope_id < 0) {
        invalidate("Unexpected unregistered scope in For", node->position());
        return;
    }
    int var_id = bcode->add_var(scope, var->type(), var->name());

    VarType type = var->type();
    VarType left_type = update_type_stack_un();
    if(type != VT_INT || left_type != VT_INT) {
        cerr << "non-int iterator in for, position " << node->position() << endl;
        invalidate("non-int iterator in for", node->position());
        return;
    }
    type_stack.pop();

    push_store(type, scope_id, var_id, node->position());
    uint32_t to_cond_pos = fun_hierarchy.back()->bytecode()->current();

    // make condtion
    push_load_i(scope_id, var_id);
    range->right()->visit(this);
    type = update_type_stack_un();
    if(type != VT_INT) {
        cerr << "non-int cond in for, position " << node->position() << endl;
        invalidate("non-int cond in for", node->position());
        return;
    }

    // set jump
    fun_hierarchy.back()->bytecode()->addInsn(BC_IFICMPL); // it needs to be false to step out of the loop
    fun_hierarchy.back()->bytecode()->addInt16(0);
    uint32_t cond_checked_pos= fun_hierarchy.back()->bytecode()->current();

    // loop body
    node->body()->visit(this);

    // set incr
    push_load_i(scope_id, var_id);
    fun_hierarchy.back()->bytecode()->addInsn(BC_ILOAD1);
    fun_hierarchy.back()->bytecode()->addInsn(BC_IADD);
    push_store(type, scope_id, var_id, node->position());

    // jump
    push_ja(to_cond_pos);
    update_jmp(cond_checked_pos);
}

void BytecodeTranslateVisitor::visitWhileNode(WhileNode* node) {
    cerr << "[While]" << endl;

    uint32_t to_cond_pos = fun_hierarchy.back()->bytecode()->current();
    // right!

    node->whileExpr()->visit(this);
    fun_hierarchy.back()->bytecode()->addInsn(BC_ILOAD0);
    type_stack.push(VT_INT);
    uint32_t cond_checked_pos = push_cond_jump(node->position());

    node->loopBlock()->visit(this);
    push_ja(to_cond_pos);
    update_jmp(cond_checked_pos);
}

void BytecodeTranslateVisitor::visitIfNode(IfNode* node) {
    cerr << "[If]" << endl;

    // If...
    node->ifExpr()->visit(this);
    fun_hierarchy.back()->bytecode()->addInsn(BC_ILOAD0);
    type_stack.push(VT_INT);
    uint32_t first_jmp_pos = push_cond_jump(node->position());

    // ... then ...
    node->thenBlock()->visit(this);
    uint32_t after_then_pos = fun_hierarchy.back()->bytecode()->current();
    uint32_t offset = after_then_pos - first_jmp_pos;
    fun_hierarchy.back()->bytecode()->setInt16(first_jmp_pos - 2, (int16_t)offset);
    // this offset is for the no-else-block case only!!!

    // ... else
    if(node->elseBlock()) {
        fun_hierarchy.back()->bytecode()->addInsn(BC_JA);
        fun_hierarchy.back()->bytecode()->addInt16(0);
        uint32_t second_jmp_pos = fun_hierarchy.back()->bytecode()->current();
        assert(second_jmp_pos == after_then_pos + 3);

        // correct the first jump offset by 3 bytes from JA
        offset = second_jmp_pos - first_jmp_pos;
        fun_hierarchy.back()->bytecode()->setInt16(first_jmp_pos - 2, (int16_t)offset);

        node->elseBlock()->visit(this);

        uint32_t after_else_pos = fun_hierarchy.back()->bytecode()->current();
        uint32_t offset2 = after_else_pos - second_jmp_pos;

        fun_hierarchy.back()->bytecode()->setInt16(second_jmp_pos - 2, offset2);
        // again, -2 because `pos` point to the first block
        // after the JA offset, the JA offset takes 2 bytes
        // ==> (`pos` - 2) is the pos of the offset itself
    }
}

void BytecodeTranslateVisitor::visitBlockNode(BlockNode* node) {
    cerr << "[Block] <- " << node->scope() << endl;

    Scope* block_scope = node->scope();

    for(Scope::VarIterator it(block_scope); it.hasNext();) {
        AstVar* var = it.next();
        pair<int, int> var_id = bcode->get_var_id(block_scope, var->name());
        fun_hierarchy.back()->define_local_var(var_id);
    }

    for(Scope::FunctionIterator it(block_scope); it.hasNext();) {
        AstFunction* fun = it.next();
        StackFrame* sf = (StackFrame*)(bcode->functionByName(fun->name()));

        fun_hierarchy.push_back(sf);
        fun->node()->visit(this);
        fun_hierarchy.pop_back();
    }

    for (uint32_t i = 0; i < node->nodes(); i++) {
        node->nodeAt(i)->visit(this);
        if(node->nodeAt(i)->isCallNode()) {
            VarType rt = bcode->functionByName(
                        node->nodeAt(i)->asCallNode()->name())->returnType();
            if(rt != VT_VOID) {
                fun_hierarchy.back()->bytecode()->addInsn(BC_POP);
            }
        }
    }
}

void BytecodeTranslateVisitor::visitFunctionNode(FunctionNode* node) {
    cerr << "[Function]" << endl;
    Scope* scope = node->body()->scope();
    int scope_id = bcode->get_scope_id(scope);
    assert(scope_id >= 0);
    fun_hierarchy.back()->setScopeId(scope_id);

    for(uint32_t i = 0; i < node->parametersNumber(); i++) {
        VarType parameterType = node->parameterType(i);
        string parameterName = node->parameterName(i);

        pair<int, int> var_id = bcode->get_var_id(scope, parameterName);
        assert(var_id.first != -1 && var_id.second != -1);
        push_store(parameterType, var_id.first, var_id.second, node->position());
        fun_hierarchy.back()->define_local_var(var_id);
    }
    node->body()->visit(this);
}

void BytecodeTranslateVisitor::visitReturnNode(ReturnNode* node) {
    cerr << "[Return]" << endl;

    AstNode* return_expr = node->returnExpr();
    if(return_expr) {
        return_expr->visit(this);

        // check for the correct return type
        VarType return_expr_type = update_type_stack_un();
        if(return_expr_type != fun_hierarchy.back()->returnType()) {
            if(!fix_type_mismatch(fun_hierarchy.back()->returnType(), return_expr_type)) {
                cerr << "Invalid return type; expected "
                     << typeToName(fun_hierarchy.back()->returnType()) << ", got "
                     << typeToName(return_expr_type)
                     << ", position " << node->position() << endl;
                invalidate("Invalid return type", node->position());
                return;
            }
        }
    }
    fun_hierarchy.back()->bytecode()->addInsn(BC_RETURN);
}

void BytecodeTranslateVisitor::visitCallNode(CallNode* node) {
    cerr << "[Call] ";// << endl;
    cerr << node->name() << endl;

    StackFrame* tsf = (StackFrame*)bcode->functionByName(node->name());
    cerr << tsf << endl;
    if(node->parametersNumber() != tsf->parametersNumber()) {
        cerr << "Parameters number mismatch at function " << node->name()
             << " at position " << node->position();
        invalidate("Parameters number mismatch at function", node->position());
        return;
    }

    VarType param_type, stack_type;
    for(int i = node->parametersNumber() - 1; i >= 0; --i) {
    // need reversed order
        node->parameterAt(i)->visit(this);

        param_type = tsf->parameterType(i);
        stack_type = update_type_stack_un();
        if(param_type != stack_type) {
            if(!fix_type_mismatch(param_type, stack_type)) {
                cerr << "Parameter type mismatch, expected "
                     << typeToName(param_type) << ", got "
                     << typeToName(stack_type) << ", position"
                     << node->position() << endl;
                invalidate("Parameter type mismatch", node->position());
                return;
            }
        }
    }

    fun_hierarchy.back()->bytecode()->addInsn(BC_CALL);
    fun_hierarchy.back()->bytecode()->addUInt16(tsf->id());
    type_stack.push(tsf->returnType());
}

void BytecodeTranslateVisitor::visitNativeCallNode(NativeCallNode* node) {
    cerr << "[NativeCall]" << endl;

    dl_fun_ptr function_handler;
    void* handle = dlopen(0,RTLD_NOW|RTLD_GLOBAL);
    *(void**)(&function_handler) = dlsym(handle, node->nativeName().c_str());

    if(!function_handler) {
        cerr << "Failed to load native function " << node->nativeName()
             << ", position " << node->position();
        invalidate("Failed to load native function", node->position());
        return;
    }

    uint16_t nat_id =
            bcode->makeNativeFunction(
                node->nativeName(),
                node->nativeSignature(),
                (const void*)function_handler);

    fun_hierarchy.back()->bytecode()->addInsn(BC_CALLNATIVE);
    fun_hierarchy.back()->bytecode()->addUInt16(nat_id);
}

void BytecodeTranslateVisitor::visitPrintNode(PrintNode* node) {
    cerr << "[Print]" << endl;

    VarType operand_type;
    for(uint32_t i = 0; i < node->operands(); i++) {
        node->operandAt(i)->visit(this);

        operand_type = update_type_stack_un();
        type_stack.pop();
        switch (operand_type) {
        case VT_INT:
            fun_hierarchy.back()->bytecode()->addInsn(BC_IPRINT);
            break;
        case VT_DOUBLE:
            fun_hierarchy.back()->bytecode()->addInsn(BC_DPRINT);
            break;
        case VT_STRING:
            fun_hierarchy.back()->bytecode()->addInsn(BC_SPRINT);
            break;
        default:
            cerr << "Invalid operand type for print: " << typeToName(operand_type)
                 << ", position " << node->position() << endl;
            invalidate("Invalid operand type for print", node->position());
            break;
        }
    }
}

}
