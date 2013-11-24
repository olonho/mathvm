#include "parser.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdexcept>

using namespace mathvm;

class CodeImpl : public Code {
public:
    CodeImpl() {}
    virtual ~CodeImpl() {}

    virtual Status* execute(vector<Var*>& vars) {
        // TODO Implement me
        return 0;
    }
};

// Exception class for type errors etc...
class error : public exception {
    string _msg;

public:
    explicit error(const string& msg): _msg(msg) {}
    explicit error(const char* msg): _msg(msg) {}

    virtual ~error() throw () {}

    const char* what() const throw () {
        return _msg.c_str();
    }
};

// Utils

bool isIntType(VarType type) {
    return type == VT_INT;
}

bool isNumericType(VarType type) {
    return type == VT_DOUBLE || type == VT_INT;
}

bool isStringType(VarType type) {
    return type == VT_STRING;
}

class TranslatorVisitor : public AstVisitor {

public: // constructors

    TranslatorVisitor(AstFunction* top, Code* code): _code(code), _scopeId(-1) {}

    virtual ~TranslatorVisitor() {}

private: // structs

    // Internal compiler structs

    struct BcVar {

        string name;
        VarType type;
        uint16_t id;
        uint16_t scopeId;

        BcVar() {}

        BcVar(string name, VarType type, uint16_t id, uint16_t scopeId)
            : name(name), type(type), id(id), scopeId(scopeId) {}
    };

    union BcVal {
        int64_t ival;
        double dval;
        uint16_t sval;
    };  


private: // fields

    Code* _code;

    // Current function Bytecode
    Bytecode* _bc;

    // Name to bytecode variable map
    // TODO It must be consistent with multiple scopes 
    map<string, BcVar> _nameToBcVarMap;

    // Variable id to BcVar map
    map<uint16_t, BcVar> _idToBcVarMap;

    // Type of the last expression
    VarType _lastType;

    // Current function id
    uint16_t _fId;

    // Scope id
    uint16_t _scopeId;


private: // methods

    // checkers

    void checkNumericTypes(VarType left, VarType right) const {
        if ((left != VT_INT && left != VT_DOUBLE) || 
            (right != VT_INT && right != VT_DOUBLE)) {

            stringstream msg;
            msg << "Invalid types for numeric operation. "
                << "Left operand type: " << typeToName(left)
                << ". Right operand type: " << typeToName(right);
            throw error(msg.str());
        }
    }

    void checkOp2(VarType left, VarType right, 
                                    VarType expected, TokenKind kind) const {

        if (left != expected || right != expected) {
            stringstream msg;
            msg << "Invalid binary operation. " << tokenOp(kind)
                << ". Left operand type: " << typeToName(left)
                << ". Right operand type: " << typeToName(right)
                << ". Expected type: " << typeToName(expected);
            throw error(msg.str());
        }
    }

    void checkVarType(VarType expected, VarType found) const {
        if (expected != found) {
            string msg("Type error. Expected: ");
            msg += typeToName(expected);
            msg += ". Found: ";
            msg += typeToName(found);
            throw error(msg);
        }
    }

    // utils

    #define GENERATE(name, type) \
        void name(type op) { _bc->name(op); }

    GENERATE(addInsn, Instruction);
    GENERATE(bind, Label&);
    GENERATE(addInt16, int16_t);
    GENERATE(addInt64, int64_t);
    GENERATE(addDouble, double);
    GENERATE(addUInt16, uint16_t);

    #undef GENERATE

    #define GENERATE(name, type1, type2) \
        void name(type1 op1, type2 op2) { _bc->name(op1, op2); }

    GENERATE(addBranch, Instruction, Label&);
    #undef GENERATE

