#include "CodeVisitor.h"

CodeVisitor::CodeVisitor() {
    curBytecode = &code.getBytecode();
}

void CodeVisitor::visitBinaryOpNode(mathvm::BinaryOpNode* node) {    
    using namespace mathvm;
    node->left()->visit(this);
    node->right()->visit(this);
    //determine type
    NodeInfo& n = saveNodeInfo(node, mathvm::VT_INVALID);
    NodeInfo& nl = loadNodeInfo(node->left());
    NodeInfo& nr = loadNodeInfo(node->right());
    mathvm::VarType resType;
    
    genInstrBinNode(nl, nr, node->kind(), resType);
    n.type = resType;
}

void CodeVisitor::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
    using namespace mathvm;
    node->operand()->visit(this);
    NodeInfo& n = saveNodeInfo(node, mathvm::VT_INVALID);
    NodeInfo& nop = loadNodeInfo(node->operand());
    if (nop.type == mathvm::VT_INT && node->kind() == mathvm::tNOT) {
        n.type = mathvm::VT_INT;
        Label lblFalse(&cCode()); //adress where result of negation will be false
        Label lblTrue (&cCode()); //adress where result of negation will be true
        cCode().addByte(BC_ILOAD0);
        cCode().addBranch(BC_IFICMPNE, lblFalse);
        cCode().addByte(BC_ILOAD1);
        cCode().addBranch(BC_JA, lblTrue);
        cCode().bind(lblFalse);
        cCode().addByte(BC_ILOAD0);
        cCode().bind(lblTrue);
    } else if (nop.type == mathvm::VT_INT && node->kind() == mathvm::tSUB) {
        n.type = mathvm::VT_INT;
        cCode().addByte(BC_INEG);
    } else if (nop.type == mathvm::VT_DOUBLE && node->kind() == mathvm::tSUB) {
        n.type = mathvm::VT_DOUBLE;
        cCode().addByte(BC_DNEG);
    } else {
        transError("Unary operation: " + std::string(tokenOp(node->kind())));
    }    
}

void CodeVisitor::visitStringLiteralNode(mathvm::StringLiteralNode* node) {    
    using namespace mathvm;
    //store string literal in memory
    uint16_t newId = code.makeStringConstant(node->literal());
    cCode().addByte(BC_SLOAD);
    cCode().addUInt16(newId);
    saveNodeInfo(node, mathvm::VT_STRING, (size_t)newId, CodeVisitor::NT_SCONST);
}

void CodeVisitor::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
    using namespace mathvm;
    cCode().addByte(BC_DLOAD);
    cCode().addDouble(node->literal());
    saveNodeInfo(node, mathvm::VT_DOUBLE);
}

void CodeVisitor::visitIntLiteralNode(mathvm::IntLiteralNode* node) {
    using namespace mathvm;
    cCode().addByte(BC_ILOAD);
    cCode().addInt64(node->literal());
    saveNodeInfo(node, mathvm::VT_INT);
}

void CodeVisitor::visitLoadNode(mathvm::LoadNode* node) {
    using namespace mathvm;
    NodeInfo& var = loadNodeInfo(node->var());
    saveNodeInfo(node, var.type);
    switch(var.type) {
        case mathvm::VT_INT:
            cCode().addByte(BC_LOADIVAR);
            cCode().addByte((uint8_t)var.id);
            break;
        case mathvm::VT_DOUBLE:
            cCode().addByte(BC_LOADDVAR);
            cCode().addByte((uint8_t)var.id);
            break;
        case mathvm::VT_STRING:
            cCode().addByte(BC_LOADSVAR);
            cCode().addByte((uint8_t)var.id);
            break;
        default:
           transError("Loading variable" + node->var()->name() +
                      " which type is invalid");
    }
}

