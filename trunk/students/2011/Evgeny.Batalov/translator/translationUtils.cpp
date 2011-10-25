#include "translationUtils.h"

mathvm::VarType upType(mathvm::VarType t1, mathvm::VarType t2) {
  switch (t1) {
    case VT_INI:
      if (t2 == VT_INT)
        return VT_INT;
      if (t2 == VT_DOUBLE)
        return VT_DOUBLE;
    break;
    case VT_DOUBLE:
      if (t2 == VT_INT || t2 == VT_DOUBLE)
        return VT_DOUBLE;
    break;
    case VT_STRING:
      if (t2 == VT_STRING)
        return VT_STRING;
    break;
    case VT_VOID:
      if (t2 == VT_VOID)
        return VT_VOID;
    break;
    case VT_INVALID:

    break;
  }
  throw new TranslationException("couldn't cast types");
  return mathvm::VT_INVALID;
}