    void addLogicOperator(TokenKind kind) {
        addInsn2(BC_ICMP, BC_DCMP, _lastType);
        _lastType = VT_INT;
        Label endIf(_bc);
        Label elseIf(_bc);
        switch (kind) {
            case tEQ: {
                addInsn(BC_ILOAD0);
                addBranch(BC_IFICMPE, elseIf);
                break; 
            }
            case tGT: {
                addInsn(BC_ILOAD1);
                addBranch(BC_IFICMPE, elseIf);
                break; 
            }
            case tGE: {
                addInsn(BC_ILOADM1);
                addBranch(BC_IFICMPNE, elseIf);
                break; 
            }
            case tLT: {
                addInsn(BC_ILOADM1);
                addBranch(BC_IFICMPE, elseIf);
                break;
            }
            case tNEQ: {
                addInsn(BC_ILOAD0);
                addBranch(BC_IFICMPNE, elseIf);
                break; 
            }
            case tLE: {
                addInsn(BC_ILOAD1);
                addBranch(BC_IFICMPNE, elseIf);
                break; 
            }
            default: break;
        }
        addInsn(BC_ILOAD0);
        addBranch(BC_JA, endIf);
        bind(elseIf);
        addInsn(BC_ILOAD1);
        bind(endIf);
    }

    void addIntOperator(TokenKind kind) {
        Label elseIf(_bc);
        Label endIf(_bc);
        switch (kind) {
            case tOR: {
                addInsn(BC_ILOAD0);
                addBranch(BC_IFICMPNE, elseIf);
                addInsn(BC_ILOAD0);
                addBranch(BC_IFICMPNE, elseIf);
                addInsn(BC_ILOAD0);
                addBranch(BC_JA, endIf);
                bind(elseIf);
                addInsn(BC_ILOAD1);
                bind(endIf);
                break;
            }
            case tAND: {
                addInsn(BC_ILOAD0);
                addBranch(BC_IFICMPE, elseIf);
                addInsn(BC_ILOAD0);
                addBranch(BC_IFICMPE, elseIf);
                addInsn(BC_ILOAD1);
                addBranch(BC_JA, endIf);
                bind(elseIf);
                addInsn(BC_ILOAD0);
                bind(endIf);                    
                break;
            }
            case tAOR:
                addInsn(BC_IAOR); break;
            case tAAND:
                addInsn(BC_IAAND); break;
            case tAXOR:
                addInsn(BC_IAXOR); break;
            default: break;
        }
    }

    void addNumericOperator(TokenKind kind) {
        switch (kind) {
            case tADD: 
                addInsn2(BC_IADD, BC_DADD, _lastType); break;
            case tSUB:
                addInsn2(BC_ISUB, BC_DSUB, _lastType); break;
            case tMUL:
                addInsn2(BC_IMUL, BC_DMUL, _lastType); break; 
            case tDIV:
                addInsn2(BC_IDIV, BC_DDIV, _lastType); break;
            default: break;
        }
    }

    void addConvertOps(VarType left, VarType right) {
        checkNumericTypes(left, right);
        // left is upper, right is lower
        if (left!= right) {
            if (right == VT_DOUBLE) {
                addIntToDoubleConv();
            } else if (left == VT_DOUBLE) {
                addInsn(BC_STOREDVAR3);
                addIntToDoubleConv();
                addInsn(BC_LOADDVAR3);
            }
            _lastType = VT_DOUBLE;
        }
    }

    void addDoubleToIntConv() {
        addInsn(BC_D2I);
    }

    void addIntToDoubleConv() {
        addInsn(BC_I2D);
    }

    void addLiteralOnTOS(VarType type, BcVal u) {
        switch(type) {
            case VT_INT:
                addInsn(BC_ILOAD);
                addInt64(u.ival);
                break;
            case VT_DOUBLE:
                addInsn(BC_DLOAD);
                addDouble(u.dval);
                break;
            case VT_STRING:
                addInsn(BC_SLOAD);
                addInt16(u.sval);
                break;
            default:
                throw error("Unsupported type for operation. ");
        }
    }

