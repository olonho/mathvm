#include "bytecode_translator.h"
#include "interpreter_code.h"

#include "parser.h"

namespace mathvm {

using namespace std;

namespace details {

TranslatorVisitor::TranslatorVisitor(InterpreterCodeImpl &code)
    : code_(code) {}

TranslatorVisitor::~TranslatorVisitor() {}

void TranslatorVisitor::visitUnaryOpNode(UnaryOpNode* node) {
    VarType type = eval(*node->operand());

    switch (node->kind()) {
        case tSUB:
            switch (type) {
                case VT_DOUBLE:
                    instruction(BC_DNEG, 0);
                    break;
                case VT_INT:
                    instruction(BC_INEG, 0);
                    break;
                default:
                    convert(type, VT_INT);
                    instruction(BC_INEG, 0);
                    break;
            }
            break;
        case tNOT:
            instruction(BC_ILOAD0, 0);
            instruction(BC_ICMP, 0);
            instruction(BC_ILOAD1, 0);
            instruction(BC_IAAND, 0);
            instruction(BC_ILOAD1, 0);
            instruction(BC_IAXOR, 0);
            break;
        default:
            assert(false);
    }
}

void TranslatorVisitor::visitStringLiteralNode(StringLiteralNode* node) {
    if (node->literal().size() == 0) {
        instruction(BC_SLOAD0, 0);
    } else {
        uint16_t id = code_.makeStringConstant(node->literal());
        instruction(BC_SLOAD, 0);
        bytecode().addUInt16(id);
    }

    stack_types_.push(VT_STRING);
}

void TranslatorVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    if (node->literal() == 0.0) {
        instruction(BC_DLOAD0, 0);
    } else if (node->literal() == 1.0) {
        instruction(BC_DLOAD1, 0);
    } else if (node->literal() == -1.0) {
        instruction(BC_DLOADM1, 0);
    } else {
        instruction(BC_DLOAD, 0);
        bytecode().addDouble(node->literal());
    }

    stack_types_.push(VT_DOUBLE);
}

void TranslatorVisitor::visitIntLiteralNode(IntLiteralNode* node) {
    switch (node->literal()) {
        case 0L:
            instruction(BC_ILOAD0, 0);
            break;
        case 1L:
            instruction(BC_ILOAD1, 0);
            break;
        case -1L:
            instruction(BC_ILOADM1, 0);
            break;
        default:
            instruction(BC_ILOAD, 0);
            bytecode().addInt64(node->literal());
            break;
    }

    stack_types_.push(VT_INT);
}

void TranslatorVisitor::visitWhileNode(WhileNode* node) {
    Label loop(&bytecode());
    Label end(&bytecode());

    bytecode().bind(loop);
    VarType type = eval(*node->whileExpr());
    convert(type, VT_INT);
    instruction(BC_ILOAD0, 1);
    bytecode().addBranch(BC_IFICMPE, end);

    type = eval(*node->loopBlock());
    convert(type, VT_VOID);

    bytecode().addBranch(BC_JA, loop);
    bytecode().bind(end);
}

void TranslatorVisitor::visitIfNode(IfNode* node) {
    Label afterThen(&bytecode());

    VarType type = eval(*node->ifExpr());
    convert(type, VT_INT);

    instruction(BC_ILOAD0, 1);
    bytecode().addBranch(BC_IFICMPE, afterThen);

    type = eval(*node->thenBlock());
    convert(type, VT_VOID);

    if (node->elseBlock() == nullptr) {
        bytecode().bind(afterThen);
        return;
    }

    Label afterElse(&bytecode());
    bytecode().addBranch(BC_JA, afterElse);
    bytecode().bind(afterThen);

    type = eval(*node->elseBlock());
    convert(type, VT_VOID);
    bytecode().bind(afterElse);
}

void TranslatorVisitor::visitPrintNode(PrintNode* node) {
    for (uint32_t i = 0; i < node->operands(); ++i) {
        VarType type = eval(*node->operandAt(i));

        Instruction print = BC_INVALID;
        switch (type) {
            case VT_DOUBLE:
                print = BC_DPRINT;
                break;
            case VT_INT:
                print = BC_IPRINT;
                break;
            case VT_STRING:
                print = BC_SPRINT;
                break;
            default:
                assert(false);
        }

        instruction(print, 1);
    }

}

void TranslatorVisitor::visitReturnNode(ReturnNode* node) {
    VarType returnType = function_scope_.top()->returnType();

    if (node->returnExpr() != nullptr) {
        VarType type = eval(*node->returnExpr());
        convert(type, returnType);
    }

    instruction(BC_RETURN, 0);
}

void TranslatorVisitor::visitStoreNode(StoreNode* node) {
    VarType type = eval(*node->value());
    VarType storeType = node->var()->type();
    convert(type, storeType);
    store(*node->var());
}

void TranslatorVisitor::visitLoadNode(LoadNode* node) {
    load(*node->var());
}

void TranslatorVisitor::visitBlockNode(BlockNode* node) {
    Scope* scope = node->scope();
    scopes_.push(scope);

    index_functions();

    auto var_iter = Scope::VarIterator(scope);
    while (var_iter.hasNext()) {
        index_variable(*var_iter.next());
    }

    auto function_iter = Scope::FunctionIterator(scope);
    while (function_iter.hasNext()) {
        auto function = function_iter.next();
        translateFunction(*function);
    }

    for (uint32_t i = 0; i < node->nodes(); ++i) {
        VarType type = eval(*node->nodeAt(i));
        convert(type, VT_VOID);
    }

    var_iter = Scope::VarIterator(scope);
    while (var_iter.hasNext()) {
        unindex_variable(*var_iter.next());
    }

    scopes_.pop();
}

void TranslatorVisitor::visitForNode(ForNode* node) {
    BinaryOpNode* range = node->inExpr()->asBinaryOpNode();
    AstVar const* counter = node->var();

    StoreNode startValue(-1, counter, range->left(), tASSIGN);
    visitStoreNode(&startValue);

    AstVar endRange = newVar(VT_INT);
    StoreNode endValue(-1, &endRange, range->right(), tASSIGN);
    visitStoreNode(&endValue);

    Label start(&bytecode());
    Label end(&bytecode());

    bytecode().bind(start);
    load(endRange);
    load(*counter);

    bytecode().addBranch(BC_IFICMPG, end);

    VarType type = eval(*node->body());
    convert(type, VT_VOID);

    StoreNode next(0, counter, new IntLiteralNode(0, 1L), tINCRSET);
    eval(next);

    bytecode().addBranch(BC_JA, start);
    bytecode().bind(end);
}

void TranslatorVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    if (node->kind() == tOR || node->kind() == tAND) {
        generate_lazy_binary_op(node);
        return;
    }

