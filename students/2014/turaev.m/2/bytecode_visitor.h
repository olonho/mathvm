#ifndef BYTECODE_VISITOR_H
#define BYTECODE_VISITOR_H

#include "visitors.h"
#include "parser.h"
#include "rich_function.h"
#include "interpreter_code_impl.h"
#include "function_crawler.h"
#include <stack>

using namespace mathvm;

class BytecodeMainVisitor: public AstVisitor {
public:
    BytecodeMainVisitor(Code *code, AstFunction *topFunction);

    virtual void visitBinaryOpNode(BinaryOpNode *node);

    virtual void visitUnaryOpNode(UnaryOpNode *node);

    virtual void visitStringLiteralNode(StringLiteralNode *node);

    virtual void visitDoubleLiteralNode(DoubleLiteralNode *node);

    virtual void visitIntLiteralNode(IntLiteralNode *node);

    virtual void visitLoadNode(LoadNode *node);

    virtual void visitStoreNode(StoreNode *node);

    virtual void visitForNode(ForNode *node);

    virtual void visitWhileNode(WhileNode *node);

    virtual void visitIfNode(IfNode *node);

    virtual void visitBlockNode(BlockNode *node);

    virtual void visitFunctionNode(FunctionNode *node);

    virtual void visitReturnNode(ReturnNode *node);

    virtual void visitCallNode(CallNode *node);

    virtual void visitNativeCallNode(NativeCallNode *node);

    virtual void visitPrintNode(PrintNode *node);

private:
    Code *_code;
    RichFunction *_currentFunction;
    stack<VarType> _typesStack;
    map<VarType, map<TokenKind, Instruction> > _typeTokenInstruction;
    map<Scope *, uint16_t> _scopeToFunctionMap; //TODO: fill

    Bytecode *bytecode() {
        return _currentFunction->bytecode();
    }

    void emit(Instruction inst) {
        bytecode()->addInsn(inst);
    }

    void emitUInt16(uint16_t value) {
        bytecode()->addInt16(value);
    }

    void emitDouble(double value) {
        bytecode()->addDouble(value);
    }

    void emitInt64(int64_t value) {
        bytecode()->addInt64(value);
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

        RichFunction *actualFunction = dynamic_cast<RichFunction *>(_code->functionById(_scopeToFunctionMap[astVar->owner()]));
        assert(actualFunction != 0);

        bool is_local = actualFunction->id() == _currentFunction->id();
        uint16_t functionId = actualFunction->id();
        uint16_t variableId = actualFunction->getVariableId(variableName);

        if (varType == VT_INT) {
            if (is_local) {
                emit(BC_LOADIVAR);
            } else {
                emit(BC_LOADCTXIVAR);
                emitUInt16(functionId);
            }
        } else if (varType == VT_DOUBLE) {
            if (is_local) {
                emit(BC_LOADDVAR);
            } else {
                emit(BC_LOADCTXDVAR);
                emitUInt16(functionId);
            }
        } else if (varType == VT_STRING) {
            if (is_local) {
                emit(BC_LOADSVAR);
            } else {
                emit(BC_LOADCTXSVAR);
                emitUInt16(functionId);
            }
        }

        _typesStack.push(varType);
        emitUInt16(variableId);
    }

