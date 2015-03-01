#include "live_analyzer.h"


namespace  mathvm {
    namespace IR {

        void printLiveInfo(LiveInfo &info, std::ostream &out) {
            out << "\n-------------------------------\n   Live variables \n-------------------------------\n";
            IrPrinter printer(out);
            size_t idx = 0;
            for (auto fun : info.data) {
                out << "---- Function id " << idx++ << " ----\n";
                out << "  Ranges:\n";
                for (auto liveRange : fun->varIntervals)
                    out << liveRange.first << " : ["
                            << liveRange.second.getFrom()
                            << ", "
                            << liveRange.second.getTo()
                            << "] used "
                            << liveRange.second.getUsageCount()
                            << " times\n";

                size_t stIdx = 0;
                for (auto st: fun->orderedStatements) {
//                    out << "-- Block " << b->name << " --\n";
//                    auto currentStatement = fun.second.orderedStatements[stIdx];
                    out << stIdx << ":\n";
                    st->visit(&printer);
                    out << std::endl << " " << " --> ";
                    for (auto liveRange : fun->varIntervals)
                        if (liveRange.second.getFrom() <= stIdx && liveRange.second.getTo() >= stIdx)
                            out << " " << liveRange.second.var;

                    out << std::endl;
                    stIdx++;
                }
            }
        }

        bool LiveAnalyzer::varMatters(Function const& f, VarId var) const {
            if (std::find(f.memoryCells.cbegin(), f.memoryCells.cend(), var) != f.memoryCells.cend()) return false;
            if (_hasAccRegs && var >= varMeta->size()) return false;
            return true;
        }

    }
}