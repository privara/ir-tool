/*
 * Copyright (c) 2022 Jan Privara
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IR_RECV_H
#define IR_RECV_H

#include <inttypes.h>
#include <stdbool.h>
#include "ir_types.h"

#define IR_RECV_STACKSIZE 1024
#define IR_RECV_PRIORITY 7

/**
 * @brief Get IR receiver timing adjustments
 *
 * @return Structure with timing adj. data
 */
ir_tim_adj_t ir_recv_get_tim_adj();

/**
 * @brief Set IR receiver timing adjustments
 *
 * @param Structure with timing adj. data
 */
void ir_recv_set_tim_adj(ir_tim_adj_t tim);

/**
 * @brief Enable IR receiving
 *
 * Enable IR receiving.
 * If it is already enabled, this function has no effect, i.e.
 * the possible ongoing receiving process is not affected (reset etc.).
 */
void ir_recv_enable();

/**
 * @brief Disable IR receiving
 *
 * Disable IR receiving.
 * If there is a receiving in progress, it will be cancelled.
 */
void ir_recv_disable();

/**
 * @brief Get IR receiving state
 *
 * @return true if receiving is enabled, false otherwise
 */
bool ir_recv_is_enabled();

/**
 * @brief Set receiving timeout
 *
 * @param timeout_ms Receiving timeout in miliseconds. 
 */
void ir_recv_set_timeout(uint32_t timeout_ms);

/**
 * @brief Get receiving timeout
 *
 * @return Receiving timeout in miliseconds. 
 */
uint32_t ir_recv_get_timeout();

/**
 * @brief Set receive IR samples buffer
 *
 * @param buf Pointer to the IR samples buffer. 
 */
void ir_recv_set_buf(ir_raw_bit_buf_t * buf);

/**
 * @brief Receive thread entry function
 *
 * Entry function for the IR receive thread.
 */
void ir_recv_thread_entry();

#endif // IR_RECV_H
