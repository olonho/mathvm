#pragma once

#include <sstream>
#include <stack>
#include <map>
#include "../../../../../vm/parser.h"
#include "../ir/ir.h"
#include "../common.h"
#include "../ir/ir_printer.h"
#include "../ast_metadata.h"
#include "ssa_utils.h"

namespace mathvm {


    class SimpleIrBuilder : public AstVisitor {
        struct AstVarMetadata {
            AstVarMetadata(AstVar const *const var, uint64_t const id) : isTemp(false), var(var), id(id) { }


            AstVarMetadata(uint64_t const id) : isTemp(true), var(NULL), id(id) {}

            const bool isTemp;
            const AstVar* const var;
            const uint64_t id;
        };

        struct AstFunctionMetadata {
            AstFunctionMetadata(const AstFunction* f, const uint16_t id) : fun(f), id(id) {}
            const AstFunction* const fun;
            const uint16_t id;

            std::vector<AstVarMetadata*> args;
            std::vector<AstVarMetadata*> locals;
        };

        IR::SimpleIr* _ir;

        IR::Block* _currentBlock;
        AstFunction* _currentFunction;
        Scope *_lastScope;

        uint64_t  _varCounter;
        uint64_t _blockCounter;

        std::map<AstVar const*, AstVarMetadata*> _astvarMeta;
        std::map<uint64_t, AstVarMetadata*> _allvarMeta;
        std::map<AstFunction*, AstFunctionMetadata*> _funMeta;


        std::stack<IR::Atom const *> _lastAtoms;
        IR::Atom const* _popAtom() { const IR::Atom* a = _lastAtoms.top();_lastAtoms.pop(); return a; }
        void _pushAtom(IR::Atom const* atom) { _lastAtoms.push(atom); }

        void visitAstFunction(AstFunction *function);
        void declareFunction(AstFunction * fun) ;

        void insertPhi();
    public:
        SimpleIrBuilder(Parser const &parser, ostream &debug)
                : _currentBlock(NULL), _varCounter(0), _blockCounter(0), _lastScope(parser.top()->scope()), _parser(parser), _out(debug),
                  _currentFunction(NULL), _ir(new IR::SimpleIr()) {
        }

        AstVarMetadata& varMeta(AstVar const* var) { return *(_astvarMeta[var]); }
        AstVarMetadata& varMetaById(uint64_t var) { return *(_allvarMeta[var]); }
        AstFunctionMetadata& funMeta(AstFunction *f) { return *(_funMeta[f]); }

        IR::SimpleSsaIr* start();

        FOR_NODES(VISITOR_FUNCTION)



private:

        Parser const &_parser;
        std::ostream &_out;

        void embraceArgs(AstFunction*);
        void embraceVars(Scope*);

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

        uint64_t makeAstVar(AstVar const* var);
        uint64_t makeTempVar();

    public:
        virtual ~SimpleIrBuilder() {
            for(auto v : _allvarMeta)
                delete v.second;
            for (auto f : _funMeta)
                delete f.second;
            while(! _lastAtoms.empty()){
                delete _lastAtoms.top();
                _lastAtoms.pop();
            }
        }
    };
}
