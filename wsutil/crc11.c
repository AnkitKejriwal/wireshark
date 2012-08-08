/*
 * crc11.c
 * $Id: crc11.c
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <glib.h>

/**
 * Functions and types for CRC checks.
 *
 * Generated on Tue Aug  7 15:45:57 2012,
 * by pycrc v0.7.10, http://www.tty1.net/pycrc/
 * using the configuration:
 *    Width        = 11
 *    Poly         = 0x307
 *    XorIn        = 0x000
 *    ReflectIn    = False
 *    XorOut       = 0x000
 *    ReflectOut   = False
 *    Algorithm    = table-driven
 *****************************************************************************/
/**
 * Static table used for the table_driven implementation.
 *****************************************************************************/
static const guint16 crc11_table_307_noreflect_noxor[256] = {
    0x000, 0x307, 0x60e, 0x509, 0x71b, 0x41c, 0x115, 0x212, 0x531, 0x636, 0x33f, 0x038, 0x22a, 0x12d, 0x424, 0x723,
    0x165, 0x262, 0x76b, 0x46c, 0x67e, 0x579, 0x070, 0x377, 0x454, 0x753, 0x25a, 0x15d, 0x34f, 0x048, 0x541, 0x646,
    0x2ca, 0x1cd, 0x4c4, 0x7c3, 0x5d1, 0x6d6, 0x3df, 0x0d8, 0x7fb, 0x4fc, 0x1f5, 0x2f2, 0x0e0, 0x3e7, 0x6ee, 0x5e9,
    0x3af, 0x0a8, 0x5a1, 0x6a6, 0x4b4, 0x7b3, 0x2ba, 0x1bd, 0x69e, 0x599, 0x090, 0x397, 0x185, 0x282, 0x78b, 0x48c,
    0x594, 0x693, 0x39a, 0x09d, 0x28f, 0x188, 0x481, 0x786, 0x0a5, 0x3a2, 0x6ab, 0x5ac, 0x7be, 0x4b9, 0x1b0, 0x2b7,
    0x4f1, 0x7f6, 0x2ff, 0x1f8, 0x3ea, 0x0ed, 0x5e4, 0x6e3, 0x1c0, 0x2c7, 0x7ce, 0x4c9, 0x6db, 0x5dc, 0x0d5, 0x3d2,
    0x75e, 0x459, 0x150, 0x257, 0x045, 0x342, 0x64b, 0x54c, 0x26f, 0x168, 0x461, 0x766, 0x574, 0x673, 0x37a, 0x07d,
    0x63b, 0x53c, 0x035, 0x332, 0x120, 0x227, 0x72e, 0x429, 0x30a, 0x00d, 0x504, 0x603, 0x411, 0x716, 0x21f, 0x118,
    0x02f, 0x328, 0x621, 0x526, 0x734, 0x433, 0x13a, 0x23d, 0x51e, 0x619, 0x310, 0x017, 0x205, 0x102, 0x40b, 0x70c,
    0x14a, 0x24d, 0x744, 0x443, 0x651, 0x556, 0x05f, 0x358, 0x47b, 0x77c, 0x275, 0x172, 0x360, 0x067, 0x56e, 0x669,
    0x2e5, 0x1e2, 0x4eb, 0x7ec, 0x5fe, 0x6f9, 0x3f0, 0x0f7, 0x7d4, 0x4d3, 0x1da, 0x2dd, 0x0cf, 0x3c8, 0x6c1, 0x5c6,
    0x380, 0x087, 0x58e, 0x689, 0x49b, 0x79c, 0x295, 0x192, 0x6b1, 0x5b6, 0x0bf, 0x3b8, 0x1aa, 0x2ad, 0x7a4, 0x4a3,
    0x5bb, 0x6bc, 0x3b5, 0x0b2, 0x2a0, 0x1a7, 0x4ae, 0x7a9, 0x08a, 0x38d, 0x684, 0x583, 0x791, 0x496, 0x19f, 0x298,
    0x4de, 0x7d9, 0x2d0, 0x1d7, 0x3c5, 0x0c2, 0x5cb, 0x6cc, 0x1ef, 0x2e8, 0x7e1, 0x4e6, 0x6f4, 0x5f3, 0x0fa, 0x3fd,
    0x771, 0x476, 0x17f, 0x278, 0x06a, 0x36d, 0x664, 0x563, 0x240, 0x147, 0x44e, 0x749, 0x55b, 0x65c, 0x355, 0x052,
    0x614, 0x513, 0x01a, 0x31d, 0x10f, 0x208, 0x701, 0x406, 0x325, 0x022, 0x52b, 0x62c, 0x43e, 0x739, 0x230, 0x137
};
/**
 * Update the crc value with new data.
 *
 * \param crc      The current crc value.
 * \param data     Pointer to a buffer of \a data_len bytes.
 * \param data_len Number of bytes in the \a data buffer.
 * \return         The updated crc value.
 *****************************************************************************/
guint16 crc11_307_noreflect_noxor(const guint8 *data, guint64 data_len)
{
    guint16 crc = 0;
    guint tbl_idx;

    while (data_len--) {
        tbl_idx = ((crc >> 3) ^ *data) & 0xff;
        crc = (crc11_table_307_noreflect_noxor[tbl_idx] ^ (crc << 8)) & 0x7ff;

        data++;
    }
    return crc & 0x7ff;
}
