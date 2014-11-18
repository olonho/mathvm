#include "visitors.h"
#include "parser.h"
#include "rich_function.h"
#include "interpreter_code_impl.h"
#include <stack>

using namespace mathvm;

class BytecodeMainVisitor: public AstVisitor {
public:
    BytecodeMainVisitor(Code *code, AstFunction *topFunction): _code(code) {
        //TODO set the first 0 to actual value of the topmost function
        _code->addFunction(_currentFunction = new RichFunction(topFunction, 0, 0));

        _typeTokenInstruction[VT_DOUBLE][tADD] = BC_DADD;
        _typeTokenInstruction[VT_INT][tADD] = BC_IADD;
        _typeTokenInstruction[VT_DOUBLE][tSUB] = BC_DSUB;
        _typeTokenInstruction[VT_INT][tSUB] = BC_ISUB;
        _typeTokenInstruction[VT_DOUBLE][tMUL] = BC_DMUL;
        _typeTokenInstruction[VT_INT][tMUL] = BC_IMUL;
        _typeTokenInstruction[VT_DOUBLE][tDIV] = BC_DDIV;
        _typeTokenInstruction[VT_INT][tDIV] = BC_IDIV;

        _typeTokenInstruction[VT_INT][tAOR] = BC_IAOR;
        _typeTokenInstruction[VT_INT][tAND] = BC_IAAND;
        _typeTokenInstruction[VT_INT][tAXOR] = BC_IAXOR;

        _typeTokenInstruction[VT_INT][tEQ] = BC_IFICMPE;
        _typeTokenInstruction[VT_INT][tNEQ] = BC_IFICMPNE;
        _typeTokenInstruction[VT_INT][tGT] = BC_IFICMPG;
        _typeTokenInstruction[VT_INT][tGE] = BC_IFICMPGE;
        _typeTokenInstruction[VT_INT][tLT] = BC_IFICMPL;
        _typeTokenInstruction[VT_INT][tLE] = BC_IFICMPLE;
    }

    virtual void visitBinaryOpNode(BinaryOpNode *node) {
        TokenKind operation = node->kind();
        switch (operation) {
        case (tOR):
        case (tAND): {
            binary_logic(node);
            break;
        }
        case (tADD):
        case (tSUB):
        case (tMUL):
        case (tDIV): {
            node->right()->visit(this);
            node->left()->visit(this);
            binary_math(node->kind());
            break;
        }
        case (tEQ):
        case (tNEQ):
        case (tGT):
        case (tGE):
        case (tLT):
        case (tLE): {
            binary_comparsion(node);
            break;
        }
        default: {
            assert(0);
        }
        }
    }

    virtual void visitUnaryOpNode(UnaryOpNode *node) {
        node->operand()->visit(this);
        unary_math(node->kind());
    }

    virtual void visitStringLiteralNode(StringLiteralNode *node) {
        uint16_t constantId = _code->makeStringConstant(node->literal());
        _currentFunction->bytecode()->addInsn(BC_SLOAD);
        _currentFunction->bytecode()->addInt16(constantId);
        _typesStack.push(VT_STRING);
    }

    virtual void visitDoubleLiteralNode(DoubleLiteralNode *node) {
        _currentFunction->bytecode()->addInsn(BC_DLOAD);
        _currentFunction->bytecode()->addInt64(node->literal());
        _typesStack.push(VT_DOUBLE);
    }

    virtual void visitIntLiteralNode(IntLiteralNode *node) {
        _currentFunction->bytecode()->addInsn(BC_ILOAD);
        _currentFunction->bytecode()->addInt64(node->literal());
        _typesStack.push(VT_INT);
    }

    virtual void visitLoadNode(LoadNode *node) {
        loadVariable(node->var());
    }

    virtual void visitStoreNode(StoreNode *node) {
        node->value()->visit(this);

        switch (node->op()) {
        case (tASSIGN): {
            storeToVariable(node->var());
            break;
        }
        case (tINCRSET): {
            loadVariable(node->var());
            binary_math(tADD);
            storeToVariable(node->var());
            break;
        }
        case (tDECRSET): {
            loadVariable(node->var());
            binary_math(tSUB);
            storeToVariable(node->var());
            break;
        }
        default: {
            assert(0);
        }
        }
    }

