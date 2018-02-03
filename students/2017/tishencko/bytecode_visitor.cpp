#include "bytecode_visitor.h"

namespace mathvm {

BytecodeVisitor::BytecodeVisitor(AstFunction *top, InterpreterCode *code): _code(code) {
    BytecodeFunction * bytecodeFun = new BytecodeFunction(top);
    auto idx = code->addFunction(bytecodeFun);
    context = new Context(0);
    code->addContext(context);
    context->addFunction(top->node()->name(), idx);
    top->node()->visit(this);
}

void BytecodeVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    if (node->literal() == "") {
        bytecode()->addInsn(BC_SLOAD0);
    } else {
        bytecode()->addInsn(BC_SLOAD);
        bytecode()->addUInt16(_code->makeStringConstant(node->literal()));
    }
    addType(VT_STRING);
}

void BytecodeVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    auto literal = node->literal();
    switch (literal) {
    case 0:
        bytecode()->addInsn(BC_ILOAD0);
        break;
    case 1:
        bytecode()->addInsn(BC_ILOAD1);
        break;
    case -1:
        bytecode()->addInsn(BC_ILOADM1);
        break;
    default:
        bytecode()->addInsn(BC_ILOAD);
        bytecode()->addInt64(literal);
    }
    addType(VT_INT);
}

void BytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    double literal = node->literal();
    if (fabs(literal - 0) < 0e-5) {
        bytecode()->addInsn(BC_DLOAD0);
    } else if (fabs(literal - 1) < 0e-5) {
        literal > 0  ? bytecode()->addInsn(BC_DLOAD1) : bytecode()->addInsn(BC_DLOADM1);
    } else {
        bytecode()->addInsn(BC_DLOAD);
        bytecode()->addDouble(literal);
    }
    addType(VT_DOUBLE);
}

void BytecodeVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    node->visitChildren(this);
    switch (node->kind()) {
    case tSUB:
        if (lastType() != VT_INT && lastType() != VT_DOUBLE) {
            throw TranslatorException("Sub operation type is invalid", node->position());
        }
        lastType() == VT_DOUBLE ? bytecode()->addInsn(BC_DNEG) : bytecode()->addInsn(BC_INEG);
        break;
    case tNOT:
        if (lastType() != VT_INT) {
            throw TranslatorException("Not operation type is invalid", node->position());
        }
        bytecode()->addInsn(BC_ILOADM1);
        bytecode()->addInsn(BC_IMUL);
        bytecode()->addInsn(BC_ILOAD1);
        bytecode()->addInsn(BC_IADD);
        break;
    default:
        throw TranslatorException("Unknown unary operator", node->position());
    }
}

