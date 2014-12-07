#pragma once

#include <sstream>
#include <stack>
#include <map>
#include "../../../../../vm/parser.h"
#include "../ir/ir.h"
#include "../common.h"
#include "../ir/ir_printer.h"
#include "../ast_metadata.h"

namespace mathvm {


    class IrBuilder : public AstVisitor {
        struct AstVarMetadata {
            AstVarMetadata(AstVar const *const var, uint64_t const id) : isTemp(false), var(var), id(id) {

            }

            AstVarMetadata(uint64_t const id) : isTemp(true), var(NULL), id(id) {
            }

            const bool isTemp;
            const AstVar* const var;
            const uint64_t id;
        };



        struct AstFunctionMetadata {
            AstFunctionMetadata(const AstFunction* f, const uint64_t id) : fun(f), id(id) {}
            const AstFunction* const fun;
            const uint64_t id;

            std::vector<AstVarMetadata*> args;
            std::vector<AstVarMetadata*> locals;
        };
        IR::Block* _currentBlock;
        uint64_t  _varCounter;
        uint64_t _blockCounter;
        AstFunction* _currentFunction;
        IR::IrRepr _ir;
        std::map<AstVar const*, AstVarMetadata*> _astvarMeta;
        std::map<uint64_t, AstVarMetadata*> _allvarMeta;
        std::map<AstFunction*, AstFunctionMetadata*> _funMeta;



        std::stack<IR::Atom const *> _lastAtoms;
        IR::Atom const* _popAtom() { const IR::Atom* a = _lastAtoms.top();_lastAtoms.pop(); return a; }
        void _pushAtom(IR::Atom const* atom) { _lastAtoms.push(atom); }

        void embraceArgs(AstFunction*);
        void embraceVars(Scope*);

        Scope *_lastScope;
        Parser const &_parser;

        std::ostream &_out;

        void visitAstFunction(AstFunction *function);

        uint64_t makeAstVar(AstVar const* var) {
             uint64_t id = _varCounter++;
            AstVarMetadata* md = new AstVarMetadata(var, id);
            _astvarMeta[var] = md;
            _allvarMeta[id] = md;
            return id;
        }
        uint64_t makeTempVar() {
            uint64_t id = _varCounter++;
            AstVarMetadata* md = new AstVarMetadata(id);
            _allvarMeta[id] = md;
            return id;
        }

    public:
        IrBuilder(Parser const &parser, ostream &debug)
                : _currentBlock(NULL), _varCounter(0), _blockCounter(0), _lastScope(parser.top()->scope()), _parser(parser), _out(debug),
                  _currentFunction(NULL) {
        }

        AstVarMetadata& varMeta(AstVar const* var) { return *(_astvarMeta[var]); }
        AstVarMetadata& varMetaById(uint64_t var) { return *(_allvarMeta[var]); }
        AstFunctionMetadata* funMeta(AstFunction *f) { return _funMeta[f]; }

        void start();

        FOR_NODES(VISITOR_FUNCTION)



private:
        std::string nextBlockName() {
            std::ostringstream str;
            str << _currentFunction->name() << '[' << _blockCounter++ << ']';
            return str.str();
        }
        IR::Block *newBlock() {
            std::string name = nextBlockName();
            return new IR::Block(name);
        }
        void emit(IR::Statement const* const statement) {
            _currentBlock->contents.push_back(statement);
        }

//        IR::Block *addBlock() {
//
//            IR::FunctionRecord *f = _lastFunction.top();
//            IR::Block *next = newBlock();
//
//
//            if (f->entry == NULL) {
//                f->entry = next;
//            }
//            else {
//                _lastBlock->link(next);
//                _lastBlock = next;
//            }
//
//            _lastBlock = next;
//            return next;
//        }

    };
}
