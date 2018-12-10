#ifndef INTERPRETER_CODE_H
#define INTERPRETER_CODE_H
#include <mathvm.h>
#include <stack>


namespace mathvm {

    class StackValue {
        VarType type;

        union {
            int64_t vt_int;
            double vt_double;
            const char* vt_string;
        };
    public:
        
        StackValue() {
            type = VT_VOID;
        }

        StackValue(int64_t i) {
            type = VT_INT;
            vt_int = i;
        }

        StackValue(double d) {
            type = VT_DOUBLE;
            vt_double = d;
        }

        StackValue(const char* s) {
            type = VT_STRING;
            vt_string = s;
        }
        
        VarType getType() {
            return type;
        }

        int64_t getInt() {
            if (type == VT_INT) {
                return vt_int;
            } else {
                throw std::runtime_error("Interpreter error: want int, has another");
            }
        }

        double getDouble() {
            if (type == VT_DOUBLE) {
                return vt_double;
            } else {
                throw std::runtime_error("Interpreter error: want double, has another");
            }

        }

        string getString() {
            if (type == VT_STRING) {
                return vt_string;
            } else {
                throw std::runtime_error("Interpreter error: want string, has another");
            }
        }
    };

    class CodeImpl : public Code {
    public:
        CodeImpl();
        Status* execute(vector<Var*> &vars);

    };

    class ScopeInterpreter {
        vector<StackValue> vars;
        ScopeInterpreter* parent;
        vector<ScopeInterpreter*> children;
        
        uint16_t varsNumber;
        uint16_t id;

    public:
        ScopeInterpreter(ScopeInterpreter* p, uint16_t varsNumber, uint16_t id);
        StackValue lookupVariable(uint16_t id, bool useParent = true);
        void addChild(ScopeInterpreter* child);
        StackValue& varById(uint16_t id);
        ScopeInterpreter* getScopeById(uint16_t id);
        uint16_t getId();

    };

    class Interpreter {
        ScopeInterpreter* currentScope;
        stack<StackValue> operandStack;
        //vector<ScopeInterpreter*> scopes;

        Bytecode* bytecode;
        CodeImpl* code;
        void pushD(double local);
        void pushI(int64_t local);
        void pushS(const char* local);
        uint16_t jumpOffset(Bytecode* bytecode, size_t currentPos);

    public:
        Interpreter(Bytecode* bytecode, CodeImpl* code);

        void interpretFunction(BytecodeFunction* func);
        void interpretMainFunction(BytecodeFunction* func);

    };

}
#endif /* INTERPRETER_CODE_H */

