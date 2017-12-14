#include "code.h"

#include "util.h"

#include "ast.h"
#include "asmjit/asmjit.h"

#include <algorithm>
#include <stack>

namespace mathvm { namespace ldvsoft {

using namespace asmjit;

class BytecodeCode::Evaluator {
private:
	JitRuntime jit_runtime;

	class NativeCaller {
	private:
		class RegDescriptor {
		public:
			enum {
				MEMORY,
				GENERAL,
				XMM
			} category;

			size_t id;
		};

		static vector<X86GpReg> integer_regs;
		static vector<X86XmmReg> double_regs;

		using call_t = void (*)(void *result, void const *call_data);

		X86Assembler x86_ass;
		call_t wrapper;
		NativeFunctionDescriptor const &desc;

	public:
		NativeCaller(BytecodeCode::Evaluator &eval, uint16_t id):
			x86_ass{&eval.jit_runtime},
			desc{eval.global.nativeById(id)} {
			vector<RegDescriptor> arg_location;
			arg_location.resize(desc.signature().size() - 1);

			size_t ints_in_regs{0}, doubles_in_regs{0};

			size_t pushed{0};
			for (size_t i{1}; i != desc.signature().size(); ++i) {
				switch (desc.signature()[i].first) {
				case VT_INT:
				case VT_STRING:
					if (ints_in_regs >= integer_regs.size())
						arg_location[i - 1] = {RegDescriptor::MEMORY, 0};
					else
						arg_location[i - 1] = {RegDescriptor::GENERAL, ints_in_regs++};
					break;
				case VT_DOUBLE:
					if (doubles_in_regs >= double_regs.size())
						arg_location[i - 1] = {RegDescriptor::MEMORY, 0};
					else
						arg_location[i - 1] = {RegDescriptor::XMM, doubles_in_regs++};
					break;
				default:
					;
				}
			}
			x86_ass.push(x86::rbp);
			x86_ass.mov(x86::rbp, x86::rsp);
			// result pointer in rdi
			// args in rsi
			x86_ass.push(x86::r12);
			x86_ass.push(x86::r13);
			x86_ass.mov(x86::r12, x86::rdi);
			x86_ass.mov(x86::r13, x86::rsi);
			// result pointer in r12
			// args in r13
			if (pushed % 2 == 1) {
				// Align
				x86_ass.sub(x86::rsp, 8);
				++pushed;
			}
			for (size_t i{desc.signature().size() - 1}; i != 0; --i) {
				auto from{X86Mem(x86::r13, 0, 0)};
				auto const &loc{arg_location[i - 1]};
				switch (loc.category) {
				case RegDescriptor::MEMORY:
					x86_ass.push(X86Mem(from));
					pushed++;
					break;
				case RegDescriptor::GENERAL: {
						auto const &target{integer_regs[loc.id]};
						x86_ass.mov(target, from);
					}
					break;
				case RegDescriptor::XMM: {
						auto const &target{double_regs[loc.id]};
						x86_ass.movq(target, from);
					}
					break;
				}
				static_assert(sizeof(int64_t) == 8 && sizeof(double) == 8 && sizeof(char const*) == 8,
						"Data sizes are wrong!");
				x86_ass.add(x86::r13, 8);
			}

			x86_ass.call(Imm(reinterpret_cast<uintptr_t>(desc.code())));
			// rax with int result, xmm0 with double result
			auto to{X86Mem(x86::r12, 0, 0)};
			switch (desc.signature()[0].first) {
			case VT_INT:
			case VT_STRING:
				x86_ass.mov(to, x86::rax);
				break;
			case VT_DOUBLE:
				x86_ass.movq(to, x86::xmm0);
				break;
			case VT_VOID:
				x86_ass.xor_(x86::rax, x86::rax);
				break;
			default:
				;
			}

			x86_ass.add(x86::rsp, 8 * pushed);
			x86_ass.pop(x86::r13);
			x86_ass.pop(x86::r12);
			x86_ass.pop(x86::rbp);
			x86_ass.ret();

			wrapper = asmjit_cast<call_t>(x86_ass.make());
		}

		template<typename B, typename A>
		static B cast(A const &value) {
			return *const_cast<B*>(reinterpret_cast<B const*>(&value));
		}

