#include <sstream>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "Translator.h"
#include "ShowVisitor.h"

#define INTERNAL_ERROR \
    throwError(node, "Internal error at %s:%d", __FILE__, __LINE__);

void throwError(mathvm::AstNode* node, const char* format, ...) {
    char *buf;
    va_list args;
    va_start(args, format);
    vasprintf(&buf, format, args);
    mathvm::Status s(buf, node->position());
    free(buf);
    throw s;
}

std::string showExpr(mathvm::AstNode* node) {
    std::stringstream str;
    ShowVisitor v(str);
    v.show(node);
    return str.str();
}

void typeMismatch(const char* e, mathvm::AstNode* expr, mathvm::VarType a) {
    throwError(expr, "Expected expression of type %s, but %s has type %s",
        e, showExpr(expr).c_str(), typeToName(a));
}

void Translator::checkTypeInt(mathvm::AstNode* expr) {
    expr->visit(this);
    if (currentType != mathvm::VT_INT) {
        typeMismatch("int", expr, currentType);
    }
    currentType = mathvm::VT_INVALID;
}

Translator::Translator(mathvm::Code* p): prog(p), currentVar(0),
    overflow(false), currentType(mathvm::VT_INVALID), resultType(mathvm::VT_VOID) { }

void Translator::put(const void* buf_, unsigned int size) {
    const uint8_t* buf = (const uint8_t*)buf_;
    while (size--) {
        code->add(*buf++);
    }
}

template<class T>
void Translator::putVar(mathvm::Instruction ins, T* node) {
    code->add(ins);
    const std::vector<VarInt>& v = vars[node->var()->name()];
    if (v.empty()) {
        VarInt x = 0;
        put(&x, sizeof(VarInt));
        return;
//        throwError(node, "Undeclared variable: %s", node->var()->name().c_str());
    }
    put(&v.back(), sizeof(VarInt));
}

void Translator::triple(mathvm::Instruction i) {
    mathvm::Label label(code), label1(code);
    code->addBranch(i, label);
    code->add(mathvm::BC_ILOAD0);
    code->addBranch(mathvm::BC_JA, label1);
    code->bind(label);
    code->add(mathvm::BC_ILOAD1);
    code->bind(label1);
}

void Translator::visitBinaryOpNode(mathvm::BinaryOpNode* node) {
    using namespace mathvm;
    
    if (node->kind() == tOR || node->kind() == tAND) {
        checkTypeInt(node->left());
        code->add(BC_ILOAD0);
        Label label(code), label1(code);
        code->addBranch(node->kind() == tOR ? BC_IFICMPE : BC_IFICMPNE, label);
        code->add(node->kind() == tOR ? BC_ILOAD1 : BC_ILOAD0);
        code->addBranch(BC_JA, label1);
        code->bind(label);
        node->right()->visit(this);
        code->bind(label1);
        if (currentType != VT_INT) {
            typeMismatch("int", node->right(), currentType);
        }
        return;
    }
    node->right()->visit(this);
    VarType right = currentType;
    currentType = VT_INVALID;
    node->left()->visit(this);
    VarType left = currentType;
    switch (node->kind()) {
        case tEQ: case tNEQ:
        case tGT: case tGE: case tLT: case tLE:
        case tADD: case tSUB: case tMUL: case tDIV:
            if (left != VT_INT && left != VT_DOUBLE) {
                typeMismatch("int or double", node->left(), left);
            }
            if (right != VT_INT && right != VT_DOUBLE) {
                typeMismatch("int or double", node->right(), right);
            }
            if (left != right) {
                if (left == VT_INT) {
                    currentType = VT_DOUBLE;
                    code->add(BC_I2D);
                } else {
                    code->add(BC_SWAP);
                    code->add(BC_I2D);
                    if (node->kind() != tADD && node->kind() != tMUL) {
                        code->add(BC_SWAP);
                    }
                }
            }
            break;
        default: throwError(node, "Unexpected token %s", tokenOp(node->kind()));
    }
    if (currentType == VT_DOUBLE) {
        switch (node->kind()) {
            case tEQ: case tNEQ: case tGT: case tGE: case tLT: case tLE:
                code->add(BC_DCMP); currentType = VT_INT;
            default:;
        }
        switch (node->kind()) {
            case tNEQ: break;
            case tEQ: code->add(BC_ILOAD0); triple(BC_IFICMPE); break;
            case tGT: code->add(BC_ILOADM1); triple(BC_IFICMPE); break;
            case tGE: code->add(BC_ILOAD1); triple(BC_IFICMPNE); break;
            case tLT: code->add(BC_ILOAD1); triple(BC_IFICMPE); break;
            case tLE: code->add(BC_ILOADM1); triple(BC_IFICMPNE); break;
            case tADD: code->add(BC_DADD); break;
            case tSUB: code->add(BC_DSUB); break;
            case tMUL: code->add(BC_DMUL); break;
            case tDIV: code->add(BC_DDIV); break;
            default:;
        }
    } else
    switch (node->kind()) {
        case tEQ: triple(BC_IFICMPE); break;
        case tNEQ: triple(BC_IFICMPNE); break;
        case tGT: triple(BC_IFICMPG); break;
        case tGE: triple(BC_IFICMPGE); break;
        case tLT: triple(BC_IFICMPL); break;
        case tLE: triple(BC_IFICMPLE); break;
        case tADD: code->add(BC_IADD); break;
        case tSUB: code->add(BC_ISUB); break;
        case tMUL: code->add(BC_IMUL); break;
        case tDIV: code->add(BC_IDIV); break;
        default:;
    }
}

