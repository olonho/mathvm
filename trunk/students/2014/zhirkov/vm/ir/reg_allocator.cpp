#include "reg_allocator.h"


namespace mathvm {
    namespace IR {
        GlobalRegAllocInfo regAlloc(SimpleIr const&ir, LiveInfo const &info, uint32_t regCount, std::ostream &debug) {
            GlobalRegAllocInfo result;
            for (auto f : ir.functions) {
                debug << "Function " << f->id << std::endl;
                LiveInfo::FunctionInfo const *inf = info.data.at(f->id);
                result.push_back(RegAllocator(regCount, debug).alloc(*inf));
            }
            return result;
        }
        void regAllocDump(GlobalRegAllocInfo const&info, std::ostream &debug) {
            debug << "-------------------------\nRegistry allocation info\n-------------------------\n";
            size_t i = 0;
            for (GlobalRegAllocInfo::value_type fun : info) {
                debug << "Function " << i++ << std::endl;
                regAllocInfoDump(fun, debug);
            }
        }

        void regAllocInfoDump(RegAllocInfo const &info, std::ostream &out) {
            out << "Vars -> registers\n";
            for (auto kvp : info.regAlloc)
                out << kvp.first << " -> " << kvp.second << std::endl;
            out << "Stack allocated: ";
            for (auto sa : info.stackAlloc)
                out << sa << " ";
            out << std::endl;
        }


    }
}