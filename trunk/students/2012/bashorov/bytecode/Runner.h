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

	struct State {
		size_t ip;
		std::map<uint16_t, Var> variables;
	};

	std::map<uint16_t, std::stack<State> > _states;
	std::stack<uint16_t> _callStack;
	std::stack<Var> _stack;
	Bytecode* _bytecode;

	State* _state;
public:
	Runner(CodeImpl* code);
	~Runner();

	Status* execute(vector<Var*>&);

private:
	Bytecode* bytecode();
	void prepareCall(uint16_t functionId);
	void returnCall();
	//Helpers
	// work with stack:
	template<typename T>
	T typedPop();
	template<typename T>
	void typedPush(T val);
	//work with inline params
	uint16_t getIdFromStream();
	int16_t getInt16FromStream();
	double getDoubleFromStream();
	int64_t getIntegerFromStream();
	const std::string& getStringFromStream();
	//
	Instruction getInstruction();
	//
	template <typename T, uint16_t id>
	T loadvar();
	template <typename T>
	T loadvarById(uint16_t id);
	template <typename T>
	T loadvarById(uint16_t  ctx, uint16_t id);
	template <typename T, uint16_t id>
	void storevar(T value);
	template <typename T>
	void storevarById(uint16_t id, T value);
	template <typename T>
	void storevarById(uint16_t ctx, uint16_t id, T value);
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

		template <typename A1, typename A2, typename R>
		void process(std::binary_function<A1, A2, R>& func) {
			T& f = (T&) func;
			A2 arg2 = _runner.typedPop<A2>();
			A1 arg1 = _runner.typedPop<A1>();
			_runner.typedPush<R>(f(arg1, arg2));
		}

		template <typename A1>
		void process(std::unary_function<A1, void>& func) {
			T& f = (T&) func;
			f(_runner.typedPop<A1>());
		}

		template <typename A1, typename R>
		void process(std::unary_function<A1, R>& func) {
			T& f = (T&) func;
			_runner.typedPush<R>(f(_runner.typedPop<A1>()));
		}

		void process(void (*func) (double) ) {
			func(_runner.typedPop<double>());
		}

		void process(void (*func) (int64_t)) {
			func(_runner.typedPop<int64_t>());
		}

		void process(void (*func) (const std::string&)) {
			func(_runner.typedPop<std::string>());
		}

		void process(double (*func) (FromStream*, double)) {
			_runner.typedPush<double>(func(0, _runner.getDoubleFromStream()));
		}

		void process(int64_t (*func) (FromStream*, int64_t)) {
			_runner.typedPush<int64_t>(func(0, _runner.getIntegerFromStream()));
		}

		void process(const std::string& (*func) (FromStream*, const std::string&)) {
			_runner.typedPush<std::string>(func(0, _runner.getStringFromStream()));
		}

		void process(double (*func) (FromStream*, uint16_t)) {
			_runner.typedPush<double>(func(0, _runner.getIdFromStream()));
		}

		void process(int64_t (*func) (FromStream*, uint16_t)) {
			_runner.typedPush<int64_t>(func(0, _runner.getIdFromStream()));
		}

		void process(string (*func) (FromStream*, uint16_t)) {
			_runner.typedPush<std::string>(func(0, _runner.getIdFromStream()));
		}

		void process(double (*func) ()) {
			_runner.typedPush<double>(func());
		}

		void process(int64_t (*func) ()) {
			_runner.typedPush<int64_t>(func());
		}

		void process(const std::string& (*func) ()) {
			_runner.typedPush<std::string>(func());
		}

		void process(const std::string (*func) ()) {
			_runner.typedPush<std::string>(func());
		}
	};

	template <typename T>
	std::pair<Instruction, Processor*> pr(Instruction instruction, T func) {
		return std::make_pair(instruction, new GenericProcessor<T>(*this, func));
	}

	template<typename MF>
	binder1st<MF> callMem(MF fun) {
		return std::bind1st(std::mem_fun(fun), this);
	}
};

}

#endif
