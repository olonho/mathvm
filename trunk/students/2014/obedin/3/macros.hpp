#ifndef MACROS_HPP
#define MACROS_HPP

#include <exception>

#define MSG_NAN_ON_TOS          "Not a number on TOS"
#define MSG_NOT_INT_ON_TOS      "Not an integer on TOS"
#define MSG_NAN_ON_TOS_OR_PREV  "One or two elements on TOS are not numbers"
#define MSG_INVALID_UNARY       "Invaid unary operation"
#define MSG_INVALID_BINARY      "Invalid binary operation"
#define MSG_INVALID_CAST        "Can't cast types"
#define MSG_UNPRINTABLE_TOS     "Can't print value of this type"
#define MSG_UNLOADABLE_TYPE     "Can't load variable of this type"
#define MSG_UNSTOREABLE_TYPE    "Can't store value of this type"
#define MSG_INVALID_FOR_EXPR    "Invalid for(...) expression"
#define MSG_INVALID_FOR_VAR     "Invalid for(...) iterator variable"
#define MSG_FUNCTION_NOT_FOUND  "Function is not found"
#define MSG_VAR_NOT_FOUND       "Variable is not found"
#define MSG_INVALID_STORE_OP    "Invalid store operation"
#define MSG_WRONG_NUM_OF_PARAMS "Wrong number of function parameters"
#define MSG_NATIVE_NOT_FOUND    "Native function is not found"

#define MSG_INVALID_INSN        "Invaid instruction"
#define MSG_CTX_NOT_FOUND       "Context is not found"
#define MSG_UNEXPECTED_TOS      "Unexpected type of TOS"
#define MSG_CANT_CONVERT_TO_ASMJIT_TYPE "Can't convert VM type to AsmJit type"


#define NUMERIC_INSN(t, insn) ((t) == VT_INT ? BC_I##insn : (t) == VT_DOUBLE ? BC_D##insn : throw std::runtime_error(MSG_NAN_ON_TOS))
#define IS_NUMERIC(t) ((t) == VT_INT || (t) == VT_DOUBLE)
#define LOAD_VAR(t, id) ((t) == VT_INT ? BC_LOADIVAR##id : (t) == VT_DOUBLE ? BC_LOADDVAR##id : (t) == VT_STRING ? BC_LOADSVAR##id : throw std::runtime_error(MSG_UNLOADABLE_TYPE))
#define LOAD_CTX_VAR(t) ((t) == VT_INT ? BC_LOADCTXIVAR : (t) == VT_DOUBLE ? BC_LOADCTXDVAR : (t) == VT_STRING ? BC_LOADCTXSVAR : throw std::runtime_error(MSG_UNLOADABLE_TYPE))
#define STORE_VAR(t, id) ((t) == VT_INT ? BC_STOREIVAR##id : (t) == VT_DOUBLE ? BC_STOREDVAR##id : (t) == VT_STRING ? BC_STORESVAR##id : throw std::runtime_error(MSG_UNSTOREABLE_TYPE))
#define STORE_CTX_VAR(t) ((t) == VT_INT ? BC_STORECTXIVAR : (t) == VT_DOUBLE ? BC_STORECTXDVAR : (t) == VT_STRING ? BC_STORECTXSVAR : throw std::runtime_error(MSG_UNSTOREABLE_TYPE))

#endif /* end of include guard: MACROS_HPP */
