#pragma once

#include "../ir.h"
#include <map>


namespace mathvm {
    namespace IR {

        struct IdentityTransformation : public IrVisitor {
            FOR_IR(VISITOR)

            IdentityTransformation(SimpleIr const &old)
                    : _old(old) , _currentSourceBlock(NULL), _currentIr(old) {
                _currentIr.functions.clear();

            }

        protected:
            Block const *_currentSourceBlock;
            SimpleIr _old;
            SimpleIr _currentIr;

            bool visited(IrElement *e) {
                return _visited.find(e) != _visited.end();
            }

        public:
            SimpleIr getResult() const {
                return _currentIr;
            }

            void start() {
                for (auto f : _old.functions)
                    _currentIr.functions.push_back(static_cast<FunctionRecord *> (f->visit(this)));
            }
            private:

            std::map<IrElement const *, IrElement *> _visited;

        };

    }
}