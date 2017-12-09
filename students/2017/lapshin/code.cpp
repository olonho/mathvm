#include "code.h"

#include "util.h"

#include <stack>

namespace mathvm::ldvsoft {

class BytecodeCode::Evaluator {
public:
	class VarEx {
	public:
		VarType type;
		union {
			double d;
			int64_t i;
			string s;
		};

		VarEx(Var const &v) {
			type = v.type();
			switch (type) {
			case VT_DOUBLE:
				d = v.getDoubleValue();
				break;
			case VT_INT:
				i = v.getIntValue();
				break;
			case VT_STRING:
				s = v.getStringValue();
				break;
			default:
				;
			}
		}
		VarEx(double d):
			type{VT_DOUBLE}, d{d} {}
		VarEx(int64_t i = 0):
			type{VT_INT}, i{i} {}
		VarEx(string const &s):
			type{VT_STRING}, s{s} {}
		VarEx(VarEx const &that) {
			type = that.type;
			switch (type) {
			case VT_DOUBLE:
				d = that.d;
				break;
			case VT_INT:
				i = that.i;
				break;
			case VT_STRING:
				s = that.s;
				break;
			default:
				;
			}
		}
		VarEx(VarEx &&that) {
			type = that.type;
			switch (type) {
			case VT_DOUBLE:
				d = that.d;
				break;
			case VT_INT:
				i = that.i;
				break;
			case VT_STRING:
				s = move(that.s);
				break;
			default:
				;
			}
		}

		~VarEx() {
			if (type == VT_STRING)
				s.~string();
		}

		VarEx &operator=(VarEx that) {
			type = that.type;
			switch (type) {
			case VT_DOUBLE:
				d = that.d;
				break;
			case VT_INT:
				i = that.i;
				break;
			case VT_STRING:
				swap(s, that.s);
				break;
			default:
				;
			}
			return *this;
		}

		friend ostream &operator<<(ostream &out, VarEx const &v) {
			switch (v.type) {
			case VT_DOUBLE:
				return out << v.d;
			case VT_INT:
				return out << v.i;
			case VT_STRING:
				return out << v.s;
			default:
				;
			}
			return out;
		}
	};

	class Frame {
	public:
		TranslatedFunction &function;
		size_t ip{0};
		unordered_map<uint16_t, unordered_map<uint16_t, VarEx>> vars;

		::mathvm::Bytecode &code() {
			return *function.bytecode();
		}

		Frame(TranslatedFunction &function):
			function{function} {
			for (auto id: function.scopes)
				vars[id] = {};
		}
		~Frame() = default;

	};

	BytecodeCode &global;
	std::vector<Frame> frames;
	std::stack<VarEx> stack;
	Status *status{nullptr};

	Frame &frame() {
		return *frames.rbegin();
	}

	unordered_map<uint16_t, VarEx> &lookupScope(uint16_t id) {
		for (auto it{frames.rbegin()}; it != frames.rend(); ++it)
			if (it->vars.count(id))
				return it->vars[id];
		assert(false);
	}

#	define DEFINE_METHOD(name, comment, sz) \
	void process##name(size_t adj_size = sz - 1);
	FOR_BYTECODES(DEFINE_METHOD)
#	undef  DEFINE_METHOD