void BytecodeVisitor::visitBinaryOpNode(BinaryOpNode *node) {
    VarType fst, snd;

    if (node->kind() == tOR) {
        node->left()->visit(this);
        if (lastType() != VT_INT) {
            throw TranslatorException("Invalid type for logical operation", node->position());
        }
        Label llazy(bytecode());
        bytecode()->addInsn(BC_ILOAD1);
        bytecode()->addBranch(BC_IFICMPE, llazy);
        node->right()->visit(this);

        Label lend(bytecode());
        bytecode()->addBranch(BC_JA, lend);

        bytecode()->bind(llazy);
        bytecode()->addInsn(BC_ILOAD1);
        bytecode()->bind(lend);
    } else if (node->kind() == tAND) {
        node->left()->visit(this);
        if (lastType() != VT_INT) {
            throw TranslatorException("Invalid type for logical operation", node->position());
        }
        Label llazy(bytecode());
        bytecode()->addInsn(BC_ILOAD0);
        bytecode()->addBranch(BC_IFICMPE, llazy);
        node->right()->visit(this);

        Label lend(bytecode());
        bytecode()->addBranch(BC_JA, lend);

        bytecode()->bind(llazy);
        bytecode()->addInsn(BC_ILOAD0);
        bytecode()->bind(lend);

    } else {
        node->visitChildren(this);
        fst = removeType();
        snd = removeType();
        auto sinc = calculateBinOpType(fst, snd, node->position());
        if (sinc != 0) {
            convertToDouble(sinc);
        }
    }
    switch (node->kind()) {
    case tADD:
        if (lastType() == VT_INT) {
            bytecode()->addInsn(BC_IADD);
        } else if (lastType() == VT_DOUBLE) {
            bytecode()->addInsn(BC_DADD);
        } else {
            throw TranslatorException("Invalid type for add operation", node->position());
        }
        break;
    case tSUB:
        if (lastType() == VT_INT) {
            bytecode()->addInsn(BC_ISUB);
        } else if (lastType() == VT_DOUBLE) {
            bytecode()->addInsn(BC_DSUB);
        } else {
            throw TranslatorException("Invalid type for add operation", node->position());
        }
        break;
    case tMUL:
        if (lastType() == VT_INT) {
            bytecode()->addInsn(BC_IMUL);
        } else if (lastType() == VT_DOUBLE) {
            bytecode()->addInsn(BC_DMUL);
        } else {
            throw TranslatorException("Invalid type for sub operation", node->position());
        }
        break;
    case tDIV:
        if (lastType() == VT_INT) {
            bytecode()->addInsn(BC_IDIV);
        } else if (lastType() == VT_DOUBLE) {
            bytecode()->addInsn(BC_DDIV);
        } else {
            throw new TranslatorException("Invalid type for div operation", node->position());
        }
        break;
    case tMOD:
        if (lastType() == VT_INT) {
            bytecode()->addInsn(BC_IMOD);
        } else {
            throw TranslatorException("Invalid type for mod operation", node->position());
        }
        break;
    case tAOR:
        if (lastType() == VT_INT) {
            bytecode()->addInsn(BC_IAOR);
        } else {
            throw TranslatorException("Invalid type for int or operation", node->position());
        }
        break;
    case tAXOR:
        if (lastType() == VT_INT) {
            bytecode()->addInsn(BC_IAXOR);
        } else {
            throw TranslatorException("Invalid type for int xor operation", node->position());
        }
        break;
    case tAAND:
        if (lastType() == VT_INT) {
            bytecode()->addInsn(BC_IAAND);
        } else {
            throw TranslatorException("Invalid type for int and operation", node->position());
        }
        break;
    case tGT:
        bytecode()->addInsn(BC_SWAP);
        if (lastType() == VT_INT) {
            bytecode()->addInsn(BC_ICMP);
        } else if (lastType() == VT_DOUBLE) {
            removeType();
            bytecode()->addInsn(BC_DCMP);
            addType(VT_INT);
        } else {
            throw TranslatorException("Invalid type for compare operation", node->position());
        }
        bytecode()->addInsn(BC_ILOAD1);
        bytecode()->addInsn(BC_ICMP);
        bytecode()->addInsn(BC_INEG);
        bytecode()->addInsn(BC_ILOAD1);
        bytecode()->addInsn(BC_IADD);
        break;
    case tGE:
        bytecode()->addInsn(BC_SWAP);
        if (lastType() == VT_INT) {
            bytecode()->addInsn(BC_ICMP);
        } else if (lastType() == VT_DOUBLE) {
            removeType();
            bytecode()->addInsn(BC_DCMP);
            addType(VT_INT);
        } else {
            throw TranslatorException("Invalid type for compare operation", node->position());
        }
        bytecode()->addInsn(BC_ILOADM1);
        bytecode()->addInsn(BC_ICMP);
        bytecode()->addInsn(BC_INEG);
        break;
    case tLT:
        if (lastType() == VT_INT) {
            bytecode()->addInsn(BC_ICMP);
        } else if (lastType() == VT_DOUBLE) {
            removeType();
            bytecode()->addInsn(BC_DCMP);
            addType(VT_INT);
        } else {
            throw TranslatorException("Invalid type for compare operation", node->position());
        }
        bytecode()->addInsn(BC_ILOAD1);
        bytecode()->addInsn(BC_ICMP);
        bytecode()->addInsn(BC_INEG);
        bytecode()->addInsn(BC_ILOAD1);
        bytecode()->addInsn(BC_IADD);
        break;
    case tLE:
        if (lastType() == VT_INT) {
            bytecode()->addInsn(BC_ICMP);
        } else if (lastType() == VT_DOUBLE) {
            removeType();
            bytecode()->addInsn(BC_DCMP);
            addType(VT_INT);
        } else {
            throw TranslatorException("Invalid type for compare operation", node->position());
        }
        bytecode()->addInsn(BC_ILOADM1);
        bytecode()->addInsn(BC_ICMP);
        bytecode()->addInsn(BC_INEG);
        break;
    case tEQ: {
        if (lastType() == VT_INT) {
            bytecode()->addInsn(BC_ICMP);
        } else if (lastType() == VT_DOUBLE) {
            removeType();
            bytecode()->addInsn(BC_DCMP);
            addType(VT_INT);
        } else {
            throw TranslatorException("Invalid type for compare operation", node->position());
        }
        bytecode()->addInsn(BC_LOADIVAR0);

        bytecode()->addInsn(BC_IMUL);
        bytecode()->addInsn(BC_ILOADM1);
        bytecode()->addInsn(BC_IADD);
        bytecode()->addInsn(BC_INEG);
        break;
    }
    case tNEQ: {
        if (lastType() == VT_INT) {
            bytecode()->addInsn(BC_ICMP);
        } else if (lastType() == VT_DOUBLE) {
            removeType();
            bytecode()->addInsn(BC_DCMP);
            addType(VT_INT);
        } else {
            throw TranslatorException("Invalid type for compare operation", node->position());
        }
        bytecode()->addInsn(BC_LOADIVAR0);
        bytecode()->addInsn(BC_IMUL);
        break;
    }
    case tAND:
    case tOR:
        break;
    default:
        throw TranslatorException("Unknown binary operation", node->position());
    }
}

