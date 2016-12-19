#include <dlfcn.h>
#include "parser.h"
#include "interpreter_impl.h"
#include "bytecode_translator.h"

namespace mathvm {

Status* BytecodeTranslatorImpl::translate(const string& program, Code* *code) {
    Parser parser;
    Status* status = parser.parseProgram(program);
    if (status->isError()) {
        return status;
    }
    *code = new InterpreterCodeImpl();
    BytecodeTranslator translator(*code);
    AstFunction* topFunc = parser.top();

    BytecodeFunction* bf = new BytecodeFunction(topFunc);

    (*code)->addFunction(bf);
    try {
        translator.processFunc(topFunc);
        bf->bytecode()->addInsn(BC_STOP);
    } catch(std::runtime_error* e) {
        return Status::Error(e->what());
    } catch(...) {
        return Status::Error("Translation error");
    }

    return Status::Ok();
}


void BytecodeTranslator::processFunc(AstFunction *f) {
    BytecodeFunction* bf = (BytecodeFunction *) code->functionByName(f->name());
    Context* newContext = new Context(bf, f->scope(), context);
    context = newContext;

    for (size_t i = 0; i < f->parametersNumber(); ++i) {
        AstVar* var = f->scope()->lookupVariable(f->parameterName(i), false);
        storeVar(var);
    }

    f->node()->visit(this);

    bf->setLocalsNumber(context->getVarsNum());
    bf->setScopeId(context->getId());

    context = newContext->getParent();
    delete newContext;
}

void BytecodeTranslator::storeVar(const AstVar * var) {
    Instruction localIns = BC_INVALID;
    Instruction globalIns = BC_INVALID;
    switch (var->type()) {
        case VT_INT:
            localIns = BC_STOREIVAR;
            globalIns = BC_STORECTXIVAR;
            break;
        case VT_DOUBLE:
            localIns = BC_STOREDVAR;
            globalIns = BC_STORECTXDVAR;
            break;
        case VT_STRING:
            localIns = BC_STORESVAR;
            globalIns = BC_STORECTXSVAR;
            break;
        default:
            throw new std::runtime_error("Wrong var type");
    }
    addVarIns(var, localIns, globalIns);
}

void BytecodeTranslator::loadVar(const AstVar * var) {
    Instruction localIns = BC_INVALID;
    Instruction globalIns = BC_INVALID;
    switch (var->type()) {
        case VT_INT:
            localIns = BC_LOADIVAR;
            globalIns = BC_LOADCTXIVAR;
            break;
        case VT_DOUBLE:
            localIns = BC_LOADDVAR;
            globalIns = BC_LOADCTXDVAR;
            break;
        case VT_STRING:
            localIns = BC_LOADSVAR;
            globalIns = BC_LOADCTXSVAR;
            break;
        default:
            throw new std::runtime_error("Wrong var type");
    }
    addVarIns(var, localIns, globalIns);
}

void BytecodeTranslator::addVarIns(const AstVar * var, Instruction& localIns, Instruction& globalIns) {
    VarDescr VarDescr = context->getVarDescr(var->name());
    if (context->getId() == VarDescr.contextId) {
        bc()->addInsn(localIns);
    } else {
        bc()->addInsn(globalIns);
        bc()->addUInt16(VarDescr.contextId);
    }
    bc()->addUInt16(VarDescr.id);
    context->setTOSType(var->type());
}

void BytecodeTranslator::visitBinaryOpNode(BinaryOpNode* node) {
   switch (node->kind()) {
    case tAND:
    case tOR:
        visitLogicOp(node);
        return;
    default:
        break;
  }

  Instruction insnInt = BC_INVALID;
  Instruction insnDouble = BC_INVALID;
  bool found = true;
  switch (node->kind()) {
    case tADD:
        insnInt = BC_IADD;
        insnDouble = BC_DADD;
        break;
    case tSUB:
        insnInt = BC_ISUB;
        insnDouble = BC_DSUB;
        break;
    case tMUL:
        insnInt = BC_IMUL;
        insnDouble = BC_DMUL;
        break;
    case tDIV:
        insnInt = BC_IDIV;
        insnDouble = BC_DDIV;
        break;
    case tMOD:
        insnInt = BC_IMOD;
        break;
    default:
        found = false;
        break;
  }
  if (found) {
      visitArithmOp(node, insnInt, insnDouble);
      return;
  }
  found = true;
  Instruction insnCmp = BC_INVALID;
  switch (node->kind()) {
    case tEQ:
        insnCmp = BC_IFICMPE;
        break;
    case tNEQ:
        insnCmp = BC_IFICMPNE;
        break;
    case tGE:
        insnCmp = BC_IFICMPGE;
        break;
    case tLE:
        insnCmp = BC_IFICMPLE;
        break;
    case tGT:
        insnCmp = BC_IFICMPG;
        break;
    case tLT:
        insnCmp = BC_IFICMPL;
        break;
    default:
        found = false;
        break;
  }
  if (found) {
      visitCompareOp(node, insnCmp);
  }
  Instruction insnBitwise = BC_INVALID;
  found = true;
  switch (node->kind()) {
      case tAOR:
          insnBitwise = BC_IAOR;
          break;
      case tAAND:
          insnBitwise = BC_IAAND;
          break;
      case tAXOR:
          insnBitwise = BC_IAXOR;
          break;
      default:
          found = false;
          break;
  }
  if (found) {
    visitBitwiseOp(node, insnBitwise);
  }

}

void BytecodeTranslator::visitArithmOp(BinaryOpNode* node, Instruction& insnInt, Instruction& insnDouble) {
  node->right()->visit(this);
  VarType rightType = context->getTOSType();
  node->left()->visit(this);
  VarType leftType = context->getTOSType();
  castArithmTypes(leftType, rightType);
  bc()->addInsn(chooseInsn(leftType, insnInt, insnDouble));
  context->setTOSType(leftType);
}

void BytecodeTranslator::visitLogicOp(BinaryOpNode* node) {
    Instruction ins = BC_INVALID;
    if (node->kind() == tAND) {
        ins = BC_ILOAD0;
    } else {
        ins = BC_ILOAD1;
    }

    node->left()->visit(this);
    castTOS(VT_INT);

    bc()->addInsn(ins);
    Label right_end = Label(bc());
    bc()->addBranch(BC_IFICMPE, right_end);

    node->right()->visit(this);
    Label operation_end = Label(bc());
    bc()->addBranch(BC_JA, operation_end);

    bc()->bind(right_end);
    bc()->addInsn(ins);

    bc()->bind(operation_end);

    castTOS(VT_INT);
    context->setTOSType(VT_INT);
}

void BytecodeTranslator::visitCompareOp(BinaryOpNode* node, Instruction& insnCmp) {
    node->right()->visit(this);
    castTOS(VT_INT);
    node->left()->visit(this);
    castTOS(VT_INT);

    Label left = Label(bc());
    bc()->addBranch(insnCmp, left);
    bc()->addInsn(BC_ILOAD0);
    Label right = Label(bc());
    bc()->addBranch(BC_JA, right);
    bc()->bind(left);
    bc()->addInsn(BC_ILOAD1);
    bc()->bind(right);

    context->setTOSType(VT_INT);
}

void BytecodeTranslator::visitBitwiseOp(BinaryOpNode* node, Instruction& insnBitwise) {
    node->right()->visit(this);
    castTOS(VT_INT);
    node->left()->visit(this);
    castTOS(VT_INT);
    Instruction insn = BC_INVALID;
    switch (node->kind()) {
        case tAOR:
            insn = BC_IAOR;
            break;
        case tAAND:
            insn = BC_IAAND;
            break;
        case tAXOR:
            insn = BC_IAXOR;
            break;
        default:
            throw new runtime_error("Invalid bitwise operation");
    }    

    bc()->addInsn(insn);
    context->setTOSType(VT_INT);
}

void BytecodeTranslator::visitUnaryOpNode(UnaryOpNode *node) {
      node->operand()->visit(this);
    switch (node->kind()) {
        case tNOT:
            this->addNotNode(node);
            break;
        case tSUB:
            this->addMinusNode(node);
            break;
        default:
            throw new std::runtime_error("Unknown operation");
    }
}

void BytecodeTranslator::addNotNode(UnaryOpNode *node) {
    if (context->getTOSType() != VT_INT) {
        throw new std::runtime_error("Invalid type");
    }
    bc()->addInsn(BC_ILOAD1);
    Label l1 = Label(bc());
    bc()->addBranch(BC_IFICMPE, l1);
    bc()->addInsn(BC_ILOAD1);
    Label l2 = Label(bc());
    bc()->addBranch(BC_JA, l2);
    bc()->bind(l1);
    bc()->addInsn(BC_ILOAD0);
    bc()->bind(l2);
    context->setTOSType(VT_INT);
}

void BytecodeTranslator::addMinusNode(UnaryOpNode *node) {
    switch (context->getTOSType()) {
        case VT_STRING:
            castTOS(VT_INT);
        case VT_INT:
            bc()->addInsn(BC_INEG);
            break;
        case VT_DOUBLE:
            bc()->addInsn(BC_DNEG);
            break;
        default:
            throw new std::runtime_error("Invalid type");
    }
}

void BytecodeTranslator::visitStringLiteralNode(StringLiteralNode *node) {
    bc()->addInsn(BC_SLOAD);
    bc()->addUInt16(code->makeStringConstant(node->literal()));
    context->setTOSType(VT_STRING);
}

void BytecodeTranslator::visitDoubleLiteralNode(DoubleLiteralNode *node) { 
    bc()->addInsn(BC_DLOAD);
    bc()->addDouble(node->literal());
    context->setTOSType(VT_DOUBLE);
} 

void BytecodeTranslator::visitIntLiteralNode(IntLiteralNode *node) {
    bc()->addInsn(BC_ILOAD);
    bc()->addInt64(node->literal());
    context->setTOSType(VT_INT);
} 

void BytecodeTranslator::visitLoadNode(LoadNode *node) { 
    loadVar(node->var());
} 

void BytecodeTranslator::visitStoreNode(StoreNode *node) { 
    node->value()->visit(this);
    VarType vt = node->var()->type();
    castTOS(vt);
    if (node->op() == tINCRSET) {
        loadVar(node->var());
        Instruction insnInc = chooseInsn(vt, BC_IADD, BC_DADD);
        bc()->addInsn(insnInc);
    } else if (node->op() == tDECRSET) {
        loadVar(node->var());
        Instruction insnDec = chooseInsn(vt, BC_ISUB, BC_DSUB);
        bc()->addInsn(insnDec);
    }

    storeVar(node->var());
    loadVar(node->var());
}

void BytecodeTranslator::visitForNode(ForNode *node) { 
    if (node->var()->type() != VT_INT) {
        throw new std::runtime_error("iter var in for expression must be int");
    }
    BinaryOpNode* inExpr = node->inExpr()->asBinaryOpNode();
    inExpr->left()->visit(this);
    if (context->getTOSType() != VT_INT) {
        throw new std::runtime_error("iterable expression must be range of ints");
    }
    storeVar(node->var());

    Label labelStart = Label(bc());
    Label labelEnd = Label(bc());

    bc()->bind(labelStart);
    inExpr->right()->visit(this);
    if (context->getTOSType() != VT_INT) {
        throw new std::runtime_error("iterable expression must be range of ints");
    }
    loadVar(node->var());
    bc()->addBranch(BC_IFICMPG, labelEnd);
    node->body()->visit(this);
    loadVar(node->var());
    bc()->addInsn(BC_ILOAD1);
    bc()->addInsn(BC_IADD);
    storeVar(node->var());
    bc()->addBranch(BC_JA, labelStart);
    bc()->bind(labelEnd);

    bc()->addInsn(BC_ILOAD0);
    context->setTOSType(VT_VOID);
} 

void BytecodeTranslator::visitWhileNode(WhileNode *node) { 
    Label labelStart = Label(bc());
    Label labelEnd = Label(bc());

    bc()->bind(labelStart);
    bc()->addInsn(BC_ILOAD0);
    node->whileExpr()->visit(this);
    castTOS(VT_INT);
    bc()->addBranch(BC_IFICMPE, labelEnd);
    node->loopBlock()->visit(this);
    bc()->addBranch(BC_JA, labelStart);
    bc()->bind(labelEnd);

    bc()->addInsn(BC_ILOAD0);
    context->setTOSType(VT_VOID);
} 

void BytecodeTranslator::visitIfNode(IfNode *node) { 
    node->ifExpr()->visit(this);
    castTOS(VT_INT);

    Label l_else = Label(bc());
    Label labelEnd = Label(bc());

    bc()->addInsn(BC_ILOAD0);
    bc()->addBranch(BC_IFICMPE, l_else);
    node->thenBlock()->visit(this);
    bc()->addBranch(BC_JA, labelEnd);

    bc()->bind(l_else);
    if (node->elseBlock()) {
        node->elseBlock()->visit(this);
    }
    bc()->bind(labelEnd);

    bc()->addInsn(BC_ILOAD0);
    context->setTOSType(VT_VOID);
} 

void BytecodeTranslator::visitBlockNode(BlockNode *node) { 
    Scope::VarIterator varsIt(node->scope());
    while (varsIt.hasNext()) {
        AstVar* var = varsIt.next();
        context->addVar(var);
    }

    Scope::FunctionIterator functionsIt(node->scope());
    while (functionsIt.hasNext()) {
        AstFunction* function = functionsIt.next();

        TranslatedFunction* bf = code->functionByName(function->name());
        if (bf) {
            throw new std::runtime_error("Function duplicate");
        }
        bf = new BytecodeFunction(function);
        code->addFunction(bf);
    }

    functionsIt = Scope::FunctionIterator(node->scope());
    while (functionsIt.hasNext()) {
        processFunc(functionsIt.next());
    }

    for (size_t i = 0; i < node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);
        bc()->add(BC_POP);
        context->setTOSType(VT_VOID);
    }
    bc()->addInsn(BC_ILOAD0);
    context->setTOSType(VT_VOID);
}

