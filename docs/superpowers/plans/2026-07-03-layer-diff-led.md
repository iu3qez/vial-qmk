# Layer Diff LED overlay + effect pruning — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ridurre le animazioni RGB Matrix del Luma40 a `solid_color` + `pixel_rain` e aggiungere un overlay automatico che, sui layer ≠ base, spegne lo sfondo e illumina solo i tasti rimappati con un colore per layer.

**Architecture:** L'overlay si aggancia a `rgb_matrix_indicators_advanced_user()` (già presente in `luma40.c`) e viene disegnato **prima** degli indicatori, così Caps/connessione/batteria/Win-lock lo sovrascrivono. Quali tasti evidenziare è precalcolato in una bitmask a ogni cambio layer (`layer_state_set_user`). Codice a livello tastiera, valido per keymap `default` e `vial`.

**Tech Stack:** QMK (~0.29, vial-qmk), RGB Matrix, C (Cortex-M0), build via immagine Docker QMK.

## Global Constraints

- RAM totale 16 KB; margine reale attuale ~2340 byte. Misurare la RAM **solo** con i simboli `__heap_base__`/`__heap_end__`, mai con `arm-none-eabi-size` (bss appare sempre pieno). Fonte: `CLAUDE.md`.
- `RGB_MATRIX_FRAMEBUFFER_EFFECTS` in `config.h` **resta definito** (scelta utente): i 96 byte del framebuffer non vanno recuperati in questo lavoro.
- Nessuna modifica al core QMK (`quantum/`, `data/`): il lavoro è confinato a `keyboards/epomaker/luma40/`.
- Non reintrodurre scritture flash nei percorsi batteria (vincolo di usura, `CLAUDE.md`).
- `keypos_t` in QMK ha ordine campi `{ uint8_t col; uint8_t row; }`. `NO_LED` = 255. `RGB_MATRIX_LED_COUNT` = 47.
- Build canonica (da `CLAUDE.md`):
  ```bash
  docker run --rm -v "$PWD:/qmk_firmware" -w /qmk_firmware ghcr.io/qmk/qmk_cli:latest \
      make SKIP_GIT=yes epomaker/luma40:vial
  ```
  Il toolchain `arm-none-eabi-*` è anche installato localmente in questo ambiente, quindi `make SKIP_GIT=yes epomaker/luma40:vial` funziona pure senza Docker.
- Pulizia `.build/` (creata da root dentro Docker):
  ```bash
  docker run --rm -v "$PWD:/qmk_firmware" -w /qmk_firmware ghcr.io/qmk/qmk_cli:latest sh -c "rm -rf .build"
  ```

**Nota sui test:** target embedded senza harness di unit test per il codice LED keyboard-specific. La verifica di ogni task è: (a) build che compila senza errori/warning nuovi, (b) misura flash/RAM, (c) checklist funzionale sul ferro (Task 4). Non ci sono test automatici da far fallire prima.

---

### Task 1: Sfoltire le animazioni RGB + baseline di misura

**Files:**
- Modify: `keyboards/epomaker/luma40/keyboard.json` (blocco `rgb_matrix.animations`)

**Interfaces:**
- Consumes: nulla.
- Produces: nulla di codice. Stabilisce la baseline flash/RAM per confronto nei task successivi.

- [ ] **Step 1: Misura baseline (prima di modificare)**

Compila entrambe le keymap e registra flash + RAM libera:
```bash
make SKIP_GIT=yes epomaker/luma40:vial
arm-none-eabi-size .build/epomaker_luma40_vial.elf
arm-none-eabi-nm .build/epomaker_luma40_vial.elf | grep -E '__heap_base__|__heap_end__'
```
Annota `text` (flash) e la differenza `__heap_end__ - __heap_base__` (RAM libera ≈ 2340). Ripeti per `:default` se serve confronto.

- [ ] **Step 2: Ridurre la lista animazioni**

In `keyboards/epomaker/luma40/keyboard.json`, sostituire l'intero blocco `animations` con solo le due volute a `true` (tutte le altre voci rimosse):
```json
        "animations": {
            "solid_color": true,
            "pixel_rain": true
        }
```

- [ ] **Step 3: Build e verifica compilazione**

```bash
make SKIP_GIT=yes epomaker/luma40:vial
make SKIP_GIT=yes epomaker/luma40:default
```
Expected: entrambe le build terminano con `[OK]` e producono il `.bin`.

- [ ] **Step 4: Misura risparmio flash**

```bash
arm-none-eabi-size .build/epomaker_luma40_vial.elf
```
Expected: `text` sensibilmente più basso della baseline (attesi diversi KB in meno). Annotare il delta.

- [ ] **Step 5: Commit**

```bash
git add keyboards/epomaker/luma40/keyboard.json
git commit -m "luma40: prune RGB animations to solid_color + pixel_rain"
```

---