void CodeVisitor::visitStoreNode(mathvm::StoreNode* node) {
    using namespace mathvm;
    
    node->value()->visit(this);
    
    NodeInfo& val = loadNodeInfo(node->value());
    NodeInfo& var = loadNodeInfo(node->var());
    saveNodeInfo(node, var.type);

    if (val.type != var.type && 
        var.type != mathvm::VT_DOUBLE &&
        val.type != mathvm::VT_INT) {
        transError(std::string("store op: types are not equal and not a conversion") +
                   "from int to double. Var: " + node->var()->name());
    }
    //now all types a valid
    if (var.type != val.type) {
        cCode().addByte(BC_I2D);
    }

    //now var.type == val.type (on stack)
    switch(node->op()) {
        case mathvm::tASSIGN:
            if (var.type == mathvm::VT_INT) {
                cCode().addByte(BC_STOREIVAR); 
                cCode().addByte((uint8_t)var.id);
            }
            else if (var.type == mathvm::VT_DOUBLE) {
                cCode().addByte(BC_STOREDVAR); 
                cCode().addByte((uint8_t)var.id);
            }
            else if (var.type == mathvm::VT_STRING) {
                cCode().addByte(BC_STORESVAR);
                cCode().addByte((uint8_t)var.id);
            }
            else {
                transError("Assigning to invalid type");
            }
            break;
        case mathvm::tINCRSET:
            if (var.type == mathvm::VT_INT) {
                cCode().addByte(BC_LOADIVAR); 
                cCode().addByte((uint8_t)var.id);
                cCode().addByte(BC_IADD);
                cCode().addByte(BC_STOREIVAR);
                cCode().addByte((uint8_t)var.id);
            }
            else if (var.type == mathvm::VT_DOUBLE) {
                cCode().addByte(BC_LOADDVAR); 
                cCode().addByte((uint8_t)var.id);
                cCode().addByte(BC_DADD);
                cCode().addByte(BC_STOREDVAR);
                cCode().addByte((uint8_t)var.id);
            }
            else {
                transError("+= to invalid type");
            }
            break;
        case mathvm::tDECRSET:
             if (var.type == mathvm::VT_INT) {
                cCode().addByte(BC_LOADIVAR); 
                cCode().addByte((uint8_t)var.id);
                cCode().addByte(BC_SWAP);
                cCode().addByte(BC_ISUB);
                cCode().addByte(BC_STOREIVAR);
                cCode().addByte((uint8_t)var.id);
            }
            else if (var.type == mathvm::VT_DOUBLE) {
                cCode().addByte(BC_LOADDVAR); 
                cCode().addByte((uint8_t)var.id);
                cCode().addByte(BC_SWAP);
                cCode().addByte(BC_DSUB);
                cCode().addByte(BC_STOREDVAR);
                cCode().addByte((uint8_t)var.id);
            }
            else {
                transError("-= to invalid type");
            }
            break;
        default:
            transError();
    }
}

void CodeVisitor::visitForNode(mathvm::ForNode* node) {
    using namespace mathvm;
    
    Label lblLoopCheck(&cCode());
    Label lblEnd(&cCode());
    
    NodeInfo& nvar = loadNodeInfo(node->var());
    //TOS: min, max
    node->inExpr()->visit(this);
    cCode().addByte(BC_STOREIVAR);
    cCode().addByte((uint8_t)nvar.id);
    cCode().addByte(BC_POP);
    
    cCode().bind(lblLoopCheck);
    node->inExpr()->visit(this);
    //TOS: min. max
    cCode().addByte(BC_POP);
    cCode().addByte(BC_LOADIVAR);
    cCode().addByte((uint8_t)nvar.id);
    cCode().addBranch(BC_IFICMPG, lblEnd);  
    node->body()->visit(this);
    cCode().addByte(BC_LOADIVAR);
    cCode().addByte((uint8_t)nvar.id);
    cCode().addByte(BC_ILOAD1);
    cCode().addByte(BC_IADD);
    cCode().addByte(BC_STOREIVAR);
    cCode().addByte((uint8_t)nvar.id);
    cCode().addBranch(BC_JA, lblLoopCheck);
    cCode().bind(lblEnd);

}

void CodeVisitor::visitWhileNode(mathvm::WhileNode* node) {
    using namespace mathvm;

    Label lblWhile(&cCode());
    Label lblEnd(&cCode());
    cCode().bind(lblWhile);
    node->whileExpr()->visit(this);
    cCode().addByte(BC_ILOAD0);
    cCode().addBranch(BC_IFICMPE, lblEnd);
    node->loopBlock()->visit(this);
    cCode().addBranch(BC_JA, lblWhile);
    cCode().bind(lblEnd);
}

void CodeVisitor::visitIfNode(mathvm::IfNode* node) {
    using namespace mathvm;
    
    Label lblElse(&cCode());
    node->ifExpr()->visit(this);
    cCode().addByte(BC_ILOAD0);
    cCode().addBranch(BC_IFICMPE, lblElse);
    node->thenBlock()->visit(this);
    cCode().bind(lblElse);
    if (node->elseBlock()) {
        node->elseBlock()->visit(this);
    }
}

