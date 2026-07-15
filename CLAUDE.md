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
- **FLASH: ~81,4 KB per il codice, NON 112 KB** — limite verificato sul ferro, vedi
  "Limite di flash reale". Sforarlo = firmware che non parte, senza alcun errore in
  compilazione. È il vincolo più stretto del porting insieme alla RAM.
- **Il flash è intermittente**: una scheda "morta" dopo un flash spesso rivive
  ricopiando lo stesso `.bin` — vedi "Flash intermittente". Non dedurre nulla sul
  firmware da un singolo tentativo.
- **EEPROM emulata** (`lib/rdmctmzt_common/user_eeprom.c`): journal a 2 pagine flash
  da 8 KB a `0x1C000`, preceduto dalla pagina "user" a `0x1BE00`. Attenzione: i
  ~112 KB che si otterrebbero contando `0x1BE00` dall'indirizzo 0 sono **sbagliati**,
  perché l'app non parte da 0 — vedi "Limite di flash reale".
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

## Limite di flash reale: ~81,4 KB (verificato sul ferro, 15 lug 2026)

**L'app ha ~81,4 KB, non ~112 KB.** Superarli produce un firmware che **non parte**
(nessun LED, nessuna enumerazione) e **il compilatore non se ne accorge**: il linker
script dichiara `flash0 org = 0x00000000, len = 128k` (`FS026.ld`), che è una bugia.

