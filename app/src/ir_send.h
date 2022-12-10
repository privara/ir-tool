/*
 * Copyright (c) 2022 Jan Privara
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IR_SEND_H
#define IR_SEND_H

#include "ir_types.h"

/**
 * @brief Get IR transmitter timing adjustments
 *
 * @return Structure with timing adj. data
 */
ir_tim_adj_t ir_send_get_tim_adj();

/**
 * @brief Set IR transmitter timing adjustments
 *
 * @param Structure with timing adj. data
 */
void ir_send_set_tim_adj(ir_tim_adj_t tim);

/**
 * @brief Initialize IR send
 */
void ir_send_init();

/**
 * @brief Transmit IR samples buffer
 *
 * @param buf Pointer to the IR samples buffer.
 */

void ir_send_buf(ir_raw_bit_buf_t * buf);

/**
 * @brief Transmit IR NEC protocol command
 *
 * @param address NEC address.
 * @param command NEC command.
 * @param buf Pointer to the IR samples buffer.
 */
void ir_send_nec(uint8_t address, uint8_t command, ir_raw_bit_buf_t * buf);

#endif // IR_SEND_H