    virtual void visitForNode(ForNode *node) {
        assert(0);
    }

    virtual void visitWhileNode(WhileNode *node) {
        assert(0);
    }

    virtual void visitIfNode(IfNode *node) {
        node->ifExpr()->visit(this);

        pushInt0();

        Label afterTrue(_currentFunction->bytecode());
        _currentFunction->bytecode()->addBranch(BC_IFICMPE, afterTrue);

        pop();
        pop();
        node->thenBlock()->visit(this);

        Label afterFalse(_currentFunction->bytecode());
        if (node->elseBlock()) {
            _currentFunction->bytecode()->addBranch(BC_JA, afterFalse);
        }
        afterTrue.bind(_currentFunction->bytecode()->current());
        if (node->elseBlock()) {
            pop();
            pop();
            node->elseBlock()->visit(this);
            afterFalse.bind(_currentFunction->bytecode()->current());
        }
    }

    virtual void visitBlockNode(BlockNode *node) {
        //variables:
        Scope::VarIterator it(node->scope());
        while (it.hasNext()) {
            AstVar *var = it.next();
            //TODO: this will override value with same name in current fucntion's scope
            _currentFunction->addLocalVariable(var->name(), var->type());
        }

        //functions:
        {
            Scope::FunctionIterator it(node->scope());
            while (it.hasNext()) {
                it.next()->node()->visit(this);
            }
        }

        node->visitChildren(this);
    }

    virtual void visitFunctionNode(FunctionNode *node) {
        assert(0);
    }

    virtual void visitReturnNode(ReturnNode *node) {
        assert(0);
    }

    virtual void visitCallNode(CallNode *node) {
        assert(0);
    }

    virtual void visitNativeCallNode(NativeCallNode *node) {
        assert(0);
    }

    virtual void visitPrintNode(PrintNode *node) {
        for (uint32_t i = 0; i < node->operands(); ++i) {
            node->operandAt(i)->visit(this);
            Instruction emitted = BC_INVALID;
            switch (_typesStack.top()) {
            case VT_DOUBLE: {
                emitted = BC_DPRINT;
                break;
            }
            case VT_INT: {
                emitted = BC_DPRINT;
                break;
            }
            case VT_STRING: {
                emitted = BC_SPRINT;
                break;
            }
            default: {
                assert(0);
            }
            }
            _typesStack.pop();
        }
    }

private:
    Code *_code;
    RichFunction *_currentFunction;
    stack<VarType> _typesStack;
    map<VarType, map<TokenKind, Instruction> > _typeTokenInstruction;

    void emit(Instruction inst) {
        _currentFunction->bytecode()->addInsn(inst);
    }

    bool convertTOS(VarType toType) {
        switch (toType) {
        case (VT_INT):  {
            switch (_typesStack.top()) {
            case (VT_DOUBLE): {
                emit(BC_D2I);
                _typesStack.pop();
                _typesStack.push(VT_INT);
                return true;
            }
            case (VT_STRING): {
                emit(BC_S2I);
                _typesStack.pop();
                _typesStack.push(VT_INT);
                return true;
            }
            default: {
                return false;
            }
            }
        }
        case (VT_DOUBLE): {
            switch (_typesStack.top()) {
            case (VT_INT): {
                emit(BC_I2D);
                _typesStack.pop();
                _typesStack.push(VT_DOUBLE);
                return true;
            }
            default: {
                return false;
            }
            }
        }
        default: {
            return false;
        }
        }
        return false;
    }

    void assertTOS(VarType type) {
        assert(_typesStack.top() == type);
    }

    void loadVariable(const AstVar *astVar) {
        const string &variableName = astVar->name();
        RichFunction *actualFunction = _currentFunction->lookupParentFunction(variableName);
        uint16_t variableId = actualFunction->getVariableId(variableName);
        uint16_t functionIndex = actualFunction->getIndex();

        Instruction emitted = BC_INVALID;
        switch (astVar->type()) {
        case VT_DOUBLE: {
            emitted = BC_LOADCTXDVAR;
            _typesStack.push(VT_DOUBLE);
            break;
        }
        case VT_INT: {
            emitted = BC_LOADCTXIVAR;
            _typesStack.push(VT_INT);
            break;
        }
        case VT_STRING: {
            emitted = BC_LOADCTXSVAR;
            _typesStack.push(VT_STRING);
            break;
        }
        default: {
            assert(0);
        }
        }

        emit(emitted);
        _currentFunction->bytecode()->addInt16(functionIndex);
        _currentFunction->bytecode()->addInt16(variableId);
    }

