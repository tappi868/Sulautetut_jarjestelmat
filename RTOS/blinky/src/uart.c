#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "app.h"

#define STACKSIZE 1024
#define PRIORITY 5

#define MSG_LEN    64   
#define IDLE_MS    1000   
#define DEFAULT_MS 1000   



static const struct device *const uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

// FIFO dispatcher data type
struct data_t {
	/*************************
	// Add fifo_reserved below
	*************************/
	void *fifo_reserved;
	char msg[20];
};

K_FIFO_DEFINE(dispatcher_fifo);

K_MSGQ_DEFINE(rx_q, 1, 128, 1);

static void uart_rx_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			while (uart_fifo_read(dev, &c, 1) == 1) {
				k_msgq_put(&rx_q, &c, K_NO_WAIT);
			}
		}
	}
}

int init_uart(void)
{
	if (!device_is_ready(uart_dev)) {
		printk("UART initialization failed\n");
		return 1;
	}
	uart_irq_callback_user_data_set(uart_dev, uart_rx_cb, NULL);
	uart_irq_rx_enable(uart_dev);
	printk("UART initialized ok\n");
	return 0;
}
static void uart_task(void *unused1, void *unused2, void *unused3)
{
	uint8_t rc;
	char uart_msg[MSG_LEN] = {0};
	int uart_msg_cnt = 0;

	while (true) {
		bool end_of_msg = false;

		k_timeout_t wait = (uart_msg_cnt == 0) ? K_FOREVER : K_MSEC(IDLE_MS);

		if (k_msgq_get(&rx_q, &rc, wait) == 0) {
			if (rc == '\r' || rc == '\n') {
				end_of_msg = true;
			} else if (uart_msg_cnt < MSG_LEN - 1) {
				uart_msg[uart_msg_cnt++] = rc;
			}
		} else {
			end_of_msg = true;
		}

		if (end_of_msg && uart_msg_cnt > 0) {
			printk("UART msg: %s\n", uart_msg);

			struct data_t *buf = k_malloc(sizeof(struct data_t));
			if (buf == NULL) {
				printk("Error: out of memory\n");
			} else {
				strncpy(buf->msg, uart_msg, sizeof(buf->msg) - 1);
				buf->msg[sizeof(buf->msg) - 1] = '\0';
				k_fifo_put(&dispatcher_fifo, buf);
			}
			uart_msg_cnt = 0;
			memset(uart_msg, 0, sizeof(uart_msg));
		}
	}
}
static void dispatcher_task(void *unused1, void *unused2, void *unused3)
{
	while (true) {
		struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
		char sequence[MSG_LEN];
		memcpy(sequence, rec_item->msg, MSG_LEN);
		k_free(rec_item);

		printk("Dispatcher: %s\n", sequence);

		char *p = sequence;
		while (*p != '\0') {
			if (isspace((unsigned char)*p)) {
				p++;
				continue;
			}

			char color = toupper((unsigned char)*p);
			p++;

			if (color != 'R' && color != 'Y' && color != 'G') {
				printk("Skipping unknown color '%c'\n", color);
				continue;
			}

			while (isspace((unsigned char)*p)) {
				p++;
			}

			int time = DEFAULT_MS;
			if (isdigit((unsigned char)*p)) {
				char *end;
				time = (int)strtol(p, &end, 10);
				p = end;
			}
			if (time <= 0) {
				continue;
			}

			printk("Data: %c %d\n", color, time);

			led_play(color, time);
		}
	}
}

K_THREAD_DEFINE(dis_thread,STACKSIZE,dispatcher_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(uart_thread,STACKSIZE,uart_task,NULL,NULL,NULL,PRIORITY,0,0);