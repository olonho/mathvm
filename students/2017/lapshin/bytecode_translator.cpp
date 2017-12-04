#include "bytecode_translator.h"

#include "bytecode.h"
#include "code.h"
#include "util.h"

#include "parser.h"

#include <unordered_map>

namespace mathvm::ldvsoft {

using namespace std::literals;

#define ASSIGN_STATUS(expr) \
	do { \
		status = expr; \
		if (status->isError()) \
			return; \
		delete status; \
	} while (0);

class BytecodeTranslator::TranslationData {
public:
	BytecodeCode &code;
	unordered_map<Scope const*, uint16_t> scopes;
	unordered_map<AstVar const*, uint16_t> vars;

	TranslationData(BytecodeCode &code):
		code(code) {}
};

class BytecodeTranslator::Visitor : public AstBaseVisitor {
public:
	Bytecode code;
	TranslationData &target;
	VarTypeEx return_type;
	Status *status{nullptr};

	Status *add_and_cast(Visitor const &src, VarTypeEx to, uint32_t position);

	Visitor(TranslationData &target):
		target{target} {}
	virtual ~Visitor() override = default;

#	define VISITOR_FUNCTION(type, name) \
	virtual void visit##type(type *node) override;
	FOR_NODES(VISITOR_FUNCTION)
#	undef  VISITOR_FUNCTION
};

Status *BytecodeTranslator::Visitor::add_and_cast(Visitor const &src, VarTypeEx to, uint32_t position) {
	code.addAll(src.code);
	return code.addCast(src.return_type, to, position);
}

void BytecodeTranslator::Visitor::visitBinaryOpNode(BinaryOpNode *node) {
	Visitor left_visitor(target), right_visitor(target);
	node->left() ->visit(&left_visitor);
	ASSIGN_STATUS(left_visitor.status);
	node->right()->visit(&right_visitor);
	ASSIGN_STATUS(right_visitor.status);

	VarTypeEx target_type;
	switch (node->kind()) {
	case tAND:
	case tOR:
		return_type = target_type = VTE_BOOL;
		break;
	case tAAND:
	case tAOR:
	case tAXOR:
		return_type = target_type = VTE_INT;
		break;
	case tEQ:
	case tNEQ:
	case tLT:
	case tLE:
	case tGT:
	case tGE:
		return_type = VTE_BOOL;
		target_type = common_of(left_visitor.return_type, right_visitor.return_type);
		if (target_type == VTE_INVALID || target_type == VTE_STRING) {
			status = StatusEx::Error(
				"Cannot apply binary operation "s + tokenStr(node->kind()) +
				" on types " + to_string(left_visitor.return_type) + " and " + to_string(right_visitor.return_type) +
				" (common deduced type: " + to_string(target_type) + ")",
				node->position()
			);
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
		code.addInsn(BC_IAOR);
		break;
	case tEQ:
	case tNEQ:
	case tLT:
	case tLE:
	case tGT:
	case tGE:
		if (target_type == VTE_DOUBLE) {
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
				;
			}
			code.addInsn(BC_ILOAD0);
			code.addBranch(BC_JA, _end);
			code.bind(_then);
			code.addInsn(BC_ILOAD1);
			code.bind(_end);
		}
		break;
	default:
		status = StatusEx::Error("Unknown binary op kind: "s + tokenStr(node->kind()), node->position());
		return;
	}
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitUnaryOpNode(UnaryOpNode *node) {
	Visitor operand_visitor(target);
	node->operand()->visit(&operand_visitor);
	ASSIGN_STATUS(operand_visitor.status);

	VarTypeEx target_type;
	switch (node->kind()) {
	case tNOT:
		return_type = target_type = VTE_BOOL;
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
	default:
		;
	}
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitStringLiteralNode(StringLiteralNode *node) {
	return_type = VTE_STRING;

	auto id{target.code.makeStringConstant(node->literal())};
	code.addInsn(BC_SLOAD);
	code.addTyped(id);
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
	return_type = VTE_DOUBLE;
	code.addInsn(BC_DLOAD);
	code.addTyped(node->literal());
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitIntLiteralNode(IntLiteralNode *node) {
	return_type = VTE_DOUBLE;
	code.addInsn(BC_ILOAD);
	code.addTyped(node->literal());
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitLoadNode(LoadNode *node) {
	return_type = extend(node->var()->type());
	switch (return_type) {
	case VTE_DOUBLE:
		code.addInsn(BC_LOADCTXDVAR);
		break;
	case VTE_INT:
	case VTE_BOOL:
		code.addInsn(BC_LOADCTXIVAR);
		break;
	case VTE_STRING:
		code.addInsn(BC_LOADCTXSVAR);
		break;
	default:
		status = Status::Error("Wrong var type", node->position());
	}
	code.addTyped(target.scopes[node->var()->owner()]);
	code.addTyped(target.vars[node->var()]);
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitStoreNode(StoreNode *node) {
	return_type = VTE_VOID;
	auto var_type{extend(node->var()->type())};

	Visitor value_visitor(target);
	node->value()->visit(&value_visitor);
	ASSIGN_STATUS(value_visitor.status);

	ASSIGN_STATUS(add_and_cast(value_visitor, var_type, node->position()));

	switch (node->op()) {
	case tINCRSET:
	case tDECRSET:
		switch (var_type) {
		case VTE_DOUBLE:
			code.addInsn(BC_LOADCTXDVAR);
			break;
		case VTE_INT:
		case VTE_BOOL:
			code.addInsn(BC_LOADCTXIVAR);
			break;
		default:
			status = Status::Error("Wrong var type for change-and-set", node->position());
			return;
		}
		code.addTyped(target.scopes[node->var()->owner()]);
		code.addTyped(target.vars[node->var()]);
		switch (node->op()) {
		case tINCRSET:
			switch (var_type) {
			case VTE_DOUBLE:
				code.addInsn(BC_DADD);
				break;
			case VTE_INT:
			case VTE_BOOL:
				code.addInsn(BC_IADD);
				break;
			default:
				;
			}
			break;
		case tDECRSET:
			switch (var_type) {
			case VTE_DOUBLE:
				code.addInsn(BC_DSUB);
				break;
			case VTE_INT:
			case VTE_BOOL:
				code.addInsn(BC_ISUB);
				break;
			default:
				;
			}
			break;
		default:
			;
		}
		[[fallthrough]];
	case tASSIGN:
		switch (var_type) {
		case VTE_DOUBLE:
			code.addInsn(BC_LOADCTXDVAR);
			break;
		case VTE_INT:
		case VTE_BOOL:
			code.addInsn(BC_LOADCTXIVAR);
			break;
		default:
			status = Status::Error("Wrong var type for change-and-set", node->position());
			return;
		}
		code.addTyped(target.scopes[node->var()->owner()]);
		code.addTyped(target.vars[node->var()]);
		break;
	default:
		status = StatusEx::Error("Unknown assignment operator: "s + tokenStr(node->op()));
		return;
	}
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitForNode(ForNode *node) {
	return_type = VTE_VOID;

	auto var_type{extend(node->var()->type())};
	auto range{dynamic_cast<BinaryOpNode*>(node->inExpr())};
	if (range == nullptr || range->kind() != tRANGE) {
		status = Status::Error("For are only supported with explicit ranges");
		return;
	}

	Visitor left_visitor(target), right_visitor(target), block_visitor(target);
	range->left()->visit(&left_visitor);
	ASSIGN_STATUS(left_visitor.status);
	range->right()->visit(&right_visitor);
	ASSIGN_STATUS(right_visitor.status);
	node->body()->visit(&block_visitor);
	ASSIGN_STATUS(block_visitor.status);

	Bytecode store, load, dupl, precmp, inc;
	/* init store/load */ {
		switch (var_type) {
		case VTE_DOUBLE:
			store.addInsn(BC_STORECTXDVAR);
			load.addInsn(BC_LOADCTXDVAR);
			dupl.addInsns({
				BC_STOREDVAR0,
				BC_LOADDVAR0,
				BC_LOADDVAR0
			});
			precmp.addInsns({
				BC_DCMP,
				BC_ILOAD0
			});
			inc.addInsns({
				BC_DLOAD1,
				BC_DADD
			});
			break;
		case VTE_INT:
		case VTE_BOOL:
			store.addInsn(BC_STORECTXIVAR);
			load.addInsn(BC_LOADCTXIVAR);
			dupl.addInsns({
				BC_STOREIVAR0,
				BC_LOADIVAR0,
				BC_LOADIVAR0
			});
			inc.addInsns({
				BC_ILOAD1,
				BC_IADD
			});
			break;
		default:
			status = StatusEx::Error(
				"Type " + to_string(var_type) + " is wrong for for loop",
				node->position()
			);
			return;
		}
		store.addTyped(target.scopes[node->var()->owner()]);
		store.addTyped(target.vars[node->var()]);
		load .addTyped(target.scopes[node->var()->owner()]);
		load .addTyped(target.vars[node->var()]);
	}

	Label _loop, _end;
	ASSIGN_STATUS(add_and_cast(right_visitor, var_type, node->position()));
	// Now, end value is on the stack. Carefully...

	ASSIGN_STATUS(add_and_cast(left_visitor, var_type, node->position()));
	code.addAll(store);
	code.bind(_loop);
	code.addAll(dupl); // limit as right
	code.addAll(load); // curret as left
	code.addAll(precmp);
	code.addBranch(BC_IFICMPG, _end);
	ASSIGN_STATUS(add_and_cast(block_visitor, VTE_VOID, node->position()));
	code.addAll(load);
	code.addAll(inc);
	code.addBranch(BC_JA, _loop);
	code.bind(_end);

	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitWhileNode(WhileNode *node) {
	return_type = VTE_VOID;

	Visitor expr_visitor(target), block_visitor(target);
	node->whileExpr()->visit(&expr_visitor);
	if (expr_visitor.status->isError()) {
		status = expr_visitor.status;
		return;
	}
	delete expr_visitor.status;
	node->loopBlock()->visit(&block_visitor);
	if (block_visitor.status->isError()) {
		status = block_visitor.status;
		return;
	}
	delete block_visitor.status;

	Label _loop, _end;
	code.bind(_loop);
	status = add_and_cast(expr_visitor, VTE_BOOL, node->position());
	if (status->isError())
		return;
	delete status;

	code.addInsn(BC_ILOAD0);
	code.addBranch(BC_IFICMPE, _end);
	status = add_and_cast(block_visitor, VTE_VOID, node->position());
	if (status->isError())
		return;
	delete status;

	code.addBranch(BC_JA, _loop);
	code.bind(_end);
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitIfNode(IfNode *node) {
	return_type = VTE_VOID;

	Visitor expr_visitor(target), then_visitor(target), else_visitor(target);
	node->ifExpr()->visit(&expr_visitor);
	if (expr_visitor.status->isError()) {
		status = expr_visitor.status;
		return;
	}
	delete expr_visitor.status;
	node->thenBlock()->visit(&then_visitor);
	if (then_visitor.status->isError()) {
		status = then_visitor.status;
		return;
	}
	delete then_visitor.status;
	if (node->elseBlock()) {
		node->elseBlock()->visit(&else_visitor);
		if (else_visitor.status->isError()) {
			status = else_visitor.status;
			return;
		}
		delete else_visitor.status;
	}

	status = add_and_cast(expr_visitor, VTE_BOOL, node->position());
	if (status->isError())
		return;
	delete status;

	if (node->elseBlock()) {
		Label _else, _end;

		code.addInsn(BC_ILOAD0);
		code.addBranch(BC_IFICMPE, _else);
		status = add_and_cast(then_visitor, VTE_VOID, node->position());
		if (status->isError())
			return;
		delete status;

		code.addBranch(BC_JA, _end);
		code.bind(_else);
		status = add_and_cast(else_visitor, VTE_VOID, node->position());
		if (status->isError())
			return;

		code.bind(_end);
	} else {
		Label _end;

		code.addInsn(BC_ILOAD0);
		code.addBranch(BC_IFICMPE, _end);
		status = add_and_cast(then_visitor, VTE_VOID, node->position());
		if (status->isError())
			return;

		code.bind(_end);
	}
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitBlockNode(BlockNode *node) {
	return_type = VTE_VOID;
	for (size_t i{0}; i < node->nodes(); ++i) {
		Visitor node_visitor(target);
		node->nodeAt(i)->visit(&node_visitor);

		status = add_and_cast(node_visitor, VTE_VOID, node->position());
		if (status->isError())
			return;
		delete status;
	}
	status = Status::Ok();
}

void BytecodeTranslator::Visitor::visitFunctionNode(FunctionNode *node) {
	assert(false);
}

void BytecodeTranslator::Visitor::visitReturnNode(ReturnNode *node) {
	Visitor expr_visitor(target);
	node->returnExpr()->visit(&expr_visitor);
	if (expr_visitor.status->isError()) {
		status = expr_visitor.status;
		return;
	}
	delete expr_visitor.status;

	status = add_and_cast(expr_visitor, VTE_VOID, node->position());
	if (status->isError())
		return;
	code.addInsn(BC_RETURN);
}

void BytecodeTranslator::Visitor::visitCallNode(CallNode *node) {
	assert(false);
}

void BytecodeTranslator::Visitor::visitNativeCallNode(NativeCallNode *node) {
	assert(false);
}

void BytecodeTranslator::Visitor::visitPrintNode(PrintNode *node) {
	assert(false);
}

Status *BytecodeTranslator::translate(string const &program, mathvm::Code **code_ptr) {
	Parser parser;
	auto status{parser.parseProgram(program)};
	if (status->isError())
		return status;
	delete status;

	auto const &parsed{parser.top()};
	TranslationData target(*new BytecodeCode());
	Visitor visitor(target);
	*code_ptr = &target.code;
	parsed->node()->visit(&visitor);
	if (visitor.status->isError()) {
		return visitor.status;
	}
};

}
