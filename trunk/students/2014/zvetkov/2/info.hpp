#ifndef INFO_HPP
#define INFO_HPP 

#include "ast.h"
#include "mathvm.h"

#include <cassert>
#include <cstdlib>

namespace mathvm {

class TypeInfo {
  VarType type_;

public:
  TypeInfo(VarType type): type_(type) {}
  VarType type() const { return type_; }
};

class VarInfo {
  uint16_t functionId_;
  uint16_t localId_;

public:
  VarInfo(uint16_t functionId, uint16_t localId)
    : functionId_(functionId),
      localId_(localId) {} 

  uint16_t functionId() const { return functionId_; }

  uint16_t localId() const { return localId_; }
};

template<typename InfoT>
InfoT* getInfo(const CustomDataHolder* dataHolder) {
  return static_cast<InfoT*>(dataHolder->info());
}

void setType(CustomDataHolder* dataHolder, VarType type);
VarType typeOf(const CustomDataHolder* dataHolder);

} // namespace mathvm
#endif