#include "TranslationUtils.h"

mathvm::VarType upType(mathvm::VarType t1, mathvm::VarType t2) {
  using namespace mathvm;
  switch (t1) {
    case VT_INT:
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
  throw new TranslationException("couldn't cast types", 0);
  return mathvm::VT_INVALID;
}

bool symUsed(const SymbolsUse& a, const std::string& str, size_t user) {
  SymbolsUse::const_iterator it = a.begin();
  for(; it != a.end(); ++it) {
    if (it->first == str && it->second == user)
      return true;
  }
  return false;
}

bool symUsed(const Strings& a, const std::string& str) {
  Strings::const_iterator it = a.begin();
  for(; it != a.end(); ++it) {
    if (*it == str)
      return true;
  }
  return false;
}

const SymbolUse& findSymUse(const SymbolsUse& sUses , const std::string& sym) {
  SymbolsUse::const_iterator it = sUses.begin();
  for(; it != sUses.end(); ++it) {
    if (it->first == sym){
      return *it;
    }
  }
  throw new TranslationException("No symbol " + sym + "  found", 0);
}
