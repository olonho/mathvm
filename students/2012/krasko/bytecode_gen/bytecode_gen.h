/*
 * bytecode_gen.h
 *
 *  Created on: 17.11.2012
 *      Author: Evgeniy Krasko
 */
#ifndef BYTECODE_GEN_H_
#define BYTECODE_GEN_H_

#include <string>
#include "parser.h"
#include <map>
#include <sstream>
#include "visitors.h"

using namespace mathvm;
using namespace std;

class BytecodeGen: public AstVisitor {
	Code * code;
	Bytecode * bc;
	int funIndex;
	VarType TOSType, returnType;

	void generateConversionToBoolean() {
		Label load0(bc);
		Label end(bc);
		bc->addInsn(BC_ILOAD0);
		bc->addBranch(BC_IFICMPE, load0);
		bc->addInsn(BC_ILOAD1);
		bc->addBranch(BC_JA, end);
		bc->bind(load0);
		bc->addInsn(BC_ILOAD0);
		bc->bind(end);
	}

	void generateConversion(VarType from, VarType to) {
		if (from == VT_DOUBLE && to == VT_INT) {
			bc->addInsn(BC_D2I);
			TOSType = VT_INT;
		} else if (from == VT_INT && to == VT_DOUBLE) {
			bc->addInsn(BC_I2D);
			TOSType = VT_DOUBLE;
		}
	}

	void generateLoading(const AstVar *var) {
		switch (var->type()) {
		case VT_INT:
			bc->addInsn(BC_LOADCTXIVAR);
			TOSType = VT_INT;
			break;
		case VT_DOUBLE:
			bc->addInsn(BC_LOADCTXDVAR);
			TOSType = VT_DOUBLE;
			break;
		case VT_STRING:
			bc->addInsn(BC_LOADCTXSVAR);
			TOSType = VT_STRING;
			break;
		default:
			break;
		}
		bc->addInt16(varIds[var].first);
		bc->addInt16(varIds[var].second);
	}

	void generateStoring(const AstVar *var) {
		switch (var->type()) {
		case VT_INT:
			bc->addInsn(BC_STORECTXIVAR);
			break;
		case VT_DOUBLE:
			bc->addInsn(BC_STORECTXDVAR);
			break;
		case VT_STRING:
			bc->addInsn(BC_STORECTXSVAR);
			break;
		default:
			break;
		}
		bc->addInt16(varIds[var].first);
		bc->addInt16(varIds[var].second);
	}

	void visitBinaryOpNode(BinaryOpNode * node) {
		node->left()->visit(this);
		VarType left = TOSType;
		node->right()->visit(this);
		VarType right = TOSType;

		Label end(bc);
		Label load1(bc);

		if (left == VT_INT && right == VT_INT) { //integral operation
			switch (node->kind()) {
			case tADD:
				bc->addInsn(BC_IADD);
				break;
			case tMUL:
				bc->addInsn(BC_IMUL);
				break;
			case tSUB:
				bc->addInsn(BC_SWAP);
				bc->addInsn(BC_ISUB);
				break;
			case tDIV:
				bc->addInsn(BC_SWAP);
				bc->addInsn(BC_IDIV);
				break;
			case tMOD:
				bc->addInsn(BC_SWAP);
				bc->addInsn(BC_IMOD);
				break;
			case tOR:
			case tAND:
				generateConversionToBoolean();
				bc->addInsn(BC_SWAP);
				generateConversionToBoolean();
				if (node->kind() == tOR) {
					bc->addInsn(BC_IADD);
					generateConversionToBoolean();
				} else {
					bc->addInsn(BC_IMUL);
				}
				break;
			case tRANGE:
				break;
			case tLE:
			case tLT:
			case tGT:
			case tGE:
			case tEQ:
			case tNEQ:
				switch (node->kind()) {
				case tNEQ:
					bc->addBranch(BC_IFICMPNE, load1);
					break;
				case tEQ:
					bc->addBranch(BC_IFICMPE, load1);
					break;
				case tLE: //args are reversed
					bc->addBranch(BC_IFICMPGE, load1);
					break;
				case tGE:
					bc->addBranch(BC_IFICMPLE, load1);
					break;
				case tLT:
					bc->addBranch(BC_IFICMPG, load1);
					break;
				case tGT:
					bc->addBranch(BC_IFICMPL, load1);
					break;
				default:
					break;
				}
				bc->addInsn(BC_ILOAD0);
				bc->addBranch(BC_JA, end); //skip loading 1: 0 already loaded
				bc->bind(load1);
				bc->addInsn(BC_ILOAD1);
				bc->bind(end);
				break;
			default:
				cerr << "Incorrect int operation" << endl;
				break;
			}
			TOSType = VT_INT;
			return;
		}

		if (left == VT_STRING || right == VT_STRING) {
			cerr << "Logical operation with string" << endl;
			return;
		}

		generateConversion(right, VT_DOUBLE);
		bc->addInsn(BC_SWAP);
		generateConversion(left, VT_DOUBLE);

		switch (node->kind()) { //permorm double op
		case tADD:
			bc->addInsn(BC_DADD);
			break;
		case tMUL:
			bc->addInsn(BC_DMUL);
			break;
		case tSUB:
			bc->addInsn(BC_DSUB);
			break;
		case tDIV:
			bc->addInsn(BC_DDIV);
			break;
		default:
			cerr << "Incorrect double operation" << endl;
			break;
		}
		TOSType = VT_DOUBLE;
	}

