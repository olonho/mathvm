#include "bc_translator.h"

#include <iostream>
#include <exception>
#include <dlfcn.h>

using std::cout;
using std::cin;
using std::exception;
using std::string;

namespace mathvm {


Status* BytecodeTranslator::translate(const string& program, Code** codePtr) {
    Parser parser;
    Status* status = parser.parseProgram(program);

    if (status && status->isError()) {
        cerr << "Error while parsing the program." << endl;
        return status;
    }

#if DEB
        mathvm::Printer printer(std::cout);
        printer.print(parser.top());
        INFO(std::endl);
        INFO(std::endl);
#endif
    DEBUG("TRANSLATION");
    
    _code = new InterpreterCodeImpl;
    *codePtr = _code;        
    try {
        functionDefinition(parser.top());

    } catch (const string& e) {
        return new Status(e);
    }            
    DEBUG(std::endl);    
    return new Status;
}


#define MK_INSN(left, type, right) \
        (type == VT_INT? left##I##right : \
        (type == VT_DOUBLE? left##D##right : \
        (type == VT_STRING? left##S##right : BC_INVALID)))

#define MK_INSN_DI(left, type, right) \
        (type == VT_INT? left##I##right : \
        (type == VT_DOUBLE? left##D##right : BC_INVALID))


void BytecodeTranslator::loading(const AstVar* var) {
    VarInfo varInfo = findVar(var);

    if (varInfo.ctx == currentContext()) {
        if (varInfo.id < 4) {
            addInsn((Instruction)
                    (MK_INSN(BC_LOAD, var->type(), VAR0) + varInfo.id));
        } else {
            addInsn(MK_INSN(BC_LOAD, var->type(), VAR));
            bc()->addUInt16(varInfo.id);
        }    
    } else {
        addInsn(MK_INSN(BC_LOADCTX, var->type(), VAR));
        bc()->addUInt16(varInfo.ctx);
        bc()->addUInt16(varInfo.id);
    }    
    _tosType = var->type();
}

void BytecodeTranslator::storing(const AstVar* var) {
    VarInfo varInfo = findVar(var);

    if (varInfo.ctx == currentContext()) {
        if (varInfo.id < 4) {
            addInsn((Instruction) 
                    (MK_INSN(BC_STORE, var->type(), VAR0) + varInfo.id));
        } else {
            addInsn(MK_INSN(BC_STORE, var->type(), VAR));
            bc()->addUInt16(varInfo.id);
        }
    } else {
        addInsn(MK_INSN(BC_STORECTX, var->type(), VAR));
        bc()->addUInt16(varInfo.ctx);
        bc()->addUInt16(varInfo.id);
    }
}

void checkLogicType(VarType type) {
    if (type != VT_INT) {
        throw string("Cannot use logic operations on ") + typeToName(type);
    }
}

void BytecodeTranslator::visitBinaryOpNode(BinaryOpNode* node) {    
    node->right()->visit(this);
    VarType rightType = _tosType;        
    node->left()->visit(this);
    VarType leftType = _tosType;  
        
    TokenKind op = node->kind();
    switch (op) {
        case tOR: case tAND: {
            checkLogicType(leftType);
            checkLogicType(rightType);
            logic(op);
            break;
        }
        case tEQ: case tNEQ: case tGT: case tGE: case tLT: case tLE: {
            VarType type = typeCastForComparison(leftType, rightType);
            comparison(type, op);
            break;
        }
        case tADD: case tSUB: case tMUL: case tDIV: case tMOD: {
            VarType type = typeCastForArithmetics(leftType, rightType);
            arithmetics(type, op);
            break;
        }     
        case tAOR: case tAAND: case tAXOR: {
            VarType type = typeCastForBitwise(leftType, rightType);
            bitwise(type, op);
            break;
        }
        default:
            throw string("Incorrect binary operation: ") + tokenOp(op);
            break;
    }
}

void BytecodeTranslator::logic(TokenKind op) {
    switch (op) {
        case tOR: addInsn(BC_IADD); break;
        case tAND: addInsn(BC_IMUL); break;
        default: assert(false); break;
    }
    _tosType = VT_INT;
}

VarType BytecodeTranslator::typeCastForComparison(VarType leftType,
                                                  VarType rightType) {  
    /* Accepts only ints, doubles get casted to ints
     */
    if (leftType == VT_DOUBLE) {
        addInsn(BC_D2I); // the left value is above the right value on stack
        leftType = VT_INT;
    }
    if (rightType == VT_DOUBLE) {
        addInsn(BC_SWAP);
        addInsn(BC_D2I);
        addInsn(BC_SWAP);
        rightType = VT_INT;
    }
    if (rightType != VT_INT || leftType != VT_INT) {
        throw string("Wrong types of arguments of comparison: ") +
              typeToName(leftType) + " and " + typeToName(rightType);
    }    
    return VT_INT;
}

void BytecodeTranslator::comparison(VarType type, TokenKind op) {
    assert(type == VT_INT);
    
    Label loadTrue(bc());
    Label keepFalse(bc());
    
    Instruction insn;    
    switch (op) {
        case tEQ: insn = BC_IFICMPE; break;
        case tNEQ: insn = BC_IFICMPNE; break;
        case tGT: insn = BC_IFICMPG; break;
        case tGE: insn = BC_IFICMPGE; break;
        case tLT: insn = BC_IFICMPL; break;
        case tLE: insn = BC_IFICMPLE; break;
        default: assert(false); break;
    }    
    addBranch(insn, loadTrue);
    addInsn(BC_ILOAD0);
    addBranch(BC_JA, keepFalse);
    bc()->bind(loadTrue);
    addInsn(BC_ILOAD1);
    bc()->bind(keepFalse);
    
    _tosType = type;
}

VarType BytecodeTranslator::typeCastForArithmetics(VarType leftType,
                                                   VarType rightType) {
    /* Accepts ints and doubles.
     * If types do not agree, int gets casted to double.
     */    
    if (leftType == VT_INT && rightType == VT_DOUBLE) {
        addInsn(BC_I2D); // the left value is above the right value on stack
        return VT_DOUBLE;
        
    } else if (leftType == VT_DOUBLE && rightType == VT_INT ) {
        addInsn(BC_SWAP);
        addInsn(BC_I2D);
        addInsn(BC_SWAP);
        return VT_DOUBLE;

    } else if (leftType == VT_INT && rightType == VT_INT) {
        return VT_INT;
        
    } else if (leftType == VT_DOUBLE && rightType == VT_DOUBLE) {
        return VT_DOUBLE;

    } else {
        throw string("Wrong types of arguments for an arithmetic operation: ") +
                     typeToName(leftType) + " and " + typeToName(rightType);
    }         
}

void BytecodeTranslator::arithmetics(VarType type, TokenKind op) {
    assert(type == VT_INT || type == VT_DOUBLE);
    
    switch (op) { 
        case tADD: addInsn(MK_INSN_DI(BC_, type, ADD)); break; 
        case tSUB: addInsn(MK_INSN_DI(BC_, type, SUB)); break; 
        case tMUL: addInsn(MK_INSN_DI(BC_, type, MUL)); break; 
        case tDIV: addInsn(MK_INSN_DI(BC_, type, DIV)); break; 
        case tMOD: assert(type == VT_INT); addInsn(BC_IMOD); break; 
        default: assert(false); break;
    } 
    _tosType = type;
}

VarType BytecodeTranslator::typeCastForBitwise(VarType leftType, VarType rightType) {
    if (leftType != VT_INT || rightType != VT_INT) {
        throw string("Bitwise operations require ints");
    }
    return VT_INT;
}

void BytecodeTranslator::bitwise(VarType type, TokenKind op) {
    assert(type == VT_INT);
    
    switch (op) {
        case tAOR: addInsn(BC_IAOR); break;
        case tAAND: addInsn(BC_IAAND); break;
        case tAXOR: addInsn(BC_IAXOR); break;
        default: assert(false); break;
    }
}

void BytecodeTranslator::visitUnaryOpNode(UnaryOpNode* node) {
    node->operand()->visit(this);    
    
    switch (node->kind()) {
        case tSUB: 
            if (_tosType != VT_INT && _tosType != VT_DOUBLE) {
                throw string("Cannot negate ") + typeToName(_tosType);
            }        
            addInsn(MK_INSN_DI(BC_, _tosType, NEG)); 
            break;
        
        case tNOT: {
            checkLogicType(_tosType);
            
            Label put1(bc());
            Label end(bc());
            addInsn(BC_ILOAD0);
            addBranch(BC_IFICMPE, put1);
            addInsn(BC_ILOAD0);
            addBranch(BC_JA, end);
            bc()->bind(put1);
            addInsn(BC_ILOAD1);
            bc()->bind(end);
            break;
        }
        default: assert(false); break; 
    }
}

void BytecodeTranslator::visitStringLiteralNode(StringLiteralNode* node) {
    addInsn(BC_SLOAD);
    bc()->addUInt16(_code->makeStringConstant(node->literal()));
    _tosType = VT_STRING;
}

void BytecodeTranslator::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    addInsn(BC_DLOAD);
    bc()->addDouble(node->literal()); 
    _tosType = VT_DOUBLE;
}

void BytecodeTranslator::visitIntLiteralNode(IntLiteralNode* node) {
    addInsn(BC_ILOAD);
    bc()->addInt64(node->literal()); 
    _tosType = VT_INT;
}

void BytecodeTranslator::visitLoadNode(LoadNode* node) {
    loading(node->var());
}

void BytecodeTranslator::visitStoreNode(StoreNode* node) {
    node->value()->visit(this);
    
    if (!(node->op() == tASSIGN)) {
        loading(node->var());
        switch (node->op()) {
            case tINCRSET: 
                addInsn(MK_INSN_DI(BC_, _tosType, ADD)); 
                break;
            case tDECRSET: 
                addInsn(MK_INSN_DI(BC_, _tosType, SUB)); 
                break;
            default: 
                assert(false);
                break;
        }
    }
    storing(node->var());
}

void BytecodeTranslator::visitForNode(ForNode* node) {
    assert(node->inExpr()->isBinaryOpNode());
    BinaryOpNode* inExpr = node->inExpr()->asBinaryOpNode();
    assert(inExpr->kind() == tRANGE);
    
    const AstVar* var = node->var();
    assert(var->type() == VT_INT);
    
    inExpr->left()->visit(this);
    assert(_tosType == VT_INT);
    storing(var);
    
    Label beginning(bc());
    Label end(bc());
    
    bc()->bind(beginning);
    inExpr->right()->visit(this);
    assert(_tosType == VT_INT);
    loading(var);
    addBranch(BC_IFICMPG, end);

    node->body()->visit(this);
    
    loading(var);
    addInsn(BC_ILOAD1);
    addInsn(BC_IADD);
    storing(var);
    
    addBranch(BC_JA, beginning);
    bc()->bind(end);
}

void BytecodeTranslator::visitWhileNode(WhileNode* node) {
    Label beginning(bc());
    Label end(bc());
    
    bc()->bind(beginning);
    node->whileExpr()->visit(this);
    addInsn(BC_ILOAD0);
    addBranch(BC_IFICMPE, end);    
    node->loopBlock()->visit(this);
    addBranch(BC_JA, beginning);   
    bc()->bind(end);
}

void BytecodeTranslator::visitIfNode(IfNode* node) {
    Label else_(bc());
    Label end(bc());
    
    node->ifExpr()->visit(this);
    addInsn(BC_ILOAD0);
    addBranch(BC_IFICMPE, else_);
    
    node->thenBlock()->visit(this);
    addBranch(BC_JA, end);    
    
    bc()->bind(else_);
    if (node->elseBlock()) 
        node->elseBlock()->visit(this);    
    
    bc()->bind(end);
}

void BytecodeTranslator::visitBlockNode(BlockNode* node) {
    addCurrentLocals(node->scope()->variablesCount());
    
    Scope::VarIterator vars(node->scope());
    while (vars.hasNext()) {        
        varDeclaration(vars.next());
    };

    Scope::FunctionIterator funs(node->scope());
    while (funs.hasNext()) {
        functionDefinition(funs.next());
    }
    
    for (uint32_t i = 0; i < node->nodes(); i++) { 
        /* Tail recursion if this situation:
         * i:   CALL recursively
         * i+1: RETURN
         */    
        if (checkTailRecCall(node->nodeAt(i)) &&
        // Is not the last node:
            i < node->nodes() - 1 &&
        // The last node is return: 
            node->nodeAt(i + 1)->isReturnNode() &&
        // And its return expression is empty:
            !node->nodeAt(i + 1)->asReturnNode()->returnExpr()) {
            
            tailRecursion(node->nodeAt(i)->asCallNode());
            i++;
            
        } else {
            node->nodeAt(i)->visit(this);
        }
    }
}

bool BytecodeTranslator::checkTailRecCall(AstNode* node) {
    if (node->isCallNode()) {
        CallNode* callNode = node->asCallNode();
        TranslatedFunction* callFunc = _code->functionByName(callNode->name());
        return callFunc->id() == currentFunction()->id();
    }        
    return false;
}

void BytecodeTranslator::tailRecursion(CallNode* node) {
    DEBUG("Tail recursion");
    processCallParameters(node, _code->functionByName(node->name()));
    Label funcBeginning(bc(), 0);
    addBranch(BC_JA, funcBeginning);
}

void BytecodeTranslator::functionDefinition(AstFunction* astFunc) {
    BytecodeFunction* func = new BytecodeFunction(astFunc);
    _code->addFunction(func);    
    pushContext(func);

#if DEB
    static int level = 0;
    DEBUG(string(level * 4, ' ') << "Defining function " << func->id());
    level++;
#endif
    
    Scope::VarIterator args(astFunc->scope());
    while (args.hasNext()) {
        AstVar* var = args.next();
        varDeclaration(var);        
        storing(var);
    }
    
    astFunc->node()->visit(this);
    
    if (currentlyTopFunc()) {
        addInsn(BC_STOP);
    }

#if DEB
    level--;    
    if (DEB) {
        for (size_t bci = 0; bci < bc()->length();) {
            printInsn(bc(), bci, cerr, level * 4);
            bci += insnLength(bc()->getInsn(bci));
        }
    }
#endif
        
    popContext(); 
}

// Called from the processAstFunction::processAstFunction method
void BytecodeTranslator::visitFunctionNode(FunctionNode* node) {
    node->body()->visit(this);
}

void BytecodeTranslator::conversion(VarType foundType, VarType expectedType) {
    if (foundType == VT_INT && expectedType == VT_DOUBLE) {
        addInsn(BC_I2D);
    } else {
        throw string("Incorrect type cast: ") +
              typeToName(foundType) + " found, " +
              typeToName(expectedType) + " expected.";
    }    
}

void BytecodeTranslator::processCallParameters(CallNode* node,
                                               TranslatedFunction* func) {
    for (int i = node->parametersNumber() - 1; i >= 0; i--) {
        node->parameterAt(i)->visit(this);
        
        if (_tosType != func->parameterType(i)) {
            conversion(_tosType, func->parameterType(i));
        }
    }
}

void BytecodeTranslator::visitReturnNode(ReturnNode* node) {
    AstNode* retExpr = node->returnExpr();
    if (retExpr) {
        if (checkTailRecCall(retExpr)) {
            tailRecursion(retExpr->asCallNode());
        } else {
            retExpr->visit(this);
        }        
        if (_tosType != currentFunction()->returnType()) {
            conversion(_tosType, currentFunction()->returnType());
        }
    }   
    addInsn(BC_RETURN);
}

void BytecodeTranslator::visitCallNode(CallNode* node) {
    TranslatedFunction* func = _code->functionByName(node->name());
    if (!func) {
        throw string("Function " + node->name() + " is not defined");
    }
    
    processCallParameters(node, func);
    
    addInsn(BC_CALL);
    bc()->addUInt16(func->id());
    _tosType = func->returnType();
}

void BytecodeTranslator::visitNativeCallNode(NativeCallNode* node) {
    void* initializer = dlsym(0, node->nativeName().c_str());
    if (initializer == 0) { 
        throw string("Native function ") + node->nativeName() + " not found.";
    }  
    uint16_t nativeFunId = _code->makeNativeFunction(
            node->nativeName(), node->nativeSignature(), initializer);
    
    addInsn(BC_CALLNATIVE);
    bc()->addUInt16(nativeFunId);
    _tosType = node->nativeSignature()[0].first;
}

void BytecodeTranslator::visitPrintNode(PrintNode* node) {    
    for (uint32_t i = 0; i < node->operands(); i++) {
        node->operandAt(i)->visit(this);
        addInsn(MK_INSN(BC_, _tosType, PRINT));
    }
}

}
