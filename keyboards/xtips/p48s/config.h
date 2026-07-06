// Copyright 2024 X.Tips (@X-Tips)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define DISABLE_JTAG

/* RGB Matrix (2x WS2812) */
#define RGB_MATRIX_SLEEP
#define RGB_MATRIX_DEFAULT_HUE 255
#define RGB_MATRIX_DEFAULT_SPD 26
#define ENABLE_RGB_MATRIX_BREATHING
#define ENABLE_RGB_MATRIX_CYCLE_ALL
#define ENABLE_RGB_MATRIX_CYCLE_PINWHEEL
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_CYCLE_PINWHEEL

/* TT() layer toggle after 2 taps */
#define TAPPING_TOGGLE 2

#define USB_POLLING_INTERVAL_MS 1
