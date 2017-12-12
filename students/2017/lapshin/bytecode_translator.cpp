#include "bytecode_translator.h"

#include "bytecode.h"
#include "code.h"
#include "util.h"

#include "parser.h"

#include <dlfcn.h>

#include <unordered_map>

namespace mathvm::ldvsoft {

using namespace std::literals;

#define ASSIGN_STATUS(expr) \
	do { \
		status = expr; \
		if (status->isError()) \
			return; \
		delete status; \
	} while (false)

#define NEVER() \
	do { \
		status = StatusEx::Error( \
			"THE MISSED CASE AT LINE " + to_string(__LINE__), \
			node->position() \
		); \
	} while (false)

class BytecodeTranslator::TranslationData {
public:
	void * const dl_handle;

	BytecodeCode &code;
	unordered_map<Scope const*, uint16_t> scopes;

	TranslationData(BytecodeCode &code):
		dl_handle(dlopen(nullptr, RTLD_LAZY | RTLD_NODELETE)),
		code(code) {}

	~TranslationData() {
		dlclose(dl_handle);
	}

	void registerScope(Scope *scope) {
		if (scopes.count(scope) > 0)
			return;
		scopes[scope] = scopes.size(); // C++17!!!
		auto &scope_desc{code.scopes[scopes[scope]]};
		assert(scopes.size() == code.scopes.size());
		for (size_t i{0}; i != scope->childScopeNumber(); ++i)
			registerScope(scope->childScopeAt(i));
		for (auto it{Scope::VarIterator(scope)}; it.hasNext(); )
			scope_desc[it.next()->name()] = scope_desc.size();
		for (auto it{Scope::FunctionIterator(scope)}; it.hasNext(); )
			code.addFunction(new BytecodeCode::TranslatedFunction(it.next()));
	}
};

class BytecodeTranslator::Visitor : public AstBaseVisitor {
public:
	Bytecode code;
	vector<size_t> &scopes;
	VarTypeEx function_return_type;
	Scope &function_scope;
	TranslationData &target;

	VarTypeEx return_type;
	Status *status{nullptr};

