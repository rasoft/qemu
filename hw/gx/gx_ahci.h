/*
 * NationalChip AHCI Emulation
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GX_AHCI_H
#define GX_AHCI_H

#include "hw/ide/ahci.h"

#define TYPE_GX_AHCI "gx-ahci"
OBJECT_DECLARE_SIMPLE_TYPE(GxAHCIState, GX_AHCI)

#define GX_AHCI_MMIO_OFF  0x80
#define GX_AHCI_MMIO_SIZE 0x80

struct GxAHCIState {
    /*< private >*/
    SysbusAHCIState parent_obj;
    /*< public >*/

    MemoryRegion mmio;
    uint32_t regs[GX_AHCI_MMIO_SIZE / 4];
};

#endif
