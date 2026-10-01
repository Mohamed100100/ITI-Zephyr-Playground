# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# The module is found via ZEPHYR_EXTRA_MODULES (see README build commands),
# NOT by copying it into the app folder.
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(d2v_demo)

target_sources(app PRIVATE src/main.c)