	Visitor(VarTypeEx function_return_type, Scope &function_scope, vector<size_t> &scopes, TranslationData &target):
		scopes{scopes}, function_return_type{function_return_type}, function_scope{function_scope}, target{target} {}
	virtual ~Visitor() override = default;

#	define VISITOR_FUNCTION(type, name) \
	virtual void visit##type(type *node) override;
	FOR_NODES(VISITOR_FUNCTION)
#	undef  VISITOR_FUNCTION

private:
	Status *add_and_cast(Visitor const &src, VarTypeEx to, uint32_t position);
	enum VarOp {
		VO_LOAD,
		VO_STORE,
		VO_ONLY_ID
	};
	Status *addVarId(VarOp op, AstVar const *var, uint32_t position);
	Status *addVarIdTo(VarOp op, AstVar const *var, uint32_t position, Bytecode &code);
	Visitor(Visitor *source):
		Visitor(source->function_return_type, source->function_scope, source->scopes, source->target) {}
};

Status *BytecodeTranslator::Visitor::add_and_cast(Visitor const &src, VarTypeEx to, uint32_t position) {
	code.addAll(src.code);
	return code.addCast(src.return_type, to, position);
}

Status *BytecodeTranslator::Visitor::addVarId(VarOp op, AstVar const* var, uint32_t position) {
	return addVarIdTo(op, var, position, code);
}

Status *BytecodeTranslator::Visitor::addVarIdTo(VarOp op, AstVar const* var, uint32_t position, Bytecode &code) {
	if (op != VO_ONLY_ID) {
		bool load{op == VO_LOAD};
		switch (var->type()) {
		case VT_DOUBLE:
			code.addInsn(load ? BC_LOADCTXDVAR : BC_STORECTXDVAR);
			break;
		case VT_INT:
			code.addInsn(load ? BC_LOADCTXIVAR : BC_STORECTXIVAR);
			break;
		case VT_STRING:
			code.addInsn(load ? BC_LOADCTXSVAR : BC_STORECTXSVAR);
			break;
		default:
			return Status::Error("Wrong var type", position);
		}
	}
	auto scope_id{target.scopes[var->owner()]};
	code.addTyped(scope_id);
	code.addTyped(target.code.scopes[scope_id].at(var->name()));
	return Status::Ok();
}

void BytecodeTranslator::Visitor::visitBinaryOpNode(BinaryOpNode *node) {
	Visitor left_visitor(this), right_visitor(this);
	node->left() ->visit(&left_visitor);
	ASSIGN_STATUS(left_visitor.status);
	node->right()->visit(&right_visitor);
	ASSIGN_STATUS(right_visitor.status);

	VarTypeEx target_type;
	switch (node->kind()) {
	case tAND:
	case tOR:
		return_type = target_type = VarTypeEx::BOOL;
		break;
	case tAAND:
	case tAOR:
	case tAXOR:
		return_type = target_type = VarTypeEx::INT;
		break;
	case tADD:
	case tSUB:
	case tMUL:
	case tDIV:
		return_type = target_type = common_of(VarTypeEx::INT, common_of(left_visitor.return_type, right_visitor.return_type));
		if (target_type == VarTypeEx::INVALID || target_type == VarTypeEx::STRING) {
			status = StatusEx::Error(
				"Cannot apply binary operation "s + tokenStr(node->kind()) +
				" on types " + to_string(left_visitor.return_type) + " and " + to_string(right_visitor.return_type) +
				" (common deduced type: " + to_string(target_type) + ")",
				node->position()
			);
			return;
		}
		break;
	case tMOD:
		return_type = target_type = VarTypeEx::INT;
		break;
	case tEQ:
	case tNEQ:
	case tLT:
	case tLE:
	case tGT:
	case tGE:
		return_type = VarTypeEx::BOOL;
		target_type = common_of(left_visitor.return_type, right_visitor.return_type);
		if (target_type == VarTypeEx::INVALID || target_type == VarTypeEx::STRING) {
			status = StatusEx::Error(
				"Cannot apply binary operation "s + tokenStr(node->kind()) +
				" on types " + to_string(left_visitor.return_type) + " and " + to_string(right_visitor.return_type) +
				" (common deduced type: " + to_string(target_type) + ")",
				node->position()
			);
			return;
		}
		break;
	default:
		status = StatusEx::Error("Unknown binary op kind: "s + tokenStr(node->kind()), node->position());
		return;
	}

	// SIC: operands are pushed in reverse...
	ASSIGN_STATUS(add_and_cast(right_visitor, target_type, node->position()));
	ASSIGN_STATUS(add_and_cast(left_visitor , target_type, node->position()));

	switch (node->kind()) {
	case tAND:
	case tAAND:
		code.addInsn(BC_IAAND);
		break;
	case tOR:
	case tAOR:
		code.addInsn(BC_IAOR);
		break;
	case tAXOR:
		code.addInsn(BC_IAXOR);
		break;
	case tADD:
		switch (target_type) {
		case VarTypeEx::DOUBLE:
			code.addInsn(BC_DADD);
			break;
		case VarTypeEx::INT:
			code.addInsn(BC_IADD);
			break;
		default:
			NEVER();
		}
		break;
	case tSUB:
		switch (target_type) {
		case VarTypeEx::DOUBLE:
			code.addInsn(BC_DSUB);
			break;
		case VarTypeEx::INT:
			code.addInsn(BC_ISUB);
			break;
		default:
			NEVER();
		}
		break;
	case tMUL:
		switch (target_type) {
		case VarTypeEx::DOUBLE:
			code.addInsn(BC_DMUL);
			break;
		case VarTypeEx::INT:
			code.addInsn(BC_IMUL);
			break;
		default:
			NEVER();
		}
		break;
	case tDIV:
		switch (target_type) {
		case VarTypeEx::DOUBLE:
			code.addInsn(BC_DDIV);
			break;
		case VarTypeEx::INT:
			code.addInsn(BC_IDIV);
			break;
		default:
			NEVER();
		}
		break;
	case tMOD:
		code.addInsn(BC_IMOD);
		break;
	case tEQ:
	case tNEQ:
	case tLT:
	case tLE:
	case tGT:
	case tGE:
		if (target_type == VarTypeEx::DOUBLE) {
			// cannot if-compare doubles directly...
			code.addInsns({
				BC_DCMP,
				BC_ILOAD0
			});
		}
		/* cmp via jumps */ {
			Label _then(&code), _end(&code);
			switch (node->kind()) {
			case tEQ : code.addBranch(BC_IFICMPE , _then); break;
			case tNEQ: code.addBranch(BC_IFICMPNE, _then); break;
			case tLT : code.addBranch(BC_IFICMPL , _then); break;
			case tLE : code.addBranch(BC_IFICMPLE, _then); break;
			case tGT : code.addBranch(BC_IFICMPG , _then); break;
			case tGE : code.addBranch(BC_IFICMPGE, _then); break;
			default:
				NEVER();
			}
			code.addInsn(BC_ILOAD0);
			code.addBranch(BC_JA, _end);
			code.bind(_then);
			code.addInsn(BC_ILOAD1);
			code.bind(_end);
		}
		break;
	default:
		status = StatusEx::Error("Unsupported binary op kind: "s + tokenStr(node->kind()), node->position());
		return;
	}
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitUnaryOpNode(UnaryOpNode *node) {
	Visitor operand_visitor(this);
	node->operand()->visit(&operand_visitor);
	ASSIGN_STATUS(operand_visitor.status);

	VarTypeEx target_type;
	switch (node->kind()) {
	case tNOT:
		return_type = target_type = VarTypeEx::BOOL;
		break;
	case tADD:
	case tSUB:
		return_type = target_type = common_of(VarTypeEx::INT, operand_visitor.return_type);
		if (target_type != VarTypeEx::INT && target_type != VarTypeEx::DOUBLE) {
			status = StatusEx::Error(
				"Cannot apply unary operation "s + tokenStr(node->kind()) +
				" on type " + to_string(operand_visitor.return_type),
				node->position()
			);
			return;
		}
		break;
	default:
		status = StatusEx::Error("Unknown unary op kind: "s + tokenStr(node->kind()), node->position());
		return;
	}

	ASSIGN_STATUS(add_and_cast(operand_visitor, target_type, node->position()));

	switch (node->kind()) {
	case tNOT:
		code.addInsns({
			BC_ILOAD1,
			BC_IAXOR
		});
		break;
	case tADD:
		// NOOP
		break;
	case tSUB:
		switch (target_type) {
		case VarTypeEx::DOUBLE:
			code.addInsn(BC_DNEG);
			break;
		case VarTypeEx::INT:
			code.addInsn(BC_INEG);
			break;
		default:
			NEVER();
		}
		break;
	default:
		NEVER();
	}
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitStringLiteralNode(StringLiteralNode *node) {
	return_type = VarTypeEx::STRING;

	auto id{target.code.makeStringConstant(node->literal())};
	code.addInsn(BC_SLOAD);
	code.addTyped(id);
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
	return_type = VarTypeEx::DOUBLE;
	code.addInsn(BC_DLOAD);
	code.addTyped(node->literal());
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitIntLiteralNode(IntLiteralNode *node) {
	return_type = VarTypeEx::INT;
	code.addInsn(BC_ILOAD);
	code.addTyped(node->literal());
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitLoadNode(LoadNode *node) {
	return_type = extend(node->var()->type());
	status = addVarId(VO_LOAD, node->var(), node->position());
}

void BytecodeTranslator::Visitor::visitStoreNode(StoreNode *node) {
	return_type = VarTypeEx::VOID;
	auto var_type{node->var()->type()};

	Visitor value_visitor(this);
	node->value()->visit(&value_visitor);
	ASSIGN_STATUS(value_visitor.status);

	ASSIGN_STATUS(add_and_cast(value_visitor, extend(var_type), node->position()));

	switch (node->op()) {
	case tINCRSET:
	case tDECRSET:
		if (var_type != VT_DOUBLE && var_type != VT_INT) {
			status = StatusEx::Error(
				"Wrong var type "s + typeToName(var_type) + " for change-and-set",
				node->position()
			);
			return;
		}
		ASSIGN_STATUS(addVarId(VO_LOAD, node->var(), node->position()));
		switch (node->op()) {
		case tINCRSET:
			switch (var_type) {
			case VT_DOUBLE:
				code.addInsn(BC_DADD);
				break;
			case VT_INT:
				code.addInsn(BC_IADD);
				break;
			default:
				NEVER();
			}
			break;
		case tDECRSET:
			switch (var_type) {
			case VT_DOUBLE:
				code.addInsn(BC_DSUB);
				break;
			case VT_INT:
				code.addInsn(BC_ISUB);
				break;
			default:
				NEVER();
			}
			break;
		default:
			NEVER();
		}
		[[fallthrough]];
	case tASSIGN:
		status = addVarId(VO_STORE, node->var(), node->position());
		return;
	default:
		status = StatusEx::Error("Unknown assignment operator: "s + tokenStr(node->op()));
		return;
	}
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitForNode(ForNode *node) {
	return_type = VarTypeEx::VOID;

	auto var_type{node->var()->type()};
	auto range{dynamic_cast<BinaryOpNode*>(node->inExpr())};
	if (range == nullptr || range->kind() != tRANGE) {
		status = Status::Error("For are only supported with explicit ranges");
		return;
	}

	auto *scope{node->var()->owner()};
	auto name{"_for_limit_" + to_string(node->position())};
	scope->declareVariable(name, var_type);
	auto *limit_var{scope->lookupVariable(name, false)};
	auto &target_scope{target.code.scopes[target.scopes[scope]]};
	target_scope[name] = target_scope.size();
	assert(limit_var);

	Visitor left_visitor(this), right_visitor(this), block_visitor(this);
	range->left()->visit(&left_visitor);
	ASSIGN_STATUS(left_visitor.status);
	range->right()->visit(&right_visitor);
	ASSIGN_STATUS(right_visitor.status);
	node->body()->visit(&block_visitor);
	ASSIGN_STATUS(block_visitor.status);

	Bytecode precmp, inc;
	/* init store/load */ {
		switch (var_type) {
		case VT_DOUBLE:
			precmp.addInsns({
				BC_DCMP,
				BC_ILOAD0
			});
			inc.addInsns({
				BC_DLOAD1,
				BC_DADD
			});
			break;
		case VT_INT:
			inc.addInsns({
				BC_ILOAD1,
				BC_IADD
			});
			break;
		default:
			status = StatusEx::Error(
				"Type "s + typeToName(var_type) + " is wrong for for loop",
				node->position()
			);
			return;
		}
	}

	Label _loop(&code), _end(&code);
	ASSIGN_STATUS(add_and_cast(right_visitor, extend(var_type), node->position()));
	ASSIGN_STATUS(addVarId(VO_STORE, limit_var, node->position()));

	ASSIGN_STATUS(add_and_cast(left_visitor, extend(var_type), node->position()));
	ASSIGN_STATUS(addVarId(VO_STORE, node->var(), node->position()));
	code.bind(_loop);
	ASSIGN_STATUS(addVarId(VO_LOAD, limit_var, node->position()));
	ASSIGN_STATUS(addVarId(VO_LOAD, node->var(), node->position()));
	code.addAll(precmp);
	code.addBranch(BC_IFICMPG, _end);
	ASSIGN_STATUS(add_and_cast(block_visitor, VarTypeEx::VOID, node->position()));
	ASSIGN_STATUS(addVarId(VO_LOAD, node->var(), node->position()));
	code.addAll(inc);
	ASSIGN_STATUS(addVarId(VO_STORE, node->var(), node->position()));
	code.addBranch(BC_JA, _loop);

	code.bind(_end);

	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitWhileNode(WhileNode *node) {
	return_type = VarTypeEx::VOID;

	Visitor expr_visitor(this), block_visitor(this);
	node->whileExpr()->visit(&expr_visitor);
	ASSIGN_STATUS(expr_visitor.status);
	node->loopBlock()->visit(&block_visitor);
	ASSIGN_STATUS(block_visitor.status);

	Label _loop(&code), _end(&code);
	code.bind(_loop);
	ASSIGN_STATUS(add_and_cast(expr_visitor, VarTypeEx::BOOL, node->position()));

	code.addInsn(BC_ILOAD0);
	code.addBranch(BC_IFICMPE, _end);
	ASSIGN_STATUS(add_and_cast(block_visitor, VarTypeEx::VOID, node->position()));

	code.addBranch(BC_JA, _loop);
	code.bind(_end);
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitIfNode(IfNode *node) {
	return_type = VarTypeEx::VOID;

	Visitor expr_visitor(this), then_visitor(this), else_visitor(this);
	node->ifExpr()->visit(&expr_visitor);
	ASSIGN_STATUS(expr_visitor.status);
	node->thenBlock()->visit(&then_visitor);
	ASSIGN_STATUS(then_visitor.status);
	if (node->elseBlock()) {
		node->elseBlock()->visit(&else_visitor);
		ASSIGN_STATUS(else_visitor.status);
	}

	ASSIGN_STATUS(add_and_cast(expr_visitor, VarTypeEx::BOOL, node->position()));

	if (node->elseBlock()) {
		Label _else(&code), _end(&code);

		code.addInsn(BC_ILOAD0);
		code.addBranch(BC_IFICMPE, _else);
		ASSIGN_STATUS(add_and_cast(then_visitor, VarTypeEx::VOID, node->position()));
		code.addBranch(BC_JA, _end);
		code.bind(_else);
		ASSIGN_STATUS(add_and_cast(else_visitor, VarTypeEx::VOID, node->position()));
		code.bind(_end);
	} else {
		Label _end(&code);

		code.addInsn(BC_ILOAD0);
		code.addBranch(BC_IFICMPE, _end);
		ASSIGN_STATUS(add_and_cast(then_visitor, VarTypeEx::VOID, node->position()));
		code.bind(_end);
	}
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitBlockNode(BlockNode *node) {
	return_type = VarTypeEx::VOID;
	scopes.push_back(target.scopes[node->scope()]);
	for (size_t i{0}; i < node->nodes(); ++i) {
		Visitor node_visitor(this);
		node->nodeAt(i)->visit(&node_visitor);
		ASSIGN_STATUS(node_visitor.status);

		VarTypeEx target_type{VarTypeEx::VOID};
		if (dynamic_cast<NativeCallNode*>(node->nodeAt(i))) {
			// Native call knows what it is doing!
			target_type = node_visitor.return_type;
		}
		ASSIGN_STATUS(add_and_cast(node_visitor, target_type, node->position()));
	}
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitFunctionNode(FunctionNode *node) {
	auto scope_id{target.scopes[&function_scope]};
	scopes.push_back(scope_id);
	for (size_t i{0}; i != node->parametersNumber(); ++i) {
		switch (node->parameterType(i)) {
		case VT_DOUBLE:
			code.addInsn(BC_STORECTXDVAR);
			break;
		case VT_INT:
			code.addInsn(BC_STORECTXIVAR);
			break;
		case VT_STRING:
			code.addInsn(BC_STORECTXIVAR);
			break;
		default:
			status = Status::Error("Wrong parameter type");
			return;
		}
		code.addTyped(scope_id);
		code.addTyped(target.code.scopes[scope_id].at(node->parameterName(i)));
	}
	node->body()->visit(this);
}

void BytecodeTranslator::Visitor::visitReturnNode(ReturnNode *node) {
	return_type = VarTypeEx::VOID;
	if (node->returnExpr()) {
		Visitor expr_visitor(this);
		node->returnExpr()->visit(&expr_visitor);
		ASSIGN_STATUS(expr_visitor.status);

		ASSIGN_STATUS(add_and_cast(expr_visitor, function_return_type, node->position()));
	}
	code.addInsn(BC_RETURN);
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitCallNode(CallNode *node) {
	auto *function{target.code.functionByName(node->name())};
	if (!function) {
		status = StatusEx::Error(
			"Unknown function " + node->name(),
			node->position()
		);
		return;
	}
	if (function->parametersNumber() != node->parametersNumber()) {
		status = StatusEx::Error(
			"Wrong number of arguments on " + node->name() + " function call: " +
			"expected " + to_string(function->parametersNumber()) + ", got " + to_string(node->parametersNumber()),
			node->position()
		);
		return;
	}
	return_type = extend(function->returnType());

	for (size_t i{0}; i != node->parametersNumber(); ++i) {
		size_t id{node->parametersNumber() - 1 - i};
		Visitor parameter_visitor(this);
		node->parameterAt(id)->visit(&parameter_visitor);
		ASSIGN_STATUS(parameter_visitor.status);

		ASSIGN_STATUS(add_and_cast(parameter_visitor, extend(function->parameterType(id)), node->position()));
	}

	code.addInsn(BC_CALL);
	code.addTyped(function->id());

	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitNativeCallNode(NativeCallNode *node) {
	// Native calls are hacked into...

	auto const &signature{node->nativeSignature()};

	return_type = extend(signature[0].first);
	for (size_t i{signature.size() - 1 /* sic! */}; i != 0; --i) {
		auto *var{function_scope.lookupVariable(signature[i].second)};
		assert(var);

		ASSIGN_STATUS(addVarId(VO_LOAD, var, node->position()));
	}

	auto addr{dlsym(target.dl_handle, node->nativeName().c_str())};
	if (!addr) {
		status = StatusEx::Error(
			"Cannot load native function " + node->nativeName() + ", reason :" + dlerror(),
			node->position()
		);
		return;
	}

	auto id{target.code.makeNativeFunction(node->nativeName(), node->nativeSignature(), addr)};
	code.addInsn(BC_CALLNATIVE);
	code.addTyped(id);

	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitPrintNode(PrintNode *node) {
	return_type = VarTypeEx::VOID;

	for (size_t i{0}; i < node->operands(); ++i) {
		auto const &op{node->operandAt(i)};
		Visitor operand_visitor(this);
		op->visit(&operand_visitor);
		ASSIGN_STATUS(operand_visitor.status);
		code.addAll(operand_visitor.code);

		switch (shrink(operand_visitor.return_type)) {
		case VT_DOUBLE:
			code.addInsn(BC_DPRINT);
			break;
		case VT_INT:
			code.addInsn(BC_IPRINT);
			break;
		case VT_STRING:
			code.addInsn(BC_SPRINT);
			break;
		default:
			status = StatusEx::Error(
				"Cannot print argument " + to_string(i + 1) + " of type" + to_string(operand_visitor.return_type),
				node->position()
			);
			return;
		}
	}
	status = Status::Ok();
}

Status *BytecodeTranslator::translate(string const &program, mathvm::Code **code_ptr) {
	Parser parser;
	auto status{parser.parseProgram(program)};
	if (status->isError())
		return status;
	delete status;

	auto *parsed{parser.top()};
	TranslationData target(*new BytecodeCode());
	*code_ptr = &target.code;

	target.registerScope(parsed->scope());
	for (auto it{Code::FunctionIterator(&target.code)}; it.hasNext(); ) {
		auto *fun{static_cast<BytecodeCode::TranslatedFunction*>(it.next())};
		auto *function{fun->function};
		Visitor visitor(extend(function->returnType()), *function->scope(), fun->scopes, target);

		function->node()->visit(&visitor);
		status = visitor.status;
		if (status->isError()) {
			return status;
		}
		delete status;
		*fun->bytecode() = visitor.code;
	}
	return Status::Ok();
};

}
