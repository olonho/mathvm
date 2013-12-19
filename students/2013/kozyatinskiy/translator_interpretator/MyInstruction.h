#pragma once

#include <stdio.h>
#include <vector>
using std::vector;
#include <string>
using std::string;
#include <sstream>
using std::stringstream;

#include <mathvm.h>
#include <ast.h>
using namespace mathvm;

#define __int64 int64_t

#include "StringStorage.h"
#include "MyVarScope.h"
#include "MyTypeStack.h"
#include "MyFuncScope.h"
#include "MyInterpreter.h"

template<typename T>
string toString(const T& val)
{
	stringstream ss;
	ss << val;
	return ss.str();
}

template<typename T>
VarType getType(const T& val);
template<> inline VarType getType<__int64>(const __int64& val){ return VT_INT; }
template<> inline VarType getType<double>(const double& val){ return VT_DOUBLE; }
template<> inline VarType getType<string>(const string& val){ return VT_STRING; }

template<typename T>
string getTypeStr(const T& val);
template<> inline string getTypeStr<__int64>(const __int64& val){ return "INT"; }
template<> inline string getTypeStr<double>(const double& val){ return "DOUBLE"; }
template<> inline string getTypeStr<string>(const string& val){ return "STRING"; }

template<typename T>
inline void printVar(const T& val)
{
	cout << val;
}
template<>
inline void printVar(const __int64& val)
{
	printf("%ld", val);
}

class MyInstruction
{
public:
	virtual ~MyInstruction(void){}
	virtual void processStack(MyTypeStack* stack){}
	virtual string text() const = 0;
	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager) = 0;

	static MyInstruction* GetCast(VarType from, VarType to);
	static MyInstruction* GetStore(VarType type, const string& name, MyVarScope* varScope);
};

template<typename T>
class ConstLoad : public MyInstruction
{
public:
	ConstLoad(T val) : var(toString(val)), val_(val){}

	virtual void processStack(MyTypeStack* stack)
	{
		stack->push(getType(T()));
	}
	virtual string text() const
	{
		return getTypeStr(T()).substr(0, 1) + "LOAD " + var;
	}

	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager)
	{
		++state->programPointer_;
		state->vals_.push(new T(val_));
	}
private:
	string var;
	T val_;
};

template<>
class ConstLoad<string> : public MyInstruction
{
public:
	ConstLoad(const string& str, StringStorage* storage) : id_(storage->add(str)){}
	virtual void processStack(MyTypeStack* stack)
	{
		stack->push(VT_STRING);
	}

	virtual string text() const
	{
		return "SLOAD " + toString(id_);
	}

	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager)
	{
		++state->programPointer_;
		state->vals_.push(manager->getConstString(id_));
	}
private:
	int id_;
};

class Load : public MyInstruction
{
public:
	Load(VarType type, const string& name, MyVarScope* varScope) : 
		id_(varScope->getLoadID(type, name)), type_(type){}
	virtual void processStack(MyTypeStack* stack)
	{
		stack->push(type_);
	}

	virtual string text() const
	{
		return "LOAD " + toString(id_);
	}

	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager)
	{
		++state->programPointer_;
		state->vals_.push(manager->get(id_));
	}
private:
	int     id_;
	VarType type_;
};

template<typename T>
class Store : public MyInstruction
{
public:
	Store(const string& name, MyVarScope* varScope) : 
		id_(varScope->getStoreID(getType(T()), name))
	{
	}
	virtual void processStack(MyTypeStack* stack)
	{
		if (stack->pop() != getType(T())) throw std::logic_error("bad stack");
	}

	virtual string text() const
	{
		return getTypeStr(T()).substr(0,1) + "STORE " + toString(id_);
	}

	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager)
	{
		++state->programPointer_;
		void* val = state->vals_.top();
		state->vals_.pop();
		manager->store(id_, val);
	}
private:
	int id_;
};

class Return : public MyInstruction
{
public:
	virtual string text() const
	{
		return "RETURN";
	}
	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager)
	{
		if (state->returnPointers_.empty())
			state->isExit_ = true;
		else
		{
			int pointer = state->returnPointers_.top();
			state->returnPointers_.pop();
			state->programPointer_ = pointer;
			manager->decMem();
		}
	}
};