**Perché:** l'app è linkata a `0x0` ma vive **fisicamente sopra il bootloader**, che la
rimappa. `ES_MCU_MEM_REMAP_OFFSET` (`user_eeprom.c:59`) legge `REALBASE` da
`SYSCFG->REMAP` a runtime. Lo spazio vero è `0x1BE00 − REALBASE` (`0x1BE00` = pagina
"user", il primo dato sopra l'app). I dati implicano **`REALBASE = 0x8000`**, cioè un
bootloader da 32 KB → limite **81408** (`0x13E00`) oppure **81920** (`0x14000`) se il
confine fosse il journal invece della pagina user: i due non sono stati separati
(servirebbe il build da 81600 B). **Usare 81408 come budget.**

**Come è stato dimostrato** (padding di byte inerti in `.rodata`, a heap costante — così
la dimensione flash è l'**unica** variabile e la RAM è esclusa per costruzione):

| build | flash | heap | boota |
|---|---|---|---|
| `f4bc334` | 73392 | 2656 | ✓ |
| `2c06b88` | 73500 | 2656 | ✓ |
| padding | 80924 | 2656 | ✓ |
| padding | **82924** | **2656** | **✗** |
| `b6f10ea` | 85580 | 2272 | ✗ |

Le due build col padding differiscono **solo** per 2000 B di `.rodata`: heap identica,
comportamento identico. Una parte, l'altra no → è la flash, non la RAM.

**Se un firmware sfora, la leva è `LTO_ENABLE = yes`**: sui 50 effetti RGB fa
**85580 → 76288 B (−10,9%)** e per giunta **+464 B di heap** (2272 → 2736), perché
elimina anche dati morti. Non è attiva di default in questo porting.
⚠️ La LTO può risolvere male i **simboli weak**: `luma40_keymap_post_init()` (bake dei
combo) è agganciata da `luma40.c` con un hook weak. Dopo un build LTO **verificare che i
combo rispondano**; se non lo fanno, togliere il `weak` e chiamarla direttamente.

## Flash intermittente: un tentativo solo NON è un test (15 lug 2026)

**Il bootloader è inaffidabile: lo stesso `.bin` può non partire a un tentativo e
partire a quello dopo.** Verificato sul ferro con due firmware diversi (`f4bc334` e
`2c06b88`): dati per "non funzionanti" dopo un flash, sono partiti entrambi
ricopiando **lo stesso identico file**, senza ricompilare nulla.

Sintomo di un flash andato male: nessun LED, nessuna enumerazione USB, nessun segno di
vita. Il **bootloader però risponde sempre**: la scheda non è mai brickata, si recupera
ricopiando il `.bin`. Se una scheda sembra morta, **riprovare il flash più volte prima
di sospettare del firmware**.

**Regola metodologica (imparata sbagliando):** non concludere mai nulla da un singolo
tentativo di flash — né "è rotto" né "è a posto". Con un apparato intermittente un
tentativo isolato misura la fortuna della copia, non il codice.

Qui c'erano **due fenomeni sovrapposti e indipendenti**: questa intermittenza (rumore)
e un limite di flash reale (segnale, vedi sezione precedente). Il rumore ha depistato
in **entrambe** le direzioni: prima ha fatto sembrare rotti dei build sani (`f4bc334`,
`2c06b88`, falliti una volta e poi partiti col medesimo file), poi — una volta scoperta
l'intermittenza — ha quasi fatto archiviare come "artefatto" la soglia vera, che invece
esisteva. La discriminante è stata la **riproducibilità**: `b6f10ea` non è partito *mai*,
gli altri partivano a ritentare. Chiedersi "è riproducibile?" al primo esito anomalo,
non al quinto.

**Domanda aperta:** perché il flash è intermittente. Non è indagato. Ipotesi non
verificate: copia non sincronizzata su disco prima dello scollegamento, oppure il
bootloader richiede un eject/attesa. Non esiste oggi un passo di **verifica**
dell'immagine scritta: sarebbe la prima cosa da aggiungere.

**Fatti sul codice raccolti durante l'indagine** (letti nei sorgenti, questi sì solidi):
- Le attese su `ES_SPI_ACK_IO` **hanno** timeout (`user_spi.c:174` = 2000 iterazioni,
  `user_system.c:249` = 100 ms): un hang del wireless non è una spiegazione plausibile.
- `DEBOUNCE_TYPE = asym_eager_defer_pk` e `debounce_init()`
  (`quantum/debounce/asym_eager_defer_pk.c:67`) fa `malloc` **senza check NULL** e
  scrive subito: se la heap finisse → deref di null → hardfault al boot. Serve
  6×16×4 = 384 B. Non è mai stato osservato accadere, ma smentisce l'idea che la heap
  sia "RAM inutilizzata": **è usata davvero**.
- `ES_MCU_MEM_REMAP_OFFSET` (`user_eeprom.c:59`) legge `REALBASE` da `SYSCFG->REMAP`
  **a runtime** (bit 12..16, granularità 4 KB): l'app è linkata a `0x0` ma vive
  fisicamente sopra il bootloader, quindi lo spazio vero per il codice è
  `0x1C000 − REALBASE`, **non** i ~112 KB dichiarati altrove in questo file. Il valore
  di `REALBASE` resta ignoto; per misurarlo sul ferro c'è `g_tst_remap_offset`
  (`user_eeprom.c:548`). Nessun problema di spazio è mai stato osservato: la cifra è
  solo **non verificata**, non necessariamente sbagliata.

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
- **Bake del layout da `.vil`**: i **layer** vengono bakeati nell'array `keymaps[]`
  di `keymap.c` (sono il default caricato da `dynamic_keymap_reset()` a EE_CLR/primo
  boot). I **combo**, invece, NON stanno in keymap.c: Vial li tiene in EEPROM e
  `dynamic_keymap_reset()` li **azzera**. Per bakearli come default c'è
  `luma40_keymap_post_init()` in `keymap.c`: gira in `keyboard_post_init_user`
  (in `luma40.c`, via hook weak) **dopo** il reset di `via_init`, riscrive gli slot
  combo **vuoti** con `dynamic_keymap_set_combo()` e ricarica la copia RAM con
  `vial_init()`. Slot editati dall'utente (output≠KC_NO) non vengono sovrascritti.
  Layout corrente (LUMA899.vil): 4 layer; combo `` ` ``+LAlt→MO(3), MO(2)+LCtrl→MO(1)
  (sostituiscono i vecchi `LT()` su layer 0). Tap dance/key override/alt-repeat: stessa
  logica se un giorno servisse bakearli (stesso hook, `dynamic_keymap_set_*`).

## LED: effetti + Layer Diff overlay

- **Effetti RGB: tutti i 50 riattivati** in `keyboard.json` (inclusi reattivi e
  framebuffer). C'era stata una fase in cui erano ridotti a `solid_color` + `pixel_rain`
  per risparmiare (−14,5 KB flash, +352 B RAM), ma con ~40 KB flash liberi si è deciso di
  ripristinarli. Costo misurato del ripristino completo sulla keymap `vial`:
  **flash 73500 → 85580 B (+11,8 KB, restano ~28 KB liberi)** e **RAM heap 2656 → 2272 B
  (−384 B)** per il tracker `RGB_MATRIX_KEYREACTIVE` che gli effetti reattivi riaggiungono.
  Se un domani servisse RAM, ritagliare partendo dai reattivi (`solid_reactive*`,
  `splash*`, `typing_heatmap`, `digital_rain`) è la leva: sono gli unici che costano RAM,
  gli altri costano solo flash. `vialrgb_direct` è interno a VialRGB (non in lista).
- **Layer Diff overlay** (`luma40_layer_diff.c/.h`, cablato in `luma40.c`): quando gli
  effetti LED sono accesi e il layer attivo è ≠ base, lo sfondo si spegne e restano
  accesi **solo** i tasti la cui definizione differisce dal layer 0. Trasparenti esclusi
  (`kc != KC_TRANSPARENT && kc != base`). Ogni tasto acceso è colorato per **tipologia del
  keycode** (non più un colore per layer): numeri=giallo, lettere=bianco, navigazione
  (frecce+Ins/Home/PgUp/Del/End/PgDn)=ciano, simboli/punteggiatura=magenta, tasti F
  (F1–F24)=blu, tutto il resto (wireless/RGB/layer/modificatori)=viola. La classificazione (`luma40_classify`)
  sfrutta i range di keycode QMK contigui ed è **dinamica**: legge il keycode reale via
  `keymap_key_to_keycode`, quindi segue i cambi di layout (keymap.c o Vial); layer-tap/
  mod-tap e custom keycode cadono in "altro". Array di 47 byte (una categoria per LED, o
  `CAT_NONE`) ricalcolato in `layer_state_set_user` (Approccio "cache"); disegnato nel hook
  indicatori **prima** di `kb_rgb_matrix_indicators_common`, così Caps/connessione/batteria/
  Win-lock restano sopra. La palette è fissa nel firmware (cambio = ricompilare). Colore
  scalato per la luminosità RGB corrente. Costo: +12 byte flash vs la versione per-layer.

## Flash sul ferro

Bootloader: bootmagic tenendo premuto **Tab** (matrice 0,0) all'inserimento del cavo,
oppure il tasto reset fisico sul retro del PCB. Il flash del `.bin` non è ancora stato
testato da questo porting (bootloader `custom`).
