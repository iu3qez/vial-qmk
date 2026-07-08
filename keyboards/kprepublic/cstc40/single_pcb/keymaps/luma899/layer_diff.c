// SPDX-License-Identifier: GPL-2.0-or-later
#include "layer_diff.h"

#include <string.h>
#include "quantum.h"
#include "rgb_matrix.h"

// Categorie di keycode per la colorazione dell'overlay diff.
enum {
    CAT_NUMBER = 0, // KC_1..KC_0
    CAT_LETTER,     // KC_A..KC_Z
    CAT_NAV,        // frecce + Ins/Home/PgUp/Del/End/PgDn
    CAT_SYMBOL,     // KC_MINUS..KC_SLASH (punteggiatura/simboli)
    CAT_FUNCTION,   // KC_F1..KC_F12 e KC_F13..KC_F24
    CAT_OTHER,      // tutto il resto (wireless, RGB, layer, modificatori, ...)
    CAT_COUNT,
};
#define CAT_NONE 0xFF // tasto non evidenziato (spento)

// Palette fissa per categoria (cambio = ricompilare). Scalata a runtime per la
// luminosità RGB corrente, come faceva l'overlay per-layer.
static const uint8_t category_colors[CAT_COUNT][3] = {
    [CAT_NUMBER] = {255, 255,   0}, // giallo
    [CAT_LETTER] = {255, 255, 255}, // bianco
    [CAT_NAV]      = {  0, 255, 255}, // ciano
    [CAT_SYMBOL]   = {255,   0, 255}, // magenta
    [CAT_FUNCTION] = {  0,   0, 255}, // blu (tasti F)
    [CAT_OTHER]    = {150,   0, 255}, // viola
};

// Categoria per LED: CAT_NONE = spento, altrimenti indice in category_colors.
static uint8_t layer_diff_cat[RGB_MATRIX_LED_COUNT];

// Classifica il keycode reale del tasto. I range QMK sfruttati sono contigui,
// quindi bastano pochi confronti. Layer-tap/mod-tap e i custom keycode (>0xFF)
// cadono naturalmente in CAT_OTHER.
static uint8_t luma40_classify(uint16_t kc) {
    if (kc >= KC_A && kc <= KC_Z) {
        return CAT_LETTER;
    }
    if (kc >= KC_1 && kc <= KC_0) {
        return CAT_NUMBER;
    }
    if (kc >= KC_MINUS && kc <= KC_SLASH) {
        return CAT_SYMBOL;
    }
    if (kc >= KC_INSERT && kc <= KC_UP) {
        return CAT_NAV;
    }
    if ((kc >= KC_F1 && kc <= KC_F12) || (kc >= KC_F13 && kc <= KC_F24)) {
        return CAT_FUNCTION;
    }
    return CAT_OTHER;
}

// La categoria è ricalcolata al CAMBIO LAYER (layer_state_set_user), non al
// cambio keymap: un remap Vial del layer già attivo si riflette solo rientrando.
void layer_diff_recompute(uint8_t layer) {
    memset(layer_diff_cat, CAT_NONE, sizeof(layer_diff_cat));
    if (layer == 0) {
        return;
    }
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            uint8_t led = g_led_config.matrix_co[row][col];
            if (led == NO_LED || led >= RGB_MATRIX_LED_COUNT) {
                continue;
            }
            keypos_t pos  = {.col = col, .row = row};
            uint16_t kc   = keymap_key_to_keycode(layer, pos);
            uint16_t base = keymap_key_to_keycode(0, pos);
            if (kc != KC_TRANSPARENT && kc != base) {
                layer_diff_cat[led] = luma40_classify(kc);
            }
        }
    }
}

void layer_diff_overlay(uint8_t led_min, uint8_t led_max) {
    if (!rgb_matrix_is_enabled()) {
        return;
    }
    if (get_highest_layer(layer_state) == 0) {
        return;
    }
    uint8_t val = rgb_matrix_get_val();

    for (uint8_t led = led_min; led < led_max && led < RGB_MATRIX_LED_COUNT; led++) {
        uint8_t cat = layer_diff_cat[led];
        if (cat == CAT_NONE) {
            rgb_matrix_set_color(led, 0, 0, 0);
        } else {
            const uint8_t *color = category_colors[cat];
            rgb_matrix_set_color(led,
                                 (uint8_t)((uint16_t)color[0] * val / 255),
                                 (uint8_t)((uint16_t)color[1] * val / 255),
                                 (uint8_t)((uint16_t)color[2] * val / 255));
        }
    }
}