void Translator::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
    node->operand()->visit(this);
    switch (node->kind()) {
        case mathvm::tSUB: switch (currentType) {
            case mathvm::VT_INT: code->add(mathvm::BC_INEG); break;
            case mathvm::VT_DOUBLE: code->add(mathvm::BC_DNEG); break;
            default: typeMismatch("int or double", node->operand(), currentType);
        } break;
        case mathvm::tNOT: {
            if (currentType != mathvm::VT_INT) {
                typeMismatch("int", node->operand(), currentType);
            }
            code->add(mathvm::BC_ILOAD0);
            triple(mathvm::BC_IFICMPE);
            break;
        }
        default: INTERNAL_ERROR
    }
}

void Translator::visitStringLiteralNode(mathvm::StringLiteralNode* node) {
    code->add(mathvm::BC_SLOAD);
    code->addInt16(prog->makeStringConstant(node->literal()));
    currentType = mathvm::VT_STRING;
}

void Translator::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
    if (node->literal() == -1) {
        code->add(mathvm::BC_DLOADM1);
    } else
    if (node->literal() == 0) {
        code->add(mathvm::BC_DLOAD0);
    } else
    if (node->literal() == 1) {
        code->add(mathvm::BC_DLOAD1);
    } else {
        code->add(mathvm::BC_DLOAD);
        if (sizeof(double) == 8) {
            double l = node->literal();
            put(&l, sizeof(l));
        } else
        if (sizeof(long double) == 8) {
            long double l = node->literal();
            put(&l, sizeof(l));
        } else
        if (sizeof(float) == 8) {
            float l = node->literal();
            put(&l, sizeof(l));
        } else throwError(node, "I'm sorry, Dave. "
            "I'm afraid I can't compile a code with floating point numbers, "
            "cause your system doesn't have 8-byte numbers.");
    }
    currentType = mathvm::VT_DOUBLE;
}

void Translator::visitIntLiteralNode(mathvm::IntLiteralNode* node) {
    int64_t l = node->literal();
    switch (l) {
        case -1: code->add(mathvm::BC_ILOADM1); break;
        case 0: code->add(mathvm::BC_ILOAD0); break;
        case 1: code->add(mathvm::BC_ILOAD1); break;
        default: code->add(mathvm::BC_ILOAD); put(&l, sizeof(l));
    }
    currentType = mathvm::VT_INT;
}

void Translator::visitLoadNode(mathvm::LoadNode* node) {
    switch (node->var()->type()) {
        case mathvm::VT_DOUBLE: putVar(mathvm::BC_LOADDVAR, node); break;
        case mathvm::VT_INT: putVar(mathvm::BC_LOADIVAR, node); break;
        case mathvm::VT_STRING: putVar(mathvm::BC_LOADSVAR, node); break;
        default: INTERNAL_ERROR
    }
    currentType = node->var()->type();
}

