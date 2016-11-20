#include "translate_to_bytecode.h"

#include "parser.h"
#include "interpreter_code_impl.h"
//#include "pretty_print.h"

using namespace mathvm;

void BytecodeGenVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    if (node->kind() == tOR || node->kind() == tAND) {
        processLogicalOpNode(node);
        return;
    }

    node->right()->visit(this);
    node->left()->visit(this);

    switch (node->kind()) {
        case tAOR:
            doIDBinOp(BC_IAOR, node->position());
            break;
        case tAAND:
            doIDBinOp(BC_IAAND, node->position());
            break;
        case tAXOR:
            doIDBinOp(BC_IAXOR, node->position());
            break;
        case tEQ:
            doCompareOp(BC_IFICMPE, node->position());
            break;
        case tNEQ:
            doCompareOp(BC_IFICMPNE, node->position());
            break;
        case tGT:
            doCompareOp(BC_IFICMPG, node->position());
            break;
        case tGE:
            doCompareOp(BC_IFICMPGE, node->position());
            break;
        case tLT:
            doCompareOp(BC_IFICMPL, node->position());
            break;
        case tLE:
            doCompareOp(BC_IFICMPLE, node->position());
            break;
        case tADD:
            doIDBinOp(getBinOpType(node->position()) == VT_INT ? BC_IADD : BC_DADD, node->position());
            break;
        case tSUB:
            doIDBinOp(getBinOpType(node->position()) == VT_INT ? BC_ISUB : BC_DSUB, node->position());
            break;
        case tMUL:
            doIDBinOp(getBinOpType(node->position()) == VT_INT ? BC_IMUL : BC_DMUL, node->position());
            break;
        case tDIV:
            doIDBinOp(getBinOpType(node->position()) == VT_INT ? BC_IDIV : BC_DDIV, node->position());
            break;
        case tMOD:
            doIDBinOp(BC_IMOD, node->position());
            break;
        default:
            throw TranslatorError("Unknown bin op", node->position());
    }
}

void BytecodeGenVisitor::visitUnaryOpNode(UnaryOpNode* node) {
    node->operand()->visit(this);
    switch (node->kind()) {
        case tNOT: {
            if (ctx.tosType() != VT_INT) {
                throw TranslatorError("Badly typed logical NOT operand", node->position());
            }
            ctx.bytecode()->addInsn(BC_ILOAD0);
            Label resTrue(ctx.bytecode());
            ctx.bytecode()->addBranch(BC_IFICMPE, resTrue);
            ctx.bytecode()->addInsn(BC_ILOAD0);
            Label resFalse(ctx.bytecode());
            ctx.bytecode()->addBranch(BC_JA, resFalse);
            ctx.bytecode()->bind(resTrue);
            ctx.bytecode()->addInsn(BC_ILOAD1);
            ctx.bytecode()->bind(resFalse);
            break;
        }
        case tSUB:
            if (ctx.tosType() != VT_INT && ctx.tosType() != VT_DOUBLE) {
                throw TranslatorError("Badly typed unary minus operand", node->position());
            }
            ctx.bytecode()->addInsn((ctx.tosType() == VT_INT) ? BC_INEG : BC_DNEG);
            break;
        default:
            throw TranslatorError("bad unary op", node->position());
    }
}

void BytecodeGenVisitor::visitStringLiteralNode(StringLiteralNode* node) {
    auto id = code->makeStringConstant(node->literal());
    ctx.bytecode()->addInsn(BC_SLOAD);
    ctx.bytecode()->addUInt16(id);
    ctx.pushType(VT_STRING);
}

void BytecodeGenVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    auto val = node->literal();
    if (val == 0.0) {
        ctx.bytecode()->addInsn(BC_DLOAD0);
    } else if (val == 1.0) {
        ctx.bytecode()->addInsn(BC_DLOAD1);
    } else if (val == -1.0) {
        ctx.bytecode()->addInsn(BC_DLOADM1);
    } else {
        ctx.bytecode()->addInsn(BC_DLOAD);
        ctx.bytecode()->addDouble(val);
    }
    ctx.pushType(VT_DOUBLE);
}

