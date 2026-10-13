#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include "app.h"

struct button {
    struct gpio_dt_spec spec;
    struct gpio_callback cb;
    int id;
};

static struct button buttons[] = {
    { .spec = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios), .id = 0 },
    { .spec = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios), .id = 1 },
    { .spec = GPIO_DT_SPEC_GET(DT_ALIAS(sw2), gpios), .id = 2 },
    { .spec = GPIO_DT_SPEC_GET(DT_ALIAS(sw3), gpios), .id = 3 },
    { .spec = GPIO_DT_SPEC_GET(DT_ALIAS(sw4), gpios), .id = 4 },
};

static void button_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    struct button *b = CONTAINER_OF(cb, struct button, cb);

    printk("Button %d pressed\n", b->id + 1);

    switch (b->id) {
        led_state = (led_state == LED_OFF) ? LED_TRAFFIC : LED_OFF;
        break;
    case 1: led_state = LED_RED;          break;
    case 2: led_state = LED_YELLOW;       break;
    case 3: led_state = LED_GREEN;        break;
    case 4: led_state = LED_BLINK_YELLOW;         break;
    }
}

int init_button(void)
{
    for (int i = 0; i < ARRAY_SIZE(buttons); i++) {
        struct button *b = &buttons[i];

        if (!gpio_is_ready_dt(&b->spec)) {
            printk("Error: button %d is not ready\n", i + 1);
            return -1;
        }
        if (gpio_pin_configure_dt(&b->spec, GPIO_INPUT) != 0) {
            printk("Error: failed to configure button %d\n", i + 1);
            return -1;
        }
        if (gpio_pin_interrupt_configure_dt(&b->spec, GPIO_INT_EDGE_TO_ACTIVE) != 0) {
            printk("Error: failed to configure interrupt on button %d\n", i + 1);
            return -1;
        }
        gpio_init_callback(&b->cb, button_handler, BIT(b->spec.pin));
        gpio_add_callback(b->spec.port, &b->cb);
        printk("Set up button %d ok\n", i + 1);
    }
    printk("Button initialized ok\n");
    return 0;
}