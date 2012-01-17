#include "mathvm.h"
#include "ast.h"
#include <vector>
#include <sstream>
#include "myinterpreter.h"
using namespace std;
using namespace mathvm;

Status* GeneratedCode :: execute(std :: vector <Var* > & vars) {
	createFrame(0);
	while(true) {
		Instruction insn = getNextInstruction();
		switch(insn) {

		case BC_DLOAD :
			add(nextDouble());
			break; 
		case BC_ILOAD :
			add(nextInt());
			break; 
		case BC_SLOAD :
			addString(nextInt16());
			break; 
		case BC_DLOAD0 :
			add((double)0);
			break; 
		case BC_ILOAD0 :
			add((int64_t)0);
			break; 
		case BC_DLOAD1 :
			add((double)1);
			break; 
		case BC_ILOAD1 :
			add((int64_t)1);
			break; 
		case BC_DLOADM1 :
			add((double)-1);
			break; 
		case BC_ILOADM1 :
			add((int64_t)-1);
			break; 


		case BC_DADD :
			{
				double result = getDouble() + getDouble();
				add(result);
			}
			break; 
		case BC_IADD :
			{
				int64_t result = getInt() + getInt();
				add(result);
			}
			break; 
		case BC_DSUB :
			{
				double a = getDouble(); 
				double result = a - getDouble()  ;
				add(result);
			}
			break; 
		case BC_ISUB :
			{
				int64_t a = getInt();
				int64_t result = a - getInt();
				add(result);
			}
			break; 
		case BC_DMUL :
			{
				double result = getDouble() * getDouble();
				add(result);
			}
			break; 
		case BC_IMUL :
			{
				int64_t result = getInt() * getInt();
				add(result);
			}
			break; 
		case BC_DDIV :
			{
				double a = getDouble(); 
				double result = a / getDouble()  ;
				add(result);
			}
			break; 
		case BC_IDIV :
			{
				int64_t a = getInt();
				int64_t result = a / getInt();
				add(result);
			}
			break; 
		case BC_DNEG :
			{
				double result = getDouble();
				add(-result);
			}
			break; 
		case BC_INEG :
			{	
				int64_t result = getInt();
				add(-result);
			}
			break; 
		case BC_IPRINT :
			print(getInt());
			break; 
		case BC_DPRINT :
			print(getDouble());
			break; 
		case BC_SPRINT :
			print(getString());
			break; 
		case BC_I2D :
			{
				uint16_t id = nextInt16();
				currentFrame->variables[id].double_part = currentFrame->variables[id].int_part;
			}
			break; 
		case BC_D2I :
			{
				uint16_t id = nextInt16();
				currentFrame->variables[id].int_part = currentFrame->variables[id].double_part;
			}
			break; 
		case BC_SWAP :
			{
				StackVariable a = getVar();
				StackVariable b = getVar();
				add(b);
				add(a);
			}
			break; 

        case BC_LOADDVAR :
		case BC_LOADIVAR :
		case BC_LOADSVAR :
			{
				uint16_t id = nextInt16();
				add(currentFrame->variables[id]);
			}
			break; 

        case BC_STOREDVAR :
			{
				uint16_t id = nextInt16();
				currentFrame->variables[id].double_part = getDouble();
			}
			break; 
        case BC_STOREIVAR :
			{
				uint16_t id = nextInt16();
				currentFrame->variables[id].int_part = getInt();
			}
			break; 
        case BC_STORESVAR :
			{
				uint16_t id = nextInt16();
				currentFrame->variables[id].string_part = getString();
			}
			break; 
        case BC_STORECTXDVAR :
			{
				uint16_t fid = nextInt16();
				uint16_t id = nextInt16();
				loadFrame(fid)->variables[id].double_part = getDouble();
			}
			break; 
        case BC_STORECTXIVAR :
			{
				uint16_t fid = nextInt16();
				uint16_t id = nextInt16();
				loadFrame(fid)->variables[id].int_part = getInt();
			}
			break; 

        case BC_STORECTXSVAR :
			{
				uint16_t fid = nextInt16();
				uint16_t id = nextInt16();
				loadFrame(fid)->variables[id].string_part = getString();
			}
			break; 

        case BC_LOADCTXDVAR :
		case BC_LOADCTXIVAR :
		case BC_LOADCTXSVAR :
			{
				uint16_t fid = nextInt16();
				uint16_t id = nextInt16();
				add(loadFrame(fid)->variables[id]);
			}
			break; 
        
		case BC_DCMP :
			{
				double a = getDouble();
				double b = getDouble();
				int64_t result = (a == b) ? 0 : (a > b) ? -1 : 1;
				add(result);	
			}
			break; 
        
		case BC_ICMP :
			{
				int64_t a = getInt();
				int64_t b = getInt();
				int64_t result = (a == b) ? 0 : (a > b) ? -1 : 1;
				add(result);	
			}
			break; 
        case BC_JA :			
			{
				int16_t jumpTo = nextInt16();
				jump(jumpTo);
			}
			break; 
        case BC_IFICMPNE :
			{
				int64_t a = getInt();
				int64_t b = getInt();
				int16_t jumpTo = nextInt16();
				if(a != b) jump(jumpTo);
			}
			break; 
        case BC_IFICMPE :
			{
				int64_t a = getInt();
				int64_t b = getInt();
				int16_t jumpTo = nextInt16();
				if(a == b) jump(jumpTo);
			}
			break; 
        case BC_IFICMPG :
			{
				int64_t a = getInt();
				int64_t b = getInt();
				int16_t jumpTo = nextInt16();
				if(a > b) jump(jumpTo);
			}
			break; 
        case BC_IFICMPGE :
			{
				int64_t a = getInt();
				int64_t b = getInt();
				int16_t jumpTo = nextInt16();
				if(a >= b) jump(jumpTo);
			}
			break; 
        case BC_IFICMPL :
			{
				int64_t a = getInt();
				int64_t b = getInt();
				int16_t jumpTo = nextInt16();
				if(a < b) jump(jumpTo);
			}
			break; 
        case BC_IFICMPLE :
			{
				int64_t a = getInt();
				int64_t b = getInt();
				int16_t jumpTo = nextInt16();
				if(a <= b) jump(jumpTo);
			}
			break; 
        case BC_CALL :
			createFrame(nextInt16());
			break; 
        case BC_RETURN :
			dropFrame();
			break; 
		case BC_STOP :
			return &Status();
		}
	}
}

