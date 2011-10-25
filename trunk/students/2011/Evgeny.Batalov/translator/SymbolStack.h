#pragma ince
#include <map>
#include <vector>
#include <string>
#include "TranslationException.h"

template<typename T> 
class SymboloStack {
protected:
  typedef std::vector<T> Stack;
  typedef std::map<std::string, Stack> Map;
  typedef typename Map::iterator MapIt;
  typedef typename Stack::iterator StackIt;

  Map map;
public:
  void pushSymbolData(std::string symbol, const T& data) {
    Stack& st = map[var.name];
    st.push_back(data);
  }

  void pushSymbol(std::string symbol) {
    map[var.name];
  }

  T& topSymbolData(std::string symbol) {
    MapIt it = map.find(symbol);
    if (it != map.end()) {
        return it->second.back();
    }
    throw new TranslationException("symbol " + name  +  " is undefined");
    return *(T*)0;
  }
  void popSymbolData(std::string symbol) {
    MapIt it = map.find(symbol);
    if (it != map.end()) {
        it->second.pop_back();
    }
    throw new TranslationException("symbol " + name  +  " is undefined");
  }
};
