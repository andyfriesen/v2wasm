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

#if !defined(RENDER_INC)
#define RENDER_INC

/*
extern int	seek_x;
extern int	seek_y;
extern void DoCameraTracking();
*/

extern unsigned char animate;
extern unsigned char cameratracking;
extern unsigned char tracker;
extern unsigned char showobs;
extern unsigned char showzone;

// tSB - I don't like this, but it's gotta be here, since vdriver.cc needs it
// briefly
extern byte inside;

extern void Render();
extern void BlitLayer(byte c);
extern void CheckTileAnimation();
extern int rnd(int min, int max);

#endif  // RENDER_INC