Instruction GeneratedCode :: getNextInstruction() {
	Instruction instruction = currentBytecode->getInsn(currentFrame->stackPointer);
	currentFrame->stackPointer++;
	return instruction;
}

void GeneratedCode :: add(int64_t item) {
	StackVariable result;
	result.int_part = item;
	dataStack.push_back(result);
}
void GeneratedCode :: add(double item) {
	StackVariable result;
	result.double_part = item;
	dataStack.push_back(result);
}
void GeneratedCode :: add(const char* item) {
	StackVariable result;
	result.string_part = item;
	dataStack.push_back(result);
}
void GeneratedCode :: add(StackVariable item) {
	dataStack.push_back(item);
}

void GeneratedCode :: addString(int id) {
	StackVariable result;
	result.string_part = constantById(id).c_str();
	dataStack.push_back(result);
}

int64_t GeneratedCode :: nextInt() {
	int64_t result = currentBytecode->getInt64(currentFrame->stackPointer);
	currentFrame->stackPointer += sizeof(result);
	return result;
}

int16_t GeneratedCode :: nextInt16() {
	int16_t result = currentBytecode->getInt16(currentFrame->stackPointer);
	currentFrame->stackPointer += sizeof(result);
	return result;
}


double GeneratedCode :: nextDouble() {
	double result = currentBytecode->getDouble(currentFrame->stackPointer);
	currentFrame->stackPointer += sizeof(result);
	return result;
}

StackVariable GeneratedCode :: getVar() {
	StackVariable result = dataStack.back();
	dataStack.pop_back();
	return result;
}

const char* GeneratedCode :: getString() {
	const char* result = dataStack.back().string_part;
	dataStack.pop_back();
	return result;
}
int64_t GeneratedCode :: getInt() {
	int64_t result = dataStack.back().int_part;
	dataStack.pop_back();
	return result;
}
double GeneratedCode :: getDouble() {
	double result = dataStack.back().double_part;
	dataStack.pop_back();
	return result;
}

void GeneratedCode :: print(const char* str) {
	cout << str;
}

void GeneratedCode :: print(int64_t item) {
	cout << item;
}

void GeneratedCode ::  print(double item) {
	std :: stringstream out;
	out << item;
	int pos;
	string result = out.str();
	if((pos = result.find("e+0")) != -1) {
		result.replace(pos, 3, "e+");
	}
	if((pos = result.find("e-0")) != -1) {
		result.replace(pos, 3, "e-");
	}
	cout << result;
}


void GeneratedCode :: jump(int16_t to) {
	currentFrame->stackPointer += to - (int16_t)2;
}

Frame* GeneratedCode ::loadFrame(int16_t fid) {
			if(currentFrame->function_id == fid) return currentFrame;
			else {
				Frame * tmpFrame = currentFrame;
				currentFrame = tmpFrame->parentFrame;
				if(currentFrame == NULL) {cout << "Error : This frame doesn't exist " << fid << endl;}
				Frame * result = loadFrame(fid);
				currentFrame = tmpFrame;
				return result;
			}
		}

void GeneratedCode :: createFrame(int16_t fid) {
	Frame * frame = new Frame;
	frame->function_id = fid;
	frame->parentFrame = currentFrame;
	frame->stackPointer = 0;
	currentFrame = frame;
	BytecodeFunction * function = ((BytecodeFunction*)functionById(currentFrame->function_id));
	currentBytecode = function->bytecode();
	for(int i = function->parametersNumber() - 1;i >= 0; --i) {
		VarType type = function->parameterType(i);
		if(type == VT_INT) 
			frame->variables[i].int_part = getInt();
		if(type == VT_DOUBLE) 
			frame->variables[i].double_part = getDouble();
		if(type == VT_STRING) 
			frame->variables[i].string_part = getString();
	}

}

void GeneratedCode :: dropFrame() {
	if(currentFrame->parentFrame) {
		currentBytecode = ((BytecodeFunction*)functionById(currentFrame->parentFrame->function_id))->bytecode();
	}
	currentFrame = currentFrame->parentFrame;
}