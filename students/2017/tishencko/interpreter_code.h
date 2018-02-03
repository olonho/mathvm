#ifndef INTERPRETER_CODE
#define INTERPRETER_CODE

#include "mathvm.h"
#include "ast.h"
#include "parser.h"
#include "exception.h"
#include "context.h"
#include <map>
#include <stack>
#include <ctgmath>

namespace mathvm {

using namespace std;

class InterpreterCode : public Code {
    stack<Var> vars;
    stack<BytecodeFunction *> functions;
    stack<uint32_t> positions;
    map<uint16_t, Context*> contexts;
    stack<map<uint16_t, Context*>> lastContext;

    Bytecode * bytecode() {
        return functions.top()->bytecode();
    }

    uint32_t position() {
        return positions.top();
    }

    void position(uint32_t offset) {
        auto newVal = positions.top() += offset;
        positions.pop();
        positions.push(newVal);
    }

    void interpretBytecode();
    void incrementPosition(Instruction insn, bool jmp);

    void pushInt(const string& name, int64_t value);
    void pushDouble(const string& name, double value);
    void pushStr(const string& name, const string &value);

    void printVarOnTop();
    Context* getContext(uint16_t ctx);

public:
    InterpreterCode();
    void addContext(Context* context);
    size_t nextContext() {
        return contexts.size();
    }

    virtual Status* execute(vector<Var *> &vars) {
        Code::FunctionIterator it(this);
        lastContext.push(contexts);
        functions.push((BytecodeFunction *)it.next());
        positions.push(0);
        try {
            interpretBytecode();
        } catch (InterpreterException& err) {
            return Status::Error(err.what());
        }
        functions.pop();
        positions.pop();
        lastContext.pop();
        return Status::Ok();
    }

    ~InterpreterCode();

};

}

#endif // INTERPRETER_CODE

