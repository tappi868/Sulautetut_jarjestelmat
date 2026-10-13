#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <stdbool.h>
#include "app.h"

#define STACKSIZE 1024
#define PRIORITY 5

volatile int led_state = LED_OFF;

static const struct gpio_dt_spec red   = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

static char uart_color;
static int uart_ms;
K_SEM_DEFINE(uart_done, 0, 1);

void led_play(char color, int ms)
{
    printk("LED play: %c %d\n", color, ms);

    
    uart_color = color;
    uart_ms = ms;
    k_sem_reset(&uart_done);
    led_state = LED_UART;
    k_sem_take(&uart_done, K_FOREVER);
}

int init_led(void)
{
    int ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        printk("Error: red led configure failed\n");
        return ret;
    }
    ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        printk("Error: green led configure failed\n");
        return ret;
    }
    printk("Led initialized ok\n");
    return 0;
}

static void set_leds(int r, int g)
{
    gpio_pin_set_dt(&red, r);
    gpio_pin_set_dt(&green, g);
}

static bool wait_in_state(int state, int ms)
{
    for (int t = 0; t < ms; t += 20) {
        if (led_state != state) {
            return false;
        }
        k_msleep(20);
    }
    return led_state == state;
}

static void led_task(void *a, void *b, void *c)
{
    printk("LED task started\n");
    while (true) {
        int state = led_state;

        switch (state) {
        case LED_OFF:
            set_leds(0, 0);
            wait_in_state(state, 50);
            break;

        case LED_TRAFFIC:
            set_leds(1, 0);                      
            if (!wait_in_state(state, 2000)) break;
            set_leds(1, 1);                       
            if (!wait_in_state(state, 1000)) break;
            set_leds(0, 1);                      
            wait_in_state(state, 2000);
            break;

        case LED_RED:
            set_leds(1, 0);
            wait_in_state(state, 50);
            break;

        case LED_YELLOW:
            set_leds(1, 1);
            wait_in_state(state, 50);
            break;

        case LED_GREEN:
            set_leds(0, 1);
            wait_in_state(state, 50);
            break;

        case LED_BLINK_YELLOW:
            set_leds(1, 1);
            if (!wait_in_state(state, 500)) break;
            set_leds(0, 0);
            wait_in_state(state, 500);
            break;
        case LED_UART:
            printk("HERE_case_uart %d \n" , uart_color);
            set_leds(uart_color != 'G', uart_color != 'R');   /* R: 1,0  Y: 1,1  G: 0,1 */
            wait_in_state(state, uart_ms);
            set_leds(0, 0);
            if (led_state == LED_UART) {
                led_state = LED_OFF;
            }
            k_sem_give(&uart_done);
            break;

        default:
            led_state = LED_OFF;
            break;
        }
    }
}

K_THREAD_DEFINE(led_thread, STACKSIZE, led_task, NULL, NULL, NULL, PRIORITY, 0, 100);