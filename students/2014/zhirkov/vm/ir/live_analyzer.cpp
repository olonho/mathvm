#include "live_analyzer.h"


namespace  mathvm {
    namespace IR {

        void printLiveInfo(LiveInfo &info, std::ostream &out) {
            out << "\n-------------------------------\n   Live variables \n-------------------------------\n";
            IrPrinter printer(out);
            for (auto fun : info.data) {
                out << "---- Function id " << fun.first->id << " ----\n";
                out << "  Ranges:\n";
                for (auto liveRange : fun.second->varIntervals)
                    out << liveRange.first << " : ["
                            << liveRange.second.getFrom()
                            << ", "
                            << liveRange.second.getTo()
                            << "] used "
                            << liveRange.second.getUsageCount()
                            << " times\n";

                size_t stIdx = 0;
                for (auto st: fun.second->orderedStatements) {
//                    out << "-- Block " << b->name << " --\n";
//                    auto currentStatement = fun.second.orderedStatements[stIdx];
                    out << stIdx << ":\n";
                    st->visit(&printer);
                    out << std::endl << " " << " --> ";
                    for (auto liveRange : fun.second->varIntervals)
                        if (liveRange.second.getFrom() <= stIdx && liveRange.second.getTo() >= stIdx)
                            out << " " << liveRange.second.var;

                    out << std::endl;
                    stIdx++;
                }
            }
        }

    }
}