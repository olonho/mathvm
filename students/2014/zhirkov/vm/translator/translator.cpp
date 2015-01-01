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
#include <unordered_set>

namespace mathvm {

    static IR::BinOp *selectBinOp(TokenKind kind, IR::Atom const *const left, IR::Atom const *const right) {
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

    static IR::UnOp const *selectUnOp(TokenKind kind, IR::Atom const *operand) {
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


    void SimpleIrBuilder::argsForCaptured(AstFunction const * const f ) {
        if (!_closureInfo->captures(f)) return;
        for (auto cptVar : closureFunMeta(f).capturedOuterVars)
            if (!closureVarMeta(cptVar).isWritten) {
                funMeta(f).capturedArgs.push_back(&varMeta(cptVar));
                _result->functions[funMeta(f).id]->parametersIds.push_back(varMeta(cptVar).id);
                _debug << "added captured variable " << cptVar->name()
                << " to function " << f->name() << " as an additional argument\n";
            } else {
                funMeta(f).capturedRefArgs.push_back(&varMeta(cptVar));
                _result->functions[funMeta(f).id]->refParameterIds.push_back(varMeta(cptVar).id);
                _debug << "added captured variable " << cptVar->name()
                        << " to function " << f->name() << " as an additional reference argument! \n";
            }
    }

    void SimpleIrBuilder::embraceArgs(AstFunction const * const f) {
        Scope::VarIterator iter(f->scope(), false);
        while (iter.hasNext()) {
            auto v = iter.next();
            uint64_t id = makeAstVar(v);
            funMeta(f).args.push_back(&varMetaById(id));
            _result->functions[funMeta(f).id]->parametersIds.push_back(id);

            _debug << "added argument " << varMetaById(id).var->name() << " (with arg id " << id << ") to function "
                    << f->name() << std::endl;
        }
    }

    void SimpleIrBuilder::declareFunction(AstFunction const *fun) {
        uint64_t id = _result->functions.size();
        IR::VarType type = vtToIrType(fun->node()->returnType());
        IR::FunctionRecord *functionRecord = new IR::FunctionRecord(id, type);
        ctx.funMeta.insert(make_pair(fun, new AstFunctionMetadata(fun, id)));
        _result->addFunction(functionRecord);
        _debug << "declared function " << fun->name() << " id " << functionRecord->id << std::endl;
        embraceArgs(fun);
        argsForCaptured(fun);
    }



    void SimpleIrBuilder::visitAstFunction(AstFunction const *fun) {

        auto savedBlock = ctx.block;
        auto savedFunction = ctx.function;

        ctx.block = &(*(_result->functions[funMeta(fun).id]->entry));
        ctx.function = fun;

        prologue(fun);
        fun->node()->visit(this);
        createMemoryCells(fun);
        epilogue(fun);
        ctx.function = savedFunction;
        ctx.block = savedBlock;
    }


    void SimpleIrBuilder::visitBinaryOpNode(BinaryOpNode *node) {
        node->left()->visit(this);  const IR::Atom *const left = _popAtom();
        node->right()->visit(this); const IR::Atom *const right = _popAtom();

        auto tmp = makeTempVar();
        emit(new IR::Assignment(tmp, selectBinOp(node->kind(), left, right)));
        _pushVar(tmp);
    }

    void SimpleIrBuilder::visitUnaryOpNode(UnaryOpNode *node) {
        node->operand()->visit(this);
        auto tmp = makeTempVar();
        emit(new IR::Assignment(tmp, selectUnOp(node->kind(), _popAtom())));
        _pushVar(tmp);
    }

    void SimpleIrBuilder::visitStringLiteralNode(StringLiteralNode *node) {
        IR::SimpleIr::StringPool &pool = _result->pool;
        uint16_t id = uint16_t(pool.size());
        pool.push_back(node->literal());
        _pushAtom(new IR::Ptr(id, true));
    }

    void SimpleIrBuilder::visitDoubleLiteralNode(DoubleLiteralNode *node) { _pushAtom(new IR::Double(node->literal())); }

    void SimpleIrBuilder::visitIntLiteralNode(IntLiteralNode *node) { _pushAtom(new IR::Int(node->literal())); }

    void SimpleIrBuilder::visitLoadNode(LoadNode *node) { _pushAtom(readVar(varMeta(node->var()).id)); }

    void SimpleIrBuilder::visitStoreNode(StoreNode *node) {

        node->value()->visit(this);
        const IR::Atom *rhs = _popAtom();

        auto varToStore = varMeta(node->var()).id;
        switch (node->op()) {
            case tINCRSET:
            {
                auto e  = new IR::BinOp(readVar(varToStore), rhs, IR::BinOp::BO_ADD);
                emit(new IR::Assignment(varToStore, e));
                break;
            }
            case tDECRSET: {
                auto e = new IR::BinOp(readVar(varToStore), rhs, IR::BinOp::BO_SUB);
                emit(new IR::Assignment(varToStore, e));
                break;
            }
            case tASSIGN:
                emit(new IR::Assignment(varToStore, rhs));
                break;
            default:
                std::cerr << "Store node contains bad token " << tokenStr(node->op()) << std::endl;
                break;
        };
    }

    void SimpleIrBuilder::visitForNode(ForNode *node) {
        IR::Block *init, *checker, *bodyFirst, *bodyLast, *beforeFor = ctx.block, *afterFor;
        auto astFrom = node->inExpr()->asBinaryOpNode()->left(),
                astTo = node->inExpr()->asBinaryOpNode()->right();

        ctx.block = init = newBlock();

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

    void SimpleIrBuilder::visitWhileNode(WhileNode *node) {
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

    void SimpleIrBuilder::visitIfNode(IfNode *node) {
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

    void SimpleIrBuilder::visitBlockNode(BlockNode *node) {
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

    void SimpleIrBuilder::visitFunctionNode(FunctionNode *node) {
        //debug("function");
        node->body()->visit(this);
    }


    void SimpleIrBuilder::visitReturnNode(ReturnNode *node) {
        if (!node->returnExpr()) return;
        epilogue(ctx.function);
        node->returnExpr()->visit(this);
        emit(new IR::Return(_popAtom()));
    }

    void SimpleIrBuilder::visitCallNode(CallNode *node) {
        AstFunction *f = ctx.scope->lookupFunction(node->name(), true);
        const uint16_t funId = funMeta(f).id;
        std::vector<IR::Atom const *> params;
        std::vector<uint64_t> refParams;
        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            node->parameterAt(i)->visit(this);
            params.push_back(_popAtom());
        }
        if (_closureInfo->captures(f)) {
            for (auto cptVar : closureFunMeta(f).capturedOuterVars)
                if (!closureVarMeta(cptVar).isWritten)
                    params.push_back(new IR::Variable(varMeta(cptVar).id));
                else
                    refParams.push_back(varMeta(cptVar).id);
        }

        for (auto cptWriteVar: closureFunMeta(f).capturedOuterVars)
            if (_closureInfo->capturedWritten(cptWriteVar)) {
                auto id = varMeta(cptWriteVar).id;
                emit(new IR::WriteRef(new IR::Variable(id), id));
            }
        auto tempVarId = makeTempVar();
        emit(new IR::Assignment(tempVarId, new IR::Call(funId, params, refParams)));

        for (auto cptVar : closureFunMeta(f).capturedOuterVars)
            if (closureVarMeta(cptVar).isWritten) {
                auto id = varMeta(cptVar).id;
                emit(new IR::Assignment(id, new IR::ReadRef(id)));
            }

        _pushAtom(new IR::Variable(tempVarId));
    }

    void SimpleIrBuilder::visitNativeCallNode(NativeCallNode *node) {
        //debug("native call");
    }

    void SimpleIrBuilder::visitPrintNode(PrintNode *node) {
        for (uint32_t i = 0; i < node->operands(); ++i) {
            node->operandAt(i)->visit(this);
            emit(new IR::Print(_popAtom()));
        };
    }


    void SimpleIrBuilder::start() {
        assert(this != NULL);
        AstAnalyzer::start();

        insertPhi();
    }

    void SimpleIrBuilder::embraceVars(Scope* const scope) {
        Scope::VarIterator iter(scope, false);
        while (iter.hasNext()) {
            auto v = iter.next();
            makeAstVar(v);
        }
    }

//fixme: too many phis! test: fib_closure.mvm
    void SimpleIrBuilder::insertPhi() {
        _debug << "inserting phi functions... \n";

        std::map<IR::Block*, std::set<uint64_t>> result;

        for (auto f : _result->functions)
        {
            for (auto elemWithFrontier : dominanceFrontier(&(*(f->entry))))
            {
                _debug << "   for block " << elemWithFrontier.first->name << " : \n";
                for (auto assignedVar : IR::modifiedVars(elemWithFrontier.first))
                    if (!varMetaById(assignedVar).isTemp)
                    {
                        _debug << "   var " << assignedVar << " : ";
                        for (auto blockWithPhi: elemWithFrontier.second) {
                            _debug << " " << blockWithPhi->name;
//                            IR::Phi const *a = new IR::Phi(assignedVar);
//                            (const_cast<IR::Block *> (blockWithPhi))->contents.push_front(a);
                            auto b = (const_cast<IR::Block *> (blockWithPhi));
                            if (result.find(b) == result.end())
                                result.insert(std::make_pair(b, std::set<uint64_t>()));
                            result.at(b).insert(assignedVar);
                        }
                        _debug << std::endl;
                    }
            }
        }

        for (auto kvp : result) {
            for (auto phiVar : kvp.second)
                kvp.first->contents.push_front(new IR::Phi(phiVar));
        }
    }

    uint64_t SimpleIrBuilder::makeAstVar(AstVar const *var) {
        uint64_t id = ctx.nVars++;
        _debug << "created variable " << id  << " for ast var " << var->name() << std::endl;
        AstVarMetadata *md = new AstVarMetadata(var, id, 0);
        ctx.astVarMeta[var] = md;
        ctx.allVarMeta[id] = md;
        IR::SimpleIr::VarMeta add(md->id, md->id, vtToIrType(var->type()));
        _result->varMeta.push_back(add);
        return id;
    }

    uint64_t SimpleIrBuilder::makeTempVar() {
        uint64_t id = ctx.nVars++;
        AstVarMetadata *md = new AstVarMetadata(id);
        ctx.allVarMeta[id] = md;
        IR::SimpleIr::VarMeta add(md->id);
        _result->varMeta.push_back(add);
        return id;
    }


    IR::Atom *SimpleIrBuilder::readVar(uint64_t varId) {
        return new IR::Variable(varId);
    }

    void SimpleIrBuilder::createMemoryCells(AstFunction const *const fun) {
        for (AstVar const* cptWriteVar : closureFunMeta(fun).capturedLocals)
            if (_closureInfo->capturedWritten(cptWriteVar))
            {
                _result->functions[funMeta(fun).id]->memoryCells.push_back(varMeta(cptWriteVar).id);
                _debug << "Created memory cell for variable " << cptWriteVar->name()
                        << " of function " << ctx.function->name() <<std::endl;

            }
    }

    void SimpleIrBuilder::prologue(AstFunction const *const fun) {
        for (auto cptVar : closureFunMeta(fun).capturedOuterVars)
            if (closureVarMeta(cptVar).isWritten) {
                auto id = varMeta(cptVar).id;
                emit(new IR::Assignment(id, new IR::ReadRef(id)));
            }
    }

    void SimpleIrBuilder::epilogue(AstFunction const *const fun) {
        for (auto cptVar : closureFunMeta(fun).capturedOuterVars)
            if (closureVarMeta(cptVar).isWritten) {
                auto id = varMeta(cptVar).id;
                emit(new IR::WriteRef(new IR::Variable(id), id));
            }
    }
}