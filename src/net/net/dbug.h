#ifndef TINY_NET_DBUG_H
#define TINY_NET_DBUG_H

#include "dbug_module.h"

typedef enum debug_level_t
{
    DBUG_LEVEL_NONE = 0,
    DBUG_LEVEL_ERROR,
    DBUG_LEVEL_WARN,
    DBUG_LEVEL_INFO,
    DBUG_LEVEL_DEBUG,
    DBUG_LEVEL_TRACE,
} debug_level_t;

#ifndef DBUG_DISPLAY_ENABLED
#define DBUG_DISPLAY_ENABLED 1
#endif

#define DBG_DISPLAY_ENABLE(module) (DBUG_DISPLAY_ENABLED)

#define DBG_DISPLAY_CHECK(module) dbug_module_is_enabled(module)

void dbug_printf(debug_level_t level, dbug_module_t module,
                const char* file, const char* func, int line,
                const char* fmt, ...);

#define dbug_info(module, fmt, ...) \
    dbug_printf(DBUG_LEVEL_INFO, module, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#define dbug_error(module, fmt, ...) \
    dbug_printf(DBUG_LEVEL_ERROR, module, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#define dbug_warn(module, fmt, ...) \
    dbug_printf(DBUG_LEVEL_WARN, module, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#define dbug_debug(module, fmt, ...) \
    dbug_printf(DBUG_LEVEL_DEBUG, module, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#define dbug_trace(module, fmt, ...) \
    dbug_printf(DBUG_LEVEL_TRACE, module, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#define dbug_assert(module, cond, fmt, ...)     \
    do                                          \
    {                                           \
        if (!(cond))                            \
        {                                       \
            dbug_printf(                        \
                DBUG_LEVEL_ERROR,               \
                module,                         \
                __FILE__,                       \
                __FUNCTION__,                   \
                __LINE__,                       \
                "ASSERTION FAILED: " fmt,       \
                ##__VA_ARGS__);                 \
        }                                       \
    } while (0)

#endif //TINY_NET_DBUG_H
