#include <stdexcept>
#include <cassert>
#include <memory>

#include "parser.h"
#include "translator_exception.h"
#include "mathvm_translator.h"

namespace mathvm
{

// Context class
///////////////////////////////////////////////////////////////////////////
BytecodeFunction* translator_context::function()
{
    return function_;
}

Bytecode* translator_context::bc()
{
    return function_->bytecode();
}

uint16_t translator_context::id()
{
    return id_;
}

translator_context* translator_context::parent()
{
    return parent_;
}

translator_context* translator_context::up(BytecodeFunction* func = nullptr)
{
    if (func == nullptr)
        func = function();

    return new translator_context(id() + 1, func, this);
}

translator_context* translator_context::down()
{
    std::unique_ptr<translator_context> bye_bye(this);
    return parent();
}
bool translator_context::contains(const std::string& var_name)
{
    return local_vars_.find(var_name) != local_vars_.end() ||
           (parent() != nullptr && parent()->contains(var_name));
}

void translator_context::add_var(const std::string& var_name)
{
    uint16_t local_id = local_vars_.size();
    local_vars_[var_name] = local_id;
}

void translator_context::add_all_vars(Scope* scope)
{
    Scope::VarIterator varIterator(scope);
    while(varIterator.hasNext())
    {
        AstVar* var(varIterator.next());
        add_var(var->name());
    }
}

std::pair<uint16_t, uint16_t> translator_context::get_var_id(const std::string& var_name)
{
    auto it = local_vars_.find(var_name);

    if (it != local_vars_.end())
    {
        uint16_t first_id = id();
        uint16_t second_id = it->second;
        return std::make_pair(first_id, second_id);
    }

    assert(parent() != nullptr);
    return parent()->get_var_id(var_name);
}


// Visitor class
///////////////////////////////////////////////////////////////////////////
Code* BytecodeVisitor::make_visit(AstFunction* top)
{
    visitAstFunction(top);
    return code_;
}

VarType BytecodeVisitor::get_tos_type()
{
    assert(tos_types_.size() != 0);
    VarType type = tos_types_.top();
    tos_types_.pop();
    return type;
}

void BytecodeVisitor::set_tos_type(VarType type)
{
    tos_types_.push(type);
}

void BytecodeVisitor::cast_tos_to(VarType type)
{
    VarType tos_type = get_tos_type();
    if (tos_type == type)
    {
        set_tos_type(tos_type);
        return;
    }

    switch (tos_type)
    {
        case VT_INT:
            context_->bc()->addInsn(BC_I2D);
            break;
        case VT_DOUBLE:
            context_->bc()->addInsn(BC_D2I);
            break;
        default:
            throw translator_exception("Can not cast tos type");
    }

    set_tos_type(type);
}

void BytecodeVisitor::visitAstFunction(AstFunction* func)
{
    BytecodeFunction* bfunc = new BytecodeFunction(func);
    code_->addFunction(bfunc);

    if (context_ != nullptr)
        context_ = context_->up(bfunc);
    else
        context_ = new translator_context(bfunc);

    func->node()->visit(this);

    if (context_->parent() == nullptr)
    {
        context_->bc()->addInsn(BC_STOP);
        delete context_;
    }
    else
    {
        context_ = context_->down();
    }
}

void BytecodeVisitor::process_arithmetic_operation(BinaryOpNode* node)
{
    node->right()->visit(this);
    VarType right_operand_type = get_tos_type();
    node->left()->visit(this);
    VarType left_operand_type = get_tos_type();

    if (left_operand_type != right_operand_type)
    {
        switch (left_operand_type)
        {
            case VT_INT:
                context_->bc()->addInsn(BC_I2D);
                break;
            case VT_DOUBLE:
                context_->bc()->addInsn(BC_SWAP);
                context_->bc()->addInsn(BC_I2D);
                context_->bc()->addInsn(BC_SWAP);
                break;
            default:
                throw translator_exception("Invalid types for arithmetic operation " + std::string(tokenOp(node->kind())));
        }
    }

    if (left_operand_type == VT_INT && right_operand_type == VT_INT)
    {
        switch(node->kind())
        {
            case tADD:
                context_->bc()->addInsn(BC_IADD);
                break;
            case tMUL:
                context_->bc()->addInsn(BC_IMUL);
                break;
            case tSUB:
                context_->bc()->addInsn(BC_ISUB);
                break;
            case tDIV:
                context_->bc()->addInsn(BC_IDIV);
                break;
            case tMOD:
                context_->bc()->addInsn(BC_IMOD);
                break;
            default:
                break;
        }

        set_tos_type(VT_INT);
    }
    else
    {
        switch(node->kind())
        {
            case tADD:
                context_->bc()->addInsn(BC_DADD);
                break;
            case tMUL:
                context_->bc()->addInsn(BC_DMUL);
                break;
            case tSUB:
                context_->bc()->addInsn(BC_DSUB);
                break;
            case tDIV:
                context_->bc()->addInsn(BC_DDIV);
                break;
            default:
                break;
        }

        set_tos_type(VT_DOUBLE);
    }
}

void BytecodeVisitor::process_logic_operation(BinaryOpNode* node)
{
    node->left()->visit(this);
    cast_tos_to(VT_INT);
    get_tos_type();

    Label true_label(context_->bc());
    Label false_label(context_->bc());

    context_->bc()->addInsn(BC_ILOAD0);
    switch (node->kind())
    {
        case tOR:
            context_->bc()->addBranch(BC_IFICMPE, true_label);
            context_->bc()->addInsn(BC_ILOAD1);
            break;
        case tAND:
            context_->bc()->addBranch(BC_IFICMPNE, true_label);
            context_->bc()->addInsn(BC_ILOAD0);
            break;
        default:
            break;
    }

    context_->bc()->addBranch(BC_JA, false_label);
    context_->bc()->bind(true_label);

    node->right()->visit(this);
    cast_tos_to(VT_INT);
    get_tos_type();

    context_->bc()->bind(false_label);

    set_tos_type(VT_INT);
}

void BytecodeVisitor::process_compare_operation(BinaryOpNode* node)
{
    node->right()->visit(this);
    cast_tos_to(VT_INT);
    get_tos_type();

    node->left()->visit(this);
    cast_tos_to(VT_INT);
    get_tos_type();

    Label label1 = Label(context_->bc());
    Label label2 = Label(context_->bc());

    switch (node->kind())
    {
        case tEQ:
            context_->bc()->addBranch(BC_IFICMPE, label1);
            break;
        case tNEQ:
            context_->bc()->addBranch(BC_IFICMPNE, label1);
            break;
        case tGE:
            context_->bc()->addBranch(BC_IFICMPGE, label1);
            break;
        case tGT:
            context_->bc()->addBranch(BC_IFICMPG, label1);
            break;
        case tLE:
            context_->bc()->addBranch(BC_IFICMPLE, label1);
            break;
        case tLT:
            context_->bc()->addBranch(BC_IFICMPL, label1);
            break;
        default:
            break;
    }

    context_->bc()->addInsn(BC_ILOAD0);
    context_->bc()->addBranch(BC_JA, label2);
    context_->bc()->bind(label1);
    context_->bc()->addInsn(BC_ILOAD1);
    context_->bc()->bind(label2);

    set_tos_type(VT_INT);
}

void BytecodeVisitor::process_bit_operation(BinaryOpNode* node)
{
    node->right()->visit(this);
    cast_tos_to(VT_INT);
    get_tos_type();

    node->left()->visit(this);
    cast_tos_to(VT_INT);
    get_tos_type();

    switch (node->kind())
    {
        case tAOR:
            context_->bc()->addInsn(BC_IAOR);
            break;
        case tAAND:
            context_->bc()->addInsn(BC_IAAND);
            break;
        case tAXOR:
            context_->bc()->addInsn(BC_IAXOR);
            break;
        default:
            break;
    }

    set_tos_type(VT_INT);
}

void BytecodeVisitor::visitBinaryOpNode(BinaryOpNode* node)
{
    switch (node->kind())
    {
        case tADD:
        case tSUB:
        case tMUL:
        case tDIV:
        case tMOD:
            process_arithmetic_operation(node);
            break;
        case tOR:
        case tAND:
            process_logic_operation(node);
            break;
        case tEQ:
        case tNEQ:
        case tGE:
        case tLE:
        case tGT:
        case tLT:
            process_compare_operation(node);
            break;
        case tAOR:
        case tAAND:
        case tAXOR:
            process_bit_operation(node);
            break;
        default:
            throw translator_exception("Invalid binary operation " + std::string(tokenOp(node->kind())));
    }
}

void BytecodeVisitor::visitUnaryOpNode(UnaryOpNode* node)
{
    node->operand()->visit(this);

    VarType tos_type = get_tos_type();
    if (tos_type == VT_INT)
    {
        switch (node->kind())
        {
            case tSUB:
                context_->bc()->addInsn(BC_INEG);
                break;
            case tNOT:
                {
                    Label label1 = Label(context_->bc());
                    Label label2 = Label(context_->bc());
                    context_->bc()->addInsn(BC_ILOAD0);
                    context_->bc()->addBranch(BC_IFICMPE, label1);
                    context_->bc()->addInsn(BC_ILOAD0);
                    context_->bc()->addBranch(BC_JA, label2);
                    context_->bc()->bind(label1);
                    context_->bc()->addInsn(BC_ILOAD1);
                    context_->bc()->bind(label2);
                }
                break;
            case tADD:
                break;
            default:
                throw translator_exception("Invalid unary operation for int");
        }

        set_tos_type(VT_INT);
    }
    else if (tos_type == VT_DOUBLE)
    {
        switch (node->kind())
        {
            case tSUB:
                context_->bc()->addInsn(BC_DNEG);
                break;
            case tADD:
                break;
            default:
                throw translator_exception("Invalid unary operation for double");
        }

        set_tos_type(VT_DOUBLE);
    }
    else
    {
        throw translator_exception("Invalid type for unary operation " + std::string(tokenOp(node->kind())));
    }
}

void BytecodeVisitor::visitStringLiteralNode(StringLiteralNode* node)
{
    context_->bc()->addInsn(BC_SLOAD);
    context_->bc()->addUInt16(code_->makeStringConstant(node->literal()));
    set_tos_type(VT_STRING);
}

void BytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node)
{
    context_->bc()->addInsn(BC_DLOAD);
    context_->bc()->addDouble(node->literal());
    set_tos_type(VT_DOUBLE);
}

