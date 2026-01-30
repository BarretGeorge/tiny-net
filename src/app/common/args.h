#ifndef TINY_NET_ARGS_H
#define TINY_NET_ARGS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct args_option_t args_option_t;

typedef enum
{
    ARGS_TYPE_INT,
    ARGS_TYPE_BOOL,
    ARGS_TYPE_STRING,
} args_type_t;

typedef struct
{
    int argc;
    char** argv;
    int index;
    args_option_t* options;
    int option_count;
    int option_capacity;
} args_parser_t;

// 初始化解析器
void args_parser_init(args_parser_t* parser, int argc, char** argv);

// 销毁解析器（释放内部资源）
void args_parser_destroy(args_parser_t* parser);

// 注册整数类型选项
void args_register_int(args_parser_t* parser, const char* long_opt, char short_opt, int* dest, int default_value, const char* help);

// 注册布尔类型选项
void args_register_bool(args_parser_t* parser, const char* long_opt, char short_opt, bool* dest, bool default_value, const char* help);

// 注册字符串类型选项
void args_register_string(args_parser_t* parser, const char* long_opt, char short_opt, const char** dest, const char* default_value, const char* help);

// 解析所有参数
bool args_parser_parse(args_parser_t* parser);

// 检查是否存在指定选项（支持 --help 或 -h 格式）
bool args_has_option(args_parser_t* parser, const char* long_opt, char short_opt);

// 打印帮助信息
void args_parser_print_help(args_parser_t* parser);


#endif //TINY_NET_ARGS_H
