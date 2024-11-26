#pragma once
#include "panic.hpp"

#if defined(__clang__) || defined(__GNUC__)
#define LIKELY(condition) __builtin_expect(!!(condition), 1)
#define UNLIKELY(condition) __builtin_expect(!!(condition), 0)
#else
#define LIKELY(condition) (condition)
#define UNLIKELY(condition) (condition)
#endif

#if DEBUG_CHECKS
#define __STR(x) #x
#define STRINGIFY(x) __STR(x)

#define ASSERT(condition)                                                      \
  (LIKELY(condition) ? static_cast<void>(0)                                    \
                     : Atlas::panic("Assertion failed: " #condition            \
                                    " at " __FILE__ ":" STRINGIFY(__LINE__)))

#define ENSURE(condition, message)                                             \
  (LIKELY(condition)                                                           \
       ? static_cast<void>(0)                                                  \
       : Atlas::panic(message " at " __FILE__ ":" STRINGIFY(__LINE__)))
#else
#define ASSERT(condition) ((void)(condition))
#define ENSURE(condition, message) ((void)(condition))
#endif