void BytecodeGenVisitor::visitIntLiteralNode(IntLiteralNode* node) {
    auto val = node->literal();
    if (val == 0) {
        ctx.bytecode()->addInsn(BC_ILOAD0);
    } else if (val == 1) {
        ctx.bytecode()->addInsn(BC_ILOAD1);
    } else if (val == -1) {
        ctx.bytecode()->addInsn(BC_ILOADM1);
    } else {
        ctx.bytecode()->addInsn(BC_ILOAD);
        ctx.bytecode()->addInt64(val);
    }
    ctx.pushType(VT_INT);
}

void BytecodeGenVisitor::visitLoadNode(LoadNode* node) {
    auto varData = ctx.getVarByName(node->var()->name());
    loadVar(varData, node->position());
}

void BytecodeGenVisitor::visitStoreNode(StoreNode* node) {
    auto varData = ctx.getVarByName(node->var()->name());

    node->value()->visit(this);

    switch (node->op()) {
        case tASSIGN:
            break;
        case tINCRSET:
            loadVar(varData, node->position());
            doIDBinOp(getBinOpType(node->position()) == VT_INT ? BC_IADD : BC_DADD, node->position());
            break;
        case tDECRSET:
            loadVar(varData, node->position());
            doIDBinOp(getBinOpType(node->position()) == VT_INT ? BC_ISUB : BC_DSUB, node->position());
            break;
        default:
            throw TranslatorError("Bad store node operation", node->position());
    }

    storeVar(varData, node->position());
    loadVar(varData, node->position()); // store node return is just stored value
}

void BytecodeGenVisitor::visitBlockNode(BlockNode* node) {
    // 1) need to register functions
    // 2) translate registered functions
    // 3) translate block commands

    // registering functions
    std::map<std::string, BytecodeFunction*> functions;
    Scope::FunctionIterator funIt(node->scope());
    while (funIt.hasNext()) {
        auto fun = funIt.next();
        auto bf = new BytecodeFunction(fun);
        code->addFunction(bf);
        functions[fun->name()] = bf;
    }

    // pushing block scope
    ctx.pushScope(node->scope(), functions);

    // translating functions
    funIt = Scope::FunctionIterator(node->scope());
    while (funIt.hasNext()) {
        translateFunction(funIt.next());
    }

//    PPrintVisitor pp = PPrintVisitor(std::cout);
    // translating block statements
    for (uint32_t i = 0; i < node->nodes(); ++i) {
        auto s = node->nodeAt(i);
        s->visit(this);

//        s->visit(&pp);
//        std::cout << std::endl;

        if (s->isIfNode() || s->isForNode() || s->isWhileNode() || s->isPrintNode() || s->isReturnNode()) {
            // statements, which do not leave not useful values on the stack
            continue;
        }
        ctx.bytecode()->addInsn(BC_POP);
        ctx.popType();
    }

    ctx.popScope();
}

void BytecodeGenVisitor::visitNativeCallNode(NativeCallNode *node) {
    // TODO
}

void BytecodeGenVisitor::visitForNode(ForNode* node) {
    auto varData = ctx.getVarByName(node->var()->name());
    if (varData.type != VT_INT || !node->inExpr()->isBinaryOpNode()
        || node->inExpr()->asBinaryOpNode()->kind() != tRANGE) {
        throw TranslatorError("Bad for loop", node->position());
    }
    auto inExpr = node->inExpr()->asBinaryOpNode();

    inExpr->left()->visit(this);
    storeVar(varData, node->position()); // init loop var

    Label loop = ctx.bytecode()->currentLabel();
    // put range right bound on stack
    inExpr->right()->visit(this);
    loadVar(varData, node->position());

    Label exit(ctx.bytecode());
    ctx.bytecode()->addBranch(BC_IFICMPG, exit);

    node->body()->visit(this);

    // inc loop var
    loadVar(varData, node->position());
    ctx.bytecode()->addInsn(BC_ILOAD1);
    ctx.bytecode()->addInsn(BC_IADD);
    storeVar(varData, node->position());

    ctx.bytecode()->addBranch(BC_JA, loop);
    ctx.bytecode()->bind(exit);
}

