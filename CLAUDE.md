# vial-qmk — porting Epomaker Luma40

Fork di [vial-qmk](https://github.com/vial-kb/vial-qmk) (base QMK ~0.29) con il porting
della **Epomaker Luma40** (MCU Eastsoft ES32 FS026, Cortex-M0, wireless tri-mode).
Il porting proviene dal fork [carlosedp/qmk_firmware](https://github.com/carlosedp/qmk_firmware)
(base QMK ~0.31): **non usare** il vecchio repo `qmk_firmware_th40` (pieno di blob binari).

## Build

Non serve toolchain locale: si compila con l'immagine Docker ufficiale QMK.

```bash
# Firmware Vial (keymap principale)
docker run --rm -v "$PWD:/qmk_firmware" -w /qmk_firmware ghcr.io/qmk/qmk_cli:latest \
    make SKIP_GIT=yes epomaker/luma40:vial

# Keymap default (senza VIA/Vial)
docker run --rm -v "$PWD:/qmk_firmware" -w /qmk_firmware ghcr.io/qmk/qmk_cli:latest \
    make SKIP_GIT=yes epomaker/luma40:default
```

Output: `epomaker_luma40_vial.bin` (copiato anche nella root del repo).

**Gotcha:** `.build/` viene creata da root dentro il container; per pulirla usare
`docker run ... sh -c "rm -rf .build"`, non `rm` diretto.

**Submodule:** `lib/chibios-contrib` deve stare sul commit `16782528` (upstream
qmk/ChibiOS-Contrib, contiene la piattaforma ES32/FS026). `lib/chibios` è lo stesso
commit pinnato da vial-qmk (`8bd61b80`).

## Struttura del porting

- `keyboards/epomaker/luma40/` — definizione tastiera (matrice 6×16, 47 tasti/LED,
  layout `LAYOUT_tkl_ansi`). Keymap: `default` (senza VIA) e `vial`.
- `lib/rdmctmzt_common/` — libreria vendor a sorgenti: wireless tri-mode
  (USB / 3×BLE / 2.4G via SPI), batteria, EEPROM emulata, LED custom.
- Modifiche al core vial-qmk (tenerle in commit separati per eventuali PR upstream):
  - `data/schemas/keyboard.jsonschema` + `lib/python/qmk/constants.py`: registrato il
    processore `FS026`;
  - `quantum/dynamic_keymap.c`: guardie `#ifdef VIAL_ENABLE` mancanti (bug di vial-qmk:
    senza, qualsiasi build con dynamic keymap ma senza Vial non compila).

## Vincoli hardware (criticità)

- **RAM 16 KB**: non fidarsi di `arm-none-eabi-size` (ChibiOS riempie la RAM con
  heap/stack, bss appare sempre pieno). Misurare il margine reale con i simboli
  `__heap_base__`/`__heap_end__` o la sezione `.heap`. Stato attuale: **~2,4 KB liberi**.
- **EEPROM emulata** (`lib/rdmctmzt_common/user_eeprom.c`): journal a 2 pagine flash
  da 8 KB a `0x1C000` (gli ultimi 16 KB dei 128 KB di flash → ~112 KB per il codice).
  Ogni record da 4 byte contiene 2 byte di dato → max teorico ~4 KB.
  `EEPROM_SIZE` è 2048; la cache RAM costa `EEPROM_SIZE + 66` byte, quindi ogni KB di
  EEPROM in più toglie 1 KB alla heap.
- **In vial-qmk le build solo-VIA sono vietate** (`#error` in `quantum/via.c`):
  il keymap `default` compila senza VIA, con fallback `keymap_key_to_keycode` in
  `user_system.c` per il tasto di wakeup.
- **Pagina flash "user" senza wear leveling**: `Keyboard_Info` è salvato con un
  erase+riscrittura completo di pagina (`eeprom_write_block_user`, 0x1BE00), diverso
  dal journal QMK. Il livello batteria NON viene più persistito (solo RAM + report SPI):
  era la fonte principale di usura. Salvano ancora su flash solo i cambi di modalità
  wireless / Win-lock / NKRO — eventi rari. Non reintrodurre `Save_Flash_Set()` nei
  percorsi batteria di `user_battery.c`.

## Keymap vial

- UID: `{0xB3, 0x52, 0xF8, 0xA7, 0x49, 0x3B, 0x36, 0xD0}`; sblocco: **Tab + Backspace**.
- Profilo feature: tutte attive (tap dance, combo, key override, alt-repeat, QMK
  Settings) con **12 slot ciascuna** (i tier automatici sarebbero 16, ridotti in
  `keymaps/vial/config.h` per lasciare margine); ~580 byte residui per le macro.
  Margini misurati: flash 85 KB su ~112, RAM libera ~2340 byte. Nota: ridurre gli
  slot libera sia EEPROM sia RAM (gli array di stato di tap dance/combo sono in RAM).
  Budget EEPROM: base 825 (57 config + 768 keymap 4 layer) + 40 settings + 3×160 entry.
- Costi flash misurati, se serve rimodulare: QMK Settings +5,4 KB; Key Override,
  Repeat, Tap Dance, Combo +1,3–1,7 KB ciascuno.
- L'ordine di `customKeycodes` in `keymaps/vial/vial.json` **deve** seguire l'enum
  `Custom_Keycodes` in `lib/rdmctmzt_common/rdmctmzt_common.h` (mapping posizionale
  da `QK_KB`). La `VIA_Mapping_Luma40.JSON` nella dir della tastiera è **stale**
  (era per il firmware a blob): non usarla come riferimento.

## LED: effetti + Layer Diff overlay

- **Effetti RGB ridotti** a soli `solid_color` + `pixel_rain` (in `keyboard.json`):
  gli altri ~48 sono disabilitati. Risparmio misurato sulla keymap `vial`:
  **flash 92062 → 77204 B (−14,5 KB)** e, bonus, **RAM libera 2336 → 2688 B (+352 B)**
  perché il taglio degli effetti reattivi rimuove il tracker di `RGB_MATRIX_KEYREACTIVE`.
  `RGB_MATRIX_FRAMEBUFFER_EFFECTS` è tenuto (96 byte): nessun effetto residuo lo usa
  (`pixel_rain` non lo richiede), quindi è recuperabile togliendo il solo `#define`.
- **Layer Diff overlay** (`luma40_layer_diff.c/.h`, cablato in `luma40.c`): quando gli
  effetti LED sono accesi e il layer attivo è ≠ base, lo sfondo si spegne e restano
  accesi **solo** i tasti la cui definizione differisce dal layer 0, un colore per layer
  (L1 rosso, L2 verde, L3 blu). Trasparenti esclusi (`kc != KC_TRANSPARENT && kc != base`).
  Mask di 6 byte ricalcolata in `layer_state_set_user` (Approccio "cache"); disegnata nel
  hook indicatori **prima** di `kb_rgb_matrix_indicators_common`, così Caps/connessione/
  batteria/Win-lock restano sopra. La palette è fissa nel firmware (cambio = ricompilare).

## Flash sul ferro

Bootloader: bootmagic tenendo premuto **Tab** (matrice 0,0) all'inserimento del cavo,
oppure il tasto reset fisico sul retro del PCB. Il flash del `.bin` non è ancora stato
testato da questo porting (bootloader `custom`).