class Swap : public MyInstruction
{
public:
	virtual string text() const
	{
		return "SWAP";
	}

	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager)
	{
		state->programPointer_++;
		void* val1 = state->vals_.top();
		state->vals_.pop();
		void* val2 = state->vals_.top();
		state->vals_.pop();
		state->vals_.push(val1);
		state->vals_.push(val2);
	}
};

template<typename From, typename To>
class ConvertTop : public MyInstruction
{
	virtual void processStack(MyTypeStack* stack)
	{
		if (stack->pop() != getType(From()))
			throw std::logic_error("bad type on top");
		stack->push(getType(To()));
	}

	virtual string text() const
	{
		return getTypeStr(From()).substr(0, 1) + "2" + getTypeStr(To()).substr(0,1);
	}

	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager)
	{
		state->programPointer_++;
		void* val1 = state->vals_.top();
		state->vals_.pop();
		void* conv = new To(static_cast<To>(*static_cast<From*>(val1)));
		//delete val1;
		state->vals_.push(conv);
	}
};

class Integer : public MyInstruction
{
public:
	virtual void processStack(MyTypeStack* stack)
	{
		if (stack->pop() != VT_INT) throw std::logic_error("not int on stack");
		if (stack->pop() != VT_INT) throw std::logic_error("not int on stack");
		stack->push(VT_INT);
	}

	virtual string text() const
	{
		return op_;
	}
	
	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager)
	{
		state->programPointer_++;
		void* val1 = state->vals_.top();
		state->vals_.pop();
		void* val2 = state->vals_.top();
		// delete val1
		// delete val2
		state->vals_.push(new __int64(exec(*static_cast<__int64*>(val1), *static_cast<__int64*>(val2))));
	}

	static Integer* GetByKind(TokenKind kind);
protected:
	Integer(const string& op) : op_(op){}
private:
	virtual __int64 exec(__int64 val1, __int64 val2) = 0;

	string op_;
};

struct LogicIntegerOr : public Integer
{
	LogicIntegerOr() : Integer("IOR"){}

	virtual __int64 exec(__int64 val1, __int64 val2)
	{
		return val1 || val2;
	}
};

struct LogicIntegerAnd : public Integer
{
	LogicIntegerAnd() : Integer("IAND"){}
	
	virtual __int64 exec(__int64 val1, __int64 val2)
	{
		return val1 && val2;
	}
};

struct LogicIntegerNeq : public Integer
{
	LogicIntegerNeq() : Integer("INEQ"){}

	virtual __int64 exec(__int64 val1, __int64 val2)
	{
		return val1 != val2;
	}
};

struct LogicIntegerEq : public Integer
{
	LogicIntegerEq() : Integer("EQ"){}

	virtual __int64 exec(__int64 val1, __int64 val2)
	{
		return val1 == val2;
	}
};

struct AIntegerOr : public Integer
{
	AIntegerOr() : Integer("IAOR"){}

	virtual __int64 exec(__int64 val1, __int64 val2)
	{
		return val1 | val2;
	}
};

struct AIntegerAnd : public Integer
{
	AIntegerAnd() : Integer("IAAND"){}

	virtual __int64 exec(__int64 val1, __int64 val2)
	{
		return val1 & val2;
	}
};

struct AIntegerMod : public Integer
{
	AIntegerMod() : Integer("IMOD"){}
	virtual __int64 exec(__int64 val1, __int64 val2)
	{
		return val1 % val2;
	}
};

struct AIntegerXor : public Integer
{
	AIntegerXor() : Integer("IAXOR"){}
	virtual __int64 exec(__int64 val1, __int64 val2)
	{
		return val1 ^ val2;
	}
};

class Numeric : public MyInstruction
{
public:
	virtual void processStack(MyTypeStack* stack)
	{
		if (stack->pop() != type_) throw std::logic_error("bad stack");
		if (stack->pop() != type_) throw std::logic_error("bad stack");
		stack->push(result_);
	}