void BytecodeVisitor::visitIntLiteralNode(IntLiteralNode* node)
{
    context_->bc()->addInsn(BC_ILOAD);
    context_->bc()->addInt64(node->literal());
    set_tos_type(VT_INT);
}

void BytecodeVisitor::load_var(const AstVar* var)
{
    assert(context_->contains(var->name()));

    auto var_id = context_->get_var_id(var->name());
    if (var_id.first == context_->id())
    {
        switch (var->type())
        {
            case VT_INT:
                context_->bc()->addInsn(BC_LOADIVAR);
                break;
            case VT_DOUBLE:
                context_->bc()->addInsn(BC_LOADDVAR);
                break;
            case VT_STRING:
                context_->bc()->addInsn(BC_LOADSVAR);
                break;
            default:
                throw translator_exception("Invalid type for load");
        }
    }
    else
    {
        switch (var->type())
        {
            case VT_INT:
                context_->bc()->addInsn(BC_LOADCTXIVAR);
                break;
            case VT_DOUBLE:
                context_->bc()->addInsn(BC_LOADCTXDVAR);
                break;
            case VT_STRING:
                context_->bc()->addInsn(BC_LOADCTXSVAR);
                break;
            default:
                throw translator_exception("Invalid type for load");
        }

        context_->bc()->addUInt16(var_id.first);
    }

    context_->bc()->addUInt16(var_id.second);
    set_tos_type(var->type());
}

