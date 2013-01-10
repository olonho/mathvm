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
    pushDouble(getNextDouble());
}

void Interpreter::loadInt() {
    pushInt(getNextInt());
}

void Interpreter::loadString() {
    StackVar var;
    var.strId = getNext2Bytes();

    pushVar(VT_STRING, var);
}

void Interpreter::printInt() {
	int64_t i = _stack.top().i;
	popVar();
	_out << i;
}

void Interpreter::printDouble() {
	double d = _stack.top().d;
    popVar();
	_out << d;
}

void Interpreter::printString() {
    uint16_t id = _stack.top().strId;
    popVar();
	_out << _code->constantById(id);
}


void Interpreter::storeCtxDouble() {
    FunctionContext* ctx = topContext();
    uint16_t ctxId = getNext2Bytes();
    uint16_t id = getNext2Bytes();
    StackVar var = _stack.top();
    popVar();
    ctx->storeDouble(ctxId, id, var.d);
}

void Interpreter::storeCtxInt() {
	FunctionContext* ctx = topContext();
	uint16_t ctxId = getNext2Bytes();
	uint16_t id = getNext2Bytes();
	StackVar var = _stack.top();
	popVar();
	ctx->storeInt(ctxId, id, var.i);
}

void Interpreter::pushInt(int64_t i)
{
    StackVar var;
    var.i = i;
    pushVar(VT_INT, var);
    //cout << "__PUSHED INT" << i << endl;
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
    pushVar(VT_DOUBLE, var);
    //cout << "__PUSHED DOUBLE" << d << endl;
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

    popVar();

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

void Interpreter::addInts() {
	int64_t top = popInt();
	int64_t down = popInt();
	int64_t sum = top + down;
	pushInt(sum);

	//cout << "adding ints" << endl;
}

void Interpreter::addDoubles() {
	pushDouble(popDouble() + popDouble());
}

/*
void Interpreter::addDoubles() {
	double top = popDouble();
	double down = popDouble();
	pushDouble(top + down);
}*/



void Interpreter::loadFunParamsInCtx(uint16_t id) {
    BytecodeFunction* fun = ((BytecodeFunction*)_code->functionById(id));
	uint16_t n = fun->parametersNumber();
    FunctionContext* ctx = this->topContext();

    //cout << "loading params: " << n << endl;
    //cout << "stack size " << _stack.size() << endl;


    for (uint16_t i = 0; i < n; ++i) {
        VarType type = fun->parameterType(i);
        switch (type) {
            case VT_INT:
                ctx->storeInt(ctx->getId(), i, popInt());
                //cout << "int loaded from bc" << endl;
                break;
            case VT_DOUBLE:
            	ctx->storeDouble(ctx->getId(), i, popDouble());
            	//cout << "double loaded from bc" << endl;
            	break;
            default:
            	//cout << "loading STRIG or INVALID from bc" << endl;
            	assert(false);
            	break;
        }
    }
}


void Interpreter::printStack() {
	stack<StackVar> s2(_stack);
	stack<VarType> t2(_typeStack);
	//cout << "__PRINTING STACK" << endl;

	while (s2.size() > 0) {
	    VarType t = t2.top();
	    t2.pop();

	    //StackVar v = s2.top();
	    s2.pop();

	    switch (t) {
	       case VT_INT:
	        	//cout << "__" << v.i << endl;
	        	break;
	        case VT_DOUBLE:
	        	//cout << "__" << v.d << endl;
	        	break;
	        case VT_STRING:
	            //cout << "__" << v.strId << endl;
	        	break;
	        default:
	        	//cout << "strange type" << endl;
	        	assert(false);
	        	break;
	    }
	}
}


void Interpreter::call() {
    uint16_t funId = getNext2Bytes();

    //cout << "BEFORE CALLING: " << endl;
    //printStack();

    pushContext(funId);
    loadFunParamsInCtx(funId);

    //printStack();

    uint32_t savedIp = _insPtr;
    Bytecode* savedBc = _bc;


    //uint32_t before = _stack.size();
    executeFun(funId);
    //uint32_t after = _stack.size();



    //cout << "__BEFORE_F_CALL " << before << endl;
    //cout << "__AFTER_F_CALL " << after << endl;

    popContext();

    _insPtr = savedIp;
    _bc = savedBc;
}


void Interpreter::execute(Code* code)
{
    //cout << "Executing.." << endl;

	_code = code;
    pushContext(0);
    executeFun(0);

    //cout << "Stack sizes: " << _stack.size() << " " << _typeStack.size() << endl;

}

void Interpreter::compareInts() {
    //cout << "Comparing ";

    int64_t top = popInt();
    int64_t down = popInt();

    //cout << down << " " << top << endl;

    int64_t result = (down > top) ? 1 : (down == top) ? 0 : -1;

    pushInt(result);
}


void Interpreter::intsEqualJMP() {
	int64_t top = popInt();
	int64_t down = popInt();

	int16_t offset = getNext2SBytes();
	if (top == down) {
        jump(offset);
	    //cout << "Jumping " << offset << endl;
	}
}

void Interpreter::intsNotEqualJMP() {
	int64_t top = popInt();
	int64_t down = popInt();

	int16_t offset = getNext2SBytes();
	if (top != down) {
        jump(offset);
	    //cout << "Jumping " << offset << endl;
	}
}

void Interpreter::jumpAlways() {
    int16_t offset = getNext2SBytes();
    jump(offset);
    //cout << "Jumping " << offset << endl;
}

void Interpreter::LEJump() {
	int64_t top = popInt();
	int64_t down = popInt();

	int16_t offset = getNext2SBytes();
	if (down <= top) {
	    jump(offset);
	    //cout << "Jumping " << offset << endl;
	}
}

/*
void Interpreter::LJump() {
	int64_t top = popInt();
	int64_t down = popInt();

	int16_t offset = getNext2SBytes();
	if (down < top) {
	    jump(offset);
	    cout << "Jumping " << offset << endl;
	}
}
*/

void Interpreter::GEJump() {
	int64_t top = popInt();
	int64_t down = popInt();

	int16_t offset = getNext2SBytes();
	if (down >= top) {
	    jump(offset);
	    //cout << "Jumping " << offset << endl;
	}
}

void Interpreter::GJump() {
	int64_t top = popInt();
	int64_t down = popInt();

	int16_t offset = getNext2SBytes();
	if (down > top) {
	    jump(offset);
	    //cout << "Jumping " << offset << endl;
	}
}


/*
void Interpreter::notEqualThenJump() {
	int64_t top = popInt();
	int64_t down = popInt();

	int16_t offset = getNext2SBytes();
	if (down != top) {
	    jump(offset);
	    cout << "Jumping " << offset << endl;
	}
}*/


void Interpreter::dMul() {
    double d1 = popDouble();
    double d2 = popDouble();

    pushDouble(d1*d2);
}

void Interpreter::iMul() {
	int64_t i1 = popInt();
	int64_t i2 = popInt();

	//cout << "Multiplying " << i1 << " " << i2 << endl;
    pushInt(i1*i2);
}

void Interpreter::divInts() {
	int64_t right = popInt();
	int64_t left = popInt();

	//cout << "Dividing " << left << " " << right << endl;
    pushInt(left / right);
}

void Interpreter::divDoubles() {
	double right = popDouble();
	double left = popDouble();

	//cout << "Dividing " << left << " " << right << endl;
    pushDouble(left / right);
}




void Interpreter::executeFun(uint16_t id)
{

	_bc = ((BytecodeFunction*)_code->functionById(id))->bytecode();
    _insPtr = 0;

	Instruction inst;
    while (true) {


    	inst = nextInsn();

    	//cout << "Current instruction " << inst << endl;

    	//cout << "__STACK:" << endl;
        printStack();


        switch (inst) {

            case BC_INVALID:
                _out << inst << " Invalid instruction" << endl;
                assert(false);
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
        		assert(false);
        		break;
        	case BC_ILOAD0:
            	pushInt(0);
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
                pushInt(1);
            	break;
        	case BC_DLOADM1:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_ILOADM1:
        		pushInt(-1);
        		break;
        	case BC_DADD:
        		addDoubles();
        		break;
        	case BC_IADD:
                addInts();
            	break;
        	case BC_DSUB:

        		subDoubles();
        		break;
        	case BC_ISUB:
                subInts();
        		break;
        	case BC_DMUL:
                dMul();
        		break;
        	case BC_IMUL:
                iMul();
        		break;
        	case BC_DDIV:
        		divDoubles();
        		break;
        	case BC_IDIV:
        		divInts();
        		break;
        	case BC_IMOD:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_DNEG:
        		dNeg();
        		break;
        	case BC_INEG:
        		iNeg();
        		break;
        	case BC_IPRINT:
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
                compareInts();
        		break;
        	case BC_JA:
                jumpAlways();
        		break;
        	case BC_IFICMPNE:
        		intsNotEqualJMP();
        		break;
        	case BC_IFICMPE:
                intsEqualJMP();
        		break;
        	case BC_IFICMPG:
        		GJump();
        		break;
        	case BC_IFICMPGE:
        		GEJump();
        		break;
        	case BC_IFICMPL:
        		//cout << "IFLESS" << endl;
                assert(false);
        		break;
        	case BC_IFICMPLE:
        	    LEJump();
        	    break;
        	case BC_DUMP:
        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STOP:
        		this->popContext();
        		return;
        	case BC_CALL:
        		//cout << "CALLING. STACK SIZE: " << _stack.size() << endl;
        		call();
        		break;
        	case BC_CALLNATIVE:
        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_RETURN:
        		//cout << "RETURNING ";
        		if (_typeStack.size() > 0)
        		{
        			switch (_typeStack.top()) {
        		    	case VT_INT:
        		    		//cout << _stack.top().i ;
        		    		break;
        		    	case VT_DOUBLE:
        		    		//cout << _stack.top().d ;
        		    		break;
        		    	default:
        		    		assert(false);
        		    		break;
        			}
        		}
        		//cout << "STACK SIZE: " << _stack.size() << endl;
        		return;
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

void Interpreter::jump(int16_t offset) {
    _insPtr += offset;
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

int16_t Interpreter::getNext2SBytes() {
    int16_t id = _bc->getInt16(_insPtr);
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
		//cout << "Stored double in current context: " << _id << endl;
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
	    //cout << "Stored int in current context: " << _id  << endl;
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
