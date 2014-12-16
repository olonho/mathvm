#pragma once
#include <iostream>
#include "../ir.h"
#include <map>


namespace mathvm {
    namespace IR {

        struct IdentityTransformation : public IrVisitor {
            FOR_IR(VISITOR)

            IdentityTransformation(SimpleIr const &old, char const *const name, std::ostream &_debug = std::cerr)
                    : _old(&old) , _currentSourceBlock(NULL), _currentIr(new SimpleIr()), name(name), _debug(_debug) {
                for( auto m : _old->varMeta)
                    _currentIr->varMeta.push_back(m);
            }

        protected:
            Block const *_currentSourceBlock;
            SimpleIr const* const _old;
            SimpleIr* _currentIr;
            std::ostream& _debug;
            char const* const name;
            bool visited(IrElement *e) {
                return _visited.find(e) != _visited.end();
            }

        public:
            SimpleIr* getResult() const {
                return _currentIr;
            }

            void start() {
                _debug << "\n-------------------------------\n   "
                        << name << " has started \n-------------------------------\n";
                for (auto f : _old->functions)
                    _currentIr->addFunction(static_cast<FunctionRecord *> (f->visit(this)));
            }
            private:

            std::map<IrElement const *, IrElement *> _visited;

        };

    }
}