void BytecodeVisitor::store_var(const AstVar* var)
{
    assert(context_->contains(var->name()));

    auto var_id = context_->get_var_id(var->name());
    if (var_id.first == context_->id())
    {
        switch (var->type())
        {
            case VT_INT:
                context_->bc()->addInsn(BC_STOREIVAR);
                break;
            case VT_DOUBLE:
                context_->bc()->addInsn(BC_STOREDVAR);
                break;
            case VT_STRING:
                context_->bc()->addInsn(BC_STORESVAR);
                break;
            default:
                throw translator_exception("Invalid type for store");
        }
    }
    else
    {
        switch (var->type())
        {
            case VT_INT:
                context_->bc()->addInsn(BC_STORECTXIVAR);
                break;
            case VT_DOUBLE:
                context_->bc()->addInsn(BC_STORECTXDVAR);
                break;
            case VT_STRING:
                context_->bc()->addInsn(BC_STORECTXSVAR);
                break;
            default:
                throw translator_exception("Invalid type for store");
        }

        context_->bc()->addUInt16(var_id.first);
    }

    context_->bc()->addUInt16(var_id.second);
    get_tos_type();
}

void BytecodeVisitor::visitLoadNode(LoadNode* node)
{
    load_var(node->var());
}

void BytecodeVisitor::visitStoreNode(StoreNode* node)
{
    node->value()->visit(this);
    VarType var_type = node->var()->type();
    cast_tos_to(var_type);

    switch (node->op())
    {
        case tINCRSET:
            load_var(node->var());
            context_->bc()->addInsn((var_type == VT_INT) ? BC_IADD : BC_DADD);
            break;
        case tDECRSET:
            load_var(node->var());
            context_->bc()->addInsn((var_type == VT_INT) ? BC_ISUB : BC_DSUB);
            break;
        default:
            break;
    }

    store_var(node->var());
}

