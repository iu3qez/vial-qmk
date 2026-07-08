// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdint.h>

// Ricalcola la bitmask dei tasti "diversi dal base" per il layer dato.
// Passare il layer attivo più alto; layer 0 azzera la mask.
void layer_diff_recompute(uint8_t layer);

// Applica l'overlay "Solo" al range [led_min, led_max): sfondo spento,
// tasti-diff accesi nel colore del layer. No-op se RGB spento o layer 0.
void layer_diff_overlay(uint8_t led_min, uint8_t led_max);
