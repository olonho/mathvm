#pragma once

#include <map>
#include <vector>
#include <set>
#include <iostream>
#include <algorithm>
#include "ir.h"
#include "ir_printer.h"
#include "ir_analyzer.h"
#include "../translator/ssa_utils.h"

namespace mathvm {
    namespace IR {
        struct LiveInfo {
            class Interval {
                bool foundUsage;
                uint64_t usageCount;
                size_t from;
                size_t to;
            public:
                typedef bool(*IntervalComparer)(Interval const &, Interval const &);

                const IR::VarId var;

                Interval(size_t const from, size_t const to, uint64_t const var)
                        : foundUsage(false), usageCount(0), from(from), to(to), var(var) {
                }

                Interval(IR::VarId const var) : foundUsage(false), usageCount(0), from(0), to(0), var(var) {
                }

                bool defined() const {
                    return foundUsage;
                }

                void mark(size_t pos) {
                    if (!foundUsage) {
                        from = pos;
                        foundUsage = true;
                    }
                    else usageCount++;
                    to = pos;
                }

                bool contains(size_t position) const { return getFrom() <= position && getTo() >= position; }

                static bool ordIncStart(Interval const &left, Interval const &right) {
                    if (left.from < right.from) return true;
                    else if (left.to < right.to) return true;
                    else return left.var < right.var;
                }

                size_t getFrom() const {
                    return from;
                }

                size_t getTo() const {
                    return to;
                }

                uint64_t getUsageCount() const {
                    return usageCount;
                }

                static bool ordMoreUsages(Interval const &left, Interval const &right) {
                    return left.getUsageCount() < right.getUsageCount();
                }

                static bool ordIncEnd(Interval const &left, Interval const &right) {
                    if (left.from > right.from) return true;
                    else if (left.to > right.to) return true;
                    else return left.var < right.var;
                }
            };


            struct FunctionInfo {
                const std::vector<Block const *> orderedBlocks;
                std::vector<Statement const *> orderedStatements;
                std::map<IR::VarId, Interval> varIntervals;

                FunctionInfo(Function const *functionRecord) :
                        orderedBlocks(blocksOrder(functionRecord->entry)) {
                    for (auto b : orderedBlocks) {
                        for (auto st : b->contents)
                            orderedStatements.push_back(st);
                        if (!b->isLastBlock()) orderedStatements.push_back(b->getTransition());
                    }

                }

                ~FunctionInfo() {
                }

                Interval *forVar(IR::VarId id) {
                    auto found = varIntervals.find(id);
                    if (found != varIntervals.end()) return &found->second; else return NULL;
                }

            };

            std::vector<FunctionInfo *> data;

            void vivify(Function const *fun) {
                if (fun->id >= data.size()) data.resize(fun->id+1);
                if (! data.at(fun->id)) data.at(fun->id) = new FunctionInfo(fun);
            }

            Interval &vivify(Function const *fun, uint64_t var) {

                if (data.at(fun->id)->varIntervals.find(var) == data.at(fun->id)->varIntervals.end())
                    data.at(fun->id)->varIntervals.insert(std::map<uint64_t, Interval>::value_type(var, Interval(var))); //.insert(make_pair(var, new Interval(var)));
                return data.at(fun->id)->varIntervals.at(var);
            }

            virtual ~LiveInfo() { 
            }
        };


        struct LiveAnalyzer : public IrAnalyzer<int, LiveInfo *> {
        private:
            const bool _hasAccRegs;
            std::vector<IR::SimpleIr::VarMeta> const *varMeta;
            bool varMatters(Function const& f, VarId var) const;
        public:
            virtual int visit(const Call *const expr)  const {
                for (auto p : expr->params) visitExpression(p);
                for (auto rp : expr->refParams) vivify(rp);
                return 0;
            }

            virtual int visit(const Print *const expr) const  {
                visitElement(expr->atom);
                return 0;
            }

            virtual int visit(const JumpCond *const expr) const  {
                visitElement(expr->condition);
                return 0;
            }

            virtual int visit(const Variable *const expr) const  {
                vivify(expr->id);
                return 0;
            }

            virtual int visit(const Assignment *const expr) const  {
                visitExpression(expr->value);
                if (!varMatters(*_currentFunction, expr->var->id)) return 0;
                if (varMeta->at(expr->var->id).type == IR::VT_Unit) return 0;
                LiveInfo::Interval &interval = _status->vivify(_currentFunction, expr->var->id);
                interval.mark(_currentPosition);
                return 0;
            }

            virtual int visit(const Phi *const expr) const  {
                if (!varMatters(*_currentFunction, expr->var->id)) return 0;
                auto &interval = _status->vivify(_currentFunction, expr->var->id);
                interval.mark(_currentPosition);
                return 0;
            }

            virtual int visit(const WriteRef *const expr) const  {
                visitExpression(expr->atom);
                if (!varMatters(*_currentFunction, expr->refId)) return 0;
                _status->vivify(_currentFunction, expr->refId);
                return 0;
            }

            virtual int visit(const BinOp *const expr) const  {
                visitElement(expr->left);
                visitElement(expr->right);
                return 0;
            }

            virtual int visit(const UnOp *const expr) const  {
                return visitElement(expr->operand);
            }

            virtual int visit(const Return *const expr)  const {
                return visitElement(expr->atom);
            }

            LiveAnalyzer(bool hasAccRegs = false, ostream &debug = std::cerr) :
                    IrAnalyzer(debug, "liveness"),
                    _hasAccRegs(hasAccRegs),
                    varMeta(NULL) {
                _status = new LiveInfo();
            }

            LiveInfo *start(SimpleIr const &ir) {
                varMeta = &ir.varMeta;
                for (auto f : ir.functions) {
                    _currentFunction = f;
                    _currentPosition = 0;
                    _status->vivify(f);

                    embraceArgs(f);
                    for (auto st : _status->data.at(f->id)->orderedStatements) {
                        visitElement(st);
                        _currentPosition++;
                    }
                   // _currentPosition++;
                }
                for (auto f: _status->data)
                    for(size_t i = 0; i < f->orderedStatements.size(); i++)
                        f->orderedStatements[i]->num = i;
                return _status;
            }

        protected:
            virtual int defaultAnswer()  const {
                return 0;
            }

        private:
            void embraceArgs(Function const *f) {
                for (auto param: f->parametersIds) if (param < varMeta->size()) {
                    auto &interval = _status->vivify(_currentFunction, param);
                    interval.mark(_currentPosition);
                }
            }

            Function *_currentFunction;
            size_t _currentPosition;

            void vivify(IR::VarId id)  const  {
                if (varMatters(*_currentFunction, id))
                     _status->vivify(_currentFunction, id).mark(_currentPosition);
            }
        };


        void printLiveInfo(LiveInfo &info, std::ostream &out);

    }
}