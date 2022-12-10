/*
 * Copyright (c) 2022 Jan Privara
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>

#include <stdlib.h>
#include <stdbool.h>

#include "ir_types.h"
#include "ir_recv.h"
#include "ir_send.h"

LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);

K_THREAD_STACK_DEFINE(ir_recv_thread_stack_area, IR_RECV_STACKSIZE);
static struct k_thread ir_recv_thread_data;

uint32_t samples_buf[CONFIG_IR_BUF_SIZE];
ir_raw_bit_buf_t ir_buf = {
	.buf = samples_buf,
	.length = 0
};

//------------------------------------------------------------------------------
static int cmd_send_nec(const struct shell *shell, size_t argc, char *argv[])
{
	long addr = strtol(argv[1], NULL, 0);
	long cmd = strtol(argv[2], NULL, 0);

	uint8_t nec_addr = addr;
	uint8_t nec_cmd = cmd;

	if (addr < 0 || addr > 255 || cmd < 0 || cmd > 255) {
		shell_fprintf(shell, SHELL_ERROR, "Invalid address or command!\n"
			"Allowed range for both is 0-255\n");
		return -1;
	}

	shell_fprintf(shell, SHELL_NORMAL,
		"Sending NEC\n  - Address: 0x%02X (%d)\n  - Command: 0x%02X (%d)\n",
		nec_addr, nec_addr, nec_cmd, nec_cmd);

	bool en_recv = false;

	if (ir_recv_is_enabled()) {
		ir_recv_disable();
		en_recv = true;
	}

	ir_send_nec(nec_addr, nec_cmd, &ir_buf);
	ir_buf.length = 0;

	if (en_recv)
		ir_recv_enable();

    return 0;
}

static int cmd_send_last(const struct shell *shell, size_t argc, char *argv[])
{
	bool en_recv = false;

	if (ir_recv_is_enabled()) {
		ir_recv_disable();
		en_recv = true;
	}

	if (ir_buf.length > 0) {
		shell_fprintf(shell, SHELL_NORMAL,
			"Sending last received command\n");

		ir_send_buf(&ir_buf);
	} else {
		shell_fprintf(shell, SHELL_NORMAL,
			"No received command available\n");
	}

	if (en_recv)
		ir_recv_enable();

    return 0;
}

static int cmd_set_timeout(const struct shell *shell, size_t argc, char *argv[])
{
	long val = strtol(argv[1], NULL, 10);
 
    if (val < 0) {
        shell_fprintf(shell, SHELL_ERROR, 
			"Invalid value (negative): %s\n", argv[1]);
        return -1;
    }
    else {
		ir_recv_set_timeout(val);
        shell_fprintf(shell, SHELL_NORMAL, 
			"IR receive timeout set to: %d ms\n", ir_recv_get_timeout());
    }

    return 0;
}

static int cmd_get_timeout(const struct shell *shell, size_t argc,
             char *argv[])
{
    shell_fprintf(shell, SHELL_NORMAL,
		"IR receive timeout: %d ms\n", ir_recv_get_timeout());
    return 0;
}

static int cmd_recv_enable(const struct shell *shell, size_t argc, char *argv[])
{
	ir_recv_enable();
    shell_fprintf(shell, SHELL_NORMAL, "IR receiving enabled\n");
    return 0;
}

static int cmd_recv_disable(const struct shell *shell, size_t argc, char *argv[])
{
	ir_recv_disable();
    shell_fprintf(shell, SHELL_NORMAL, "IR receiving disabled\n");
    return 0;
}

static int cmd_adj_get(const struct shell *shell, size_t argc, char *argv[])
{
	ir_tim_adj_t send_tim = ir_send_get_tim_adj();
	ir_tim_adj_t recv_tim = ir_recv_get_tim_adj();

	shell_fprintf(shell, SHELL_NORMAL, "IR timing adjustment:\n");
	shell_fprintf(shell, SHELL_NORMAL, "  -Transmit:\n");

	shell_fprintf(shell, SHELL_NORMAL, "    -Pulse: %d usec\n", send_tim.pulse);
	shell_fprintf(shell, SHELL_NORMAL, "    -Space: %d usec\n", send_tim.space);

	shell_fprintf(shell, SHELL_NORMAL, "  -Receive:\n");
	shell_fprintf(shell, SHELL_NORMAL, "    -Pulse: %d usec\n", recv_tim.pulse);
	shell_fprintf(shell, SHELL_NORMAL, "    -Space: %d usec\n", recv_tim.space);

    return 0;
}

static int cmd_adj_send(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "IR timing send adjustment:\n");

	ir_tim_adj_t send_tim;

	send_tim.pulse = strtol(argv[1], NULL, 0);
	send_tim.space = strtol(argv[2], NULL, 0);

	shell_fprintf(shell, SHELL_NORMAL, "    -Pulse: %d usec\n", send_tim.pulse);
	shell_fprintf(shell, SHELL_NORMAL, "    -Space: %d usec\n", send_tim.space);

	ir_send_set_tim_adj(send_tim);

    return 0;
}

static int cmd_adj_recv(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "IR timing recv adjustment:\n");

	ir_tim_adj_t recv_tim;

	recv_tim.pulse = strtol(argv[1], NULL, 0);
	recv_tim.space = strtol(argv[2], NULL, 0);

	shell_fprintf(shell, SHELL_NORMAL, "    -Pulse: %d usec\n", recv_tim.pulse);
	shell_fprintf(shell, SHELL_NORMAL, "    -Space: %d usec\n", recv_tim.space);

	ir_recv_set_tim_adj(recv_tim);

    return 0;
}

//------------------------------------------------------------------------------
SHELL_STATIC_SUBCMD_SET_CREATE(
	ir_adj_cmds,
	SHELL_CMD_ARG(list, NULL,
		"list IR timing adjustment\n",
		cmd_adj_get, 1, 0),
	SHELL_CMD_ARG(send, NULL,
		"adjust transmit IR timing\n"
		"args: <pulse> <space>\n",
		cmd_adj_send, 3, 0),
	SHELL_CMD_ARG(recv, NULL,
		"adjust receive IR timing\n"
		"args: <pulse> <space>\n",
		cmd_adj_recv, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(ir_adj, &ir_adj_cmds, "IR timing adjustment", NULL);

SHELL_STATIC_SUBCMD_SET_CREATE(
	ir_send_cmds,
	SHELL_CMD_ARG(nec, NULL,
		"transmit NEC command\n"
		"args: <address> <command>\n",
		cmd_send_nec, 3, 0),
	SHELL_CMD_ARG(last_recv, NULL,
		"transmit last received command\n",
		cmd_send_last, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(ir_send, &ir_send_cmds, "Transmit IR command", NULL);

SHELL_STATIC_SUBCMD_SET_CREATE(
	ir_recv_cmds,
	SHELL_CMD_ARG(enable, NULL,
		"enable receiving\n",
		cmd_recv_enable, 1, 0),
	SHELL_CMD_ARG(disable, NULL,
		"disable receiving\n",
		cmd_recv_disable, 1, 0),
	SHELL_CMD_ARG(set_timeout, NULL,
		"set timeout\n"
		"args: <miliseconds>\n",
		cmd_set_timeout, 2, 0),
	SHELL_CMD_ARG(get_timeout, NULL,
		"get timeout\n",
		cmd_get_timeout, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(ir_recv, &ir_recv_cmds, "Receive IR command", NULL);

//------------------------------------------------------------------------------
void main(void)
{
#if DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_shell_uart), zephyr_cdc_acm_uart)
	const struct device *dev;
	uint32_t dtr = 0;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
	if (!device_is_ready(dev) || usb_enable(NULL)) {
		return;
	}

	while (!dtr) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		k_sleep(K_MSEC(100));
	}
#endif

	ir_recv_set_buf(&ir_buf);
	ir_send_init();

	k_thread_create(&ir_recv_thread_data, ir_recv_thread_stack_area,
			K_THREAD_STACK_SIZEOF(ir_recv_thread_stack_area),
			ir_recv_thread_entry, NULL, NULL, NULL,
			IR_RECV_PRIORITY, 0, K_FOREVER);
	k_thread_name_set(&ir_recv_thread_data, "ir_recv_thread");

	k_thread_start(&ir_recv_thread_data);
}
