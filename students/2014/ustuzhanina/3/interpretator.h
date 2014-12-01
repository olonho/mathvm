#ifndef INTERPRETATOR_H
#define INTERPRETATOR_H
#include "mathvm.h"
#include "context.h"
#include "utils.h"
#include <stack>
#include "exceptions.h"
using std::stack;
using namespace mathvm;
class InterpretCode : public Code
{
public:
    InterpretCode(): position(0),
        var0(VT_INT, "none"),
        var1(VT_INT, "none")
    {}
    virtual Status * execute(vector<Var *> & vars)
    {
        Code::FunctionIterator it(this);
        currenFunction = (BytecodeFunction *)it.next();
        contextStack.push(currenFunction->id());
        currentContext = createOrGetContext(1);
        addContextToMap(1, currentContext);

        //byteCode()->dump(cout);
        try
        {
            handleByteCode();
        }
        catch(Exception ex)
        {
            return Status::Error(ex.what());
        }

        return Status::Ok();
    }

    size_t getInstructionLength(Instruction instruction)
    {
        struct
        {
            Instruction insn;
            size_t length;
        } instructions[] =
        {
#define ENUM_ELEM_LEN(b, d, l) {BC_##b, l},
            FOR_BYTECODES(ENUM_ELEM_LEN)
        };
        return instructions[instruction].length;
    }
private:
    void handleByteCode();
    BytecodeFunction * currenFunction;
    Bytecode * byteCode()
    {
        return currenFunction->bytecode();
    }
    uint32_t position;
    stack <Var> variables;
    stack <uint32_t> contextStack;
    Context * currentContext;
    //create new context on every new call
    map <int16_t, vector<Context *> > contextMap;
    Var var0, var1;

private:
    VarType getType(Instruction instruction) const;
    Var createVar(Instruction type, const string & name);
    void pushDefaultVar(Instruction instruction);
    void loadVar(Instruction instruction);
    void saveVar(Instruction instruction);
    void printVariable(Var var);
    void swapValues();
    void handleTopValue(Instruction instruction);
    void handleAdditionVars(Instruction instruction);
    void jump(int16_t position);
    void cmpInstruction(Instruction instruction);
    void logicalOperation(Instruction instruction);
    Var getValueFromStack();
    void handleCallNode(Instruction instruction);
    void handleReturnNode();

private:
    Context * createOrGetContext(int16_t idx)
    {
        Context::VariableMap vars;
        Context::FunctionMapM funcs;
        Context * context = new Context(idx, vars, funcs, NULL);

        //create empty context if no one elems in Map
        if (contextMap.find(idx) == contextMap.end()){
            vector <Context *> contexts;
            contextMap.insert(make_pair(idx, contexts));
        }
            //return contextMap.at(idx).front();

        return context;
    }

    void addContextToMap(int16_t key, Context *newContext){
        contextMap.at(key).push_back(newContext);
    }

    Context * getContext(int16_t idx)
    {
        if (contextMap.find(idx) != contextMap.end())
            return contextMap.at(idx).back();
        else
            throw Exception("try to load unexistence context");
    }

    vector<Context *> getContextMap(int16_t idx)
    {
        if (contextMap.find(idx) != contextMap.end())
            return contextMap.at(idx);
        else
            throw Exception("try to load unexistence context Map");
    }

    Var getVariable(Context * context, int16_t idx)
    {
        if (context->variableMap.find(Utils::convertToString(idx)) != context->variableMap.end())
            return context->variableMap.at(Utils::convertToString(idx)).second;
        else
            throw Exception("try to load unexistence variable "
                            + Utils::convertToString(context->idx)  + " "
                            + Utils::convertToString(idx)
                            + Utils::convertToString(position));
    }

    template <typename T>
    void setValue(Var & var, T value)
    {
        switch (var.type())
        {
        case VT_INT:
            var.setIntValue(value);
            break;

        case VT_DOUBLE:
            var.setDoubleValue((double)value);
            break;

        default:
            break;
        }
    }

    template <typename T>
    T getValue(Var & var)
    {
        switch (var.type())
        {
        case VT_INT:
            return var.getIntValue();
            break;

        case VT_DOUBLE:
            return var.getDoubleValue();
            break;

        case VT_STRING:
            //            const char *val = var.getStringValue();
            //            return val;
            break;

        default:
            break;
        }

        throw Exception("variable can be int, double or string type, but try to handle another");
    }

    void createDefaultVar(Instruction instruction)
    {
        Var var(Utils::getType(instruction), "default");

        switch (instruction)
        {
        case BC_ILOAD0:
            setValue<int64_t>(var, 0);
            break;

        case BC_ILOAD1:
            setValue<int64_t>(var, 1);
            break;

        case BC_ILOADM1:
            setValue<int64_t>(var, -1);
            break;

        case BC_DLOAD0:
            setValue<double>(var, 0);
            break;

        case BC_DLOAD1:
            setValue<double>(var, 1);
            break;

        case BC_DLOADM1:
            setValue<double>(var, -1);
            break;

        case BC_SLOAD0:
            var.setStringValue("");
            break;

        default:
            break;
        }

        variables.push(var);
    }

    template <typename T>
    void mathOperation(Instruction instruction)
    {
        T value1 = getValue<T>(variables.top());
        Var result = variables.top();
        variables.pop();
        T value2 = getValue<T>(variables.top());
        variables.pop();

        switch (instruction)
        {
        case BC_DADD:
        case BC_IADD:
            setValue(result, value1 + value2);
            break;

        case BC_DSUB:
        case BC_ISUB:
            setValue(result, value1 - value2);
            break;

        case BC_DMUL:
        case BC_IMUL:
            setValue(result, value1 * value2);
            break;

        case BC_DDIV:
        case BC_IDIV:
            setValue(result, value1 / value2);
            break;

        case BC_IMOD:
            if (Utils::getType(instruction) == VT_INT)
                setValue<int64_t>(result, (int64_t)value1 % (int64_t)value2);
            break;

        default:
            break;
        }

        variables.push(result);
    }
    template <typename T>
    void compareVar(Instruction instruction)
    {
        Var result(Utils::getType(instruction), "cmpres");
        Var varUpper = variables.top();
        variables.pop();
        Var varLower = variables.top();
        variables.pop();
        T valueUpper  = getValue<T>(varUpper);
        T valueLower = getValue<T>(varLower);

        if (valueUpper > valueLower)
        {
            setValue<T>(result, 1);
            variables.push(result);
        }
        else if (valueUpper == valueLower)
        {
            setValue<T>(result, 0);
            variables.push(result);
        }
        else
        {
            setValue<T>(result, -1.0);
            variables.push(result);
        }
    }
};

#endif // INTERPRETATOR_H