    void storeToVariable(const AstVar *astVar) {
        const string &variableName = astVar->name();
        RichFunction *actualFunction = _currentFunction->lookupParentFunction(variableName);
        uint16_t variableId = actualFunction->getVariableId(variableName);
        uint16_t functionIndex = actualFunction->getIndex();

        switch (astVar->type()) {
        case VT_DOUBLE: {
            convertTOS(VT_DOUBLE);
            emit(BC_STORECTXDVAR);
            break;
        }
        case VT_INT: {
            convertTOS(VT_INT);
            emit(BC_STORECTXIVAR);
            break;
        }
        case VT_STRING: {
            assertTOS(VT_STRING);
            emit(BC_STORECTXSVAR);
            break;
        }
        default: {
            assert(0);
        }
        }

        _currentFunction->bytecode()->addInt16(functionIndex);
        _currentFunction->bytecode()->addInt16(variableId);

        _typesStack.pop();
    }

    void pop() {
        emit(BC_POP);
        _typesStack.pop();
        emit(BC_POP);
        _typesStack.pop();
    }

    void pushInt0() {
        emit(BC_ILOAD0);
        _typesStack.push(VT_INT);
    }

    void pushInt1() {
        emit(BC_ILOAD1);
        _typesStack.push(VT_INT);
    }

    //TODO: Implement for DOUBLES too (now only for INTs)
    void binary_comparsion(BinaryOpNode *node) {
        TokenKind operation = node->kind();
        assert(operation == tEQ || operation == tNEQ ||  operation == tGT
               || operation == tGE || operation == tLT || operation == tLE);

        node->left()->visit(this);
        node->right()->visit(this);

        VarType first = _typesStack.top();
        _typesStack.pop();
        VarType second = _typesStack.top();
        _typesStack.pop();
        assert(first == VT_INT && second == VT_INT);

        Label beforeTrue(_currentFunction->bytecode());
        Label afterTrue(_currentFunction->bytecode());
        Label afterFalse(_currentFunction->bytecode());
        _currentFunction->bytecode()->addBranch(_typeTokenInstruction[VT_INT][operation], beforeTrue);
        _currentFunction->bytecode()->addBranch(BC_JA, afterTrue);
        beforeTrue.bind(_currentFunction->bytecode()->current());
        pushInt1();
        _currentFunction->bytecode()->addBranch(BC_JA, afterFalse);
        afterTrue.bind(_currentFunction->bytecode()->current());
        pushInt0();
        afterFalse.bind(_currentFunction->bytecode()->current());
    }

    void binary_logic(BinaryOpNode *node) {
        TokenKind operation = node->kind();
        assert(operation == tAND || operation == tOR);

        node->left()->visit(this);
        assert(_typesStack.top() == VT_INT);

        switch (operation) {
        case (tAND): {
            //A && B
            //if (A == false) {
            Label afterTrue(_currentFunction->bytecode());
            Label afterFalse(_currentFunction->bytecode());
            pushInt0();
            _currentFunction->bytecode()->addBranch(BC_IFICMPNE, afterTrue);
            pop();
            pop();
            pushInt0();
            _currentFunction->bytecode()->addBranch(BC_JA, afterFalse);
            //}
            // else {
            afterTrue.bind(_currentFunction->bytecode()->current());
            pop();
            pop();
            //return B;
            node->right()->visit(this);
            assert(_typesStack.top() == VT_INT);
            afterFalse.bind(_currentFunction->bytecode()->current());
            // }
            break;
        }
        case (tOR): {
            //A || B
            //if (A == true) {
            Label afterTrue(_currentFunction->bytecode());
            Label afterFalse(_currentFunction->bytecode());
            pushInt1();
            _currentFunction->bytecode()->addBranch(BC_IFICMPNE, afterTrue);
            pop();
            pop();
            pushInt1();
            _currentFunction->bytecode()->addBranch(BC_JA, afterFalse);
            //}
            // else {
            afterTrue.bind(_currentFunction->bytecode()->current());
            pop();
            pop();
            //return B;
            node->right()->visit(this);
            assert(_typesStack.top() == VT_INT);
            afterFalse.bind(_currentFunction->bytecode()->current());
            // }
            break;
        }
        default: {
            assert(0);
        }
        }
    }

