/* Copyright 2025 Carlos Eduardo de Paula <carlosedp@gmail.com>
 * Copyright 2025 EPOMAKER <https://github.com/Epomaker>
 * Copyright 2023 LiWenLiu <https://github.com/LiuLiuQMK>
 * Copyright 2021 QMK <https://github.com/qmk/qmk_firmware>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include QMK_KEYBOARD_H
#include "rdmctmzt_common.h"

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT_tkl_ansi(
        KC_TAB,  KC_Q,          KC_W,   KC_E,          KC_R,     KC_T,     KC_Y,    KC_U,    KC_I,    KC_O,     KC_P,     KC_BSPC,
        KC_CAPS, KC_A,          KC_S,   KC_D,          KC_F,     KC_G,     KC_H,    KC_J,    KC_K,    KC_L,     KC_SCLN,  KC_ENT,
        KC_LSFT, KC_Z,          KC_X,   KC_C,          KC_V,     KC_B,     KC_N,    KC_M,    KC_COMM, KC_DOT,   KC_UP ,   KC_QUOT,
        MO(2),   KC_LCTL,       KC_GRV, KC_LALT,       KC_LGUI,            KC_SPC,  KC_RALT, KC_SLSH, KC_LEFT,  KC_DOWN,  KC_RGHT
    ),
    [1] = LAYOUT_tkl_ansi(
        KC_F1,   KC_F2,   KC_F3,     KC_F4,    KC_F5,      KC_F6,     KC_F7,    KC_F8,    KC_F9,   KC_F10,   KC_F11,        KC_F12,
        KC_DEL,  KC_A,    KC_S,      KC_D,     KC_F,       KC_G,      KC_H,     KC_J,     KC_KP_7, KC_KP_8,  KC_KP_9,       KC_DEL,
        KC_LSFT, KC_Z,    KC_X,      KC_C,     KC_V,       KC_B,      KC_N,     KC_M,     KC_KP_4, KC_KP_5,  KC_KP_6,       KC_KP_DOT,
        MO(3),   KC_LCTL, KC_GRV,    KC_LALT,  KC_LGUI,               KC_SPC,   KC_KP_ASTERISK,     KC_KP_1, KC_KP_2,  KC_KP_3, KC_KP_0
    ),
    [2] = LAYOUT_tkl_ansi(
        KC_INS,  MD_BLE1, MD_BLE2,   MD_BLE3,  MD_24G,     RM_NEXT,   TO(1),    TO(0),   KC_LBRC, KC_RBRC,  KC_BSLS,  KC_DEL,
        KC_ESC,  KC_1,    KC_2,      KC_3,     KC_4,       KC_5,      KC_6,     KC_7,    KC_8,    KC_9,     KC_0,     KC_MINS,
        KC_LSFT, KC_INS,  KC_DEL,    KC_HOME,  KC_END,     KC_PGUP,   KC_PGDN,  RM_SATD, RM_HUED, RM_HUEU,  KC_PGUP,  QK_BAT,
        KC_NO,   KC_DEL,  KC_GRV,    KC_LGUI,  KC_LALT,               KC_HOME,  KC_END,  KC_EQL,  RM_VALD,  KC_PGDN,  RM_VALU
    ),
    [3] = LAYOUT_tkl_ansi(
        KC_TAB,  MD_BLE1, MD_BLE2,   MD_BLE3,  MD_24G,     RM_NEXT,   TO(1),    TO(0),   KC_LBRC, KC_RBRC,  KC_BSLS,  RM_TOGG,
        KC_ESC,  KC_1,    KC_2,      KC_3,     KC_4,       KC_5,      KC_6,     KC_7,    KC_8,    KC_9,     KC_0,     KC_MINS,
        KC_LSFT, KC_INS,  KC_DEL,    KC_HOME,  KC_END,     KC_PGUP,   KC_PGDN,  RM_SATD, RM_HUED, RM_HUEU,  RM_VALU , QK_BAT,
        KC_NO,   KC_DEL,  KC_GRV,    KC_LALT,  QK_WLO,                EE_CLR,   RM_SATU, KC_EQL,  RM_SPDD,  RM_VALD,  RM_SPDU
    )
};
// clang-format on

/* --- Bake dei combo di default (accesso ai layer) ------------------------- *
 * A differenza dei layer, i combo Vial NON stanno in keymap.c: vivono in EEPROM
 * e dynamic_keymap_reset() (chiamato da via_init a ogni EE_CLR / primo boot) li
 * azzera. Per renderli comunque dei default li riseminiamo qui se lo slot è vuoto,
 * poi ricarichiamo la copia RAM di Vial con vial_init(). Un edit dell'utente nello
 * slot lo rende non-vuoto -> non viene risovrascritto. Sorgente: LUMA899.vil.
 *   slot 0: KC_GRV + KC_LALT -> MO(3)
 *   slot 1: MO(2)  + KC_LCTL -> MO(1)
 * Chiamato da keyboard_post_init_user (in luma40.c) DOPO il reset di via_init.   */
#include "vial.h"
#include "dynamic_keymap.h"

void luma40_keymap_post_init(void) {
    static const vial_combo_entry_t defaults[] = {
        { .input = {KC_GRV, KC_LALT, KC_NO, KC_NO}, .output = MO(3) },
        { .input = {MO(2),  KC_LCTL, KC_NO, KC_NO}, .output = MO(1) },
    };
    bool seeded = false;
    for (uint8_t i = 0; i < sizeof(defaults) / sizeof(defaults[0]); i++) {
        vial_combo_entry_t cur = {0};
        if (dynamic_keymap_get_combo(i, &cur) == 0 && cur.output == KC_NO) {
            dynamic_keymap_set_combo(i, &defaults[i]);
            seeded = true;
        }
    }
    if (seeded) {
        vial_init();   // ricarica key_combos[] dall'EEPROM appena seminata
    }
}
