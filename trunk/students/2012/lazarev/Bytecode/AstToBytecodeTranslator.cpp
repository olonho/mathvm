#include "AstToBytecodeTranslator.h"
#include <iostream>

AstToBytecodeTranslator::AstToBytecodeTranslator() {
    arithmetic[VT_INT][tADD] = BC_IADD;
    arithmetic[VT_INT][tSUB] = BC_ISUB;
    arithmetic[VT_INT][tMUL] = BC_IMUL;
    arithmetic[VT_INT][tDIV] = BC_IDIV;
    arithmetic[VT_INT][tMOD] = BC_IMOD;

    arithmetic[VT_DOUBLE][tADD] = BC_IADD;
    arithmetic[VT_DOUBLE][tSUB] = BC_ISUB;
    arithmetic[VT_DOUBLE][tMUL] = BC_IMUL;
    arithmetic[VT_DOUBLE][tDIV] = BC_IDIV;

    comparison[tQE] = BC_IFICMPE;
    comparison[tNEQ] = BC_IFICMPNE;
    comparison[tGT] = BC_IFICMPG;
    comparison[tGE] = BC_IFICMPGE;
    comparison[tLT] = BC_IFICMPL;
    comparison[tLE] = BC_IFICMPLE;
}

void AstToBytecodeTranslator::visitBinaryOpNode(BinaryOpNode* node) {
    Bytecode *bytecode = getBytecode();

    node -> right() -> visit(this);
    node -> left() -> visit(this);

    VarType firstType = types.top();
    types.pop();
    VarType secondType = types.top();
    types.pop();

    VarType commonType;
    if (secondType == firstType && (firstType == VT_INT || firstType == VT_DOUBLE)) {
        commonType = firstType;
    } else if (firstType == VT_DOUBLE && secondType == VT_INT) {
        bytecode -> addInsn(BC_SWAP);
        bytecode -> addInsn(BC_I2D);
        bytecode -> addInsn(BC_SWAP);
        commonType = VT_DOUBLE;
    } else if (secondType == VT_DOUBLE && firstType == VT_INT) {
        bytecode -> addInsn(BC_I2D);
        commonType = VT_DOUBLE;
    } else {
        throw std::exception("Invalid operands types");
    }

    TokenKind op = node -> kind();
    switch (op) {
        case tRANGE:
            if (commonType == VT_INT) {
                types.push(secondType);
                types.push(firstType);
                return;
            } else {
                throw std::exception("Range bounds must integer");
            }

        case tADD:
        case tSUB:
        case tMUL:
        case tDIV:
        case tMOD:
            if (arithmetic[commonType].find(op) == arithmetic[commonType].end()) {
                throw std::exception("Can't apply given operation to operands");
            }
            bytecode -> addInsn(arithmetic[commonType][op]);
            types.push(commonType);
            break;

        case tAND:  // TODO
        case tOR:   // TODO
            break;

        case tEQ:
        case tNEQ:
        case tGT:
        case tGE:
        case tLT:
        case tLE;
            if (commonType == VT_INT) {
                compareInts(comparison[op]);
            } else if (commonType == VT_DOUBLE) {
                compareDoubles(comparison[op]);
            }
            types.push(VT_INT);
            break;
    }
}

void AstToBytecodeTranslator::visitUnaryOpNode(UnaryOpNode* node) { // DONE
    Bytecode *bytecode = getBytecode();

    node -> operand() -> visit(this);

    switch (node -> kind()) {
        case tSUB:
            if (types.top() == VT_INT) {
                bytecode -> addInsn(BC_INEG);
            } else if (types.top() == VT_DOUBLE) {
                bytecode -> addInsn(BC_DNEG);
            } else {
                throw std::exception("SUB can be applied only to INT or DOUBLE");
            }
            break;
        case tNOT:
            if (types.top() == VT_INT) {
                bytecode -> addInsn(BC_ILODA0);
                bytecode -> addInsn(BC_IFICMPE);
                bytecode -> addInt16(5);
                bytecode -> addInsn(BC_SWAP);
                bytecode -> addInsn(BC_POP);
                bytecode -> addInsn(BC_JA);
                bytecode -> addInt16(3);
                bytecode -> addInsn(BC_POP);
                bytecode -> addInsn(BC_POP);
                bytecode -> addInsn(BC_ILOAD1);
            } else if (types.top() == VT_DOUBLE) {
                bytecode -> addInsn(BC_DLODA0);
                bytecode -> addInsn(BC_DCMP);
                bytecode -> addInsn(BC_ILODA0);
                bytecode -> addInsn(BC_IFICMPE);
                bytecode -> addInt16(7);
                bytecode -> addInsn(BC_POP);
                bytecode -> addInsn(BC_POP);
                bytecode -> addInsn(BC_POP);
                bytecode -> addInsn(BC_ILOAD0);
                bytecode -> addInsn(BC_JA);
                bytecode -> addInt16(4);
                bytecode -> addInsn(BC_POP);
                bytecode -> addInsn(BC_POP);
                bytecode -> addInsn(BC_POP);
                bytecode -> addInsn(BC_ILOAD1);

                types.pop();
                types.push(VT_INT);
            } else {
                throw std::exception("NOT can be applied only to INT or DOUBLE");
            }
            break;
        default:
            throw std::exception("unknown unary operation");
    }
}

