# HANDOFF — CSTC40 single_pcb, keymap `luma899` (WIP)

Stato al 2026-07-08. Si prosegue **su Arch** (dove Bash gira in locale e si può
sondare la tastiera direttamente — cosa impossibile finora perché la sessione
girava su WSL2 e la tastiera è sulla macchina Arch).

## TL;DR

- Tastiera: **KPRepublic CSTC40 single_pcb** (NON il P48s — quello è un porting
  separato, branch `xtips-p48s`, MCU STM32F072). Questa è un **STM32F4 clone**
  (flash 256 KB, family UF2 `0x57755A57`), bootloader **TinyUF2 "FXTwink"**
  (mass-storage drag-drop, app a **`0x08010000`**).
- Firmware costruito: keymap `luma899` = layout **LUMA899** (dal Luma40) su
  `LAYOUT_planck_mit` (mappa 1:1, spazio 2u), keycode wireless → `KC_NO`,
  seeding combo, overlay **Layer Diff**, **reset EEPROM one-shot** al primo boot.
- Build come `.uf2`: `BOOTLOADER=tinyuf2` + `MCU_LDSCRIPT=STM32F401xC_tinyuf2`.
  Offset (`0x08010000`) e family (`0x57755A57`) **verificati** = combaciano col FXTwink.
- **Flashato. Il firmware GIRA**: la sua interfaccia Vial (raw HID usage `0xFF60`)
  è presente (`/dev/hidraw1`). **Ma Vial non si connette.**

## BLOCCO ATTUALE

Vial non si collega. udev era `crw------- root` (solo root) senza regola →
aggiunta regola uaccess → **permessi sistemati MA Vial ancora non connette.**
Quindi **NON è (solo) un problema di permessi.**

## Evidenza raccolta (confermata)

- `lsusb`: `586a:0040 kprepublic cstc40`, serial `vial:f64c2b3c`, 3 interfacce HID
  (Keyboard + 2× raw HID).
- `/dev/hidraw1` = interfaccia Vial (report descriptor contiene `06 60 FF` =
  usage page `0xFF60`). Le altre hidraw della cstc40 (0,2) non ce l'hanno.
- Regola udev aggiunta in `/etc/udev/rules.d/99-vial.rules`:
  `KERNEL=="hidraw*", SUBSYSTEM=="hidraw", ATTRS{serial}=="*vial:f64c2b3c*", MODE="0660", GROUP="users", TAG+="uaccess"`
- Il firmware espone **VIAL_PROTOCOL_VERSION = 6**.
- UID atteso (da `config.h`): `8A 52 5A BE 1D 82 69 72`.

## IPOTESI PRINCIPALE per domani

Permessi ora ok ma Vial non connette → **probabile che la versione dell'app Vial
sia troppo vecchia per il protocollo v6** (oppure è la web e WebHID non vede il
device). Il probe qui sotto lo dirime in 10 secondi.

## PROSSIMI PASSI (in ordine)

### 1. Verificare che i permessi siano DAVVERO applicati
```bash
ls -l /dev/hidraw1                         # deve NON essere root-only
udevadm test $(udevadm info -q path -n /dev/hidraw1) 2>&1 | grep -iE "uaccess|MODE|GROUP"
```
Se ancora root-only → la regola non matcha (controllare l'attributo `serial`:
`udevadm info -a /dev/hidraw1 | grep -i serial`).

### 2. TEST DECISIVO — parlare al protocollo Vial direttamente (isola firmware da app)
Con `sudo` bypassa i permessi. Se risponde → **firmware OK al 100%**, colpa dell'app.
```python
#!/usr/bin/env python3
# sudo python3 probe.py
import os, sys, glob
def find_vial():
    for p in sorted(glob.glob('/dev/hidraw*')):
        n=os.path.basename(p)
        try: rd=open(f'/sys/class/hidraw/{n}/device/report_descriptor','rb').read()
        except Exception: continue
        if b'\x06\x60\xff' in rd: return p
    return None
p=find_vial()
if not p: print("interfaccia Vial 0xFF60 non trovata"); sys.exit(1)
print("Vial hidraw:", p)
fd=os.open(p, os.O_RDWR)
os.write(fd, bytes([0x00, 0xFE, 0x00] + [0]*30))   # report-id 0 + 0xFE prefix + 0x00 get_keyboard_id
r=os.read(fd, 32)
ver=int.from_bytes(r[0:4],'little')
print("protocol version:", hex(ver), "| UID:", r[4:12].hex())
print("=> FIRMWARE VIAL RISPONDE (ok)" if ver==6 else "=> risposta inattesa / firmware ko")
os.close(fd)
```
- Risponde `version 0x6` + UID `8a525abe1d826972` → firmware perfetto, problema = **app Vial**.
- Non risponde → problema firmware/protocollo (scavare lì).

### 3. Versione app Vial / WebHID
- App desktop Vial: **aggiornare all'ultima** (le vecchie non parlano protocol v6).
- Oppure **https://vial.rocks** in Chrome (WebHID). Su Linux WebHID vuole comunque
  la hidraw accessibile all'utente (la regola udev di sopra).

### 4. dmesg
```bash
sudo dmesg | tail -40    # errori HID / disconnect / re-enumerazione
```

## Build & flash (promemoria)

```bash
# rebuild (da WSL2 con docker, o nativamente con toolchain arm)
docker run --rm -v "$PWD:/qmk_firmware" -w /qmk_firmware ghcr.io/qmk/qmk_cli:latest \
    make SKIP_GIT=yes kprepublic/cstc40/single_pcb:luma899
# -> kprepublic_cstc40_single_pcb_luma899.uf2 in root
```
Flash: trascina il `.uf2` sul disco USB del bootloader FXTwink (tieni il tasto
bootloader / doppio reset per entrarci).
**Rete di sicurezza**: se serve tornare indietro, ritrascina `CURRENT.UF2`
(dump originale, l'utente ce l'ha) — il bootloader sta a `0x08000000`, intatto.

## Dettagli firmware (cosa è stato messo nel keymap)

- 4 layer LUMA899 (`keymap.c`). Layer 0 = QWERTY (identico allo stock cstc40:
  per questo "sembrava non cambiato"). Le differenze sono nei layer 2/3 e nei combo.
- Combo seminati a primo boot: `` ` ``+LAlt→MO(3), MO(2)+LCtrl→MO(1).
- Overlay Layer Diff (`layer_diff.c/.h`, portato dal Luma40): su layer ≠ 0 sfondo
  spento + tasti-diff colorati per categoria. Cablato via
  `rgb_matrix_indicators_advanced_user` + `layer_state_set_user`.
- Reset EEPROM one-shot (`keyboard_post_init_user`, marker `LUMA899_EE_MAGIC`):
  al primo boot dopo il flash azzera e ricarica i default (il flash UF2 non tocca
  la regione EEPROM, quindi senza questo restava la keymap vecchia).
