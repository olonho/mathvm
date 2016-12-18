//
// Created by dsavvinov on 11.11.16.
//

#include <algorithm>
#include <cstring>
#include "BytecodeTranslatorVisitor.h"
#include "TranslationException.h"

namespace mathvm {

void BytecodeTranslatorVisitor::visitFunctionNode(FunctionNode *node) {
    if (node->body()->nodes() > 0 &&
            node->body()->nodeAt(0)->isNativeCallNode()) {
        code->makeNativeFunction(node->name(), node->signature(), nullptr);
        return;
    }
    BytecodeFunction * bytecodeFunction = (BytecodeFunction *) code->functionByName(node->name());
    Scope * innerScope = node->body()->scope();

    ctx.enterFunction(bytecodeFunction);

    if (node->name() != AstFunction::top_name) {
        // WTF, WHY FUNCTION ARGUMENTS DECLARED IN SEPARATE SCOPE????????
        Scope * funcArgsScope = innerScope->parent();
        declareScope(funcArgsScope);
    }

    ctx.pushScope(innerScope);

    declareScope(innerScope);
    visitScope(innerScope);

    if (node->name() != AstFunction::top_name) {
        storeParams(bytecodeFunction);
    }

    node->body()->visit(this);

    ctx.exitFunction();
    ctx.popScope();
}

void BytecodeTranslatorVisitor::visitBlockNode(BlockNode *node) {
    for (uint32_t i = 0; i < node->nodes(); ++i) {
        AstNode *statement = node->nodeAt(i);
        if (statement->isBlockNode()) {
            Scope * innerScope = statement->asBlockNode()->scope();
            ctx.pushScope(innerScope);
            declareScope(innerScope);
            visitScope(innerScope);

            statement->visit(this);

            ctx.popScope();
        } else {
            statement->visit(this);

            // Remove ignored return value
            if (pushesOnStack(statement)) {
                ctx.curFunction()->bytecode()->addInsn(BC_POP);
            }
        }
    }
}

void BytecodeTranslatorVisitor::visitScope(Scope *scope) {
    Scope::FunctionIterator funIter = Scope::FunctionIterator(scope);
    while(funIter.hasNext()) {
        AstFunction * curFun = funIter.next();
        curFun->node()->visit(this);
    }
}

BytecodeCode *BytecodeTranslatorVisitor::getCode() const {
    return code;
}

void BytecodeTranslatorVisitor::storeParams(BytecodeFunction *function) {
    for (int i = function->parametersNumber() - 1; i >= 0; --i) {
        string const & name = function->parameterName(i);
        Info *info = ctx.getVarInfo(name);

        storeVar(info);
    }
}

void BytecodeTranslatorVisitor::visitForNode(ForNode *node) {
    Scope * innerScope = node->body()->scope();
    Bytecode * curBytecode = ctx.curFunction()->bytecode();

    // For some unknown reason Parser doesn't check anything about correctness of range expression. Duh.
    verifyRangeExpr(node->inExpr());

    BinaryOpNode *range = node->inExpr()->asBinaryOpNode();
    Info * forVarInfo = static_cast<Info *>(node->var()->info());

    // assign initial value to iterator
    VarType leftType = visitExpressionWithResult(range->left());
    switch(leftType) {
        case VT_DOUBLE:
            curBytecode->addInsn(BC_D2I);
            break;
        case VT_STRING:
            curBytecode->addInsn(BC_S2I);
        default:
            break;
    }
    storeVar(forVarInfo);

    // Prepare labels
    Label boundsCheckLabel(curBytecode), end(curBytecode);

    // Set boundCheckLabel to current offset
    curBytecode->bind(boundsCheckLabel);
    // Emit bounds check code
    VarType rightType = visitExpressionWithResult(range->right());
    switch(rightType) {
        case VT_DOUBLE:
            curBytecode->addInsn(BC_D2I);
            break;
        case VT_STRING:
            curBytecode->addInsn(BC_S2I);
        default:
            break;
    }
    loadVar(forVarInfo);
    curBytecode->addBranch(BC_IFICMPG, end);

    ctx.pushScope(innerScope);
    declareScope(innerScope);
    visitScope(innerScope);

    node->body()->visit(this);

    // increment for-variable
    loadVar(forVarInfo);
    curBytecode->addInsn(BC_ILOAD1);
    curBytecode->addInsn(BC_IADD);
    storeVar(forVarInfo);

    curBytecode->addBranch(BC_JA, boundsCheckLabel);

    curBytecode->bind(end);

    ctx.popScope();
}

void BytecodeTranslatorVisitor::verifyRangeExpr(AstNode *node) {
    if (!node->isBinaryOpNode() || node->asBinaryOpNode()->kind() != tRANGE) {
        throw TranslationException("For loop allows only iteration by range");
    }
}

template<class T>
void BytecodeTranslatorVisitor::loadConstant(VarType type, T value) {
    Bytecode * curBytecode = ctx.curFunction()->bytecode();
    Instruction insn;

    switch (type) {
        case VT_INT : insn = BC_ILOAD; break;
        case VT_DOUBLE : insn = BC_DLOAD; break;
        case VT_STRING : insn = BC_SLOAD; break;
        default : throw TranslationException(string("Can't load variable of type ", typeToName(type)).c_str());
    }

    curBytecode->addInsn(insn);
    curBytecode->addTyped(value);
    ctx.tosType = type;
}

void BytecodeTranslatorVisitor::storeVar(Info *info) {
    Bytecode *curBytecode = ctx.curFunction()->bytecode();
    uint16_t curFuncID = ctx.curFunction()->id();

    uint16_t varFuncID = info->funcId;
    uint16_t varLocalID = info->localId;

    bool isLocal = varFuncID == curFuncID;

    Instruction insn;
    switch (info->type) {
        case VT_DOUBLE:
            insn = isLocal ? BC_STOREDVAR : BC_STORECTXDVAR;
            break;
        case VT_INT:
            insn = isLocal ? BC_STOREIVAR : BC_STORECTXIVAR;
            break;
        case VT_STRING:
            insn = isLocal ? BC_STORESVAR : BC_STORECTXSVAR;
            break;
        default:
            throw TranslationException((string("Can't store variable of type: ")
                                        + string(typeToName(info->type))).c_str());
    }


    curBytecode->addInsn(insn);
    if (!isLocal) {
        curBytecode->addUInt16(varFuncID);
    }
    curBytecode->addUInt16(varLocalID);
}

void BytecodeTranslatorVisitor::loadVar(Info * info) {
    Bytecode *bc= ctx.curFunction()->bytecode();
    uint16_t curFuncID = ctx.curFunction()->id();

    uint16_t varFuncID = info->funcId;
    uint16_t varLocalID = info->localId;

    bool isLocal = varFuncID == curFuncID;

    Instruction insn;
    switch (info->type) {
        case VT_INT:
            insn = isLocal ? BC_LOADIVAR : BC_LOADCTXIVAR;
            break;
        case VT_DOUBLE:
            insn = isLocal ? BC_LOADDVAR : BC_LOADCTXDVAR;
            break;
        case VT_STRING:
            insn = isLocal ? BC_LOADSVAR : BC_LOADCTXSVAR;
            break;
        default: throw TranslationException(
                    (
                            string("Can't load variable of type ") + string(typeToName(info->type))
                    ).c_str()
            );
    }

    bc->addInsn(insn);
    if (!isLocal) {
        bc->addUInt16(varFuncID);
    }
    bc->addUInt16(varLocalID);
    ctx.tosType = info->type;
}

void BytecodeTranslatorVisitor::visitPrintNode(PrintNode *node) {
    for (uint32_t i = 0; i < node->operands(); ++i) {
        AstNode * op = node->operandAt(i);
        VarType type = visitExpressionWithResult(op);

        Instruction insn;
        switch (type) {
            case VT_INT: insn = BC_IPRINT; break;
            case VT_DOUBLE: insn = BC_DPRINT; break;
            case VT_STRING: insn = BC_SPRINT; break;
            default : throw TranslationException("Can print only int/double/string");
        }

        Bytecode * bytecode = ctx.curFunction()->bytecode();
        bytecode->addInsn(insn);
    }
}

VarType BytecodeTranslatorVisitor::visitExpressionWithResult(AstNode * node) {
    if (node->isBinaryOpNode()) {
        node->visit(this);
        return ctx.tosType;
    } else if (node->isUnaryOpNode()) {
        node->visit(this);
        return ctx.tosType;
    } else if (node->isCallNode()) {
        BytecodeFunction * function = (BytecodeFunction *) code->functionByName(node->asCallNode()->name());
        if (function->returnType() == VT_VOID) {
            throw TranslationException("Cant return result of the call to VOID function");
        }

        node->visit(this);
        return function->returnType();
    } else if (node->isLoadNode()) {
        node->visit(this);
        return ctx.tosType;
    } else if (node->isIntLiteralNode()) {
        node->visit(this);
        return VT_INT;
    } else if (node->isDoubleLiteralNode()) {
        node->visit(this);
        return VT_DOUBLE;
    } else if (node->isStringLiteralNode()) {
        node->visit(this);
        return VT_STRING;
    } else {
        throw TranslationException("Unknown node type, can't return with result");
    }
}

void BytecodeTranslatorVisitor::visitLoadNode(LoadNode *node) {
    const AstVar * var = node->var();
    Info *info = static_cast<Info *>(var->info());
    loadVar(info);
}

void BytecodeTranslatorVisitor::visitIfNode(IfNode *node) {
    AstNode * ifExpr = node->ifExpr();
    Bytecode *bytecode = ctx.curFunction()->bytecode();
    Label falseBranch(bytecode), end(bytecode);

    // check condition
    visitExpressionWithResult(ifExpr);
    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(BC_IFICMPE, falseBranch);

    // true branch
    Scope *thenScope = node->thenBlock()->scope();
    ctx.pushScope(thenScope);
    declareScope(thenScope);
    visitScope(thenScope);
    node->thenBlock()->visit(this);
    bytecode->addBranch(BC_JA, end);
    ctx.popScope();

    // false branch
    bytecode->bind(falseBranch);
    if (node->elseBlock()) {
        Scope *elseScope = node->elseBlock()->scope();
        ctx.pushScope(elseScope);
        declareScope(elseScope);
        visitScope(elseScope);
        node->elseBlock()->visit(this);
        ctx.popScope();
    }
    bytecode->bind(end);
}

void BytecodeTranslatorVisitor::visitStoreNode(StoreNode *node) {
    const AstVar * var = node->var();
    Info * info = static_cast<Info *>(var->info());

    VarType valueType = visitExpressionWithResult(node->value());

    castType(valueType, var->type());

    if (node->op() == tINCRSET) {
        incrementIntoVariable(info);
        return;
    }

    if (node->op() == tDECRSET) {
        decrementIntoVariable(info);
        return;
    }

    storeVar(info);
}

void BytecodeTranslatorVisitor::visitWhileNode(WhileNode *node) {
    Bytecode * bytecode = ctx.curFunction()->bytecode();
    Label end(bytecode), check(bytecode);

    // Condition check
    bytecode->bind(check);
    visitExpressionWithResult(node->whileExpr());
    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(BC_IFICMPE, end);

    // Body
    Scope * loopScope = node->loopBlock()->scope();
    ctx.pushScope(loopScope);

    declareScope(loopScope);
    visitScope(loopScope);

    node->loopBlock()->visit(this);
    bytecode->addBranch(BC_JA, check);

    ctx.popScope();
    bytecode->bind(end);
}

void BytecodeTranslatorVisitor::visitReturnNode(ReturnNode *node) {
    Bytecode * bytecode = ctx.curFunction()->bytecode();
    AstNode * returnExpr = node->returnExpr();
    if (returnExpr == nullptr) {
        if (ctx.curFunction()->returnType() != VT_VOID) {
            throw TranslationException("Empty return in non-void function");
        }
        return;
    }
    VarType exprType = visitExpressionWithResult(returnExpr);

    castType(exprType, ctx.curFunction()->returnType());
    bytecode->addInsn(BC_RETURN);
}

void BytecodeTranslatorVisitor::visitNativeCallNode(NativeCallNode *node) {
    // We shouldn't ever get here because we check native call as the degenerate case in visitFunctionNode
    assert(false);
}

void BytecodeTranslatorVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    loadConstant(VT_INT, node->literal());
}

void BytecodeTranslatorVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    uint16_t id = code->makeStringConstant(node->literal());
    Bytecode * bc = ctx.curFunction()->bytecode();
    bc->addInsn(BC_SLOAD);
    bc->addUInt16(id);
    ctx.tosType = VT_STRING;
}

void BytecodeTranslatorVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    loadConstant(VT_DOUBLE, node->literal());
}

void BytecodeTranslatorVisitor::visitCallNode(CallNode *node) {
    Bytecode * bytecode = ctx.curFunction()->bytecode();
    BytecodeFunction * function = static_cast<BytecodeFunction *>(code->functionByName(node->name()));

    if (function->parametersNumber() != node->parametersNumber()) {
        throw TranslationException(
                (
                        string("Bad arguments number: expected ") +
                        std::to_string(function->signature().size()) +
                        string(" got ") +
                        std::to_string(node->parametersNumber())
                ).c_str()
        );
    }

    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        AstNode *expr = node->parameterAt(i);
        VarType type = visitExpressionWithResult(expr);
        castType(type, function->parameterType(i));
    }

    bytecode->addInsn(BC_CALL);
    bytecode->addUInt16(function->id());
}

void BytecodeTranslatorVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    if (node->kind() == tNOT) {
        Bytecode * bc = ctx.curFunction()->bytecode();
        VarType type = visitExpressionWithResult(node->operand());
        if (type != VT_INT) {
            throw TranslationException("Can't use unary NOT on non-int parameter");
        }
        ctx.tosType = type;

        /**
         * We will emit the following code now:
         *  ILOAD0
         *  IFICMPE setTrue
         *  # setFalse
         *  ILOAD0
         *  JA end
         *
         *  # setTrue
         *  ILOAD1
         *  # end
         *
         *
         *  0 | 0 => 1
         *  1 | 0 => 0
         */

        Label setTrue(bc), end(bc);
        bc->addInsn(BC_ILOAD0);
        bc->addBranch(BC_IFICMPE, setTrue);
        bc->addInsn(BC_ILOAD0);
        bc->addBranch(BC_JA, end);

        bc->bind(setTrue);
        bc->addInsn(BC_ILOAD1);
        bc->bind(end);
    } else if (node->kind() == tSUB) {
        Bytecode * bytecode = ctx.curFunction()->bytecode();
        VarType type = visitExpressionWithResult(node->operand());
        ctx.tosType = type;
        switch (type) {
            case VT_INT:
                bytecode->addInsn(BC_INEG);
                break;
            case VT_DOUBLE:
                bytecode->addInsn(BC_DNEG);
                break;
            default:
                throw TranslationException(
                        (
                                string("Can't use unary MINUS on arg of type") +
                                string(typeToName(type))
                        ).c_str()
                );
        }
    }
}