void BytecodeTranslator::visitFunctionNode(FunctionNode *node) {
    if (node->body() != NULL)
        node->body()->visit(this);
} 

void BytecodeTranslator::visitReturnNode(ReturnNode *node) { 
    if (node->returnExpr()) {
        node->returnExpr()->visit(this);
        castTOS(context->getBf()->returnType());
        bc()->addInsn(BC_RETURN);
        context->setTOSType(context->getBf()->returnType());
    } else {      
        bc()->addInsn(BC_ILOAD0);
        context->setTOSType(VT_VOID);
        bc()->addInsn(BC_RETURN);
    }
} 

void BytecodeTranslator::visitCallNode(CallNode *node) { 
    TranslatedFunction* bf = code->functionByName(node->name());
    if (!bf) {
        throw new runtime_error("Unknown function");
    }
    if (node->parametersNumber() != bf->parametersNumber()) {
        throw new runtime_error("Function " + bf->name() + " takes " + to_string(bf->parametersNumber())
            + " arguments (" + to_string(node->parametersNumber()) + " given)");
    }

    for (size_t i = node->parametersNumber(); node->parametersNumber() > 0 && i > 0; --i) {
        node->parameterAt(i-1)->visit(this);
        castTOS(bf->parameterType(i-1));
    }

    bc()->addInsn(BC_CALL);
    bc()->addUInt16(bf->id());
    context->setTOSType(bf->returnType());
} 