void CodeVisitor::visitBlockNode(mathvm::BlockNode* node) {
    using namespace mathvm;
    BlockNode *parentBlock = &cBlock();
    curBlock = node;
    mathvm::Scope::VarIterator it(node->scope());
    while(it.hasNext()) {
        mathvm::AstVar* curr = it.next();
        saveNodeInfo(curr, curr->type(), newVarId(), CodeVisitor::NT_VAR);
    }
    node->visitChildren(this);
    curBlock = parentBlock;
}

void CodeVisitor::visitCallNode(mathvm::CallNode* node) {
    using namespace mathvm;
    
    TranslatedFunction *f = code.functionByName(node->name());
    saveNodeInfo(node, f->returnType(), 0, NT_OTHER);
    
    for (uint32_t i = 0; i < node->parametersNumber(); i++) {
        node->parameterAt(i)->visit(this);
        NodeInfo& npar = loadNodeInfo(node->parameterAt(i));
        if (f->parameterType(i) != npar.type) {
            transError("Incompatible types of incoming parameters for " + f->name() + " function");
        }
    }
    cCode().addByte(BC_CALL);
    cCode().addUInt16(f->id());
}

void CodeVisitor::visitFunctionNode(mathvm::FunctionNode* node) {
    using namespace mathvm;

    Bytecode* parentBytecode = &cCode();
    AstFunction *astFunc = new AstFunction(node, cBlock().scope());
    BytecodeFunction *func = new BytecodeFunction(astFunc);
    code.addFunction(func);
    code.addFunctionId(func->id()); 
    curBytecode = func->bytecode();
    node->body()->visit(this);
    curBytecode = parentBytecode;
}

void CodeVisitor::visitReturnNode(mathvm::ReturnNode* node) {
    using namespace mathvm;
    
    node->returnExpr()->visit(this);
    saveNodeInfo(node, VT_VOID);
    cCode().addByte(BC_RETURN);
}


void CodeVisitor::visitPrintNode(mathvm::PrintNode* node) {
    using namespace mathvm;
    saveNodeInfo(node, mathvm::VT_INVALID);
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        NodeInfo& nop = loadNodeInfo(node->operandAt(i));
        switch (nop.type) {
            case VT_INT:
                cCode().addByte(BC_IPRINT);
                break;
            case VT_DOUBLE:
                cCode().addByte(BC_DPRINT);
                break;
            case VT_STRING:
                cCode().addByte(BC_SPRINT);
                break;
            default:
                transError("Print: unprintable type");
                break;
        }
    }
}

size_t CodeVisitor::newVarId() {
    static size_t counter = 1; //0 is reserved for non stored nodes
    if (counter > 255) {
        transError("Limit for number variables (255) is exceeded");
    }
    return counter++;
}

CodeVisitor::NodeInfo& CodeVisitor::saveNodeInfo(const void* node, mathvm::VarType type, 
                                                 size_t id, CodeVisitor::NodeType nodeType) {
    NodeInfo& n = nodeInfoMap[node];
    n.id = id;
    n.type = type;
    n.nodeType = nodeType;
    return n;
}

CodeVisitor::NodeInfo& CodeVisitor::loadNodeInfo(const void* node) {
    return nodeInfoMap[node];
}

void CodeVisitor::transError(std::string str) { 
    cCode().addByte(mathvm::BC_INVALID);
    cCode().dump();
    std::cout << "Error in translation (" << str << ")" << std::endl;   
    exit(-1); 
}

