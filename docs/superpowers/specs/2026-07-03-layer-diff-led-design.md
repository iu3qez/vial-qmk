# Layer Diff LED overlay + effect pruning — Design

**Data:** 2026-07-03
**Tastiera:** Epomaker Luma40 (`keyboards/epomaker/luma40`)
**Branch:** `claude/brainstorming-5d82cq`

## Obiettivo

Una feature LED in due parti, con beneficio anche sul budget flash/RAM:

1. **Sfoltimento effetti** — ridurre le ~50 animazioni RGB Matrix abilitate a sole
   due: `solid_color` (uniforme) e `pixel_rain` (pioggia di pixel). Recupera
   diversi KB di flash.
2. **Overlay "Layer Diff"** — quando gli effetti LED sono accesi e si è su un
   layer diverso dal base, lo sfondo si spegne e restano illuminati **solo** i
   tasti la cui definizione differisce dal layer base, con un colore diverso per
   ogni layer.

## Decisioni prese (brainstorming)

| # | Tema | Decisione |
|---|------|-----------|
| D2 | Attivazione overlay | **Automatico**: sempre attivo quando gli effetti LED sono accesi e il layer attivo è ≠ base. Non è un effetto selezionabile né un toggle. |
| D3/D4 | Stile overlay | **Singolo stile "Solo"**: sfondo spento, solo i tasti-diff accesi. (Scartata l'ipotesi dei due stili.) |
| D5 | Colori per layer | **Palette fissa** nel firmware. Default: L1 = rosso, L2 = verde, L3 = blu. L0 nessun highlight. |
| D6 | Definizione di "diverso" | **Confronto semplice** sul layer attivo più alto vs layer 0. Un tasto è "diff" se il suo keycode `!= KC_TRANSPARENT` **e** `!=` keycode del base. I trasparenti non si accendono. |
| D7 | Indicatori | **Priorità sopra l'overlay**: Caps Lock, connessione (BLE/2.4G/USB), batteria, Win-lock restano sempre visibili, disegnati dopo l'overlay. Se un tasto-diff coincide con un indicatore, vince l'indicatore. |
| Impl | Approccio calcolo | **Approccio 1**: bitmask in cache ricalcolata al cambio layer (`layer_state_set_user`). |
| Impl | Framebuffer | `RGB_MATRIX_FRAMEBUFFER_EFFECTS` **resta definito** (96 byte RAM). Nessun effetto residuo lo usa (`pixel_rain` non lo richiede), quindi è recuperabile in futuro rimuovendo il solo `#define`. |
| Impl | Collocazione | Codice a **livello tastiera** (`luma40.c` + file dedicato `luma40_layer_diff.c/.h`), valido sia per keymap `default` che `vial`. |

## Architettura

Il RGB Matrix di QMK disegna l'animazione base nel buffer LED a ogni frame,
poi chiama il callback indicatori. Sul Luma40 il flusso è:

```
rgb_matrix_indicators_advanced_user()   [luma40.c]
    -> kb_rgb_matrix_indicators_common() [lib/rdmctmzt_common/keyboard_common.c]
```

L'overlay si inserisce **prima** del disegno indicatori, così gli indicatori lo
sovrascrivono (D7):

```c
bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    luma40_layer_diff_overlay(led_min, led_max);              // 1) sfondo off + tasti-diff
    return kb_rgb_matrix_indicators_common(led_min, led_max); // 2) indicatori sopra
}
```

### Unità 1 — Calcolo della bitmask (cache)

- **Cosa fa:** al cambio layer calcola quali LED evidenziare per il layer attivo.
- **Come:** hook `layer_state_set_user(layer_state_t state)` in `luma40.c` che
  chiama `luma40_layer_diff_recompute(get_highest_layer(state))`.
- **Stato:** `static uint8_t layer_diff_mask[6];` (47 bit, uno per LED — 6 byte RAM).
- **Algoritmo:** per ogni `(row, col)` della matrice:
  1. `uint8_t led = g_led_config.matrix_co[row][col];` se `led == NO_LED` salta.
  2. `uint16_t kc = keymap_key_to_keycode(layer, (keypos_t){col, row});`
  3. `uint16_t base = keymap_key_to_keycode(0, (keypos_t){col, row});`
  4. bit `led` settato se `kc != KC_TRANSPARENT && kc != base`.
- Se `layer == 0` la mask è azzerata (nessun highlight).
- **Dipendenze:** `g_led_config`, `keymap_key_to_keycode`, `MATRIX_ROWS/COLS`.

### Unità 2 — Disegno dell'overlay

- **Cosa fa:** applica lo stile "Solo" al range LED del frame corrente.
- **Come:** `void luma40_layer_diff_overlay(uint8_t led_min, uint8_t led_max);`
  - Se overlay **non** attivo (RGB spento/sleep, oppure highest layer == 0):
    return senza toccare nulla (gira l'animazione normale).
  - Se attivo: per ogni `led` in `[led_min, led_max)`:
    - se bit settato → `rgb_matrix_set_color(led, colore_layer)`;
    - altrimenti → `rgb_matrix_set_color(led, 0, 0, 0)` (spento).
- **Condizione "effetti accesi":** `rgb_matrix_is_enabled()` (e il callback non
  viene invocato in suspend). Highest layer letto da `get_highest_layer(layer_state)`.
- **Dipendenze:** `layer_diff_mask`, palette, `rgb_matrix_set_color`,
  `rgb_matrix_is_enabled`.

### Unità 3 — Palette

```c
static const rgb_t layer_colors[] = {
    {255,   0,   0}, // L1 rosso
    {  0, 255,   0}, // L2 verde
    {  0,   0, 255}, // L3 blu
};
```
Indicizzata per `highest_layer - 1`. Se in futuro si aggiungono layer oltre L3,
si estende l'array (o si applica un fallback). Valori RGB esatti rifinibili in
implementazione tenendo conto della `RGB_MATRIX_MAXIMUM_BRIGHTNESS`.

## Parte A — Sfoltimento effetti

In `keyboards/epomaker/luma40/keyboard.json`, blocco `rgb_matrix.animations`:
lasciare `true` **solo** `solid_color` e `pixel_rain`; portare a `false` (o
rimuovere) tutte le altre voci.

- Nessuna modifica al core QMK.
- `RGB_MATRIX_FRAMEBUFFER_EFFECTS` in `config.h` **resta** (scelta utente).

## File toccati

| File | Modifica |
|------|----------|
| `keyboards/epomaker/luma40/keyboard.json` | Ridurre `animations` a `solid_color` + `pixel_rain`. |
| `keyboards/epomaker/luma40/luma40.c` | Aggiungere `layer_state_set_user`; chiamare `luma40_layer_diff_overlay` prima degli indicatori. |
| `keyboards/epomaker/luma40/luma40_layer_diff.c` (nuovo) | Mask, palette, `recompute`, `overlay`. |
| `keyboards/epomaker/luma40/luma40_layer_diff.h` (nuovo) | Prototipi. |
| `keyboards/epomaker/luma40/rules.mk` | Aggiungere `luma40_layer_diff.c` alle SRC. |

## Budget e verifica

- **RAM:** +6 byte (mask) + qualche byte di codice. Framebuffer invariato (+0).
  Netto trascurabile sulla RAM; il vero guadagno è in flash.
- **Flash:** taglio di ~48 animazioni → attesi diversi KB liberati. Da misurare.
- **Misura:** build Docker `epomaker/luma40:vial` e `:default`, prima e dopo.
  RAM libera reale via simboli `__heap_base__`/`__heap_end__` (non
  `arm-none-eabi-size`, vedi CLAUDE.md).

## Test funzionale

1. L0 (base): gira l'animazione scelta (solid o pixel_rain), nessun highlight.
2. L1/L2/L3: sfondo spento, solo i tasti rimappati accesi nel colore del layer
   (rosso/verde/blu).
3. Tasti trasparenti su un layer superiore: **spenti** (non sono "diff").
4. Caps Lock / connessione / batteria / Win-lock: sempre visibili anche sul
   layer superiore (vincono sull'overlay).
5. RGB spento: nessun overlay (comportamento invariato).

## Fuori scope (YAGNI)

- Colori per layer configurabili da Vial/VIA o persistiti in EEPROM.
- Overlay "Sopra" (evidenziazione sull'animazione in corso).
- Keycode/toggle per attivare/disattivare o cambiare stile.
- Rimozione del framebuffer (rimandata; recuperabile in futuro).
