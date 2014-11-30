#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>

namespace mathvm {

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

} // namespace mathvm

#endif