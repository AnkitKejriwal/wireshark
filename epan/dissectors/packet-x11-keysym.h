/* packet-x11-keysym.h
 *
 * $Id$
 *
 * Put here so as to make packet-x11.c lighter. See packet-x11.c
 *
 * This file is also used by packet-vnc.c
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __PACKET_X11_KEYSYM_H__
#define __PACKET_X11_KEYSYM_H__

static const value_string keysym_vals_source[] = {
      { 0, "NoSymbol" }, { 0x020, "space" }, { 0x021, "!" }, { 0x022, "\"" },
      { 0x023, "#" }, { 0x024, "$" }, { 0x025, "%" }, { 0x026, "&" },
      { 0x027, "'" }, { 0x028, "(" }, { 0x029, ")" }, { 0x02a, "*" },
      { 0x02b, "+" }, { 0x02c, "," }, { 0x02d, "-" }, { 0x02e, "." },
      { 0x02f, "/" }, { 0x030, "0" }, { 0x031, "1" }, { 0x032, "2" },
      { 0x033, "3" }, { 0x034, "4" }, { 0x035, "5" }, { 0x036, "6" },
      { 0x037, "7" }, { 0x038, "8" }, { 0x039, "9" }, { 0x03a, ":" },
      { 0x03b, ";" }, { 0x03c, "<" }, { 0x03d, "=" }, { 0x03e, ">" },
      { 0x03f, "?" }, { 0x040, "@" }, { 0x041, "A" }, { 0x042, "B" },
      { 0x043, "C" }, { 0x044, "D" }, { 0x045, "E" }, { 0x046, "F" },
      { 0x047, "G" }, { 0x048, "H" }, { 0x049, "I" }, { 0x04a, "J" },
      { 0x04b, "K" }, { 0x04c, "L" }, { 0x04d, "M" }, { 0x04e, "N" },
      { 0x04f, "O" }, { 0x050, "P" }, { 0x051, "Q" }, { 0x052, "R" },
      { 0x053, "S" }, { 0x054, "T" }, { 0x055, "U" }, { 0x056, "V" },
      { 0x057, "W" }, { 0x058, "X" }, { 0x059, "Y" }, { 0x05a, "Z" },
      { 0x05b, "[" }, { 0x05c, "\\" }, { 0x05d, "]" }, { 0x05e, "^" },
      { 0x05f, "_" }, { 0x060, "`" }, { 0x061, "a" }, { 0x062, "b" },
      { 0x063, "c" }, { 0x064, "d" }, { 0x065, "e" }, { 0x066, "f" },
      { 0x067, "g" }, { 0x068, "h" }, { 0x069, "i" }, { 0x06a, "j" },
      { 0x06b, "k" }, { 0x06c, "l" }, { 0x06d, "m" }, { 0x06e, "n" },
      { 0x06f, "o" }, { 0x070, "p" }, { 0x071, "q" }, { 0x072, "r" },
      { 0x073, "s" }, { 0x074, "t" }, { 0x075, "u" }, { 0x076, "v" },
      { 0x077, "w" }, { 0x078, "x" }, { 0x079, "y" }, { 0x07a, "z" },
      { 0x07b, "{" }, { 0x07c, "|" }, { 0x07d, ", HFILL }" }, { 0x07e, "~" },
      { 0x0a0, "nobreakspace" }, { 0x0a1, "\x0a1" }, { 0x0a2, "\x0a2" }, { 0x0a3, "\x0a3" },
      { 0x0a4, "\x0a4" }, { 0x0a5, "\x0a5" }, { 0x0a6, "\x0a6" }, { 0x0a7, "\x0a7" },
      { 0x0a8, "\x0a8" }, { 0x0a9, "\x0a9" }, { 0x0aa, "\x0aa" }, { 0x0ab, "\x0ab" },
      { 0x0ac, "\x0ac" }, { 0x0ad, "\x0ad" }, { 0x0ae, "\x0ae" }, { 0x0af, "\x0af" },
      { 0x0b0, "\x0b0" }, { 0x0b1, "\x0b1" }, { 0x0b2, "\x0b2" }, { 0x0b3, "\x0b3" },
      { 0x0b4, "\x0b4" }, { 0x0b5, "\x0b5" }, { 0x0b6, "\x0b6" }, { 0x0b7, "\x0b7" },
      { 0x0b8, "\x0b8" }, { 0x0b9, "\x0b9" }, { 0x0ba, "\x0ba" }, { 0x0bb, "\x0bb" },
      { 0x0bc, "\x0bc" }, { 0x0bd, "\x0bd" }, { 0x0be, "\x0be" }, { 0x0bf, "\x0bf" },
      { 0x0c0, "\x0c0" }, { 0x0c1, "\x0c1" }, { 0x0c2, "\x0c2" }, { 0x0c3, "\x0c3" },
      { 0x0c4, "\x0c4" }, { 0x0c5, "\x0c5" }, { 0x0c6, "\x0c6" }, { 0x0c7, "\x0c7" },
      { 0x0c8, "\x0c8" }, { 0x0c9, "\x0c9" }, { 0x0ca, "\x0ca" }, { 0x0cb, "\x0cb" },
      { 0x0cc, "\x0cc" }, { 0x0cd, "\x0cd" }, { 0x0ce, "\x0ce" }, { 0x0cf, "\x0cf" },

      { 0x0d0, "\x0d0" }, { 0x0d1, "\x0d1" }, { 0x0d2, "\x0d2" }, { 0x0d3, "\x0d3" },
      { 0x0d4, "\x0d4" }, { 0x0d5, "\x0d5" }, { 0x0d6, "\x0d6" }, { 0x0d7, "\x0d7" },
      { 0x0d8, "\x0d8" }, { 0x0d9, "\x0d9" }, { 0x0da, "\x0da" }, { 0x0db, "\x0db" },
      { 0x0dc, "\x0dc" }, { 0x0dd, "\x0dd" }, { 0x0de, "\x0de" }, { 0x0df, "\x0df" },
      { 0x0e0, "\x0e0" }, { 0x0e1, "\x0e1" }, { 0x0e2, "\x0e2" }, { 0x0e3, "\x0e3" },
      { 0x0e4, "\x0e4" }, { 0x0e5, "\x0e5" }, { 0x0e6, "\x0e6" }, { 0x0e7, "\x0e7" },
      { 0x0e8, "\x0e8" }, { 0x0e9, "\x0e9" }, { 0x0ea, "\x0ea" }, { 0x0eb, "\x0eb" },
      { 0x0ec, "\x0ec" }, { 0x0ed, "\x0ed" }, { 0x0ee, "\x0ee" }, { 0x0ef, "\x0ef" },
      { 0x0f0, "\x0f0" }, { 0x0f1, "\x0f1" }, { 0x0f2, "\x0f2" }, { 0x0f3, "\x0f3" },
      { 0x0f4, "\x0f4" }, { 0x0f5, "\x0f5" }, { 0x0f6, "\x0f6" }, { 0x0f7, "\x0f7" },
      { 0x0f8, "\x0f8" }, { 0x0f9, "\x0f9" }, { 0x0fa, "\x0fa" }, { 0x0fb, "\x0fb" },
      { 0x0fc, "\x0fc" }, { 0x0fd, "\x0fd" }, { 0x0fe, "\x0fe" }, { 0x0ff, "\x0ff" },
      { 0x13bc, "OE" }, { 0x13bd, "oe" }, { 0x13be, "Ydiaeresis" },
      { 0x1a1, "Aogonek" }, { 0x1a2, "breve" }, { 0x1a3, "Lstroke" }, { 0x1a5, "Lcaron" },
      { 0x1a6, "Sacute" }, { 0x1a9, "Scaron" }, { 0x1aa, "Scedilla" }, { 0x1ab, "Tcaron" },
      { 0x1ac, "Zacute" }, { 0x1ae, "Zcaron" }, { 0x1af, "Zabovedot" }, { 0x1b1, "aogonek" },
      { 0x1b2, "ogonek" }, { 0x1b3, "lstroke" }, { 0x1b5, "lcaron" }, { 0x1b6, "sacute" },
      { 0x1b7, "caron" }, { 0x1b9, "scaron" }, { 0x1ba, "scedilla" }, { 0x1bb, "tcaron" },
      { 0x1bc, "zacute" }, { 0x1bd, "doubleacute" }, { 0x1be, "zcaron" }, { 0x1bf, "zabovedot" },
      { 0x1c0, "Racute" }, { 0x1c3, "Abreve" }, { 0x1c5, "Lacute" }, { 0x1c6, "Cacute" },
      { 0x1c8, "Ccaron" }, { 0x1ca, "Eogonek" }, { 0x1cc, "Ecaron" }, { 0x1cf, "Dcaron" },
      { 0x1d0, "Dstroke" }, { 0x1d1, "Nacute" }, { 0x1d2, "Ncaron" }, { 0x1d5, "Odoubleacute" },
      { 0x1d8, "Rcaron" }, { 0x1d9, "Uring" }, { 0x1db, "Udoubleacute" }, { 0x1de, "Tcedilla" },
      { 0x1e0, "racute" }, { 0x1e3, "abreve" }, { 0x1e5, "lacute" }, { 0x1e6, "cacute" },
      { 0x1e8, "ccaron" }, { 0x1ea, "eogonek" }, { 0x1ec, "ecaron" }, { 0x1ef, "dcaron" },
      { 0x1f0, "dstroke" }, { 0x1f1, "nacute" }, { 0x1f2, "ncaron" }, { 0x1f5, "odoubleacute" },
      { 0x1f8, "rcaron" }, { 0x1f9, "uring" }, { 0x1fb, "udoubleacute" }, { 0x1fe, "tcedilla" },
      { 0x1ff, "abovedot" }, { 0x20a0, "EcuSign" }, { 0x20a1, "ColonSign" }, { 0x20a2, "CruzeiroSign" },
      { 0x20a3, "FFrancSign" }, { 0x20a4, "LiraSign" }, { 0x20a5, "MillSign" }, { 0x20a6, "NairaSign" },
      { 0x20a7, "PesetaSign" }, { 0x20a8, "RupeeSign" }, { 0x20a9, "WonSign" }, { 0x20aa, "NewSheqelSign" },
      { 0x20ab, "DongSign" }, { 0x20ac, "EuroSign" }, { 0x2a1, "Hstroke" }, { 0x2a6, "Hcircumflex" },
      { 0x2a9, "Iabovedot" }, { 0x2ab, "Gbreve" }, { 0x2ac, "Jcircumflex" }, { 0x2b1, "hstroke" },
      { 0x2b6, "hcircumflex" }, { 0x2b9, "idotless" }, { 0x2bb, "gbreve" }, { 0x2bc, "jcircumflex" },
      { 0x2c5, "Cabovedot" }, { 0x2c6, "Ccircumflex" }, { 0x2d5, "Gabovedot" }, { 0x2d8, "Gcircumflex" },
      { 0x2dd, "Ubreve" }, { 0x2de, "Scircumflex" }, { 0x2e5, "cabovedot" }, { 0x2e6, "ccircumflex" },
      { 0x2f5, "gabovedot" }, { 0x2f8, "gcircumflex" }, { 0x2fd, "ubreve" }, { 0x2fe, "scircumflex" },
      { 0x3a2, "kappa" }, { 0x3a2, "kra" }, { 0x3a3, "Rcedilla" }, { 0x3a5, "Itilde" },
      { 0x3a6, "Lcedilla" }, { 0x3aa, "Emacron" }, { 0x3ab, "Gcedilla" }, { 0x3ac, "Tslash" },
      { 0x3b3, "rcedilla" }, { 0x3b5, "itilde" }, { 0x3b6, "lcedilla" }, { 0x3ba, "emacron" },
      { 0x3bb, "gcedilla" }, { 0x3bc, "tslash" }, { 0x3bd, "ENG" }, { 0x3bf, "eng" },
      { 0x3c0, "Amacron" }, { 0x3c7, "Iogonek" }, { 0x3cc, "Eabovedot" }, { 0x3cf, "Imacron" },
      { 0x3d1, "Ncedilla" }, { 0x3d2, "Omacron" }, { 0x3d3, "Kcedilla" }, { 0x3d9, "Uogonek" },
      { 0x3dd, "Utilde" }, { 0x3de, "Umacron" }, { 0x3e0, "amacron" }, { 0x3e7, "iogonek" },
      { 0x3ec, "eabovedot" }, { 0x3ef, "imacron" }, { 0x3f1, "ncedilla" }, { 0x3f2, "omacron" },
      { 0x3f3, "kcedilla" }, { 0x3f9, "uogonek" }, { 0x3fd, "utilde" }, { 0x3fe, "umacron" },
      { 0x47e, "overline" }, { 0x4a1, "kana_fullstop" }, { 0x4a2, "kana_openingbracket" }, { 0x4a3, "kana_closingbracket" },
      { 0x4a4, "kana_comma" }, { 0x4a5, "kana_conjunctive" }, { 0x4a5, "kana_middledot" }, { 0x4a6, "kana_WO" },
      { 0x4a7, "kana_a" }, { 0x4a8, "kana_i" }, { 0x4a9, "kana_u" }, { 0x4aa, "kana_e" },
      { 0x4ab, "kana_o" }, { 0x4ac, "kana_ya" }, { 0x4ad, "kana_yu" }, { 0x4ae, "kana_yo" },
      { 0x4af, "kana_tsu" }, { 0x4af, "kana_tu" }, { 0x4b0, "prolongedsound" }, { 0x4b1, "kana_A" },
      { 0x4b2, "kana_I" }, { 0x4b3, "kana_U" }, { 0x4b4, "kana_E" }, { 0x4b5, "kana_O" },
      { 0x4b6, "kana_KA" }, { 0x4b7, "kana_KI" }, { 0x4b8, "kana_KU" }, { 0x4b9, "kana_KE" },
      { 0x4ba, "kana_KO" }, { 0x4bb, "kana_SA" }, { 0x4bc, "kana_SHI" }, { 0x4bd, "kana_SU" },
      { 0x4be, "kana_SE" }, { 0x4bf, "kana_SO" }, { 0x4c0, "kana_TA" }, { 0x4c1, "kana_CHI" },
      { 0x4c1, "kana_TI" }, { 0x4c2, "kana_TSU" }, { 0x4c2, "kana_TU" }, { 0x4c3, "kana_TE" },
      { 0x4c4, "kana_TO" }, { 0x4c5, "kana_NA" }, { 0x4c6, "kana_NI" }, { 0x4c7, "kana_NU" },
      { 0x4c8, "kana_NE" }, { 0x4c9, "kana_NO" }, { 0x4ca, "kana_HA" }, { 0x4cb, "kana_HI" },
      { 0x4cc, "kana_FU" }, { 0x4cc, "kana_HU" }, { 0x4cd, "kana_HE" }, { 0x4ce, "kana_HO" },
      { 0x4cf, "kana_MA" }, { 0x4d0, "kana_MI" }, { 0x4d1, "kana_MU" }, { 0x4d2, "kana_ME" },
      { 0x4d3, "kana_MO" }, { 0x4d4, "kana_YA" }, { 0x4d5, "kana_YU" }, { 0x4d6, "kana_YO" },
      { 0x4d7, "kana_RA" }, { 0x4d8, "kana_RI" }, { 0x4d9, "kana_RU" }, { 0x4da, "kana_RE" },
      { 0x4db, "kana_RO" }, { 0x4dc, "kana_WA" }, { 0x4dd, "kana_N" }, { 0x4de, "voicedsound" },
      { 0x4df, "semivoicedsound" }, { 0x5ac, "Arabic_comma" }, { 0x5bb, "Arabic_semicolon" }, { 0x5bf, "Arabic_question_mark" },
      { 0x5c1, "Arabic_hamza" }, { 0x5c2, "Arabic_maddaonalef" }, { 0x5c3, "Arabic_hamzaonalef" }, { 0x5c4, "Arabic_hamzaonwaw" },
      { 0x5c5, "Arabic_hamzaunderalef" }, { 0x5c6, "Arabic_hamzaonyeh" }, { 0x5c7, "Arabic_alef" }, { 0x5c8, "Arabic_beh" },
      { 0x5c9, "Arabic_tehmarbuta" }, { 0x5ca, "Arabic_teh" }, { 0x5cb, "Arabic_theh" }, { 0x5cc, "Arabic_jeem" },
      { 0x5cd, "Arabic_hah" }, { 0x5ce, "Arabic_khah" }, { 0x5cf, "Arabic_dal" }, { 0x5d0, "Arabic_thal" },
      { 0x5d1, "Arabic_ra" }, { 0x5d2, "Arabic_zain" }, { 0x5d3, "Arabic_seen" }, { 0x5d4, "Arabic_sheen" },
      { 0x5d5, "Arabic_sad" }, { 0x5d6, "Arabic_dad" }, { 0x5d7, "Arabic_tah" }, { 0x5d8, "Arabic_zah" },
      { 0x5d9, "Arabic_ain" }, { 0x5da, "Arabic_ghain" }, { 0x5e0, "Arabic_tatweel" }, { 0x5e1, "Arabic_feh" },
      { 0x5e2, "Arabic_qaf" }, { 0x5e3, "Arabic_kaf" }, { 0x5e4, "Arabic_lam" }, { 0x5e5, "Arabic_meem" },
      { 0x5e6, "Arabic_noon" }, { 0x5e7, "Arabic_ha" }, { 0x5e7, "Arabic_heh" }, { 0x5e8, "Arabic_waw" },
      { 0x5e9, "Arabic_alefmaksura" }, { 0x5ea, "Arabic_yeh" }, { 0x5eb, "Arabic_fathatan" }, { 0x5ec, "Arabic_dammatan" },
      { 0x5ed, "Arabic_kasratan" }, { 0x5ee, "Arabic_fatha" }, { 0x5ef, "Arabic_damma" }, { 0x5f0, "Arabic_kasra" },
      { 0x5f1, "Arabic_shadda" }, { 0x5f2, "Arabic_sukun" }, { 0x6a1, "Serbian_dje" }, { 0x6a2, "Macedonia_gje" },
      { 0x6a3, "Cyrillic_io" }, { 0x6a4, "Ukrainian_ie" }, { 0x6a4, "Ukranian_je" }, { 0x6a5, "Macedonia_dse" },
      { 0x6a6, "Ukrainian_i" }, { 0x6a6, "Ukranian_i" }, { 0x6a7, "Ukrainian_yi" }, { 0x6a7, "Ukranian_yi" },
      { 0x6a8, "Cyrillic_je" }, { 0x6a8, "Serbian_je" }, { 0x6a9, "Cyrillic_lje" }, { 0x6a9, "Serbian_lje" },
      { 0x6aa, "Cyrillic_nje" }, { 0x6aa, "Serbian_nje" }, { 0x6ab, "Serbian_tshe" }, { 0x6ac, "Macedonia_kje" },
      { 0x6ae, "Byelorussian_shortu" }, { 0x6af, "Cyrillic_dzhe" }, { 0x6af, "Serbian_dze" }, { 0x6b0, "numerosign" },
      { 0x6b1, "Serbian_DJE" }, { 0x6b2, "Macedonia_GJE" }, { 0x6b3, "Cyrillic_IO" }, { 0x6b4, "Ukrainian_IE" },
      { 0x6b4, "Ukranian_JE" }, { 0x6b5, "Macedonia_DSE" }, { 0x6b6, "Ukrainian_I" }, { 0x6b6, "Ukranian_I" },
      { 0x6b7, "Ukrainian_YI" }, { 0x6b7, "Ukranian_YI" }, { 0x6b8, "Cyrillic_JE" }, { 0x6b8, "Serbian_JE" },
      { 0x6b9, "Cyrillic_LJE" }, { 0x6b9, "Serbian_LJE" }, { 0x6ba, "Cyrillic_NJE" }, { 0x6ba, "Serbian_NJE" },
      { 0x6bb, "Serbian_TSHE" }, { 0x6bc, "Macedonia_KJE" }, { 0x6be, "Byelorussian_SHORTU" }, { 0x6bf, "Cyrillic_DZHE" },
      { 0x6bf, "Serbian_DZE" }, { 0x6c0, "Cyrillic_yu" }, { 0x6c1, "Cyrillic_a" }, { 0x6c2, "Cyrillic_be" },
      { 0x6c3, "Cyrillic_tse" }, { 0x6c4, "Cyrillic_de" }, { 0x6c5, "Cyrillic_ie" }, { 0x6c6, "Cyrillic_ef" },
      { 0x6c7, "Cyrillic_ghe" }, { 0x6c8, "Cyrillic_ha" }, { 0x6c9, "Cyrillic_i" }, { 0x6ca, "Cyrillic_shorti" },
      { 0x6cb, "Cyrillic_ka" }, { 0x6cc, "Cyrillic_el" }, { 0x6cd, "Cyrillic_em" }, { 0x6ce, "Cyrillic_en" },
      { 0x6cf, "Cyrillic_o" }, { 0x6d0, "Cyrillic_pe" }, { 0x6d1, "Cyrillic_ya" }, { 0x6d2, "Cyrillic_er" },
      { 0x6d3, "Cyrillic_es" }, { 0x6d4, "Cyrillic_te" }, { 0x6d5, "Cyrillic_u" }, { 0x6d6, "Cyrillic_zhe" },
      { 0x6d7, "Cyrillic_ve" }, { 0x6d8, "Cyrillic_softsign" }, { 0x6d9, "Cyrillic_yeru" }, { 0x6da, "Cyrillic_ze" },
      { 0x6db, "Cyrillic_sha" }, { 0x6dc, "Cyrillic_e" }, { 0x6dd, "Cyrillic_shcha" }, { 0x6de, "Cyrillic_che" },
      { 0x6df, "Cyrillic_hardsign" }, { 0x6e0, "Cyrillic_YU" }, { 0x6e1, "Cyrillic_A" }, { 0x6e2, "Cyrillic_BE" },
      { 0x6e3, "Cyrillic_TSE" }, { 0x6e4, "Cyrillic_DE" }, { 0x6e5, "Cyrillic_IE" }, { 0x6e6, "Cyrillic_EF" },
      { 0x6e7, "Cyrillic_GHE" }, { 0x6e8, "Cyrillic_HA" }, { 0x6e9, "Cyrillic_I" }, { 0x6ea, "Cyrillic_SHORTI" },
      { 0x6eb, "Cyrillic_KA" }, { 0x6ec, "Cyrillic_EL" }, { 0x6ed, "Cyrillic_EM" }, { 0x6ee, "Cyrillic_EN" },
      { 0x6ef, "Cyrillic_O" }, { 0x6f0, "Cyrillic_PE" }, { 0x6f1, "Cyrillic_YA" }, { 0x6f2, "Cyrillic_ER" },
      { 0x6f3, "Cyrillic_ES" }, { 0x6f4, "Cyrillic_TE" }, { 0x6f5, "Cyrillic_U" }, { 0x6f6, "Cyrillic_ZHE" },
      { 0x6f7, "Cyrillic_VE" }, { 0x6f8, "Cyrillic_SOFTSIGN" }, { 0x6f9, "Cyrillic_YERU" }, { 0x6fa, "Cyrillic_ZE" },
      { 0x6fb, "Cyrillic_SHA" }, { 0x6fc, "Cyrillic_E" }, { 0x6fd, "Cyrillic_SHCHA" }, { 0x6fe, "Cyrillic_CHE" },
      { 0x6ff, "Cyrillic_HARDSIGN" }, { 0x7a1, "Greek_ALPHAaccent" }, { 0x7a2, "Greek_EPSILONaccent" }, { 0x7a3, "Greek_ETAaccent" },
      { 0x7a4, "Greek_IOTAaccent" }, { 0x7a5, "Greek_IOTAdiaeresis" }, { 0x7a7, "Greek_OMICRONaccent" }, { 0x7a8, "Greek_UPSILONaccent" },
      { 0x7a9, "Greek_UPSILONdieresis" }, { 0x7ab, "Greek_OMEGAaccent" }, { 0x7ae, "Greek_accentdieresis" }, { 0x7af, "Greek_horizbar" },
      { 0x7b1, "Greek_alphaaccent" }, { 0x7b2, "Greek_epsilonaccent" }, { 0x7b3, "Greek_etaaccent" }, { 0x7b4, "Greek_iotaaccent" },
      { 0x7b5, "Greek_iotadieresis" }, { 0x7b6, "Greek_iotaaccentdieresis" }, { 0x7b7, "Greek_omicronaccent" }, { 0x7b8, "Greek_upsilonaccent" },
      { 0x7b9, "Greek_upsilondieresis" }, { 0x7ba, "Greek_upsilonaccentdieresis" }, { 0x7bb, "Greek_omegaaccent" }, { 0x7c1, "Greek_ALPHA" },
      { 0x7c2, "Greek_BETA" }, { 0x7c3, "Greek_GAMMA" }, { 0x7c4, "Greek_DELTA" }, { 0x7c5, "Greek_EPSILON" },
      { 0x7c6, "Greek_ZETA" }, { 0x7c7, "Greek_ETA" }, { 0x7c8, "Greek_THETA" }, { 0x7c9, "Greek_IOTA" },
      { 0x7ca, "Greek_KAPPA" }, { 0x7cb, "Greek_LAMBDA" }, { 0x7cb, "Greek_LAMDA" }, { 0x7cc, "Greek_MU" },
      { 0x7cd, "Greek_NU" }, { 0x7ce, "Greek_XI" }, { 0x7cf, "Greek_OMICRON" }, { 0x7d0, "Greek_PI" },
      { 0x7d1, "Greek_RHO" }, { 0x7d2, "Greek_SIGMA" }, { 0x7d4, "Greek_TAU" }, { 0x7d5, "Greek_UPSILON" },
      { 0x7d6, "Greek_PHI" }, { 0x7d7, "Greek_CHI" }, { 0x7d8, "Greek_PSI" }, { 0x7d9, "Greek_OMEGA" },
      { 0x7e1, "Greek_alpha" }, { 0x7e2, "Greek_beta" }, { 0x7e3, "Greek_gamma" }, { 0x7e4, "Greek_delta" },
      { 0x7e5, "Greek_epsilon" }, { 0x7e6, "Greek_zeta" }, { 0x7e7, "Greek_eta" }, { 0x7e8, "Greek_theta" },
      { 0x7e9, "Greek_iota" }, { 0x7ea, "Greek_kappa" }, { 0x7eb, "Greek_lambda" }, { 0x7eb, "Greek_lamda" },
      { 0x7ec, "Greek_mu" }, { 0x7ed, "Greek_nu" }, { 0x7ee, "Greek_xi" }, { 0x7ef, "Greek_omicron" },
      { 0x7f0, "Greek_pi" }, { 0x7f1, "Greek_rho" }, { 0x7f2, "Greek_sigma" }, { 0x7f3, "Greek_finalsmallsigma" },
      { 0x7f4, "Greek_tau" }, { 0x7f5, "Greek_upsilon" }, { 0x7f6, "Greek_phi" }, { 0x7f7, "Greek_chi" },
      { 0x7f8, "Greek_psi" }, { 0x7f9, "Greek_omega" }, { 0x8a1, "leftradical" }, { 0x8a2, "topleftradical" },
      { 0x8a3, "horizconnector" }, { 0x8a4, "topintegral" }, { 0x8a5, "botintegral" }, { 0x8a6, "vertconnector" },
      { 0x8a7, "topleftsqbracket" }, { 0x8a8, "botleftsqbracket" }, { 0x8a9, "toprightsqbracket" }, { 0x8aa, "botrightsqbracket" },
      { 0x8ab, "topleftparens" }, { 0x8ac, "botleftparens" }, { 0x8ad, "toprightparens" }, { 0x8ae, "botrightparens" },
      { 0x8af, "leftmiddlecurlybrace" }, { 0x8b0, "rightmiddlecurlybrace" }, { 0x8b1, "topleftsummation" }, { 0x8b2, "botleftsummation" },
      { 0x8b3, "topvertsummationconnector" }, { 0x8b4, "botvertsummationconnector" }, { 0x8b5, "toprightsummation" }, { 0x8b6, "botrightsummation" },
      { 0x8b7, "rightmiddlesummation" }, { 0x8bc, "lessthanequal" }, { 0x8bd, "notequal" }, { 0x8be, "greaterthanequal" },
      { 0x8bf, "integral" }, { 0x8c0, "therefore" }, { 0x8c1, "variation" }, { 0x8c2, "infinity" },
      { 0x8c5, "nabla" }, { 0x8c8, "approximate" }, { 0x8c9, "similarequal" }, { 0x8cd, "ifonlyif" },
      { 0x8ce, "implies" }, { 0x8cf, "identical" }, { 0x8d6, "radical" }, { 0x8da, "includedin" },
      { 0x8db, "includes" }, { 0x8dc, "intersection" }, { 0x8dd, "union" }, { 0x8de, "logicaland" },
      { 0x8df, "logicalor" }, { 0x8ef, "partialderivative" }, { 0x8f6, "function" }, { 0x8fb, "leftarrow" },
      { 0x8fc, "uparrow" }, { 0x8fd, "rightarrow" }, { 0x8fe, "downarrow" }, { 0x9df, "blank" },
      { 0x9e0, "soliddiamond" }, { 0x9e1, "checkerboard" }, { 0x9e2, "ht" }, { 0x9e3, "ff" },
      { 0x9e4, "cr" }, { 0x9e5, "lf" }, { 0x9e8, "nl" }, { 0x9e9, "vt" },
      { 0x9ea, "lowrightcorner" }, { 0x9eb, "uprightcorner" }, { 0x9ec, "upleftcorner" }, { 0x9ed, "lowleftcorner" },
      { 0x9ee, "crossinglines" }, { 0x9ef, "horizlinescan1" }, { 0x9f0, "horizlinescan3" }, { 0x9f1, "horizlinescan5" },
      { 0x9f2, "horizlinescan7" }, { 0x9f3, "horizlinescan9" }, { 0x9f4, "leftt" }, { 0x9f5, "rightt" },
      { 0x9f6, "bott" }, { 0x9f7, "topt" }, { 0x9f8, "vertbar" }, { 0xaa1, "emspace" },
      { 0xaa2, "enspace" }, { 0xaa3, "em3space" }, { 0xaa4, "em4space" }, { 0xaa5, "digitspace" },
      { 0xaa6, "punctspace" }, { 0xaa7, "thinspace" }, { 0xaa8, "hairspace" }, { 0xaa9, "emdash" },
      { 0xaaa, "endash" }, { 0xaac, "signifblank" }, { 0xaae, "ellipsis" }, { 0xaaf, "doubbaselinedot" },
      { 0xab0, "onethird" }, { 0xab1, "twothirds" }, { 0xab2, "onefifth" }, { 0xab3, "twofifths" },
      { 0xab4, "threefifths" }, { 0xab5, "fourfifths" }, { 0xab6, "onesixth" }, { 0xab7, "fivesixths" },
      { 0xab8, "careof" }, { 0xabb, "figdash" }, { 0xabc, "leftanglebracket" }, { 0xabd, "decimalpoint" },
      { 0xabe, "rightanglebracket" }, { 0xabf, "marker" }, { 0xac3, "oneeighth" }, { 0xac4, "threeeighths" },
      { 0xac5, "fiveeighths" }, { 0xac6, "seveneighths" }, { 0xac9, "trademark" }, { 0xaca, "signaturemark" },
      { 0xacb, "trademarkincircle" }, { 0xacc, "leftopentriangle" }, { 0xacd, "rightopentriangle" }, { 0xace, "emopencircle" },
      { 0xacf, "emopenrectangle" }, { 0xad0, "leftsinglequotemark" }, { 0xad1, "rightsinglequotemark" }, { 0xad2, "leftdoublequotemark" },
      { 0xad3, "rightdoublequotemark" }, { 0xad4, "prescription" }, { 0xad6, "minutes" }, { 0xad7, "seconds" },
      { 0xad9, "latincross" }, { 0xada, "hexagram" }, { 0xadb, "filledrectbullet" }, { 0xadc, "filledlefttribullet" },
      { 0xadd, "filledrighttribullet" }, { 0xade, "emfilledcircle" }, { 0xadf, "emfilledrect" }, { 0xae0, "enopencircbullet" },
      { 0xae1, "enopensquarebullet" }, { 0xae2, "openrectbullet" }, { 0xae3, "opentribulletup" }, { 0xae4, "opentribulletdown" },
      { 0xae5, "openstar" }, { 0xae6, "enfilledcircbullet" }, { 0xae7, "enfilledsqbullet" }, { 0xae8, "filledtribulletup" },
      { 0xae9, "filledtribulletdown" }, { 0xaea, "leftpointer" }, { 0xaeb, "rightpointer" }, { 0xaec, "club" },
      { 0xaed, "diamond" }, { 0xaee, "heart" }, { 0xaf0, "maltesecross" }, { 0xaf1, "dagger" },
      { 0xaf2, "doubledagger" }, { 0xaf3, "checkmark" }, { 0xaf4, "ballotcross" }, { 0xaf5, "musicalsharp" },
      { 0xaf6, "musicalflat" }, { 0xaf7, "malesymbol" }, { 0xaf8, "femalesymbol" }, { 0xaf9, "telephone" },
      { 0xafa, "telephonerecorder" }, { 0xafb, "phonographcopyright" }, { 0xafc, "caret" }, { 0xafd, "singlelowquotemark" },
      { 0xafe, "doublelowquotemark" }, { 0xaff, "cursor" }, { 0xba3, "leftcaret" }, { 0xba6, "rightcaret" },
      { 0xba8, "downcaret" }, { 0xba9, "upcaret" }, { 0xbc0, "overbar" }, { 0xbc2, "downtack" },
      { 0xbc3, "upshoe" }, { 0xbc4, "downstile" }, { 0xbc6, "underbar" }, { 0xbca, "jot" },
      { 0xbcc, "quad" }, { 0xbce, "uptack" }, { 0xbcf, "circle" }, { 0xbd3, "upstile" },
      { 0xbd6, "downshoe" }, { 0xbd8, "rightshoe" }, { 0xbda, "leftshoe" }, { 0xbdc, "lefttack" },
      { 0xbfc, "righttack" }, { 0xcdf, "hebrew_doublelowline" }, { 0xce0, "hebrew_aleph" }, { 0xce1, "hebrew_bet" },
      { 0xce1, "hebrew_beth" }, { 0xce2, "hebrew_gimel" }, { 0xce2, "hebrew_gimmel" }, { 0xce3, "hebrew_dalet" },
      { 0xce3, "hebrew_daleth" }, { 0xce4, "hebrew_he" }, { 0xce5, "hebrew_waw" }, { 0xce6, "hebrew_zain" },
      { 0xce6, "hebrew_zayin" }, { 0xce7, "hebrew_chet" }, { 0xce7, "hebrew_het" }, { 0xce8, "hebrew_tet" },
      { 0xce8, "hebrew_teth" }, { 0xce9, "hebrew_yod" }, { 0xcea, "hebrew_finalkaph" }, { 0xceb, "hebrew_kaph" },
      { 0xcec, "hebrew_lamed" }, { 0xced, "hebrew_finalmem" }, { 0xcee, "hebrew_mem" }, { 0xcef, "hebrew_finalnun" },
      { 0xcf0, "hebrew_nun" }, { 0xcf1, "hebrew_samech" }, { 0xcf1, "hebrew_samekh" }, { 0xcf2, "hebrew_ayin" },
      { 0xcf3, "hebrew_finalpe" }, { 0xcf4, "hebrew_pe" }, { 0xcf5, "hebrew_finalzade" }, { 0xcf5, "hebrew_finalzadi" },
      { 0xcf6, "hebrew_zade" }, { 0xcf6, "hebrew_zadi" }, { 0xcf7, "hebrew_kuf" }, { 0xcf7, "hebrew_qoph" },
      { 0xcf8, "hebrew_resh" }, { 0xcf9, "hebrew_shin" }, { 0xcfa, "hebrew_taf" }, { 0xcfa, "hebrew_taw" },
      { 0xda1, "Thai_kokai" }, { 0xda2, "Thai_khokhai" }, { 0xda3, "Thai_khokhuat" }, { 0xda4, "Thai_khokhwai" },
      { 0xda5, "Thai_khokhon" }, { 0xda6, "Thai_khorakhang" }, { 0xda7, "Thai_ngongu" }, { 0xda8, "Thai_chochan" },
      { 0xda9, "Thai_choching" }, { 0xdaa, "Thai_chochang" }, { 0xdab, "Thai_soso" }, { 0xdac, "Thai_chochoe" },
      { 0xdad, "Thai_yoying" }, { 0xdae, "Thai_dochada" }, { 0xdaf, "Thai_topatak" }, { 0xdb0, "Thai_thothan" },
      { 0xdb1, "Thai_thonangmontho" }, { 0xdb2, "Thai_thophuthao" }, { 0xdb3, "Thai_nonen" }, { 0xdb4, "Thai_dodek" },
      { 0xdb5, "Thai_totao" }, { 0xdb6, "Thai_thothung" }, { 0xdb7, "Thai_thothahan" }, { 0xdb8, "Thai_thothong" },
      { 0xdb9, "Thai_nonu" }, { 0xdba, "Thai_bobaimai" }, { 0xdbb, "Thai_popla" }, { 0xdbc, "Thai_phophung" },
      { 0xdbd, "Thai_fofa" }, { 0xdbe, "Thai_phophan" }, { 0xdbf, "Thai_fofan" }, { 0xdc0, "Thai_phosamphao" },
      { 0xdc1, "Thai_moma" }, { 0xdc2, "Thai_yoyak" }, { 0xdc3, "Thai_rorua" }, { 0xdc4, "Thai_ru" },
      { 0xdc5, "Thai_loling" }, { 0xdc6, "Thai_lu" }, { 0xdc7, "Thai_wowaen" }, { 0xdc8, "Thai_sosala" },
      { 0xdc9, "Thai_sorusi" }, { 0xdca, "Thai_sosua" }, { 0xdcb, "Thai_hohip" }, { 0xdcc, "Thai_lochula" },
      { 0xdcd, "Thai_oang" }, { 0xdce, "Thai_honokhuk" }, { 0xdcf, "Thai_paiyannoi" }, { 0xdd0, "Thai_saraa" },
      { 0xdd1, "Thai_maihanakat" }, { 0xdd2, "Thai_saraaa" }, { 0xdd3, "Thai_saraam" }, { 0xdd4, "Thai_sarai" },
      { 0xdd5, "Thai_saraii" }, { 0xdd6, "Thai_saraue" }, { 0xdd7, "Thai_sarauee" }, { 0xdd8, "Thai_sarau" },
      { 0xdd9, "Thai_sarauu" }, { 0xdda, "Thai_phinthu" }, { 0xdde, "Thai_maihanakat_maitho" }, { 0xddf, "Thai_baht" },
      { 0xde0, "Thai_sarae" }, { 0xde1, "Thai_saraae" }, { 0xde2, "Thai_sarao" }, { 0xde3, "Thai_saraaimaimuan" },
      { 0xde4, "Thai_saraaimaimalai" }, { 0xde5, "Thai_lakkhangyao" }, { 0xde6, "Thai_maiyamok" }, { 0xde7, "Thai_maitaikhu" },
      { 0xde8, "Thai_maiek" }, { 0xde9, "Thai_maitho" }, { 0xdea, "Thai_maitri" }, { 0xdeb, "Thai_maichattawa" },
      { 0xdec, "Thai_thanthakhat" }, { 0xded, "Thai_nikhahit" }, { 0xdf0, "Thai_leksun" }, { 0xdf1, "Thai_leknung" },
      { 0xdf2, "Thai_leksong" }, { 0xdf3, "Thai_leksam" }, { 0xdf4, "Thai_leksi" }, { 0xdf5, "Thai_lekha" },
      { 0xdf6, "Thai_lekhok" }, { 0xdf7, "Thai_lekchet" }, { 0xdf8, "Thai_lekpaet" }, { 0xdf9, "Thai_lekkao" },
      { 0xea1, "Hangul_Kiyeog" }, { 0xea2, "Hangul_SsangKiyeog" }, { 0xea3, "Hangul_KiyeogSios" }, { 0xea4, "Hangul_Nieun" },
      { 0xea5, "Hangul_NieunJieuj" }, { 0xea6, "Hangul_NieunHieuh" }, { 0xea7, "Hangul_Dikeud" }, { 0xea8, "Hangul_SsangDikeud" },
      { 0xea9, "Hangul_Rieul" }, { 0xeaa, "Hangul_RieulKiyeog" }, { 0xeab, "Hangul_RieulMieum" }, { 0xeac, "Hangul_RieulPieub" },
      { 0xead, "Hangul_RieulSios" }, { 0xeae, "Hangul_RieulTieut" }, { 0xeaf, "Hangul_RieulPhieuf" }, { 0xeb0, "Hangul_RieulHieuh" },
      { 0xeb1, "Hangul_Mieum" }, { 0xeb2, "Hangul_Pieub" }, { 0xeb3, "Hangul_SsangPieub" }, { 0xeb4, "Hangul_PieubSios" },
      { 0xeb5, "Hangul_Sios" }, { 0xeb6, "Hangul_SsangSios" }, { 0xeb7, "Hangul_Ieung" }, { 0xeb8, "Hangul_Jieuj" },
      { 0xeb9, "Hangul_SsangJieuj" }, { 0xeba, "Hangul_Cieuc" }, { 0xebb, "Hangul_Khieuq" }, { 0xebc, "Hangul_Tieut" },
      { 0xebd, "Hangul_Phieuf" }, { 0xebe, "Hangul_Hieuh" }, { 0xebf, "Hangul_A" }, { 0xec0, "Hangul_AE" },
      { 0xec1, "Hangul_YA" }, { 0xec2, "Hangul_YAE" }, { 0xec3, "Hangul_EO" }, { 0xec4, "Hangul_E" },
      { 0xec5, "Hangul_YEO" }, { 0xec6, "Hangul_YE" }, { 0xec7, "Hangul_O" }, { 0xec8, "Hangul_WA" },
      { 0xec9, "Hangul_WAE" }, { 0xeca, "Hangul_OE" }, { 0xecb, "Hangul_YO" }, { 0xecc, "Hangul_U" },
      { 0xecd, "Hangul_WEO" }, { 0xece, "Hangul_WE" }, { 0xecf, "Hangul_WI" }, { 0xed0, "Hangul_YU" },
      { 0xed1, "Hangul_EU" }, { 0xed2, "Hangul_YI" }, { 0xed3, "Hangul_I" }, { 0xed4, "Hangul_J_Kiyeog" },
      { 0xed5, "Hangul_J_SsangKiyeog" }, { 0xed6, "Hangul_J_KiyeogSios" }, { 0xed7, "Hangul_J_Nieun" }, { 0xed8, "Hangul_J_NieunJieuj" },
      { 0xed9, "Hangul_J_NieunHieuh" }, { 0xeda, "Hangul_J_Dikeud" }, { 0xedb, "Hangul_J_Rieul" }, { 0xedc, "Hangul_J_RieulKiyeog" },
      { 0xedd, "Hangul_J_RieulMieum" }, { 0xede, "Hangul_J_RieulPieub" }, { 0xedf, "Hangul_J_RieulSios" }, { 0xee0, "Hangul_J_RieulTieut" },
      { 0xee1, "Hangul_J_RieulPhieuf" }, { 0xee2, "Hangul_J_RieulHieuh" }, { 0xee3, "Hangul_J_Mieum" }, { 0xee4, "Hangul_J_Pieub" },
      { 0xee5, "Hangul_J_PieubSios" }, { 0xee6, "Hangul_J_Sios" }, { 0xee7, "Hangul_J_SsangSios" }, { 0xee8, "Hangul_J_Ieung" },
      { 0xee9, "Hangul_J_Jieuj" }, { 0xeea, "Hangul_J_Cieuc" }, { 0xeeb, "Hangul_J_Khieuq" }, { 0xeec, "Hangul_J_Tieut" },
      { 0xeed, "Hangul_J_Phieuf" }, { 0xeee, "Hangul_J_Hieuh" }, { 0xeef, "Hangul_RieulYeorinHieuh" }, { 0xef0, "Hangul_SunkyeongeumMieum" },
      { 0xef1, "Hangul_SunkyeongeumPieub" }, { 0xef2, "Hangul_PanSios" }, { 0xef3, "Hangul_KkogjiDalrinIeung" }, { 0xef4, "Hangul_SunkyeongeumPhieuf" },
      { 0xef5, "Hangul_YeorinHieuh" }, { 0xef6, "Hangul_AraeA" }, { 0xef7, "Hangul_AraeAE" }, { 0xef8, "Hangul_J_PanSios" },
      { 0xef9, "Hangul_J_KkogjiDalrinIeung" }, { 0xefa, "Hangul_J_YeorinHieuh" }, { 0xeff, "Korean_Won" }, { 0xFD01, "3270_Duplicate" },
      { 0xFD02, "3270_FieldMark" }, { 0xFD03, "3270_Right2" }, { 0xFD04, "3270_Left2" }, { 0xFD05, "3270_BackTab" },
      { 0xFD06, "3270_EraseEOF" }, { 0xFD07, "3270_EraseInput" }, { 0xFD08, "3270_Reset" }, { 0xFD09, "3270_Quit" },
      { 0xFD0A, "3270_PA1" }, { 0xFD0B, "3270_PA2" }, { 0xFD0C, "3270_PA3" }, { 0xFD0D, "3270_Test" },
      { 0xFD0E, "3270_Attn" }, { 0xFD0F, "3270_CursorBlink" }, { 0xFD10, "3270_AltCursor" }, { 0xFD11, "3270_KeyClick" },
      { 0xFD12, "3270_Jump" }, { 0xFD13, "3270_Ident" }, { 0xFD14, "3270_Rule" }, { 0xFD15, "3270_Copy" },
      { 0xFD16, "3270_Play" }, { 0xFD17, "3270_Setup" }, { 0xFD18, "3270_Record" }, { 0xFD19, "3270_ChangeScreen" },
      { 0xFD1A, "3270_DeleteWord" }, { 0xFD1B, "3270_ExSelect" }, { 0xFD1C, "3270_CursorSelect" }, { 0xFD1D, "3270_PrintScreen" },
      { 0xFD1E, "3270_Enter" }, { 0xFE01, "ISO_Lock" }, { 0xFE02, "ISO_Level2_Latch" }, { 0xFE03, "ISO_Level3_Shift" },
      { 0xFE04, "ISO_Level3_Latch" }, { 0xFE05, "ISO_Level3_Lock" }, { 0xFE06, "ISO_Group_Latch" }, { 0xFE07, "ISO_Group_Lock" },
      { 0xFE08, "ISO_Next_Group" }, { 0xFE09, "ISO_Next_Group_Lock" }, { 0xFE0A, "ISO_Prev_Group" }, { 0xFE0B, "ISO_Prev_Group_Lock" },
      { 0xFE0C, "ISO_First_Group" }, { 0xFE0D, "ISO_First_Group_Lock" }, { 0xFE0E, "ISO_Last_Group" }, { 0xFE0F, "ISO_Last_Group_Lock" },
      { 0xFE20, "ISO_Left_Tab" }, { 0xFE21, "ISO_Move_Line_Up" }, { 0xFE22, "ISO_Move_Line_Down" }, { 0xFE23, "ISO_Partial_Line_Up" },
      { 0xFE24, "ISO_Partial_Line_Down" }, { 0xFE25, "ISO_Partial_Space_Left" }, { 0xFE26, "ISO_Partial_Space_Right" }, { 0xFE27, "ISO_Set_Margin_Left" },
      { 0xFE28, "ISO_Set_Margin_Right" }, { 0xFE29, "ISO_Release_Margin_Left" }, { 0xFE2A, "ISO_Release_Margin_Right" }, { 0xFE2B, "ISO_Release_Both_Margins" },
      { 0xFE2C, "ISO_Fast_Cursor_Left" }, { 0xFE2D, "ISO_Fast_Cursor_Right" }, { 0xFE2E, "ISO_Fast_Cursor_Up" }, { 0xFE2F, "ISO_Fast_Cursor_Down" },
      { 0xFE30, "ISO_Continuous_Underline" }, { 0xFE31, "ISO_Discontinuous_Underline" }, { 0xFE32, "ISO_Emphasize" }, { 0xFE33, "ISO_Center_Object" },
      { 0xFE34, "ISO_Enter" }, { 0xFE50, "dead_grave" }, { 0xFE51, "dead_acute" }, { 0xFE52, "dead_circumflex" },
      { 0xFE53, "dead_tilde" }, { 0xFE54, "dead_macron" }, { 0xFE55, "dead_breve" }, { 0xFE56, "dead_abovedot" },
      { 0xFE57, "dead_diaeresis" }, { 0xFE58, "dead_abovering" }, { 0xFE59, "dead_doubleacute" }, { 0xFE5A, "dead_caron" },
      { 0xFE5B, "dead_cedilla" }, { 0xFE5C, "dead_ogonek" }, { 0xFE5D, "dead_iota" }, { 0xFE5E, "dead_voiced_sound" },
      { 0xFE5F, "dead_semivoiced_sound" }, { 0xFE60, "dead_belowdot" }, { 0xFE70, "AccessX_Enable" }, { 0xFE71, "AccessX_Feedback_Enable" },
      { 0xFE72, "RepeatKeys_Enable" }, { 0xFE73, "SlowKeys_Enable" }, { 0xFE74, "BounceKeys_Enable" }, { 0xFE75, "StickyKeys_Enable" },
      { 0xFE76, "MouseKeys_Enable" }, { 0xFE77, "MouseKeys_Accel_Enable" }, { 0xFE78, "Overlay1_Enable" }, { 0xFE79, "Overlay2_Enable" },
      { 0xFE7A, "AudibleBell_Enable" }, { 0xFED0, "First_Virtual_Screen" }, { 0xFED1, "Prev_Virtual_Screen" }, { 0xFED2, "Next_Virtual_Screen" },
      { 0xFED4, "Last_Virtual_Screen" }, { 0xFED5, "Terminate_Server" }, { 0xFEE0, "Pointer_Left" }, { 0xFEE1, "Pointer_Right" },
      { 0xFEE2, "Pointer_Up" }, { 0xFEE3, "Pointer_Down" }, { 0xFEE4, "Pointer_UpLeft" }, { 0xFEE5, "Pointer_UpRight" },
      { 0xFEE6, "Pointer_DownLeft" }, { 0xFEE7, "Pointer_DownRight" }, { 0xFEE8, "Pointer_Button_Dflt" }, { 0xFEE9, "Pointer_Button1" },
      { 0xFEEA, "Pointer_Button2" }, { 0xFEEB, "Pointer_Button3" }, { 0xFEEC, "Pointer_Button4" }, { 0xFEED, "Pointer_Button5" },
      { 0xFEEE, "Pointer_DblClick_Dflt" }, { 0xFEEF, "Pointer_DblClick1" }, { 0xFEF0, "Pointer_DblClick2" }, { 0xFEF1, "Pointer_DblClick3" },
      { 0xFEF2, "Pointer_DblClick4" }, { 0xFEF3, "Pointer_DblClick5" }, { 0xFEF4, "Pointer_Drag_Dflt" }, { 0xFEF5, "Pointer_Drag1" },
      { 0xFEF6, "Pointer_Drag2" }, { 0xFEF7, "Pointer_Drag3" }, { 0xFEF8, "Pointer_Drag4" }, { 0xFEF9, "Pointer_EnableKeys" },
      { 0xFEFA, "Pointer_Accelerate" }, { 0xFEFB, "Pointer_DfltBtnNext" }, { 0xFEFC, "Pointer_DfltBtnPrev" }, { 0xFEFD, "Pointer_Drag5" },
      { 0xFF08, "BackSpace" }, { 0xFF09, "Tab" }, { 0xFF0A, "Linefeed" }, { 0xFF0B, "Clear" },
      { 0xFF0D, "Return" }, { 0xFF13, "Pause" }, { 0xFF14, "Scroll_Lock" }, { 0xFF15, "Sys_Req" },
      { 0xFF1B, "Escape" }, { 0xFF20, "Multi_key" }, { 0xFF21, "Kanji" }, { 0xFF22, "Muhenkan" },
      { 0xFF23, "Henkan" }, { 0xFF23, "Henkan_Mode" }, { 0xFF24, "Romaji" }, { 0xFF25, "Hiragana" },
      { 0xFF26, "Katakana" }, { 0xFF27, "Hiragana_Katakana" }, { 0xFF28, "Zenkaku" }, { 0xFF29, "Hankaku" },
      { 0xFF2A, "Zenkaku_Hankaku" }, { 0xFF2B, "Touroku" }, { 0xFF2C, "Massyo" }, { 0xFF2D, "Kana_Lock" },
      { 0xFF2E, "Kana_Shift" }, { 0xFF2F, "Eisu_Shift" }, { 0xFF30, "Eisu_toggle" }, { 0xff31, "Hangul" },
      { 0xff32, "Hangul_Start" }, { 0xff33, "Hangul_End" }, { 0xff34, "Hangul_Hanja" }, { 0xff35, "Hangul_Jamo" },
      { 0xff36, "Hangul_Romaja" }, { 0xFF37, "Codeinput" }, { 0xff37, "Hangul_Codeinput" }, { 0xFF37, "Kanji_Bangou" },
      { 0xff38, "Hangul_Jeonja" }, { 0xff39, "Hangul_Banja" }, { 0xff3a, "Hangul_PreHanja" }, { 0xff3b, "Hangul_PostHanja" },
      { 0xff3c, "Hangul_SingleCandidate" }, { 0xFF3C, "SingleCandidate" }, { 0xff3d, "Hangul_MultipleCandidate" }, { 0xFF3D, "MultipleCandidate" },
      { 0xFF3D, "Zen_Koho" }, { 0xff3e, "Hangul_PreviousCandidate" }, { 0xFF3E, "Mae_Koho" }, { 0xFF3E, "PreviousCandidate" },
      { 0xff3f, "Hangul_Special" }, { 0xFF50, "Home" }, { 0xFF51, "Left" }, { 0xFF52, "Up" },
      { 0xFF53, "Right" }, { 0xFF54, "Down" }, { 0xFF55, "Page_Up" }, { 0xFF55, "Prior" },
      { 0xFF56, "Next" }, { 0xFF56, "Page_Down" }, { 0xFF57, "End" }, { 0xFF58, "Begin" },
      { 0xFF60, "Select" }, { 0xFF61, "Print" }, { 0xFF62, "Execute" }, { 0xFF63, "Insert" },
      { 0xFF65, "Undo" }, { 0xFF66, "Redo" }, { 0xFF67, "Menu" }, { 0xFF68, "Find" },
      { 0xFF69, "Cancel" }, { 0xFF6A, "Help" }, { 0xFF6B, "Break" }, { 0xFF7E, "Arabic_switch" },
      { 0xFF7E, "Greek_switch" }, { 0xFF7E, "Hangul_switch" }, { 0xFF7E, "Hebrew_switch" }, { 0xFF7E, "ISO_Group_Shift" },
      { 0xFF7E, "kana_switch" }, { 0xFF7E, "Mode_switch" }, { 0xFF7E, "script_switch" }, { 0xFF7F, "Num_Lock" },
      { 0xFF80, "KP_Space" }, { 0xFF89, "KP_Tab" }, { 0xFF8D, "KP_Enter" }, { 0xFF91, "KP_F1" },
      { 0xFF92, "KP_F2" }, { 0xFF93, "KP_F3" }, { 0xFF94, "KP_F4" }, { 0xFF95, "KP_Home" },
      { 0xFF96, "KP_Left" }, { 0xFF97, "KP_Up" }, { 0xFF98, "KP_Right" }, { 0xFF99, "KP_Down" },
      { 0xFF9A, "KP_Page_Up" }, { 0xFF9A, "KP_Prior" }, { 0xFF9B, "KP_Next" }, { 0xFF9B, "KP_Page_Down" },
      { 0xFF9C, "KP_End" }, { 0xFF9D, "KP_Begin" }, { 0xFF9E, "KP_Insert" }, { 0xFF9F, "KP_Delete" },
      { 0xFFAA, "KP_Multiply" }, { 0xFFAB, "KP_Add" }, { 0xFFAC, "KP_Separator" }, { 0xFFAD, "KP_Subtract" },
      { 0xFFAE, "KP_Decimal" }, { 0xFFAF, "KP_Divide" }, { 0xFFB0, "KP_0" }, { 0xFFB1, "KP_1" },
      { 0xFFB2, "KP_2" }, { 0xFFB3, "KP_3" }, { 0xFFB4, "KP_4" }, { 0xFFB5, "KP_5" },
      { 0xFFB6, "KP_6" }, { 0xFFB7, "KP_7" }, { 0xFFB8, "KP_8" }, { 0xFFB9, "KP_9" },
      { 0xFFBD, "KP_Equal" }, { 0xFFBE, "F1" }, { 0xFFBF, "F2" }, { 0xFFC0, "F3" },
      { 0xFFC1, "F4" }, { 0xFFC2, "F5" }, { 0xFFC3, "F6" }, { 0xFFC4, "F7" },
      { 0xFFC5, "F8" }, { 0xFFC6, "F9" }, { 0xFFC7, "F10" }, { 0xFFC8, "F11" },
      { 0xFFC8, "L1" }, { 0xFFC9, "F12" }, { 0xFFC9, "L2" }, { 0xFFCA, "F13" },
      { 0xFFCA, "L3" }, { 0xFFCB, "F14" }, { 0xFFCB, "L4" }, { 0xFFCC, "F15" },
      { 0xFFCC, "L5" }, { 0xFFCD, "F16" }, { 0xFFCD, "L6" }, { 0xFFCE, "F17" },
      { 0xFFCE, "L7" }, { 0xFFCF, "F18" }, { 0xFFCF, "L8" }, { 0xFFD0, "F19" },
      { 0xFFD0, "L9" }, { 0xFFD1, "F20" }, { 0xFFD1, "L10" }, { 0xFFD2, "F21" },
      { 0xFFD2, "R1" }, { 0xFFD3, "F22" }, { 0xFFD3, "R2" }, { 0xFFD4, "F23" },
      { 0xFFD4, "R3" }, { 0xFFD5, "F24" }, { 0xFFD5, "R4" }, { 0xFFD6, "F25" },
      { 0xFFD6, "R5" }, { 0xFFD7, "F26" }, { 0xFFD7, "R6" }, { 0xFFD8, "F27" },
      { 0xFFD8, "R7" }, { 0xFFD9, "F28" }, { 0xFFD9, "R8" }, { 0xFFDA, "F29" },
      { 0xFFDA, "R9" }, { 0xFFDB, "F30" }, { 0xFFDB, "R10" }, { 0xFFDC, "F31" },
      { 0xFFDC, "R11" }, { 0xFFDD, "F32" }, { 0xFFDD, "R12" }, { 0xFFDE, "F33" },
      { 0xFFDE, "R13" }, { 0xFFDF, "F34" }, { 0xFFDF, "R14" }, { 0xFFE0, "F35" },
      { 0xFFE0, "R15" }, { 0xFFE1, "Shift_L" }, { 0xFFE2, "Shift_R" }, { 0xFFE3, "Control_L" },
      { 0xFFE4, "Control_R" }, { 0xFFE5, "Caps_Lock" }, { 0xFFE6, "Shift_Lock" }, { 0xFFE7, "Meta_L" },
      { 0xFFE8, "Meta_R" }, { 0xFFE9, "Alt_L" }, { 0xFFEA, "Alt_R" }, { 0xFFEB, "Super_L" },
      { 0xFFEC, "Super_R" }, { 0xFFED, "Hyper_L" }, { 0xFFEE, "Hyper_R" }, { 0xFFFF, "Delete" },
      { 0xFFFFFF, "VoidSymbol" }, { 0, NULL }
};

#endif
