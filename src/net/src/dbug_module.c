#include "dbug_module.h"
#include "sys_plat.h"

// 模块开关状态数组（默认全部启用）
static int module_enabled[DBG_MOD_COUNT] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

// 模块名称表
static const char* module_names[DBG_MOD_COUNT] = {
    "PKTBUF",    /* DBG_MOD_PKTBUF = 0 */
    "NETIF",     /* DBG_MOD_NETIF = 1 */
    "EXMSG",     /* DBG_MOD_EXMSG = 2 */
    "TIMER",     /* DBG_MOD_TIMER = 3 */
    "ETHER",     /* DBG_MOD_ETHER = 4 */
    "LOOP",      /* DBG_MOD_LOOP = 5 */
    "ARP",       /* DBG_MOD_ARP = 6 */
    "IPV4",      /* DBG_MOD_IPV4 = 7 */
    "ICMP",      /* DBG_MOD_ICMP = 8 */
    "RAW",       /* DBG_MOD_RAW = 9 */
    "TCP",       /* DBG_MOD_TCP = 10 */
    "UDP",       /* DBG_MOD_UDP = 11 */
    "SOCKET",    /* DBG_MOD_SOCKET = 12 */
    "SOCK",      /* DBG_MOD_SOCK = 13 */
    "PLATFORM",  /* DBG_MOD_PLATFORM = 14 */
    "APP",       /* DBG_MOD_APP = 15 */
    "COMMON"     /* DBG_MOD_COMMON = 16 */
};

const char* dbug_module_name(dbug_module_t module)
{
    if (module >= 0 && module < DBG_MOD_COUNT)
    {
        return module_names[module];
    }
    return "UNKNOWN";
}

void dbug_module_enable(dbug_module_t module)
{
    if (module >= 0 && module < DBG_MOD_COUNT)
    {
        module_enabled[module] = 1;
    }
}

void dbug_module_disable(dbug_module_t module)
{
    if (module >= 0 && module < DBG_MOD_COUNT)
    {
        module_enabled[module] = 0;
    }
}

int dbug_module_is_enabled(dbug_module_t module)
{
    if (module >= 0 && module < DBG_MOD_COUNT)
    {
        return module_enabled[module];
    }
    return 0;
}

void dbug_module_enable_all(void)
{
    for (int i = 0; i < DBG_MOD_COUNT; i++)
    {
        module_enabled[i] = 1;
    }
}

void dbug_module_disable_all(void)
{
    for (int i = 0; i < DBG_MOD_COUNT; i++)
    {
        module_enabled[i] = 0;
    }
}

void dbug_module_enable_only(const dbug_module_t module)
{
    dbug_module_disable_all();
    dbug_module_enable(module);
}

void dbug_module_set_multiple(const dbug_module_t* modules, const int count, const int enable)
{
    for (int i = 0; i < count; i++)
    {
        if (modules[i] >= 0 && modules[i] < DBG_MOD_COUNT)
        {
            module_enabled[modules[i]] = enable;
        }
    }
}

void dbug_module_show_status(void)
{
    plat_printf("\n=== Debug Module Status ===\n");
    plat_printf("%-15s %s\n", "Module", "Status");
    plat_printf("%-15s %s\n", "---------------", "------");

    for (int i = 0; i < DBG_MOD_COUNT; i++)
    {
        plat_printf("%-15s %s\n",
                   module_names[i],
                   module_enabled[i] ? "ENABLED" : "DISABLED");
    }
    plat_printf("===========================\n\n");
}
