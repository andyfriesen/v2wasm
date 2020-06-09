#ifndef PCX_H
#define PCX_H

// -- globals --

extern word width, depth;

// -- prototypes --

extern void ReadPCXLine(byte *dest);
extern void LoadPCXHeader(char *fname);
extern void LoadPCXHeaderNP(char *fname);
extern void LoadPCX(char *fname, byte *dest);
extern byte *LoadPCXBuf(char *fname);

#endif  // PCX_H