void AstToBytecodeTranslator::visitStringLiteralNode(StringLiteralNode* node) {
    Bytecode *bytecode = getBytecode();
    bytecode -> addInsn(BC_SLOAD);
    bytecode -> addTyped(node -> literal());    // wrong!!! should be address of the string! TODO
    types.push(VT_STRING);
}

void AstToBytecodeTranslator::visitDoubleLiteralNode(DoubleLiteralNode* node) { // DONE
    Bytecode *bytecode = getBytecode();
    bytecode -> addInsn(BC_DLOAD);
    bytecode -> addDouble(node -> literal());
    types.push(VT_DOUBLE);
}

void AstToBytecodeTranslator::visitIntLiteralNode(IntLiteralNode* node) { // DONE
    Bytecode *bytecode = getBytecode();
    bytecode -> addInsn(BC_ILOAD);
    bytecode -> addInt64(node -> literal());
    types.push(VT_INT);
}

void AstToBytecodeTranslator::visitLoadNode(LoadNode* node) {
    std::cout << node->var()->name();
    blockEnded = false;

    types.push(node -> var() -> type());
}

void AstToBytecodeTranslator::visitStoreNode(StoreNode* node) {
    Bytecode *bytecode = getBytecode();

    node -> value() -> visit(this);

    TokenKind op = node -> op();
    switch (op) {
        case tASSIGN:
            bytecode -> addInsn();
        case tINCRSET:
        case tDECRSET:
    }
    std::cout << node->var()->name() << " " << opToString(node->op()) << " ";
    
    blockEnded = false;

    types.push(node -> var() -> type());
}

void AstToBytecodeTranslator::visitForNode(ForNode* node) { // Add variable id
    Bytecode *bytecode = getBytecode();

    node -> inExpr() -> visit(this);

    int jumpIndex = bytecode() -> length();
    bytecode -> addInsn(BC_IFICMPG);
    int jumpOffsetIndex = bytecode() -> length();
    bytecode -> addInt16(0);
    
    node->body()->visit(this);

    // updating cycle variable
    bytecode -> addInsn(BC_STOREIVAR);
    bytecode -> addUInt16();                //add varID
    bytecode -> addInsn(BC_LOADIVAR);
    bytecode -> addUInt16();                //add varID


    bytecode -> addInsn(BC_JA);
    bytecode -> addInt16(jumpIndex - (bytecode() -> length() - 1));

    int endIndex = bytecode -> length();
    bytecode -> setInt16(jumpOffsetIndex, endIndex - jumpIndex);

    // deleting cycle bounds parameters
    bytecode -> addInsn(BC_POP);
    bytecode -> addInsn(BC_POP);
    types.pop();
    types.pop();
}

void AstToBytecodeTranslator::visitWhileNode(WhileNode* node) { // DONE
    Bytecode *bytecode = getBytecode();

    int startIndex = bytecode -> length();
    node -> whileExpr() -> visit(this);
    bytecode -> addInsn(BC_ILOAD0);
    int jumpIndex = bytecode -> length();
    bytecode -> addInsn(BC_IFICMPE);
    int jumpOffsetIndex = bytecode -> length();
    bytecode -> addInt16(0);

    removeConditionCheckingParams();

    node->loopBlock()->visit(this);
    int returnIndex = bytecode -> length();
    bytecode -> addInsn(BC_JA);
    bytecode -> addInt16(startIndex - returnIndex);

    int endIndex = bytecode -> length();
    bytecode -> setInt16(jumpOffsetIndex, endIndex - jumpIndex);

    removeConditionCheckingParams();
}

