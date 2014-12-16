#include "info.hpp"
#include "errors.hpp"

namespace mathvm {

static TypeInfo vtInvalid(VT_INVALID); 
static TypeInfo vtVoid(VT_VOID);
static TypeInfo vtDouble(VT_DOUBLE);
static TypeInfo vtInt(VT_INT);
static TypeInfo vtString(VT_STRING);

void setType(CustomDataHolder* dataHolder, VarType type) {
  TypeInfo* info;
  
  switch (type) {
    case VT_INVALID: 
      info = &vtInvalid;
      break;
    case VT_VOID: 
      info = &vtVoid;
      break;
    case VT_DOUBLE: 
      info = &vtDouble;
      break;
    case VT_INT: 
      info = &vtInt;
      break;
    case VT_STRING: 
      info = &vtString;
      break;
    default:
      throw InternalException("Unknown type %s", typeToName(type));
  }

  dataHolder->setInfo(info);
}

VarType typeOf(const CustomDataHolder* dataHolder) {
  return getInfo<TypeInfo>(dataHolder)->type();
} 

} // namespace mathvm