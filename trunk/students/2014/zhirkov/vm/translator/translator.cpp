#include <iostream>
#include <sstream>
#include <set>
#include <queue>
#include <map>
#include "../../../../../include/ast.h"
#include "../ir/ir.h"
#include "translator.h"
#include "../typechecker.h"
#include "ssa_utils.h"

namespace mathvm {


    void debug(char const *tr) {
//        std::cerr << "Visiting " << tr << " node" << std::endl;
    }

    void IrBuilder::embraceArgs(AstFunction * f) {
        Scope::VarIterator iter(f->scope(), false);
        while (iter.hasNext()) {
            uint64_t id = makeAstVar(iter.next());
            funMeta(f)->args.push_back(&varMetaById(id));
        }
    }

    void IrBuilder::visitAstFunction(AstFunction *fun) {
        debug("ast_function");
        uint64_t id = _ir.functions.size();
        IR::VarType type = vtToIrType(fun->node()->returnType());
        IR::FunctionRecord* functionRecord = new IR::FunctionRecord(id, type);
        _funMeta.insert(make_pair(fun, new AstFunctionMetadata(fun, id)));
        _ir.functions.push_back(functionRecord);
        embraceArgs(fun);
        IR::Block * savedBlock =_currentBlock;
        _currentBlock = & (functionRecord->entry);
        AstFunction* savedFunction = _currentFunction;
        _currentFunction = fun;
        fun->node()->visit(this);
        _currentBlock = savedBlock;
        _currentFunction = savedFunction;
    }


