#pragma once

#include <iostream>
#include <map>
#include <set>
#include "live_analyzer.h"
#include <stack>

namespace mathvm {
    namespace IR {
        struct RegAllocInfo {
            std::map<IR::VarId, uint64_t> regAlloc;
            std::set<IR::VarId> stackAlloc;

            std::map<uint64_t, IR::VarId> vregToVarId;

            void allocateReg(IR::VarId id, uint64_t vreg) { regAlloc[id] = vreg; vregToVarId[vreg] = id; }
        };


        class RegAllocator {
            std::ostream &_debug;

            RegAllocInfo result;
            std::stack<uint32_t> regPool;

            typedef std::set<LiveInfo::Interval, LiveInfo::Interval::IntervalComparer> Intervals;

            Intervals actives;

            Intervals intervals;

            bool isRegAlloc(uint64_t var) const {
                return result.regAlloc.find(var) != result.regAlloc.end();
            }

            void expireOld(LiveInfo::Interval const &interval) {
                for (LiveInfo::Interval const &act : actives) {
                    if (act.getTo() >= interval.getFrom()) return;
                    if (isRegAlloc(act.var)) regPool.push(result.regAlloc[act.var]);
                    actives.erase(act);
                }
            }

            void spillAtInterval(LiveInfo::Interval const &interval) {
                LiveInfo::Interval const &spill = *(actives.rbegin());

                if (spill.getTo() > interval.getTo()) {
                    result.allocateReg(interval.var, result.regAlloc[spill.var]);
                    result.stackAlloc.insert(spill.var);
                    actives.erase(spill);
                    actives.insert(interval);
                }
                else
                    result.stackAlloc.insert(interval.var);
            }

        public:
            RegAllocator(uint32_t const regCount, std::ostream &debug)
                    : _debug(debug),
                      actives(LiveInfo::Interval::ordIncEnd),
                      intervals(LiveInfo::Interval::ordIncStart),
                      regCount(regCount) {
                for (uint32_t i = 0; i < regCount; i++) regPool.push(i);
            }

            const uint32_t regCount;

            RegAllocInfo alloc(LiveInfo::FunctionInfo const &liveInfo) {

                for (auto &item : liveInfo.varIntervals)   intervals.insert(item.second);

                //fixme  the order is shit?
                for (LiveInfo::Interval const &current : intervals) {
                    _debug << "Looking on the interval [" << current.getFrom() << "," << current.getTo() << "] for var " << current.var << "\n";
                    expireOld(current);
                    if (actives.size() == regCount)
                        spillAtInterval(current);
                    else {
                        auto newReg = regPool.top();
                        regPool.pop();
                        result.allocateReg(current.var, newReg);
                        actives.insert(current);
                    }
                }
                return result;
            }
        };

        void regAllocInfoDump(RegAllocInfo const &info, std::ostream &out);

        typedef std::vector<RegAllocInfo> GlobalRegAllocInfo;

        GlobalRegAllocInfo regAlloc(SimpleIr const &ir, LiveInfo const& info, uint32_t regCount, std::ostream &debug);

        void regAllocDump(GlobalRegAllocInfo const &info, std::ostream &debug);

    }
}