    if (node->kind() >= tEQ && node->kind() <= tLE) {
        generate_compare(node);
        return;
    }

    VarType left = eval(*node->left());
    VarType right = eval(*node->right());
    VarType common_type = unify_top(left, right);

    Instruction instruction = BC_INVALID;
    VarType resultType = common_type;
    switch (node->kind()) {
        case tAOR:
            instruction = BC_IAOR;
            break;
        case tAAND:
            instruction = BC_IAAND;
            break;
        case tAXOR:
            instruction = BC_IAXOR;
            break;
        case tADD:
            instruction = (common_type == VT_DOUBLE) ? BC_DSUB : BC_ISUB;
            break;
        case tSUB:
            instruction = (common_type == VT_DOUBLE) ? BC_DSUB : BC_ISUB;
            break;
        case tMUL:
            instruction = (common_type == VT_DOUBLE) ? BC_DMUL : BC_IMUL;
            break;
        case tDIV:
            instruction = (common_type == VT_DOUBLE) ? BC_DDIV : BC_IDIV;
            break;
        case tMOD:
            instruction = BC_IMOD;
            break;
        case tRANGE:
            assert(false);
            break;
        default:
            assert(false);
    }

    if (instruction == BC_INVALID) {
        assert(false);
    }

    this->instruction(instruction, 2);
    stack_types_.push(resultType);
}

void TranslatorVisitor::visitFunctionNode(FunctionNode* node) {
    assert(false);
}

void TranslatorVisitor::visitCallNode(CallNode* node) {
    auto handler = scopes_.top()->lookupFunction(node->name());

    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        VarType type = eval(*node->parameterAt(i));
        convert(type, handler->parameterType(i));
    }

    Instruction callInstruction =
            (handler->node()->body()->nodes() > 0 && handler->node()->body()->nodeAt(0)->isNativeCallNode()) ?
            BC_CALLNATIVE : BC_CALL;

    uint16_t id = reinterpret_cast<BytecodeFunction*>(handler->info())->id();
    instruction(callInstruction, node->parametersNumber());
    bytecode().addUInt16(id);

    VarType returnType = handler->returnType();
    stack_types_.push(returnType);
}

void TranslatorVisitor::visitNativeCallNode(NativeCallNode* node) {
    assert(false);
}

