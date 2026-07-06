// Copyright 2024 X.Tips (@X-Tips)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_Q,           KC_W,           KC_E,           KC_R,          KC_T,     KC_VOLD, KC_VOLU,   KC_Y,   KC_U,  KC_I,   KC_O,   KC_P,
        KC_A,           KC_S,           KC_D,           KC_F,          KC_G,     KC_MPLY, KC_MNXT,   KC_H,   KC_J,  KC_K,   KC_L,   KC_ENT,
        KC_Z,           KC_X,           KC_C,           KC_V,          KC_B,     KC_MUTE, KC_MPRV,   KC_N,   KC_M,  KC_COMM, KC_DOT, KC_BSPC,
        LGUI_T(KC_DEL), LALT_T(KC_ESC), LCTL_T(KC_TAB), LT(1, KC_SPC), KC_LSFT,  MO(1),   MO(1),     KC_SPC, MO(1), KC_TAB, KC_ESC, KC_DEL
    ),

    [1] = LAYOUT(
        KC_1,    KC_2,    KC_3,    KC_4,    KC_5,     RGB_TOG, RGB_MOD,   KC_6,    KC_7,    KC_8,    KC_9,    KC_0,
        KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,    RGB_HUI, RGB_SAI,   KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT, KC_ENT,
        KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,   RGB_VAI, RGB_SPI,   KC_HOME, KC_END,  KC_PGUP, KC_PGDN, KC_BSPC,
        KC_LGUI, KC_LALT, KC_LCTL, _______, KC_LSFT,  QK_BOOT, QK_BOOT,   KC_SPC,  _______, KC_INS,  KC_APP,  KC_DEL
    )
};