    void binary_math(TokenKind operation) {
        VarType first = _typesStack.top();
        _typesStack.pop();
        VarType second = _typesStack.top();
        _typesStack.pop();

        switch (first) {
        case (VT_DOUBLE): {
            switch (second) {
            case (VT_DOUBLE): {
                switch (operation) {
                case (tADD):
                case (tSUB):
                case (tMUL):
                case (tDIV): {
                    emit(_typeTokenInstruction[VT_DOUBLE][operation]);
                    break;
                }
                default: {
                    assert(0);
                }
                }
                _typesStack.push(VT_DOUBLE);
                break;
            }
            case (VT_INT): {
                emit(BC_SWAP);
                _typesStack.push(VT_INT);
                convertTOS(VT_DOUBLE);
                _typesStack.pop();
                emit(BC_SWAP);

                switch (operation) {
                case (tADD):
                case (tSUB):
                case (tMUL):
                case (tDIV): {
                    emit(_typeTokenInstruction[VT_DOUBLE][operation]);
                    break;
                }
                default: {
                    assert(0);
                }
                }

                _typesStack.push(VT_DOUBLE);
                break;
            }
            default: {
                assert(0);
            }
            }
            break;
        }
        case (VT_INT): {
            switch (second) {
            case (VT_DOUBLE): {
                _typesStack.push(VT_INT);
                convertTOS(VT_DOUBLE);
                _typesStack.pop();
                switch (operation) {
                case (tADD):
                case (tSUB):
                case (tMUL):
                case (tDIV): {
                    emit(_typeTokenInstruction[VT_DOUBLE][operation]);
                    break;
                }
                default: {
                    assert(0);
                }
                }
                _typesStack.push(VT_DOUBLE);
                break;
            }
            case (VT_INT): {
                switch (operation) {
                case (tADD):
                case (tSUB):
                case (tMUL):
                case (tDIV):
                case (tAAND):
                case (tAOR):
                case (tAXOR): {
                    emit(_typeTokenInstruction[VT_INT][operation]);
                    break;
                }
                default: {
                    assert(0);
                }
                }
                _typesStack.push(VT_INT);
                break;
            }
            default: {
                assert(0);
            }
            }
            break;
        }
        default: {
            assert(0);
        }
        }
    }

    void unary_math(TokenKind operation) {
        switch (_typesStack.top()) {
        case (VT_DOUBLE): {
            switch (operation) {
            case (tSUB): {
                emit(BC_DLOAD0);
                _typesStack.push(VT_DOUBLE);
                binary_math(tSUB);
                break;
            }
            case (tNOT): {
                emit(BC_DNEG);
                break;
            }
            default: {
                assert(0);
            }
            }
            break;
        }

        case (VT_INT): {
            switch (operation) {
            case (tSUB): {
                emit(BC_ILOAD0);
                _typesStack.push(VT_INT);
                binary_math(tSUB);
                break;
            }
            case (tNOT): {
                emit(BC_INEG);
                break;
            }
            default: {
                assert(0);
            }
            }
            break;
        }

        default: {
            assert(0);
        }
        }
    }
};

Status *BytecodeTranslatorImpl::translate(const string &program, Code **code)  {
    Parser parser;
    Status *status = parser.parseProgram(program);
    if (status && status->isError()) {
        return status;
    }

    BytecodeMainVisitor visitor(*code = new InterpreterCodeImpl(), parser.top());
    parser.top()->node()->body()->visit(&visitor);

    return Status::Ok();
}

Translator *Translator::create(const string &impl) {
    return new BytecodeTranslatorImpl();
}