	Evaluator(BytecodeCode &code):
		global{code} {
		frames.emplace_back(*static_cast<TranslatedFunction*>(
			global.functionById(0)
		));
	};
};

void BytecodeCode::Evaluator::processINVALID(size_t) {
	status = StatusEx::Error("INVALID at " + to_string(frame().ip - 1));
}

void BytecodeCode::Evaluator::processDLOAD(size_t adj_size) {
	double value{frame().code().getDouble(frame().ip)};
	frame().ip += adj_size;
	stack.emplace(value);
}

void BytecodeCode::Evaluator::processILOAD(size_t adj_size) {
	auto value{frame().code().getInt64(frame().ip)};
	frame().ip += adj_size;
	stack.emplace(value);
}

void BytecodeCode::Evaluator::processSLOAD(size_t adj_size) {
	auto id{frame().code().getUInt16(frame().ip)};
	frame().ip += adj_size;
	stack.emplace(global.constantById(id));
}

void BytecodeCode::Evaluator::processDLOAD0(size_t) {
	stack.emplace(.0);
}

void BytecodeCode::Evaluator::processILOAD0(size_t) {
	stack.emplace(static_cast<int64_t>(0));
}

void BytecodeCode::Evaluator::processSLOAD0(size_t) {
	stack.emplace("");
}

void BytecodeCode::Evaluator::processDLOAD1(size_t) {
	stack.emplace(1.0);
}

void BytecodeCode::Evaluator::processILOAD1(size_t) {
	stack.emplace(static_cast<int64_t>(1));
}

void BytecodeCode::Evaluator::processDLOADM1(size_t) {
	stack.emplace(-1.0);
}

void BytecodeCode::Evaluator::processILOADM1(size_t) {
	stack.emplace(static_cast<int64_t>(-1));
}

void BytecodeCode::Evaluator::processDADD(size_t) {
	auto left {stack.top().d}; stack.pop();
	auto right{stack.top().d}; stack.pop();
	stack.emplace(left + right);
}

void BytecodeCode::Evaluator::processIADD(size_t) {
	auto left {stack.top().i}; stack.pop();
	auto right{stack.top().i}; stack.pop();
	stack.emplace(left + right);
}

void BytecodeCode::Evaluator::processDSUB(size_t) {
	auto left {stack.top().d}; stack.pop();
	auto right{stack.top().d}; stack.pop();
	stack.emplace(left - right);
}

void BytecodeCode::Evaluator::processISUB(size_t) {
	auto left {stack.top().i}; stack.pop();
	auto right{stack.top().i}; stack.pop();
	stack.emplace(left - right);
}

void BytecodeCode::Evaluator::processDMUL(size_t) {
	auto left {stack.top().d}; stack.pop();
	auto right{stack.top().d}; stack.pop();
	stack.emplace(left * right);
}

void BytecodeCode::Evaluator::processIMUL(size_t) {
	auto left {stack.top().i}; stack.pop();
	auto right{stack.top().i}; stack.pop();
	stack.emplace(left * right);
}

void BytecodeCode::Evaluator::processDDIV(size_t) {
	auto left {stack.top().d}; stack.pop();
	auto right{stack.top().d}; stack.pop();
	stack.emplace(left / right);
}

void BytecodeCode::Evaluator::processIDIV(size_t) {
	auto left {stack.top().i}; stack.pop();
	auto right{stack.top().i}; stack.pop();
	stack.emplace(left / right);
}

void BytecodeCode::Evaluator::processIMOD(size_t) {
	auto left {stack.top().i}; stack.pop();
	auto right{stack.top().i}; stack.pop();
	stack.emplace(left % right);
}

void BytecodeCode::Evaluator::processDNEG(size_t) {
	auto op{stack.top().d}; stack.pop();
	stack.emplace(-op);
}

void BytecodeCode::Evaluator::processINEG(size_t) {
	auto op{stack.top().i}; stack.pop();
	stack.emplace(-op);
}

void BytecodeCode::Evaluator::processIAOR(size_t) {
	auto left {stack.top().i}; stack.pop();
	auto right{stack.top().i}; stack.pop();
	stack.emplace(left | right);
}

void BytecodeCode::Evaluator::processIAAND(size_t) {
	auto left {stack.top().i}; stack.pop();
	auto right{stack.top().i}; stack.pop();
	stack.emplace(left & right);
}

void BytecodeCode::Evaluator::processIAXOR(size_t) {
	auto left {stack.top().i}; stack.pop();
	auto right{stack.top().i}; stack.pop();
	stack.emplace(left ^ right);
}

void BytecodeCode::Evaluator::processIPRINT(size_t) {
	auto op{stack.top().i}; stack.pop();
	cout << op << flush;
}

void BytecodeCode::Evaluator::processDPRINT(size_t) {
	auto op{stack.top().d}; stack.pop();
	cout << op << flush;
}

void BytecodeCode::Evaluator::processSPRINT(size_t) {
	auto op{stack.top().s}; stack.pop();
	cout << op << flush;
}

void BytecodeCode::Evaluator::processI2D(size_t) {
	auto op{stack.top().i}; stack.pop();
	stack.emplace(static_cast<double>(op));
}

void BytecodeCode::Evaluator::processD2I(size_t) {
	auto op{stack.top().d}; stack.pop();
	stack.emplace(static_cast<int64_t>(op));
}

void BytecodeCode::Evaluator::processS2I(size_t) {
	auto op{stack.top().s}; stack.pop();
	stack.emplace(static_cast<int64_t>(stoll(op)));
}

void BytecodeCode::Evaluator::processSWAP(size_t) {
	auto left {stack.top().i}; stack.pop();
	auto right{stack.top().i}; stack.pop();
	stack.emplace(move(right));
	stack.emplace(move(left ));
}

void BytecodeCode::Evaluator::processPOP(size_t) {
	stack.pop();
}

#define APPLY(pref, DO) \
	DO(pref)
#define FOR_IDS(pref, DO) \
	DO(pref, 0) \
	DO(pref, 1) \
	DO(pref, 2) \
	DO(pref, 3)
#define FOR_TYPES(pref, ITER, DO) \
	ITER(pref##D, DO) \
	ITER(pref##I, DO) \
	ITER(pref##S, DO)

#define PROCESS_LOAD(name, id) \
void BytecodeCode::Evaluator::process##name##VAR##id(size_t) { \
	status = Status::Error("unsupported"); \
}
FOR_TYPES(LOAD, FOR_IDS, PROCESS_LOAD)
#undef  PROCESS_LOAD

#define PROCESS_STORE(name, id) \
void BytecodeCode::Evaluator::process##name##VAR##id(size_t) { \
	status = Status::Error("unsupported"); \
}
FOR_TYPES(STORE, FOR_IDS, PROCESS_STORE)
#undef  PROCESS_STORE

#define PROCESS_LOAD(name) \
void BytecodeCode::Evaluator::process##name##VAR(size_t) { \
	status = Status::Error("unsupported"); \
}
FOR_TYPES(LOAD, APPLY, PROCESS_LOAD)
#undef  PROCESS_LOAD

#define PROCESS_STORE(name) \
void BytecodeCode::Evaluator::process##name##VAR(size_t) { \
	status = Status::Error("unsupported"); \
}
FOR_TYPES(STORE, APPLY, PROCESS_STORE)
#undef  PROCESS_STORE

