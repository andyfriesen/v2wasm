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

// Changelog
// <tSB> Dec 4, '00
//       Pretty much rewritten to use MIDAS instead of MikMod.  MikMod won't
//       compile under GCC. :P

#include "verge.h"

const int MAXSOUNDS = 90;

// ============================ Interface type crap ==========================

struct sample_t {};

// most of this is unimplemented
int sfx_volume = 63;   // misnomer -- music volume
int sfx_numchans = 50; // for EVERYTHING
int sfx_mixrate = 44100;
int sfx_bufflen = 100;
int sfx_safemode = 0;

// ================================= Data ====================================

int musicloaded = 0; // gah.
int musicvolume;

int numsfx = 0;
sample_t sfx[MAXSOUNDS];

// ================================= Code ====================================

void ShutdownMusicSystem(); // BLEH! :P~
void StopMusic();
void FreeAllSounds();

void InitMusicSystem() {}

void ShutdownMusicSystem() {
    StopMusic();
    FreeAllSounds();
}

void UpdateSound(void) {
    //	Mikmod_Update(md);
}

void PlayMusic(const char* fname) {
    char temp[255];

    strcpy(temp, fname);

    StopMusic();

    musicloaded = 1;
}

int GetMusicVolume() {
    return musicvolume << 1; // *2
}

void SetMusicVolume(int volume) {
    volume >>= 1; // half
    if (volume >= 0 && volume <= 64) {
        musicvolume = volume;
        if (musicloaded) {
        }
    }
}

void StopMusic() {
    if (musicloaded) {
    }
    musicloaded = 0;
}

int SampleRate(const char* fname)
// poo.  MIDAS makes you figure this one out on your own.  Oh well! --tSB
{
    VFILE* f;

    f = vopen(fname);
    if (!f)
        Sys_Error("Unable to read sample rate of %s", fname);

    vseek(f, 24, SEEK_SET);

    unsigned int i;
    vread(&i, 4, f);
    vclose(f);

    return i;
}

int CacheSound(const char* fname) {
    if (numsfx == MAXSOUNDS)
        Sys_Error("Too many sound effects");

    return numsfx++;
}

void FreeAllSounds() { numsfx = 0; }

void PlaySFX(int index, int vol, int pan) {}

static int soundon = 1;

void SoundPause() {
    if (sfx_safemode)
        return; // not needed
    if (soundon) {
        soundon = 0;
    }
}

void SoundResume() {
    if (sfx_safemode)
        return; // extraneous
    if (!soundon) {
        soundon = 1;
    }
}