void BytecodeTranslator::visitNativeCallNode(NativeCallNode * node) { 
    string funcName = node->nativeName();
    void* nativeCode = dlsym(RTLD_DEFAULT, funcName.c_str());
    if (!nativeCode) {
        throw new std::runtime_error("Native function '" + funcName + "' does not exist");
    }
    uint16_t fn_id = code->makeNativeFunction(node->nativeName(), node->nativeSignature(), nativeCode);
    bc()->addInsn(BC_CALLNATIVE);
    bc()->addUInt16(fn_id);
    castTOS(node->nativeSignature()[0].first);

    bc()->addInsn(BC_ILOAD0);
    context->setTOSType(VT_INT);
} 

void BytecodeTranslator::visitPrintNode(PrintNode *node) { 
        for (size_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        Instruction ins = BC_INVALID;
        switch (context->getTOSType()) {
            case VT_INT:
                ins = BC_IPRINT;
                break;
            case VT_DOUBLE:
                ins = BC_DPRINT;
                break;
            case VT_STRING:
                ins = BC_SPRINT;
                break;
            default:
                throw new std::runtime_error("Unknown type of printing parameter");
        }
        bc()->addInsn(ins);
        context->setTOSType(VT_VOID);
    }
    bc()->addInsn(BC_ILOAD0);
    context->setTOSType(VT_VOID);
} 

