/* Copyright 2026 Simone Fabris
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#define VIAL_KEYBOARD_UID {0xB3, 0x52, 0xF8, 0xA7, 0x49, 0x3B, 0x36, 0xD0}

/* Unlock combo: Tab (0,0) + Backspace (0,11) held together */
#define VIAL_UNLOCK_COMBO_ROWS {0, 0}
#define VIAL_UNLOCK_COMBO_COLS {0, 11}

/* Con EEPROM_SIZE 2048 i tier automatici di vial.h assegnerebbero 16 entry a
 * ciascuna feature; le riduciamo a 12 per lasciare piu' margine alle macro
 * (~144 byte recuperati, macro da ~440 a ~580 byte). */
#define VIAL_TAP_DANCE_ENTRIES 12
#define VIAL_COMBO_ENTRIES 12
#define VIAL_KEY_OVERRIDE_ENTRIES 12
#define VIAL_ALT_REPEAT_KEY_ENTRIES 12
