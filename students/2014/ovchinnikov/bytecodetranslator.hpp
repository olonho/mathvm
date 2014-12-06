#ifndef BYTECODETRANSLATOR_HPP
#define BYTECODETRANSLATOR_HPP

#include <cmath>
#include "ast.h"
#include "mathvm.h"
#include "visitors.h"
#include "blockscope.hpp"
#include "bytecodeinterpreter.hpp"

#define TYPE_AND_ACTION_TO_BC_NUMERIC(t,bcprefix,bcsuffix) \
( \
    t == VT_INT \
        ? BC_##bcprefix##I##bcsuffix \
        : t == VT_DOUBLE \
            ? BC_##bcprefix##D##bcsuffix \
            : BC_INVALID \
)

#define TYPE_AND_ACTION_TO_BC(t,bcprefix,bcsuffix) \
( \
    t == VT_INT \
        ? BC_##bcprefix##I##bcsuffix \
        : t == VT_DOUBLE \
            ? BC_##bcprefix##D##bcsuffix \
            : t == VT_STRING \
                ? BC_##bcprefix##S##bcsuffix\
                : BC_INVALID \
)


class BytecodeTranslator : public Translator, AstBaseVisitor {

public:

    BytecodeTranslator() : tos(VT_INVALID), interpreter(new BytecodeInterpreter()), bc(0), scope(0) {}
    virtual ~BytecodeTranslator() {}
    virtual Status *translate(const string & program, Code **out);

#define VISITOR_FUNCTION(type, name) \
     virtual void visit##type(type* node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
    void coerceToBoolean() {
        bc->add(BC_ILOAD0);
        processComparison(tNEQ, tos, tos);
    }

    void processLoadVar(const AstVar *astVar) {
        processLoadStoreVar(astVar, true);
    }

    void processStoreVar(const AstVar *astVar) {
        processLoadStoreVar(astVar, false);
    }

    void processLoadStoreVar(const AstVar *astVar, bool load) {
        auto var = scope->resolveVar(astVar->name());
        auto fun = interpreter->functionById(var.first);
        bool local = var.first == scope->function()->id();
        Instruction code = load
                           ? local
                           ? TYPE_AND_ACTION_TO_BC(astVar->type(), LOAD, VAR)
                           : TYPE_AND_ACTION_TO_BC(astVar->type(), LOADCTX, VAR)
                           : local
                           ? TYPE_AND_ACTION_TO_BC(astVar->type(), STORE, VAR)
                           : TYPE_AND_ACTION_TO_BC(astVar->type(), STORECTX, VAR);
        if (code == BC_INVALID) { throw "Unsupported reference type"; }
        bc->add(code);
        if (!local) { bc->addUInt16(var.first); }
        bc->addUInt16(var.second);
        tos = load ? astVar->type() : VT_VOID;
    }

    void enterScope() {
        BlockScope *entering = new BlockScope(scope);
        scope = entering;
    }

    void leaveScope() {
        BlockScope *leaving = scope;
        scope = scope->parent();
        scope->setChildLocals(leaving->size() + leaving->childLocals());
        delete leaving;
    }

    void processArithmeticOperator(TokenKind token, VarType leftType, VarType rightType) {
        VarType type = lub(leftType, rightType);
        if (leftType != type) {
            assert(type == VT_DOUBLE);
            assert(leftType == VT_INT);
            bc->add(BC_SWAP);
            bc->add(BC_I2D);
            bc->add(BC_SWAP);
        }
        if (rightType != type) {
            assert(type == VT_DOUBLE);
            assert(rightType == VT_INT);
            bc->add(BC_I2D);
        }
        switch (token) {
            case tADD:  bc->add(TYPE_AND_ACTION_TO_BC_NUMERIC(type, , ADD)); break;
            case tSUB:  bc->add(BC_SWAP); bc->add(TYPE_AND_ACTION_TO_BC_NUMERIC(type, , SUB)); break;
            case tMUL:  bc->add(TYPE_AND_ACTION_TO_BC_NUMERIC(type, , MUL)); break;
            case tDIV:  bc->add(BC_SWAP); bc->add(TYPE_AND_ACTION_TO_BC_NUMERIC(type, , DIV)); break;
            default: throw "Unsupported arithmetic operator";
        }
        tos = type;
    }

    void processLogicOperator(TokenKind token, VarType leftType, VarType rightType) {
        bc->add(BC_SWAP);
        coerceToBoolean();
        bc->add(BC_SWAP);
        coerceToBoolean();
        switch (token) {
            case tOR:   bc->add(BC_IADD); coerceToBoolean(); break;
            case tAND:  bc->add(BC_IMUL); coerceToBoolean(); break;
            default: throw "Unsupported logic operator";
        }
        tos = VT_INT;
    }

    void processArithmeticLogicOperator(TokenKind token, VarType leftType, VarType rightType) {
        if (leftType != VT_INT) {
            assert (leftType == VT_DOUBLE);
            bc->add(BC_SWAP);
            bc->add(BC_D2I);
            bc->add(BC_SWAP);
        }
        if (rightType != VT_INT) {
            assert (rightType == VT_DOUBLE);
            bc->add(BC_D2I);
        }
        switch (token) {
            case tAAND: bc->add(BC_IAAND); break;
            case tAOR:  bc->add(BC_IAOR); break;
            case tAXOR: bc->add(BC_IAXOR); break;
            default: throw "Unsupported arithmetic logic operator";
        }
        tos = VT_INT;
    }

    void processComparison(TokenKind token, VarType leftType, VarType rightType) {
        VarType type = lub(leftType, rightType);

        if (leftType != type) {
            assert(type == VT_DOUBLE);
            assert(leftType == VT_INT);
            bc->add(BC_SWAP);
            bc->add(BC_I2D);
            bc->add(BC_SWAP);
        }
        if (rightType != type) {
            assert(type == VT_DOUBLE);
            assert(rightType == VT_INT);
            bc->add(BC_I2D);
        }

        bc->add(TYPE_AND_ACTION_TO_BC_NUMERIC(type, , CMP));
        switch (token) {
            case tEQ:   processComparison(BC_IFICMPE); break;
            case tNEQ:  processComparison(BC_IFICMPNE); break;
            case tGT:   processComparison(BC_IFICMPG); break;
            case tGE:   processComparison(BC_IFICMPGE); break;
            case tLT:   processComparison(BC_IFICMPL); break;
            case tLE:   processComparison(BC_IFICMPLE); break;
            default: throw "Unsupported comparison token";
        }
        tos = VT_INT;
    }

    void processComparison(Instruction action) {
        bc->add(BC_ILOAD0); // push 0
        Label elseLabel(bc);
        Label endLabel(bc);
        bc->addBranch(action, elseLabel); // compare with 0
        bc->add(BC_ILOAD0);              // if comparison returns false - push 0
        bc->addBranch(BC_JA, endLabel);
        elseLabel.bind(bc->current());
        bc->add(BC_ILOAD1);              // if comparison returns true  - push 1
        endLabel.bind(bc->current());
    }

    VarType lub(VarType left, VarType right) {
        return left == VT_DOUBLE || right == VT_DOUBLE
               ? VT_DOUBLE
               : VT_INT;
    }

private:
    VarType tos;
    BytecodeInterpreter *const interpreter;

    Bytecode *bc;
    BlockScope *scope;
};

#endif // BYTECODETRANSLATOR_HPP
