#include "code.h"

namespace mathvm::ldvsoft {

Status *BytecodeCode::execute(vector<Var*> &vars) {
	static_cast<void>(vars);
	return Status::Error("Execution is not supported yet");
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
