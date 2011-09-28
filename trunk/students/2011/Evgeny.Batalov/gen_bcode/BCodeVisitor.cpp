#include <iostream>
#include <sstream>

#include "BCodeVisitor.h"

BCodeVisitor::BCodeVisitor(std::ostream& o)
    : stream(o)
    , id_counter(1) //0 is reserved
    {}

//FIXME:replace generating of INVLID instruction with compilation error message

void BCodeVisitor::visitBinaryOpNode(mathvm::BinaryOpNode* node) {    
    node->left()->visit(this);
    node->right()->visit(this);
    //determine type
    NodeInfo& n = saveInfo(node, newId(), mathvm::VT_INVALID);
    NodeInfo& nl = getInfo(node->left());
    NodeInfo& nr = getInfo(node->right());
    mathvm::VarType resType;
    //mathvm::Instruction instruction;
    std::string instruction;
    if (genInstrBinNode(nl, nr, node->kind(), resType, instruction)) {
        stream << "ERROR in typing" << std::endl;
        stream << nl.type << " " << tokenOp(node->kind()) << " " << nr.type << std::endl;
        stream << "Exiting..." << std::endl;
        return;
    }
    
    //if types and op are ok, we can change type only to double
    //(if needed)
    if (resType != nl.type || resType != nr.type) {
        //Generate type conversions to double
        //Value of right node is on TOS
        if (nr.type == mathvm::VT_INT)
            stream << "I2D" << std::endl;
        if (nr.type == mathvm::VT_INT) {
            stream << "SWAP" << std::endl;
            stream << "I2D"  << std::endl;
            stream << "SWAP" << std::endl; 
        }
    }
    n.type = resType;
    stream << instruction << std::endl;
}

void BCodeVisitor::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
    node->operand()->visit(this);
    NodeInfo& n = saveInfo(node, newId(), mathvm::VT_INVALID);
    NodeInfo& nop = getInfo(node->operand());
    if (nop.type == mathvm::VT_INT && node->kind() == mathvm::tNOT) {
        n.type = mathvm::VT_INT;
        stream << "ILOAD0"  << std::endl;        
        stream << "IFICMPNE +3"  << std::endl;        
        stream << "ILOAD1"  << std::endl;//!FALSE 
        stream << "JA +2"  << std::endl;        
        stream << "ILOAD0"  << std::endl;//!TRUE
    } else if (nop.type == mathvm::VT_INT && node->kind() == mathvm::tSUB) {
        n.type = mathvm::VT_INT;
        stream << "INEG" << std::endl;
    } else {
        stream << "ERROR in typing" << std::endl;
        stream << nop.type << " " << tokenOp(node->kind()) << std::endl;
        stream << "Exiting..." << std::endl;
        return;
    }    
}

void BCodeVisitor::visitStringLiteralNode(mathvm::StringLiteralNode* node) {    
    stream << "SLOAD (int64_t)my_strdup('" << node->literal() << "')" << std::endl;
    saveInfo(node, 0, mathvm::VT_STRING);
}

void BCodeVisitor::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
    std::stringstream str;
    str << node->literal();
    std::string s = str.str();
    stream << "DLOAD " << s << std::endl;
    saveInfo(node, 0, mathvm::VT_DOUBLE);
}

void BCodeVisitor::visitIntLiteralNode(mathvm::IntLiteralNode* node) {
    stream << "ILOAD " << node->literal()  << std::endl;
    saveInfo(node, 0, mathvm::VT_INT);
}

