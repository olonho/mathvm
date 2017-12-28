#include <dlfcn.h>
#include "translator_visitor.h"
#include "bytecode_impl.h"

namespace mathvm {

TranslatorVisitor::TranslatorVisitor(BytecodeImpl *code, Bytecode *bc, TranslatedFunction *top_function)
        : code_(code)
        , bc_(bc)
        , current_function_(top_function)
        , current_type_(VT_INVALID)
        , current_scope_(nullptr)
        , local_count_(0) {}

TranslatorVisitor::~TranslatorVisitor() {
    for (auto const &new_var_info : new_variables) {
        delete new_var_info;
    }
}

void TranslatorVisitor::visitBinaryOpNode(BinaryOpNode *node) {
    node->left()->visit(this);
    VarType left_type = current_type_;
    node->right()->visit(this);
    VarType right_type = current_type_;
    
    // branch for preserving precision.
    if (left_type == VT_INT && right_type == VT_DOUBLE) {
        bc_->addInsn(BC_SWAP);
        bc_->addInsn(BC_I2D);
        bc_->addInsn(BC_SWAP);
    } else {
        cast(left_type);
    }
    
    switch (node->kind()) {
        case tOR:
        case tAND:
        case tAOR:
        case tAAND:
        case tAXOR:
            logical_op(node->kind(), current_type_);
            break;
        case tEQ:
        case tNEQ:
        case tGT:
        case tGE:
        case tLT:
        case tLE:
            compare_op(node->kind(), current_type_);
            break;
        case tADD:
        case tSUB:
        case tMUL:
        case tDIV:
        case tMOD:
            numeric_op(node->kind(), current_type_);
            break;
        default:
            throw std::logic_error("wrong type of binary operation.");
    }
    // current_type_ is set in ops
}

void TranslatorVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    node->operand()->visit(this);
    
    switch (node->kind()) {
        case tNOT:
            cast(VT_INT);
            bc_->addInsn(BC_ILOAD1);
            bc_->addInsn(BC_IAXOR);
            break;
        case tSUB:
            switch (current_type_) {
                case VT_DOUBLE:
                    bc_->addInsn(BC_DNEG);
                    break;
                case VT_STRING:
                    cast(VT_INT);
                case VT_INT:
                    bc_->addInsn(BC_INEG);
                    break;
                default:
                    throw std::logic_error("wrong type for negation.");
            }
            break;
        default:
            throw std::logic_error("wrong type of unary operation.");
    }
}

void TranslatorVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    bc_->addInsn(BC_SLOAD);
    const uint16_t constant_id = code_->makeStringConstant1(node->literal());
    bc_->addUInt16(constant_id);
    current_type_ = VT_STRING;
}

void TranslatorVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    bc_->addInsn(BC_DLOAD);
    bc_->addDouble(node->literal());
    current_type_ = VT_DOUBLE;
}

void TranslatorVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    bc_->addInsn(BC_ILOAD);
    bc_->addInt64(node->literal());
    current_type_ = VT_INT;
}

void TranslatorVisitor::visitLoadNode(LoadNode *node) {
    auto var = node->var();
    load_variable(var->name(), var->type());
}

void TranslatorVisitor::visitStoreNode(StoreNode *node) {
    auto var = node->var();
    const auto &var_name = var->name();
    const auto &var_type = var->type();
    const auto op_kind = node->op();
    
    if (op_kind == tINCRSET || op_kind == tDECRSET) {
        load_variable(var_name, var_type);
    }
    
    node->value()->visit(this);
    cast(var_type);
    
    if (op_kind == tINCRSET || op_kind == tDECRSET) {
        numeric_op(op_kind == tINCRSET ? tADD : tSUB, var_type);
    }
    store_variable(var_name, var_type);
}

void TranslatorVisitor::visitForNode(ForNode *node) {
    const BinaryOpNode *range = node->inExpr()->asBinaryOpNode();
    assert(node->inExpr()->isBinaryOpNode() && range->kind() == tRANGE);
    assert(node->var()->type() == VT_INT);
    
    auto const &var_name = node->var()->name();
    auto const &var_type = node->var()->type();
    
    range->left()->visit(this);
    cast(VT_INT);
    store_variable(var_name, var_type);
    
    Label condition_label{bc_};
    Label exit_label{bc_};
    
    bc_->bind(condition_label);
    
    range->right()->visit(this);
    cast(VT_INT);
    bc_->addInsn(BC_STOREIVAR0);
    

    load_variable(var_name, var_type);
    bc_->addInsn(BC_LOADIVAR0);
    bc_->addBranch(BC_IFICMPG, exit_label);
    
    node->body()->visit(this);
    
    load_variable(var_name, var_type);
    bc_->addInsn(BC_ILOAD1);
    bc_->addInsn(BC_IADD);
    
    store_variable(var_name, var_type);
    bc_->addBranch(BC_JA, condition_label);
    bc_->bind(exit_label);
    
    current_type_ = VT_VOID;
}

