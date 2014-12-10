#ifndef VALUE_HPP
#define VALUE_HPP

#include <iostream>

#ifdef MATHVM_WITH_SDL
#include <dlfcn.h>

inline void pre_init_sdl() {
    void *cocoa_lib = dlopen("/System/Library/Frameworks/Cocoa.framework/Cocoa", RTLD_LAZY );
    void (*nsappload)(void);
    nsappload = (void(*)()) dlsym(cocoa_lib, "NSApplicationLoad");
    nsappload();
}

#endif

typedef union _v {
    double d;
    int64_t i;
    const char *c;
    _v() {}
    _v(double d) : d(d) {}
    _v(int64_t i) : i(i) {}
    _v(const char *c): c(c) {}
} Value;

inline std::ostream & operator<<(std::ostream & out, const Value & v) {
    return out << v.i;
}

#endif // VALUE_HPP
