#ifndef BCTRANSLATOR_H
#define BCTRANSLATOR_H

#include "mathvm.h"
#include "ast.h"

namespace mathvm {

class BCTranslator : public Translator, AstVisitor {
public:
    BCTranslator() { }

    Status *translate(const string &program, Code **code);

private:

#define VISITOR_FUNCTION(type, name) void visit##type(type* node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

    void translateFunction(AstFunction *f);
    void convertTOSType(VarType to);
    void convertTOSType2(VarType to);
    void convertTOSToBool();

    //------------------------------------------------------------

    bool isNumType(VarType t) const {
        return t == VT_INT || t == VT_DOUBLE;
    }

    VarType makeNumTypeCast(VarType ltype, VarType rtype, VarType toType, bool soft = false);
    void makeComparisonOperation(TokenKind op);
    void makeMathOperation(TokenKind op);
    void makeLogicOperation(TokenKind op);

    //------------------------------------------------------------

    void enterScope(BytecodeFunction *f) {
        currentScope = new TScope(f, currentScope);
    }

    void exitScope() {
        TScope *pScope = currentScope->parent;
        delete currentScope;
        currentScope = pScope;
    }

    Bytecode *currentBC() {
        return currentScope->function->bytecode();
    }

    //------------------------------------------------------------

    void processVar(const AstVar *var, bool loading = false);

    void loadVar(const AstVar *var) {
        processVar(var, true);
        tosType = var->type();
    }

    void storeVar(const AstVar *var) {
        processVar(var, false);
    }

    //------------------------------------------------------------

    class TScope {
        typedef std::map<const AstVar*, uint16_t> VarMap;

    public:
        TScope(BytecodeFunction *f, TScope *p = 0) : parent(p), function(f) {}

        void addVar(AstVar *v) {
            vars.insert(std::make_pair(v, vars.size()));
        }

        std::pair<uint16_t, TScope*> getVar(const AstVar *v) {
            VarMap::iterator vi = vars.find(v);
            if(vi != vars.end()) return std::make_pair(vi->second, this);
            else {
                if(!parent) throw std::string("Var " + v->name() + " not found");
                return parent->getVar(v);
            }
        }

        TScope *parent;
        BytecodeFunction *function;
        VarMap vars;
    };

    Code *code;
    TScope *currentScope;
    VarType tosType;
};

}

#endif // BCTRANSLATOR_H
