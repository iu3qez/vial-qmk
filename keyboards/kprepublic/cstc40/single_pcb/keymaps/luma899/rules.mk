VIA_ENABLE = yes
VIAL_ENABLE = yes
VIALRGB_ENABLE = yes

# Overlay Layer Diff (portato dal Luma40)
SRC += layer_diff.c

# Questa board monta il bootloader TinyUF2 (app @ 0x08010000), NON stm32-dfu:
# build in formato .uf2 da trascinare sul disco del bootloader.
BOOTLOADER = tinyuf2
MCU_LDSCRIPT = STM32F401xC_tinyuf2