	void visitBlockNode(BlockNode * node) {
		declareVariables(node->scope());
		node->visitChildren(this);
	}

	void visitCallNode(CallNode * node) {
		unsigned n = node->parametersNumber();
		TranslatedFunction * fun = code->functionByName(node->name());
		for (unsigned i = 0; i < n; ++i) {
			node->parameterAt(n - i - 1)->visit(this);
			generateConversion(TOSType, fun->parameterType(n - i - 1));
		}
		bc->addInsn(BC_CALL);
		bc->addInt16(fun->id());
		TOSType = fun->returnType();
	}

	void visitDoubleLiteralNode(DoubleLiteralNode * node) {
		bc->addInsn(BC_DLOAD);
		bc->addDouble(node->literal());
		TOSType = VT_DOUBLE;
	}

	void visitForNode(ForNode * node) {
		//init loop var
		node->inExpr()->asBinaryOpNode()->left()->visit(this);
		generateStoring(node->var());
		Label start(bc);
		Label end(bc);
		//compare loop var with right
		bc->bind(start);
		node->inExpr()->asBinaryOpNode()->right()->visit(this);
		generateLoading(node->var());
		bc->addBranch(BC_IFICMPG, end);
		//visit loop body
		node->body()->visit(this);
		//increment
		bc->addInsn(BC_ILOAD1);
		generateLoading(node->var());
		bc->addInsn(BC_IADD);
		generateStoring(node->var());
		//jump to start
		bc->addBranch(BC_JA, start);
		bc->bind(end);
	}

	void visitFunctionNode(FunctionNode * node) {

	}

	void visitIfNode(IfNode * node) {
		Label els(bc);
		Label end(bc);
		node->ifExpr()->visit(this);
		bc->addInsn(BC_ILOAD0);
		bc->addBranch(BC_IFICMPE, els);
		node->thenBlock()->visit(this);
		bc->addBranch(BC_JA, end);
		bc->bind(els);
		if (node->elseBlock()) {
			node->elseBlock()->visit(this);
		}
		bc->bind(end);
	}

	void visitIntLiteralNode(IntLiteralNode * node) {
		bc->addInsn(BC_ILOAD);
		bc->addInt64(node->literal());
		TOSType = VT_INT;
	}

	void visitLoadNode(LoadNode * node) {
		generateLoading(node->var());
	}

	void visitNativeCallNode(NativeCallNode * node) {
	}

	void visitPrintNode(PrintNode * node) {
		for (unsigned i = 0; i < node->operands(); i++) {
			AstNode *operand = node->operandAt(i);
			operand->visit(this);
			switch (TOSType) {
			case VT_INT:
				bc->addInsn(BC_IPRINT);
				break;
			case VT_DOUBLE:
				bc->addInsn(BC_DPRINT);
				break;
			case VT_STRING:
				bc->addInsn(BC_SPRINT);
				break;
			default:
				cerr << "Error in print argument" << endl;
				break;
			}
		}
	}

