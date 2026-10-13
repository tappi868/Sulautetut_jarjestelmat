#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "app.h"

//tavoittelen 2/4 pisteitä

int main(void)
{
    if (init_led() < 0) {
        return 0;
    }
    if (init_button() < 0) {
        return 0;
    }
    if (init_uart() < 0) {
        return 0;
    }
    printk("Ready\n");
    return 0;
}