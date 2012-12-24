/*
 * Interpreter.cpp
 *
 *  Created on: Dec 17, 2012
 *      Author: yarik
 */

#include "Interpreter.h"

Interpreter::Interpreter(ostream& out)
    : _out(out)
{
}

Interpreter::~Interpreter() {

}


void Interpreter::loadDouble() {
    StackVar var;
    var.d = getNextDouble();

    _stack.push(var);
    _typeStack.push(VT_DOUBLE);
}

void Interpreter::loadInt() {
    StackVar var;
    var.i = getNextInt();

    _stack.push(var);
    _typeStack.push(VT_INT);
}

void Interpreter::loadString() {
    StackVar var;
    var.strId = getNext2Bytes();

    _stack.push(var);
    _typeStack.push(VT_STRING);
}

void Interpreter::printInt() {
	int64_t i = _stack.top().i;

	_stack.pop();
	_typeStack.pop();

	_out << i;
}

void Interpreter::printDouble() {
	double d = _stack.top().d;

	_stack.pop();
	_typeStack.pop();

	_out << d;
}

void Interpreter::printString() {
    uint16_t id = _stack.top().strId;

    _stack.pop();
	_typeStack.pop();

	_out << _code->constantById(id);
}


void Interpreter::storeCtxDouble() {
    FunctionContext* ctx = topContext();
    uint16_t ctxId = getNext2Bytes();
    uint16_t id = getNext2Bytes();

    StackVar var = _stack.top();
    _stack.pop();
    _typeStack.pop();

    //cout << "CTX: " << ctxId << " ID: " << id << endl;
    ctx->storeDouble(ctxId, id, var.d);
}

void Interpreter::storeCtxInt() {
	FunctionContext* ctx = topContext();
	uint16_t ctxId = getNext2Bytes();
	uint16_t id = getNext2Bytes();

	StackVar var = _stack.top();
	_stack.pop();
	_typeStack.pop();

	//cout << "CTX: " << ctxId << " ID: " << id << endl;
	ctx->storeInt(ctxId, id, var.i);
}

void Interpreter::pushInt(int64_t i)
{
    StackVar var;
    var.i = i;

    _stack.push(var);
    _typeStack.push(VT_INT);
}

int64_t Interpreter::popInt() {
    checkTypesTOS(VT_INT);
    int64_t i = _stack.top().i;
    popVar();
    return i;
}

double Interpreter::popDouble() {
	checkTypesTOS(VT_DOUBLE);
	double d = _stack.top().d;
	popVar();
	return d;
}

void Interpreter::pushDouble(double d)
{
	StackVar var;
    var.d = d;

    _stack.push(var);
    _typeStack.push(VT_DOUBLE);
}


void Interpreter::loadCtxInt() {
	FunctionContext* ctx = topContext();
    uint16_t ctxId = getNext2Bytes();
	uint16_t id = getNext2Bytes();

	pushInt(ctx->readInt(ctxId, id));
}

void Interpreter::loadCtxDouble() {
	FunctionContext* ctx = topContext();
	uint16_t ctxId = getNext2Bytes();
	uint16_t id = getNext2Bytes();

	pushDouble(ctx->readDouble(ctxId, id));
}

void Interpreter::swap() {
    StackVar upVar = _stack.top();
    VarType upType = _typeStack.top();

    popVar();

    StackVar downVar = _stack.top();
    VarType downType = _typeStack.top();

    pushVar(upType, upVar);
    pushVar(downType, downVar);
}

void Interpreter::subInts() {
    int64_t top = popInt();
    int64_t down = popInt();
    int64_t sub = top - down;

    pushInt(sub);
}

void Interpreter::subDoubles() {
	double top = popDouble();
	double down = popDouble();

	pushDouble(top - down);
}


void Interpreter::loadFunParamsInCtx(BytecodeFunction* fun) {
    uint16_t n = fun->parametersNumber();
    FunctionContext* ctx = this->topContext();
    for (uint16_t i = 0; i < n; ++i) {
        VarType type = fun->parameterType(i);
        switch (type) {
            case VT_INT:
                ctx->storeInt(ctx->getId(), i, getNextInt());
                break;
            case VT_DOUBLE:
            	ctx->storeDouble(ctx->getId(), i, getNextDouble());
            	break;
            default:
            	assert(false);
            	break;
        }

    }
}


void Interpreter::call() {
    uint16_t funId = getNext2Bytes();

    this->pushContext(funId);


    BytecodeFunction* fun = (BytecodeFunction*) _code->functionById(funId);
    loadFunParamsInCtx(fun);
    uint32_t savedIp = _insPtr;

    assert(false);

    this->popContext();
    _insPtr = savedIp;

}