    static IR::BinOp *selectBinOp(TokenKind kind, IR::Expression const * const left, IR::Expression const *const right) {
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

    static IR::UnOp const* selectUnOp(TokenKind kind, IR::Expression const* operand) {
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

    void IrBuilder::visitBinaryOpNode(BinaryOpNode *node) {
        debug("binop");
        node->left()->visit(this);
        const IR::Atom * const left = _popAtom();
        node->right()->visit(this);
        const IR::Atom * const right = _popAtom();

        IR::Assignment* a = new IR::Assignment(makeTempVar(), selectBinOp(node->kind(), left, right));
        _pushAtom(& (a->var) );

        emit(a);

    }

    void IrBuilder::visitUnaryOpNode(UnaryOpNode *node) {
        debug("unop");
        node->operand()->visit(this);
        IR::Expression const* operand = _popAtom();
        IR::Assignment const* a = new IR::Assignment(makeTempVar(), selectUnOp(node->kind(), operand));
        emit(a);
        _pushAtom(&(a->var));
    }

    void IrBuilder::visitStringLiteralNode(StringLiteralNode *node) {
        debug("string");
        IR::FunctionRecord::StringPool& pool = _ir.functions[funMeta(_currentFunction)->id]->pool;
        uint16_t  id = uint16_t(pool.size()) ;
        pool.push_back(node->literal());
        _pushAtom(new IR::Ptr(id, true));
    }

    void IrBuilder::visitDoubleLiteralNode(DoubleLiteralNode *node) {
        debug("double");
        _pushAtom(new IR::Double(node->literal()));
    }

    void IrBuilder::visitIntLiteralNode(IntLiteralNode *node) {
        debug("int");
        _pushAtom(new IR::Int(node->literal()));
    }

    void IrBuilder::visitLoadNode(LoadNode *node) {
        debug("load");
        _pushAtom(new IR::Variable(varMeta((AstVar *) (node->var())).id));
//        uint64_t id = _sourceVariables.at(node->var()).back();
//        IR::Variable *v = new IR::Variable(vtToIrType(node->var()->type()), id);
//        _lastAtom.push(v);
    }

    void IrBuilder::visitStoreNode(StoreNode *node) {
        debug("store");
        node->value()->visit(this);
        const IR::Expression *rhs = _popAtom();
        const IR::Variable varToStore(varMeta(node->var()).id);
        switch(node->op()) {
            case tASSIGN:
                break;
            case tINCRSET:
                rhs = new IR::BinOp(new IR::Variable(varToStore), rhs, IR::BinOp::BO_ADD);
                break;
            case tDECRSET:
                rhs = new IR::BinOp(new IR::Variable(varToStore), rhs, IR::BinOp::BO_SUB);
                break;
            default:
            std::cerr<< "Store node contains bad token " << tokenStr(node->op()) << std::endl;
                break;
        }
        emit(new IR::Assignment(varToStore, rhs));
    }

    void IrBuilder::visitForNode(ForNode *node) {
        IR::Block* init, *checker, *bodyFirst, *bodyLast, *beforeFor = _currentBlock, *afterFor;
        auto astFrom = node->inExpr()->asBinaryOpNode()->left(),
                astTo = node->inExpr()->asBinaryOpNode()->right();

        _currentBlock  = init = newBlock();

//        const IR::Variable var(makeAstVar(node->var()));
        const IR::Variable var(varMeta(node->var()).id);
        astFrom->visit(this);
        emit(new IR::Assignment(var, _popAtom()));

        _currentBlock = checker = newBlock();
        IR::Variable toValue(makeTempVar()), compResult(makeTempVar());
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

    void IrBuilder::visitWhileNode(WhileNode *node) {
        IR::Block* checker = newBlock(),
                *beforeWhile = _currentBlock,
                *bodyFirstBlock = newBlock(),
                *bodyLastBlock = NULL;

        beforeWhile->link(checker);

        _currentBlock = checker;
        node->whileExpr()->visit(this);
        IR::Assignment const* condAssign =  new IR::Assignment(makeTempVar(), _popAtom());
        emit(condAssign);

        _currentBlock = bodyFirstBlock;
        node->loopBlock()->visit(this);
        bodyLastBlock = _currentBlock;

        IR::Block* afterWhile = newBlock();
        bodyLastBlock->link(checker);
        checker->link(new IR::JumpCond(bodyFirstBlock, afterWhile, &(condAssign->var) ));


    }

    void IrBuilder::visitIfNode(IfNode *node) {
        node->ifExpr()->visit(this);
        IR::Assignment* a =  new IR::Assignment(makeTempVar(), _popAtom());
        emit(a);

        IR::Block *blockBeforeIf = _currentBlock;

        IR::Block *yesblock = newBlock();
        IR::Block *noblock = newBlock();

        blockBeforeIf->link(new IR::JumpCond(yesblock, noblock, &(a->var)));

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

    void IrBuilder::visitBlockNode(BlockNode *node) {
        embraceVars(node->scope());
        _lastScope = node->scope();
        Scope::FunctionIterator fit(node->scope(), false);
        while (fit.hasNext())
            visitAstFunction(_currentFunction = fit.next());
        node->visitChildren(this);

        _lastScope = _lastScope->parent();
    }

    void IrBuilder::visitFunctionNode(FunctionNode *node) {
        debug("function");
        node->body()->visit(this);
    }


    void IrBuilder::visitReturnNode(ReturnNode *node) {
        debug("return");
        if (! node->returnExpr()) return;
        node->returnExpr()->visit(this);
        emit(new IR::Return(_popAtom()));
    }

    void IrBuilder::visitCallNode(CallNode *node) {
        debug("call");
        const uint16_t funId = funMeta(_lastScope->lookupFunction(node->name(), true))->id;
        std::vector<IR::Atom const*> params;
        for(uint32_t i = 0; i < node->parametersNumber(); ++i) {
            node->parameterAt(i)->visit(this);
            params.push_back(_popAtom());
        }
        IR::Variable temp(makeTempVar());
        emit(new IR::Assignment(temp, new IR::Call(funId, params)));
    }

    void IrBuilder::visitNativeCallNode(NativeCallNode *node) {
        debug("native call");
    }

    void IrBuilder::visitPrintNode(PrintNode *node) {
        for( uint32_t i = 0; i < node->operands(); ++i)
        {
            node->operandAt(i)->visit(this);
            emit(new IR::Print(_popAtom()));
        }
        ;
        debug("print");
    }


    void IrBuilder::start() {

        visitAstFunction(_parser.top());


        IR::IrPrinter printer(_out);
        
        for (auto it = _ir.functions.begin(); it != _ir.functions.end(); ++it)
        {
            _out<<"Function " << (*it)->id << " vars: " << std::endl;
            for (auto kvp :_allvarMeta)
                _out << kvp.first << " -> " << ((kvp.second->isTemp)?"temp" : kvp.second->var->name().c_str())
                        << std::endl;
            _out << std::endl;
            (**it).visit(&printer);
        }


        _out << "Dominance frontiers" << std::endl;

        for (auto f : _ir.functions) {
            auto front = dominanceFrontier(&(f->entry));
            for (auto kvp : front) {
                _out << kvp.first->name << " -> ";
                for (auto elem : kvp.second)
                    _out << " " << elem->name;
                _out << std::endl;
            }
        }

//
//        }
//        _out<< "blocks in reverse order " << std::endl;
//        for (auto it = _ir.functions.begin(); it != _ir.functions.end(); ++it)
//            for(auto b : blocksPostOrder((**it).entry))
//                _out<< "-- " << b->name << std::endl;


//        _out<< "Dominators" << std::endl;
//        for (auto it = _ir.functions.begin(); it != _ir.functions.end(); ++it){
//            auto doms = dominators((**it).entry);
//            for (auto kvp : doms) {
//                _out << kvp.first->name << "->" ;
//                for (auto e : kvp.second)
//                    _out << e->name  << " ";
//                _out << std::endl;
//            }
//        }
//
//        _out<< "Immediate dominators!" << std::endl;
//        for (auto it = _ir.functions.begin(); it != _ir.functions.end(); ++it){
//            auto doms = immediateDominators((**it).entry);
//            for (auto kvp : doms)
//                _out << kvp.first->name << "->" << kvp.second->name  << " " << std::endl;
//
//        }

    }

//    IR::Block *IrBuilder::newBlock() {
//        std::string name = nextBlockName();
//        return new IR::Block(name);
//    }
//
//    IR::Block *IrBuilder::addBlock() {
//
//        IR::FunctionRecord *f = _lastFunction.top();
//        IR::Block *next = newBlock();
//
//
//        if (f->entry == NULL) {
//            f->entry = next;
//        }
//        else {
//            _lastBlock->link(next);
//            _lastBlock = next;
//        }
//
//        _lastBlock = next;
//        return next;
//    }


    void IrBuilder::embraceVars(Scope *scope) {
        Scope::VarIterator iter(scope, false);
        while (iter.hasNext()) makeAstVar(iter.next());

    }
}




