// SPDX-License-Identifier: GPL-2.0-or-later
//
// Layout LUMA899 (Epomaker Luma40) portato sulla KPRepublic CSTC40 single_pcb.
// La CSTC40 è LAYOUT_planck_mit (47 tasti, spazio 2u) = stessa disposizione del
// Luma40, quindi i layer si mappano 1:1. I keycode wireless/batteria del Luma40
// (MD_BLE*, MD_24G, QK_BAT, QK_WLO) non esistono qui -> resi KC_NO.
// Extra: overlay "Layer Diff" (come sul Luma40) e seeding dei combo di default.

#include QMK_KEYBOARD_H
#include "vial.h"
#include "dynamic_keymap.h"
#include "layer_diff.h"

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT_planck_mit( // Alphabet
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_BSPC,
        KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_ENT,
        KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_UP,   KC_QUOT,
        MO(2),   KC_LCTL, KC_GRV,  KC_LALT, KC_LGUI,      KC_SPC,     KC_RALT, KC_SLSH, KC_LEFT, KC_DOWN, KC_RGHT
    ),
    [1] = LAYOUT_planck_mit( // F-keys & Numpad
        KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,
        KC_DEL,  KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_KP_7, KC_KP_8, KC_KP_9, KC_DEL,
        KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_KP_4, KC_KP_5, KC_KP_6, KC_KP_DOT,
        MO(3),   KC_LCTL, KC_GRV,  KC_LALT, KC_LGUI,      KC_SPC,     KC_KP_ASTERISK, KC_KP_1, KC_KP_2, KC_KP_3, KC_KP_0
    ),
    [2] = LAYOUT_planck_mit( // Symbols / Nav / RGB (wireless -> KC_NO)
        KC_INS,  KC_NO,   KC_NO,   KC_NO,   KC_NO,   RM_NEXT, TO(1),   TO(0),   KC_LBRC, KC_RBRC, KC_BSLS, KC_DEL,
        KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS,
        KC_LSFT, KC_INS,  KC_DEL,  KC_HOME, KC_END,  KC_PGUP, KC_PGDN, RM_SATD, RM_HUED, RM_HUEU, KC_PGUP, KC_NO,
        KC_NO,   KC_DEL,  KC_GRV,  KC_LGUI, KC_LALT,      KC_HOME,    KC_END,  KC_EQL,  RM_VALD, KC_PGDN, RM_VALU
    ),
    [3] = LAYOUT_planck_mit( // Numbers / Nav / RGB (wireless -> KC_NO)
        KC_TAB,  KC_NO,   KC_NO,   KC_NO,   KC_NO,   RM_NEXT, TO(1),   TO(0),   KC_LBRC, KC_RBRC, KC_BSLS, RM_TOGG,
        KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS,
        KC_LSFT, KC_INS,  KC_DEL,  KC_HOME, KC_END,  KC_PGUP, KC_PGDN, RM_SATD, RM_HUED, RM_HUEU, RM_VALU, KC_NO,
        KC_NO,   KC_DEL,  KC_GRV,  KC_LALT, KC_NO,        EE_CLR,     RM_SATU, KC_EQL,  RM_SPDD, RM_VALD, RM_SPDU
    )
};
// clang-format on

// Layer Diff: ricalcola la mask al cambio layer.
layer_state_t layer_state_set_user(layer_state_t state) {
    layer_diff_recompute(get_highest_layer(state));
    return state;
}

// Layer Diff: disegna l'overlay (sfondo spento + tasti-diff colorati per categoria).
bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    layer_diff_overlay(led_min, led_max);
    return true;
}

// Seeding dei combo di default (accesso layer) + primo ricalcolo Layer Diff.
// Gira dopo il reset di via_init; slot già editati (output != KC_NO) non toccati.
//   slot 0: KC_GRV + KC_LALT -> MO(3)
//   slot 1: MO(2)  + KC_LCTL -> MO(1)
// Marker di versione in eeconfig_user: se non combacia (EEPROM vergine o di un
// firmware diverso) forziamo UN reset completo, poi lo scriviamo così i boot
// successivi preservano gli edit fatti in Vial.
#define LUMA899_EE_MAGIC 0x4C393031UL  // "L901"

void keyboard_post_init_user(void) {
    if (eeconfig_read_user() != LUMA899_EE_MAGIC) {
        eeconfig_init();          // reset eeconfig quantum (RGB, ecc.)
        dynamic_keymap_reset();   // ricarica la keymap bakeata + azzera macro/combo
        eeconfig_update_user(LUMA899_EE_MAGIC);
    }

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
        vial_init();
    }
    layer_diff_recompute(get_highest_layer(layer_state));
}