void Interpreter::execute(Code* code)
{
	//cout << "\nlaunching..." << endl << endl;

	_code = code;

	uint16_t top = 0;
    _bc = ((BytecodeFunction*)code->functionById(top))->bytecode();
    _insPtr = 0;

    pushContext(top);

    Instruction inst;
    while (true) {


    	inst = nextInsn();

    	//cout << "Current instruction " << inst << endl;



        switch (inst) {

            case BC_INVALID:
                _out << inst << " Invalid instruction" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_DLOAD:
        		loadDouble();
        		break;
        	case BC_ILOAD:
                loadInt();
        		break;
        	case BC_SLOAD:
                loadString();
        		break;
        	case BC_DLOAD0:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_ILOAD0:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_SLOAD0:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_DLOAD1:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_ILOAD1:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_DLOADM1:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_ILOADM1:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_DADD:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_IADD:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_DSUB:

        		subDoubles();
        		break;
        	case BC_ISUB:
                subInts();
        		break;
        	case BC_DMUL:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_IMUL:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_DDIV:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_IDIV:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_IMOD:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_DNEG:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_INEG:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_IPRINT:

        		//_out << inst << " Not implemented" << endl;
        		//exit(EXIT_FAILURE);

        		printInt();
        		break;
        	case BC_DPRINT:
                printDouble();
        		break;
        	case BC_SPRINT:
        		printString();
        		break;
        	case BC_I2D:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_D2I:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_S2I:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_SWAP:
                swap();
        		break;
        	case BC_POP:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_LOADDVAR0:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_LOADDVAR1:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_LOADDVAR2:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_LOADDVAR3:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_LOADIVAR0:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_LOADIVAR1:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_LOADIVAR2:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_LOADIVAR3:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_LOADSVAR0:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_LOADSVAR1:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_LOADSVAR2:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_LOADSVAR3:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STOREDVAR0:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STOREDVAR1:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STOREDVAR2:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STOREDVAR3:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STOREIVAR0:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STOREIVAR1:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STOREIVAR2:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STOREIVAR3:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STORESVAR0:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STORESVAR1:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STORESVAR2:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STORESVAR3:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_LOADDVAR:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_LOADIVAR:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_LOADSVAR:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STOREDVAR:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STOREIVAR:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STORESVAR:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_LOADCTXDVAR:
                loadCtxDouble();
        		break;
        	case BC_LOADCTXIVAR:
                loadCtxInt();
        		break;
        	case BC_LOADCTXSVAR:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STORECTXDVAR:
        		storeCtxDouble();
            	break;
        	case BC_STORECTXIVAR:
                storeCtxInt();
        		break;
        	case BC_STORECTXSVAR:
        		_out << inst << " STORE STRING ? Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_DCMP:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_ICMP:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_JA:

        	    _out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_IFICMPNE:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_IFICMPE:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_IFICMPG:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_IFICMPGE:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_IFICMPL:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_IFICMPLE:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_DUMP:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STOP:
        		return;
        	case BC_CALL:
        		call();
        		break;
        	case BC_CALLNATIVE:
        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_RETURN:
        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_BREAK:
        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	default:
                _out << "Bad instruction" << endl;
         		break;
        }




    }

    this->popContext();
}


void Interpreter::movePtr2Bytes() {
    _insPtr += 2;
}



void Interpreter::movePtrInsnDown() {
    _insPtr += BytecodeConstants::insn_size;
}

void Interpreter::movePtrIntDown() {
    _insPtr += BytecodeConstants::int_size;
}

void Interpreter::movePtrDoubleDown() {
    _insPtr += BytecodeConstants::double_size;
}


Instruction Interpreter::nextInsn() {
 	Instruction insn = _bc->getInsn(_insPtr);
    movePtrInsnDown();
    return insn;
}

double Interpreter::getNextDouble() {
    double d = _bc->getDouble(_insPtr);
    movePtrDoubleDown();
    return d;
}

int64_t Interpreter::getNextInt() {
    int64_t i = _bc->getInt64(_insPtr);
    movePtrIntDown();
    return i;
}

uint16_t Interpreter::getNext2Bytes() {
    uint16_t id = _bc->getUInt16(_insPtr);
    movePtr2Bytes();
    return id;
}





FunctionContext::FunctionContext(uint16_t functionId, FunctionContext* parent)
	: _id(functionId)
	, _parent(parent)
{
}

int64_t FunctionContext::readInt(uint16_t context, uint16_t id) {
    if (context == _id)
        return readIntFromCurrCTX(id);
    else if (_parent)
        return _parent->readInt(context, id);
    assert(false);
}


double FunctionContext::readDouble(uint16_t context, uint16_t id) {
    if (context == _id)
        return readDoubleFromCurrCTX(id);
    else if (_parent)
        return _parent->readDouble(context, id);
    assert(false);
}

void FunctionContext::storeDouble(uint16_t context, uint16_t id, double value) {
	if (context == _id)
	{
		storeDoubleInCurrCTX(id, value);
		//cout << "Stored double in current" << endl;
		return;
	}
    else if (_parent) {
	    _parent->storeDouble(context, id, value);
	    return;
	}
	assert(false);
}

void FunctionContext::storeInt(uint16_t context, uint16_t id, int64_t value) {
	if (context == _id)
	{
	    storeIntInCurrCTX(id, value);
	    //cout << "Stored int in current" << endl;
	    return;
	}
	else if (_parent)
	{
	    _parent->storeInt(context, id, value);
	    return;
	}
	assert(false);
}

int64_t FunctionContext::readIntFromCurrCTX(uint16_t id) {
    return vars[id].i;
}

double FunctionContext::readDoubleFromCurrCTX(uint16_t id) {
    return vars[id].d;
}

void FunctionContext::storeDoubleInCurrCTX(uint16_t id, double value) {
    StackVar var;
    var.d = value;
  	vars[id] = var;
}

void FunctionContext::storeIntInCurrCTX(uint16_t id, int64_t value) {
   StackVar var;
   var.i = value;
   vars[id] = var;
}

uint16_t FunctionContext::getId() {
	return _id;
}