void AstToBytecodeTranslator::visitIfNode(IfNode* node) {   // DONE
    Bytecode *bytecode = getBytecode();

    node -> ifExpr() -> visit(this);
    bytecode -> addInsn(BC_ILOAD0);
    int jumpFromIf = bytecode -> length();
    bytecode -> addInsn(BC_IFICMPE);
    int jumpFromIfOffset = bytecode -> length();
    bytecode -> addInt16(0);

    removeConditionCheckingParams();

    int thenBlockStartIndex = bytecode -> length();
    node->thenBlock()->visit(this);

    if ((node -> elseBlock()) != 0) {
        int jumpFromThen = bytecode -> length();
        bytecode -> addInsn(BC_JA);
        int jumpFromThenOffset = bytecode -> length();
        bytecode -> addInt16(0);

        bytecode -> setInt16(jumpFromIfOffset, (bytecode -> length()) - jumpFromIf);

        node -> elseBlock() -> visit(this);
        int elseBlockEndIndex = bytecode -> length();
        bytecode -> setInt16(jumpFromThenOffset, elseBlockEndIndex - jumpFromThen);
    } else {
        bytecode -> setInt16(jumpFromIfOffset, (bytecode -> length()) - jumpFromIf);
    }

    removeConditionCheckingParams();
}

void AstToBytecodeTranslator::visitBlockNode(BlockNode* node) {
    Scope::VarIterator vars(node->scope());
    while (vars.hasNext()) {
        AstVar* var = vars.next();
        std::cout << typeToName(var->type()) << " " << var->name() << ";" << std::endl;
    }

    Scope::FunctionIterator functions(node->scope());
    while (functions.hasNext()) {
        functions.next()->node()->visit(this);
    }

    for (unsigned int i = 0; i < node->nodes(); i++) {
        node->nodeAt(i)->visit(this);
        if (!blockEnded) {
            std::cout << ";" << std::endl;
        }
    }
    blockEnded = false;
}

void AstToBytecodeTranslator::visitFunctionNode(FunctionNode* node) {
    std::cout << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
    for (unsigned int i = 0; i < node->parametersNumber(); i++) {
        std::cout << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
        if (i != node->parametersNumber() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << ") ";
    if (node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->nodeAt(0)->visit(this);
        std::cout << ";" << std::endl;
        blockEnded = false;
    } else {
        std::cout << "{" << std::endl;
        node->body()->visit(this);
        std::cout << "}" << std::endl;
        blockEnded = true;
    }
}

void AstToBytecodeTranslator::visitReturnNode(ReturnNode* node) {
    std::cout << "return";
    if (node->returnExpr() != 0) {
        std::cout << " ";
        node->returnExpr()->visit(this);
    }
    blockEnded = false;
}

void AstToBytecodeTranslator::visitCallNode(CallNode* node) {
    std::cout << node->name() << "(";
    for (unsigned int i = 0; i < node->parametersNumber(); i++) {
        node->parameterAt(i)->visit(this);
        if (i != node->parametersNumber() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << ")";
    blockEnded = false;
}

void AstToBytecodeTranslator::visitNativeCallNode(NativeCallNode* node) {
    std::cout << "native '" << node->nativeName() << "'";
    blockEnded = false;
}

void AstToBytecodeTranslator::visitPrintNode(PrintNode* node) {
    std::cout << "print(";
    for (unsigned int i = 0; i < node->operands(); i++) {
        node->operandAt(i)->visit(this);
        if (i != node->operands() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << ")";
    blockEnded = false;
}


void AstToBytecodeTranslator::removeConditionCheckingParams() {
    Bytecode *bytecode = getBytecode();
    // deleting zero constant and result of condition evaluation from stack
    bytecode -> addInsn(BC_POP);
    bytecode -> addInsn(BC_POP);
    // deleting type of condition evaluation result from types stack
    types.pop();
}

void AstToBytecodeTranslator::compareInts(Instruction insn) {
    Bytecode *bytecode = getBytecode();

    bytecode -> addInsn(insn);
    bytecode -> addInt16(6);
    bytecode -> addInsn(BC_POP);
    bytecode -> addInsn(BC_POP);
    bytecode -> addInsn(BC_ILOAD0);
    bytecode -> addInsn(BC_POP);
    bytecode -> addInsn(BC_POP);
    bytecode -> addInsn(BC_ILOAD1);
}

void AstToBytecodeTranslator::compareDoubles(Instruction insn) {
    Bytecode *bytecode = getBytecode();

    bytecode -> addInsn(BC_DCMP);
    bytecode -> addInsn(BC_SWAP);
    bytecode -> addInsn(BC_POP);
    bytecode -> addInsn(BC_SWAP);
    bytecode -> addInsn(BC_POP);
    bytecode -> addInsn(BC_ILOAD0);

    compareInts(insn);
}