void BCodeVisitor::visitLoadNode(mathvm::LoadNode* node) {
    NodeInfo& var = getInfo(node->var());
    saveInfo(node, 0, var.type);
    switch(var.type) {
        case mathvm::VT_INT:
            stream << "LOADIVAR " << var.id << std::endl;
            break;
        case mathvm::VT_DOUBLE:
            stream << "LOADDVAR " << var.id << std::endl;
            break;
        case mathvm::VT_STRING:
            stream << "LOADSVAR " << var.id << std::endl;
            break;
        default:
            stream << "INVALID"  << std::endl;
            return;
    }
}
void BCodeVisitor::visitStoreNode(mathvm::StoreNode* node) {
    node->value()->visit(this);
    
    NodeInfo& val = getInfo(node->value());
    NodeInfo& var = getInfo(node->var());
    saveInfo(node, 0, var.type);

    if (val.type != var.type && 
        var.type != mathvm::VT_DOUBLE &&
        val.type != mathvm::VT_INT) {
        stream << "INVALID" << std::endl;
        return;
    }
    //type conversion on stack from int to double
    if (var.type != val.type)
        stream << "I2D"  << std::endl;

    //var.type == val.type (on stack)
    switch(node->op()) {
        case mathvm::tASSIGN:
            if (var.type == mathvm::VT_INT)
                stream << "STOREIVAR " << var.id  << std::endl;
            else if (var.type == mathvm::VT_DOUBLE)
                stream << "STOREDVAR " << var.id << std::endl;
            else if (var.type == mathvm::VT_STRING)
                stream << "STORESVAR " << var.id << std::endl;
            else
                stream << "INVALID" << std::endl;
            break;
        case mathvm::tINCRSET:
            if (var.type == mathvm::VT_INT) {
                stream << "ILOAD1" << std::endl;
                stream << "IADD" << std::endl;
                stream << "STOREIVAR " << var.id  << std::endl;
            }
            else if (var.type == mathvm::VT_DOUBLE) {
                stream << "DLOAD1" << std::endl;
                stream << "DADD" << std::endl;
                stream << "STOREDVAR " << var.id << std::endl;
            }
            else
                stream << "INVALID" << std::endl;
            break;
        case mathvm::tDECRSET:
             if (var.type == mathvm::VT_INT) {
                stream << "ILOAD1" << std::endl;
                stream << "SWAP" << std::endl;
                stream << "ISUB" << std::endl;
                stream << "STOREIVAR " << var.id  << std::endl;
            }
            else if (var.type == mathvm::VT_DOUBLE) {
                stream << "DLOAD1" << std::endl;
                stream << "SWAP" << std::endl;
                stream << "DSUB" << std::endl;
                stream << "STOREDVAR " << var.id << std::endl;
            }
            else
                stream << "INVALID" << std::endl;           
            break;
        default:
            stream << "INVALID" << std::endl;
            return;
    }
}

void BCodeVisitor::visitForNode(mathvm::ForNode* node) {
    //this is OLD, NOT WORKING
    mathvm::BinaryOpNode* inExpr = 
        dynamic_cast<mathvm::BinaryOpNode*>(node->inExpr());
    if (!inExpr)
        stream << "fail::visitForNode" << std::endl;
    //UnaryOpNode
    mathvm::IntLiteralNode* from = 
        dynamic_cast<mathvm::IntLiteralNode*>(inExpr->left());
    mathvm::IntLiteralNode* to = 
        dynamic_cast<mathvm::IntLiteralNode*>(inExpr->right());
    stream << "store " << from->literal()  << " to " << node->var()->name() << std::endl;
    stream << "store " << to->literal() << " to " << "";
    int cmp_addr  = stream.tellp();
    //Get code for body() and let its size to jump after it
    //stream  = body_stream
    stream << "push local TO var" << std::endl;
    stream << "push " << node->var()->name() << std::endl;
    stream << "IFICMPG  +sizeof_block" << std::endl;
    node->body()->visit(this);
    stream << "JA -(current-" << cmp_addr  << ")" << std::endl;
}

void BCodeVisitor::visitWhileNode(mathvm::WhileNode* node) {
    node->whileExpr()->visit(this);
    node->loopBlock()->visit(this);
}

void BCodeVisitor::visitIfNode(mathvm::IfNode* node) {
    node->ifExpr()->visit(this);
    node->thenBlock()->visit(this);
    if (node->elseBlock()) {
        node->elseBlock()->visit(this);
    }
}

void BCodeVisitor::visitBlockNode(mathvm::BlockNode* node) {
    mathvm::Scope::VarIterator it(node->scope());
    while(it.hasNext()) {
        mathvm::AstVar* curr = it.next();
        saveInfo(node, newId(), curr->type());
    }
    node->visitChildren(this);
}

void BCodeVisitor::visitFunctionNode(mathvm::FunctionNode* node) {
    node->name();
    node->args()->visit(this);
    node->body()->visit(this);
}

void BCodeVisitor::visitPrintNode(mathvm::PrintNode* node) {
    saveInfo(node, 0, mathvm::VT_INVALID);
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        NodeInfo& nop = getInfo(node->operandAt(i));
        switch(nop.type) {
            case(mathvm::VT_STRING):
                stream << "LOADSVAR " << nop.id << std::endl;
                break;
            case(mathvm::VT_INT):
                stream << "LOADIVAR " << nop.id << std::endl;
                break;
            case(mathvm::VT_DOUBLE):
                stream << "LOADDVAR " << nop.id << std::endl;
                break;
            default:
                stream << "INVALID";
        }
        stream << "DUMP "  << std::endl;
        stream << "POP " << std::endl;
    }
}

size_t BCodeVisitor::newId() {
    return id_counter++;
}

BCodeVisitor::NodeInfo& BCodeVisitor::saveInfo(const void* node, size_t id, mathvm::VarType type) {
    NodeInfo& n = nodeInfoMap[node];
    n.id = id;
    n.type = type;
    //n.node = node;
    return n;
}

