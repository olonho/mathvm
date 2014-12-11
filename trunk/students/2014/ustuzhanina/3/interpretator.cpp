#include "interpretator.h"
void InterpretCode::handleByteCode()
{
	Instruction instruction;

	do
	{
		instruction = byteCode()->getInsn(position);
		position += getInstructionLength(instruction);
        //std::cout << "instr = " << (Instruction)instruction << std::endl;

		switch (instruction)
		{
		case BC_ILOAD:
		case BC_DLOAD:
		case BC_SLOAD:
			createVar(instruction, "constant");
			break;

		case BC_LOADCTXDVAR:
		case BC_LOADCTXIVAR:
			loadVar(instruction);
			break;

		case BC_STORECTXDVAR:
		case BC_STORECTXIVAR:
			saveVar(instruction);
			break;

		case BC_IPRINT:
		case BC_DPRINT:
		case BC_SPRINT:
			printVariable(variables.top());
			variables.pop();
			break;

		case BC_ILOAD0:
		case BC_ILOAD1:
		case BC_ILOADM1:
		case BC_DLOAD0:
		case BC_DLOAD1:
		case BC_DLOADM1:
		case BC_SLOAD0:
			createDefaultVar(instruction);
			break;

		case BC_DADD:
		case BC_DSUB:
		case BC_DMUL:
		case BC_DDIV:
			mathOperation<double>(instruction);
			break;

		case BC_IADD:
		case BC_ISUB:
		case BC_IMUL:
		case BC_IDIV:
		case BC_IMOD:
			mathOperation<int64_t>(instruction);
			break;

		case BC_SWAP:
			swapValues();
			break;

		case BC_ICMP:
			compareVar<int64_t>(instruction);
			break;

		case BC_DCMP:
			compareVar<double>(instruction);
			break;

		case BC_JA:
			jump(byteCode()->getInt16(position - 2));
			break;

		case BC_IFICMPE:
		case BC_IFICMPG:
			cmpInstruction(instruction);
			break;

		case BC_DNEG:
		case BC_INEG:
		case BC_D2I:
		case BC_I2D:
			handleTopValue(instruction);
			break;

		case BC_IAAND:
		case BC_IAOR:
		case BC_IAXOR:
			logicalOperation(instruction);
			break;

		case BC_STOP:
			break;

		case BC_LOADIVAR0:
		case BC_LOADIVAR1:
		case BC_STOREIVAR0:
		case BC_STOREIVAR1:
			handleAdditionVars(instruction);
			break;

		case BC_CALL:
			handleCallNode(instruction);
			break;

		case BC_RETURN:
			handleReturnNode();
			break;

		default:
			cout << "no such instruction = " << (Instruction)instruction << " " << position << endl;
			return;
			break;
		}
	}
	while (instruction != BC_STOP);
}
void InterpretCode::jump(int16_t offset)
{
	// -3 + 1 because 3 -instructionLength & we have added already len to position
	position += ((int32_t)offset - 3 + 1);
}
Var InterpretCode::getValueFromStack()
{
	assert(variables.size() > 0);
	Var var = variables.top();
	variables.pop();
	return var;
}

void InterpretCode::logicalOperation(Instruction instruction)
{
	Var var1 = getValueFromStack();
	Var var2 = getValueFromStack();
	Var result(VT_INT, "arlogical");

	switch (instruction)
	{
	case BC_IAAND:
		result.setIntValue(var1.getIntValue()&var2.getIntValue());
		break;

	case BC_IAOR:
		result.setIntValue(var1.getIntValue() | var2.getIntValue());
		break;

	case BC_IAXOR:
		result.setIntValue(var1.getIntValue()^var2.getIntValue());
		break;

	default:
		break;
	}

	variables.push(result);
}
void InterpretCode::cmpInstruction(Instruction instruction)
{
	Var var1 = getValueFromStack();
	Var var2 = getValueFromStack();

	switch (instruction)
	{
	case BC_IFICMPE:
		if (getValue<int64_t>(var1) == getValue<int64_t>(var2))
			jump(byteCode()->getInt16(position - 2));

		break;

	case BC_IFICMPG:
		if (getValue<int64_t>(var1) > getValue<int64_t>(var2))
			jump(byteCode()->getInt16(position - 2));

		break;

	default:
		break;
	}
}
void InterpretCode::loadVar(Instruction instruction)
{
    uint16_t idxContext = byteCode()->getInt16(position - 4);
    uint16_t idxVar = byteCode()->getInt16(position - 2);
	Var var = getVariable(getContext(idxContext), idxVar);
	variables.push(var);
}