void TranslatorVisitor::visitWhileNode(WhileNode *node) {
    Label condition_label{bc_};
    Label exit_label{bc_};
    
    bc_->bind(condition_label);
    node->whileExpr()->visit(this);
    cast(VT_INT);
    
    bc_->addInsn(BC_ILOAD0);
    bc_->addBranch(BC_IFICMPE, exit_label);
    
    node->loopBlock()->visit(this);
    
    bc_->addBranch(BC_JA, condition_label);
    bc_->bind(exit_label);
    
    current_type_ = VT_VOID;
}

void TranslatorVisitor::visitIfNode(IfNode *node) {
    node->ifExpr()->visit(this);
    cast(VT_INT);
    
    Label else_label{bc_};
    Label exit_label{bc_};
    
    bc_->addInsn(BC_ILOAD0);
    bc_->addBranch(BC_IFICMPE, else_label);
    
    node->thenBlock()->visit(this);
    
    bc_->addBranch(BC_JA, exit_label);
    bc_->bind(else_label);
    
    if (node->elseBlock() != nullptr) {
        node->elseBlock()->visit(this);
    }
    
    bc_->bind(exit_label);
    
    current_type_ = VT_VOID;
}

void TranslatorVisitor::visitBlockNode(BlockNode *node) {
    Scope *old_scope = current_scope_;
    current_scope_ = node->scope();
    
    Scope::VarIterator var_it{current_scope_};
    while (var_it.hasNext()) {
        auto next = var_it.next();
        auto new_var_info = new VariableInfo(current_function_->id(), local_count_++);
        new_variables.push_back(new_var_info);
        next->setInfo(new_var_info);
    }
    
    for (Scope::FunctionIterator func_it{node->scope()}; func_it.hasNext();) {
        code_->addFunction(new BytecodeFunction(func_it.next()));
    }
    
    for (Scope::FunctionIterator func_it{node->scope()}; func_it.hasNext();) {
        func_it.next()->node()->visit(this);
    }
    
    for (uint32_t i = 0; i < node->nodes(); ++i) {
        auto child_node = node->nodeAt(i);
        child_node->visit(this);
        if (!dynamic_cast<NativeCallNode *>(child_node)) {
            assert(current_type_ != VT_INVALID);
            int bytes = 0;
            if (current_type_ == VT_STRING) {
                bytes = sizeof(uint16_t);
            } else if (current_type_ != VT_VOID) {
                bytes = 8;
            }
            for (int j = 0; j < bytes; ++j) {
                bc_->addInsn(BC_POP);
            }
            current_type_ = VT_VOID;
        }
    }
    
    current_scope_ = old_scope;
}

void TranslatorVisitor::visitFunctionNode(FunctionNode *node) {
    uint16_t old_local_count = local_count_;
    Bytecode *old_bc = bc_;
    TranslatedFunction *old_function = current_function_;
    Scope *old_scope = current_scope_;
    
    local_count_ = 0;
    current_function_ = code_->functionByName(node->name());
    bc_ = dynamic_cast<BytecodeFunction *>(current_function_)->bytecode();
    current_scope_ = node->body()->scope()->parent();
    for (int i = (int) (node->parametersNumber()) - 1; i >= 0; --i) {
        const VarType &param_type = node->parameterType(static_cast<uint32_t>(i));
        const string &param_name = node->parameterName(static_cast<uint32_t>(i));
        auto new_variable = new VariableInfo(current_function_->id(), local_count_++);
        new_variables.push_back(new_variable);
        current_scope_->lookupVariable(param_name, false)->setInfo(new_variable);
        current_type_ = param_type;
        store_variable(param_name, param_type);
    }
    
    // trying to find native call node.
//    NativeCallNode *native_call_node;
//    if (node->body()->nodes() > 0 && (native_call_node = dynamic_cast<NativeCallNode *>(node->body()->nodeAt(0)))) {
//        assert(node->body()->nodes() == 2);
//        auto native_name = native_call_node->nativeName();
//        code_->native_map_[current_function_->id()] = native_name;
//        visitNativeCallNode(native_call_node);
//        return;
//    }
    node->body()->visit(this);
    current_function_->setLocalsNumber(local_count_);
    
    local_count_ = old_local_count;
    current_function_ = old_function;
    bc_ = old_bc;
    current_scope_ = old_scope;
}

void TranslatorVisitor::visitReturnNode(ReturnNode *node) {
    auto return_expression = node->returnExpr();
    if (return_expression != nullptr) {
        return_expression->visit(this);
        cast(current_function_->returnType());
    }
    bc_->addInsn(BC_RETURN);
}

