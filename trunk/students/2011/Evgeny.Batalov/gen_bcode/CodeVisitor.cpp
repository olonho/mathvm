#include "CodeVisitor.h"

CodeVisitor::CodeVisitor(mathvm::BlockNode* top) {
    using namespace mathvm;
    //it is better to make this then trying to avoid asserts
    //and write stange code
    code.addFunction(new TranslatedFunction(new AstFunction(0, 0)));

    std::vector<std::pair<VarType, std::string> > signature;
    signature.push_back(std::pair<VarType, std::string>(VT_VOID, "return"));

    topFuncNode = new FunctionNode(0, AstFunction::top_name, signature, top);
    //AstFunction* astFunc = new AstFunction(0, top->scope());
    //BytecodeFunction *bcFunction = new BytecodeFunction(astFunc);
    //uint16_t id = code.addFunction(bcFunction);
    //code.addFunctionId(id);
    //curBytecode = bcFunction->bytecode();
    curBlock = top;
}

void CodeVisitor::translate() { 
    using namespace mathvm;
    topFuncNode->visit(this);

    if (code.funcCount() > 0) {
        BytecodeFunction* topFunc = 
            dynamic_cast<BytecodeFunction*>(code.functionById(code.funcIdByIndex(0)));
        topFunc->bytecode()->addByte(mathvm::BC_STOP);
    }
}

void CodeVisitor::visitBinaryOpNode(mathvm::BinaryOpNode* node) {    
    using namespace mathvm;
    VarType resType;
    
    if (node->kind() == tOR || node->kind() == tAND) {
        Label lazyLabel(&cCode());
        node->left()->visit(this);
        putLazyLogic(node->kind(), lazyLabel);
        node->right()->visit(this);
        cCode().bind(lazyLabel);
    } else {
        //TOS: left, right
        node->right()->visit(this);
        node->left()->visit(this);
    }
    
    NodeInfo& nl = getNodeInfo(node->left());
    NodeInfo& nr = getNodeInfo(node->right());
    
    procBinNode(nl, nr, node->kind(), resType);
    setNodeInfo(node, resType);
}

void CodeVisitor::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
    using namespace mathvm;
    VarType resType;
    node->operand()->visit(this);

    NodeInfo& nop = getNodeInfo(node->operand());
    if (nop.type == VT_INT && node->kind() == tNOT) {
        resType = VT_INT;        
        Label lblFalse(&cCode()); //addres where result of negation will be false
        Label lblTrue (&cCode()); //addres where result of negation will be true
        cCode().addByte(BC_ILOAD0);
        cCode().addBranch(BC_IFICMPNE, lblFalse);
        cCode().addByte(BC_ILOAD1);
        cCode().addBranch(BC_JA, lblTrue);
        cCode().bind(lblFalse);
        cCode().addByte(BC_ILOAD0);
        cCode().bind(lblTrue);
    } else if (nop.type == VT_INT && node->kind() == tSUB) {
        resType = VT_INT;
        cCode().addByte(BC_INEG);
    } else if (nop.type == VT_DOUBLE && node->kind() == tSUB) {
        resType = VT_DOUBLE;
        cCode().addByte(BC_DNEG);
    } else {
        transError("Unary operation: " + std::string(tokenOp(node->kind())));
    }    
    setNodeInfo(node, resType);
}

void CodeVisitor::visitStringLiteralNode(mathvm::StringLiteralNode* node) {    
    using namespace mathvm;
    //store string literal in memory
    std::string s = node->literal();
    size_t pos = 0;
    while(std::string::npos != (pos = s.find('\n', pos))) {
        s.replace(pos, 1,  "\\n");
    }
        
    uint16_t newId = code.makeStringConstant(s);
    cCode().addByte(BC_SLOAD);
    cCode().addUInt16(newId);
    setNodeInfo(node, VT_STRING);
}

void CodeVisitor::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
    using namespace mathvm;
    cCode().addByte(BC_DLOAD);
    cCode().addDouble(node->literal());
    setNodeInfo(node, VT_DOUBLE);
}

void CodeVisitor::visitIntLiteralNode(mathvm::IntLiteralNode* node) {
    using namespace mathvm;
    cCode().addByte(BC_ILOAD);
    cCode().addInt64(node->literal());
    setNodeInfo(node, VT_INT);
}

void CodeVisitor::visitLoadNode(mathvm::LoadNode* node) {
    using namespace mathvm;    
    VarInfo& var = getVarInfo(node->var()->name());
    setNodeInfo(node, var.type);

    switch(var.type) {
        case VT_INT:
            cCode().addByte(BC_LOADIVAR);
            cCode().addByte((uint8_t)var.id);
            break;
        case VT_DOUBLE:
            cCode().addByte(BC_LOADDVAR);
            cCode().addByte((uint8_t)var.id);
            break;
        case VT_STRING:
            cCode().addByte(BC_LOADSVAR);
            cCode().addByte((uint8_t)var.id);
            break;
        default:
           transError("Loading variable " + node->var()->name() + " which type is invalid");
    }
}

