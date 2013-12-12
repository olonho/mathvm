#pragma once

#include "mathvm.h"

namespace mathvm
{

typedef double d_t;
typedef int64_t i_t;
typedef const char *s_t;

typedef uint16_t context_id_t;
typedef uint16_t var_id_t;
typedef uint16_t function_id_t;


template<typename T> T get_val(Var const &var); // no casts
template<> inline i_t get_val<i_t>(Var const &var) { return var.getIntValue   (); }
template<> inline d_t get_val<d_t>(Var const &var) { return var.getDoubleValue(); }
template<> inline s_t get_val<s_t>(Var const &var) { return var.getStringValue(); }

template<typename T> void set_val(Var &var, T val); // no casts
inline void set_val(Var &var, i_t val) { var.setIntValue   (val); }
inline void set_val(Var &var, d_t val) { var.setDoubleValue(val); }
inline void set_val(Var &var, s_t val) { var.setStringValue(val); }

template<typename T> Var create_val(T val);  // no casts
inline Var create_val(i_t val) { Var var(VT_INT   , ""); var.setIntValue   (val); return var; }
inline Var create_val(d_t val) { Var var(VT_DOUBLE, ""); var.setDoubleValue(val); return var; }
inline Var create_val(s_t val) { Var var(VT_STRING, ""); var.setStringValue(val); return var; }


struct error
    : std::runtime_error
{
    error(string const &msg)
        : std::runtime_error(msg)
    { }
};


} // namespace mathvm