void BytecodeVisitor::visitForNode(ForNode* node)
{
    if (node->var()->type() != VT_INT)
        throw translator_exception("Invalid type for counter");

    Label start_label = Label(context_->bc());
    Label stop_label = Label(context_->bc());

    BinaryOpNode* in_expression = node->inExpr()->asBinaryOpNode();
    in_expression->left()->visit(this);
    cast_tos_to(VT_INT);
    store_var(node->var());

    context_->bc()->bind(start_label);

    in_expression->right()->visit(this);
    cast_tos_to(VT_INT);
    load_var(node->var());
    context_->bc()->addBranch(BC_IFICMPG,stop_label);

    node->body()->visit(this);

    load_var(node->var());
    context_->bc()->addInsn(BC_ILOAD1);
    context_->bc()->addInsn(BC_IADD);
    store_var(node->var());
    context_->bc()->addBranch(BC_JA, start_label);

    context_->bc()->bind(stop_label);
}

void BytecodeVisitor::visitWhileNode(WhileNode* node)
{
    Label start_label = Label(context_->bc());
    Label stop_label = Label(context_->bc());

    context_->bc()->bind(start_label);
    node->whileExpr()->visit(this);
    cast_tos_to(VT_INT);
    context_->bc()->addInsn(BC_ILOAD0);
    context_->bc()->addBranch(BC_IFICMPE, stop_label);
    node->loopBlock()->visit(this);
    context_->bc()->addBranch(BC_JA, start_label);
    context_->bc()->bind(stop_label);
}

void BytecodeVisitor::visitIfNode(IfNode* node)
{
    node->ifExpr()->visit(this);
    cast_tos_to(VT_INT);

    Label else_label = Label(context_->bc());
    Label stop_label = Label(context_->bc());

    context_->bc()->addInsn(BC_ILOAD0);
    context_->bc()->addBranch(BC_IFICMPE, else_label);

    node->thenBlock()->visit(this);
    context_->bc()->addBranch(BC_JA, stop_label);

    context_->bc()->bind(else_label);
    if (node->elseBlock())
        node->elseBlock()->visit(this);
    context_->bc()->bind(stop_label);
}

