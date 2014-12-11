#ifndef INTERPRETATOR_H
#define INTERPRETATOR_H
#include "mathvm.h"
#include "context.h"

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

	~InterpretCode()
	{
		for (auto it = contextMap.begin(); it != contextMap.end(); ++it)
		{
			auto val = it->second;

			for (int i = 0; i != val.size(); ++i)
				delete val[i];
		}

		//delete currentContext;
	}
	virtual Status * execute(vector<Var *> & vars)
	{
		Code::FunctionIterator it(this);
		currenFunction = (BytecodeFunction *)it.next();
		contextStack.push(currenFunction->id());
		currentContext = createOrGetContext(1);
		addContextToMap(1, currentContext);
		initMap();

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
    map <uint16_t, vector<Context *> > contextMap;
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
    Context * createOrGetContext(uint16_t idx)
	{
		Context::VariableMap_idx vars;
		Context::FunctionMapM funcs;
		Context * context = new Context(idx, vars, funcs, NULL);

		//create empty context if no one elems in Map
		if (contextMap.find(idx) == contextMap.end())
		{
			vector <Context *> contexts;
			contextMap.insert(make_pair(idx, contexts));
		}

		//return contextMap.at(idx).front();
		return context;
	}

    void addContextToMap(uint16_t key, Context * newContext)
	{
		contextMap.at(key).push_back(newContext);
	}

    Context * getContext(uint16_t idx)
	{
		if (contextMap.find(idx) != contextMap.end())
			return contextMap.at(idx).back();
		else
			throw Exception("try to load unexistence context");
	}

    vector<Context *> getContextMap(uint16_t idx)
	{
		if (contextMap.find(idx) != contextMap.end())
			return contextMap.at(idx);
		else
			throw Exception("try to load unexistence context Map");
	}

    Var getVariable(Context * context, uint16_t idx)
	{
		if (context->varMap_idx.find(idx) != context->varMap_idx.end())
			return context->varMap_idx.at(idx);
		else
			throw Exception("try to load unexistence variable ");
	}

	template <typename T>
	void setValue(Var & var, T value)
	{
		VarType type = var.type();

		if (type == VT_INT)
			var.setIntValue(value);
		else
			var.setDoubleValue((double)value);
	}

	template <typename T>
	T getValue(Var & var)
	{
		VarType type = var.type();

		if (type == VT_INT)
			return var.getIntValue();
		else
			return var.getDoubleValue();

		throw Exception("variable can be int, double or string type, but try to handle another");
	}

	void createDefaultVar(Instruction instruction)
	{
		Var var(getType(instruction), "default");

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
            setValue(result, value2 / value1);
			break;

		case BC_IMOD:
			if (getType(instruction) == VT_INT)
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
		Var result(getType(instruction), "cmpres");
		Var varUpper = getValueFromStack();
		Var varLower = getValueFromStack();
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

	std::map <Instruction, VarType> typeMap;
	void initMap()
	{
		typeMap.insert(std::make_pair(BC_ILOAD, VT_INT));
		typeMap.insert(std::make_pair(BC_ILOAD0, VT_INT));
		typeMap.insert(std::make_pair(BC_ILOAD1, VT_INT));
		typeMap.insert(std::make_pair(BC_ILOADM1, VT_INT));
		typeMap.insert(std::make_pair(BC_IADD, VT_INT));
		typeMap.insert(std::make_pair(BC_ISUB, VT_INT));
		typeMap.insert(std::make_pair(BC_IMUL, VT_INT));
		typeMap.insert(std::make_pair(BC_IDIV, VT_INT));
		typeMap.insert(std::make_pair(BC_IMOD, VT_INT));
		typeMap.insert(std::make_pair(BC_INEG, VT_INT));
		typeMap.insert(std::make_pair(BC_IAOR, VT_INT));
		typeMap.insert(std::make_pair(BC_IAAND, VT_INT));
		typeMap.insert(std::make_pair(BC_IAXOR, VT_INT));
		typeMap.insert(std::make_pair(BC_I2D, VT_INT));
		typeMap.insert(std::make_pair(BC_ICMP, VT_INT));
		typeMap.insert(std::make_pair(BC_LOADCTXIVAR, VT_INT));
		typeMap.insert(std::make_pair(BC_STORECTXIVAR, VT_INT));
		typeMap.insert(std::make_pair(BC_DLOAD, VT_DOUBLE));
		typeMap.insert(std::make_pair(BC_DLOAD0, VT_DOUBLE));
		typeMap.insert(std::make_pair(BC_DLOAD1, VT_DOUBLE));
		typeMap.insert(std::make_pair(BC_DLOADM1, VT_DOUBLE));
		typeMap.insert(std::make_pair(BC_DADD, VT_DOUBLE));
		typeMap.insert(std::make_pair(BC_DSUB, VT_DOUBLE));
		typeMap.insert(std::make_pair(BC_DMUL, VT_DOUBLE));
		typeMap.insert(std::make_pair(BC_DDIV, VT_DOUBLE));
		typeMap.insert(std::make_pair(BC_DNEG, VT_DOUBLE));
		typeMap.insert(std::make_pair(BC_D2I, VT_INT));
		typeMap.insert(std::make_pair(BC_DCMP, VT_INT));
		typeMap.insert(std::make_pair(BC_LOADCTXDVAR, VT_INT));
		typeMap.insert(std::make_pair(BC_STORECTXDVAR, VT_INT));
		typeMap.insert(std::make_pair(BC_SLOAD, VT_STRING));
		typeMap.insert(std::make_pair(BC_SLOAD0, VT_STRING));
	}

	VarType getType(Instruction instruction)
	{
		return typeMap[instruction];
	}
};

#endif // INTERPRETATOR_H
