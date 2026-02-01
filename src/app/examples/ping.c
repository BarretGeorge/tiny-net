#include "ping/ping.h"
#include "args.h"
#include "common.h"
#include <stdio.h>

int main(const int argc, char** argv)
{
    tiny_net_init();
    args_parser_t parser;
    args_parser_init(&parser, argc, argv);

    bool show_help;
    int count = 0, interval = 0, timeout = 0, size = 0;
    const char* host = "192.168.100.104";

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

    // 第一个参数固定为目标主机
    if (parser.argc > 1)
    {
        host = parser.argv[parser.argc - 1];
    }

    ping_t ping;
    ping_run(&ping, host, count, size, interval, timeout);

    args_parser_destroy(&parser);
}
