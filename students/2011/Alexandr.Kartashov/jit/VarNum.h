#pragma once

#include "common.h"

// ================================================================================

namespace mathvm {
  class VarNum {
    public:

    typedef std::map<AstVar*, unsigned int> Map;

    VarNum() {
      _ints = 0;
      _doubles = 0;
    }

    int16_t add(AstVar* var) {
      //_vars.push_back(var);
      
      switch(var->type()) {
      case VT_INT:
        _intMap.insert(make_pair(var, _ints++));
        break;

      case VT_DOUBLE:
        _doubleMap.insert(make_pair(var, _doubles++));
        break;

      default:
        ABORT("Unable to handle a variable of the type other than VT_INT or VT_DOUBLE");
        break;
      }

      return 0; // just in case
    }

    bool exists(AstVar* var) {
      switch(var->type()) {
      case VT_INT:
        return _intMap.find(var) != _intMap.end();

      case VT_DOUBLE:
        return _doubleMap.find(var) != _doubleMap.end();

      default:
        ABORT("Unable to handle a variable of the type other than VT_INT or VT_DOUBLE");
      }

      return false;
    }
      
    unsigned int getId(AstVar* var) {
      switch(var->type()) {
      case VT_INT:
        return _intMap.find(var)->second;

      case VT_DOUBLE:
        return _doubleMap.find(var)->second;

      default:
        ABORT("Unable to handle a variable of the type other than VT_INT or VT_DOUBLE");
      }

      return 0;
    }

    unsigned int size() const {
      return _ints + _doubles;
    }

    Map& ints() {
      return _intMap;
    }

    Map& doubles() {
      return _doubleMap;
    }
  

  private:
    //std::vector<AstVar*> _vars;
    Map _intMap, _doubleMap;
    size_t _ints, _doubles;
  };
}
