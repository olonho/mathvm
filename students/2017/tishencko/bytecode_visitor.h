#ifndef BYTECODE_VISITOR
#define BYTECODE_VISITOR

#include "mathvm.h"
#include "visitors.h"
#include "ast.h"
#include "parser.h"
#include "exception.h"
#include "context.h"
#include "interpreter_code.h"
#include <map>
#include <ctgmath>
#include <dlfcn.h>

namespace mathvm {

using namespace std;

class BytecodeVisitor : public AstVisitor {
public:
#define VISITOR_FUNCTION(type, name) \
    void visit##type(type* node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

    BytecodeVisitor(AstFunction * top, InterpreterCode * _code);
    Code* code() {
        return _code;
    }

private:
    vector<Bytecode *> _bytecode;
    vector<VarType> types;
    InterpreterCode * _code;
    Context* context;
    vector<BytecodeFunction*> _currentFunction;

    Bytecode* bytecode() {
        return _bytecode.back();
    }

    BytecodeFunction* currentFunction() {
        return _currentFunction.back();
    }

    void translateFunctionBody(Scope* scope) {
        Scope::FunctionIterator funIt(scope);
        while(funIt.hasNext()) {
           AstFunction * fun = funIt.next();
           fun->node()->visit(this);
        }
    }

    pair<uint16_t, uint16_t> contextFunc(const string& name) {
        auto current = context;
        while (current && !current->includeFunc(name)) {
            current = current->parent;
        }
        assert(current);
        return make_pair(current->addr(), current->findFunAddr(name));
    }

    pair<uint16_t, uint16_t> contextVar(const string& name) {
        auto current = context;
        while (current && !current->includeVar(name)) {
            current = current->parent;
        }
        assert(current);
        return make_pair(current->addr(), current->findVarAddr(name));
    }

    VarType lastType() {
        return types.back();
    }

    void addType(VarType type) {
        types.push_back(type);
    }

    VarType removeType() {
        auto last = lastType();
        types.pop_back();
        return last;
    }

    size_t calculateBinOpType(VarType fst, VarType snd, size_t position) {
        if (fst == VT_STRING && snd == VT_STRING) {
            addType(VT_STRING);
            return 0;
        } else if (fst == VT_INT) {
            if (snd == VT_INT) {
                addType(VT_INT);
                return 0;
            } else if (snd == VT_DOUBLE) {
                addType(VT_DOUBLE);
                return 1;
            }
        } else if (fst == VT_DOUBLE) {
            addType(VT_DOUBLE);
            if (snd == VT_INT) {
                return 2;
            } else if (snd == VT_DOUBLE) {
                return 0;
            }
        }
        throw TranslatorException("Invalid type in ", position);
    }

    void convertToDouble(size_t sinc) {
        if (sinc == 1) {
            bytecode()->addInsn(BC_I2D);
        } else {
            bytecode()->addInsn(BC_SWAP);
            bytecode()->addInsn(BC_I2D);
            bytecode()->addInsn(BC_SWAP);
        }
    }

    void loadVar(VarType type, uint16_t ctx, uint16_t idx, size_t pos)
    {
        switch (type) {
        case VT_INT:
            bytecode()->addInsn(BC_LOADCTXIVAR);
            break;
        case VT_DOUBLE:
            bytecode()->addInsn(BC_LOADCTXDVAR);
            break;
        case VT_STRING:
            bytecode()->addInsn(BC_LOADCTXSVAR);
            break;
        case VT_VOID:
            bytecode()->addInsn(BC_ILOAD0);
        default:
            throw TranslatorException("Couldn't load variable", pos);
        }

        if (type != VT_VOID) {
            bytecode()->addInt16(ctx);
            bytecode()->addInt16(idx);
        }

        addType(type);
    }

    void storeVar(VarType type, uint16_t ctx, uint16_t idx, size_t pos)
    {
        switch (type) {
        case VT_INT:
            bytecode()->addInsn(BC_STORECTXIVAR);
            break;
        case VT_DOUBLE:
            bytecode()->addInsn(BC_STORECTXDVAR);
            break;
        case VT_STRING:
            bytecode()->addInsn(BC_STORECTXSVAR);
            break;
        default:
            throw TranslatorException("Couldn't store variable", pos);
        }

        if (type != VT_VOID) {
            bytecode()->addInt16(ctx);
            bytecode()->addInt16(idx);
        }
    }
    void createNewContext();
};
}

#endif // BYTECODE_VISITOR