	virtual string text() const
	{
		return op_;
	}
	
	static Numeric* GetByKind(TokenKind kind, VarType type);

	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager)
	{
		state->programPointer_++;
		void* val1 = state->vals_.top();
		state->vals_.pop();
		void* val2 = state->vals_.top();
		state->vals_.pop();
		state->vals_.push(exec(val2, val1));
	}

protected:
	virtual void* exec(void* val1, void* val2) = 0;

	Numeric(const string& op, VarType type, VarType result) : op_(op), type_(type), result_(result){}
private:
	string  op_;
	VarType type_;
	VarType result_;
};

template<typename T>
struct GT : public Numeric
{ 
	GT() : Numeric(getTypeStr(T()).substr(0,1) + "GT", getType(T()), VT_INT){} 

	virtual void* exec(void* val1, void* val2)
	{
		return new __int64(*(T*)val1 > *(T*)val2);
	}
};

template<typename T>
struct GE : public Numeric
{ 
	GE() : Numeric(getTypeStr(T()).substr(0,1) + "GE", getType(T()), VT_INT){} 

	virtual void* exec(void* val1, void* val2)
	{
		return new __int64(*(T*)val1 >= *(T*)val2);
	}
};

template<typename T>
struct LT : public Numeric
{ 
	LT() : Numeric(getTypeStr(T()).substr(0,1) + "LT", getType(T()), VT_INT){} 

	virtual void* exec(void* val1, void* val2)
	{
		return new __int64(*(T*)val1 < *(T*)val2);
	}
};

template<typename T>
struct LE : public Numeric
{ 
	LE() : Numeric(getTypeStr(T()).substr(0,1) + "LE", getType(T()), VT_INT){} 

	virtual void* exec(void* val1, void* val2)
	{
		return new __int64(*(T*)val1 <= *(T*)val2);
	}
};

template<typename T>
struct ADD : public Numeric
{ 
	ADD() : Numeric(getTypeStr(T()).substr(0,1) + "ADD", getType(T()), getType(T())){} 

	virtual void* exec(void* val1, void* val2)
	{
		return new T(*(T*)val1 + *(T*)val2);
	}
};

template<typename T>
struct SUB : public Numeric
{ 
	SUB() : Numeric(getTypeStr(T()).substr(0,1) + "SUB", getType(T()), getType(T())){} 

	virtual void* exec(void* val1, void* val2)
	{
		return new T(*(T*)val1 - *(T*)val2);
	}
};

template<typename T>
struct MUL : public Numeric
{ 
	MUL() : Numeric(getTypeStr(T()).substr(0,1) + "MUL", getType(T()), getType(T())){} 

	virtual void* exec(void* val1, void* val2)
	{
		return new T(*(T*)val1 * *(T*)val2);
	}
};

template<typename T>
struct DIV : public Numeric
{ 
	DIV() : Numeric(getTypeStr(T()).substr(0,1) + "DIV", getType(T()), getType(T())){} 

	virtual void* exec(void* val1, void* val2)
	{
		return new T(*(T*)val1 / *(T*)val2);
	}
};

class INOT : public MyInstruction
{
public:
	virtual void processStack(MyTypeStack* stack)
	{
		if (stack->pop() != VT_INT) throw std::logic_error("bad stack");
		stack->push(VT_INT);
	}

	virtual string text() const
	{
		return "INOT";
	}

	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager)
	{
		state->programPointer_++;
		void* val1 = state->vals_.top();
		*static_cast<__int64*>(val1) = ! *static_cast<__int64*>(val1);
	}
};

template<typename T>
class NEG : public MyInstruction
{
public:
	virtual void processStack(MyTypeStack* stack)
	{
		VarType top = stack->pop();
		if (top != getType(T())) throw std::logic_error("bad stack");
		stack->push(top);
	}

	virtual string text() const
	{
		return getTypeStr(T()).substr(0,1) + "NEG";
	}

	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager)
	{
		state->programPointer_++;
		void* val1 = state->vals_.top();
		*static_cast<T*>(val1) = - *static_cast<T*>(val1);
	}
};