void BytecodeGenVisitor::visitWhileNode(WhileNode* node) {
    Label loop = ctx.bytecode()->currentLabel();
    node->whileExpr()->visit(this);
    castTos(VT_INT, node->position());
    Label exit(ctx.bytecode());
    ctx.bytecode()->addInsn(BC_ILOAD0);
    ctx.bytecode()->addBranch(BC_IFICMPE, exit);
    node->loopBlock()->visit(this);
    ctx.bytecode()->addBranch(BC_JA, loop);
    ctx.bytecode()->bind(exit);
}

void BytecodeGenVisitor::visitIfNode(IfNode* node) {
    node->ifExpr()->visit(this);
    castTos(VT_INT, node->position());

    Label elseBranch(ctx.bytecode());
    ctx.bytecode()->addInsn(BC_ILOAD0);
    ctx.bytecode()->addBranch(BC_IFICMPE, elseBranch);

    node->thenBlock()->visit(this);
    Label exit(ctx.bytecode());
    if (node->elseBlock() != nullptr) {
        ctx.bytecode()->addBranch(BC_JA, exit);
    }

    ctx.bytecode()->bind(elseBranch);
    if (node->elseBlock() != nullptr) {
        node->elseBlock()->visit(this);
        ctx.bytecode()->bind(exit);
    }
}

void BytecodeGenVisitor::visitReturnNode(ReturnNode* node) {
    if (node->returnExpr() != nullptr) {
        node->returnExpr()->visit(this);
        castTos(ctx.returnType(), node->position());
        ctx.popType();
    }
    ctx.bytecode()->addInsn(BC_RETURN);
    ctx.setReturnedPos(ctx.bytecode()->length());

}

void BytecodeGenVisitor::visitFunctionNode(FunctionNode* node) {
    // we got there only from translateFunction
    node->body()->visit(this);
    if (ctx.getReturnedPos() < 0 && ctx.returnType() != VT_VOID) {
        throw TranslatorError("No return statement in function: [" + ctx.curFunctionName() + "]", node->position());
    }
    if (ctx.getReturnedPos() != (int32_t) ctx.bytecode()->length() && ctx.returnType() != VT_VOID) {
        throw TranslatorError("Return statement is not last statement in function: [" + ctx.curFunctionName() + "]",
                              node->position());
    }
    if (ctx.returnType() == VT_VOID && ctx.getReturnedPos() != (int32_t) ctx.bytecode()->length()) {
        if (node->name() == AstFunction::top_name) {
            ctx.bytecode()->addInsn(BC_STOP);
        } else {
            ctx.bytecode()->addInsn(BC_RETURN);
        }
        ctx.setReturnedPos(ctx.bytecode()->length());
    }
}


void BytecodeGenVisitor::visitCallNode(CallNode* node) {
    BytecodeFunction* fun = ctx.getFunByName(node->name());
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        auto expectedType = fun->parameterType(i);
        castTos(expectedType, node->parameterAt(i)->position()); // pushes type on type stack
    }
    ctx.bytecode()->addInsn(BC_CALL);
    ctx.bytecode()->addInt16(fun->id());
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        ctx.popType(); // pop parameter types from stack
    }
    ctx.pushType(fun->returnType());
}

void BytecodeGenVisitor::visitPrintNode(PrintNode* node) {
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        auto type = ctx.tosType();
        if (type == VT_INT) {
            ctx.bytecode()->addInsn(BC_IPRINT);
        } else if (type == VT_DOUBLE) {
            ctx.bytecode()->addInsn(BC_DPRINT);
        } else if (type == VT_STRING) {
            ctx.bytecode()->addInsn(BC_SPRINT);
        } else {
            throw TranslatorError("Bad print operand", node->operandAt(i)->position());
        }
        ctx.popType();
    }
}

