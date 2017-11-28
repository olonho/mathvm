#include <unordered_map>

#include "q_bytecode_translator_impl.h"

#include "ast.h"
#include <iostream>
#include <dlfcn.h>
#include "code_impl.h"

using namespace std;

namespace mathvm {


void QBytecodeTranslatorImpl::processFunctions(Scope *scope, CodeImpl& code) {
    Scope::FunctionIterator it(scope);

    while (it.hasNext()) {
        auto node = it.next();
        auto function = new BytecodeFunction(node);
        function->setScopeId(code.getScope(node->scope()));
        code.addFunction(function);
        code.getScope(node->scope());
    }

    for (size_t i = 0; i < scope->childScopeNumber(); i++) {
        processFunctions(scope->childScopeAt(i), code);
    }
}

Status* QBytecodeTranslatorImpl::translate(const string& program, Code* *code) { // 100
    Parser *parser = new Parser();
    Status* status = parser->parseProgram(program);

    if (status->isError()) {
        return status;
    }

    *code = new CodeImpl();
    BytecodeVisitor* visitor = new BytecodeVisitor(*dynamic_cast<CodeImpl*>(*code));

    processFunctions(parser->top()->scope(), *dynamic_cast<CodeImpl*>(*code));
    parser->top()->node()->visit(visitor);
    //cout << "****" << endl;
    //(*code)->disassemble();

    return status;
}

BytecodeVisitor::BytecodeVisitor(CodeImpl& code) : _code(code) {
}

Bytecode* BytecodeVisitor::bytecode() {
    return _stack.top()->bytecode();
}

VarType BytecodeVisitor::returnType() {
    return _stack.top()->returnType();
}

uint16_t BytecodeVisitor::functionId() {
    return _stack.top()->id();
}

#define CMP(cmd) \
    _type.pop(); \
    _type.pop(); \
    _type.push(VT_INT); \
\
    Label push0(bytecode()); \
    Label out(bytecode()); \
    if (lower == VT_INT) { \
        bytecode()->addBranch(cmd, push0); \
    } else { \
        bytecode()->addInsn(BC_DCMP); \
        bytecode()->addInsn(BC_ILOAD0); \
        bytecode()->addInsn(BC_SWAP); \
        bytecode()->addBranch(cmd, push0); \
    } \
\
    bytecode()->addInsn(BC_ILOAD1); \
    bytecode()->addBranch(BC_JA, out); \
    bytecode()->bind(push0); \
    bytecode()->addInsn(BC_ILOAD0); \
    bytecode()->bind(out);

void BytecodeVisitor::visitBinaryOpNode(BinaryOpNode *node) {
    node->right()->visit(this);
    node->left()->visit(this);

    VarType upper = _type.top();
    _type.pop();
    VarType lower = _type.top();
    _type.pop();

    if (node->kind() != tRANGE) {
        if (upper != lower) {
            if (lower == VT_INT) {
                bytecode()->addInsn(BC_SWAP);
                bytecode()->addInsn(BC_I2D);
                bytecode()->addInsn(BC_SWAP);
            } else {
                bytecode()->addInsn(BC_I2D);
            }
            lower = VT_DOUBLE;
        }

        _type.push(lower);
        _type.push(upper);
    }

    switch (node->kind())
    {
        case tRANGE: break;

        case tEQ: {
            CMP(BC_IFICMPNE)
        }
        break;

        case tNEQ: {
            CMP(BC_IFICMPE)
        }
        break;

        case tLT: {
            CMP(BC_IFICMPGE)
        }
        break;

        case tLE: {
            CMP(BC_IFICMPG)
        }
        break;

        case tGT: {
            CMP(BC_IFICMPLE)
        }
        break;

        case tGE: {
            CMP(BC_IFICMPL)
        }
        break;

        case tOR: {
            _type.pop();
            {
                Label push0(bytecode());
                Label out(bytecode());
                bytecode()->addInsn(BC_ILOAD0);
                bytecode()->addBranch(BC_IFICMPE, push0);
                bytecode()->addInsn(BC_ILOAD1);
                bytecode()->addBranch(BC_JA, out);
                bytecode()->bind(push0);
                bytecode()->addInsn(BC_ILOAD0);
                bytecode()->bind(out);
            }
            bytecode()->addInsn(BC_SWAP);
            {
                Label push0(bytecode());
                Label out(bytecode());
                bytecode()->addInsn(BC_ILOAD0);
                bytecode()->addBranch(BC_IFICMPE, push0);
                bytecode()->addInsn(BC_ILOAD1);
                bytecode()->addBranch(BC_JA, out);
                bytecode()->bind(push0);
                bytecode()->addInsn(BC_ILOAD0);
                bytecode()->bind(out);
            }

            bytecode()->addInsn(BC_IAOR);
        }
        break;

        case tAND: {
            _type.pop();

            {
                Label push0(bytecode());
                Label out(bytecode());
                bytecode()->addInsn(BC_ILOAD0);
                bytecode()->addBranch(BC_IFICMPE, push0);
                bytecode()->addInsn(BC_ILOAD1);
                bytecode()->addBranch(BC_JA, out);
                bytecode()->bind(push0);
                bytecode()->addInsn(BC_ILOAD0);
                bytecode()->bind(out);
            }
            bytecode()->addInsn(BC_SWAP);
            {
                Label push0(bytecode());
                Label out(bytecode());
                bytecode()->addInsn(BC_ILOAD0);
                bytecode()->addBranch(BC_IFICMPE, push0);
                bytecode()->addInsn(BC_ILOAD1);
                bytecode()->addBranch(BC_JA, out);
                bytecode()->bind(push0);
                bytecode()->addInsn(BC_ILOAD0);
                bytecode()->bind(out);
            }

            bytecode()->addInsn(BC_IAAND);
        }
        break;

        case tAOR: {
            _type.pop();
            bytecode()->addInsn(BC_IAOR);
        }
        break;

        case tAAND: {
            _type.pop();
            bytecode()->addInsn(BC_IAAND);
        }
        break;

        case tAXOR: {
            _type.pop();
            bytecode()->addInsn(BC_IAXOR);
        }
        break;

        case tADD: {
            _type.pop();
            bytecode()->addInsn(lower == VT_INT ? BC_IADD : BC_DADD);
        }
        break;

        case tSUB: {
            _type.pop();
            bytecode()->addInsn(lower == VT_INT ? BC_ISUB : BC_DSUB);
        }
        break;

        case tMUL:
        {
            _type.pop();
            bytecode()->addInsn(lower == VT_INT ? BC_IMUL : BC_DMUL);
        }
        break;

        case tDIV:
        {
            _type.pop();
            bytecode()->addInsn(lower == VT_INT ? BC_IDIV : BC_DDIV);
        }
        break;

        case tMOD:
        {
            _type.pop();
            bytecode()->addInsn(BC_IMOD);
        }
        break;

        default: assert(false);
    }

}

void BytecodeVisitor::visitBlockNode(BlockNode *node) {

    for (Scope::FunctionIterator it(node->scope()); it.hasNext();) {
        AstFunction *foo = it.next();
        foo->node()->visit(this);
    }

    for (uint32_t i = 0; i < node->nodes(); ++i) {
        AstNode *cld = node->nodeAt(i);
        cld->visit(this);

        if (cld->isCallNode()
                && _code.functionByName(cld->asCallNode()->name())->returnType() != VT_VOID) {
            bytecode()->addInsn(BC_POP);
        }
    }

    _code.addDependencies(functionId(), _code.getScope(node->scope()));
}

void BytecodeVisitor::visitCallNode(CallNode *node) {
    auto function = _code.functionByName(node->name());

    for (uint32_t i = node->parametersNumber(); i > 0; i--) {
        node->parameterAt(i - 1)->visit(this);
        if (_type.top() != function->parameterType(i - 1))
        {
            switch (_type.top())
            {
                case VT_INT: bytecode()->addInsn(BC_I2D); break;
                case VT_DOUBLE: bytecode()->addInsn(BC_D2I); break;
                case VT_STRING: bytecode()->addInsn(BC_S2I); break;
                default: break;
            }
        }
        _type.pop();
    }

    bytecode()->addInsn(BC_CALL);
    bytecode()->addTyped(function->id());
    if (function->returnType() != VT_VOID) {
        _type.push(function->returnType());
    }
}

void BytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    bytecode()->addInsn(BC_DLOAD);
    bytecode()->addTyped(node->literal());
    _type.push(VT_DOUBLE);
}

