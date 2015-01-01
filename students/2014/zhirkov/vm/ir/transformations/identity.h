#pragma once

#include <iostream>
#include "../ir.h"
#include "../../translator/ssa_utils.h"
#include <map>


namespace mathvm {
    namespace IR {


        struct Transformation : public IrVisitor {

            virtual ~Transformation() {
            }

            virtual void visit(SimpleIr::StringPool const &pool);

            virtual IrElement *visit(const BinOp *const expr);

            virtual IrElement *visit(const UnOp *const expr);

            virtual IrElement *visit(const Variable *const expr);

            virtual IrElement *visit(const Return *const expr);

            virtual IrElement *visit(const Phi *const expr);

            virtual IrElement *visit(const Int *const expr);

            virtual IrElement *visit(const Double *const expr);

            virtual IrElement *visit(const Ptr *const expr);

            virtual IrElement *visit(const Block *const expr);

            virtual IrElement *visit(const Assignment *const expr);

            virtual IrElement *visit(const Call *const expr);

            virtual IrElement *visit(const Print *const expr);

            virtual IrElement *visit(const FunctionRecord *const expr);

            virtual IrElement *visit(const JumpAlways *const expr);

            virtual IrElement *visit(const JumpCond *const expr);

            virtual IrElement *visit(const WriteRef *const expr);

            virtual IrElement *visit(const ReadRef *const expr);

            Transformation(SimpleIr const *old, char const *const name, std::ostream &_debug = std::cerr)
                    :
                    _currentSourceBlock(NULL),
                    _currentSourceFunction(NULL),
                    _currentResultFunction(NULL),
                    _currentResultBlock(NULL),
                    _old(old),
                    _currentIr((SimpleIr *) (old ? (new SimpleIr()) : NULL)),
                    _debug(_debug),
                    name(name) {
                if (_old)
                    for (auto m : _old->varMeta)
                        _currentIr->varMeta.push_back(m);

            }

        protected:
            Block const *_currentSourceBlock;
            FunctionRecord const *_currentSourceFunction;
            FunctionRecord *_currentResultFunction;
            Block *_currentResultBlock;

            SimpleIr const *const _old;
            SimpleIr *_currentIr;
            std::ostream &_debug;
            char const *const name;

            virtual bool visited(IrElement *e) {
                return _visited.find(e) != _visited.end();
            }

            void emit(Statement const *statement) {
                _currentResultBlock->contents.push_back(statement);
            }

            VarId makeVar(VarType type = VT_Undefined) {
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

            virtual void start() {
                _debug << "\n-------------------------------\n   "
                        << name << " has started \n-------------------------------\n";
                for (auto f : _old->functions) {
                    FunctionRecord *ft = static_cast<FunctionRecord *> (f->visit(this));
                    if (ft) {
                        _currentIr->addFunction(ft);
                    }
                }
                visit(_old->pool);

                for (auto kvp : _visited)
                    if (kvp.first->isBlock()) {
                        Block * b = const_cast<Block*>(kvp.first->asBlock());
                        std::vector<Block const*> newpreds;
                        for (auto oldpred : b->predecessors){
                            newpreds.push_back(_visited[oldpred]->asBlock());
                        }
                        b->predecessors.erase(b->predecessors.begin(), b->predecessors.end());
                        b->predecessors.insert(newpreds.begin(), newpreds.end());
                    }
            }

        private:

            std::map<IrElement const*, IrElement const*> _visited;

        };

        extern Transformation copier;
    }
}