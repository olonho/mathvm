#include <sstream>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <assert.h>

#include "Translator.h"
#include "AstShowVisitor.h"

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
    AstShowVisitor v(str);
    v.show(node);
    return str.str();
}

void typeMismatch(const char* e, mathvm::AstNode* expr, mathvm::VarType a) {
    throwError(expr, "Expected expression of type %s, but %s has type %s",
        e, showExpr(expr).c_str(), typeToName(a));
}

void Translator::checkTypeInt(mathvm::AstNode* expr) {
    expr->visit(this);
    assert(currentType == mathvm::VT_INT);
    currentType = mathvm::VT_INVALID;
}

Translator::Translator(mathvm::Code* p): prog(p), currentVar(0),
    overflow(false), currentType(mathvm::VT_INVALID), resultType(mathvm::VT_VOID) { }

// TODO
void Translator::visitNativeCallNode( mathvm::NativeCallNode* node ) {
///
}

void Translator::put(const void* buf_, unsigned int size) {
    const uint8_t* buf = (const uint8_t*)buf_;
    while (size--) {
        code->add(*buf++);
    }
}

template<class T>
void Translator::putVar(mathvm::Instruction ins, T* node) {
    code->add(ins);
    std::vector<VarInt>& v = vars[node->var()->name()];
    if (v.empty()) {
	throwError(node, "Error: Undeclared variable: %s", node->var()->name().c_str());
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
    if (node->kind() == mathvm::tOR || node->kind() == mathvm::tAND) {
        checkTypeInt(node->left());
        code->add(mathvm::BC_ILOAD0);
//
        mathvm::Label label(code), label1(code);
        code->addBranch(node->kind() == mathvm::tOR ? mathvm::BC_IFICMPE : mathvm::BC_IFICMPNE, label);
        code->add(node->kind() == mathvm::tOR ? mathvm::BC_ILOAD1 : mathvm::BC_ILOAD0);
        code->addBranch(mathvm::BC_JA, label1);
        code->bind(label);
        node->right()->visit(this);
        code->bind(label1);
	assert(currentType == mathvm::VT_INT);
        return;
    }
    node->right()->visit(this);
    mathvm::VarType right = currentType;
    currentType = mathvm::VT_INVALID;
    node->left()->visit(this);
    mathvm::VarType left = currentType;
    switch (node->kind()) {
        case mathvm::tEQ: 
	case mathvm::tNEQ:
        case mathvm::tGT: 
	case mathvm::tGE: 
	case mathvm::tLT: 
	case mathvm::tLE:
        case mathvm::tADD: 
	case mathvm::tSUB: 
	case mathvm::tMUL: 
	case mathvm::tDIV:
            if (left != mathvm::VT_INT && left != mathvm::VT_DOUBLE) {
                typeMismatch("int or double", node->left(), left);
            }
            if (right != mathvm::VT_INT && right != mathvm::VT_DOUBLE) {
                typeMismatch("int or double", node->right(), right);
            }
            if (left != right) {
                if (left == mathvm::VT_INT) {
                    currentType = mathvm::VT_DOUBLE;
                    code->add(mathvm::BC_I2D);
                } else {
                    code->add(mathvm::BC_SWAP);
                    code->add(mathvm::BC_I2D);
                    if (node->kind() != mathvm::tADD && node->kind() != mathvm::tMUL) {
                        code->add(mathvm::BC_SWAP);
                    }
                }
            }
            break;
        default: throwError(node, "Error: Unexpected token %s", tokenOp(node->kind()));
    }

    if (currentType == mathvm::VT_DOUBLE) {
        switch (node->kind()) {
            case mathvm::tEQ: case mathvm::tNEQ: case mathvm::tGT: case mathvm::tGE: case mathvm::tLT: case mathvm::tLE:
                code->add(mathvm::BC_DCMP); currentType = mathvm::VT_INT;
            default: std::cout << "Error: Unexpected token" << std::endl;
        }
        switch (node->kind()) {
            case mathvm::tNEQ: break;
            case mathvm::tEQ: code->add(mathvm::BC_ILOAD0); triple(mathvm::BC_IFICMPE); break;
            case mathvm::tGT: code->add(mathvm::BC_ILOADM1); triple(mathvm::BC_IFICMPE); break;
            case mathvm::tGE: code->add(mathvm::BC_ILOAD1); triple(mathvm::BC_IFICMPNE); break;
            case mathvm::tLT: code->add(mathvm::BC_ILOAD1); triple(mathvm::BC_IFICMPE); break;
            case mathvm::tLE: code->add(mathvm::BC_ILOADM1); triple(mathvm::BC_IFICMPNE); break;
            case mathvm::tADD: code->add(mathvm::BC_DADD); break;
            case mathvm::tSUB: code->add(mathvm::BC_DSUB); break;
            case mathvm::tMUL: code->add(mathvm::BC_DMUL); break;
            case mathvm::tDIV: code->add(mathvm::BC_DDIV); break;
            default: std::cout << "Error: Unexpected token" << std::endl;
        }
    } else {
    	switch (node->kind()) {
    	    case mathvm::tEQ: triple(mathvm::BC_IFICMPE); break;
    	    case mathvm::tNEQ: triple(mathvm::BC_IFICMPNE); break;
    	    case mathvm::tGT: triple(mathvm::BC_IFICMPG); break;
    	    case mathvm::tGE: triple(mathvm::BC_IFICMPGE); break;
    	    case mathvm::tLT: triple(mathvm::BC_IFICMPL); break;
    	    case mathvm::tLE: triple(mathvm::BC_IFICMPLE); break;
    	    case mathvm::tADD: code->add(mathvm::BC_IADD); break;
    	    case mathvm::tSUB: code->add(mathvm::BC_ISUB); break;
    	    case mathvm::tMUL: code->add(mathvm::BC_IMUL); break;
    	    case mathvm::tDIV: code->add(mathvm::BC_IDIV); break;
    	    default: std::cout << "Error: Unexpected token" << std::endl;
        }
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
        default: throwError(node, "Internal error");
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
        } else throwError(node, "Error: could not comilate code");
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
        default: throwError(node, "Internal error");
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
        default: throwError(node, "Internal error");
    }
    right->visit(this);
    if (node->var()->type() != currentType) {
        typeMismatch(typeToName(node->var()->type()), node->value(), currentType);
    }
    switch (currentType) {
        case mathvm::VT_DOUBLE: putVar(mathvm::BC_STOREDVAR, node); break;
        case mathvm::VT_INT: putVar(mathvm::BC_STOREIVAR, node); break;
        case mathvm::VT_STRING: putVar(mathvm::BC_STORESVAR, node); break;
        default: throwError(node, "Internal error");
    }
    currentType = mathvm::VT_INVALID;
}

