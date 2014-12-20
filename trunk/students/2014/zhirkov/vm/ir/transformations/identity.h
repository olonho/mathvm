#pragma once

#include <iostream>
#include "../ir.h"
#include <map>


namespace mathvm {
    namespace IR {

        struct IdentityTransformation : public IrVisitor {

            FOR_IR(VISITOR)

            IdentityTransformation(SimpleIr const &old, char const *const name, std::ostream &_debug = std::cerr)
                    : _old(&old), _currentSourceBlock(NULL), _currentIr(new SimpleIr()), name(name), _debug(_debug) {
                for (auto m : _old->varMeta)
                    _currentIr->varMeta.push_back(m);
            }

        protected:
            Block const*_currentSourceBlock;
            FunctionRecord const* _currentSourceFunction;
            FunctionRecord * _currentResultFunction;
            Block *_currentResultBlock;

            SimpleIr const *const _old;
            SimpleIr *_currentIr;
            std::ostream &_debug;
            char const *const name;

            virtual bool visited(IrElement *e) {
                return _visited.find(e) != _visited.end();
            }

            void emit(Statement const* statement) { _currentResultBlock->contents.push_back(statement); }
            uint64_t makeVar(VarType type = VT_Undefined) {
                auto newid = _currentIr->varMeta.size();
                SimpleIr::VarMeta newvarmeta(newid);
                newvarmeta.type = type;
                _currentIr->varMeta.push_back(newvarmeta);
                return newid;
            }
        public:
            SimpleIr *result() const {
                return _currentIr;
            }

            void start() {
                _debug << "\n-------------------------------\n   "
                        << name << " has started \n-------------------------------\n";
                for (auto f : _old->functions) {
                    auto ft = f->visit(this);
                    if (ft)
                        _currentIr->addFunction(static_cast<FunctionRecord *> (ft));
                }
            }


        private:

            std::map<IrElement const *, IrElement *> _visited;

        };

    }
}