void BytecodeGenVisitor::translateFunction(AstFunction *f) {
    BytecodeFunction* fun = dynamic_cast<BytecodeFunction*>(code->functionByName(f->name()));
    if (fun == nullptr) {
        throw TranslatorError("Function not registered [" + f->name() + "]", f->node()->position());
    }
    ctx.pushFunctionScope(f->scope(), fun);
    // storing parameter variables
    for (uint32_t i = 0; i < f->parametersNumber(); ++i) {
        auto varInfo = ctx.getVarByName(f->parameterName(i));
        genStoreVarBytecode(varInfo, f->node()->position());
    }
    f->node()->visit(this);
    fun->setLocalsNumber(ctx.getLocalsNumber());
    ctx.popScope();
    fun->setScopeId(ctx.currentContextId());
}

void BytecodeGenVisitor::loadVar(VarData varData, uint32_t position) {
    switch (varData.type) {
        case VT_DOUBLE:
            ctx.bytecode()->addInsn(BC_LOADCTXDVAR);
            break;
        case VT_INT:
            ctx.bytecode()->addInsn(BC_LOADCTXIVAR);
            break;
        case VT_STRING:
            ctx.bytecode()->addInsn(BC_LOADCTXSVAR);
            break;
        default:
            throw TranslatorError("Bad var type to load.", position);
    }
    ctx.bytecode()->addInt16(varData.context);
    ctx.bytecode()->addInt16(varData.id);
    ctx.pushType(varData.type);
}

void BytecodeGenVisitor::storeVar(VarData varData, uint32_t position) {
    castTos(varData.type, position);
    genStoreVarBytecode(varData, position);
    ctx.popType();
}

void BytecodeGenVisitor::genStoreVarBytecode(VarData varData, uint32_t position) {
    switch (varData.type) {
        case VT_DOUBLE:
            ctx.bytecode()->addInsn(BC_STORECTXDVAR);
            break;
        case VT_INT:
            ctx.bytecode()->addInsn(BC_STORECTXIVAR);
            break;
        case VT_STRING:
            ctx.bytecode()->addInsn(BC_STORECTXSVAR);
            break;
        default:
            throw TranslatorError("Unknown type to store into var: " + varData.name, position);
    }
    ctx.bytecode()->addInt16(varData.context);
    ctx.bytecode()->addInt16(varData.id);
}

void BytecodeGenVisitor::castTos(VarType to, uint32_t position) {
    if (ctx.tosType() == to) {
        return;
    }
    if (ctx.tosType() == VT_INT && to == VT_DOUBLE) {
        ctx.bytecode()->addInsn(BC_I2D);
    } else if (ctx.tosType() == VT_DOUBLE && to == VT_INT) {
        ctx.bytecode()->addInsn(BC_D2I);
    } else {
        throw TranslatorError("Can't cast stack top", position);
    }
    ctx.popType();
    ctx.pushType(to);
}

VarType BytecodeGenVisitor::getBinOpType(uint32_t position) {
    if (ctx.tosType() != VT_INT && ctx.tosType() != VT_DOUBLE &&
            ctx.prevTosType() != VT_INT && ctx.prevTosType() != VT_DOUBLE) {
        throw TranslatorError("Badly typed bin op operands", position);
    }
    if (ctx.tosType() == VT_DOUBLE || ctx.prevTosType() == VT_DOUBLE) {
        return VT_DOUBLE;
    }
    return VT_INT;
}

VarType BytecodeGenVisitor::castBinOpOperands(uint32_t position) {
    VarType opType = getBinOpType(position);
    if (ctx.tosType() != opType) {
        castTos(opType, position);
    }

    if (ctx.prevTosType() != opType) {
        ctx.bytecode()->addInsn(BC_SWAP);
        castTos(opType, position);
        ctx.bytecode()->addInsn(BC_SWAP);
    }
    return opType;
}


/**
 * Generate bytecode for binary operation.
 * Without casts.
 *
 */