void BytecodeTranslatorVisitor::visitBinaryOpNode(BinaryOpNode *node) {
    Bytecode * bc = ctx.curFunction()->bytecode();
    VarType rightType = visitExpressionWithResult(node->right());
    VarType leftType = visitExpressionWithResult(node->left());
    VarType commonType = unifyTypes(leftType, rightType);

    castType(leftType, commonType);

    if (rightType != commonType) {
        bc->addInsn(BC_SWAP);
        castType(rightType, commonType);
        bc->addInsn(BC_SWAP);
    }

    ctx.tosType = commonType;
    switch (node->kind()) {
        case tADD:
            if (commonType == VT_INT) {
                bc->addInsn(BC_IADD);
            } else {
                bc->addInsn(BC_DADD);
            }
            break;
        case tSUB:
            if (commonType == VT_INT) {
                bc->addInsn(BC_ISUB);
            } else {
                bc->addInsn(BC_DSUB);
            }
            break;
        case tMUL:
            if (commonType == VT_INT) {
                bc->addInsn(BC_IMUL);
            } else {
                bc->addInsn(BC_DMUL);
            }
            break;
        case tDIV:
            if (commonType == VT_INT) {
                bc->addInsn(BC_IDIV);
            } else {
                bc->addInsn(BC_DDIV);
            }
            break;
        case tMOD:
            bc->addInsn(BC_IMOD);
            break;
        case tRANGE:
            break;
        case tEQ:
            compare(Instruction::BC_IFICMPE, commonType);
            break;
        case tNEQ:
            compare(Instruction::BC_IFICMPNE, commonType);
            break;
        case tGT:
            compare(Instruction::BC_IFICMPG, commonType);
            break;
        case tGE:
            compare(Instruction::BC_IFICMPGE, commonType);
            break;
        case tLT:
            compare(Instruction::BC_IFICMPL, commonType);
            break;
        case tLE:
            compare(Instruction::BC_IFICMPLE, commonType);
            break;
        case tOR:
            bc->addInsn(BC_IAOR);
            break;
        case tAND:
            bc->addInsn(BC_IAAND);
            break;
        case tAOR:
            bc->addInsn(BC_IAOR);
            break;
        case tAAND:
            bc->addInsn(BC_IAAND);
            break;
        case tAXOR:
            bc->addInsn(BC_IAXOR);
            break;
        default:
            throw TranslationException("Not a binary op!");
    }
}