    void addVarInsn3(Instruction bcInt, Instruction bcDouble, 
                                       Instruction bcString, const BcVar& var) {

        switch (var.type) {
            case VT_INT:
                addInsn(bcInt);
                break;
            case VT_DOUBLE:
                addInsn(bcDouble);
                break;
            case VT_STRING:
                addInsn(bcString);
                break; 
            default:
                throw error("Invalid type of variable: " + var.name);
        }
        addUInt16(var.id);
    }

    void addInsn2(Instruction bcInt, Instruction bcDouble, VarType type) {
        // FIXME What to do with string? Now we can't add strings etc...
        switch (type) {
            case VT_INT:
                addInsn(bcInt);
                break;
            case VT_DOUBLE:
                addInsn(bcDouble);
                break;
            case VT_STRING:
                throw error("Invalid operation on string. " );
                break;
            default:
                throw error("Unsupported type for operation. ");
        }
    }

    void addLoadVarInsn(const BcVar& var) {
        addVarInsn3(BC_LOADIVAR, BC_LOADDVAR, BC_LOADSVAR, var);
    }

    void addStoreVarInsn(const BcVar& var) {
        addVarInsn3(BC_STOREIVAR, BC_STOREDVAR, BC_STORESVAR, var);
    }

    void addAddInsn(VarType type) {
        addInsn2(BC_IADD, BC_DADD, type);
    }

    void addSubInsn(VarType type) {
        addInsn2(BC_ISUB, BC_DSUB, type);
    }

    BcVar* findBcVarForName(const string& name) {
        map<string, BcVar>::iterator variter = _nameToBcVarMap.find(name);

        if (variter == _nameToBcVarMap.end()) {
            throw error("Unresolved reference: " + name);
        }

        return &variter->second;
    }

    BcVar* findBcVarForId(uint16_t id) {
        map<uint16_t, BcVar>::iterator variter = _idToBcVarMap.find(id);

        if (variter == _idToBcVarMap.end()) {
            stringstream msg;
            msg << "Can't find variable by id. Id: " << id;
            throw error(msg.str());
        }

        return &variter->second;
    }

    // visitors

    void visitBinaryOpNode(BinaryOpNode* node) {
        TokenKind kind = node->kind();
        // left is upper, right is lower
        node->right()->visit(this);
        VarType lower = _lastType;
        node->left()->visit(this);
        VarType upper = _lastType;
        switch (kind) {
            case tOR: 
            case tAND:
            case tAOR:
            case tAAND:
            case tAXOR:  
                checkOp2(upper, lower, VT_INT, kind);
                addIntOperator(kind);
                break;

            case tEQ:
            case tNEQ:
            case tGT:
            case tGE:
            case tLT:
            case tLE: 
                addConvertOps(upper, lower);
                addLogicOperator(kind);
                break;

            case tADD:
            case tSUB:
            case tMUL:
            case tDIV:
                addConvertOps(upper, lower);
                addNumericOperator(kind);
                break;

            case tMOD: 
                checkOp2(upper, lower, VT_INT, kind);
                addInsn(BC_IMOD);
                break;
            case tRANGE:
                checkOp2(upper, lower, VT_INT, kind);
                break;
            default: throw error("Unknown token. ");
        }
    }

    void visitUnaryOpNode(UnaryOpNode* node) {
        TokenKind kind = node->kind();
        node->operand()->visit(this);
        checkNumericTypes(_lastType, VT_INT);

        if (_lastType == VT_DOUBLE && kind == tNOT) {
            throw error("Can't do logical not on double. ");
        }
        switch (kind) {
            case tSUB:
                addInsn2(BC_INEG, BC_DNEG, _lastType);
                break;
            case tNOT: {
                Label elseIf(_bc);
                Label endIf(_bc);
                addInsn(BC_ILOAD0);
                addBranch(BC_IFICMPNE, elseIf);
                addInsn(BC_ILOAD1);
                addBranch(BC_JA, endIf);
                bind(elseIf);
                addInsn(BC_ILOAD0);
                bind(endIf);
                break;
            }
            default: throw error("Unknown token");
        }

    }