void Translator::visitStoreNode(mathvm::StoreNode* node) {
    mathvm::AstNode* right = node->value();
    mathvm::LoadNode l(0, node->var());
    mathvm::BinaryOpNode tnode(0, node->op() == mathvm::tINCRSET
        ? mathvm::tADD : mathvm::tSUB, &l, right);
    switch (node->op()) {
        case mathvm::tASSIGN: break;
        case mathvm::tINCRSET:
        case mathvm::tDECRSET: right = &tnode; break;
        default: INTERNAL_ERROR
    }
    right->visit(this);
    if (node->var()->type() != currentType) {
        typeMismatch(typeToName(node->var()->type()), node->value(), currentType);
    }
    switch (currentType) {
        case mathvm::VT_DOUBLE: putVar(mathvm::BC_STOREDVAR, node); break;
        case mathvm::VT_INT: putVar(mathvm::BC_STOREIVAR, node); break;
        case mathvm::VT_STRING: putVar(mathvm::BC_STORESVAR, node); break;
        default: INTERNAL_ERROR
    }
    currentType = mathvm::VT_INVALID;
}

void Translator::visitForNode(mathvm::ForNode* node) {
    if (node->var()->type() != mathvm::VT_INT) {
        throwError(node, "Variable %s should have type int",
            node->var()->name().c_str());
    }
    mathvm::BinaryOpNode* in =
        dynamic_cast<mathvm::BinaryOpNode*>(node->inExpr());
    if (!in || in->kind() != mathvm::tRANGE) {
        node->inExpr()->visit(this);
        typeMismatch("range", node->inExpr(), currentType);
    }
    checkTypeInt(in->left());
    putVar(mathvm::BC_STOREIVAR, node);
    mathvm::Label start(code), end(code);
    code->bind(start);
    checkTypeInt(in->right());
    putVar(mathvm::BC_LOADIVAR, node);
    code->addBranch(mathvm::BC_IFICMPGE, end);
    node->body()->visit(this);
    currentType = mathvm::VT_INVALID;
    putVar(mathvm::BC_LOADIVAR, node);
    code->add(mathvm::BC_ILOAD1);
    code->add(mathvm::BC_IADD);
    putVar(mathvm::BC_STOREIVAR, node);
    code->addBranch(mathvm::BC_JA, start);
    code->bind(end);
}

void Translator::visitWhileNode(mathvm::WhileNode* node) {
    mathvm::Label start(code), end(code);
    code->bind(start);
    mathvm::IntLiteralNode* in =
        dynamic_cast<mathvm::IntLiteralNode*>(node->whileExpr());
    if (in) {
        if (in->literal() == 0) {
            return;
        }
    } else {
        checkTypeInt(node->whileExpr());
        code->add(mathvm::BC_ILOAD0);
        code->addBranch(mathvm::BC_IFICMPE, end);
    }
    node->loopBlock()->visit(this);
    currentType = mathvm::VT_INVALID;
    code->addBranch(mathvm::BC_JA, start);
    code->bind(end);
}

void Translator::visitIfNode(mathvm::IfNode* node) {
    checkTypeInt(node->ifExpr());
    code->add(mathvm::BC_ILOAD0);
    mathvm::Label label(code);
    code->addBranch(mathvm::BC_IFICMPE, label);
    node->thenBlock()->visit(this);
    currentType = mathvm::VT_INVALID;
    if (node->elseBlock()) {
        mathvm::Label label1(code);
        code->addBranch(mathvm::BC_JA, label1);
        code->bind(label);
        node->elseBlock()->visit(this);
        currentType = mathvm::VT_INVALID;
        code->bind(label1);
    } else {
        code->bind(label);
    }
}

