#include <iostream>
#include <sstream>
#include <set>
#include <queue>
#include <map>
#include "../../../../../include/ast.h"
#include "../ir/ir.h"
#include "../ir/util.h"
#include "../ir/transformations/identity.h"
#include "../ir/transformations/ssa.h"
#include "translator.h"

namespace mathvm {


    void debug(char const *tr) {
//        std::cerr << "Visiting " << tr << " node" << std::endl;
    }

    void SsaIrBuilder::embraceArgs(AstFunction const*f) {
        Scope::VarIterator iter(f->scope(), false);
        while (iter.hasNext()) {
            uint64_t id = makeAstVar(iter.next());
            funMeta(f).args.push_back(&varMetaById(id));
        }
    }

    void SsaIrBuilder::declareFunction(AstFunction const*fun) {
        uint64_t id = _result->functions.size();
        IR::VarType type = vtToIrType(fun->node()->returnType());
        IR::FunctionRecord* functionRecord = new IR::FunctionRecord(id, type);
        _funMeta.insert(make_pair(fun, new AstFunctionMetadata(fun, id)));
        _result->addFunction(functionRecord);

        embraceArgs(fun);
    }

    void SsaIrBuilder::visitAstFunction(AstFunction const *fun) {
        debug("ast_function");

        auto savedBlock = ctx.block;
        auto savedFunction = ctx.function;

        ctx.block = &(*(_result->functions[funMeta(fun).id]->entry));
        ctx.function= fun;

        fun->node()->visit(this);

        ctx.function = savedFunction;
        ctx.block = savedBlock;
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

    void SsaIrBuilder::visitBinaryOpNode(BinaryOpNode *node) {
        debug("binop");
        node->left()->visit(this);
        const IR::Atom *const left = _popAtom();
        node->right()->visit(this);
        const IR::Atom *const right = _popAtom();

        IR::Assignment *a = new IR::Assignment(makeTempVar(), selectBinOp(node->kind(), left, right));
        _pushAtom(new IR::Variable(a->var->id));

        emit(a);

    }

    void SsaIrBuilder::visitUnaryOpNode(UnaryOpNode *node) {
        debug("unop");
        node->operand()->visit(this);
        IR::Expression const *operand = _popAtom();
        IR::Assignment const *a = new IR::Assignment(makeTempVar(), selectUnOp(node->kind(), operand));
        emit(a);
        _pushAtom(new IR::Variable(a->var->id));
    }

    void SsaIrBuilder::visitStringLiteralNode(StringLiteralNode *node) {
        debug("string");
        IR::SimpleIr::StringPool &pool = _result->pool;
        uint16_t id = uint16_t(pool.size());
        pool.push_back(node->literal());
        _pushAtom(new IR::Ptr(id, true));
    }

    void SsaIrBuilder::visitDoubleLiteralNode(DoubleLiteralNode *node) {
        debug("double");
        _pushAtom(new IR::Double(node->literal()));
    }

    void SsaIrBuilder::visitIntLiteralNode(IntLiteralNode *node) {
        debug("int");
        _pushAtom(new IR::Int(node->literal()));
    }

    void SsaIrBuilder::visitLoadNode(LoadNode *node) {
        debug("load");
        _pushAtom(new IR::Variable(varMeta((AstVar *) (node->var())).id));
    }

    void SsaIrBuilder::visitStoreNode(StoreNode *node) {
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

    void SsaIrBuilder::visitForNode(ForNode *node) {
        IR::Block *init, *checker, *bodyFirst, *bodyLast, *beforeFor = ctx.block, *afterFor;
        auto astFrom = node->inExpr()->asBinaryOpNode()->left(),
                astTo = node->inExpr()->asBinaryOpNode()->right();

        ctx.block = init = newBlock();

//        const IR::Variable var(makeAstVar(node->var()));
        auto var = varMeta(node->var()).id;
        astFrom->visit(this);
        emit(new IR::Assignment(var, _popAtom()));

        ctx.block = checker = newBlock();
        auto toValue = makeTempVar(), compResult = makeTempVar();
        astTo->visit(this);
        emit(new IR::Assignment(toValue, _popAtom()));
        emit(new IR::Assignment(compResult, new IR::BinOp(new IR::Variable(var), new IR::Variable(toValue), IR::BinOp::BO_EQ)));


        ctx.block = bodyFirst = newBlock();
        node->body()->visit(this);
        emit(new IR::Assignment(var, new IR::BinOp(new IR::Variable(var), new IR::Int(1), IR::BinOp::BO_ADD)));

        bodyLast = ctx.block;

        ctx.block = afterFor = newBlock();

        beforeFor->link(init);
        init->link(checker);
        checker->link(new IR::JumpCond(afterFor, bodyFirst, new IR::Variable(compResult)));
        bodyLast->link(checker);


    }

    void SsaIrBuilder::visitWhileNode(WhileNode *node) {
        IR::Block *checker = newBlock(),
                *beforeWhile = ctx.block,
                *bodyFirstBlock = newBlock(),
                *bodyLastBlock = NULL;

        beforeWhile->link(checker);

        ctx.block = checker;
        node->whileExpr()->visit(this);
        IR::Assignment const *condAssign = new IR::Assignment(makeTempVar(), _popAtom());
        emit(condAssign);

        ctx.block = bodyFirstBlock;
        node->loopBlock()->visit(this);
        bodyLastBlock = ctx.block;

        IR::Block *afterWhile = newBlock();
        bodyLastBlock->link(checker);
        checker->link(new IR::JumpCond(bodyFirstBlock, afterWhile, new IR::Variable(condAssign->var->id)));

    }

    void SsaIrBuilder::visitIfNode(IfNode *node) {
        node->ifExpr()->visit(this);
        IR::Assignment *a = new IR::Assignment(makeTempVar(), _popAtom());
        emit(a);

        IR::Block *blockBeforeIf = ctx.block;

        IR::Block *yesblock = newBlock();
        IR::Block *noblock = newBlock();

        blockBeforeIf->link(new IR::JumpCond(yesblock, noblock, new IR::Variable(a->var->id)));

        ctx.block = yesblock;
        node->thenBlock()->visit(this);
        IR::Block *lastYesBlock = ctx.block;
        ctx.block = noblock;
        if (node->elseBlock()) node->elseBlock()->visit(this);
        IR::Block *lastNoBlock = ctx.block;

        IR::Block *afterIf = newBlock();
        lastYesBlock->link(afterIf);
        lastNoBlock->link(afterIf);

        ctx.block = afterIf;
    }

    void SsaIrBuilder::visitBlockNode(BlockNode *node) {
        embraceVars(node->scope());
        ctx.scope = node->scope();

        Scope::FunctionIterator fit(node->scope(), false);
        while (fit.hasNext())
            declareFunction(ctx.function = fit.next());


        node->visitChildren(this);

        fit = Scope::FunctionIterator(node->scope(), false);
        while (fit.hasNext())
            visitAstFunction(ctx.function = fit.next());

        ctx.scope = ctx.scope->parent();
    }

    void SsaIrBuilder::visitFunctionNode(FunctionNode *node) {
        debug("function");
        node->body()->visit(this);
    }


    void SsaIrBuilder::visitReturnNode(ReturnNode *node) {
        debug("return");
        if (!node->returnExpr()) return;
        node->returnExpr()->visit(this);
        emit(new IR::Return(_popAtom()));
    }

    void SsaIrBuilder::visitCallNode(CallNode *node) {
        debug("call");
        AstFunction *f = ctx.scope->lookupFunction(node->name(), true);
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

    void SsaIrBuilder::visitNativeCallNode(NativeCallNode *node) {
        debug("native call");
    }

    void SsaIrBuilder::visitPrintNode(PrintNode *node) {
        for (uint32_t i = 0; i < node->operands(); ++i) {
            node->operandAt(i)->visit(this);
            emit(new IR::Print(_popAtom()));
        };
        debug("print");
    }


    void SsaIrBuilder::start() {
        declareFunction(top);
        visitAstFunction(top);

        insertPhi();


        //IR::IrPrinter printer(_out);
        IR::SsaTransformation ssaTransformation(*_result);
        ssaTransformation.start();
        IR::SimpleSsaIr* ssa = ssaTransformation.getResult();
        delete _result;
        _result = ssa;
    }

    void SsaIrBuilder::embraceVars(Scope *scope) {
        Scope::VarIterator iter(scope, false);
        while (iter.hasNext()) makeAstVar(iter.next());

    }

    void SsaIrBuilder::insertPhi() {
        for (auto f : _result->functions)
            for (auto elemWithFrontier : dominanceFrontier(&(*(f->entry))))
                for (auto assignedVar : IR::modifiedVars(elemWithFrontier.first))
                    if (!varMetaById(assignedVar).isTemp)
                        for (auto blockWithPhi: elemWithFrontier.second) {
                            IR::Phi const *a = new IR::Phi(assignedVar);
                            (const_cast<IR::Block *> (blockWithPhi))->contents.push_front(a);
                        }
    }

    uint64_t SsaIrBuilder::makeAstVar(AstVar const* var) {
        uint64_t id = ctx.nVars++;
        AstVarMetadata* md = new AstVarMetadata(var, id);
        _astvarMeta[var] = md;
        _allvarMeta[id] = md;
        IR::SimpleIr::VarMeta add(md->id, md->id, vtToIrType(var->type()));
        _result->varMeta.push_back(add);
        return id;
    }

    uint64_t SsaIrBuilder::makeTempVar() {
        uint64_t id = ctx.nVars++;
        AstVarMetadata* md = new AstVarMetadata(id);
        _allvarMeta[id] = md;
        IR::SimpleIr::VarMeta add(md->id, 0, IR::VT_Undefined);
        _result->varMeta.push_back(add);
        return id;
    }


//    void SsaIrBuilder::visitPrintNode(PrintNode *node) {
//        AstAnalyzer::visitPrintNode(node);
//    }
//
//    void SsaIrBuilder::visitLoadNode(LoadNode *node) {
//        AstAnalyzer::visitLoadNode(node);
//    }
//
//    void SsaIrBuilder::visitDoubleLiteralNode(DoubleLiteralNode *node) {
//        AstAnalyzer::visitDoubleLiteralNode(node);
//    }
//
//    void SsaIrBuilder::visitStoreNode(StoreNode *node) {
//        AstAnalyzer::visitStoreNode(node);
//    }
//
//    void SsaIrBuilder::visitCallNode(CallNode *node) {
//        AstAnalyzer::visitCallNode(node);
//    }
//
//    void SsaIrBuilder::visitStringLiteralNode(StringLiteralNode *node) {
//        AstAnalyzer::visitStringLiteralNode(node);
//    }
//
//    void SsaIrBuilder::visitWhileNode(WhileNode *node) {
//        AstAnalyzer::visitWhileNode(node);
//    }
//
//    void SsaIrBuilder::visitIntLiteralNode(IntLiteralNode *node) {
//        AstAnalyzer::visitIntLiteralNode(node);
//    }
//
//    void SsaIrBuilder::visitBlockNode(BlockNode *node) {
//        AstAnalyzer::visitBlockNode(node);
//    }
//
//    void SsaIrBuilder::visitBinaryOpNode(BinaryOpNode *node) {
//        AstAnalyzer::visitBinaryOpNode(node);
//    }
//
//    void SsaIrBuilder::visitUnaryOpNode(UnaryOpNode *node) {
//        AstAnalyzer::visitUnaryOpNode(node);
//    }
//
//    void SsaIrBuilder::visitNativeCallNode(NativeCallNode *node) {
//        AstAnalyzer::visitNativeCallNode(node);
//    }
//
//    void SsaIrBuilder::visitIfNode(IfNode *node) {
//        AstAnalyzer::visitIfNode(node);
//    }
//
//    void SsaIrBuilder::visitReturnNode(ReturnNode *node) {
//        AstAnalyzer::visitReturnNode(node);
//    }
//
//    void SsaIrBuilder::visitFunctionNode(FunctionNode *node) {
//        AstAnalyzer::visitFunctionNode(node);
//    }
//
//    void SsaIrBuilder::visitForNode(ForNode *node) {
//        AstAnalyzer::visitForNode(node);
//    }
}