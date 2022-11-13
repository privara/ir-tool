/*
 * Copyright (c) 2022 Jan Privara
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ir_recv.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "nec_prot.h"

#define IR_SEN_NODE	DT_ALIAS(ir_sen)
#if !DT_NODE_HAS_STATUS(IR_SEN_NODE, okay)
#error "Unsupported board: ir-sen devicetree alias is not defined"
#endif

LOG_MODULE_DECLARE(app, CONFIG_APP_LOG_LEVEL);

static const struct gpio_dt_spec irsen = GPIO_DT_SPEC_GET(IR_SEN_NODE, gpios);
static struct gpio_callback irsen_cb_data;

struct k_timer tout_timer;
uint32_t recv_timeout_ms = 10;

ir_raw_bit_buf_t * ir_recv_buf;

int recv_enabled = 0;
int recv_started = 0;

int prev_val;
uint32_t prev_time;

typedef enum {
	ST_DISABLED = 0,
	ST_ENABLED,
	ST_RECEIVING,
	ST_FINISHED
} state_t;

state_t current_state = ST_DISABLED;

//------------------------------------------------------------------------------
void finished()
{
	recv_enabled = 0;
	current_state = ST_FINISHED;
}

void timeout_handler(struct k_timer *timer_id)
{
	uint32_t time = k_cycle_get_32();
	const uint32_t us_div = (sys_clock_hw_cycles_per_sec() / 1000000);

	// add the timeouted sample
	if (ir_recv_buf->length < CONFIG_IR_BUF_SIZE - 1) {
		ir_recv_buf->buf[ir_recv_buf->length++] = (time - prev_time) / us_div;
	}

	finished();
}

K_TIMER_DEFINE(tout_timer, timeout_handler, NULL);

//------------------------------------------------------------------------------
void irsen_change(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	if (!recv_enabled)
		return;

	// capture pin state
	uint32_t time = k_cycle_get_32();
	int val = gpio_pin_get_dt(&irsen);

	if (val != 0 && val != 1) {
		LOG_ERR("GPIO read error\n");
		return;
	}

	const uint32_t us_div = (sys_clock_hw_cycles_per_sec() / 1000000);

	// wait for initial gap -> mark transition
	if (!recv_started)
	{
		if (val == 1)
		{
			prev_val = val;
			prev_time = time;
			recv_started = 1;
			ir_recv_buf->length = 0;
			current_state = ST_RECEIVING;
			k_timer_start(&tout_timer, K_MSEC(recv_timeout_ms), K_NO_WAIT);
		}
		return;
	}

	// reset the timeout counter
	k_timer_start(&tout_timer, K_MSEC(recv_timeout_ms), K_NO_WAIT);

	if (val == prev_val) {
		LOG_WRN("Missed a transition of IR sensor\n");
		return;
	} else if (ir_recv_buf->length < CONFIG_IR_BUF_SIZE - 1) {
		uint32_t duration = (time - prev_time) / us_div;

		duration += prev_val == 1 ? CONFIG_ADJUST_RECEIVE_PULSE :
		                            CONFIG_ADJUST_RECEIVE_SPACE;

		ir_recv_buf->buf[ir_recv_buf->length++] = duration;
		prev_time = time;
		prev_val = val;
	}

	if (ir_recv_buf->length == CONFIG_IR_BUF_SIZE)
	{
		k_timer_stop(&tout_timer);
		finished();
	}
}

//------------------------------------------------------------------------------
void init()
{
	if (!device_is_ready(irsen.port)) {
		LOG_ERR("irsen device %s is not ready\n",
		        irsen.port->name);
		return;
	}

	int ret = gpio_pin_configure_dt(&irsen, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d\n",
		        ret, irsen.port->name, irsen.pin);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&irsen, GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, irsen.port->name, irsen.pin);
		return;
	}

	gpio_init_callback(&irsen_cb_data, irsen_change, BIT(irsen.pin));
	gpio_add_callback(irsen.port, &irsen_cb_data);
}

//------------------------------------------------------------------------------
void ir_recv_enable()
{
	if (current_state != ST_DISABLED && current_state != ST_FINISHED)
		return;

	recv_started = 0;
	recv_enabled = 1;
	current_state = ST_ENABLED;
}

void ir_recv_disable()
{
	recv_enabled = 0;
	k_timer_stop(&tout_timer);
	current_state = ST_DISABLED;
}

bool ir_recv_is_enabled()
{
	return current_state != ST_DISABLED;
}

void ir_recv_set_timeout(uint32_t timeout_ms)
{
	recv_timeout_ms = timeout_ms;
}

uint32_t ir_recv_get_timeout()
{
	return recv_timeout_ms;
}

void ir_recv_set_buf(ir_raw_bit_buf_t * buf)
{
	ir_recv_buf = buf;
}

void decode_ir_buf()
{
	// NEC
	nec_waveform_t nec_wave = {
		.length = ir_recv_buf->length,
		.samples = ir_recv_buf->buf
	};

	nec_decoded_data_t nec_result;

	int samples_used = nec_decode(&nec_wave, &nec_result);
	
	if (samples_used > 0) {
		LOG_INF("NEC address: 0x%04x", nec_result.address);
		LOG_INF("NEC command: 0x%04x", nec_result.command);
	} else { 
		LOG_INF("NEC decoding failed");
	}
}

//------------------------------------------------------------------------------
void ir_recv_thread_entry()
{
	init();

	ir_recv_enable();

	while (1) {
		if (current_state == ST_FINISHED) {
			LOG_INF("Receiving finished, %d samples", ir_recv_buf->length);
			
#ifdef CONFIG_PRINT_RECV_CMDS
			// log the received command
			for (int i = 0; i < ir_recv_buf->length; i+=2) {
				if (i + 1 < ir_recv_buf->length)
					LOG_INF("%03d: %d - %d", i, ir_recv_buf->buf[i],
					        ir_recv_buf->buf[i+1]);
				else
					LOG_INF("%03d: %d", i, ir_recv_buf->buf[i]);
				k_msleep(10);
			}
#endif // CONFIG_PRINT_RECV_CMDS

			decode_ir_buf();

			ir_recv_enable();
		}

		k_msleep(1);
	}
}
