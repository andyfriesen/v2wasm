/*
Copyright (C) 1998 BJ Eirich (aka vecna)
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

// dpmi.h

/*
        coded by Charles Rector AKA aen
        last updated: 25sep99
*/

#ifndef DPMI_INC
#define DPMI_INC

#define LOWBUF_SIZE (1024 * 16)  // 16K
extern char *_lowbuf;

#pragma pack(1)
struct RM_REGS {  // real-mode registers
    unsigned long edi, esi, ebp, reserved, ebx, edx, ecx, eax;
    unsigned short flags, es, ds, fs, gs, ip, cs, sp, ss;
};
#pragma pack()

#define DPMI_INT 0x31

// DPMI services
#define DPMI_SIM_REAL_MODE_INT 0x0300
#define DPMI_ALLOCATE_DOS_MEMORY 0x0100
#define DPMI_FREE_DOS_MEMORY 0x0101

extern void __setup_lowbuf();
extern void simulate_real_mode_int(int intno, struct RM_REGS *in);
extern void *dpmi_real_malloc(int size);
extern void dpmi_real_free(void *ptr);
extern void *dpmi_map_physical(void *physical, int bytes);
extern void dpmi_unmap_physical(void *linear);

#endif  // DPMI_INC