void TranslatorVisitor::visitCallNode(CallNode *node) {
    const auto function_name = node->name();
    auto function = code_->functionByName(function_name);
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        cast(function->parameterType(i));
    }
    
    uint16_t function_id = function->id();
    bc_->addInsn(BC_CALL);
    bc_->addUInt16(function_id);
    
    current_type_ = function->returnType();
}

void TranslatorVisitor::cast(const VarType &to) {
    if (current_type_ == to) {
        return;
    }

#define CAST_FAILED \
    throw std::logic_error("Failed to cast from " + string(typeToName(current_type_)) + \
    " into " + typeToName(to));
    
    Instruction conversion_instruction;
    if (current_type_ == VT_INT) {
        if (to == VT_DOUBLE) {
            conversion_instruction = BC_I2D;
        } else {
            CAST_FAILED;
        }
    } else if (current_type_ == VT_DOUBLE) {
        if (to == VT_INT) {
            conversion_instruction = BC_D2I;
        } else {
            CAST_FAILED;
        }
    } else if (current_type_ == VT_STRING) {
        if (to == VT_INT) {
            conversion_instruction = BC_S2I;
        } else {
            CAST_FAILED;
        }
    } else {
        CAST_FAILED
    }
    bc_->addInsn(conversion_instruction);
    current_type_ = to;
}

