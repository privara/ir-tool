#-------------------------------------------------------------------------------
# Zephyr IR Tool
#
# Copyright (c) 2022 Jan Privara
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ir_tool)

target_include_directories(app PRIVATE nec_protocol/lib)
target_sources(app PRIVATE 
    nec_protocol/lib/nec_prot.c
    src/ir_recv.c
    src/ir_send.c
    src/main.c
)