    void visitStringLiteralNode(StringLiteralNode* node) {
        BcVal literal;
        uint16_t id = _code->makeStringConstant(node->literal());
        literal.sval = id;
        addLiteralOnTOS(VT_STRING, literal);
        _lastType = VT_STRING;
    }

    void visitDoubleLiteralNode(DoubleLiteralNode* node) {
        BcVal literal;
        literal.dval = node->literal();
        addLiteralOnTOS(VT_DOUBLE, literal);
        _lastType = VT_DOUBLE;
    }

    void visitIntLiteralNode(IntLiteralNode* node) {
        BcVal literal;
        literal.ival = node->literal();
        addLiteralOnTOS(VT_INT, literal);
        _lastType = VT_INT;
    }

    void visitLoadNode(LoadNode* node) {
        VarType nodeType = node->var()->type();
        string nodeName = node->var()->name();

        BcVar* var = findBcVarForName(nodeName);
        checkVarType(var->type, nodeType);

        addLoadVarInsn(*var);
        _lastType = var->type;
    }

    void visitStoreNode(StoreNode* node) {
        // TODO add int/double implicit conversions
        node->visitChildren(this);

        BcVar* var = findBcVarForName(node->var()->name());

        // Int to double implicit conversion
        if (var->type == VT_DOUBLE && _lastType == VT_INT) {
            addIntToDoubleConv();
        } else {
            checkVarType(var->type, _lastType);
        }

        try {
            switch (node->op()) {
                case tINCRSET:
                    addLoadVarInsn(*var);
                    addAddInsn(var->type);
                    break;
                case tDECRSET:
                    addLoadVarInsn(*var);
                    addSubInsn(var->type);
                    break;
                case tASSIGN:
                    break;
                default: throw error("Unknown store operator. ");
            }
        } catch (exception& e) {
            stringstream msg;
            msg << e.what() << "Variable name is: " << var->name << endl;
            throw error(msg.str());
        }
        
        addStoreVarInsn(*var);
    }

    void visitForNode(ForNode* node) {
        // IVAR0 - counter, IVAR3 - general purpose register
        // Save IVAR0 on stack
        addInsn(BC_LOADIVAR0);
        // FIXME Range variable could be used in for block
        node->inExpr()->visit(this);
        addInsn(BC_STOREIVAR0);

        Label beginFor(_bc);
        Label endFor(_bc);

        bind(beginFor);
        addInsn(BC_STOREIVAR3);
        addInsn(BC_LOADIVAR3);
        addInsn(BC_LOADIVAR0);
        addBranch(BC_IFICMPG, endFor);
        addInsn(BC_LOADIVAR3);

        node->body()->visit(this);

        addBranch(BC_JA, beginFor);
        bind(endFor);

        // Restore IVAR0 from TOS
        addInsn(BC_STOREIVAR0);
    }
        
    void visitWhileNode(WhileNode* node) {
        Label loopWhile(_bc);
        Label endWhile(_bc);
        bind(loopWhile);

        node->whileExpr()->visit(this);
        VarType exprType = _lastType;
        checkVarType(exprType, VT_INT);

        addInsn(BC_ILOAD0);
        addBranch(BC_IFICMPE, endWhile);
        node->loopBlock()->visit(this);
        addBranch(BC_JA, loopWhile);
        bind(endWhile);
    }

    void visitIfNode(IfNode* node) {
        node->ifExpr()->visit(this);
        VarType exprType = _lastType;
        checkVarType(exprType, VT_INT);
        Label elseIf(_bc);
        Label endIf(_bc);
        addInsn(BC_ILOAD0);

        if (node->elseBlock()) {
            addBranch(BC_IFICMPE, elseIf);
            node->thenBlock()->visit(this);
            addBranch(BC_JA, endIf);
            bind(elseIf);
            node->elseBlock()->visit(this);
        } else {
            addBranch(BC_IFICMPE, endIf);
            node->thenBlock()->visit(this);
        }
        bind(endIf);
    }