void BytecodeVisitor::visitForNode(ForNode *node) {
    Label body(bytecode());
    Label condition(bytecode());

    uint16_t scope_id = _code.getScope(node->var()->owner());
    uint16_t var_id = _code.getVar(scope_id, node->var());

    BinaryOpNode* range = node->inExpr()->asBinaryOpNode();

    range->right()->visit(this);
    _type.pop();
    range->left()->visit(this);
    _type.pop();

    bytecode()->addBranch(BC_JA, condition);
    bytecode()->bind(body);

    bytecode()->addInsn(BC_LOADIVAR0);
    bytecode()->addInsn(BC_STORECTXIVAR);
    bytecode()->addTyped(scope_id);
    bytecode()->addTyped(var_id);

    node->body()->visit(this);

    bytecode()->addInsn(BC_ILOAD1);
    bytecode()->addInsn(BC_IADD);


    bytecode()->bind(condition);

    bytecode()->addInsn(BC_STOREIVAR0);
    bytecode()->addInsn(BC_STOREIVAR1);
    bytecode()->addInsn(BC_LOADIVAR1);
    bytecode()->addInsn(BC_LOADIVAR0);
    bytecode()->addInsn(BC_LOADIVAR1);
    bytecode()->addInsn(BC_LOADIVAR0);

    bytecode()->addBranch(BC_IFICMPLE, body);

    bytecode()->addInsn(BC_POP);
    bytecode()->addInsn(BC_POP);
}

