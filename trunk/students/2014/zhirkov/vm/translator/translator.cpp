#include <iostream>
#include <sstream>
#include <set>
#include <queue>
#include <map>
#include "../../../../../include/ast.h"
#include "../ir/ir.h"
#include "translator.h"
#include "../ir/util.h"
#include "../ir/transformations/identity.h"
#include "../ir/transformations/ssa.h"
#include "../ast_printer.h"

namespace mathvm {


    void debug(char const *tr) {
//        std::cerr << "Visiting " << tr << " node" << std::endl;
    }

    void SimpleIrBuilder::embraceArgs(AstFunction *f) {
        Scope::VarIterator iter(f->scope(), false);
        while (iter.hasNext()) {
            uint64_t id = makeAstVar(iter.next());
            funMeta(f).args.push_back(&varMetaById(id));
        }
    }

    void SimpleIrBuilder::declareFunction(AstFunction *fun) {
        uint64_t id = _ir->functions.size();
        IR::VarType type = vtToIrType(fun->node()->returnType());
        IR::FunctionRecord* functionRecord = new IR::FunctionRecord(id, type);
        _funMeta.insert(make_pair(fun, new AstFunctionMetadata(fun, id)));
        _ir->addFunction(functionRecord);

        embraceArgs(fun);
    }

    void SimpleIrBuilder::visitAstFunction(AstFunction *fun) {
        debug("ast_function");

        auto savedBlock = _currentBlock;
        auto savedFunction = _currentFunction;

        _currentBlock = &(*(_ir->functions[funMeta(fun).id]->entry));
        _currentFunction = fun;

        fun->node()->visit(this);

        _currentFunction = savedFunction;
        _currentBlock = savedBlock;
    }


    static IR::BinOp *selectBinOp(TokenKind kind, IR::Expression const *const left, IR::Expression const *const right) {
        switch (kind) {
            case tOR:
                return new IR::BinOp(left, right, IR::BinOp::Type::BO_LOR);
            case tAND:
                return new IR::BinOp(left, right, IR::BinOp::Type::BO_LAND);
            case tAOR:
                return new IR::BinOp(left, right, IR::BinOp::Type::BO_OR);
            case tAAND:
                return new IR::BinOp(left, right, IR::BinOp::Type::BO_AND);;
            case tAXOR:
                return new IR::BinOp(left, right, IR::BinOp::Type::BO_XOR);
            case tEQ:
                return new IR::BinOp(left, right, IR::BinOp::Type::BO_EQ);
            case tNEQ:
                return new IR::BinOp(left, right, IR::BinOp::Type::BO_NEQ);
            case tGT:
                return new IR::BinOp(right, left, IR::BinOp::Type::BO_LT);
            case tGE:
                return new IR::BinOp(right, left, IR::BinOp::Type::BO_LE);
            case tLT:
                return new IR::BinOp(left, right, IR::BinOp::Type::BO_LT);
            case tLE:
                return new IR::BinOp(left, right, IR::BinOp::Type::BO_LE);
            case tADD:
                return new IR::BinOp(left, right, IR::BinOp::Type::BO_ADD);
            case tSUB:
                return new IR::BinOp(left, right, IR::BinOp::Type::BO_SUB);
            case tMUL:
                return new IR::BinOp(left, right, IR::BinOp::Type::BO_MUL);
            case tDIV:
                return new IR::BinOp(left, right, IR::BinOp::Type::BO_DIV);
            case tMOD:
                return new IR::BinOp(left, right, IR::BinOp::Type::BO_MOD);
            default:
                std::cerr << "Invalid translator state: unsupported binary operation token" << std::endl;
                return NULL;
        }
    }

    static IR::UnOp const *selectUnOp(TokenKind kind, IR::Expression const *operand) {
        switch (kind) {
            case tNOT:
                return new IR::UnOp(operand, IR::UnOp::Type::UO_NOT);
            case tSUB:
                return new IR::UnOp(operand, IR::UnOp::Type::UO_NEG);
            default:
                std::cerr << "Invalid translator state: unsupported binary operation token" << std::endl;
                return NULL;
        }
    }

    void SimpleIrBuilder::visitBinaryOpNode(BinaryOpNode *node) {
        debug("binop");
        node->left()->visit(this);
        const IR::Atom *const left = _popAtom();
        node->right()->visit(this);
        const IR::Atom *const right = _popAtom();

        IR::Assignment *a = new IR::Assignment(makeTempVar(), selectBinOp(node->kind(), left, right));
        _pushAtom(new IR::Variable(a->var->id));

        emit(a);

    }

