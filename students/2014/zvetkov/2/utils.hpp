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

template<typename A1, typename A2, typename A3, typename A4>
void debug(A1 a1, A2 a2, A3 a3, A4 a4) {
  std::cout << a1 << a2 << a3 << a4;
  std::cout << std::endl;
}

template<typename A1, typename A2, typename A3, typename A4, typename A5>
void debug(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) {
  std::cout << a1 << a2 << a3 << a4 << a5;
  std::cout << std::endl;
}

template<typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
void debug(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) {
  std::cout << a1 << a2 << a3 << a4 << a5 << a6;
  std::cout << std::endl;
}

} // namespace mathvm

#endif