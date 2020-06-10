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

/*
        mouse.h
        coded by aen
        ---
        last updated: 30oct99
*/

#if !defined(MOUSE_INC)
#define MOUSE_INC

extern void Mouse_Init();
extern void Mouse_Read();
extern int Mouse_X();
extern int Mouse_Y();
extern void Mouse_SetPosition(int x, int y);
extern int Mouse_Button(int button);
extern int Mouse_ButtonFlags();
extern void Mouse_ButtonSetFlags(int flags);

#endif // MOUSE_INC