void Translator::visitForNode(mathvm::ForNode* node) {
    if (node->var()->type() != mathvm::VT_INT) {
        throwError(node, "Variable %s should have type int", node->var()->name().c_str());
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

Translator::VarInt Translator::addVar(mathvm::AstNode* node, const std::string& name) {
    VarInt r = currentVar++;
    if (overflow) {
        throwError(node, "Error: limit variable");
    }
    if (!currentVar) {
        overflow = true;
    }
    vars[name].push_back(r);
    return r;
}

void Translator::delVar(const std::string& name) {
    vars[name].pop_back();
    overflow = false;
    --currentVar;
}

void Translator::visitBlockNode(mathvm::BlockNode* node) {
    for (mathvm::Scope::VarIterator it(node->scope()); it.hasNext();) {
        addVar(node, it.next()->name());
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
    for (uint32_t i = 0; i < node->nodes(); ++i) {
        mathvm::CallNode* call = dynamic_cast<mathvm::CallNode*>(node->nodeAt(i));
        node->nodeAt(i)->visit(this);
        if (call && currentType != mathvm::VT_VOID) {
            code->add(mathvm::BC_POP);
        }
    }
    for (mathvm::Scope::VarIterator it(node->scope()); it.hasNext();) {
        delVar(it.next()->name());
    }
}

void Translator::visitFunctionNode(mathvm::FunctionNode* node) {
    mathvm::VarType resType = resultType;
    resultType = node->returnType();
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        switch (node->parameterType(i)) {
            case mathvm::VT_DOUBLE: code->add(mathvm::BC_STOREDVAR); break;
            case mathvm::VT_INT: code->add(mathvm::BC_STOREIVAR); break;
            case mathvm::VT_STRING: code->add(mathvm::BC_STORESVAR); break;
            default: throwError(node, "Internal error");
        }
        code->addTyped(addVar(node, node->parameterName(i)));
    }
    node->body()->visit(this);
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        delVar(node->parameterName(i));
    }
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
            default: throwError(node, "Internal error");
        }
        currentType = mathvm::VT_INVALID;
    }
    currentType = mathvm::VT_VOID;
}

void Translator::visitReturnNode(mathvm::ReturnNode* node) {
    if (node->returnExpr()) {
        if (resultType == mathvm::VT_VOID) {
            throwError(node, "Error: Returning a value");
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
            throwError(node, "Error: Returning no value from a non-void function");
        }
    }
    currentType = mathvm::VT_INVALID;
    code->add(mathvm::BC_RETURN);
}

void Translator::visitCallNode(mathvm::CallNode* node) {
    mathvm::TranslatedFunction* fun = prog->functionByName(node->name());
    if (!fun) {
        throwError(node, "Error: Undeclared function: %s", node->name().c_str());
    }
    if (fun->parametersNumber() != node->parametersNumber()) {
        throwError(node, "Error: Function %s expects %d arguments but got %d",
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
