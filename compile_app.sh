#!/usr/bin/env bash

west build -b nrf52840dongle -d build/nrf52840dongle/app app --pristine -- "-DOVERLAY_CONFIG=overlay-bt_ll_sw_split.conf"
