/*
 * Copyright (c) 2022 Jan Privara
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ir_send.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

#include "nec_prot.h"

#define IR_LED_NODE	DT_ALIAS(ir_led)
#if !DT_NODE_HAS_STATUS(IR_LED_NODE, okay)
#error "Unsupported board: ir-led devicetree alias is not defined"
#endif

LOG_MODULE_DECLARE(app, CONFIG_APP_LOG_LEVEL);

static const struct pwm_dt_spec irled = PWM_DT_SPEC_GET(IR_LED_NODE);

const uint32_t period_38khz = PWM_NSEC(26313U);
const uint32_t pulse_38khz  = PWM_NSEC(26313U) / 3U;

//------------------------------------------------------------------------------
void ir_send_init()
{
	if (!device_is_ready(irled.dev)) {
		LOG_ERR("ir_led device %s is not ready\n",
		        irled.dev->name);
		return;
	}

	int ret = pwm_set_dt(&irled, period_38khz, 0);
	if (ret) {
		LOG_ERR("Error %d: failed to set pulse width\n", ret);
		return;
	}
}

//------------------------------------------------------------------------------
void ir_send_buf(ir_raw_bit_buf_t * buf)
{
	int ret;
	uint32_t prev;
	uint32_t now;
	uint32_t usec_elapsed;

	prev = k_cycle_get_32();

	for (int i = 0; i < buf->length; i+=2) {
		ret = pwm_set_dt(&irled, period_38khz, pulse_38khz);
		if (ret) {
			LOG_ERR("Error %d: failed to set pulse width\n", ret);
			return;
		}

		do {
			now = k_cycle_get_32();
			usec_elapsed = (now - prev) / (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 1000000);
		} while (usec_elapsed < (buf->buf[i] + CONFIG_ADJUST_TRANSMIT_PULSE));
		prev = now;

		ret = pwm_set_dt(&irled, period_38khz, 0);
		if (ret) {
			LOG_ERR("Error %d: failed to set pulse width\n", ret);
			return;
		}

		if (i + 1 < buf->length) {
			do {
				now = k_cycle_get_32();
				usec_elapsed = (now - prev) / (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 1000000);
			} while (usec_elapsed < (buf->buf[i+1] + CONFIG_ADJUST_TRANSMIT_SPACE));
			prev = now;
		}
	}
}

//------------------------------------------------------------------------------
void ir_send_nec(uint8_t address, uint8_t command, ir_raw_bit_buf_t * buf)
{
	nec_waveform_t nec_wave = {
		.samples = buf->buf
	};

	nec_encode(address, command, &nec_wave);

	buf->length = nec_wave.length;

	ir_send_buf(buf);
}
