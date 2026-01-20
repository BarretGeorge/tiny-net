#include "timer.h"
#include "sys.h"

void timer_work_proc(net_timer_t* timer, void* arg)
{
    plat_printf("timer %s exec...\n", timer->name);
}

void timer_test()
{
    net_timer_init();

    static net_timer_t t0, t1, t2, t3, t4;

    net_timer_add(&t0, "t0", timer_work_proc, NULL, 200, 0);
    net_timer_add(&t1, "t1", timer_work_proc, NULL, 1000, TIMER_FLAG_PERIODIC);
    net_timer_add(&t2, "t2", timer_work_proc, NULL, 1000, TIMER_FLAG_PERIODIC);
    net_timer_add(&t3, "t3", timer_work_proc, NULL, 4000, TIMER_FLAG_PERIODIC);
    net_timer_add(&t4, "t4", timer_work_proc, NULL, 3000, TIMER_FLAG_PERIODIC);
}

int main()
{
    timer_test();
}