		void operator()(stack<VarEx> &stack) const {
			vector<VarEx> args;
			for (size_t i{1}; i != desc.signature().size(); ++i) {
				args.push_back(stack.top()); stack.pop();
			}
			vector<uint64_t> raw_args;
			for (auto const &arg: args) {
				uint64_t raw_arg;
				switch (arg.type()) {
				case VT_DOUBLE:
					raw_arg = cast<uint64_t, double>(arg.d());
					break;
				case VT_INT:
					raw_arg = cast<uint64_t, int64_t>(arg.i());
					break;
				case VT_STRING:
					raw_arg = cast<uint64_t, char const*>(arg.s());
					break;
				default:
					;
				}
				raw_args.push_back(raw_arg);
			}
			reverse(raw_args.begin(), raw_args.end());
			uint64_t raw_result{0};

			wrapper(&raw_result, raw_args.data());

			VarEx result;
			switch (desc.signature()[0].first) {
			case VT_INT:
				result = cast<int64_t, uint64_t>(raw_result);
				break;
			case VT_STRING:
				result = cast<char const*, uint64_t>(raw_result);
				break;
			case VT_DOUBLE:
				result = cast<double, uint64_t>(raw_result);
				break;
			case VT_VOID:
				return;
			default:
				;
			}
			stack.emplace(move(result));
		}
	};

	unordered_map<uint16_t, NativeCaller> native_callers;
	NativeCaller const &native_caller_for(uint16_t id) {
		if (native_callers.count(id) == 0)
			native_callers.emplace(
				piecewise_construct,
				tuple<uint16_t>{id},
				tuple<Evaluator &, uint16_t>{*this, id}
			);
		return native_callers.at(id);
	}

public:
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
			global.functionByName(AstFunction::top_name)
		));
	};

	~Evaluator() = default;
};

vector<X86GpReg> BytecodeCode::Evaluator::NativeCaller::integer_regs({
	x86::rdi,
	x86::rsi,
	x86::rdx,
	x86::rcx,
	x86::r8,
	x86::r9
});

vector<X86XmmReg> BytecodeCode::Evaluator::NativeCaller::double_regs({
	x86::xmm0,
	x86::xmm1,
	x86::xmm2,
	x86::xmm3,
	x86::xmm4,
	x86::xmm5,
	x86::xmm6,
	x86::xmm7
});

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
	stack.emplace(global.constantById(id).c_str());
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
	auto left {stack.top().d()}; stack.pop();
	auto right{stack.top().d()}; stack.pop();
	stack.emplace(left + right);
}

void BytecodeCode::Evaluator::processIADD(size_t) {
	auto left {stack.top().i()}; stack.pop();
	auto right{stack.top().i()}; stack.pop();
	stack.emplace(left + right);
}

void BytecodeCode::Evaluator::processDSUB(size_t) {
	auto left {stack.top().d()}; stack.pop();
	auto right{stack.top().d()}; stack.pop();
	stack.emplace(left - right);
}

void BytecodeCode::Evaluator::processISUB(size_t) {
	auto left {stack.top().i()}; stack.pop();
	auto right{stack.top().i()}; stack.pop();
	stack.emplace(left - right);
}

void BytecodeCode::Evaluator::processDMUL(size_t) {
	auto left {stack.top().d()}; stack.pop();
	auto right{stack.top().d()}; stack.pop();
	stack.emplace(left * right);
}

void BytecodeCode::Evaluator::processIMUL(size_t) {
	auto left {stack.top().i()}; stack.pop();
	auto right{stack.top().i()}; stack.pop();
	stack.emplace(left * right);
}

void BytecodeCode::Evaluator::processDDIV(size_t) {
	auto left {stack.top().d()}; stack.pop();
	auto right{stack.top().d()}; stack.pop();
	stack.emplace(left / right);
}

void BytecodeCode::Evaluator::processIDIV(size_t) {
	auto left {stack.top().i()}; stack.pop();
	auto right{stack.top().i()}; stack.pop();
	stack.emplace(left / right);
}

void BytecodeCode::Evaluator::processIMOD(size_t) {
	auto left {stack.top().i()}; stack.pop();
	auto right{stack.top().i()}; stack.pop();
	stack.emplace(left % right);
}