void InterpretCode::saveVar(Instruction instruction)
{
    uint16_t idxContext = byteCode()->getInt16(position - 4);
    uint16_t idxVar = byteCode()->getInt16(position - 2);
	Context * context = getContext(idxContext);
	Var var = variables.top();
	variables.pop();

	if (context->varMap_idx.find(idxVar) != context->varMap_idx.end())
		context->varMap_idx.at(idxVar) = var;
	else
		context->varMap_idx.insert(std::make_pair(idxVar, var));
}

void InterpretCode::handleTopValue(Instruction instruction)
{
	Var doubleV(VT_DOUBLE, "double");
	Var intV(VT_INT, "int");
	Var var = variables.top();
	variables.pop();

	switch (instruction)
	{
	case BC_I2D:
		doubleV.setDoubleValue((double)getValue<double>(var));
		variables.push(doubleV);
		break;

	case BC_D2I:
		intV.setIntValue((int)getValue<int64_t>(var));
		variables.push(intV);
		break;

	case BC_DNEG:
		if (var.getDoubleValue() == 0.0)
			doubleV.setDoubleValue(1.0);
		else
			doubleV.setDoubleValue(0.0);

		variables.push(doubleV);
		break;

	case BC_INEG:
		if (var.getIntValue() == 0)
			intV.setIntValue(1);
		else
			intV.setIntValue(0);

		variables.push(intV);
		break;

	default:
		break;
	}
}
void InterpretCode::swapValues()
{
	if (variables.size() < 2)
		assert(variables.size() > 2);

	Var var1 = getValueFromStack();
	Var var2 = getValueFromStack();
	variables.push(var1);
	variables.push(var2);
}
Var InterpretCode::createVar(Instruction instr, const string & name)
{
	Var var(getType(instr), name);

	switch (var.type())
	{
	case VT_INT:
		var.setIntValue(byteCode()->getInt64(position - 8));
		break;

	case VT_DOUBLE:
		var.setDoubleValue(byteCode()->getDouble(position - 8));
		break;

	case VT_STRING:
		var.setStringValue(constantById(byteCode()->getInt16(position - 2)).c_str());
		break;

	default:
		break;
	}

	variables.push(var);
	return var;
}

void InterpretCode::printVariable(Var var)
{
	switch (var.type())
	{
	case VT_INT:
		cout << var.getIntValue();
		break;

	case VT_DOUBLE:
		cout << var.getDoubleValue();
		break;

	case VT_STRING:
		cout << var.getStringValue();
		break;

	default:
		break;
	}
}

void InterpretCode::handleAdditionVars(Instruction instruction)
{
	switch (instruction)
	{
	case BC_STOREIVAR0:
		var0 = variables.top();
		variables.pop();
		break;

	case BC_STOREIVAR1:
		var1 = variables.top();
		variables.pop();
		break;

	case BC_LOADIVAR0:
		variables.push(var0);
		break;

	case BC_LOADIVAR1:
		variables.push(var1);
		break;

	default:
		break;
	}
}

void InterpretCode::handleCallNode(Instruction instruction)
{
    uint16_t funcIdx = byteCode()->getInt16(position - 2);
	contextStack.push(currenFunction->id());
	contextStack.push(position);
	position = 0;
	currenFunction = (BytecodeFunction *) functionById(funcIdx);
	Context * nContext = createOrGetContext(funcIdx + 1);
	contextMap.at(funcIdx + 1).push_back(nContext);
}

void InterpretCode::handleReturnNode()
{
	Context * context = contextMap.at(currenFunction->id() + 1).back();
	contextMap.at(currenFunction->id() + 1).pop_back();
	delete context;
	//return to position
	position = contextStack.top();
	contextStack.pop();
	//return to function
	currenFunction = (BytecodeFunction *) functionById(contextStack.top());
	contextStack.pop();
}