### Task 2: Modulo Layer Diff (mask + palette + disegno)

**Files:**
- Create: `keyboards/epomaker/luma40/luma40_layer_diff.h`
- Create: `keyboards/epomaker/luma40/luma40_layer_diff.c`
- Modify: `keyboards/epomaker/luma40/rules.mk` (append `SRC += luma40_layer_diff.c`)

**Interfaces:**
- Consumes: `g_led_config.matrix_co`, `keymap_key_to_keycode(uint8_t, keypos_t)`, `rgb_matrix_set_color(idx,r,g,b)`, `rgb_matrix_is_enabled()`, `get_highest_layer(layer_state_t)`, `layer_state` (global), `MATRIX_ROWS`, `MATRIX_COLS`, `RGB_MATRIX_LED_COUNT`, `NO_LED`, `KC_TRANSPARENT`.
- Produces:
  - `void luma40_layer_diff_recompute(uint8_t layer);`
  - `void luma40_layer_diff_overlay(uint8_t led_min, uint8_t led_max);`

- [ ] **Step 1: Creare l'header**

`keyboards/epomaker/luma40/luma40_layer_diff.h`:
```c
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdint.h>

// Ricalcola la bitmask dei tasti "diversi dal base" per il layer dato.
// Passare il layer attivo più alto; layer 0 azzera la mask.
void luma40_layer_diff_recompute(uint8_t layer);

// Applica l'overlay "Solo" al range [led_min, led_max): sfondo spento,
// tasti-diff accesi nel colore del layer. No-op se RGB spento o layer 0.
void luma40_layer_diff_overlay(uint8_t led_min, uint8_t led_max);
```

- [ ] **Step 2: Creare l'implementazione**

`keyboards/epomaker/luma40/luma40_layer_diff.c`:
```c
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
    uint8_t             idx   = (layer - 1 < LAYER_DIFF_COLOR_COUNT) ? (layer - 1) : (LAYER_DIFF_COLOR_COUNT - 1);
    const uint8_t      *color = layer_colors[idx];

    for (uint8_t led = led_min; led < led_max; led++) {
        if (layer_diff_mask[led / 8] & (uint8_t)(1u << (led % 8))) {
            rgb_matrix_set_color(led, color[0], color[1], color[2]);
        } else {
            rgb_matrix_set_color(led, 0, 0, 0);
        }
    }
}
```

- [ ] **Step 3: Aggiungere il file al build**

In coda a `keyboards/epomaker/luma40/rules.mk` aggiungere:
```make
# Layer Diff LED overlay
SRC += luma40_layer_diff.c
```

- [ ] **Step 4: Build e verifica compilazione**

Il modulo non è ancora chiamato da nessuno: verifichiamo solo che compili e linki (le funzioni possono risultare inutilizzate a questo stadio — è atteso).
```bash
make SKIP_GIT=yes epomaker/luma40:vial
```
Expected: build `[OK]`. Nessun errore su `g_led_config`, `keymap_key_to_keycode`, `NO_LED`, `KC_TRANSPARENT`.

- [ ] **Step 5: Commit**

```bash
git add keyboards/epomaker/luma40/luma40_layer_diff.c keyboards/epomaker/luma40/luma40_layer_diff.h keyboards/epomaker/luma40/rules.mk
git commit -m "luma40: add Layer Diff LED module (mask + overlay draw)"
```

---

### Task 3: Cablaggio in luma40.c + misura RAM

**Files:**
- Modify: `keyboards/epomaker/luma40/luma40.c` (include, `layer_state_set_user`, chiamata overlay nel hook indicatori, recompute al post_init)

**Interfaces:**
- Consumes: `luma40_layer_diff_recompute`, `luma40_layer_diff_overlay` (Task 2); `kb_rgb_matrix_indicators_common` (esistente); `get_highest_layer`, `layer_state`.
- Produces: comportamento overlay attivo a runtime.

- [ ] **Step 1: Includere l'header**

In `keyboards/epomaker/luma40/luma40.c`, dopo `#include "keyboard_common.h"` aggiungere:
```c
#include "luma40_layer_diff.h"
```

- [ ] **Step 2: Chiamare l'overlay prima degli indicatori**

Sostituire la funzione esistente `rgb_matrix_indicators_advanced_user` (attualmente alle righe ~67-69) con:
```c
bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    luma40_layer_diff_overlay(led_min, led_max);
    return kb_rgb_matrix_indicators_common(led_min, led_max);
}
```

- [ ] **Step 3: Aggiungere l'hook di cambio layer**

Aggiungere (accanto agli altri `*_user` in `luma40.c`) l'hook che ricalcola la mask a ogni cambio layer:
```c
layer_state_t layer_state_set_user(layer_state_t state) {
    luma40_layer_diff_recompute(get_highest_layer(state));
    return state;
}
```

- [ ] **Step 4: Ricalcolo iniziale al post_init**