#define PROCESS_LOAD(name) \
void BytecodeCode::Evaluator::process##name##VAR(size_t adj_size) { \
	auto scope_id{frame().code().getUInt16(frame().ip)}; \
	frame().ip += adj_size / 2; \
	auto var_id{frame().code().getUInt16(frame().ip)}; \
	frame().ip += adj_size / 2; \
	stack.push(lookupScope(scope_id)[var_id]); \
}
FOR_TYPES(LOADCTX, APPLY, PROCESS_LOAD)
#undef  PROCESS_LOAD

#define PROCESS_STORE(name) \
void BytecodeCode::Evaluator::process##name##VAR(size_t adj_size) { \
	auto scope_id{frame().code().getUInt16(frame().ip)}; \
	frame().ip += adj_size / 2; \
	auto var_id{frame().code().getUInt16(frame().ip)}; \
	frame().ip += adj_size / 2; \
	lookupScope(scope_id)[var_id] = stack.top(); stack.pop(); \
}
FOR_TYPES(STORECTX, APPLY, PROCESS_STORE)
#undef  PROCESS_STORE

// sigh... operator <=>, we need you!
template <typename T>
int64_t sign(T val) {
	return (T{} < val) - (val < T{});
}

void BytecodeCode::Evaluator::processDCMP(size_t) {
	auto left {stack.top().d}; stack.pop();
	auto right{stack.top().d}; stack.pop();
	stack.emplace(sign(left - right));
}

void BytecodeCode::Evaluator::processICMP(size_t) {
	auto left {stack.top().i}; stack.pop();
	auto right{stack.top().i}; stack.pop();
	stack.emplace(sign(left - right));
}

void BytecodeCode::Evaluator::processJA(size_t) {
	auto value{frame().code().getInt16(frame().ip)};
	frame().ip += value;
}

void BytecodeCode::Evaluator::processIFICMPNE(size_t adj_size) {
	auto left {stack.top().i}; stack.pop();
	auto right{stack.top().i}; stack.pop();
	auto value{frame().code().getInt16(frame().ip)};
	if (left != right)
		frame().ip += value;
	else
		frame().ip += adj_size;
}

void BytecodeCode::Evaluator::processIFICMPE(size_t adj_size) {
	auto left {stack.top().i}; stack.pop();
	auto right{stack.top().i}; stack.pop();
	auto value{frame().code().getInt16(frame().ip)};
	if (left == right)
		frame().ip += value;
	else
		frame().ip += adj_size;
}

