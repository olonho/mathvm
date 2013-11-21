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

// Utils

size_t getSizeOfType(VarType type) {
    switch (type) {
        case VT_INT:
            return sizeof(int64_t);
        case VT_DOUBLE:
            return sizeof(double);
        default:
            return 0;
    }
}

// Internal compiler structs

struct BcVar {

    string name;
    VarType type;
    uint16_t id;

    BcVar(): name("DEFAULT NAME"), type(VT_INVALID), id(-1) {}

    BcVar(string name, VarType type, uint16_t id): name(name), type(type), id(id) {}
};

// This class is simply translates body of the main function (without function calls, scopes and function declaration yet)
class AstVisitorHelper : public AstVisitor {
    Bytecode* _code;

private: // fields

    vector<string> _constants;
    map<string, BcVar> _nameToBcVarMap;
    map<uint16_t, uint32_t> _idToBciMap;

    VarType _lastType;
    uint32_t _sp;

public: // constructors

    AstVisitorHelper(): _lastType(VT_INVALID), _sp(0) {}

    virtual ~AstVisitorHelper() {}

    void setBytecode(Bytecode* code) {
        _code = code;
    }

public: // methods

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
    void addLoadVarInsn(BcVar* var);

    BcVar* findBcVarForName(const string& name);
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

void AstVisitorHelper::addLoadVarInsn(BcVar* var) {
    switch (var->type) {
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
            throw error("Invalid type of variable: " + var->name);
            break;
        default:
            break;
    }
    _code->addInt16(var->id);
}

BcVar* AstVisitorHelper::findBcVarForName(const string& name) {
    map<string, BcVar>::iterator variter = _nameToBcVarMap.find(name);

    if (variter == _nameToBcVarMap.end()) {
        throw error("Unresolved reference: " + name);
    }

    return &variter->second;
}

// visitors

void AstVisitorHelper::visitLoadNode(LoadNode* node) {
    VarType nodeType = node->var()->type();
    string nodeName = node->var()->name();

    BcVar* var = findBcVarForName(nodeName);
    checkVarType(var->type, nodeType);

    addLoadVarInsn(var);
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
    uint16_t index = 0;

    while (variter.hasNext()) {
        AstVar* ptr = variter.next();
        BcVar var(ptr->name(), ptr->type(), index);
        _nameToBcVarMap[ptr->name()] = var;
        _idToBciMap[index] = _sp;
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