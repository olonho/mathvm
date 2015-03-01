#pragma once

#include <map>
#include <iostream>
#include "../ir.h"


namespace mathvm {
    namespace IR {

        struct BaseTransform : public IrVisitor<> {

            BaseTransform() :
                    _currentSourceBlock(NULL),
                    _currentSourceFunction(NULL),
                    _currentResultFunction(NULL),
                    _currentResultBlock(NULL) {
            }

            virtual ~BaseTransform() {
            }

            virtual SimpleIr::StringPool visit(SimpleIr::StringPool const &pool);

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

            virtual IrElement *visit(const Function *const expr);

            virtual IrElement *visit(const JumpAlways *const expr);

            virtual IrElement *visit(const JumpCond *const expr);

            virtual IrElement *visit(const WriteRef *const expr);

            virtual IrElement *visit(const ReadRef *const expr);

        protected:
            typedef BaseTransform base;
            Block const *_currentSourceBlock;
            Function const *_currentSourceFunction;
            Function *_currentResultFunction;
            Block *_currentResultBlock;

            std::map<IrElement const *, IrElement const *> _visited;

            virtual bool visited(IrElement *e) {
                return _visited.find(e) != _visited.end();
            }
        };

        template<typename Status=void>
        struct Transformation : public BaseTransform {

            char const *const name;

            Transformation(SimpleIr const &source, SimpleIr &dest, char const *const name, std::ostream &_debug = std::cerr)
                    :
                    name(name),
                    _oldIr(source),
                    _newIr(dest),
                    _debug(_debug) {
            }

            virtual ~Transformation() {
            }

            virtual SimpleIr *visit(const SimpleIr *const expr) {
                _debug << "\n-------------------------------\n   "
                        << name << " has started \n-------------------------------\n";
                for (auto &v: _oldIr.varMeta)
                    _newIr.varMeta.push_back(v);

                for (auto f : _oldIr.functions) {
                    Function *ft = static_cast<Function *> (f->visit(this));
                    if (ft) {
                        _newIr.addFunction(ft);
                    }
                }
                _newIr.pool = BaseTransform::visit(_oldIr.pool);

                for (auto kvp : _visited)
                    if (kvp.first->isBlock()) {
                        Block *b = const_cast<Block *>(kvp.first->asBlock());
                        std::vector<Block const *> newpreds;
                        for (auto oldpred : b->predecessors) {
                            newpreds.push_back(_visited[oldpred]->asBlock());
                        }
                        b->predecessors.erase(b->predecessors.begin(), b->predecessors.end());
                        b->predecessors.insert(newpreds.begin(), newpreds.end());
                    }

                return &_newIr;
            }

            virtual Status operator()() = 0;

        protected:


            SimpleIr const &_oldIr;
            SimpleIr &_newIr;
            std::ostream &_debug;


            void emit(Statement const *statement) {
                _currentResultBlock->contents.push_back(statement);
            }

            VarId makeVar(VarType type = VT_Undefined) {
                auto newid = _newIr.varMeta.size();
                SimpleIr::VarMeta newvarmeta(newid);
                newvarmeta.type = type;
                _newIr.varMeta.push_back(newvarmeta);
                return newid;
            }

        };

        struct Copier : public Transformation<> {

            Copier(SimpleIr const &source, SimpleIr &dest, std::ostream &_debug)
                    : Transformation(source, dest, "copier", _debug) {
            }

            virtual void operator()() {
                Transformation<>::visit(&_oldIr);
            }
        };

        extern BaseTransform copier;
    }
}