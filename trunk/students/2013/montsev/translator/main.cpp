#include "parser.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdexcept>

using namespace mathvm;

// Exception class for handling type errors etc...
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

// Configuration functions 

// Is stack variable type checker. Now int and double is stack variable types.
bool isStackVariableType(VarType type) {
    switch (type) {
        case VT_INT:
        case VT_DOUBLE:
            return true;
        default:
            return false;
    }
}

// Utils

size_t getSizeOfType(VarType type) {
    switch (type) {
        case VT_INT:
            return sizeof(int64_t);
        case VT_DOUBLE:
            return sizeof(double);
        default:
        // If VarType is String then return 0 because String variables stores not in stack memory
            return 0;
    }
}

// Internal compiler structs

struct BcVar {

    string name;
    VarType type;
    uint16_t id;
    uint32_t address;

    BcVar(): name("DEFAULT NAME"), type(VT_INVALID), id(-1), address(Status::INVALID_POSITION) {}

    BcVar(string name, VarType type, uint16_t id, uint32_t address = Status::INVALID_POSITION)
        : name(name), type(type), id(id), address(address) {}
};

// This class is simply translates body of the main function (without function calls, scopes and function declaration yet)
class AstVisitorHelper : public AstVisitor {
    
private: // fields

    Bytecode* _code;

    // String literal constants, id of the constant is position in vector
    vector<string> _constants;

    // Name to bytecode variable map
    // TODO fix it to be consistent with multiple scopes 
    map<string, BcVar> _nameToBcVarMap;

    // Variable id to stack address map, if it'snt a stack variable then return INVALID_POSITION   
    map<uint16_t, uint32_t> _idToSaMap;

    // Variable id to BcVar map
    map<uint16_t, BcVar> _idToBcVarMap;

    // Type of the last expression
    VarType _lastType;

    // Stack pointer to last variable
    uint32_t _sp;

public: // constructors

    AstVisitorHelper(): _lastType(VT_INVALID), _sp(0) {}

    virtual ~AstVisitorHelper() {}

    void setBytecode(Bytecode* code) {
        _code = code;
    }

public: // methods

    // visitors

    virtual void visitBinaryOpNode(BinaryOpNode* node);

    virtual void visitUnaryOpNode(UnaryOpNode* node);

    virtual void visitStringLiteralNode(StringLiteralNode* node);

    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node);

    virtual void visitIntLiteralNode(IntLiteralNode* node);

    virtual void visitLoadNode(LoadNode* node);

    virtual void visitStoreNode(StoreNode* node);

    virtual void visitForNode(ForNode* node);

    virtual void visitWhileNode(WhileNode* node);

    virtual void visitIfNode(IfNode* node);

    virtual void visitBlockNode(BlockNode* node);

    virtual void visitFunctionNode(FunctionNode* node);

private: // methods

    // checkers    

    void checkVarType(VarType expected, VarType found) const;

    // utils

    void addLoadVarInsn(const BcVar& var);

    BcVar* findBcVarForName(const string& name);
    BcVar* findBcVarForId(uint16_t id);

    void putIdToSaValue(uint16_t id);
};

// Choose the right visitor
Status* translateAST(AstFunction* main, Bytecode* code) {
    AstVisitorHelper visitor;
    visitor.setBytecode(code);

    try {
        main->node()->body()->visit(&visitor);
    } catch (exception& e) {
        return new Status(e.what());
    }

    return 0;
}


// AstVisitorHelper checkers

void AstVisitorHelper::checkVarType(VarType expected, VarType found) const {
   // TODO try type error cases
    if (expected != found) {
        string msg("Type error. Expected: ");
        msg += typeToName(expected);
        msg += ". Found: ";
        msg += typeToName(found);
        throw error(msg);
    }
}

// AstVisitorHelper utils

void AstVisitorHelper::addLoadVarInsn(const BcVar& var) {
    switch (var.type) {
        case VT_INT:
            _code->addInsn(BC_LOADIVAR);
            break;
        case VT_DOUBLE:
            _code->addInsn(BC_LOADDVAR);
            break;
        case VT_STRING:
            _code->addInsn(BC_LOADSVAR);
            break;
        case VT_INVALID:
            throw error("Invalid type of variable: " + var.name);
            break;
        default:
            break;
    }
    _code->addInt16(var.id);
}

BcVar* AstVisitorHelper::findBcVarForName(const string& name) {
    map<string, BcVar>::iterator variter = _nameToBcVarMap.find(name);

    if (variter == _nameToBcVarMap.end()) {
        throw error("Unresolved reference: " + name);
    }

    return &variter->second;
}

BcVar* AstVisitorHelper::findBcVarForId(uint16_t id) {
    map<uint16_t, BcVar>::iterator variter = _idToBcVarMap.find(id);

    if (variter == _idToBcVarMap.end()) {
        stringstream msg;
        msg << "Can't find variable by id. Id: " << id;
        throw error(msg.str());
    }

    return &variter->second;
}

void AstVisitorHelper::putIdToSaValue(uint16_t id) {

    BcVar* var = findBcVarForId(id);

    if (isStackVariableType(var->type)) {
        _idToSaMap[id] = _sp;
    } else {
        _idToSaMap[id] = Status::INVALID_POSITION;
    }
}

// AstVisitorHelper visitors

void AstVisitorHelper::visitBinaryOpNode(BinaryOpNode* node) {

}

void AstVisitorHelper::visitUnaryOpNode(UnaryOpNode* node) {

}

void AstVisitorHelper::visitStringLiteralNode(StringLiteralNode* node) {

}

void AstVisitorHelper::visitDoubleLiteralNode(DoubleLiteralNode* node) {

}

void AstVisitorHelper::visitIntLiteralNode(IntLiteralNode* node) {

}

void AstVisitorHelper::visitLoadNode(LoadNode* node) {
    VarType nodeType = node->var()->type();
    string nodeName = node->var()->name();

    BcVar* var = findBcVarForName(nodeName);
    checkVarType(var->type, nodeType);

    addLoadVarInsn(*var);
}

void AstVisitorHelper::visitStoreNode(StoreNode* node) {

}

void AstVisitorHelper::visitForNode(ForNode* node) {

}

void AstVisitorHelper::visitWhileNode(WhileNode* node) {

}

void AstVisitorHelper::visitIfNode(IfNode* node) {

}

void AstVisitorHelper::visitBlockNode(BlockNode* node) {

    // Initializing block variables declarations
    Scope* scope = node->scope();
    Scope::VarIterator variter(scope);

    uint32_t size = node->nodes();
    uint16_t vId = 0;

    while (variter.hasNext()) {
        AstVar* ptr = variter.next();
        BcVar var(ptr->name(), ptr->type(), vId);

        _nameToBcVarMap[ptr->name()] = var;
        _idToBcVarMap[vId] = var;

        putIdToSaValue(vId);
        _sp += getSizeOfType(ptr->type());
    }

    for (uint32_t i = 0; i < size; ++i) {
        node->nodeAt(i)->visit(this);
    }

}

void AstVisitorHelper::visitFunctionNode(FunctionNode* node) {

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

    Bytecode* code = new Bytecode();

    AstFunction* main = parser.top();
    if (Status* s = translateAST(main, code)) {
        cout << "There is some error while translating. Error message:\n"
                  << s->getError() << endl;
        return 1;
    }

    code->dump(cout);

    return 0;
}