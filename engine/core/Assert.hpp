#pragma once

#include <cstdlib>
#include <fmt/core.h>

#ifdef NDEBUG
#  define HIROMI_ASSERT(cond, ...) ((void)0)
#else
#  define HIROMI_ASSERT(cond, ...)                                          \
     do {                                                                    \
       if (!(cond)) {                                                        \
         fmt::print(stderr, "[HIROMI_ASSERT] {}:{}: condition `{}` failed", \
                    __FILE__, __LINE__, #cond);                              \
         if constexpr (sizeof(#__VA_ARGS__) > 1)                            \
           fmt::print(stderr, " — " __VA_ARGS__);                           \
         fmt::print(stderr, "\n");                                           \
         std::abort();                                                       \
       }                                                                     \
     } while (false)
#endif
