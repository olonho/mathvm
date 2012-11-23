#ifndef RUNNER_H
#define RUNNER_H

#include <string>
#include <stack>
#include <map>
// #include <tr1/shared_ptr.h>

#include "mathvm.h"
#include "CodeImpl.h"

namespace mathvm {

struct FromStream;
class Runner
{
	struct Processor;
	typedef std::map<Instruction, Processor*> insn2Proc_t;

	CodeImpl* _code;
	insn2Proc_t _processors;
	std::stack<Var> _stack;
	Bytecode* _bytecode;
	size_t _ip;
public:
	Runner(CodeImpl* code);
	~Runner();

	Status* execute(vector<Var*>&);

private:
	Bytecode* bytecode();
	//Helpers
	// work with stack:
	int64_t popInteger();
	void pushInteger(int64_t val);
	double popDouble();
	void pushDouble(double val);
	const std::string popString();
	void pushString(const std::string& val);
	//work with inline params
	uint16_t getIdFromStream();
	double getDoubleFromStream();
	int64_t getIntegerFromStream();
	const std::string& getStringFromStream();
	//
	Instruction getInstruction();
	//
	struct Processor {
		virtual void operator()() = 0;
	};

	template <typename T>
	class GenericProcessor :public Processor {
		Runner& _runner;
		T _func;
	public:
		GenericProcessor(Runner& runner, T func)
		: _runner(runner)
		,  _func(func)
		{}

		virtual void operator()() {
			process(_func);
		}

		void process(void (*func) (double) ) {
			func(_runner.popDouble());
		}

		void process(void (*func) (int64_t)) {
			func(_runner.popInteger());
		}

		void process(void (*func) (const std::string&)) {
			func(_runner.popString());
		}

		void process(double (*func) (FromStream*, double)) {
			_runner.pushDouble(func(0, _runner.getDoubleFromStream()));
		}

		void process(int64_t (*func) (FromStream*, int64_t)) {
			_runner.pushInteger(func(0, _runner.getIntegerFromStream()));
		}

		void process(const std::string& (*func) (FromStream*, const std::string&)) {
			_runner.pushString(func(0, _runner.getStringFromStream()));
		}

		void process(double (*func) ()) {
			_runner.pushDouble(func());
		}

		void process(int64_t (*func) ()) {
			_runner.pushInteger(func());
		}

		void process(const std::string& (*func) ()) {
			_runner.pushString(func());
		}

		void process(const std::string (*func) ()) {
			_runner.pushString(func());
		}
	};

	template <typename T>
	std::pair<Instruction, Processor*> pr(Instruction instruction, T func) {
		return std::make_pair(instruction, new GenericProcessor<T>(*this, func));
	}

};

}

#endif