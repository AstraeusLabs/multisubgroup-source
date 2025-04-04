# Copyright (c) 2025 Demant A/S
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(multisubgroup-source)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

set(lc3_dir src/lc3/)
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

file(GLOB lc3_files "${lc3_dir}/*.lc3")

foreach(lc3_file ${lc3_files})
    get_filename_component(lc3_filename ${lc3_file} NAME)
    set(output_file "${gen_dir}/${lc3_filename}.inc")

    generate_inc_file_for_target(app ${lc3_file} ${output_file})
endforeach()
