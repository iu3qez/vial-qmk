# EPOMAKER LUMA40

A customizable 40% (47-key) tri-mode wireless keyboard (USB / 3x Bluetooth / 2.4 GHz).

* Keyboard Maintainer: [CarlosEDP](https://github.com/carlosedp)
* Hardware Supported: EPOMAKER LUMA40 PCB with Eastsoft ES32FS026 microcontroller (Cortex-M0, 128 KB flash, 16 KB RAM)
* Wireless/battery/EEPROM support: [`lib/rdmctmzt_common`](../../../lib/rdmctmzt_common)

## Keymaps

* `vial` — the main keymap, with [Vial](https://get.vial.today) support (VialRGB
  lighting, QMK Settings, 16 combo/key-override/alt-repeat slots, ~770 bytes of
  macro space). Unlock combo: **Tab + Backspace**.
* `default` — plain QMK keymap without VIA/Vial (VIA-only builds are not supported
  by the vial-qmk tree).

## Build

No local toolchain is needed; use the official QMK docker image from the repo root:

    docker run --rm -v "$PWD:/qmk_firmware" -w /qmk_firmware ghcr.io/qmk/qmk_cli:latest \
        make SKIP_GIT=yes epomaker/luma40:vial

With a local build environment the usual command works too:

    make epomaker/luma40:vial

Requirement: the `lib/chibios-contrib` submodule must be at commit `16782528` or
later (it provides the ES32/FS026 platform).

## Custom keycodes

Wireless mode/pairing, battery query, Win-lock, NKRO toggle etc. are exposed as
custom keycodes (see `Custom_Keycodes` in `lib/rdmctmzt_common/rdmctmzt_common.h`).
In Vial they appear under the "User" tab; their order in `keymaps/vial/vial.json`
follows that enum, positionally mapped from `QK_KB`.

Note: `VIA_Mapping_Luma40.JSON` in this directory is a leftover from the old
blob-based vendor firmware and does not match the current keycode layout.

## Bootloader

Enter the bootloader in 2 ways:

* **Bootmagic reset**: Hold down the key at (0,0) in the matrix (the Tab key) and plug in the keyboard
* **Physical reset button**: Briefly press the button on the back of the PCB
