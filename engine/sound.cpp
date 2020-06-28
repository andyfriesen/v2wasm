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

#include <emscripten.h>

#include "verge.h"
#include "vfile.h"

const int MAXSOUNDS = 90;

// ============================ Interface type crap ==========================

// most of this is unimplemented
int sfx_volume = 63;   // misnomer -- music volume
int sfx_numchans = 50; // for EVERYTHING
int sfx_mixrate = 44100;
int sfx_bufflen = 100;
int sfx_safemode = 0;

// ================================= Data ====================================

int musicloaded = 0; // gah.
int musicvolume;

std::vector<std::string> sounds;

// ================================= Code ====================================

EM_JS(void, wasm_initSound, (), {
    const ctx = new AudioContext();
    const gainNode = ctx.createGain();
    gainNode.connect(ctx.destination);

    window.verge.audioContext = ctx;
    window.verge.gainNode = gainNode;
    window.verge.sounds = {};

    if (ctx.audioWorklet) {
        window.verge.mptInited = ctx.audioWorklet.addModule('mpt-worklet.js').then(() => {
            console.log('mpt-worklet initialized');
            window.verge.mptNode = new AudioWorkletNode(ctx, 'libopenmpt-processor', {
                numberOfInputs: 0,
                numberOfOutputs: 1,
                outputChannelCount: [2],
            });

            window.verge.mptNode.connect(gainNode);
        });
    } else {
        console.warn("AudioWorklet is not supported in this browser.  No music.  Sorry!");
        window.verge.mptInited = Promise.resolve();
    }
});

EM_JS(void, wasm_loadSound, (const char* filename, const char* soundData, int soundDataSize), {
    const name = UTF8ToString(filename);
    const audioData = HEAPU8.buffer.slice(soundData, soundData + soundDataSize);
    window.verge = window.verge || {};
    window.verge.audioContext.decodeAudioData(
        audioData,
        decoded => {
            // This is super, super sloppy.  We don't do anything to wait for this decode step to finish.
            // Instead, we just pray that it finishes before the sound is needed for the first time!
            // wasm_playSound is coded to silently do nothing if you try to play something that's not loaded.
            // -- andy 17 March 2020
            // console.log("Loaded ", name, " ok!");
            window.verge.sounds[name] = decoded;
        },
        () => {
            console.log("Unable to load sound data for ", name); // fixme
        }
    );
});

EM_JS(void, wasm_playSound, (const char* filename), {
    const name = UTF8ToString(filename);
    const sound = window.verge.sounds[name];
    if (!sound) {
        console.error("Unknown sound ", name);
        return;
    }

    const source = window.verge.audioContext.createBufferSource();
    source.connect(window.verge.audioContext.destination);
    source.buffer = sound;
    source.start(0);
});

EM_JS(void, wasm_playSong, (const void* data, int length), {
    const buffer = Module.HEAP8.buffer.slice(data, data + length);
    const v = new Uint8Array(buffer);

    window.verge.mptInited.then(() => {
        if (!window.verge.mptNode) {
            return;
        }

        window.verge.mptNode.port.postMessage({
            songData: buffer,
            setRepeatCount: -1
        });
    });
});

EM_JS(void, wasm_stopMusic, (), {
    window.verge.mptInited.then(() => {
        window.verge.mptNode.port.postMessage({
            songData: new ArrayBuffer(0)
        });
    });
})

EM_JS(void, wasm_setVolume, (int volume), {
    console.log('setvolume', volume);
    window.verge.gainNode.gain.setValueAtTime(volume / 100, window.verge.audioContext.currentTime);
});

void ShutdownMusicSystem(); // BLEH! :P~
void StopMusic();
void FreeAllSounds();

void InitMusicSystem() {
    wasm_initSound();
}

void ShutdownMusicSystem() {
    StopMusic();
    FreeAllSounds();
}

void PlayMusic(const char* fname) {
    printf("PlayMusic %s\n", fname);
    StopMusic();

    VFILE* f = vopen(fname);
    if (!f) {
        printf("Unable to load song %s\n", fname);
        return;
    }

    vseek(f, 0, SEEK_END);
    size_t size = vtell(f);
    vseek(f, 0, SEEK_SET);

    char* songData = new char[size];
    vread(songData, size, f);

    vclose(f);

    wasm_playSong(songData, size);
    delete[] songData;

    musicloaded = 1;
}

int GetMusicVolume() {
    return musicvolume << 1;
}

void SetMusicVolume(int volume) {
    volume >>= 1;
    if (volume >= 0 && volume <= 64) {
        musicvolume = volume;
        if (musicloaded) {
        }
    }
}

void StopMusic() {
    if (!musicloaded) {
        return;
    }

    wasm_stopMusic();

    musicloaded = 0;
}

int CacheSound(const char* fname) {
    if (sounds.size() == MAXSOUNDS) {
        Sys_Error("Too many sound effects");
    }

    VFILE* f = vopen(fname);
    if (!f) {
        printf("Unable to load sound effect %s\n", fname);
        return 0;
    }

    vseek(f, 0, SEEK_END);
    size_t size = vtell(f);
    vseek(f, 0, SEEK_SET);

    char* soundData = new char[size];
    vread(soundData, size, f);

    wasm_loadSound(fname, soundData, size);

    delete[] soundData;

    sounds.emplace_back(fname);

    return sounds.size() - 1;
}

void FreeAllSounds() {
    sounds.clear();
}

void PlaySFX(int index, int vol, int pan) {
    // TODO volume and pan
    wasm_playSound(sounds[index].c_str());
}

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
