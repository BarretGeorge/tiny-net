#include <stdio.h>
#include "ping/ping.h"
#include "args.h"
#include "common.h"
#include "dbug_module.h"

int main(const int argc, char** argv)
{
    dbug_module_disable_all();
    tiny_net_init();
    args_parser_t parser;
    args_parser_init(&parser, argc, argv);

    bool show_help;
    int count, interval, timeout, size;
    // const char* host = "192.168.100.120";
    const char* host = "8.8.8.8";

    args_register_bool(&parser, "help", 'h', &show_help, false, "Show this help message");
    args_register_int(&parser, "count", 'c', &count, 4, "Number of pings to send");
    args_register_int(&parser, "interval", 'i', &interval, 1000, "Interval between pings in milliseconds");
    args_register_int(&parser, "timeout", 't', &timeout, 1000, "Timeout for each ping in milliseconds");
    args_register_int(&parser, "size", 's', &size, 56, "Packet size in bytes");

    // 解析参数
    args_parser_parse(&parser);

    // 检查帮助选项
    if (show_help)
    {
        args_parser_print_help(&parser);
        printf("\nExample:\n");
        printf("  ping --count=10 --interval=500 192.168.1.1\n");
        args_parser_destroy(&parser);
        return 0;
    }

    // 第一个非选项参数作为目标主机
    for (int i = 1; i < parser.argc; i++)
    {
        if (parser.argv[i][0] != '-')
        {
            host = parser.argv[i];
            break;
        }
    }

    ping_t ping;
    ping_run(&ping, host, count, size, interval, timeout);
    args_parser_destroy(&parser);
}
