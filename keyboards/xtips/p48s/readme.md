# X.Tips P48s

A 48-key (4x12) ortholinear keyboard by X.Tips.

* Keyboard Maintainer: [X.Tips](https://github.com/X-Tips)
* Hardware Supported: X.Tips P48s (STM32F072), revisions v1 and v2
* Hardware Availability: www.umux.com

Two hardware revisions are supported:

* **v1** — classic COL2ROW diode matrix
* **v2** — direct-pin matrix (default)

Make example for this keyboard (after setting up your build environment):

    make xtips/p48s/v2:vial
    make xtips/p48s/v1:vial
    make xtips/p48s/v2:default

Flashing example for this keyboard:

    make xtips/p48s/v2:vial:flash

## Bootloader

Enter the bootloader in one of two ways:

* **Bootmagic**: hold the top-left key (`Q`, matrix `0,0`) while plugging in the keyboard.
* **Physical reset**: hold the RESET switch, then hold the BOOT switch, release RESET, release BOOT.

See the [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) and the
[make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information.