void BytecodeCode::Evaluator::processDNEG(size_t) {
	auto op{stack.top().d()}; stack.pop();
	stack.emplace(-op);
}

void BytecodeCode::Evaluator::processINEG(size_t) {
	auto op{stack.top().i()}; stack.pop();
	stack.emplace(-op);
}

void BytecodeCode::Evaluator::processIAOR(size_t) {
	auto left {stack.top().i()}; stack.pop();
	auto right{stack.top().i()}; stack.pop();
	stack.emplace(left | right);
}

void BytecodeCode::Evaluator::processIAAND(size_t) {
	auto left {stack.top().i()}; stack.pop();
	auto right{stack.top().i()}; stack.pop();
	stack.emplace(left & right);
}

void BytecodeCode::Evaluator::processIAXOR(size_t) {
	auto left {stack.top().i()}; stack.pop();
	auto right{stack.top().i()}; stack.pop();
	stack.emplace(left ^ right);
}

void BytecodeCode::Evaluator::processIPRINT(size_t) {
	auto op{stack.top().i()}; stack.pop();
	cout << op << flush;
}

void BytecodeCode::Evaluator::processDPRINT(size_t) {
	auto op{stack.top().d()}; stack.pop();
	cout << op << flush;
}

void BytecodeCode::Evaluator::processSPRINT(size_t) {
	auto op{stack.top().s()}; stack.pop();
	cout << op << flush;
}

void BytecodeCode::Evaluator::processI2D(size_t) {
	auto op{stack.top().i()}; stack.pop();
	stack.emplace(static_cast<double>(op));
}

void BytecodeCode::Evaluator::processD2I(size_t) {
	auto op{stack.top().d()}; stack.pop();
	stack.emplace(static_cast<int64_t>(op));
}

void BytecodeCode::Evaluator::processS2I(size_t) {
	auto op{stack.top().s()}; stack.pop();
// 1. String-to-int version
/*
	try {
		stack.emplace(static_cast<int64_t>(atoll(op)));
	} catch (exception &e) {
		status = StatusEx::Error("S2I failed on \""s + op + "\"");
	}
*/
// 2. Pointer-to-int version
	stack.emplace(reinterpret_cast<int64_t>(op));
}