void CodeVisitor::visitStoreNode(mathvm::StoreNode* node) {
    using namespace mathvm;    
    setNodeInfo(node, VT_VOID);

    node->value()->visit(this);    
    NodeInfo& val = getNodeInfo(node->value());
    VarInfo& var = getVarInfo(node->var()->name());

    if (val.type != var.type && 
        var.type != VT_DOUBLE &&
        val.type != VT_INT) {
        transError(std::string("store op: types are not equal and not a conversion") +
                   "from int to double. Var: " + node->var()->name());
    }
    //now all types a valid
    if (var.type != val.type) {
        cCode().addByte(BC_I2D);
    }

    //now var.type == val.type (on stack)
    switch(node->op()) {
        case tASSIGN:
            if (var.type == VT_INT) {
                cCode().addByte(BC_STOREIVAR); 
                cCode().addByte((uint8_t)var.id);
            }
            else if (var.type == VT_DOUBLE) {
                cCode().addByte(BC_STOREDVAR); 
                cCode().addByte((uint8_t)var.id);
            }
            else if (var.type == VT_STRING) {
                cCode().addByte(BC_STORESVAR);
                cCode().addByte((uint8_t)var.id);
            }
            else {
                transError("Assigning to invalid type");
            }
            break;
        case tINCRSET:
            if (var.type == VT_INT) {
                cCode().addByte(BC_LOADIVAR); 
                cCode().addByte((uint8_t)var.id);
                cCode().addByte(BC_IADD);
                cCode().addByte(BC_STOREIVAR);
                cCode().addByte((uint8_t)var.id);
            }
            else if (var.type == VT_DOUBLE) {
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
        case tDECRSET:
             if (var.type == VT_INT) {
                cCode().addByte(BC_LOADIVAR); 
                cCode().addByte((uint8_t)var.id);
                cCode().addByte(BC_ISUB);
                cCode().addByte(BC_STOREIVAR);
                cCode().addByte((uint8_t)var.id);
            }
            else if (var.type == VT_DOUBLE) {
                cCode().addByte(BC_LOADDVAR); 
                cCode().addByte((uint8_t)var.id);
                cCode().addByte(BC_DSUB);
                cCode().addByte(BC_STOREDVAR);
                cCode().addByte((uint8_t)var.id);
            }
            else {
                transError("-= to invalid type");
            }
            break;
        default:
            transError("only =, +=, -= are permitted");
    }
}

void CodeVisitor::visitForNode(mathvm::ForNode* node) {
    using namespace mathvm;    
    BinaryOpNode* op = dynamic_cast<BinaryOpNode*>(node->inExpr());
    if (!op)
        transError("for node needs binary operation to evaluate");
    if (op->kind() != tRANGE)
        transError("for node needs range ... operation to evaluate");

    setNodeInfo(node, VT_VOID);
    
    Label lblLoopCheck(&cCode());
    Label lblEnd(&cCode());
    
    VarInfo& var = getVarInfo(node->var()->name());
    //TOS: min, max
    node->inExpr()->visit(this);
    cCode().addByte(BC_STOREIVAR);
    cCode().addByte((uint8_t)var.id);
    cCode().addByte(BC_POP);
    
    cCode().bind(lblLoopCheck);
    node->inExpr()->visit(this);
    //TOS: min. max
    cCode().addByte(BC_POP);
    cCode().addByte(BC_LOADIVAR);
    cCode().addByte((uint8_t)var.id);
    cCode().addBranch(BC_IFICMPG, lblEnd);  
    node->body()->visit(this);
    cCode().addByte(BC_LOADIVAR);
    cCode().addByte((uint8_t)var.id);
    cCode().addByte(BC_ILOAD1);
    cCode().addByte(BC_IADD);
    cCode().addByte(BC_STOREIVAR);
    cCode().addByte((uint8_t)var.id);
    cCode().addBranch(BC_JA, lblLoopCheck);
    cCode().bind(lblEnd);
}

void CodeVisitor::visitWhileNode(mathvm::WhileNode* node) {
    using namespace mathvm;
    setNodeInfo(node, VT_VOID);

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
    BlockNode* parentBlock = &cBlock();
    curBlock = node;
    mathvm::Scope::VarIterator it(node->scope());

    while(it.hasNext()) {
        mathvm::AstVar* curr = it.next();
        setVarInfo(curr->name(), newVarId(), curr->type());
    }    
    mathvm::Scope::FunctionIterator fit(node->scope());
    while(fit.hasNext()) {
        fit.next()->node()->visit(this);
    }

    node->visitChildren(this);
    curBlock = parentBlock;
}

void CodeVisitor::visitCallNode(mathvm::CallNode* node) {
    using namespace mathvm;
    checkFunction(node->name());
    TranslatedFunction *f = code.functionByName(node->name());
    setNodeInfo(node, f->returnType());
    
    for (uint32_t i = 0; i < node->parametersNumber(); i++) {
        
        node->parameterAt(i)->visit(this);
        NodeInfo& nparValue = getNodeInfo(node->parameterAt(i));
        
        if (f->parameterType(i) != nparValue.type) {
            transError("Incompatible types of incoming parameters for " + f->name() + " function");
        }

        switch(f->parameterType(i)) {
            case VT_INT:
                cCode().addByte(BC_STOREIVAR);
                break;
            case VT_DOUBLE:
                cCode().addByte(BC_STOREDVAR);
                break;
            case VT_STRING:
                cCode().addByte(BC_STORESVAR);
                break;
            default:
                transError("passing parameter with uknown type to function " + f->name());
        }
        cCode().addByte(getParamInfo(node->name(), i).id);
    }
    cCode().addByte(BC_CALL);
    cCode().addUInt16(f->id());
}

void CodeVisitor::visitFunctionNode(mathvm::FunctionNode* node) {
    using namespace mathvm;
    
    AstFunction *astFunc = new AstFunction(node, cBlock().scope());
    BytecodeFunction *func = new BytecodeFunction(astFunc);
    code.addFunction(func);
    code.addFunctionId(func->id());
    funcParams[astFunc->name()]; //add func to params

    for(uint32_t i = 0; i < func->parametersNumber(); ++i) {
        ParamInfo par;
        par.id = newVarId();
        par.type = func->parameterType(i);
        par.index = i;
        par.name = astFunc->parameterName(i);
        setParamInfo(astFunc->name(), par);
    }

    Bytecode* parentBytecode = &cCode();
    curBytecode = func->bytecode();
    pushFuncParams(astFunc->name());

    node->body()->visit(this);
    
    popFuncParams(astFunc->name());
    curBytecode = parentBytecode;
}

void CodeVisitor::visitReturnNode(mathvm::ReturnNode* node) {
    using namespace mathvm;
    setNodeInfo(node, VT_VOID);

    node->returnExpr()->visit(this);
    cCode().addByte(BC_RETURN);
}

void CodeVisitor::visitPrintNode(mathvm::PrintNode* node) {
    using namespace mathvm;
    setNodeInfo(node, VT_VOID);

    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        NodeInfo& nop = getNodeInfo(node->operandAt(i));
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
    static size_t counter = 1; //0 is reserved for non var and str const nodes
    if (counter > 255) {
        transError("Limit for number variables (255) is exceeded");
    }
    return counter++;
}

void CodeVisitor::setNodeInfo(mathvm::AstNode* node, mathvm::VarType type) {
    NodeInfo& n = nodeInfo[node];
    n.type = type;
}

CodeVisitor::NodeInfo& CodeVisitor::getNodeInfo(mathvm::AstNode* node) {
    NodeInfoMap::iterator it = nodeInfo.find(node);
    if (it != nodeInfo.end()) {
        return it->second;
    }
    transError("NodeInfo for AstNode not found");
    return *(NodeInfo*)0;
}

void CodeVisitor::setVarInfo(std::string name, size_t id, mathvm::VarType type) {
    VarInfo var;
    var.name = name;
    var.id = id;
    var.type = type;

    VarDefs& vd = varInfo[var.name];
    VarDefs::iterator it = vd.begin();
    for(;it != vd.end(); ++it) {
        if (it->id == id) {
            *it = var;
            return;
        }
    }
    vd.push_back(var);
}

CodeVisitor::VarInfo& CodeVisitor::getVarInfo(std::string name) {
    VarInfoMap::iterator it = varInfo.find(name);
    if (it != varInfo.end()) {
        return it->second.back();
    }
    transError("variable " + name  +  " is undeclared");
    return *(VarInfo*)0;
}


void CodeVisitor::checkFunction(std::string fName) {
    FuncParamsMap::const_iterator f = funcParams.find(fName);
    if (f == funcParams.end())
        transError("function " + fName + " is undefined");
}

void CodeVisitor::pushFuncParams(const std::string& fName) {
    checkFunction(fName);
    FuncParams& fParams = funcParams[fName];
    FuncParams::iterator it = fParams.begin();
    for(; it != fParams.end(); ++it) {
        VarInfo v;
        v.name = it->second.name;
        v.id = it->second.id;
        v.type = it->second.type;
        varInfo[v.name].push_back(v);
    }
}
void CodeVisitor::popFuncParams(const std::string& fName) {
    checkFunction(fName);
    FuncParams& fParams = funcParams[fName];
    FuncParams::iterator it = fParams.begin();
    for(; it != fParams.end(); ++it) {
        varInfo[it->second.name].pop_back();
    }
}


CodeVisitor::ParamInfo& CodeVisitor::getParamInfo(const std::string& fName, size_t index) {
    checkFunction(fName);
    FuncParams& f = funcParams[fName];
   
    for(FuncParams::iterator it = f.begin(); it != f.end(); ++it) {
        if (it->second.index == index) {
            return it->second;
        }
    }
    
    transError("parameter for function " + fName  +  " not found");
    return *(ParamInfo*)0;
}

CodeVisitor::ParamInfo& CodeVisitor::getParamInfo(const std::string& fName, const std::string& pName) {
    checkFunction(fName);
    FuncParams& f = funcParams[fName];
    FuncParams::iterator p = f.find(pName);
    if (p != f.end()) {
        return p->second;
    }

    transError("parameter " + pName  +  " for function " + fName  +  " not found");
    return *(ParamInfo*)0;
}

void CodeVisitor::setParamInfo(const std::string& fName, CodeVisitor::ParamInfo& info) {
    checkFunction(fName);
    FuncParams& f = funcParams[fName];
    f[info.name] = info;
}

void CodeVisitor::transError(std::string str) { 
    cCode().addByte(mathvm::BC_INVALID);
    cCode().dump();
    throw TranslationException("Error during translation: " + str + "\n");
    //std::cerr << "Error during translation (" << str << ")" << std::endl;   
    //exit(-1); 
}

void CodeVisitor::putLazyLogic(mathvm::TokenKind op, mathvm::Label& lbl) {
    using namespace mathvm;
    
    Label lbl1(&cCode());
    //type checking will be done by procBinNode
    if (op == tAND) {
        cCode().addByte(BC_ILOAD0);
        cCode().addBranch(BC_IFICMPNE, lbl1);
        cCode().addByte(BC_ILOAD0);
        cCode().addBranch(BC_JA, lbl);
        cCode().bind(lbl1);
        
    } else if (op == tOR) {
        cCode().addByte(BC_ILOAD0);
        cCode().addBranch(BC_IFICMPE, lbl1);
        cCode().addByte(BC_ILOAD1);
        cCode().addBranch(BC_JA, lbl);
        cCode().bind(lbl1);
    }
}

void  CodeVisitor::procBinNode(const CodeVisitor::NodeInfo &a, const CodeVisitor::NodeInfo &b, 
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
                //lazy logic has checked first operand and it was false
                cCode().addByte(BC_ILOAD0);
                cCode().addBranch(BC_IFICMPNE, lblTrue);
                cCode().addByte(BC_ILOAD0);
                cCode().addBranch(BC_JA, lblFalse);
                cCode().bind(lblTrue);
                cCode().addByte(BC_ILOAD1);
                cCode().bind(lblFalse);
                break;
            case tAND:
                //lazy logic has checked first operand and it was true
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
                else if (op == tNEQ)
                    cCode().addBranch(BC_IFICMPNE, lblTrue);
                else if (op == tGT)
                    cCode().addBranch(BC_IFICMPG, lblTrue);
                else if (op == tLT)
                    cCode().addBranch(BC_IFICMPL, lblTrue);
                else if (op == tGE)
                    cCode().addBranch(BC_IFICMPGE, lblTrue);
                else if (op == tLE)
                    cCode().addBranch(BC_IFICMPLE, lblTrue);
                cCode().addByte(BC_ILOAD0);
                cCode().addBranch(BC_JA, lblFalse);
                cCode().bind(lblTrue);
                cCode().addByte(BC_ILOAD1);
                cCode().bind(lblFalse);
                break;
            case tRANGE:
                //left on TOS and then right yet
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
                //pseudocode says that DCMP returns -1 if left > right and 1 if left < right
                cCode().addByte(BC_DCMP);
                cCode().addByte(BC_ILOAD0);
                if (op == tEQ)
                    cCode().addBranch(BC_IFICMPE, lblTrue);
                else if (op == tNEQ)
                    cCode().addBranch(BC_IFICMPNE, lblTrue);
                else if (op == tGT)
                    cCode().addBranch(BC_IFICMPL, lblTrue);
                else if (op == tLT)
                    cCode().addBranch(BC_IFICMPG, lblTrue);
                else if (op == tGE)
                    cCode().addBranch(BC_IFICMPLE, lblTrue);
                else if (op == tLE)
                    cCode().addBranch(BC_IFICMPGE, lblTrue);
                
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

