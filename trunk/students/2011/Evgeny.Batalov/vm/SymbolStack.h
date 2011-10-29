#pragma once
#include <map>
#include <vector>
#include <string>
#include "TranslationException.h"

template<typename T> 
class SymbolStack {
protected:
  typedef std::vector<T> Stack;
  typedef std::map<std::string, Stack> Map;
  typedef typename Map::iterator MapIt;
  typedef typename Stack::iterator StackIt;

  Map map;
public:
  void pushSymbolData(std::string symbol, const T& data) {
    DEBUG("push " << symbol << std::endl);
    Stack& st = map[symbol];
    st.push_back(data);
  }
  
  void pushSymbol(std::string symbol) {
    map[symbol];
  }

  T& topSymbolData(std::string symbol) {
    MapIt it = map.find(symbol);
    if (it != map.end()) {
        return it->second.back();
    }
    throw new TranslationException("symbol " + symbol  +  " is undefined", 0);
    return *(T*)0;
  }
  
  void popSymbolData(std::string symbol) {
    DEBUG("pop " << symbol << std::endl);
    MapIt it = map.find(symbol);
    if (it->second.size()) {
        it->second.pop_back();
        return;
    }
    throw new TranslationException("symbol " + symbol  +  " is undefined", 0);
  }
};