    void storeToVariable(const AstVar *astVar) {
        VarType varType = astVar->type();
        string variableName = astVar->name();
        assert(varType == VT_DOUBLE || varType == VT_INT || varType == VT_STRING);
        //ASSUMPTION: no casts when storing variables
        assert(varType == _typesStack.top());

        RichFunction *actualFunction = dynamic_cast<RichFunction *>(_code->functionById(_scopeToFunctionMap[astVar->owner()]));
        assert(actualFunction != 0);

        bool is_local = actualFunction->id() == _currentFunction->id();
        uint16_t functionId = actualFunction->id();
        uint16_t variableId = actualFunction->getVariableId(variableName);

        if (varType == VT_INT) {
            if (is_local) {
                emit(BC_STOREIVAR);
            } else {
                emit(BC_STORECTXIVAR);
                emitUInt16(functionId);
            }
        } else if (varType == VT_DOUBLE) {
            if (is_local) {
                emit(BC_STOREDVAR);
            } else {
                emit(BC_STORECTXDVAR);
                emitUInt16(functionId);
            }
        } else if (varType == VT_STRING) {
            if (is_local) {
                emit(BC_STORESVAR);
            } else {
                emit(BC_STORECTXSVAR);
                emitUInt16(functionId);
            }
        }
        emitUInt16(variableId);
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
    //TODO: Refactor using builder
    void binary_comparison(BinaryOpNode *node) {
        TokenKind operation = node->kind();
        assert(operation == tEQ || operation == tNEQ ||  operation == tGT
               || operation == tGE || operation == tLT || operation == tLE);

        node->right()->visit(this);
        node->left()->visit(this);

        unify_binary_arg_types();

        VarType first = _typesStack.top();
        _typesStack.pop();
        VarType second = _typesStack.top();
        _typesStack.pop();
        assert(first == VT_DOUBLE && second == VT_DOUBLE
               || first == VT_INT && second == VT_INT);

        if (first == VT_DOUBLE && second == VT_DOUBLE) {
            emit(BC_DCMP);
            pushInt0(); //TODO: type erasure!
            emit(BC_SWAP);
        }
        Label beforeTrue(bytecode());
        Label afterTrue(bytecode());
        Label afterFalse(bytecode());
        bytecode()->addBranch(_typeTokenInstruction[VT_INT][operation], beforeTrue);
        bytecode()->addBranch(BC_JA, afterTrue);
        beforeTrue.bind(bytecode()->current());
        pushInt1();
        bytecode()->addBranch(BC_JA, afterFalse);
        afterTrue.bind(bytecode()->current());
        pushInt0();
        afterFalse.bind(bytecode()->current());
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
                Label afterTrue(bytecode());
                Label afterFalse(bytecode());
                pushInt0();
                bytecode()->addBranch(BC_IFICMPNE, afterTrue);
                pushInt0();
                bytecode()->addBranch(BC_JA, afterFalse);
                //}
                // else {
                afterTrue.bind(bytecode()->current());
                //return B;
                node->right()->visit(this);
                assert(_typesStack.top() == VT_INT);
                afterFalse.bind(bytecode()->current());
                // }
                break;
            }
            case (tOR): {
                //A || B
                //if (A == true) {
                Label afterTrue(bytecode());
                Label afterFalse(bytecode());
                pushInt1();
                bytecode()->addBranch(BC_IFICMPNE, afterTrue);
                pushInt1();
                bytecode()->addBranch(BC_JA, afterFalse);
                //}
                // else {
                afterTrue.bind(bytecode()->current());
                //return B;
                node->right()->visit(this);
                assert(_typesStack.top() == VT_INT);
                afterFalse.bind(bytecode()->current());
                // }
                break;
            }
            default: {
                assert(0);
            }
        }
    }

    void unify_binary_arg_types() {
        VarType topType = _typesStack.top();
        _typesStack.pop();

        switch (topType) {
            case (VT_DOUBLE): {
                switch (_typesStack.top()) {
                    case (VT_DOUBLE): {
                        _typesStack.push(VT_DOUBLE);
                        break;
                    }
                    case (VT_INT): {
                        emit(BC_SWAP);
                        convertTOS(VT_DOUBLE);
                        emit(BC_SWAP);
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
                switch (_typesStack.top()) {
                    case (VT_DOUBLE): {
                        _typesStack.push(VT_INT);
                        convertTOS(VT_DOUBLE);
                        break;
                    }
                    case (VT_INT): {
                        _typesStack.push(VT_INT);
                        break;
                    }
                    default: {
                        assert(0);
                    }
                }
            }
            default: {
                break;
            }
        }
    }

    void binary_math(TokenKind operation) {
        unify_binary_arg_types();
        VarType first = _typesStack.top();
        _typesStack.pop();
        VarType second = _typesStack.top();
        _typesStack.pop();

        assert(first == second); //after unification
        assert(first == VT_DOUBLE || first == VT_INT);

        emit(_typeTokenInstruction[first][operation]);
        _typesStack.push(first);
    }

    void unary_math(TokenKind operation) {
        switch (_typesStack.top()) {
            case (VT_DOUBLE): {
                switch (operation) {
                    case (tSUB): {
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
                        emit(BC_INEG);
                        break;
                    }
                    case (tNOT): {
                        emit(BC_ILOAD1);
                        emit(BC_IAXOR);
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

#endif