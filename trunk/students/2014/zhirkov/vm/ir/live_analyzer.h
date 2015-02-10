#pragma once

#include <map>
#include <vector>
#include <set>
#include <iostream>
#include "ir.h"
#include "ir_printer.h"
#include "ir_analyzer.h"
#include "../translator/ssa_utils.h"
#include "../translator/reg_common.h"

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

                const uint64_t var;

                Interval(size_t const from, size_t const to, uint64_t const var)
                        : foundUsage(false), usageCount(0), from(from), to(to), var(var) {
                }

                Interval(uint64_t const var) : foundUsage(false), usageCount(0), from(0), to(0), var(var) {
                }

                bool defined() {
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

                static bool ordIncStart(Interval const &left, Interval const &right) {
                    if (left.from == right.from) return left.to < right.to;
                    return left.from < right.from;
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
                    if (left.to == right.to) return left.from < right.from;
                    return left.to < right.to;
                }
            };


            struct FunctionInfo {
                const std::vector<Block const *> orderedBlocks;
                std::vector<Statement const *> orderedStatements;
                std::map<uint64_t, Interval> varIntervals;

                FunctionInfo(FunctionRecord const *functionRecord) :
                        orderedBlocks(blocksOrder(functionRecord->entry)) {
                    for (auto b : orderedBlocks) {
                        for (auto st : b->contents)
                            orderedStatements.push_back(st);
                        if (!b->isLastBlock()) orderedStatements.push_back(b->getTransition());
                    }

                }

                ~FunctionInfo() {
                }

                Interval *forVar(uint64_t id) {
                    auto found = varIntervals.find(id);
                    if (found != varIntervals.end()) return &found->second; else return NULL;
                }

            };

            std::map<FunctionRecord const *, FunctionInfo *> data;

            void vivify(FunctionRecord const *fun) {
                if (data.find(fun) == data.end()) data.insert(make_pair(fun, new FunctionInfo(fun)));
            }

            Interval &vivify(FunctionRecord const *fun, uint64_t var) {

                if (data.at(fun)->varIntervals.find(var) == data.at(fun)->varIntervals.end())
                    data.at(fun)->varIntervals.insert(std::map<uint64_t, Interval>::value_type(var, Interval(var))); //.insert(make_pair(var, new Interval(var)));
                return data.at(fun)->varIntervals.at(var);
            }
        };


        struct LiveAnalyzer : public IrAnalyzer<int, LiveInfo *> {
        private:
            std::vector<IR::SimpleIr::VarMeta> const *varMeta;
        public:
            virtual int visit(const Call *const expr) {
                for (auto p : expr->params) visitElement(p);
                for (auto rp : expr->refParams) vivify(rp);
                return 0;
            }

            virtual int visit(const Print *const expr) {
                visitElement(expr->atom);
                return 0;
            }

            virtual int visit(const JumpCond *const expr) {
                visitElement(expr->condition);
                return 0;
            }

            virtual int visit(const Variable *const expr) {
                vivify(expr->id);
                return 0;
            }

            virtual int visit(const Assignment *const expr) {
                if (!expr->var->id == SINK && varMeta->at(expr->var->id).type == IR::VT_Unit) return 0;
                LiveInfo::Interval &interval = _status->vivify(_currentFunction, expr->var->id);
                interval.mark(_currentPosition);
                visitExpression(expr->value);
                return 0;
            }

            virtual int visit(const Phi *const expr) {
                auto &interval = _status->vivify(_currentFunction, expr->var->id);
                interval.mark(_currentPosition);
                return 0;
            }

            virtual int visit(const WriteRef *const expr) {
                _status->vivify(_currentFunction, expr->refId);
                visitExpression(expr->atom);
                return 0;
            }

            virtual int visit(const BinOp *const expr) {
                visitElement(expr->left);
                visitElement(expr->right);
                return 0;
            }

            virtual int visit(const UnOp *const expr) {
                return visitElement(expr->operand);
            }

            virtual int visit(const Return *const expr) {
                return visitElement(expr->atom);
            }

            LiveAnalyzer(ostream &debug) : IrAnalyzer(debug, "liveness"), varMeta(NULL) {
                _status = new LiveInfo();
            }

            LiveInfo *start(SimpleIr const &ir) {
                varMeta = &ir.varMeta;
                for (auto f : ir.functions) {
                    _currentFunction = f;
                    _currentPosition = 0;
                    _status->vivify(f);

                    embraceArgs(f);
                    for (auto st : _status->data.at(f)->orderedStatements) {
                        visitElement(st);
                        _currentPosition++;
                    }
                    _currentPosition++;
                }
                return _status;
            }

        protected:
            virtual int defaultAnswer() {
                return 0;
            }

        private:
            void embraceArgs(FunctionRecord const *f) {
                for (auto param: f->parametersIds) {
                    auto &interval = _status->vivify(_currentFunction, param);
                    interval.mark(_currentPosition);
                }
            }

            FunctionRecord *_currentFunction;
            size_t _currentPosition;

            void vivify(uint64_t id) {
                _status->vivify(_currentFunction, id).mark(_currentPosition);
            }
        };


        void printLiveInfo(LiveInfo &info, std::ostream &out);

    }
}