void CodeVisitor::genInstrBinNode(const CodeVisitor::NodeInfo &a, const CodeVisitor::NodeInfo &b, 
                                  mathvm::TokenKind op, mathvm::VarType& resType) {
    using namespace mathvm;
    
    //Type conversion on TOS from int to double
    //Type checking for op is performed lately
    if (a.type != b.type && a.type != VT_STRING && b.type != VT_STRING) {
        //Value of b node is on TOS
        if (b.type == mathvm::VT_INT) {
            cCode().addByte(BC_I2D);
        } else if (a.type == mathvm::VT_INT) {
            cCode().addByte(BC_SWAP);
            cCode().addByte(BC_I2D);
            cCode().addByte(BC_SWAP);
        } else {
            cCode().addByte(BC_INVALID);
            transError(std::string("type conversion in binary operation ") + tokenOp(op));
        }
    } 
    
    Label lblTrue(&cCode()); //result of op is true
    Label lblFalse(&cCode());//result of op is false

    if (a.type == VT_INT && b.type == VT_INT) {
        resType = VT_INT;
        switch (op) {
            case tADD:
                cCode().addByte(BC_IADD);
                break;
            case tSUB:
                cCode().addByte(BC_ISUB);
                break;
            case tMUL:
                cCode().addByte(BC_IMUL);
                break;
            case tDIV:
                cCode().addByte(BC_IDIV);
                break;
            case tOR:
                cCode().addByte(BC_ILOAD0);
                cCode().addBranch(BC_IFICMPNE, lblTrue);
                cCode().addByte(BC_ILOAD0);
                cCode().addBranch(BC_IFICMPNE, lblTrue);
                cCode().addByte(BC_ILOAD0);
                cCode().addBranch(BC_JA, lblFalse);
                cCode().bind(lblTrue);
                cCode().addByte(BC_ILOAD1);
                cCode().bind(lblFalse);
                break;
            case tAND:
                cCode().addByte(BC_ILOAD0);
                cCode().addBranch(BC_IFICMPE, lblFalse);
                cCode().addByte(BC_ILOAD0);
                cCode().addBranch(BC_IFICMPE, lblFalse);
                cCode().addByte(BC_ILOAD1);
                cCode().addBranch(BC_JA, lblTrue);
                cCode().bind(lblFalse);
                cCode().addByte(BC_ILOAD0);
                cCode().bind(lblTrue);
                break;
            case tEQ:
            case tNEQ:
            case tGT:
            case tLT:
            case tGE:
            case tLE:
                if (op == tEQ)
                    cCode().addBranch(BC_IFICMPE, lblTrue);
                if (op == tNEQ)
                    cCode().addBranch(BC_IFICMPNE, lblTrue);
                if (op == tGT)
                    cCode().addBranch(BC_IFICMPG, lblTrue);
                if (op == tLT)
                    cCode().addBranch(BC_IFICMPL, lblTrue);
                if (op == tGE)
                    cCode().addBranch(BC_IFICMPGE, lblTrue);
                if (op == tLE)
                    cCode().addBranch(BC_IFICMPLE, lblTrue);
                cCode().addByte(BC_ILOAD0);
                cCode().addBranch(BC_JA, lblFalse);
                cCode().bind(lblTrue);
                cCode().addByte(BC_ILOAD1);
                cCode().bind(lblFalse);
                break;
            case tRANGE:
                //change order from max,min to min,max
                cCode().addByte(BC_SWAP);
                break;
            default:
                transError(std::string("operation ") + tokenOp(op) + " on int and int is not permitted");
        }
    } else
    if ((a.type == VT_DOUBLE || a.type == VT_INT) &&
        (b.type == VT_DOUBLE || b.type == VT_INT)) {
        resType = VT_DOUBLE;                
        switch (op) {
            case tADD:
                cCode().addByte(BC_DADD);
                break;
            case tSUB:
                cCode().addByte(BC_DSUB);
                break;
            case tMUL:
                cCode().addByte(BC_DMUL);
                break;
            case tDIV:
                cCode().addByte(BC_DDIV);
                break;
            case tEQ:
            case tNEQ:
            case tGT:
            case tLT:
            case tGE:
            case tLE:
                cCode().addByte(BC_DCMP);
                cCode().addByte(BC_ILOAD0);
                if (op == tEQ)
                    cCode().addBranch(BC_IFICMPE, lblTrue);
                if (op == tNEQ)
                    cCode().addBranch(BC_IFICMPNE, lblTrue);
                if (op == tGT)
                    cCode().addBranch(BC_IFICMPG, lblTrue);
                if (op == tLT)
                    cCode().addBranch(BC_IFICMPL, lblTrue);
                if (op == tGE)
                    cCode().addBranch(BC_IFICMPGE, lblTrue);
                if (op == tLE)
                    cCode().addBranch(BC_IFICMPLE, lblTrue);
                
                cCode().addByte(BC_ILOAD0);
                cCode().addBranch(BC_JA, lblFalse);
                cCode().bind(lblTrue);
                cCode().addByte(BC_ILOAD1);
                cCode().bind(lblFalse); 
                break;
            default:
                transError(std::string("operation ") + tokenOp(op) + " on int and double is not permitted" );
        }
    } else transError("binary operation on unsupported (constant) types");
}