void BytecodeVisitor::visitPrintNode(PrintNode *node) {
    for (uint32_t i = 0; i < node->operands(); ++i) {
            node->operandAt(i)->visit(this);
            switch (removeType()) {
            case VT_INT:
                bytecode()->addInsn(BC_IPRINT);
                break;
            case VT_DOUBLE:
                bytecode()->addInsn(BC_DPRINT);
                break;
            case VT_STRING:
                bytecode()->addInsn(BC_SPRINT);
                break;
            default:
                throw TranslatorException("Unknown printable type", node->position());
            }
        }
}

void BytecodeVisitor::visitReturnNode(ReturnNode *node) {
    if (node->returnExpr()) {
        node->visitChildren(this);
    } else {
        addType(VT_VOID);
    }
    bytecode()->addInsn(BC_RETURN);
}

void BytecodeVisitor::visitLoadNode(LoadNode *node) {
    auto pair = contextVar(node->var()->name());
    loadVar(node->var()->type(), pair.first, pair.second, node->position());
}

void BytecodeVisitor::visitStoreNode(StoreNode *node) {
    node->visitChildren(this);
    auto pair = contextVar(node->var()->name());
    uint16_t ctx = pair.first;
    uint16_t idx = pair.second;
    if (node->var()->type() != lastType()) {
        if (node->var()->type() == VT_INT && lastType() == VT_DOUBLE) {
            convertToDouble(1);
        } else if (node->var()->type() == VT_STRING && lastType() == VT_INT) {
            bytecode()->addInsn(BC_S2I);
        } else {
            throw TranslatorException("Not convertable type", node->position());
        }
    }
    switch(node->op()) {
    case tASSIGN:
        break;
    case tINCRSET:
        loadVar(lastType(), ctx, idx, node->position());
        if (lastType() == VT_INT) {
            bytecode()->addInsn(BC_IADD);
        } else if (lastType() == VT_DOUBLE) {
            bytecode()->addInsn(BC_DADD);
        } else {
            throw TranslatorException("Not incrementable type", node->position());
        }
        break;
    case tDECRSET:
        loadVar(lastType(), ctx, idx, node->position());
        bytecode()->addInsn(BC_SWAP);
        if (lastType() == VT_INT) {
            bytecode()->addInsn(BC_ISUB);
        } else if (lastType() == VT_DOUBLE) {
            bytecode()->addInsn(BC_DSUB);
        } else {
            throw TranslatorException("Not decrementable type", node->position());
        }
        break;
    default:
        throw TranslatorException("Unexpected store operation", node->position());
    }

    storeVar(lastType(), ctx, idx, node->position());
    removeType();
}

void BytecodeVisitor::visitCallNode(CallNode *node) {
    TranslatedFunction* function = _code->functionByName(node->name());
    if (!function) {
        throw TranslatorException("Undeclared function " + node->name(), node->position());
    }

    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        auto type = function->parameterType(i);
        auto realType = removeType();
        if (type != realType) {
           if (type == VT_DOUBLE && realType == VT_INT) {
               convertToDouble(1);
           } else if (type == VT_INT && realType == VT_STRING) {
               bytecode()->addInsn(BC_S2I);
           } else {
               throw TranslatorException("Not convertable type", node->position());
            }
        }
    }

   bytecode()->addInsn(BC_CALL);
   bytecode()->addInt16(function->id());

   addType(function->returnType());
}

void BytecodeVisitor::visitNativeCallNode(NativeCallNode *node) {
    auto name = node->nativeName();
    auto signature = node->nativeSignature();
    void *codeNative = dlsym(RTLD_DEFAULT, name.c_str());
    uint16_t id = _code->makeNativeFunction(name, signature, codeNative);
    for (size_t i = signature.size() - 1; i >= 1; --i) {
            auto param_name = signature[i].second;
            auto pair = contextVar(param_name);
            loadVar(signature[i].first, pair.first, pair.second, node->position());
    }
    bytecode()->addInsn(BC_CALLNATIVE);
    bytecode()->addInt16(id);
    addType(signature[0].first);
}