class MyLabel : public MyInstruction
{
public:
	MyLabel()
	{
		static int lastID = 0;
		id_ = ++lastID;
	}

	virtual string text() const
	{
		return "LABEL " + toString(id_);
	}

	int id() const{ return id_; }

	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager)
	{
		throw std::logic_error(":(");
	}

private:
	int id_;
};

class Jump : public MyInstruction
{
public:
	Jump(MyLabel* target) : target_(target), offset_(0){}

	int labelId() const
	{
		if (target_)
			return target_->id();
		return -1;
	}

	void replaceToOffset(int offset)
	{
		target_ = 0;
		offset_ = offset;
	}

	int offset() const { return offset_; }

private:
	MyLabel* target_;
	int    offset_;
};

class JA : public Jump
{
public:
	JA(MyLabel* target) : Jump(target){}

	virtual string text() const
	{
		int label = labelId();
		if (label != -1)
			return "JA " + toString(label);
		else
			return "JA @" + toString(offset());
	}

	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager)
	{
		state->programPointer_ += offset();
	}
};

class JNIF : public Jump
{
public:
	JNIF(MyLabel* target) : Jump(target){}

	virtual void processStack(MyTypeStack* stack)
	{
		VarType top = stack->pop();
		if (top != VT_INT) throw std::logic_error("bad stack");
	}

	virtual string text() const
	{
		int label = labelId();
		if (label != -1)
			return "JNIF " + toString(label);
		else
			return "JNIF @" + toString(offset());
	}

	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager)
	{
		void* val1 = state->vals_.top();
		state->vals_.pop();
		if (!*static_cast<__int64*>(val1))
			state->programPointer_ += offset();
		else
			state->programPointer_++;
	}
};

struct INCM : public MyInstruction
{
	virtual string text() const
	{
		return "INCM";
	}

	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager)
	{
		state->programPointer_++;
		manager->incMem();
	}
};

struct DECM : public MyInstruction
{
	virtual string text() const
	{
		return "DECM";
	}

	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager)
	{
		state->programPointer_++;
		manager->decMem();
	}
};

template<typename T>
class PRINT : public MyInstruction
{
	virtual void processStack(MyTypeStack* stack)
	{
		VarType top = stack->pop();
		if (top != getType(T())) throw std::logic_error("bad stack");
	}

	virtual string text() const
	{
		return getTypeStr(T()).substr(0,1) + "PRINT";
	}

	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager)
	{
		state->programPointer_++;
		void* val1 = state->vals_.top();
		state->vals_.pop();

		printVar(*static_cast<T*>(val1));
	}
};

struct CALL : public MyInstruction
{
	CALL(const string& name, MyFuncScope* scope) : funcID_(-1), pointer_(-1)
	{
		pair<VarType, int> f = scope->getFuncID(name);
		funcID_ = f.second;
		result_ = f.first;
	}

	int funcId() const{ return funcID_; }

	virtual void processStack(MyTypeStack* stack)
	{
		if (result_ == VT_INT || result_ == VT_STRING || result_ == VT_DOUBLE)
			stack->push(result_);
	}

	void setPointer(int pointer)
	{
		pointer_ = pointer;
	}

	virtual string text() const
	{
		if (pointer_ == -1)
			return "CALL " + toString(funcID_);
		else
			return "CALL #" + toString(pointer_);
	}

	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager)
	{
		state->returnPointers_.push(state->programPointer_ + 1);
		state->programPointer_ = pointer_;
	}
private:
	int funcID_;
	int pointer_;
	VarType result_;
};

struct StringConstant : public MyInstruction
{
	StringConstant(const string& str) : str_(str){}

	const string& get() const{ return str_; }
	
	virtual string text() const
	{
		string tmp = str_;
		size_t start_pos = 0;
		while((start_pos = tmp.find("\n", start_pos)) != std::string::npos) {
			tmp.replace(start_pos, 1, "\\n");
			start_pos += 2; // ...
		}
		return "'" + tmp + "'";
	}

	virtual void exec(MyInterpreter::State* state, MyMemoryManager* manager)
	{
		state->programPointer_++;
		manager->addConstString(str_);
	}

private:
	string str_;
};
