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
	// TODO Auto-generated destructor stub
}

void Interpreter::setLogger(ostream* log)
{
	_log = log;
}


void Interpreter::loadDouble() {
    StackVar var;
    var.d = _bc->getDouble(_insPtr);

    _insPtr += sizeof(double);

    _stack.push(var);
    _typeStack.push(VT_DOUBLE);

    _out << "loadDouble" << endl;
}

void Interpreter::loadInt() {
    StackVar var;
    var.i = _bc->getInt64(_insPtr);

    _insPtr += sizeof(uint64_t);

    _stack.push(var);
    _typeStack.push(VT_INT);

    _out << "loadInt" << endl;
}

void Interpreter::loadString() {
    StackVar var;
    var.strId = _bc->getUInt16(_insPtr);

    _insPtr += 2;

    _stack.push(var);
    _typeStack.push(VT_STRING);

    _out << "loadString" << endl;
}


void Interpreter::execute(Code* code)
{

    _out << "EXECUTING: START" << endl;

    _bc = ((BytecodeFunction*)code->functionById(0))->bytecode();
    _insPtr = 0;


    Instruction inst;
    while (true) {


    	inst = _bc->getInsn(_insPtr);
    	_out << "Current instruction " << inst << endl;


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

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
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

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_ISUB:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
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

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_DPRINT:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_SPRINT:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
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

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
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

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_LOADCTXIVAR:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_LOADCTXSVAR:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STORECTXDVAR:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STORECTXIVAR:

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
            	break;
        	case BC_STORECTXSVAR:

        		_out << inst << " Not implemented" << endl;
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

        		_out << inst << " Not implemented" << endl;
                exit(EXIT_FAILURE);
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

}