BytecodeTranslatorVisitor::BytecodeTranslatorVisitor()
        : AstBaseVisitor()
        , code(new BytecodeCode())
{ }

void BytecodeTranslatorVisitor::incrementIntoVariable(Info * info) {
    Bytecode *bytecode = ctx.curFunction()->bytecode();
    loadVar(info);
    switch (info->type) {
        case VT_INT:
            bytecode->addInsn(BC_IADD);
            break;
        case VT_DOUBLE:
            bytecode->addInsn(BC_DADD);
            break;
        default:
            throw TranslationException("Can increment-set only int/double");
    }
    storeVar(info);
}

void BytecodeTranslatorVisitor::decrementIntoVariable(Info * info) {
    Bytecode *bytecode = ctx.curFunction()->bytecode();
    loadVar(info);
    switch (info->type) {
        case VT_INT:
            bytecode->addInsn(BC_ISUB);
            break;
        case VT_DOUBLE:
            bytecode->addInsn(BC_DSUB);
            break;
        default:
            throw TranslationException("Can increment-set only int/double");
    }
    storeVar(info);
}

void BytecodeTranslatorVisitor::compare(Instruction insn, VarType type) {
    /* We will emit following code:
     *  I(D)CMP # put comparator value onto stack
     *  # now just compare it with zero with right instruction
     *  ILOAD0
     *  SWAP # note the necessity of swap
     *  INSN setTrueLabel
     *  # false branch
     *  DLOAD0
     *  JA endLabel # exit comparison block
     *
     *  setTrueLabel:
     *  DLOAD1
     *  endLabel:
     *  ...
     */

    Bytecode * bc = ctx.curFunction()->bytecode();
    Label setTrue(bc), end(bc);

    if (type == VT_INT) {
        bc->addInsn(BC_ICMP);
    } else if (type == VT_DOUBLE) {
        bc->addInsn(BC_DCMP);
    } else {
        throw TranslationException("Comparison supported only for int/double");
    }

    bc->addInsn(BC_ILOAD0);
    bc->addInsn(BC_SWAP);
    bc->addBranch(insn, setTrue);
    bc->addInsn(BC_ILOAD0);
    bc->addBranch(BC_JA, end);

    bc->bind(setTrue);
    bc->addInsn(BC_ILOAD1);
    bc->bind(end);

    ctx.tosType = VT_INT;
}