    void SimpleIrBuilder::visitUnaryOpNode(UnaryOpNode *node) {
        debug("unop");
        node->operand()->visit(this);
        IR::Expression const *operand = _popAtom();
        IR::Assignment const *a = new IR::Assignment(makeTempVar(), selectUnOp(node->kind(), operand));
        emit(a);
        _pushAtom(new IR::Variable(a->var->id));
    }

    void SimpleIrBuilder::visitStringLiteralNode(StringLiteralNode *node) {
        debug("string");
        IR::SimpleIr::StringPool &pool = _ir->pool;
        uint16_t id = uint16_t(pool.size());
        pool.push_back(node->literal());
        _pushAtom(new IR::Ptr(id, true));
    }

    void SimpleIrBuilder::visitDoubleLiteralNode(DoubleLiteralNode *node) {
        debug("double");
        _pushAtom(new IR::Double(node->literal()));
    }

    void SimpleIrBuilder::visitIntLiteralNode(IntLiteralNode *node) {
        debug("int");
        _pushAtom(new IR::Int(node->literal()));
    }

    void SimpleIrBuilder::visitLoadNode(LoadNode *node) {
        debug("load");
        _pushAtom(new IR::Variable(varMeta((AstVar *) (node->var())).id));
    }

    void SimpleIrBuilder::visitStoreNode(StoreNode *node) {
        debug("store");

        node->value()->visit(this);
        const IR::Expression *rhs = _popAtom();
        auto varToStore = varMeta(node->var()).id;
        switch (node->op()) {
            case tASSIGN:
                break;
            case tINCRSET:
                rhs = new IR::BinOp(new IR::Variable(varToStore), rhs, IR::BinOp::BO_ADD);
                break;
            case tDECRSET:
                rhs = new IR::BinOp(new IR::Variable(varToStore), rhs, IR::BinOp::BO_SUB);
                break;
            default:
                std::cerr << "Store node contains bad token " << tokenStr(node->op()) << std::endl;
                break;
        }
        emit(new IR::Assignment(varToStore, rhs));
    }

    void SimpleIrBuilder::visitForNode(ForNode *node) {
        IR::Block *init, *checker, *bodyFirst, *bodyLast, *beforeFor = _currentBlock, *afterFor;
        auto astFrom = node->inExpr()->asBinaryOpNode()->left(),
                astTo = node->inExpr()->asBinaryOpNode()->right();

        _currentBlock = init = newBlock();

//        const IR::Variable var(makeAstVar(node->var()));
        auto var = varMeta(node->var()).id;
        astFrom->visit(this);
        emit(new IR::Assignment(var, _popAtom()));

        _currentBlock = checker = newBlock();
        auto toValue = makeTempVar(), compResult = makeTempVar();
        astTo->visit(this);
        emit(new IR::Assignment(toValue, _popAtom()));
        emit(new IR::Assignment(compResult, new IR::BinOp(new IR::Variable(var), new IR::Variable(toValue), IR::BinOp::BO_EQ)));


        _currentBlock = bodyFirst = newBlock();
        node->body()->visit(this);
        emit(new IR::Assignment(var, new IR::BinOp(new IR::Variable(var), new IR::Int(1), IR::BinOp::BO_ADD)));

        bodyLast = _currentBlock;

        _currentBlock = afterFor = newBlock();

        beforeFor->link(init);
        init->link(checker);
        checker->link(new IR::JumpCond(afterFor, bodyFirst, new IR::Variable(compResult)));
        bodyLast->link(checker);


    }

    void SimpleIrBuilder::visitWhileNode(WhileNode *node) {
        IR::Block *checker = newBlock(),
                *beforeWhile = _currentBlock,
                *bodyFirstBlock = newBlock(),
                *bodyLastBlock = NULL;

        beforeWhile->link(checker);

        _currentBlock = checker;
        node->whileExpr()->visit(this);
        IR::Assignment const *condAssign = new IR::Assignment(makeTempVar(), _popAtom());
        emit(condAssign);

        _currentBlock = bodyFirstBlock;
        node->loopBlock()->visit(this);
        bodyLastBlock = _currentBlock;

        IR::Block *afterWhile = newBlock();
        bodyLastBlock->link(checker);
        checker->link(new IR::JumpCond(bodyFirstBlock, afterWhile, new IR::Variable(condAssign->var->id)));


    }

