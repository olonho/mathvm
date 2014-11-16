#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>

template<typename A1>
void debug(A1 a1) {
  std::cout << a1;
  std::cout << std::endl;
}

template<typename A1, typename A2>
void debug(A1 a1, A2 a2) {
  std::cout << a1 << a2;
  std::cout << std::endl;
}

template<typename A1, typename A2, typename A3>
void debug(A1 a1, A2 a2, A3 a3) {
  std::cout << a1 << a2 << a3;
  std::cout << std::endl;
}

namespace mathvm {

bool isTopLevel(AstFunction* function) {
  return function->name() == AstFunction::top_name;
}

}

/*namespace mathvm {

const char* tokenToName(TokenKind token) {
  #define ENUM_ELEM(t, s, p) if (token == t) { return s; }
    FOR_TOKENS(ENUM_ELEM)
  #undef ENUM_ELEM

  return "<unknown>";
}

}*/
#endif