static void* get_native(const char* name)
{
    static void *handle = dlopen("libc.so", RTLD_LAZY);
    auto f = dlsym(handle, name);
    return f == NULL ? dlsym(NULL, name) : f;
}

void BytecodeVisitor::visitFunctionNode(FunctionNode *node) {
    auto ast_function = new AstFunction(node, node->body()->scope());

    auto function = dynamic_cast<BytecodeFunction*>(_code.functionByName(node->name()));
    _stack.push(function);


    if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
        for (uint32_t i = 0; i < node->parametersNumber(); i++) {
            auto var = node->body()->scope()->parent()->lookupVariable(ast_function->parameterName(i), false);
            uint16_t scope_id = _code.getScope(var->owner());
            (void)scope_id;
        }
    } else {
        for (uint32_t i = 0; i < node->parametersNumber(); i++) {
            auto var = node->body()->scope()->parent()->lookupVariable(ast_function->parameterName(i), false);
            uint16_t scope_id = _code.getScope(var->owner());
            uint16_t var_id = _code.getVar(scope_id, var);
            switch (node->parameterType(i))
            {
                case VT_INT: bytecode()->addInsn(BC_STORECTXIVAR); break;
                case VT_DOUBLE: bytecode()->addInsn(BC_STORECTXDVAR); break;
                case VT_STRING: bytecode()->addInsn(BC_STORECTXSVAR); break;
                default: assert(false);
            }

            bytecode()->addTyped(scope_id);
            bytecode()->addTyped(var_id);
        }
    }

    node->visitChildren(this);
    _stack.pop();
}

void BytecodeVisitor::visitIfNode(IfNode *node) {
    Label out(bytecode());
    Label el(bytecode());

    node->ifExpr()->visit(this);
    _type.pop();
    bytecode()->addInsn(BC_ILOAD0);

    if (node->elseBlock()) {
        bytecode()->addBranch(BC_IFICMPE, el);
        node->thenBlock()->visit(this);
        bytecode()->addBranch(BC_JA, out);
        bytecode()->bind(el);
        node->elseBlock()->visit(this);
        bytecode()->bind(out);
    } else {
        bytecode()->addBranch(BC_IFICMPE, out);
        node->thenBlock()->visit(this);
        bytecode()->bind(out);
    }
}

void BytecodeVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    bytecode()->addInsn(Instruction::BC_ILOAD);
    bytecode()->addTyped(node->literal());
    _type.push(VT_INT);
}

void BytecodeVisitor::visitLoadNode(LoadNode *node) {
    _type.push(node->var()->type());

    switch (_type.top())
    {
        case VT_INT: bytecode()->addInsn(BC_LOADCTXIVAR); break;
        case VT_DOUBLE: bytecode()->addInsn(BC_LOADCTXDVAR); break;
        case VT_STRING: bytecode()->addInsn(BC_LOADCTXSVAR); break;
        default: assert(false);
    }

    uint16_t scope_id = _code.getScope(node->var()->owner());
    uint16_t var_id = _code.getVar(scope_id, node->var());

    bytecode()->addTyped(scope_id);
    bytecode()->addTyped(var_id);
}

void BytecodeVisitor::visitNativeCallNode(NativeCallNode *node) {
    auto id = _code.makeNativeFunction(node->nativeName(), node->nativeSignature(), get_native(node->nativeName().c_str()));
    bytecode()->addInsn(BC_CALLNATIVE);
    bytecode()->addTyped(id);

    if (node->nativeSignature()[0].first != VT_VOID) {
        _type.push(node->nativeSignature()[0].first);
    }
}

void BytecodeVisitor::visitPrintNode(PrintNode *node) {
    for (uint32_t i = 0; i < node->operands(); i++) {
        node->operandAt(i)->visit(this);
        switch (_type.top())
        {
            case VT_INT: bytecode()->addInsn(BC_IPRINT); break;
            case VT_DOUBLE: bytecode()->addInsn(BC_DPRINT); break;
            case VT_STRING: bytecode()->addInsn(BC_SPRINT); break;
            default: break;
        }
        _type.pop();
    }
}

