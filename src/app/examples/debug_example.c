/**
 * 模块化日志系统使用示例
 *
 * 演示如何使用模块化日志功能
 */

#include "common.h"
#include "dbug.h"
#include "dbug_module.h"
#include "sys_plat.h"

/**
 * 示例1: 基本使用
 */
void example_basic_usage(void)
{
    plat_printf("\n=== 示例1: 基本使用 ===\n\n");

    // 使用不同模块的日志
    dbug_info(DBG_MOD_ARP, "ARP module message");
    dbug_info(DBG_MOD_IPV4, "IPv4 module message");
    dbug_info(DBG_MOD_ICMP, "ICMP module message");

    // 使用不同级别的日志
    dbug_error(DBG_MOD_ARP, "This is an ERROR message");
    dbug_warn(DBG_MOD_ARP, "This is a WARN message");
    dbug_info(DBG_MOD_ARP, "This is an INFO message");
    dbug_debug(DBG_MOD_ARP, "This is a DEBUG message");
    dbug_trace(DBG_MOD_ARP, "This is a TRACE message");
}

/**
 * 示例2: 控制模块开关
 */
void example_module_control(void)
{
    plat_printf("\n=== 示例2: 控制模块开关 ===\n\n");

    plat_printf("--- 启用所有模块 ---\n");
    dbug_module_enable_all();
    dbug_info(DBG_MOD_ARP, "ARP is enabled");
    dbug_info(DBG_MOD_IPV4, "IPv4 is enabled");

    plat_printf("\n--- 禁用所有模块 ---\n");
    dbug_module_disable_all();
    dbug_info(DBG_MOD_ARP, "You won't see this (ARP disabled)");
    dbug_info(DBG_MOD_IPV4, "You won't see this (IPv4 disabled)");

    plat_printf("\n--- 只启用 ARP 模块 ---\n");
    dbug_module_enable_only(DBG_MOD_ARP);
    dbug_info(DBG_MOD_ARP, "ARP is enabled");
    dbug_info(DBG_MOD_IPV4, "You won't see this (IPv4 disabled)");
}

/**
 * 示例3: 批量设置模块
 */
void example_batch_setting(void)
{
    plat_printf("\n=== 示例3: 批量设置模块 ===\n\n");

    // 定义网络层模块
    dbug_module_t network_modules[] = {
        DBG_MOD_ARP,
        DBG_MOD_IPV4,
        DBG_MOD_ICMP
    };

    plat_printf("--- 禁用所有，只启用网络层模块 ---\n");
    dbug_module_disable_all();
    dbug_module_set_multiple(network_modules, 3, 1);  // 1 = 启用

    dbug_info(DBG_MOD_ARP, "ARP is enabled");
    dbug_info(DBG_MOD_IPV4, "IPv4 is enabled");
    dbug_info(DBG_MOD_ICMP, "ICMP is enabled");
    dbug_info(DBG_MOD_ETHER, "You won't see this (Ether disabled)");
}

/**
 * 示例4: 查看模块状态
 */
void example_show_status(void)
{
    plat_printf("\n=== 示例4: 查看模块状态 ===\n\n");

    // 设置一些模块
    dbug_module_disable_all();
    dbug_module_enable(DBG_MOD_ARP);
    dbug_module_enable(DBG_MOD_IPV4);
    dbug_module_enable(DBG_MOD_ICMP);

    // 显示状态
    dbug_module_show_status();
}

/**
 * 示例5: 动态控制日志输出
 */
void example_dynamic_control(void)
{
    plat_printf("\n=== 示例5: 动态控制 ===\n\n");

    plat_printf("--- 运行时启用模块 ---\n");
    dbug_module_enable(DBG_MOD_ARP);
    dbug_info(DBG_MOD_ARP, "ARP is now enabled");

    plat_printf("\n--- 运行时禁用模块 ---\n");
    dbug_module_disable(DBG_MOD_ARP);
    dbug_info(DBG_MOD_ARP, "You won't see this");

    plat_printf("\n--- 重新启用 ---\n");
    dbug_module_enable(DBG_MOD_ARP);
    dbug_info(DBG_MOD_ARP, "ARP is enabled again");
}

/**
 * 示例6: 实际应用场景 - 调试 Ping
 */
void example_debug_ping(void)
{
    plat_printf("\n=== 示例6: 调试 Ping 场景 ===\n\n");

    plat_printf("配置: 只看 Ping 相关模块的日志\n\n");

    dbug_module_disable_all();

    // 启用 Ping 相关模块
    dbug_module_enable(DBG_MOD_APP);
    dbug_module_enable(DBG_MOD_ICMP);
    dbug_module_enable(DBG_MOD_IPV4);
    dbug_module_enable(DBG_MOD_ARP);
    dbug_module_enable(DBG_MOD_ETHER);

    // 模拟 Ping 过程的日志
    dbug_info(DBG_MOD_APP, "Starting ping to 8.8.8.8");
    dbug_info(DBG_MOD_ICMP, "Sending ICMP echo request, seq=0");
    dbug_debug(DBG_MOD_IPV4, "Building IPv4 header, dst=8.8.8.8");
    dbug_debug(DBG_MOD_ARP, "Looking up MAC for 192.168.1.1");
    dbug_trace(DBG_MOD_ETHER, "Sending Ethernet frame, size=98");

    // 这些日志不会显示（模块未启用）
    dbug_info(DBG_MOD_PKTBUF, "You won't see this");
    dbug_info(DBG_MOD_TIMER, "You won't see this");
}

/**
 * 示例7: 检查模块状态
 */
void example_check_status(void)
{
    plat_printf("\n=== 示例7: 检查模块状态 ===\n\n");

    dbug_module_enable(DBG_MOD_ARP);
    dbug_module_disable(DBG_MOD_IPV4);

    if (dbug_module_is_enabled(DBG_MOD_ARP))
    {
        plat_printf("ARP module is ENABLED\n");
    }

    if (!dbug_module_is_enabled(DBG_MOD_IPV4))
    {
        plat_printf("IPv4 module is DISABLED\n");
    }
}

int main(void)
{
    plat_printf("\n");
    plat_printf("╔════════════════════════════════════════════╗\n");
    plat_printf("║   模块化日志系统 - 使用示例                ║\n");
    plat_printf("╚════════════════════════════════════════════╝\n");

    // 运行所有示例
    example_basic_usage();
    example_module_control();
    example_batch_setting();
    example_show_status();
    example_dynamic_control();
    example_debug_ping();
    example_check_status();

    plat_printf("\n");
    plat_printf("╔════════════════════════════════════════════╗\n");
    plat_printf("║   示例运行完成                             ║\n");
    plat_printf("╚════════════════════════════════════════════╝\n");
    plat_printf("\n");

    return 0;
}