Instruction BytecodeTranslator::chooseInsn(VarType type, Instruction intInsn, Instruction doubleInsn) {
    if (type == VT_INT) return intInsn;
    if (type == VT_DOUBLE) return doubleInsn;
    throw new runtime_error("Unknown type to choose");
}

void BytecodeTranslator::castArithmTypes(VarType type1, VarType type2) {
    if (type1 == VT_INT) {
        if (type2 == VT_INT) {
            return;
        } else if (type2 == VT_DOUBLE) {
            bc()->addInsn(BC_I2D);
            context->setTOSType(VT_DOUBLE);
            return;
        }
    } else if (type1 == VT_DOUBLE) {
        if (type2 == VT_DOUBLE) {
            return;
        } else if (type2 == VT_INT) {
            bc()->addInsn(BC_SWAP);
            bc()->addInsn(BC_I2D);
            bc()->addInsn(BC_SWAP);
            return;
        }
    }
    throw new runtime_error(string("Incompatible types: ") + typeToName(type1) + 
                            "," + typeToName(type2));
}

void BytecodeTranslator::castTOS(VarType type) {
    if ((type == VT_STRING && context->getTOSType() == VT_STRING) ||
        (type == VT_VOID && context->getTOSType() == VT_VOID)) {
        return;
    }
    if (type == VT_INT) {
        switch (context->getTOSType()) {
            case VT_INT:
                break;
            case VT_DOUBLE:
                bc()->addInsn(BC_D2I);
                break;
           case VT_STRING:
               bc()->addInsn(BC_S2I);
               break;
            default:
                throw new std::runtime_error("Incorrect cast");
        }
        context->setTOSType(VT_INT);
        return;
    }
    if (type == VT_DOUBLE) {
        switch (context->getTOSType()) {
            case VT_DOUBLE:
                break;
            case VT_INT:
                bc()->addInsn(BC_I2D);
                break;
           case VT_STRING:
               bc()->addInsn(BC_S2I);
               bc()->addInsn(BC_I2D);
               break;
            default:
                throw new std::runtime_error("Incorrect cast");
        }
        context->setTOSType(VT_DOUBLE);
        return;
    }
    throw new runtime_error("Incorrect cast");
}

}