	void visitReturnNode(ReturnNode * node) {
		node->visitChildren(this);
		generateConversion(TOSType, returnType);
		bc->addInsn(BC_RETURN);
	}

	void visitStoreNode(StoreNode * node) {
		node->value()->visit(this);
		VarType type = node->var()->type();
		generateConversion(TOSType, type);
		switch (node->op()) {
		case tINCRSET:
			generateLoading(node->var());
			if (type == VT_INT)
				bc->addInsn(BC_IADD);
			else
				bc->addInsn(BC_DADD);
			break;
		case tDECRSET:
			generateLoading(node->var());
			if (type == VT_INT)
				bc->addInsn(BC_ISUB);
			else
				bc->addInsn(BC_DSUB);
			break;
		case tASSIGN:
			break;
		default:
			cout << "unknown assignment op" << endl;
			break;
		}
		generateStoring(node->var());
	}

	void visitStringLiteralNode(StringLiteralNode * node) {
		bc->addInsn(BC_SLOAD);
		bc->addInt16(code->makeStringConstant(node->literal()));
		TOSType = VT_STRING;
	}

	void visitUnaryOpNode(UnaryOpNode * node) {
		node->operand()->visit(this);
		switch (node->kind()) {
		case tNOT:
			if (TOSType != VT_INT)
				cerr << "Incorrect operand of logical not" << endl;
			generateConversionToBoolean();
			bc->addInsn(BC_ILOAD1);
			bc->addInsn(BC_ISUB);
			break;
		case tSUB:
			switch (TOSType) {
			case VT_INT:
				bc->addInsn(BC_INEG);
				break;
			case VT_DOUBLE:
				bc->addInsn(BC_DNEG);
				break;
			default:
				cerr << "Incorrect inversion" << endl;
				break;
			}
			break;
		default:
			cerr << "Incorrect unary op" << endl;
			break;
		}
	}

	void visitWhileNode(WhileNode * node) {
		Label begin(bc);
		Label end(bc);
		bc->bind(begin);
		node->whileExpr()->visit(this);
		bc->addInsn(BC_ILOAD0);
		bc->addBranch(BC_IFICMPE, end);
		node->loopBlock()->visit(this);
		bc->addBranch(BC_JA, begin);
		bc->bind(end);
	}

	map<const AstVar*, pair<int16_t, int16_t> > varIds;
	int funcToMaxIndices[65536];

	void declareVariable(const AstVar *var) {
		int index = funcToMaxIndices[funIndex]++;
		varIds[var] = make_pair(funIndex, index);
	}

	void declareVariables(Scope * scope) {
		Scope::VarIterator iter(scope);
		while (iter.hasNext()) {
			declareVariable(iter.next());
		}
	}

	void processFunctions(Scope * scope, bool generateCode) {
		Scope::FunctionIterator iter(scope);
		while (iter.hasNext()) {
			AstFunction *func = iter.next();
			if (generateCode) {
				BytecodeFunction *bcf =
						(BytecodeFunction*) code->functionByName(func->name());
				bc = bcf->bytecode();
				funIndex = bcf->id();
				returnType = func->returnType();
				func->node()->body()->visit(this);
				bcf->setLocalsNumber(funcToMaxIndices[funIndex]);
			} else { //first function visit
				BytecodeFunction *bcf = new BytecodeFunction(func);
				funIndex = code->addFunction(bcf);
				funcToMaxIndices[funIndex] = 1;
				bc = bcf->bytecode();
				for (unsigned i = 0; i < func->parametersNumber(); ++i) {
					AstVar * var = func->scope()->lookupVariable(
							func->parameterName(i), false);
					declareVariable(var);
					generateStoring(var);
				}
			}
		}

		for (unsigned i = 0; i < scope->childScopeNumber(); ++i) {
			processFunctions(scope->childScopeAt(i), generateCode);
		}
	}

	Parser parser;
public:

	void generateCode(Code *code, const char * source) {
		Status * status = parser.parseProgram(source);
		if (status != 0 && status->isError()) {
			cerr << "Error parsing program";
			return;
		}

		this->code = code;

		processFunctions(parser.top()->scope(), false);
		processFunctions(parser.top()->scope(), true);
	}
};

#endif /* BYTECODE_GEN_H_ */