void TranslatorVisitor::generate_lazy_binary_op(BinaryOpNode *node) {
    VarType left = eval(*node->left());
    convert(left, VT_INT);
    Label afterFirst(&bytecode());
    Label afterSecond(&bytecode());

    instruction(BC_ILOAD0, 0);
    Instruction cmp = node->kind() == tAND ? BC_IFICMPE : BC_IFICMPNE;
    // if left part enough
    bytecode().addBranch(cmp, afterFirst);

    // evaluate right part
    VarType result = eval(*node->right());
    convert(result, VT_INT);
    bytecode().addBranch(BC_JA, afterSecond);

    bytecode().bind(afterFirst);
    instruction(node->kind() == tAND ? BC_ILOAD0 : BC_ILOAD1, 0);

    bytecode().bind(afterSecond);
}

void TranslatorVisitor::generate_compare(BinaryOpNode *node) {
    VarType left = eval(*node->left());
    VarType right = eval(*node->right());
    VarType resultType = unify_top(left, right);

    instruction(resultType == VT_INT ? BC_ICMP : BC_DCMP, 1);

    Instruction instruction = BC_INVALID;
    switch (node->kind()) {
        case tEQ:
            instruction = BC_IFICMPE;
            break;
        case tNEQ:
            instruction = BC_IFICMPNE;
            break;
        case tGT:
            instruction = BC_IFICMPL;
            break;
        case tGE:
            instruction = BC_IFICMPLE;
            break;
        case tLT:
            instruction = BC_IFICMPG;
            break;
        case tLE:
            instruction = BC_IFICMPGE;
            break;
        default:
            assert(false);
    }

    this->instruction(BC_ILOAD0, 0);
    Label t(&bytecode());
    Label end(&bytecode());

    bytecode().addBranch(instruction, t);
    this->instruction(BC_ILOAD0, 0);
    bytecode().addBranch(BC_JA, end);

    bytecode().bind(t);
    this->instruction(BC_ILOAD1, 0);

    bytecode().bind(end);
}

void TranslatorVisitor::translateFunction(AstFunction &function) {
    if (function.node()->body()->nodes() > 0 && function.node()->body()->nodeAt(0)->isNativeCallNode()) {
        return;
    }

    BytecodeFunction* current = reinterpret_cast<BytecodeFunction*>(function.info());

    function_scope_.push(current);
    stack_size_.push(0);

    for (uint32_t i = 0; i < function.parametersNumber(); ++i) {
        AstVar* variable = function.scope()->lookupVariable(function.parameterName(i), false);
        index_variable(*variable);
    }

    function.node()->body()->visit(this);

    for (uint32_t i = 0; i < function.parametersNumber(); ++i) {
        AstVar* variable = function.scope()->lookupVariable(function.parameterName(i), false);
        unindex_variable(*variable);
    }

    stack_size_.pop();
    function_scope_.pop();
}

void TranslatorVisitor::translate(AstFunction &top) {
    auto bytecode = new BytecodeFunction(&top);
    code_.addFunction(bytecode);
    top.setInfo(bytecode);

    translateFunction(top);
}

VarType TranslatorVisitor::eval(AstNode &node) {
    node.visit(this);
    if (stack_types_.size() == 0) {
        return VT_VOID;
    }

    return stack_types_.top();
}

void TranslatorVisitor::instruction(Instruction ins, int stack_pop) {
    function_scope_.top()->bytecode()->addInsn(ins);
    while (0 != stack_pop) {
        stack_types_.pop();
        --stack_pop;
    }
}

void TranslatorVisitor::convert(VarType from, VarType to) {
    if (from == to) {
        return;
    }

    if (to == VT_VOID) {
        instruction(BC_POP, 1);
        return;
    }

    if (from == VT_INT && to == VT_DOUBLE) {
        instruction(BC_I2D, 1);
        stack_types_.push(VT_DOUBLE);
        return;
    }

    if (from == VT_DOUBLE && to == VT_INT) {
        instruction(BC_D2I, 1);
        stack_types_.push(VT_INT);
        return;
    }

    if (from == VT_STRING && to == VT_INT) {
        instruction(BC_S2I, 1);
        stack_types_.push(VT_INT);
        return;
    }

    if (from == VT_STRING && to == VT_DOUBLE) {
        instruction(BC_S2I, 0);
        instruction(BC_I2D, 1);
        stack_types_.push(VT_DOUBLE);
        return;
    }

    assert(false);
}

