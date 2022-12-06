#pragma once

#include "odyssey/core/logger.h"

#if _MSC_VER
#include <intrin.h>
/** @brief Causes a debug breakpoint to be hit. */
#define debugBreak() __debugbreak()
#else
/** @brief Causes a debug breakpoint to be hit. */
#define debugBreak() __builtin_trap()
#endif

#if !IS_RELEASE

#define ASSERT(expr)                                                                                    \
    {                                                                                                   \
        if (expr) {                                                                                     \
        } else {                                                                                        \
            Logger::LogError("ASSERT FAILED {} {} {} ", #expr, __FILE__, __LINE__);                     \
            debugBreak();                                                                               \
        }                                                                                               \
    }


#define ASSERT_MSG(expr, message)                                                                       \
    {                                                                                                   \
        if (expr) {                                                                                     \
        } else {                                                                                        \
            Logger::LogError("ASSERT FAILED {} {} {} {} ", #expr, message, __FILE__, __LINE__);         \
            debugBreak();                                                                               \
        }                                                                                               \
    }

#else

#define ASSERT(expr)
#define ASSERT_MSG(expr, message)

#endif