void BytecodeCode::Evaluator::processIFICMPG(size_t adj_size) {
	auto left {stack.top().i}; stack.pop();
	auto right{stack.top().i}; stack.pop();
	auto value{frame().code().getInt16(frame().ip)};
	if (left > right)
		frame().ip += value;
	else
		frame().ip += adj_size;
}

void BytecodeCode::Evaluator::processIFICMPGE(size_t adj_size) {
	auto left {stack.top().i}; stack.pop();
	auto right{stack.top().i}; stack.pop();
	auto value{frame().code().getInt16(frame().ip)};
	if (left >= right)
		frame().ip += value;
	else
		frame().ip += adj_size;
}

void BytecodeCode::Evaluator::processIFICMPL(size_t adj_size) {
	auto left {stack.top().i}; stack.pop();
	auto right{stack.top().i}; stack.pop();
	auto value{frame().code().getInt16(frame().ip)};
	if (left < right)
		frame().ip += value;
	else
		frame().ip += adj_size;
}

void BytecodeCode::Evaluator::processIFICMPLE(size_t adj_size) {
	auto left {stack.top().i}; stack.pop();
	auto right{stack.top().i}; stack.pop();
	auto value{frame().code().getInt16(frame().ip)};
	if (left <= right)
		frame().ip += value;
	else
		frame().ip += adj_size;
}

void BytecodeCode::Evaluator::processDUMP(size_t) {
	auto op{stack.top()}; stack.pop();
	cout << op << '\n';
}

void BytecodeCode::Evaluator::processSTOP(size_t) {
	status = Status::Error("Execution stopped");
}

void BytecodeCode::Evaluator::processCALL(size_t) {
	status = Status::Error("CALL is not implemented yet");
}

void BytecodeCode::Evaluator::processCALLNATIVE(size_t) {
	status = Status::Error("NATIVE CALL shit is not implemented yet");
}

void BytecodeCode::Evaluator::processRETURN(size_t) {
	status = Status::Error("RETURN is not implemented yet");
}

void BytecodeCode::Evaluator::processBREAK(size_t) {}

Status *BytecodeCode::execute(vector<Var*> &global_vars) {
	Evaluator e(*this);

	// Priming the execution
	/* FIXME */ if (false) {
	auto &root_scope{scopes[e.frames[0].function.function->scope()]};
	for (auto const *v: global_vars) {
		auto var_id{root_scope.vars[v->name()]};
		e.frames[0].vars[root_scope.id][var_id] = *v;
	}}

	while (e.status == nullptr || !e.status->isError()) {
		if (e.frame().ip >= e.frame().code().length()) {
			if (e.frame().function.signature()[0].first != VT_VOID) {
				e.status = StatusEx::Error(
					"Out of function " + e.frame().function.name() + " returning non-void"
				);
				continue;
			}
			if (e.frames.size() == 1) {
				// OVER
				e.status = Status::Ok();
				break;
			}
		}
		auto ip{e.frame().ip++};
		switch (e.frame().code().getInsn(ip)) {
#		define CASE_INSN(insn, comment, size) \
		case BC_##insn: \
			e.process##insn(); \
			break;
		FOR_BYTECODES(CASE_INSN)
#		undef  CASE_INSN
		default:
			;
		}
	}
	if (e.status->isOk()) {
		// TODO upload variables back
	}
	return e.status;
}

void BytecodeCode::disassemble(ostream &out, FunctionFilter *filter) {
	for (auto it{NativeFunctionIterator(this)}; it.hasNext(); ) {
		auto const &n{it.next()};
		out << "n*" << makeNativeFunction(
			n.name(), n.signature(), n.code()
		) << ' ' << n.name() << " <(";
		auto const &sign{n.signature()};
		for (size_t i{1}; i != sign.size(); ++i) {
			if (i > 1)
				out << ", ";
			out << typeToName(sign[i].first);
		}
		out << ") -> " << typeToName(sign[0].first);
		out << "> at " << n.code() << '\n';
	}
	for (auto it{ConstantIterator(this)}; it.hasNext(); ) {
		auto const &s{it.next()};
		out << "s@" << makeStringConstant(s) << ' ' << escape(s) << '\n';
	}
	Code::disassemble(out, filter);
}

}