    void SimpleIrBuilder::visitIfNode(IfNode *node) {
        node->ifExpr()->visit(this);
        IR::Assignment *a = new IR::Assignment(makeTempVar(), _popAtom());
        emit(a);

        IR::Block *blockBeforeIf = _currentBlock;

        IR::Block *yesblock = newBlock();
        IR::Block *noblock = newBlock();

        blockBeforeIf->link(new IR::JumpCond(yesblock, noblock, new IR::Variable(a->var->id)));

        _currentBlock = yesblock;
        node->thenBlock()->visit(this);
        IR::Block *lastYesBlock = _currentBlock;
        _currentBlock = noblock;
        if (node->elseBlock()) node->elseBlock()->visit(this);
        IR::Block *lastNoBlock = _currentBlock;

        IR::Block *afterIf = newBlock();
        lastYesBlock->link(afterIf);
        lastNoBlock->link(afterIf);

        _currentBlock = afterIf;
    }

    void SimpleIrBuilder::visitBlockNode(BlockNode *node) {
        embraceVars(node->scope());
        _lastScope = node->scope();

        Scope::FunctionIterator fit(node->scope(), false);
        while (fit.hasNext())
            declareFunction(_currentFunction = fit.next());


        node->visitChildren(this);

        fit = Scope::FunctionIterator(node->scope(), false);
        while (fit.hasNext())
            visitAstFunction(_currentFunction = fit.next());

        _lastScope = _lastScope->parent();
    }

    void SimpleIrBuilder::visitFunctionNode(FunctionNode *node) {
        debug("function");
        node->body()->visit(this);
    }


    void SimpleIrBuilder::visitReturnNode(ReturnNode *node) {
        debug("return");
        if (!node->returnExpr()) return;
        node->returnExpr()->visit(this);
        emit(new IR::Return(_popAtom()));
    }

    void SimpleIrBuilder::visitCallNode(CallNode *node) {
        debug("call");
        AstFunction *f = _lastScope->lookupFunction(node->name(), true);
        const uint16_t funId = funMeta(f).id;
        std::vector<IR::Atom const *> params;
        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            node->parameterAt(i)->visit(this);
            params.push_back(_popAtom());
        }
        auto tempVarId = makeTempVar();
        emit(new IR::Assignment(tempVarId, new IR::Call(funId, params)));
        _pushAtom(new IR::Variable(tempVarId));
    }

    void SimpleIrBuilder::visitNativeCallNode(NativeCallNode *node) {
        debug("native call");
    }

    void SimpleIrBuilder::visitPrintNode(PrintNode *node) {
        for (uint32_t i = 0; i < node->operands(); ++i) {
            node->operandAt(i)->visit(this);
            emit(new IR::Print(_popAtom()));
        };
        debug("print");
    }


    IR::SimpleSsaIr* SimpleIrBuilder::start() {
        declareFunction(_parser.top());
        visitAstFunction(_parser.top());

        insertPhi();


        //IR::IrPrinter printer(_out);
        IR::SsaTransformation ssaTransformation(*_ir);
        ssaTransformation.start();
        IR::SimpleSsaIr* ssa = ssaTransformation.getResult();
        delete _ir;
        _ir = ssa;
        return ssa;
    }

    void SimpleIrBuilder::embraceVars(Scope *scope) {
        Scope::VarIterator iter(scope, false);
        while (iter.hasNext()) makeAstVar(iter.next());

    }

    void SimpleIrBuilder::insertPhi() {
        for (auto f : _ir->functions)
            for (auto elemWithFrontier : dominanceFrontier(&(*(f->entry))))
                for (auto assignedVar : IR::modifiedVars(elemWithFrontier.first))
                    if (!varMetaById(assignedVar).isTemp)
                        for (auto blockWithPhi: elemWithFrontier.second) {
                            IR::Phi const *a = new IR::Phi(assignedVar);
                            (const_cast<IR::Block *> (blockWithPhi))->contents.push_front(a);
                        }
    }

    uint64_t SimpleIrBuilder::makeAstVar(AstVar const* var) {
        uint64_t id = _varCounter++;
        AstVarMetadata* md = new AstVarMetadata(var, id);
        _astvarMeta[var] = md;
        _allvarMeta[id] = md;
        IR::SimpleIr::VarMeta add(md->id, md->id, vtToIrType(var->type()));
        _ir->varMeta.push_back(add);
        return id;
    }

    uint64_t SimpleIrBuilder::makeTempVar() {
        uint64_t id = _varCounter++;
        AstVarMetadata* md = new AstVarMetadata(id);
        _allvarMeta[id] = md;
        IR::SimpleIr::VarMeta add(md->id, 0, IR::VT_Undefined);
        _ir->varMeta.push_back(add);
        return id;
    }


}