#pragma once

#include <map>
#include <set>
#include "identity.h"
#include "../../common.h"

namespace mathvm {
    namespace IR {
        struct SsaTransformation : public IdentityTransformation {
            std::map<uint64_t, uint64_t> _latestVersion;
            SsaTransformation(SimpleIr& old) : IdentityTransformation(old), meta(_currentIr->varMeta) {
            }

            bool shouldBeRenamed(uint64_t id) {
                return _latestVersion.find(id) != _latestVersion.end();
            }

            FOR_IR(VISITOR);

        private:
            std::vector<IR::SimpleIr::VarMeta>& meta;
        };


//
//        SimpleIr start(SimpleIr const& old) {
//            SimpleIr result;
//            result.varMeta = old.varMeta;
//            result.pool = old.pool;
//            for ( auto f : old.functions)
//                result.functions.push_back(f->visit(this)->asFunctionRecord());
//            return result;
//        }

    }
}