void BytecodeVisitor::visitIfNode(IfNode *node) {
    node->ifExpr()->visit(this);
    if (lastType() != VT_INT) {
        throw TranslatorException("Unexpected expression type", node->position());
    }
    Label lelse(bytecode());
    bytecode()->addInsn(BC_ILOAD0);
    bytecode()->addBranch(BC_IFICMPE, lelse);
    node->thenBlock()->visit(this);
    Label lend(bytecode());
    if (node->elseBlock()) {
        bytecode()->addBranch(BC_JA, lend);
    }
    bytecode()->bind(lelse);
    if (node->elseBlock()) {
        node->elseBlock()->visit(this);
        bytecode()->bind(lend);
    }
}

void BytecodeVisitor::visitWhileNode(WhileNode *node) {
    Label lstart(bytecode());
    bytecode()->bind(lstart);
    node->whileExpr()->visit(this);
    if (lastType() != VT_INT) {
        throw TranslatorException("Unexpected expression type", node->position());
    }
    Label lend(bytecode());
    bytecode()->addInsn(BC_ILOAD0);
    bytecode()->addBranch(BC_IFICMPE, lend);
    node->loopBlock()->visit(this);
    bytecode()->addBranch(BC_JA, lstart);
    bytecode()->bind(lend);
}

void BytecodeVisitor::visitForNode(ForNode *node) {
    auto pair = contextVar(node->var()->name());
    uint16_t ctx = pair.first;
    uint16_t idx = pair.second;

    if (node->var()->type() != VT_INT || !node->inExpr()->isBinaryOpNode() || node->inExpr()->asBinaryOpNode()->kind() != tRANGE) {
        throw TranslatorException("Invalid for expression", node->position());
    }

    node->inExpr()->asBinaryOpNode()->left()->visit(this);
    storeVar(node->var()->type(), ctx, idx, node->position());
    removeType();
    Label lstart(bytecode());
    bytecode()->bind(lstart);
    node->inExpr()->asBinaryOpNode()->right()->visit(this);
    loadVar(node->var()->type(), ctx, idx, node->position());

    Label lend(bytecode());
    bytecode()->addBranch(BC_IFICMPG, lend);
    node->body()->visit(this);

    loadVar(node->var()->type(), ctx, idx, node->position());
    bytecode()->addInsn(BC_ILOAD1);
    bytecode()->addInsn(BC_IADD);
    storeVar(node->var()->type(), ctx, idx, node->position());

    bytecode()->addBranch(BC_JA, lstart);
    bytecode()->bind(lend);
}

bool isTopLevelBlock = false;

void BytecodeVisitor::createNewContext() {
    auto newContext = new Context(_code->nextContext(), context);
    _code->addContext(newContext);
    context = newContext;
}

void BytecodeVisitor::visitFunctionNode(FunctionNode *node) {
    auto fun = (BytecodeFunction *) _code->functionByName(node->name());
    _currentFunction.push_back(fun);
    _bytecode.push_back(fun->bytecode());
    createNewContext();
    isTopLevelBlock = true;
    for (int64_t i = static_cast<int64_t>(node->parametersNumber()) - 1; i >= 0; --i) {
         context->addVariable(node->parameterName(i));
         auto pair = contextVar(node->parameterName(i));
         storeVar(node->parameterType(i), pair.first, pair.second, node->position());
    }
    bool native = node->body()->nodes() > 0 && node->body()->nodeAt(0) && node->body()->nodeAt(0)->isNativeCallNode();
    if (native) {
       node->body()->nodeAt(0)->visit(this);
    } else {
       node->body()->visit(this);
    }
    bool stop = node->name() == AstFunction::top_name;
    bytecode()->addInsn(stop ? BC_STOP : BC_RETURN);
    _bytecode.pop_back();
    _currentFunction.pop_back();
    addType(node->returnType());
}

void BytecodeVisitor::visitBlockNode(BlockNode *node) {
    if (isTopLevelBlock) {
        isTopLevelBlock = false;
    } else {
        createNewContext();
    }
    context->addScope(node->scope(), _code);
    context->owner(currentFunction());
    translateFunctionBody(node->scope());
    for (uint32_t i = 0; i < node->nodes(); i++) {
        node->nodeAt(i)->visit(this);
    }
    context = context->parent;
}

Status * BytecodeTranslatorImpl::translate(const string & program, Code ** code)
{
    Parser parser;
    Status * status = parser.parseProgram(program);

    if (status && status->isError()) return status;

    InterpreterCode* icode = new InterpreterCode();

    try {
        BytecodeVisitor result(parser.top(), icode);
        *code = result.code();
    } catch (TranslatorException& ex) {
        return Status::Error(ex.what());
    }

    return Status::Ok();
}

}
