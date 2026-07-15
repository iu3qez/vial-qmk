VIA_ENABLE = yes
VIAL_ENABLE = yes
VIALRGB_ENABLE = yes

# Obbligatoria, non un'ottimizzazione: senza LTO il build con tutti i 50 effetti RGB
# è 85580 B e sfora il limite reale di ~81,4 KB -> firmware che non parte, senza alcun
# errore in compilazione (vedi CLAUDE.md, "Limite di flash reale").
# Con LTO: 76288 B e +464 B di heap. Verificato sul ferro il 15 lug 2026, combo inclusi
# (l'hook weak luma40_keymap_post_init sopravvive alla LTO).
LTO_ENABLE = yes

# Tutte le feature Vial attive (tap dance, combo, key override, alt-repeat,
# QMK Settings) con i tier automatici a 16 slot dettati da EEPROM_SIZE=2048;
# lo spazio EEPROM residuo (~600 byte) va alle macro.