void TranslatorVisitor::numeric_op(const TokenKind &op_type, const VarType &type) {
#define GENERATE_COMMON_NUMERIC_CASES(letter) \
    case tADD:\
    bc_->addInsn(BC_ ## letter ## ADD);\
    break;\
    case tSUB:\
    bc_->addInsn(BC_ ## letter ## SUB);\
    break;\
    case tMUL:\
    bc_->addInsn(BC_ ## letter ## MUL);\
    break;\
    case tDIV:\
    bc_->addInsn(BC_ ## letter ## DIV);\
    break;
    
    switch (type) {
        case VT_INT:
            switch (op_type) {
                GENERATE_COMMON_NUMERIC_CASES(I)
                case tMOD:
                    bc_->addInsn(BC_IMOD);
                    break;
                default:
                    throw std::logic_error("wrong kind of numeric operation");
            }
            break;
        case VT_DOUBLE:
            switch (op_type) {
                GENERATE_COMMON_NUMERIC_CASES(D)
                default:
                    throw std::logic_error("wrong kind of numeric operation");
            }
            break;
        default:
            throw std::logic_error("wrong type for numeric operation");\

    }
    // may be excessive.
    current_type_ = type;
}



void TranslatorVisitor::logical_op(const TokenKind &op_type, const VarType &type) {
    if (type != VT_INT) {
        throw std::logic_error("wrong kind for logical operation.");
    }
    auto convert_and_store_booleanized = [&]() {
        // if 0 store itself (or don't touch).
        // else store 1.
        Label non_zero_label{bc_};
        Label exit_label{bc_};
        
        bc_->addInsn(BC_ILOAD0);
        bc_->addBranch(BC_IFICMPNE, non_zero_label);
        
        bc_->addInsn(BC_ILOAD0);
        bc_->addBranch(BC_JA, exit_label);
        
        bc_->bind(non_zero_label);
        bc_->addInsn(BC_ILOAD1);
        bc_->bind(exit_label);
    };
    switch (op_type) {
        case tOR:
            bc_->addInsn(BC_STOREIVAR0);
            convert_and_store_booleanized();
            bc_->addInsn(BC_LOADIVAR0);
            convert_and_store_booleanized();
        case tAOR:
            bc_->addInsn(BC_IAOR);
            break;
            
        case tAND:
            bc_->addInsn(BC_STOREIVAR0);
            convert_and_store_booleanized();
            bc_->addInsn(BC_LOADIVAR0);
            convert_and_store_booleanized();
        case tAAND:
            bc_->addInsn(BC_IAAND);
            break;
            
        case tAXOR:
            bc_->addInsn(BC_IAXOR);
            break;
        default:
            throw std::logic_error("wrong kind of logical operation.");
    }
    current_type_ = VT_INT;
}

void TranslatorVisitor::compare_op(const TokenKind &op_type, const VarType &type) {
    Instruction instruction;
    switch (op_type) {
        case tEQ:
            instruction = BC_IFICMPE;
            break;
        case tNEQ:
            instruction = BC_IFICMPNE;
            break;
        case tGT:
            instruction = BC_IFICMPG;
            break;
        case tGE:
            instruction = BC_IFICMPGE;
            break;
        case tLT:
            instruction = BC_IFICMPL;
            break;
        case tLE:
            instruction = BC_IFICMPLE;
            break;
        default:
            throw std::logic_error("invalid kind of compare operation.");
    }
    Label true_label{bc_};
    Label exit_label{bc_};
    
    bc_->addBranch(instruction, true_label);
    bc_->addInsn(BC_ILOAD0);
    bc_->addBranch(BC_JA, exit_label);
    bc_->bind(true_label);
    bc_->addInsn(BC_ILOAD1);
    bc_->bind(exit_label);
    
    current_type_ = VT_INT;
}

TranslatorVisitor::VariableInfo *TranslatorVisitor::resolve(const string &name) {
    auto it = current_scope_->lookupVariable(name, true);
    if (it == nullptr) {
        throw std::logic_error("No variable " + name + " found in scope");
    }
    return static_cast<VariableInfo *>(it->info());
}

void TranslatorVisitor::load_variable(const string &name, const VarType &type) {
    auto var_info = resolve(name);
    bool isLocal = current_function_->id() == var_info->func_id;
    
    Instruction instruction;
    switch (type) {
        case VT_DOUBLE:
            instruction = BC_LOADDVAR;
            break;
        case VT_INT:
            instruction = BC_LOADIVAR;
            break;
        case VT_STRING:
            instruction = BC_LOADSVAR;
            break;
        default:
            throw std::logic_error("wrong type for loading variable");
    }
    if (!isLocal) {
        // convert into BC_LOADCTXIVAR, BC_LOADCTXDVAR, BC_LOADCTXSVAR
        instruction = static_cast<Instruction>(static_cast<int>(instruction) + 6);
        bc_->addInsn(instruction);
        bc_->addUInt16(var_info->func_id);
    } else {
        bc_->addInsn(instruction);
    }
    
    bc_->addUInt16(var_info->var_id);
    current_type_ = type;
}

void TranslatorVisitor::store_variable(const string &name, const VarType &type) {
    auto var_info = resolve(name);
    bool isLocal = current_function_->id() == var_info->func_id;
    
    if (current_type_ != type) {
        throw std::logic_error("cannot store value of type " + string(typeToName(current_type_)) +
                               " into variable of type " + string(typeToName(type)));
    }
    
    Instruction instruction;
    switch (type) {
        case VT_DOUBLE:
            instruction = BC_STOREDVAR;
            break;
        case VT_INT:
            instruction = BC_STOREIVAR;
            break;
        case VT_STRING:
            instruction = BC_STORESVAR;
            break;
        default:
            throw std::logic_error("wrong type for loading variable");
    }
    if (!isLocal) {
        // convert into BC_STORECTXIVAR, BC_STORECTXDVAR, BC_STORECTXSVAR
        instruction = static_cast<Instruction>(static_cast<int>(instruction) + 6);
        bc_->addInsn(instruction);
        bc_->addUInt16(var_info->func_id);
    } else {
        bc_->addInsn(instruction);
    }
    
    bc_->addUInt16(var_info->var_id);
    current_type_ = VT_VOID;
}

void TranslatorVisitor::visitNativeCallNode(NativeCallNode *node) {
    static void *stdlib_handle = nullptr;
    if (stdlib_handle == nullptr) {
//        stdlib_handle = dlopen("/lib/x86_64-linux-gnu/libc.so.6", RTLD_LAZY | RTLD_GLOBAL);
//        if (!stdlib_handle) {
//            throw std::logic_error("Unable to dynamically load stdlib.");
//        }
        stdlib_handle = RTLD_DEFAULT;
    }
    
    const auto &signature = node->nativeSignature();
    const auto &native_name = node->nativeName();
    void *address = dlsym(stdlib_handle, native_name.c_str());
    const char *error_message = dlerror();
    if (error_message != nullptr) {
        throw std::logic_error(error_message);
    }
    
    uint16_t native_id = code_->makeNativeFunction(native_name, signature, address);
    
    for (size_t i = signature.size() - 1; i >= 1; --i) {
        auto param_type = signature[i].first;
        auto param_name = signature[i].second;
        current_type_ = param_type;
        load_variable(param_name, param_type);
//        store_variable(param_name, param_type);
    }
    
//    code_->native_functions_[native_name] = native_id;
    bc_->addInsn(BC_CALLNATIVE);
    bc_->addUInt16(native_id);
    
    code_->natives_ids_[native_name] = native_id;
    current_type_ = signature.front().first;
}

void TranslatorVisitor::visitPrintNode(PrintNode *node) {
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        switch (current_type_) {
            case VT_DOUBLE:
                bc_->addInsn(BC_DPRINT);
                break;
            case VT_INT:
                bc_->addInsn(BC_IPRINT);
                break;
            case VT_STRING:
                bc_->addInsn(BC_SPRINT);
                break;
            default:
                throw std::logic_error("wrong type for print");
        }
        current_type_ = VT_VOID;
    }
}
    
}
