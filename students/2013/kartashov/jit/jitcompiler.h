#ifndef JITCOMPILER_H
#define JITCOMPILER_H

#include "machcode.h"
#include "ast.h"

#include <map>
#include <stdexcept>

#include "codeanalyzer.h"

namespace mathvm {

class JITCompiler : public Translator, AstVisitor {
public:
    JITCompiler(bool debugInfo) : code(0), debugInfo(debugInfo) {}

    Status *translate(const string &program, Code **code);

private:

#define VISITOR_FUNCTION(type, name) void visit##type(type* node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

    void compileFunction(AstFunction *f);

    void convertTOSType(VarType from, VarType to);
    void storeVar(const AstVar *var);
    void loadVar(const AstVar *var);

    VarType numTypeCast(VarType ltype, VarType rtype, VarType toType, bool soft);
    void makeMathOperation(TokenKind op, VarType ntype);
    void makeComparisonOperation(TokenKind op, VarType ntype);
    void makeBitwiseOperation(TokenKind op, VarType ntype);
    void makeLazyLogicOperation(BinaryOpNode *node);
    bool makeMathOperationWithVarReuse(BinaryOpNode *node);

    inline void loadIntConst(int64_t val, bool replace = false);
    inline void loadDoubleConst(double val);

    bool isNumType(VarType t) const {
        return t == VT_INT || t == VT_DOUBLE;
    }

    //--------------------------------------------------------------------------------

    VarType tosType() {
        if(typeStack.empty()) return VT_INVALID;
        return typeStack.top();
    }

    void enterScope(AsmJitFunction *f) {
        currentScope = new TScope(f, currentScope);
    }

    void exitScope() {
        TScope *pScope = currentScope->parent;
        delete currentScope;
        currentScope = pScope;
    }

    AsmJit::Compiler &currentCode() {
        return currentScope->function->code();
    }

    ASJStack &currentStack() {
        return currentScope->function->stack();
    }

    AsmJitVar popCurrentStack() {
        if(currentStack().empty() || typeStack.empty()) throw new std::logic_error("Empty stack detected");
        AsmJitVar var = currentStack().top();
        currentStack().pop();
        typeStack.pop();
        return var;
    }

    void pushCurrentStack(const AsmJitVar &var) {
        currentStack().push(var);
        typeStack.push(var.type);
    }

    void swapCurrentStack() {
        AsmJitVar a = popCurrentStack();
        AsmJitVar b = popCurrentStack();
        pushCurrentStack(a);
        pushCurrentStack(b);
    }

    AsmJitVar createXMMVar(VarType t) {
        return AsmJitVar(t, new AsmJit::XMMVar(currentCode().newXMM(AsmJit::VARIABLE_TYPE_XMM_1D)));
    }

    AsmJitVar createXMMVar(double val) {
        using namespace AsmJit;
        AsmJitVar var = createXMMVar(VT_DOUBLE);
        Compiler &c = currentCode();
        GPVar tmp(c.newGP());
        int64_t *ival = (int64_t*)&val;
        c.mov(tmp, imm(*ival));
        c.movq(*var.xmm, tmp);
        c.unuse(tmp);
        return var;
    }

    AsmJitVar createXMMVar(AsmJit::Mem &ptr) {
        AsmJitVar var = createXMMVar(VT_DOUBLE);
        currentCode().movq(*var.xmm, ptr);
        return var;
    }

    AsmJitVar createGPVar(VarType t) {
        using namespace AsmJit;
        return AsmJitVar(t, new GPVar(currentCode().newGP()));
    }

    AsmJitVar createGPVar(VarType t, int64_t v) {
        AsmJitVar var = createGPVar(t);
        currentCode().mov(*var.gp, AsmJit::imm(v));
        return var;
    }

    AsmJitVar createGPVar(VarType t, AsmJit::Mem &ptr) {
        AsmJitVar var = createGPVar(t);
        currentCode().mov(*var.gp, ptr);
        return var;
    }

    void pushVarOnFunctionStack(AsmJitVar var);

    //--------------------------------------------------------------------------------

    class TScope {
        typedef std::map<const AstVar*, uint16_t> VarMap;

    public:
        TScope(AsmJitFunction *f, TScope *p = 0) : parent(p), function(f), inPrintNode(false), varReuseMode(false) {}

        void addVar(AstVar *v) {
            vars.insert(std::make_pair(v, vars.size()));
        }

        std::pair<uint16_t, TScope*> getVar(const AstVar *v) {
            VarMap::iterator vi = vars.find(v);
            if(vi != vars.end()) return std::make_pair(vi->second, this);
            else {
                if(!parent) throw std::logic_error("Scope::getVar: var " + v->name() + " not found");
                return parent->getVar(v);
            }
        }

        const AstVar *findVar(const std::string &name, uint16_t ownerId) {
            if(function->id() != ownerId) {
                if(!parent) throw std::logic_error("Scope::findVar: var " + name + " not found");
                return parent->findVar(name, ownerId);
            }
            for(VarMap::const_iterator vi = vars.begin(); vi != vars.end(); ++vi) {
                if(vi->first->name() == name) return vi->first;
            }
            throw std::logic_error("Scope::findVar: var " + name + " not found");
        }

        TScope *parent;
        AsmJitFunction *function;
        VarMap vars;
        bool inPrintNode;
        bool varReuseMode;
    };

    //--------------------------------------------------------------------------------

    CodeAnalyzer analyzer;
    AsmJitCodeImpl *code;
    AstNode *currentNode;
    std::stack<VarType> typeStack;
    TScope *currentScope;
    bool debugInfo;
};
}


#endif // JITCOMPILER_H