BCodeVisitor::NodeInfo& BCodeVisitor::getInfo(const void* node) {
    return nodeInfoMap[node];
}

int BCodeVisitor::genInstrBinNode(const BCodeVisitor::NodeInfo &a, const BCodeVisitor::NodeInfo &b, 
                                     mathvm::TokenKind op, mathvm::VarType& resType, std::string& instruction) {
    //the most readable solution - switches
    using namespace mathvm;
    if (a.type == VT_INT && b.type == VT_INT) {
        resType = VT_INT;
        switch (op) {
            case tADD:
                instruction = "BC_IADD\n";
                break;
            case tSUB:
                instruction = "BC_ISUB\n";
                break;
            case tMUL:
                instruction = "BC_IMUL\n";
                break;
            case tDIV:
                instruction = "BC_IDIV\n";
                break;
            case tOR:
                instruction += "ILOAD0\n";
                instruction += "IFICMPNE +5\n";
                instruction += "ILOAD0\n";
                instruction += "IFICMPNE +3\n";
                instruction += "ILOAD0\n"; //FALSE
                instruction += "JA +2\n";
                instruction += "ILOAD1\n"; //TRUE
                break;
            case tAND:
                instruction += "ILOAD0\n";
                instruction += "IFICMPE +3\n";
                instruction += "ILOAD0\n";
                instruction += "IFICMPNE +3\n";
                instruction += "ILOAD0\n"; //FALSE
                instruction += "JA +2\n";
                instruction += "ILOAD1\n"; //TRUE
                break;
            case tEQ:
                instruction += "IFICMPE +3\n";
                instruction += "ILOAD0\n"; //FALSE
                instruction += "JA +2\n";
                instruction += "ILOAD1";//TRUE
                break;
            case tNEQ:
                instruction += "IFICMPNE +3\n";
                instruction += "ILOAD0\n"; //FALSE
                instruction += "JA +2\n";
                instruction += "ILOAD1";//TRUE
                break;
            case tGT:
                instruction += "IFICMG +3\n";
                instruction += "ILOAD0\n";//FALSE
                instruction += "JA +2\n";
                instruction += "ILOAD1\n";//TRUE
                break;
            case tLT:
                instruction += "IFICML +3\n";
                instruction += "ILOAD0\n";//FALSE
                instruction += "JA +2\n";
                instruction += "ILOAD1\n";//TRUE
                break;
            case tGE:
                instruction += "IFICMGE +3\n";
                instruction += "ILOAD0\n";//FALSE
                instruction += "JA +2\n";
                instruction += "ILOAD1\n";//TRUE
                break;
            case tLE:
                instruction += "IFICMLE +3\n";
                instruction += "ILOAD0\n";//FALSE
                instruction += "JA +2\n";
                instruction += "ILOAD1\n";//TRUE
                break;
            default:
                return -1;
        }
        return 0;
    }

    if ((a.type == VT_DOUBLE || a.type == VT_INT) &&
        (b.type == VT_DOUBLE || b.type == VT_INT)) {
        resType = VT_DOUBLE;                
        switch (op) {
            case tADD:
                instruction = "BC_DADD\n";
                break;
            case tSUB:
                instruction = "BC_DSUB\n";
                break;
            case tMUL:
                instruction = "BC_DMUL\n";
                break;
            case tDIV:
                instruction = "BC_DDIV\n";
                break;
            case tEQ:
                instruction += "IFDCMPE +3\n";
                instruction += "ILOAD0\n"; //FALSE
                instruction += "JA +2\n";
                instruction += "ILOAD1\n";//TRUE
                break;
            case tNEQ:
                instruction += "IFDCMPNE +3\n";
                instruction += "ILOAD0\n"; //FALSE
                instruction += "JA +2\n";
                instruction += "ILOAD1\n";//TRUE
                break;
            case tGT:
                instruction += "IFDCMG +3\n";
                instruction += "ILOAD0\n";//FALSE
                instruction += "JA +2\n";
                instruction += "ILOAD1\n";//TRUE
                break;
            case tLT:
                instruction += "IFDCML +3\n";
                instruction += "ILOAD0\n";//FALSE
                instruction += "JA +2\n";
                instruction += "ILOAD1\n";//TRUE
                break;
            case tGE:
                instruction += "IFDCMGE +3\n";
                instruction += "ILOAD0\n";//FALSE
                instruction += "JA +2\n";
                instruction += "ILOAD1\n";//TRUE
                break;
            case tLE:
                instruction += "IFICMLE +3\n";
                instruction += "ILOAD0\n";//FALSE
                instruction += "JA +2\n";
                instruction += "ILOAD1\n";//TRUE
                break;
            default:
                return -1;
        }
        return 0;
    } else return -1; //string binary ops are ot permitted
}