void TranslatorVisitor::store(AstVar const& variable) {
    VarLocation location = locals_[variable.name()].top();
    bool outer_scope = location.scope == function_scope_.top()->id();

    Instruction storeInstruction = BC_INVALID;
    switch (variable.type()) {
        case VT_INT:
            storeInstruction = outer_scope ? BC_STORECTXIVAR : BC_STOREIVAR;
            break;
        case VT_DOUBLE:
            storeInstruction = outer_scope ? BC_STORECTXDVAR : BC_STOREDVAR;
            break;
        case VT_STRING:
            storeInstruction = outer_scope ? BC_STORECTXSVAR : BC_STORESVAR;
            break;
        default:
            assert(false);
    }

    instruction(storeInstruction, 1);
    if (outer_scope) {
        bytecode().addUInt16(location.scope);
        bytecode().addUInt16(location.local);
        return;
    }

    bytecode().addUInt16(location.local);
}

void TranslatorVisitor::load(AstVar const& variable) {
    VarLocation location = locals_[variable.name()].top();
    bool outer_scope = location.scope == function_scope_.top()->id();

    Instruction loadInstrction = BC_INVALID;
    switch (variable.type()) {
        case VT_INT:
            loadInstrction = outer_scope ? BC_LOADCTXIVAR : BC_LOADIVAR;
            stack_types_.push(VT_INT);
            break;
        case VT_DOUBLE:
            loadInstrction = outer_scope ? BC_LOADCTXDVAR : BC_LOADDVAR;
            stack_types_.push(VT_DOUBLE);
            break;
        case VT_STRING:
            loadInstrction = outer_scope ? BC_LOADCTXSVAR : BC_LOADSVAR;
            stack_types_.push(VT_STRING);
            break;
        default:
            assert(false);
    }

    instruction(loadInstrction, 0);
    if (outer_scope) {
        bytecode().addUInt16(location.scope);
        bytecode().addUInt16(location.local);
        return;
    }

    bytecode().addUInt16(location.local);
}

void TranslatorVisitor::index_variable(AstVar const& variable) {
    locals_[variable.name()].push(
            {function_scope_.top()->id(),
             static_cast<uint16_t>(stack_size_.top()++) });

    function_scope_.top()->setLocalsNumber(std::max(
            stack_size_.top(),
            function_scope_.top()->localsNumber())
    );
}

void TranslatorVisitor::unindex_variable(AstVar const& variable) {
    locals_[variable.name()].pop();
}

void TranslatorVisitor::index_functions() {
    auto function_iter = Scope::FunctionIterator(scopes_.top());

    while (function_iter.hasNext()) {
        auto function = function_iter.next();
        if (function->node()->body()->nodes() > 0 && function->node()->body()->nodeAt(0)->isNativeCallNode()) {
            code_.makeNativeFunction(function->name(), function->node()->signature(), 0);
        }

        auto bytecode = new BytecodeFunction(function);
        code_.addFunction(bytecode);
        function->setInfo(bytecode);
    }

}

AstVar TranslatorVisitor::newVar(VarType type) {
    static int index = 0;
    AstVar result = AstVar("tmp$" + to_string(index), VT_INT, nullptr);
    ++index;

    index_variable(result);
    return result;
}

Bytecode& TranslatorVisitor::bytecode() {
    return *function_scope_.top()->bytecode();
}

VarType TranslatorVisitor::unify_top(VarType left, VarType right) {
    if (left == right) {
        return left;
    }

    VarType result = left;

    if (left == VT_STRING) {
        AstVar tmp = newVar(VT_DOUBLE);
        store(tmp);
        convert(VT_STRING, VT_INT);
        load(tmp);

        result = VT_INT;
    }

    if (right == VT_STRING) {
        convert(VT_STRING, VT_INT);
        result = VT_INT;
    }

    if (right == VT_INT && left != VT_STRING) {
        convert(VT_INT, VT_DOUBLE);
        result = VT_DOUBLE;
    }

    if (left == VT_INT && right != VT_STRING) {
        AstVar tmp = newVar(VT_DOUBLE);
        store(tmp);
        convert(VT_INT, VT_DOUBLE);
        load(tmp);

        result = VT_DOUBLE;
    }

    return result;
}

}

Status* BytecodeTranslatorImpl::translateBytecode(const string& program, InterpreterCodeImpl* *code) {
    Parser parser;
    auto status = parser.parseProgram(program);
    if (status->isError()) {
        return status;
    }

    details::TranslatorVisitor visitor(**code);
    visitor.translate(*parser.top());
    return Status::Ok();
}

Status* BytecodeTranslatorImpl::translate(const string &program, Code **code) {
    auto result = new InterpreterCodeImpl();
    *code = result;
    return translateBytecode(program, &result);
}

}