bool BytecodeTranslatorVisitor::pushesOnStack(AstNode * statement) {
    // Call is a separate case
    if (statement->isCallNode()) {
        string const & functionName = statement->asCallNode()->name();
        TranslatedFunction *function = code->functionByName(functionName);
        return function->returnType() != VT_VOID;
    }
    return statement->isIntLiteralNode()
            || statement->isStringLiteralNode()
            || statement->isDoubleLiteralNode()
            || statement->isLoadNode()
            || statement->isBinaryOpNode()
            || statement->isUnaryOpNode();
}

void BytecodeTranslatorVisitor::castType(VarType from, VarType to) {
    if (from == to) {
        return;
    }

    if (from == VT_INT && to == VT_DOUBLE) {
        ctx.curFunction()->bytecode()->addInsn(BC_I2D);
        return;
    }

    if (from == VT_DOUBLE && to == VT_INT) {
        ctx.curFunction()->bytecode()->addInsn(BC_D2I);
        return;
    }

    if (from == VT_STRING && to == VT_INT) {
        ctx.curFunction()->bytecode()->addInsn(BC_S2I);
        return;
    }

    throw TranslationException(
            (
                    string("Can't assign value of type ") +
                    string(typeToName(from)) +
                    string(" to ") +
                    string(typeToName(to))
            ).c_str());
    }


VarType BytecodeTranslatorVisitor::unifyTypes(VarType left, VarType right) {
    return std::min(left, right);
}

void BytecodeTranslatorVisitor::declareScope(Scope * scope) {
    Scope::VarIterator varIterator(scope);
    while(varIterator.hasNext()) {
        AstVar * curVar = varIterator.next();
        ctx.declareVariable(curVar, scope);
    }

    Scope::FunctionIterator funIter(scope);
    while (funIter.hasNext()) {
        AstFunction * curFun = funIter.next();
        code->addFunction(new BytecodeFunction(curFun));
    }
}
}  // mathvm namespace