void BytecodeCode::Evaluator::processSWAP(size_t) {
	auto left {stack.top().i()}; stack.pop();
	auto right{stack.top().i()}; stack.pop();
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
	status = StatusEx::Error("unsupported " #name "VAR" #id " at " + to_string(frame().ip)); \
}
FOR_TYPES(LOAD, FOR_IDS, PROCESS_LOAD)
#undef  PROCESS_LOAD

#define PROCESS_STORE(name, id) \
void BytecodeCode::Evaluator::process##name##VAR##id(size_t) { \
	status = StatusEx::Error("unsupported " #name "VAR" #id " at " + to_string(frame().ip)); \
}
FOR_TYPES(STORE, FOR_IDS, PROCESS_STORE)
#undef  PROCESS_STORE

#define PROCESS_LOAD(name) \
void BytecodeCode::Evaluator::process##name##VAR(size_t) { \
	status = StatusEx::Error("unsupported " #name "VAR at " + to_string(frame().ip)); \
}
FOR_TYPES(LOAD, APPLY, PROCESS_LOAD)
#undef  PROCESS_LOAD

#define PROCESS_STORE(name) \
void BytecodeCode::Evaluator::process##name##VAR(size_t) { \
	status = StatusEx::Error("unsupported " #name "VAR at " + to_string(frame().ip)); \
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
	auto left {stack.top().d()}; stack.pop();
	auto right{stack.top().d()}; stack.pop();
	stack.emplace(sign(left - right));
}

void BytecodeCode::Evaluator::processICMP(size_t) {
	auto left {stack.top().i()}; stack.pop();
	auto right{stack.top().i()}; stack.pop();
	stack.emplace(sign(left - right));
}

void BytecodeCode::Evaluator::processJA(size_t) {
	auto value{frame().code().getInt16(frame().ip)};
	frame().ip += value;
}

void BytecodeCode::Evaluator::processIFICMPNE(size_t adj_size) {
	auto left {stack.top().i()}; stack.pop();
	auto right{stack.top().i()}; stack.pop();
	auto value{frame().code().getInt16(frame().ip)};
	if (left != right)
		frame().ip += value;
	else
		frame().ip += adj_size;
}

void BytecodeCode::Evaluator::processIFICMPE(size_t adj_size) {
	auto left {stack.top().i()}; stack.pop();
	auto right{stack.top().i()}; stack.pop();
	auto value{frame().code().getInt16(frame().ip)};
	if (left == right)
		frame().ip += value;
	else
		frame().ip += adj_size;
}

void BytecodeCode::Evaluator::processIFICMPG(size_t adj_size) {
	auto left {stack.top().i()}; stack.pop();
	auto right{stack.top().i()}; stack.pop();
	auto value{frame().code().getInt16(frame().ip)};
	if (left > right)
		frame().ip += value;
	else
		frame().ip += adj_size;
}

void BytecodeCode::Evaluator::processIFICMPGE(size_t adj_size) {
	auto left {stack.top().i()}; stack.pop();
	auto right{stack.top().i()}; stack.pop();
	auto value{frame().code().getInt16(frame().ip)};
	if (left >= right)
		frame().ip += value;
	else
		frame().ip += adj_size;
}

void BytecodeCode::Evaluator::processIFICMPL(size_t adj_size) {
	auto left {stack.top().i()}; stack.pop();
	auto right{stack.top().i()}; stack.pop();
	auto value{frame().code().getInt16(frame().ip)};
	if (left < right)
		frame().ip += value;
	else
		frame().ip += adj_size;
}

void BytecodeCode::Evaluator::processIFICMPLE(size_t adj_size) {
	auto left {stack.top().i()}; stack.pop();
	auto right{stack.top().i()}; stack.pop();
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

void BytecodeCode::Evaluator::processCALL(size_t adj_size) {
	auto value{frame().code().getInt16(frame().ip)};
	frame().ip += adj_size;

	frames.emplace_back(*static_cast<TranslatedFunction*>(
		global.functionById(value)
	));
}

void BytecodeCode::Evaluator::processCALLNATIVE(size_t adj_size) {
	auto value{frame().code().getInt16(frame().ip)};
	frame().ip += adj_size;
	native_caller_for(value)(stack);
}

void BytecodeCode::Evaluator::processRETURN(size_t) {
	frames.pop_back();
}

void BytecodeCode::Evaluator::processBREAK(size_t) {}

Status *BytecodeCode::execute(vector<Var*> &global_vars) {
	Evaluator e(*this);

	// Priming the execution
	auto root_scope_id{e.frames[0].function.scopes[1]};
	// Wait, why 1? 0 is FunctionNode's scope with arguments, 1 is body scope!
	auto const &root_scope{scopes[root_scope_id]};
	for (auto const *v: global_vars) {
		auto var_id{root_scope.at(v->name())};
		e.frames[0].vars[root_scope_id][var_id] = *v;
	}

	while (e.status == nullptr || !e.status->isError()) {
		if (e.frames.size() == 0) {
			// return from top...
			e.status = Status::Ok();
			break;
		}
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
		for (auto *v: global_vars) {
			auto var_id{root_scope.at(v->name())};
			assign(*v, e.frames[0].vars[root_scope_id][var_id]);
		}
	}
	return e.status;
}

void BytecodeCode::disassemble(ostream &out, FunctionFilter *filter) {
	for (auto it{NativeFunctionIterator(this)}; it.hasNext(); ) {
		auto const &n{it.next()};
		out << "n*" << makeNativeFunction(
			n.name(), n.signature(), const_cast<void*>(n.code())
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

uint16_t BytecodeCode::makeNativeFunction(string const &name, Signature const &sign, void *addr) {
	auto id{Code::makeNativeFunction(name, sign, addr)};
	if (natives.count(id) == 0) {
		natives.emplace(id, NativeFunctionDescriptor(name, sign, addr));
	}
	return id;
}

NativeFunctionDescriptor const &BytecodeCode::nativeById(uint16_t id) const {
	return natives.at(id);
}

}}