void BytecodeVisitor::visitBlockNode(BlockNode* node)
{
    context_->add_all_vars(node->scope());

    Scope::FunctionIterator it(node->scope());
    while (it.hasNext())
    {
        visitAstFunction(it.next());
    }

    for (std::size_t i = 0; i < node->nodes(); ++i)
        node->nodeAt(i)->visit(this);
}

void BytecodeVisitor::visitFunctionNode(FunctionNode* node)
{
    for (std::size_t i = 0; i < node->parametersNumber(); ++i)
    {
        context_->add_var(node->parameterName(i));
        switch(node->parameterType(i))
        {
            case VT_INT:
                context_->bc()->addInsn(BC_STOREIVAR);
                break;
            case VT_DOUBLE:
                context_->bc()->addInsn(BC_STOREDVAR);
                break;
            case VT_STRING:
                context_->bc()->addInsn(BC_STORESVAR);
                break;
            default:
                throw translator_exception("Invalid parameter type in function " + node->name());
        }

        std::pair<uint16_t, uint16_t> var_id = context_->get_var_id(node->parameterName(i));
        context_->bc()->addInt16(var_id.second);
    }

    if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode())
    {
        node->body()->nodeAt(0)->visit(this);
    }
    else
    {
    node->body()->visit(this);
    }
}

void BytecodeVisitor::visitReturnNode(ReturnNode* node)
{
    if(node->returnExpr())
    {
        node->returnExpr()->visit(this);
        cast_tos_to(context_->function()->returnType());
    }
    context_->bc()->addInsn(BC_RETURN);
}

void BytecodeVisitor::visitCallNode(CallNode* node)
{
    BytecodeFunction* bfunc = static_cast<BytecodeFunction*>(code_->functionByName(node->name()));
    if (!bfunc || node->parametersNumber() != bfunc->parametersNumber())
        throw translator_exception("Function " + node->name() + " is not found");

    for (std::size_t i = node->parametersNumber(); i > 0; --i)
    {
        node->parameterAt(i - 1)->visit(this);
        cast_tos_to(bfunc->parameterType(i - 1));
        get_tos_type();
    }

    context_->bc()->addInsn(BC_CALL);
    context_->bc()->addUInt16(bfunc->id());

    if (bfunc->returnType() != VT_VOID)
        set_tos_type(bfunc->returnType());
}

void BytecodeVisitor::visitNativeCallNode(NativeCallNode* node)
{
    throw translator_exception("Native call is unsupported operation");
}

void BytecodeVisitor::visitPrintNode(PrintNode* node)
{
    for (std::size_t i = 0; i < node->operands(); ++i)
    {
       node->operandAt(i)->visit(this);
       VarType tos_type = get_tos_type();
       switch (tos_type)
       {
           case VT_INT:
               context_->bc()->addInsn(BC_IPRINT);
               break;
           case VT_DOUBLE:
               context_->bc()->addInsn(BC_DPRINT);
               break;
           case VT_STRING:
               context_->bc()->addInsn(BC_SPRINT);
               break;
           default:
               throw translator_exception("Invalid parameter type for print");
       }
   }
}


// Translator class
///////////////////////////////////////////////////////////////////////////
Status* BytecodeTranslatorImpl::translate(const std::string& program, Code** code)
{
    Parser parser;
    Status* status = parser.parseProgram(program);

    if (status->isError())
        return status;

    try
    {
        BytecodeVisitor visitor = BytecodeVisitor(*code);
        AstFunction* top = parser.top();
        *code = visitor.make_visit(top);
    }
    catch (const translator_exception& e)
    {
        return Status::Error(e.what());
    }

    return Status::Ok();
}

Translator* Translator::create(const std::string& impl)
{
    if (impl == "" || impl == "translator")
    {
        return new BytecodeTranslatorImpl();
    }
    else
    {
        throw std::logic_error("Unknown argument");
    }
}

} // mathvm namespace