void BytecodeVisitor::visitReturnNode(ReturnNode *node) {
    node->visitChildren(this);

    if (node->returnExpr() && returnType() != _type.top()) {
        switch (_type.top())
        {
            case VT_INT: bytecode()->addInsn(BC_I2D); break;
            case VT_DOUBLE: bytecode()->addInsn(BC_D2I); break;
            case VT_STRING: bytecode()->addInsn(BC_S2I); break;
            default: assert(false);
        }
    }

    if (node->returnExpr()) {
        _type.pop();
    }

    bytecode()->addInsn(Instruction::BC_RETURN);
}

void BytecodeVisitor::visitStoreNode(StoreNode *node) {
    node->value()->visit(this);

    if (node->var()->type() != _type.top()) {
        switch (_type.top()) {
            case VT_INT: bytecode()->addInsn(BC_I2D); break;
            case VT_DOUBLE: bytecode()->addInsn(BC_D2I); break;
            case VT_STRING: bytecode()->addInsn(BC_S2I); break;
            default: assert(false);
        }
    }

    uint16_t scope_id = _code.getScope(node->var()->owner());
    uint16_t var_id = _code.getVar(scope_id, node->var());

    switch (node->op())
    {
        case tINCRSET:
            switch (_type.top())
            {
                case VT_INT:
                    bytecode()->addInsn(BC_LOADCTXIVAR);
                    bytecode()->addTyped(scope_id);
                    bytecode()->addTyped(var_id);
                    bytecode()->addInsn(BC_IADD);
                    break;
                case VT_DOUBLE:
                    bytecode()->addInsn(BC_LOADCTXDVAR);
                    bytecode()->addTyped(scope_id);
                    bytecode()->addTyped(var_id);
                    bytecode()->addInsn(BC_DADD);
                    break;
                case VT_STRING: assert(false);
                default: assert(false);
            }
            break;


        case tDECRSET:
            switch (_type.top())
            {
                case VT_INT:
                    bytecode()->addInsn(BC_LOADCTXIVAR);
                    bytecode()->addTyped(scope_id);
                    bytecode()->addTyped(var_id);
                    bytecode()->addInsn(BC_ISUB);
                    break;
                case VT_DOUBLE:
                    bytecode()->addInsn(BC_LOADCTXDVAR);
                    bytecode()->addTyped(scope_id);
                    bytecode()->addTyped(var_id);
                    bytecode()->addInsn(BC_DSUB);
                    break;
                case VT_STRING: assert(false);
                default: assert(false);
            }
            break;
        case tASSIGN: break;
        default: assert(false);
    }

    switch (_type.top())
    {
        case VT_INT: bytecode()->addInsn(BC_STORECTXIVAR); break;
        case VT_DOUBLE: bytecode()->addInsn(BC_STORECTXDVAR); break;
        case VT_STRING: bytecode()->addInsn(BC_STORECTXSVAR); break;
        default: assert(false);
    }

    bytecode()->addTyped(scope_id);
    bytecode()->addTyped(var_id);

    _type.pop();
}

void BytecodeVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    uint16_t id = _code.makeStringConstant(node->literal());
    bytecode()->addInsn(BC_SLOAD);
    bytecode()->addTyped(id);
    _type.push(VT_STRING);
}

void BytecodeVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    node->visitChildren(this);

    switch (node->kind())
    {
        case mathvm::tNOT: {
            _type.pop();

            Label push0(bytecode());
            Label out(bytecode());
            bytecode()->addInsn(BC_ILOAD0);
            bytecode()->addBranch(BC_IFICMPE, push0);
            bytecode()->addInsn(BC_ILOAD1);
            bytecode()->addBranch(BC_JA, out);
            bytecode()->bind(push0);
            bytecode()->addInsn(BC_ILOAD0);
            bytecode()->bind(out);


            bytecode()->addInsn(BC_ILOAD1);
            bytecode()->addInsn(BC_IAXOR);
            _type.push(VT_INT);
        }
        break;

        case mathvm::tSUB:
            switch (_type.top())
            {
                case VT_INT: bytecode()->addInsn(BC_INEG); break;
                case VT_DOUBLE: bytecode()->addInsn(BC_DNEG); break;
                default: assert(false);
            }
            break;

        default: assert(false);
    }
}

void BytecodeVisitor::visitWhileNode(WhileNode *node) {
    Label begin(bytecode());
    Label finish(bytecode());

    bytecode()->bind(begin);

    node->whileExpr()->visit(this);

    bytecode()->addInsn(BC_ILOAD0);
    bytecode()->addBranch(BC_IFICMPE, finish);

    node->loopBlock()->visit(this);

    bytecode()->addBranch(BC_JA, begin);
    bytecode()->bind(finish);
    _type.pop();
}

}
