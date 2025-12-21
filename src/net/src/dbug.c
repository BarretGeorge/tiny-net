#include "dbug.h"
#include "sys_plat.h"
#include <stdarg.h>

#define DBUG_STYLE_ERROR "\033[31m"
#define DBUG_STYLE_WARN "\033[33m"
#define DBUG_STYLE_INFO "\033[32m"
#define DBUG_STYLE_DEBUG "\033[34m"
#define DBUG_STYLE_TRACE "\033[35m"
#define DBUG_STYLE_RESET "\033[0m"
#define DBUG_STYLE_NONE ""

static const char* dbug_file_name(const char* path)
{
    const char* file = path;
    for (const char* p = path; *p != '\0'; ++p)
    {
        if (*p == '/' || *p == '\\')
        {
            file = p + 1;
        }
    }
    return file;
}

static const char* dbug_level_to_string(const debug_level_t level)
{
    switch (level)
    {
    case DBUG_LEVEL_NONE:
        return DBUG_STYLE_NONE"[NONE]";
    case DBUG_LEVEL_ERROR:
        return DBUG_STYLE_ERROR"[ERROR]";
    case DBUG_LEVEL_WARN:
        return DBUG_STYLE_WARN"[WARN]";
    case DBUG_LEVEL_INFO:
        return DBUG_STYLE_INFO"[INFO]";
    case DBUG_LEVEL_DEBUG:
        return DBUG_STYLE_DEBUG"[DEBUG]";
    case DBUG_LEVEL_TRACE:
        return DBUG_STYLE_TRACE"[TRACE]";
    default:
        return "UNKNOWN";
    }
}

void dbug_printf(const debug_level_t level, const char* file, const char* func, const int line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    plat_printf("%s[%s:%s:%d] ", dbug_level_to_string(level), dbug_file_name(file), func, line);
    char buf[1024];
    plat_vsprintf(buf, fmt, args);
    // vsnprintf(buf, sizeof(buf), fmt, args);
    plat_printf("%s\n"DBUG_STYLE_RESET, buf);
    va_end(args);
}
