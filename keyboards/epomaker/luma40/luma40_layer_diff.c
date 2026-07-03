// SPDX-License-Identifier: GPL-2.0-or-later
#include "luma40_layer_diff.h"

#include <string.h>
#include "quantum.h"
#include "rgb_matrix.h"

#define LAYER_DIFF_MASK_BYTES ((RGB_MATRIX_LED_COUNT + 7) / 8)

// Palette fissa per L1, L2, L3 (L0 non evidenziato).
static const uint8_t layer_colors[][3] = {
    {255,   0,   0}, // L1 rosso
    {  0, 255,   0}, // L2 verde
    {  0,   0, 255}, // L3 blu
};
#define LAYER_DIFF_COLOR_COUNT (sizeof(layer_colors) / sizeof(layer_colors[0]))

static uint8_t layer_diff_mask[LAYER_DIFF_MASK_BYTES];

// La mask è ricalcolata al CAMBIO LAYER (layer_state_set_user), non al cambio
// keymap: un remap Vial del layer già attivo si riflette solo rientrando nel layer.
void luma40_layer_diff_recompute(uint8_t layer) {
    memset(layer_diff_mask, 0, sizeof(layer_diff_mask));
    if (layer == 0) {
        return;
    }
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            uint8_t led = g_led_config.matrix_co[row][col];
            if (led == NO_LED) {
                continue;
            }
            keypos_t pos  = {.col = col, .row = row};
            uint16_t kc   = keymap_key_to_keycode(layer, pos);
            uint16_t base = keymap_key_to_keycode(0, pos);
            if (kc != KC_TRANSPARENT && kc != base) {
                layer_diff_mask[led / 8] |= (uint8_t)(1u << (led % 8));
            }
        }
    }
}

void luma40_layer_diff_overlay(uint8_t led_min, uint8_t led_max) {
    if (!rgb_matrix_is_enabled()) {
        return;
    }
    uint8_t layer = get_highest_layer(layer_state);
    if (layer == 0) {
        return;
    }
    // Fallback all'ultimo colore se ci sono più layer dei colori definiti.
    uint8_t             idx   = ((uint8_t)(layer - 1) < LAYER_DIFF_COLOR_COUNT) ? (layer - 1) : (LAYER_DIFF_COLOR_COUNT - 1);
    const uint8_t      *color = layer_colors[idx];

    // Scala la palette per la luminosità RGB corrente, così i tasti-diff seguono
    // il livello impostato dall'utente (come fa l'animazione base).
    uint8_t val = rgb_matrix_get_val();
    uint8_t r   = (uint8_t)((uint16_t)color[0] * val / 255);
    uint8_t g   = (uint8_t)((uint16_t)color[1] * val / 255);
    uint8_t b   = (uint8_t)((uint16_t)color[2] * val / 255);

    for (uint8_t led = led_min; led < led_max; led++) {
        if (layer_diff_mask[led / 8] & (uint8_t)(1u << (led % 8))) {
            rgb_matrix_set_color(led, r, g, b);
        } else {
            rgb_matrix_set_color(led, 0, 0, 0);
        }
    }
}
