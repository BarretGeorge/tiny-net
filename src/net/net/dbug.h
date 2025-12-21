#ifndef TINY_NET_DBUG_H
#define TINY_NET_DBUG_H

typedef enum debug_level_t
{
    DBUG_LEVEL_NONE = 0,
    DBUG_LEVEL_ERROR,
    DBUG_LEVEL_WARN,
    DBUG_LEVEL_INFO,
    DBUG_LEVEL_DEBUG,
    DBUG_LEVEL_TRACE,
} debug_level_t;

void dbug_printf(debug_level_t level, const char* file, const char* func, int line, const char* fmt, ...);

#define dbug_info(fmt, ...) dbug_printf(DBUG_LEVEL_INFO, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#define dbug_error(fmt, ...) dbug_printf(DBUG_LEVEL_ERROR, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#define dbug_warn(fmt, ...) dbug_printf(DBUG_LEVEL_WARN, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#define dbug_debug(fmt, ...) dbug_printf(DBUG_LEVEL_DEBUG, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#define dbug_trace(fmt, ...) dbug_printf(DBUG_LEVEL_TRACE, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#define dbug_assert(cond, fmt, ...)            \
    do                                   \
    {                                    \
        if (!(cond))                    \
        {                                \
            dbug_printf(                 \
                DBUG_LEVEL_ERROR,        \
                __FILE__,                \
                __FUNCTION__,            \
                __LINE__,                \
                "ASSERTION FAILED: " fmt, \
                ##__VA_ARGS__);          \
        }                                \
    } while (0)

#endif //TINY_NET_DBUG_H
