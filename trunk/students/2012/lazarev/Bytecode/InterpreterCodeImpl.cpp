#include "InterperterCodeImpl.h"

static int commandLen[] = {
	#define ENUM_ELEM(b, d, l) l,
    	FOR_BYTECODES(ENUM_ELEM)
	#undef ENUM_ELEM
    1
}

Status* InterperterCodeImpl::execute(vector<Var*>& vars) {


}

void InterperterCodeImpl::runBytecode(Bytecode *bytecode) {
	int bci = 0;
	while (bci < (bytecode -> length())) {
		Instruction insn = bytecode -> getInsn(bci);

		switch (insn) {
			case DLOAD:
				push(Val(bytecode -> getDouble(bci + 1)));
				break;
			case ILOAD:
				push(Val(bytecode -> getInt64(bci + 1)));
				break;
			case SLOAD:
				push(Val(constantById(bytecode -> getUInt16(bci + 1))));
				break;

			case DLOAD0:
				push(Val(0.0));
				break;
			case ILOAD0:
				push(Val(0));
				break;
			case SLOAD0:
				push(Val(""));
				break;

			case DLOAD1:
				push(Val(1.0));
				break;
			case ILOAD1:
				push(Val(1));
				break;

			case DLOAD1:
				push(Val(-1.0));
				break;
			case ILOAD1:
				push(Val(-1));
				break;

			case DADD:
				double first = popDouble();
				double second = popDouble();
				push(Val(first + second));
				break;
			case IADD:
				int first = popInt();
				int second = popInt();
				push(Val(first + second));
				break;

			case DSUB:
				double first = popDouble();
				double second = popDouble();
				push(Val(first - second));
				break;
			case ISUB:
				int first = popInt();
				int second = popInt();
				push(Val(first - second));
				break;

			case DMUL:
				double first = popDouble();
				double second = popDouble();
				push(Val(first * second));
				break;
			case IMUL:
				int first = popInt();
				int second = popInt();
				push(Val(first * second));
				break;

			case DDIV:
				double first = popDouble();
				double second = popDouble();
				push(Val(first / second));
				break;
			case IDIV:
				int first = popInt();
				int second = popInt();
				push(Val(first / second));
				break;
			case IMOD:
				int first = popInt();
				int second = popInt();
				push(Val(first % second));
				break;

			case DNEG:
				double val = popDouble();
				push(Val(-val));
				break;
			case INEG:
				int val = popInt();
				push(Val(-val));
				break;

			case DPRINT:
				break;
			case IPRINT:
				break;
			case SPRINT:
				break;

			case I2D:
				int val = popInt();
				push(Val((double)val));
				break;
			case D2I:
				double val = popDouble();
				push(Val((int)val));
				break;

			case POP:
				programStack.pop();


			case DCMP:
				double first = popDouble();
				double second = popDouble();
				if (first == second) {
					push(Val(0));
				} else if (first > second) {
					push(Val(1));
				} else {
					push(Val(-1));
				}
				break;
			case ICMP:
				int first = popInt();
				int second = popInt();
				if (first == second) {
					push(Val(0));
				} else if (first > second) {
					push(Val(1));
				} else {
					push(Val(-1));
				}
				break;

			case JA:
				bci += (bytecode -> getInt16(bci + 1));
				break;
			case IFICMPNE:
				int first = popInt();
				int second = popInt();
				if (first != second) {
					bci += bytecode -> getInt16(bci + 1);
				}
				break;
			case IFICMPE:
				int first = popInt();
				int second = popInt();
				if (first == second) {
					bci += bytecode -> getInt16(bci + 1);
				}
				break;
			case IFICMPG:
				int first = popInt();
				int second = popInt();
				if (first > second) {
					bci += bytecode -> getInt16(bci + 1);
				}
				break;
			case IFICMPGE:
				int first = popInt();
				int second = popInt();
				if (first >= second) {
					bci += bytecode -> getInt16(bci + 1);
				}
				break;
			case IFICMPL:
				int first = popInt();
				int second = popInt();
				if (first < second) {
					bci += bytecode -> getInt16(bci + 1);
				}
				break;
			case IFICMPLE:
				int first = popInt();
				int second = popInt();
				if (first <= second) {
					bci += bytecode -> getInt16(bci + 1);
				}
				break;


		}

		bci += commandLen[insn];
	}
}


double InterperterCodeImpl::popDouble() {
	double res = programStack.top().getDouble();
	programStack.pop();
	return res;
}

int InterperterCodeImpl::popInt() {
	int res = programStack.top().getInt();
	programStack.pop();
	return res;
}

string InterperterCodeImpl::popString() {
	string res = programStack.top().getString();
	programStack.pop();
	return res;
}

void InterperterCodeImpl::push(Val& val) {
	programStack.push(val);
}




void InterperterCodeImpl::getBytecode() {
	
}