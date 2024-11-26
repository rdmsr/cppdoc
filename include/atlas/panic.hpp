#pragma once
#include "impl.hpp"

namespace Atlas {

[[noreturn]] inline void panic(const char *message) {
  Impl::panic(message);
  __builtin_unreachable();
}

[[noreturn]] inline void todo() { panic("Not implemented"); }

} // namespace Atlas