void BytecodeGenVisitor::doIDBinOp(Instruction binOp, uint32_t position) {
    VarType resType;

    switch (binOp) {
        case BC_DADD:
        case BC_DSUB:
        case BC_DMUL:
        case BC_DDIV:
            resType = VT_DOUBLE;
            break;
        case BC_DCMP:
            resType = VT_INT;
            break;
        case BC_IADD:
        case BC_ISUB:
        case BC_IMUL:
        case BC_IDIV:
        case BC_IMOD:
        case BC_IAOR:
        case BC_IAAND:
        case BC_IAXOR:
        case BC_ICMP:
            resType = VT_INT;
            break;
        default:
            throw TranslatorError("Bad bin op!", position);
    }

    castBinOpOperands(position);

    ctx.bytecode()->addInsn(binOp);
    ctx.popType();
    ctx.popType();
    ctx.pushType(resType);
}

void BytecodeGenVisitor::processLogicalOpNode(BinaryOpNode *node) {
    if (node->kind() != tOR && node->kind() != tAND) {
        throw TranslatorError("Not logical operation", node->position());
    }

    node->left()->visit(this);
    if (ctx.tosType() != VT_INT) {
        throw TranslatorError("Bad logical operation operand type", node->left()->position());
    }
    ctx.popType();

    // making lazy evaluation
    ctx.bytecode()->addInsn(BC_ILOAD0);
    Label evalSecondOperand(ctx.bytecode());
    ctx.bytecode()->addBranch(node->kind() == tAND ? BC_IFICMPNE : BC_IFICMPE, evalSecondOperand);

    // put 0 or 1 on stack due to lazy evaluated AND or OR
    ctx.bytecode()->addInsn(node->kind() == tAND ? BC_ILOAD0 : BC_ILOAD1);

    // label for jumping over second op evaluation
    Label lazyHit(ctx.bytecode());
    ctx.bytecode()->addBranch(BC_JA, lazyHit);

    // eval second op
    ctx.bytecode()->bind(evalSecondOperand);
    node->right()->visit(this);
    if (ctx.tosType() != VT_INT) {
        throw TranslatorError("Bad logical operation operand type", node->right()->position());
    }
    ctx.popType();

    ctx.bytecode()->bind(lazyHit);
    ctx.pushType(VT_INT);
}

void BytecodeGenVisitor::doCompareOp(Instruction ifCmpInstruction, uint32_t position) {
    if (ifCmpInstruction != BC_IFICMPE && ifCmpInstruction != BC_IFICMPG && ifCmpInstruction != BC_IFICMPNE &&
            ifCmpInstruction != BC_IFICMPGE && ifCmpInstruction != BC_IFICMPL && ifCmpInstruction != BC_IFICMPLE) {
        throw TranslatorError("bad ificmp instruction", position);
    }

    VarType opType = castBinOpOperands(position);
    Instruction cmpInstruction = (opType == VT_DOUBLE) ? BC_DCMP : BC_ICMP;

    ctx.bytecode()->addInsn(cmpInstruction);
    ctx.bytecode()->addInsn(BC_ILOAD0); // stack will be: <- 0 - cmp - ...
    ctx.bytecode()->addInsn(BC_SWAP); // now stack is <- cmp - 0 - ...; cmp -- compare result

    Label trueL(ctx.bytecode());
    ctx.bytecode()->addBranch(ifCmpInstruction, trueL);
    ctx.bytecode()->addInsn(BC_ILOAD0);
    Label falseL(ctx.bytecode());
    ctx.bytecode()->addBranch(BC_JA, falseL);
    ctx.bytecode()->bind(trueL);
    ctx.bytecode()->addInsn(BC_ILOAD1);
    ctx.bytecode()->bind(falseL);
}

void BytecodeGenVisitor::translateToBytecode(AstFunction *root) {
    BytecodeFunction* bf = new BytecodeFunction(root);
    code->addFunction(bf);
    translateFunction(root);
}


// translator class

Status* BytecodeGenTranslator::translate(const std::string& program, Code* *code) {
    Parser parser;
    Status* status = parser.parseProgram(program);
    if (status->isError())
        return status;

    *code = new InterpreterCodeImpl();

    BytecodeGenVisitor visitor(*code);
    try {
        visitor.translateToBytecode(parser.top());
    } catch (TranslatorError e) {
        status = Status::Error(e.what(), e.getPosition());
    }

    return status;
}
