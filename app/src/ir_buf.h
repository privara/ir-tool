/*
 * Copyright (c) 2022 Jan Privara
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IR_BUF_H
#define IR_BUF_H

#include <inttypes.h>

/**
 * Infra-red samples buffer
 */
typedef struct
{
	int length;
	uint32_t * buf;
} ir_raw_bit_buf_t;

#endif // IR_BUF_H