void Translator::visitBlockNode(mathvm::BlockNode* node) {
    for (mathvm::Scope::VarIterator it(node->scope()); it.hasNext();) {
        if (overflow) {
            throwError(node, "I'm sorry, Dave. I'm afraid you exceeded the limit of"
                " variables which is %d.", (unsigned long long)1 + VarInt(-1));
        }
        vars[it.next()->name()].push_back(currentVar++);
        if (!currentVar) {
            overflow = true;
        }
    }
    for (mathvm::Scope::FunctionIterator it(node->scope()); it.hasNext();) {
        mathvm::Bytecode* code1 = code;
        mathvm::AstFunction* fun = it.next();
        mathvm::BytecodeFunction* bfun = new mathvm::BytecodeFunction(fun);
        prog->addFunction(bfun);
        code = bfun->bytecode();
        fun->node()->visit(this);
        code = code1;
    }
    node->visitChildren(this);
    
    mathvm::Scope::VarIterator it(node->scope());
    overflow = overflow && !it.hasNext();
    while (it.hasNext()) {
        vars[it.next()->name()].pop_back();
        --currentVar;
    }
}

void Translator::visitFunctionNode(mathvm::FunctionNode* node) {
    mathvm::VarType resType = resultType;
    resultType = node->returnType();
    node->body()->visit(this);
    resultType = resType;
}

void Translator::visitPrintNode(mathvm::PrintNode* node) {
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        switch (currentType) {
            case mathvm::VT_INT: code->add(mathvm::BC_IPRINT); break;
            case mathvm::VT_DOUBLE: code->add(mathvm::BC_DPRINT); break;
            case mathvm::VT_STRING: code->add(mathvm::BC_SPRINT); break;
            case mathvm::VT_VOID: typeMismatch("int or double or string",
                node->operandAt(i), mathvm::VT_VOID); break;
            default: INTERNAL_ERROR
        }
        currentType = mathvm::VT_INVALID;
    }
    currentType = mathvm::VT_VOID;
}

void Translator::visitReturnNode(mathvm::ReturnNode* node) {
    if (node->returnExpr()) {
        if (resultType == mathvm::VT_VOID) {
            throwError(node, "Returning a value from a void function");
        }
        node->returnExpr()->visit(this);
        if (resultType != currentType) {
            if (currentType == mathvm::VT_INT && resultType == mathvm::VT_DOUBLE) {
                code->add(mathvm::BC_I2D);
            } else {
                typeMismatch(typeToName(resultType), node->returnExpr(), currentType);
            }
        }
    } else {
        if (resultType != mathvm::VT_VOID) {
            throwError(node, "Returning no value from a non-void function");
        }
    }
    currentType = mathvm::VT_INVALID;
    code->add(mathvm::BC_RETURN);
}

void Translator::visitCallNode(mathvm::CallNode* node) {
    mathvm::TranslatedFunction* fun = prog->functionByName(node->name());
    if (!fun) {
        throwError(node, "Undeclared function: %s", node->name().c_str());
    }
    if (fun->parametersNumber() != node->parametersNumber()) {
        throwError(node, "Function %s expects %d arguments but got %d",
            node->name().c_str(), fun->parametersNumber(), node->parametersNumber());
    }
    for (uint32_t i = 0; i < fun->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        if (currentType != fun->parameterType(i)) {
            if (currentType == mathvm::VT_INT && fun->parameterType(i) == mathvm::VT_DOUBLE) {
                code->add(mathvm::BC_I2D);
            } else {
                throwError(node, "Expected expression of type %s "
                    "for argument %d to function %s but %s has type %s",
                    typeToName(fun->parameterType(i)), i, node->name().c_str(),
                    showExpr(node->parameterAt(i)).c_str(), typeToName(currentType));
            }
        }
    }
    code->add(mathvm::BC_CALL);
    code->addTyped(fun->id());
    currentType = fun->returnType();
}

mathvm::Status Translator::translate(mathvm::AstFunction* fun) {
    try {
        mathvm::BytecodeFunction* main = new mathvm::BytecodeFunction(fun);
        prog->addFunction(main);
        code = main->bytecode();
        fun->node()->body()->visit(this);
        code->add(mathvm::BC_STOP);
    } catch (const mathvm::Status& e) {
        return e;
    }
    return mathvm::Status();
}