Nella funzione esistente `keyboard_post_init_user` di `luma40.c`, dopo `kb_keyboard_post_init();`, aggiungere il calcolo iniziale (copre l'avvio su un default layer ≠ 0):
```c
    luma40_layer_diff_recompute(get_highest_layer(layer_state));
```
Risultato atteso della funzione:
```c
void keyboard_post_init_user(void) {
    kb_keyboard_post_init();
    luma40_layer_diff_recompute(get_highest_layer(layer_state));
}
```

- [ ] **Step 5: Build entrambe le keymap**

```bash
make SKIP_GIT=yes epomaker/luma40:vial
make SKIP_GIT=yes epomaker/luma40:default
```
Expected: entrambe `[OK]`.

- [ ] **Step 6: Misura RAM libera (deve restare ≥ ~2300 byte)**

```bash
arm-none-eabi-nm .build/epomaker_luma40_vial.elf | grep -E '__heap_base__|__heap_end__'
```
Expected: `__heap_end__ - __heap_base__` ≈ baseline − ~6 byte (mask) − pochi byte di codice statico. Deve restare abbondantemente positivo (~2330). Se calasse molto più del previsto, indagare prima di procedere.

- [ ] **Step 7: Commit**

```bash
git add keyboards/epomaker/luma40/luma40.c
git commit -m "luma40: wire Layer Diff overlay into indicators + layer hooks"
```

---

### Task 4: Verifica funzionale sul ferro + note

**Files:**
- Modify: `docs/superpowers/specs/2026-07-03-layer-diff-led-design.md` (spuntare l'esito test, se si vuole tracciare)
- Modify: `CLAUDE.md` (una riga nella sezione keymap sui nuovi margini flash/RAM misurati, opzionale ma consigliato)

**Interfaces:**
- Consumes: firmware costruito nei Task 1-3.
- Produces: conferma comportamento + budget aggiornato.

- [ ] **Step 1: Flash del `.bin` sull'hardware**

Entrare in bootloader (tenere **Tab** = matrice 0,0 all'inserimento del cavo, oppure reset fisico sul retro) e flashare `epomaker_luma40_vial.bin`. Nota: il flash da questo porting non è ancora stato validato (bootloader `custom`) — procedere con cautela.

- [ ] **Step 2: Checklist funzionale**

Verificare sull'hardware:
- [ ] L0 (base): gira l'animazione scelta (solid o pixel_rain), nessun highlight.
- [ ] L1: sfondo spento, solo i tasti rimappati accesi in **rosso**.
- [ ] L2: idem in **verde**. L3: idem in **blu**.
- [ ] I tasti **trasparenti** sui layer superiori restano **spenti**.
- [ ] Caps Lock / connessione (BLE/2.4G/USB) / batteria / Win-lock **sempre visibili** anche sui layer superiori.
- [ ] Con RGB **spento**: nessun overlay, comportamento invariato.

- [ ] **Step 3: Registrare i margini misurati**

Aggiornare in `CLAUDE.md` (sezione "Keymap vial" / margini) i valori flash e RAM libera reali misurati dopo lo sfoltimento + overlay, così la documentazione resta veritiera.

- [ ] **Step 4: Commit**

```bash
git add CLAUDE.md docs/superpowers/specs/2026-07-03-layer-diff-led-design.md
git commit -m "docs: record Layer Diff results and updated flash/RAM margins"
```

---

## Self-Review

**Spec coverage:**
- Parte A (sfoltimento effetti) → Task 1. ✓
- Overlay "Solo", attivazione automatica su layer ≠ base con RGB acceso → Task 2 (`luma40_layer_diff_overlay`) + Task 3 (cablaggio). ✓
- Bitmask ricalcolata al cambio layer (Approccio 1) → Task 2 (`recompute`) + Task 3 (`layer_state_set_user`, post_init). ✓
- Regola "diff" con esclusione `KC_TRANSPARENT` (D6) → Task 2, Step 2. ✓
- Palette fissa rosso/verde/blu (D5) → Task 2, Step 2. ✓
- Indicatori con priorità (D7) → Task 3, Step 2 (overlay chiamato **prima** di `kb_rgb_matrix_indicators_common`). ✓
- Framebuffer invariato → nessun task lo tocca (constraint globale). ✓
- Budget/misura RAM con simboli heap → Task 1, 3. ✓
- Test funzionale sul ferro → Task 4. ✓

**Placeholder scan:** nessun TBD/TODO; ogni step ha codice/comandi concreti.

**Type consistency:** `luma40_layer_diff_recompute(uint8_t)` e `luma40_layer_diff_overlay(uint8_t, uint8_t)` usati con le stesse firme in header (Task 2), definizione (Task 2) e chiamate (Task 3). `keypos_t {.col, .row}`, `NO_LED`, `KC_TRANSPARENT`, `LAYER_DIFF_COLOR_COUNT` coerenti.
