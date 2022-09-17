/*
 * Copyright (c) 2022 Jan Privara
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IR_RECV_H
#define IR_RECV_H

#include <inttypes.h>
#include <stdbool.h>
#include "ir_buf.h"

#define IR_RECV_STACKSIZE 1024
#define IR_RECV_PRIORITY 7

void ir_recv_enable();
void ir_recv_disable();
bool ir_recv_is_enabled();

void ir_recv_set_timeout(uint32_t timeout_ms);
uint32_t ir_recv_get_timeout();

void ir_recv_set_buf(ir_raw_bit_buf_t * buf);

void ir_recv_thread_entry();

#endif // IR_RECV_H
