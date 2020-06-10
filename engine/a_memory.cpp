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

#include <stdio.h>
#include <string.h>

#include "verge.h"

// *****
// TODO: Move the chunk list from an array to a linked list?
//       Would eliminate hardcoded chunk limit, but would make
//       general operation slower. Probably not The Right Thing,
//       sides the chunk limit can be interesting sometimes. If
//       it becomes problematic, consider a Binary Tree.
// *****

// ***************************** Data *****************************

#define MAXCHUNKS 1000
#define PARANOID
#define PADFILLVALUE 254
#define PADSIZE 256

typedef struct {
    void* pointer;
    int size;
    int owner;
    char desc[40];
} memblockType;

memblockType chunks[MAXCHUNKS + 1];
int numchunks = 0;

static int ohfuck = 0;
// set if memory is corrupted.  This is so that vfree can release all the memory
// before bombing out with an error --tSB

// ***************************** Code *****************************

void* valloc(int amount, char* desc, int owner) {
    char* ptr;

    if (numchunks == MAXCHUNKS)
        Sys_Error("Failed allocated %d bytes (%s), reason: Out of chunks.",
            amount, desc);

    if (amount == 0)
        Sys_Error(
            "Zero-size block requested (desc: %s, owner: %d)", desc, owner);
    if (amount < 0)
        Sys_Error("valloc: %s: bogus request for %d bytes", desc, amount);

#ifdef PARANOID
    CheckCorruption();
    ptr = (char*)malloc(amount + 2 * PADSIZE);
    if (!ptr)
        Sys_Error("valloc: %s: memory exhausted on %d + %d bytes.", desc,
            amount, PADSIZE * 2);
    chunks[numchunks].pointer = (void*)(ptr + PADSIZE);
    chunks[numchunks].size = amount;
    V_memset((char*)chunks[numchunks].pointer - PADSIZE, PADFILLVALUE, PADSIZE);
    V_memset((char*)chunks[numchunks].pointer + chunks[numchunks].size,
        PADFILLVALUE, PADSIZE);
#else
    ptr = malloc(amount);
    if (!ptr)
        Sys_Error("valloc: %s: memory exhausted on %d bytes.", desc, amount);
    chunks[numchunks].pointer = (void*)ptr;
    chunks[numchunks].size = amount;
#endif
    chunks[numchunks].owner = owner;
    V_strncpy(chunks[numchunks].desc, desc, 39);
    chunks[numchunks].desc[39] = 0;
    V_memset(chunks[numchunks].pointer, 0, chunks[numchunks].size);

    return chunks[numchunks++].pointer;
}

void* qvalloc(int amount) {
    void* ptr;

    // Quick and dirty memory allocation. Should be used ONLY
    // for temporary blocks in speed-critical loops.

    ptr = malloc(amount);
    if (!ptr)
        Sys_Error("qvalloc: Failed allocating %d bytes.", amount);

    return ptr;
}

void qvfree(void* ptr) { free(ptr); }

int TotalBytesAllocated(void) {
    int i, tally = 0;

    for (i = 0; i < numchunks; i++)
        tally += chunks[i].size;

    return tally;
}

int FindChunk(void* pointer) {
    int i;

    for (i = 0; i < numchunks; i++)
        if (chunks[i].pointer == pointer)
            return i;
    return -1;
}

void FreeChunk(int i) {
#ifdef PARANOID
    if (!ohfuck)
        CheckCorruption(); // if ohfuck is set, then we already know there's
                           // corruption, and we're clearing all the memory out
    free((void*)((int)chunks[i].pointer - PADSIZE));
#else
    free(chunks[i].pointer);
#endif
    for (; i < numchunks; i++)
        chunks[i] = chunks[i + 1];
    numchunks--;
}

int vfree(void* ptr) {
    int i = FindChunk(ptr);
    if (i == -1) {
        Log(va("vfree: Attempted to free ptr 0x%08X that was not allocated. "
               "[dumping mem report]",
            ptr));
        MemReport();
        return -1;
    }
    FreeChunk(i);

    return 0;
}

void FreeByOwner(int owner) {
    int i;

    for (i = 0; i < numchunks; i++)
        if (chunks[i].owner == owner)
            FreeChunk(i--);
}

void MemReport(void) {
    int i;

    Log("");
    Log("========================================");
    Log("= Memory usage report for this session =");
    Log("========================================");
    Log(va(
        "Chunks currently allocated: %d (MAXCHUNKS %d)", numchunks, MAXCHUNKS));
    Log(va("%d total bytes allocated. ", TotalBytesAllocated()));
#ifdef PARANOID
    Log(va(
        "PARANOID is ON. (pad size: %d pad value: %d)", PADSIZE, PADFILLVALUE));
#else
    Log("PARANOID is OFF.");
#endif
    Log("");
    Log("Per-chunk analysis: ");

    for (i = 0; i < numchunks; i++) {
        Log(va("[%3d] Ptr at: 0x%08X size: %8d owner: %3d desc: %s", i,
            chunks[i].pointer, chunks[i].size, chunks[i].owner,
            chunks[i].desc));
    }
}

void FreeAllMemory()
// Clears it all out!
// ohfuck is set to 1 so that FreeChunk doesn't check for integrity. (it's
// entirely possible that we're clearing out
// BECAUSE memory is corrupted, so we want to avoid a loop ^_~)
{
    int i;

    ohfuck = 1;
    for (i = 0; i < numchunks; i++)
        FreeChunk(i);
}

#ifdef PARANOID
int ChunkIntegrity(int i) {
    char* tptr;

    tptr = (char*)malloc(PADSIZE);
    V_memset(tptr, PADFILLVALUE, PADSIZE);
    if (V_memcmp((char*)chunks[i].pointer - PADSIZE, tptr, PADSIZE))
        return -1; // Prefix corruption
    if (V_memcmp((char*)chunks[i].pointer + chunks[i].size, tptr, PADSIZE))
        return 1; // Suffix corruption
    free(tptr);
    return 0; // no corruption
}

void CheckCorruption(void) {
    int i, j;

    for (i = 0; i < numchunks; i++) {
        j = ChunkIntegrity(i);
        if (!j)
            continue;

        MemReport();
        FreeAllMemory();

        if (j == -1)
            Sys_Error("Prefix corruption on chunk %d.", i);
        if (j == 1)
            Sys_Error("Suffix corruption on chunk %d.", i);
    }
}
#else
void CheckCorruption(void) { return; }
#endif
