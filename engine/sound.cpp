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

#define SOUND_H

#include "midasdll.h"
#include "verge.h"

// that enough? >:D  --tSB
#define MAXSOUNDS 90

// lil' struct, since MIDAS leaves you to your own devices concerning sample
// rate >_<
typedef struct {
    MIDASsample sample;
    int rate;
} sample_t;

// ============================ Interface type crap ==========================

// most of this is unimplemented
int sfx_volume = 63;    // misnomer -- music volume
int sfx_numchans = 50;  // for EVERYTHING
int sfx_mixrate = 44100;
int sfx_bufflen = 100;
int sfx_safemode = 0;

// ================================= Data ====================================

MIDASmodule curmod;  // the currently loaded music file
MIDASmodulePlayHandle
    playhandle;       // I dunno, I just use the lib, I didn't write it. ;)
int musicloaded = 0;  // gah.
int musicvolume;

int numsfx = 0;
sample_t sfx[MAXSOUNDS];

// ================================= Code ====================================

void ShutdownMusicSystem();  // BLEH! :P~
void StopMusic();
void FreeAllSounds();

inline void test(int i)
// error checking crap
{
    if (!i) {
        int j = MIDASgetLastError();
        Sys_Error(va("MIDAS: %s", MIDASgetErrorMessage(j)));
    }
}

void InitMusicSystem(unsigned int hWnd) {
    if (!MIDASstartup()) {
        Sys_Error("MIDASstartup failed");
        ShutdownMusicSystem();
    }

    if (!sfx_safemode) {
        MIDASsetOption(MIDAS_OPTION_DSOUND_MODE, MIDAS_DSOUND_PRIMARY);
        MIDASsetOption(MIDAS_OPTION_DSOUND_HWND, hWnd);
    }

    test(MIDASinit());

    test(MIDASopenChannels(sfx_numchans));
    /* if (!MIDASopenChannels(sfx_numchans))
      {
       int err=MIDASgetLastError();
       Sys_Error(va("MIDAS: %s",MIDASgetErrorMessage(err)));
      }*/

    test(MIDASstartBackgroundPlay(0));
    test(MIDASallocAutoEffectChannels(10));  // TODO: make this variable

    /* if (!MIDASallocAutoEffectChannels(10))
      {
       int err=MIDASgetLastError();
       Sys_Error(va("MIDAS: %s",MIDASgetErrorMessage(err)));
      }*/
}

void ShutdownMusicSystem() {
    StopMusic();
    FreeAllSounds();
    MIDASfreeAutoEffectChannels();
    MIDAScloseChannels();
    MIDASstopBackgroundPlay();
    MIDASclose();
    // uh... what would we do if the shutdown failed?  I dunno --tSB
}

void UpdateSound(void) {
    //	Mikmod_Update(md);
}

void PlayMusic(const char *fname) {
    char temp[255];

    strcpy(temp, fname);

    StopMusic();

    curmod = MIDASloadModule(temp);
    if (curmod == NULL) {
        // Sys_Error(va("Unable to load music file, '%s'",temp));
        Message_Send(va("MIDAS: Failed loading music file, '%s'", temp), 100);
        return;
    } else
        musicloaded = 1;

    playhandle = MIDASplayModule(curmod, TRUE);
}

int GetMusicVolume() {
    return musicvolume << 1;  // *2
}

void SetMusicVolume(int volume) {
    volume >>= 1;  // half
    if (volume >= 0 && volume <= 64) {
        musicvolume = volume;
        if (musicloaded) MIDASsetMusicVolume(playhandle, musicvolume);
    }
}

void StopMusic() {
    if (musicloaded) {
        MIDASstopModule(playhandle);
        MIDASfreeModule(curmod);
    }
    musicloaded = 0;
}

int SampleRate(const char *fname)
// poo.  MIDAS makes you figure this one out on your own.  Oh well! --tSB
{
    VFILE *f;

    f = vopen(fname);
    if (!f)
        Sys_Error(
            "Shit of the biblical type hath happened.  Consult your local "
            "priest for further advide.");

    vseek(f, 24, SEEK_SET);

    unsigned int i;
    vread(&i, 4, f);
    vclose(f);

    return i;
}

int CacheSound(const char *fname) {
    if (numsfx == MAXSOUNDS) Sys_Error("Too many sound effects");

    char temp[255];

    strcpy(temp, fname);
    sfx[numsfx].sample = MIDASloadWaveSample(temp, FALSE);
    if (sfx[numsfx].sample == 0)
        Sys_Error("WAV [%s] load error: %s", fname, "");

    sfx[numsfx].rate = SampleRate(fname);

    return numsfx++;
}

void FreeAllSounds() {
    for (int i = 0; i < numsfx; i++) MIDASfreeSample(sfx[i].sample);
    numsfx = 0;
}

void PlaySFX(int index, int vol, int pan) {
    MIDASsamplePlayHandle i =
        MIDASplaySample(sfx[index].sample, MIDAS_CHANNEL_AUTO, 10,
            sfx[index].rate, vol, MIDAS_PAN_MIDDLE);  //(pan+64)<<1);
    test(i);
}

static int soundon = 1;

void SoundPause() {
    if (sfx_safemode) return;  // not needed
    if (soundon) {
        MIDASsuspend();
        soundon = 0;
    }
}

void SoundResume() {
    if (sfx_safemode) return;  // extraneous
    if (!soundon) {
        MIDASresume();
        soundon = 1;
    }
}
