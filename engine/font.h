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

#ifndef FONT_H
#define FONT_H

extern void Font_Print(int slot, const char* zstr);
extern void Font_PrintImbed(int slot, const char* zstr);
extern int Font_GetWidth(int slot);
extern int Font_GetLength(int slot);
extern int Font_Load(const char* filename);

extern void Font_GotoXY(int x, int y);
extern int Font_GetX();
extern int Font_GetY();

#endif // FONT_H
