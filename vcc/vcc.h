/*
VERGE 2.5+j (AKA V2k+j) -  A video game creation engine
Copyright (C) 1998-2000  Benjamin Eirich (AKA vecna), et al
Please see authors.txt for a complete list of contributing authors.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#if !defined(__VCC_INC)
#define __VCC_INC

#define QUIET 0
#define DEFAULT 1
#define VERBOSE 2

#define CONTROL 1
#define DIGIT 2
#define IDENTIFIER 3
#define RESERVED 4
#define FUNCTION 5

// extern char *strbuf;
extern int locate;

extern void log(const char* message, ...);

extern char* va(const char* format, ...);
extern void err(const char* message, ...);
extern void vcerr(const char* message, ...);
extern void dprint(const char* message, ...);
extern void vprint(const char* message, ...);

#endif // __VCC_INC
