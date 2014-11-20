#include "visitors.h"
#include "parser.h"
#include "rich_function.h"
#include "interpreter_code_impl.h"
#include "function_crawler.h"
#include <stack>

using namespace mathvm;

class BytecodeMainVisitor: public AstVisitor {
public:
    BytecodeMainVisitor(Code *code, AstFunction *topFunction): _code(code) {
        _code->addFunction(_currentFunction = new RichFunction(topFunction));
        FunctionCrawler crawler(code);
        topFunction->node()->body()->visit(&crawler);

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

    //TODO: re-think assuming binary logic
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
            node->elseBlock()->visit(this);
            afterFalse.bind(_currentFunction->bytecode()->current());
        }
    }

    virtual void visitBlockNode(BlockNode *node) {
        _scopeToFuncitonMap[node->scope()] = _currentFunction->id();

        //variables:
        Scope::VarIterator it(node->scope());
        while (it.hasNext()) {
            AstVar *var = it.next();
            //TODO: this will override value with same name in current fucntion's scope
            _currentFunction->addLocalVariable(var->name());
        }

        //functions:
        {
            RichFunction *parentFunction = _currentFunction;
            Scope::FunctionIterator it(node->scope());
            while (it.hasNext()) {
                AstFunction *currentAstFunction = it.next();
                _currentFunction = dynamic_cast<RichFunction *>(_code->functionByName(currentAstFunction->name()));
                assert(_currentFunction != 0);
                _scopeToFuncitonMap[currentAstFunction->scope()] = _currentFunction->id();
                currentAstFunction->node()->visit(this);
            }
            _currentFunction = parentFunction;
        }

        node->visitChildren(this);
    }

    //TODO: add native calls
    virtual void visitFunctionNode(FunctionNode *node) {
        uint16_t adjustment = node->parametersNumber() - 1;
        for (uint16_t i = 0; i < node->parametersNumber(); ++i) {
            switch (node->parameterType(adjustment - i)) {
            case (VT_INVALID): { //for () -> Type functions
                continue;
            }
            case (VT_INT): {
                emit(BC_STOREIVAR);
                emitInt16(adjustment - i);
                break;
            }
            case (VT_DOUBLE): {
                emit(BC_STOREDVAR);
                emitInt16(adjustment - i);
                break;
            }
            case (VT_STRING): {
                emit(BC_STORESVAR);
                emitInt16(adjustment - i);
                break;
            }
            default: {
                assert(0);
            }
            }
        }

        node->visitChildren(this);
    }

    virtual void visitReturnNode(ReturnNode *node) {
        node->visitChildren(this);
        emit(BC_RETURN);
    }

    virtual void visitCallNode(CallNode *node) {
        node->visitChildren(this);
        emit(BC_CALL);
        uint16_t functionId = _code->functionByName(node->name())->id();
        _currentFunction->bytecode()->addInt16(functionId);
    }

    virtual void visitNativeCallNode(NativeCallNode *node) {
        assert(0);
    }

    virtual void visitPrintNode(PrintNode *node) {
        for (uint32_t i = 0; i < node->operands(); ++i) {
            node->operandAt(i)->visit(this);
            switch (_typesStack.top()) {
            case VT_DOUBLE: {
                emit(BC_DPRINT);
                break;
            }
            case VT_INT: {
                emit(BC_DPRINT);
                break;
            }
            case VT_STRING: {
                emit(BC_DPRINT);
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
    map<Scope *, uint16_t> _scopeToFuncitonMap; //TODO: fill

    void emit(Instruction inst) {
        _currentFunction->bytecode()->addInsn(inst);
    }

    void emitInt16(uint16_t value) {
        _currentFunction->bytecode()->addInt16(value);
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
        VarType varType = astVar->type();
        string variableName = astVar->name();
        assert(varType == VT_DOUBLE || varType == VT_INT || varType == VT_STRING);

        RichFunction *actualFunction = dynamic_cast<RichFunction *>(_code->functionById(_scopeToFuncitonMap[astVar->owner()]));
        assert(actualFunction != 0);

        bool is_local = actualFunction->id() == _currentFunction->id();
        uint16_t functionId = actualFunction->id();
        uint16_t variableId = actualFunction->getVariableId(variableName);

        if (varType == VT_INT) {
            if (is_local) {
                emit(BC_LOADIVAR);
            } else {
                emit(BC_LOADCTXIVAR);
                emitInt16(functionId);
            }
        } else if (varType == VT_DOUBLE) {
            if (is_local) {
                emit(BC_LOADDVAR);
            } else {
                emit(BC_LOADCTXDVAR);
                emitInt16(functionId);
            }
        } else if (varType == VT_DOUBLE) {
            if (is_local) {
                emit(BC_LOADSVAR);
            } else {
                emit(BC_LOADCTXSVAR);
                emitInt16(functionId);
            }
        }

        _typesStack.push(varType);
        emitInt16(variableId);
    }

    void storeToVariable(const AstVar *astVar) {
        VarType varType = astVar->type();
        string variableName = astVar->name();
        assert(varType == VT_DOUBLE || varType == VT_INT || varType == VT_STRING);
        //ASSUMPTION: no casts when storing variables
        assert(varType == _typesStack.top());

        RichFunction *actualFunction = dynamic_cast<RichFunction *>(_code->functionById(_scopeToFuncitonMap[astVar->owner()]));
        assert(actualFunction != 0);

        bool is_local = actualFunction->id() == _currentFunction->id();
        uint16_t functionId = actualFunction->id();
        uint16_t variableId = actualFunction->getVariableId(variableName);

        if (varType == VT_INT) {
            if (is_local) {
                emit(BC_STOREIVAR);
            } else {
                emit(BC_STORECTXIVAR);
                emitInt16(functionId);
            }
        } else if (varType == VT_DOUBLE) {
            if (is_local) {
                emit(BC_STOREDVAR);
            } else {
                emit(BC_STORECTXDVAR);
                emitInt16(functionId);
            }
        } else if (varType == VT_DOUBLE) {
            if (is_local) {
                emit(BC_STORESVAR);
            } else {
                emit(BC_STORECTXSVAR);
                emitInt16(functionId);
            }
        }
        emitInt16(variableId);
        _typesStack.pop();
    }

    void pop() {
        emit(BC_POP);
        _typesStack.pop();
    }

    //TODO: push to types stack more consciously.
    void pushInt0() {
        emit(BC_ILOAD0);
        _typesStack.push(VT_INT);
    }

    //TODO: push to types stack more consciously.
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