    void visitBlockNode(BlockNode* node) {
        // Block variables declarations
        _scopeId++;

        Scope* scope = node->scope();
        Scope::VarIterator variter(scope);

        uint32_t size = node->nodes();
        uint16_t vId = 0;

        while (variter.hasNext()) {
            AstVar* ptr = variter.next();
            BcVar var(ptr->name(), ptr->type(), vId, _scopeId);

            _nameToBcVarMap[ptr->name()] = var;
            _idToBcVarMap[vId] = var;

            ++vId;
        }

        // Block functions declarations
        Scope::FunctionIterator funciter(scope);
        while (funciter.hasNext()) {
            AstFunction* func = funciter.next();
            func->node()->visit(this);
        }

        for (uint32_t i = 0; i < size; ++i) {
            node->nodeAt(i)->visit(this);
        }

    }

    void visitPrintNode(PrintNode* node) {
        size_t nodes = node->operands();
        for (size_t i = 0; i < nodes; ++i) {
            node->operandAt(i)->visit(this);
            switch (_lastType) {
                case VT_INT:
                    addInsn(BC_IPRINT);
                    break;
                case VT_DOUBLE:
                    addInsn(BC_DPRINT);
                    break;
                case VT_STRING:
                    addInsn(BC_SPRINT); 
                    // What about string constants? 
                    // SPRINT expects that string value pushed on TOS,
                    // but constants stored by id. 
                    break;
                default: throw error("Unknown type: ");
            }
        }
    }

    void visitCallNode(CallNode* node) {
    // TODO Implement me    

    }

    void visitReturnNode(ReturnNode* node) {
        if (node->returnExpr()) {
            node->returnExpr()->visit(this);
            checkVarType(_code->functionById(_fId)->returnType(), _lastType);
        }
    }

    void visitNativeCallNode(NativeCallNode* node) {
    // FIXME Stub code only 
        uint16_t nativeId = _code->makeNativeFunction(node->nativeName(), 
                                                      node->nativeSignature(), 0);
        addInsn(BC_CALLNATIVE);
        addUInt16(nativeId);
    }

    void visitFunctionNode(FunctionNode* node) {
        Bytecode* saveFunc = _bc;
        uint16_t saveId = _fId;
        Scope* scope = node->body()->scope();

        BytecodeFunction* bcFunction = new BytecodeFunction(scope->lookupFunction(node->name()));
        _bc = bcFunction->bytecode();
        _fId = _code->addFunction(bcFunction);
        bcFunction->setScopeId(_scopeId);

        node->body()->visit(this);

        addInsn(BC_STOP);

        _bc = saveFunc;
        _fId = saveId;

    }

};

// Choose the right visitor
Status* translateAST(AstFunction* main, Code* code) {
    TranslatorVisitor visitor(main, code);
    try {
        main->node()->visit(&visitor);
    } catch (exception& e) {
        return new Status(e.what());
    }

    return 0;
}


int main(int argc, char const *argv[]) {

    if (argc != 2) {
        cerr << "USAGE: <source filename>. " << endl;
        return 1;
    }

    string filename = argv[1];

    ifstream input(filename.c_str());
    if (!input) {
        cerr << "File: " << filename << "  does not exist. " 
                  << endl;
        return 1;
    }

    stringstream stream;
    stream << input.rdbuf();
    string source(stream.str());

    Parser parser;

    if (Status* s = parser.parseProgram(source)) {
        cout << "There is some error while parsing. Error message:\n"  
                  << s->getError() << endl;
        return 1;
    }

    Code* code = new CodeImpl;

    AstFunction* main = parser.top();

    if (Status* s = translateAST(main, code)) {
        cout << "There is some error while translating. Error message:\n"
                  << s->getError() << endl;
        return 1;
    }

    code->disassemble(cout);

    delete code;

    return 0;
}