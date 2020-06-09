// tSB's DirectDraw module for WinV2k

// Portability: Win32 only.  Compiles under mingw32 and MSVC. (untested on all others)
// This is designed to work in a manner as close to the old DOS driver as conceivably possible.
// Thus, tiles, CHRs and such are not stored on surfaces.  Consider changing this after
// the port is complete.

// This, in fact, is technicly triple buffering, since the main buffer is a simple array of chars,
// to mimic the DOS framebuffer.  It is then copied to the back surface, and flipped. (or blitted in windowed mode)

// There is a little bit of window-moving in here, mostly to make windows fit, and such when in windowed mode.
// In fact, the rest of the engine doesn't really care about the window, so this is about the only place where
// it gets toyed with during runtime.

// Note: All of the clipping coords are inclusive.  (0,0)-(319,199) means that 319 is the last pixel to write on the X axis, etc...

#define DIRECTDRAW_VERSION 0X0200

#include "verge.h"

#include <math.h>           // for RotScale

// Comment this to use all the C blitters
#define GR_ASM

bool __cdecl IsMMX(void);   // prototype

GrDriver::GrDriver()
{
 lpdd=NULL;                 // none of these objects are initially allocated, so let's make sure they're all null
 mainsurf=NULL;
 offsurf=NULL;
 mainclip=NULL;
 ddpal=NULL;

 screen=NULL;
 truescreen=NULL; 

 morphlut=NULL;
 lucentlut16=NULL;
 lucentlut8=NULL;

 MMX=IsMMX();               // what the hell, get a jumpstart. ;)
}

GrDriver::~GrDriver()
{
 ShutDown();
}

// This sure beats fishing around ddraw.h :D
void GrDriver::LogDDErr(HRESULT errorcode)
{
 Log("");
 Logp("DirectDraw Error: ");
 switch(errorcode)
  {
   case DDERR_WASSTILLDRAWING:		Log("Was still drawing");				break;
   case DDERR_SURFACELOST:			Log("SurfaceLost");						break;
   case DDERR_INVALIDRECT:			Log("InvalidRect");						break;
   case DDERR_UNSUPPORTED:			Log("Unsupported");						break;
   case DDERR_EXCLUSIVEMODEALREADYSET: Log("Exclusive mode already set");	break;
   case DDERR_HWNDALREADYSET:		Log("hWnd already set");				break;
   case DDERR_INVALIDOBJECT:		Log("Invalid object");					break;
   case DDERR_INVALIDPARAMS:		Log("Invalid Params");					break;
   case DDERR_OUTOFMEMORY:			Log("Out of memory");					break;
   case DDERR_INVALIDPIXELFORMAT:	Log("Invalid pixel format");			break;
   case DDERR_INVALIDCAPS:			Log("Invalid caps");					break;
   case DDERR_UNSUPPORTEDMODE:		Log("Unsupported mode");				break;
  }
 Log("");
}

// if we're fullscreen, then offsurf is attached to the main surface, and we simply flip() in ShowPage.
// if not, then offsurf is just another surface (at the requested colour depth) then, we can blit it to
// the primary and not have to worry about the desktop colour depth. ^_^
// returns 1 on success
// x and y are the resolution, c is the colour depth (in bits per pixel) fullscr is true if we aren't running windowed
int GrDriver::Init(HWND hwnd,int x,int y,int c,bool fullscr)
{
 // TODO: add windowed stuff
 HRESULT result;

                                  // bounds checking
 if (c!=8 && c!=16) return 0;     // TODO: 24/32bit support?
 if (!x     || !y)     return 0;  // a mode with pixels on it would be preferable

 hWnd=hwnd; // save this for later :)

 bpp=c/8; // bpp is in BYTES per pixel, c is in bits.  Adjust.
 xres=x;      yres=y;
 // create lpdd
 result=DirectDrawCreate(NULL,&lpdd,NULL);
 if (result!=DD_OK)
  {
   Log("Failed creating DDraw Object");
   return 0;
  }

 if (SetMode(x,y,c,fullscr))
  return 1; // yay!

 return 0;  // :(
}

int GrDriver::SetMode(int x,int y,int c,bool fs)
{
 // This is completely different depending on whether the engine is running in a window or not
 // FullScreen: Resize the screen, change a few variables, voila!
 // Windowed:   Delete the off surface, recreate it, and resize the client window to fit.
 // TODO: add support for switching bit depths in mid-run

 // if x and y are null, then the resolution stays the same.  Useful for switching to/from windowed mode.
 // TODO: Why doesn't switching to/from windowed mode work?

 HRESULT result;
 DDSURFACEDESC ddsc;
 DDSCAPS ddscaps;

 if (c/8!=bpp)
  {
   Log("Error: Unable to change bit depth at this point.");
   return 0; // not yet
  }

 if (x) xres=x;
 if (y) yres=y;

 bpp=c/8;
 if (truescreen!=NULL)
  delete truescreen;
 truescreen=new byte[xres*yres*bpp];
 RestoreRenderSettings(); // heh, why not? Saves a bit of redundant code :)

 DestroySurfaces();
 fullscreen=fs;

 if (fullscreen)
  {
   // FULLSCREEN MODE CODE GOES HERE
   result=lpdd->SetCooperativeLevel(hWnd,DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
   if (result!=DD_OK)
    {
     Log("Unable to set co-op level");
     LogDDErr(result);
     return 0;
    }
   result=lpdd->SetDisplayMode(x,y,c);
   if (result!=DD_OK)
    {
     Log("Unable to set display mode");
     LogDDErr(result);
     return 0;
    }

   ZeroMemory(&ddsc,sizeof ddsc);
   ddsc.dwSize=sizeof ddsc;
   ddsc.dwFlags=DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
   ddsc.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
   ddsc.dwBackBufferCount=1;
   result=lpdd->CreateSurface(&ddsc,&mainsurf,NULL);
   if (result!=DD_OK)
    {
     Log("Unable to create primary surface");
     LogDDErr(result);
     return 0;
    }

   ddscaps.dwCaps=DDSCAPS_BACKBUFFER;
   result=mainsurf->GetAttachedSurface(&ddscaps,&offsurf);
   if (result!=DD_OK) return 0;
  }
 else
  {
   // WINDOWED MODE CODE HERE 
   result=lpdd->SetCooperativeLevel(hWnd,DDSCL_NORMAL);
   if (result!=DD_OK)
    {
     Log("Error setting co-op mode (windowed)");
     LogDDErr(result);
     return 0;
    }

   ddsc.dwSize=sizeof ddsc;
   ddsc.dwFlags=DDSD_CAPS;
   ddsc.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE;
   result=lpdd->CreateSurface(&ddsc,&mainsurf,NULL);
   if (result!=DD_OK)
    {
     Log("Unable to create primary surface (windowed)");
     LogDDErr(result);
     return 0;
    }

   ZeroMemory(&ddsc,sizeof ddsc);
   ddsc.dwSize=sizeof ddsc;
   ddsc.dwFlags=DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
   ddsc.dwHeight=yres;
   ddsc.dwWidth=xres;
   ddsc.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
   if (bpp==1)
    {
     ddsc.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
     ddsc.ddpfPixelFormat.dwFlags=DDPF_PALETTEINDEXED8 | DDPF_RGB;
     ddsc.ddpfPixelFormat.dwRGBBitCount=8;
    }
   else
    {
     ddsc.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
     ddsc.ddpfPixelFormat.dwFlags=DDPF_RGB;
     ddsc.ddpfPixelFormat.dwRGBBitCount=16;
     ddsc.ddpfPixelFormat.dwBBitMask = 0x001F; // set a 5:6:5 surface.
     ddsc.ddpfPixelFormat.dwGBitMask = 0x07E0;
     ddsc.ddpfPixelFormat.dwRBitMask = 0xF800;
    }
   result=lpdd->CreateSurface(&ddsc,&offsurf,NULL);
   if (result!=DD_OK)
    {
     Log("Error creating back surface (windowed)");
     LogDDErr(result);
     return 0;
    }
  }

 if (bpp==1)
  {
   PALETTEENTRY ddp[256];
   int i;

   for (i=0; i<256; i++)
    {
     ddp[i].peRed  = pal[i*3];
     ddp[i].peBlue = pal[i*3+1];
     ddp[i].peGreen= pal[i*3+2];
     ddp[i].peFlags= 0;
    }

   // create a palette
   result=lpdd->CreatePalette(DDPCAPS_8BIT|DDPCAPS_ALLOW256,ddp,&ddpal,NULL);
   if (result!=DD_OK) return 0;
   mainsurf->SetPalette(ddpal);
  }
 else
  {
   GetPixelFormat();
   trans_mask=PackPixel(255,0,255);
  }

 result=lpdd->CreateClipper(0,&mainclip,NULL);
 if (result!=DD_OK)
  {   Log("Error creating clipper");   return 0;  }

 result=mainclip->SetHWnd(0,hWnd);
 if (result!=DD_OK)
  {   Log("Error setting window handle"); return 0; }

 result=mainsurf->SetClipper(mainclip);
 if (result!=DD_OK)
  {   Log("Error assigning clipper"); return 0; }

 if (!fullscreen)
  MakeClientFit();
 else
  ShowCursor(false);

 sprintf(driverdesc,"DirectDraw %i.%i %ix%ix%i",DIRECTDRAW_VERSION>>8,DIRECTDRAW_VERSION&255,xres,yres,bpp*8);
 return 1;
}

void GrDriver::DestroySurfaces()
{
 // heh, sure are a lot of surfaces to destroy
 if (mainsurf!=NULL) mainsurf->Release(); mainsurf=NULL;
 if (!fullscreen && offsurf !=NULL)  offsurf->Release(), offsurf=NULL;
 if (mainclip!=NULL) mainclip->Release(); mainclip=NULL;
}

void GrDriver::ShutDown()
{
 if (lpdd==NULL) return;

 if (mainclip!=NULL)
  {
   mainclip->Release();     mainclip=NULL;
  }
 if (mainsurf!=NULL)
  {
   mainsurf->Release();     mainsurf=NULL;
  }

 // if we're fullscreen, then offsurf is attached to mainsurf, and thus is destroyed along with it
 // FIXME? offsurf isn't nullified in fullscreen mode after going through this.  Do we care?
 if (offsurf!=NULL && !fullscreen)
  {
   offsurf->Release();      offsurf=NULL;
  }

 if (ddpal!=NULL)
  {
   ddpal->Release();        ddpal=NULL;
  }

 if (screen!=NULL)
  {
   delete screen;
   screen=NULL;
  }

 lpdd->Release();           lpdd=NULL;
}

void GrDriver::MakeClientFit()
{
 RECT client,window,goal;
 int ox,oy; // how far off are we?

 if (fullscreen) return;  // why?
 goal.left=goal.top=0;
 goal.right=xres; goal.bottom=yres;

 GetClientRect(hWnd,&client);
 GetWindowRect(hWnd,&window);

 // find out how much adjustment we need to do
 ox=xres-client.right;
 oy=yres-client.bottom;

 // do it!
 window.right+=ox;
 window.bottom+=oy;

 POINT pt;

 pt.x=pt.y=0;

 ClientToScreen(hWnd,&pt);
// OffsetRect(&window,pt.x,pt.y);

 MoveWindow(hWnd,window.left,window.top,window.right-window.left,window.bottom-window.top,true);
}

void GrDriver::ShowPage()
{
 byte *s,*d;
 quad srcinc,destinc; // incrememt values for the copy loop
 int yl;

 RenderGUI(); // gah! --tSB

 // ooh, pre-emptive surface checking.  I wonder why nobody else does this. :P --tSB
 if (mainsurf->IsLost())
  {
   mainsurf->Restore();
   offsurf->Restore();
  }

 cpubyte=PFLIP;
 if (!Lock()) return;
 s=screen;
 d=(byte*)(ddsd.lpSurface);
 yl=yres;
 if (!morphlut || bpp==1)
  {
   if (MMX)
    {
     srcinc  = xres*bpp;
     destinc = ddsd.lPitch;

     for (; yl; yl--) // TODO: asm
      {
       memcpy(d,s,xres*bpp);
       s+=srcinc;
       d+=destinc;
      } // straight copy
    }
   else
    {
     int quadsperline=xres*bpp/8;
     destinc=ddsd.lPitch-xres*bpp;

  /* Bitchass MMX Enhanced Mem Copier(tm)
     registers:
     esi - source
     edi - dest
     eax - surface pitch-bytes per line
     ebx - xres*bpp/8
     ecx - x loop counter
     edx - y loop counter
     Notes:
     There isn't any code at all for the event of a graphics mode whose width isn't a multiple of 8. 
     (or 16 in the case of 8bit modes)  I can't think of any fullscreen modes that do that, but it might 
     be an issue in windowed mode.
     TODO:
     play with it
  */
     __asm
         {
         mov     esi,s
         mov     edi,d
         mov     eax,destinc
         mov     ebx,quadsperline

         mov     edx,yl

yloop:
         mov     ecx,ebx         // ecx=quadsperline

         xloop:
             movq   mm0,[esi]    // grab a bunch of pixels
             add    esi,8

             movq   [edi],mm0    // dump 'em back
             add    edi,8        // inc the source/dest pointers

         loop    xloop

         add     edi,eax         // dest+=pitch-xres*bpp
     
         dec     edx             // yl--;
         jnz     yloop           // if (yl) goto yloop

         emms                    // clean up the MMX stuff
         }
    }
  }
 else
  {
   srcinc=xres*bpp;
   destinc=ddsd.lPitch;
   word* s16=(word*)s;
   word* d16=(word*)d;
   // colour blending for hicolour palettemorph emulation
   for (; yl; yl--)
    {
     for (int x=0; x<xres; x++)
      {
       d16[x]=morphlut[s16[x]];
      }
     s16+=srcinc/2;
     d16+=destinc/2;
    }
  }
 if (!UnLock()) return;

 HRESULT result;

 if (fullscreen)
  {
   result=mainsurf->Flip(NULL,vsync);
   if (result!=DD_OK && result!=DDERR_WASSTILLDRAWING)
    {
     Log("Flipping error");
     LogDDErr(result);
    }
  }
 else
  {
   // TODO; add windowed showpage
   RECT ClientRect,source;
   HRESULT result;
   POINT pt;

   if (mainsurf==NULL) return;
   if (offsurf==NULL) return;
   GetClientRect(hWnd,&ClientRect);
   pt.x=pt.y=0;

   source.left=0; source.top=0;
   source.right=xres; source.bottom=yres; // TODO: zoom? @_@

   ClientRect.right=ClientRect.left+xres;
   ClientRect.bottom=ClientRect.top+yres;
   ClientToScreen(hWnd,&pt);
   OffsetRect(&ClientRect,pt.x,pt.y);
   result=mainsurf->Blt(&ClientRect,offsurf,&source,DDBLT_WAIT,NULL);
   if (result!=DD_OK)
    {
     Log("Flipping error (windowed mode)");
     LogDDErr(result);
    }
  }
 cpubyte=ETC;
}

int  GrDriver::Lock()
{
 HRESULT result;

 ddsd.dwSize=sizeof ddsd;
 result=offsurf->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
 if (result!=DD_OK)
  {
   Log("Error locking off surface");
   LogDDErr(result);
   return 0;
  }
 return 1;
}

int  GrDriver::UnLock()
{
 HRESULT result;

 result=offsurf->Unlock(ddsd.lpSurface);
 if (result!=DD_OK)
  {
   Log("Error unlocking off surface");
   LogDDErr(result);
   return 0;
  }
 return 1;
}

// accessors
int GrDriver::XRes()
{
 return xres;
}

int GrDriver::YRes()
{
 return yres;
}

bool GrDriver::IsFullScreen()
{
 return fullscreen;
}

char* GrDriver::DriverDesc()
{
 return driverdesc;
}

void GrDriver::Clear()
{
 memset(screen,0,xres*yres*bpp);
}

void GrDriver::VSync(bool on)
{
 if (on)
  vsync=0;
 else
  vsync=DDFLIP_NOVSYNC;
}

// ===================================== OPAQUE BLITS =====================================

void GrDriver::CopySprite(int x,int y,int width,int height,byte* src)
// Assumes the surface is locked!!!
{
  int xl,yl,xs,ys;

  if (screen==NULL) return;

  cpubyte=RENDER;
  xl=width;  yl=height;
  xs=ys=0;
  if (x>=clip.right || y>=clip.bottom || x+xl<=clip.left || y+yl<=clip.top)
    return;

  if (x+xl-1 >= clip.right ) xl=clip.right -x+1;
  if (y+yl-1 >= clip.bottom) yl=clip.bottom-y+1;
  if (x<clip.left) { xs=clip.left-x;   xl-=xs;   x=clip.left; }
  if (y<clip.top ) { ys=clip.top -y;   yl-=ys;   y=clip.top ; }

  if (bpp==2) // blit for hicolour
   {
    word *s16,*d16;

    s16=(word*)(src)+((ys*width)+xs);
    d16=(word*)(screen)+y*scrx+x;
#ifndef GR_ASM
    xl<<=1;
    for (; yl; yl--)
     {
      memcpy(d16,s16,xl);
      s16+=width;
      d16+=scrx;
     }
#else
    // Registers:
    // esi: source
    // edi: dest
    // ecx: x loop counter
    // edx: y loop counter
    // eax: source pointer increment
    // ebx: dest pointer increment
     
    // The ops that are paired are the ones I'm hoping will fit into the u & v pipes
    // TODO: Bitchass MMX stuff?  Unlikely, since ASM doesn't seem to be improving the framerate much.

    // Teeny bit faster than the C version.  The MMX version is a teeny bit faster yet.
    
    int sx=scrx;

//*
    if (MMX)
        {

        int numquads=xl/4;          // (widthinpixels)*bpp/8  (where bpp=2 here)
        int leftover=xl%4;

        __asm
            {
            mov     esi,s16         // esi=s16
            mov     edi,d16         // edi=d16

            mov     eax,width       // eax=width
            mov     ebx,sx          // ebx=sx

            sub     eax,xl          // eax=width-xl

            sub     ebx,xl          // ebx=sx-xl
            shl     eax,1           // eax=(width-xl)*2 (bytes, not pixels)

            shl     ebx,1           // ebx=(sx-xl)*2

            mov     edx,yl          // edx=yl

        yloop16mmx:
                mov     ecx,numquads
                jcxz    leftovers16mmx
           
            xloop16mmx:
                    movq   mm0,[esi]    // grab a bunch of pixels
                    add    esi,8

                    movq   [edi],mm0    // dump 'em back
                    add    edi,8        // inc the source/dest pointers

                loop    xloop16mmx

        leftovers16mmx:
                mov     ecx,leftover
                jcxz    endline16mmx

                rep     movsw       // copy any odd pixels

        endline16mmx:
                add     esi,eax     // move the pointers down to the beginning of the next line
                add     edi,ebx
                
                dec     edx
            jnz     yloop16mmx

            emms                
            }
        }
    else      //*/
        __asm
            {
            mov     esi,s16
            mov     edi,d16
    
            mov     eax,width   // eax=width
            mov     ebx,sx      // ebx=scrx
    
            sub     eax,xl      // eax=width-xl
    
            sub     ebx,xl      // ebx=scrx-xl
            shl     eax,1       // eax=(width-xl)*2; (2 bpp)

            shl     ebx,1       // ebx=(scrx-xl)*2; (2bpp)

            mov     edx,yl      // dx=y length
    yloop16:
                mov     ecx,xl      // ecx=xl (in pixels)
                rep     movsw       // copy a line

            add     esi,eax
            add     edi,ebx
    
            dec     edx
            jnz     yloop16
            }
#endif // GR_ASM
   }
  else // 8bit blit
   {
    byte *s8,*d8;
    s8=src+(ys*width+xs);
    d8=screen+(y*scrx+x);
/*
#ifndef GR_ASM
    if (MMX)
     __asm
        {
        }
	else
#else
//*/    
    for (; yl; yl--)
     {
      memcpy(d8,s8,xl);
      s8+=width;
      d8+=scrx;
     }
   }
}

void GrDriver::TCopySprite(int x,int y,int width,int height,byte* src)
{
  int xl,yl,xs,ys;
  int srci,desti; // source inc, dest inc
    
  if (screen==NULL) return;
 
  cpubyte=RENDER;
  xl=width;
  yl=height;
  xs=ys=0;
  if (x>=clip.right || y>=clip.bottom || x+xl<=clip.left || y+yl<=clip.top)
    return;

  if (x+xl-1> clip.right ) xl=clip.right -x+1;
  if (y+yl-1 > clip.bottom) yl=clip.bottom-y+1;
  if (x<clip.left) { xs=clip.left-x;   xl-=xs;   x=clip.left; }
  if (y<clip.top ) { ys=clip.top -y;   yl-=ys;   y=clip.top ; }

  desti=scrx-xl;
  srci=width-xl;

  if (bpp>1) // blit for hicolour
   {
    word *s16,*d16;

    s16=(word*)(src)+ys*width+xs;
    d16=(word*)(screen)+y*scrx+x;
#ifndef GR_ASM
    for (; yl>0; yl--)
     {
      int a=xl;
      while (a--)
       {
        if (*s16!=trans_mask)
         *d16=*s16;
        d16++; s16++;
       }
      d16+=desti;
      s16+=srci;
     }
#else
    int sx=scrx;
    word t=trans_mask;
    /*
    Registers:
    esi - Source ptr
    edi - dest ptr
    eax - source increment / current pixel (whatever)
    ebx - dest increment
    ecx - x loop counter
    edx - y loop counter
    */
    __asm
        {
        mov     esi,s16
        mov     edi,d16
        
        mov     eax,srci        // eax=srci
        mov     ebx,desti       // ebx=desti

        shl     eax,1           // bytes, not pixels
        shl     ebx,1           // ditto

        mov     edx,yl
    yloop16:
            mov     ecx,xl
            push    ax          // we need ax for awhile
            
        xloop16:
                mov     ax,[esi]    // grab a pixel
                add     esi,2       // increment the source pointer

                cmp     ax,t
                je      nodraw16

                mov     [edi],ax

            nodraw16:
                add     edi,2
                loop    xloop16

            pop     ax
            add     esi,eax
            add     edi,ebx

        dec     edx
        jnz     yloop16

//end16:
        }
#endif
   }
  else // 8bit blit
   {
    byte *s8,*d8;
    s8=src+ys*width+xs;
    d8=screen+y*scrx+x;
#ifndef GR_ASM
    for (; yl; yl--)
     {
      x=xl;
      while (x--)
       {
        if (*s8)
         *d8=*s8;
        d8++; s8++;         
       }
      d8+=desti;
      s8+=srci;
     }
#else    
    /* registers
       esi - source pointer
       edi - dest pointer
       edx - y loop counter
       ecx - x loop counter
       ebx - desti (above)
       eax - srci (above), and the current pixel

    Notes:  MMX isn't feasable for nonopaque blits, so don't even consider it.
            This one improves the framerate by about %8 on my system. (pretty good, IMHO)
    */
    int sx=scrx;            // can't access class members in inline asm.
    __asm
        {
        mov     esi,s8
        mov     edi,d8
        
        mov     edx,yl      // edx=yl
        mov     ebx,desti
        mov     eax,srci

    yloop8:
        mov     ecx,xl      // set up the x loop counter
        push    eax         // we need this register for awhile

        xloop8:
            mov     al,[esi]    // faster than lodsb o_O
            inc     esi

            cmp     al,0        // is it the transparent pixel value?
            je      nodraw8     // yes.  Skip it

            mov     [edi],al    // faster than stosb O_o (even when you take the inc into account)
        nodraw8:
            inc     edi
            loop    xloop8

        pop     eax         // get our source incrementor back
        add     edi,ebx     // d8+=desti;
        add     esi,eax     // s8+=srci;

        dec     edx         // dec the y loop counter
        jnz     yloop8
        }
#endif
   }
  cpubyte=ETC;
}

// Seeing as how I spent the day I got off thanks to Rememberance day porting this code segment,
// I hereby dedicate ScaleSprite to all who died in the War(s).
void GrDriver::ScaleSprite(int x,int y, int iwidth, int iheight,int dwidth,int dheight,byte* src)
{
 int i, j;
 int xerr, yerr;
 int xerr_start, yerr_start;
 int xadj, yadj;
 int xl, yl, xs, ys;
 int c;

 cpubyte = RENDER;

 if (dwidth < 1 || dheight < 1)
  return;

 xl = dwidth;
 yl = dheight;
 xs = ys = 0;
 if (x > clip.right || y > clip.bottom
     || x + xl < clip.left || y + yl < clip.top)
  return;

 if (x+xl-1 > clip.right)   xl = clip.right - x+1;
 if (y+yl-1 > clip.bottom)  yl = clip.bottom - y+1;

 if (x < clip.left) { xs = clip.left - x; xl -= xs; x = clip.left; }
 if (y < clip.top)  { ys = clip.top - y;  yl -= ys; y = clip.top;  }

 xadj = (iwidth << 16)/dwidth;
 yadj = (iheight << 16)/dheight;
 xerr_start = xadj * xs;
 yerr_start = yadj * ys;

 yerr = yerr_start & 0xffff;

 if (bpp==1)
  {
   byte *s8,*d8;

   s8=src+((yerr_start>>16)*iwidth);
   d8=screen+y*scrx+x;

   for (i = 0; i < yl; i += 1)
    {
     xerr = xerr_start;
     for (j = 0; j < xl; j += 1)
      {
       c = s8[(xerr >> 16)];
       d8[j] = c;//translucency_table[d[j] | (c << 8)];
       xerr += xadj;
      }
     d8   += scrx;
     yerr += yadj;
     s8   += (yerr >> 16)*iwidth;
     yerr &= 0xffff;
    }
  }
 else
  {
   word *s16,*d16;

   s16=(word*)(src)+(yerr_start>>16)*iwidth;
   d16=(word*)(screen)+y*scrx+x;

   for (i = 0; i < yl; i += 1)
    {
     xerr = xerr_start;
     for (j = 0; j < xl; j += 1)
      {
       c = s16[(xerr >> 16)];
       d16[j] = c;
       xerr += xadj;
      }
     d16  += scrx;
     yerr += yadj;
     s16  += (yerr >> 16)*iwidth;
     yerr &= 0xffff;
    }
  }
}

void GrDriver::TScaleSprite(int x,int y, int iwidth, int iheight,int dwidth,int dheight,byte* src)
{
 int i, j;
 int xerr, yerr;
 int xerr_start, yerr_start;
 int xadj, yadj;
 int xl, yl, xs, ys;
 int c;

 cpubyte = RENDER;

 if (dwidth < 1 || dheight < 1)
  return;

 xl = dwidth;
 yl = dheight;
 xs = ys = 0;
 if (x > clip.right || y > clip.bottom
     || x + xl < clip.left || y + yl < clip.top)
  return;

 if (x+xl-1 > clip.right)   xl = clip.right - x+1;
 if (y+yl-1 > clip.bottom)  yl = clip.bottom - y+1;

 if (x < clip.left) { xs = clip.left - x; xl -= xs; x = clip.left; }
 if (y < clip.top) { ys = clip.top - y; yl -= ys; y = clip.top; }

 xadj = (iwidth << 16)/dwidth;
 yadj = (iheight << 16)/dheight;
 xerr_start = xadj * xs;
 yerr_start = yadj * ys;

 yerr = yerr_start & 0xffff;

 if (bpp==1)
  {
   byte *s8,*d8;

   s8=src+(ys*iwidth);
   d8=screen+y*scrx+x;

   for (i = 0; i < yl; i += 1)
    {
     xerr = xerr_start;
     for (j = 0; j < xl; j += 1)
      {
       c = s8[(xerr >> 16)];
       if (c)
        d8[j]=c;//translucency_table[d[j] | (c << 8)];
       xerr += xadj;
      }
     d8   += scrx;
     yerr += yadj;
     s8   += (yerr >> 16)*iwidth;
     yerr &= 0xffff;
    }
  }
 else
  {
   word *s16,*d16;

   s16=(word*)(src)+(yerr_start>>16)*iwidth;
   d16=(word*)(screen)+y*scrx+x;

   for (i = 0; i < yl; i += 1)
    {
     xerr = xerr_start;
     for (j = 0; j < xl; j += 1)
      {
       c = s16[(xerr >> 16)];
       if (c!=trans_mask)
        d16[j] = c;
       xerr += xadj;
      }
     d16  += scrx;
     yerr += yadj;
     s16  += (yerr >> 16)*iwidth;
     yerr &= 0xffff;
    }
  }
}

void GrDriver::RotScale(int posx,int posy,int width,int height,float angle,float scale, byte* src)
{
 // new! shamelessly ripped off from alias.zip
 // except the atan2 stuff which i had to make up myself AEN so there :p

 int xs,ys,xl,yl;
 int sinas,cosas,xc,yc,srcx,srcy,x,y,tempx,tempy,T_WIDTH_CENTER,T_HEIGHT_CENTER,W_WIDTH_CENTER,W_HEIGHT_CENTER,W_HEIGHT,W_WIDTH;
 word pt;
 float ft;

 ft=(float)atan2((float)width,(float)height);

 T_WIDTH_CENTER=width>>1;
 T_HEIGHT_CENTER=height>>1;
 W_WIDTH=(int)((float)width/scale*sin(ft) + (float)height/scale*cos(ft));
 W_HEIGHT=W_WIDTH;
 W_HEIGHT_CENTER=W_HEIGHT>>1;
 W_WIDTH_CENTER=W_HEIGHT_CENTER; //W_WIDTH/2;

 sinas=(int)(sin(-angle)*65536*scale);
 cosas=(int)(cos(-angle)*65536*scale);

 xc=T_WIDTH_CENTER*65536 - (W_HEIGHT_CENTER*(cosas+sinas));
 yc=T_HEIGHT_CENTER*65536 - (W_WIDTH_CENTER*(cosas-sinas));
 posx-=W_WIDTH_CENTER;
 posy-=W_HEIGHT_CENTER;

 // clipping
 if (W_WIDTH<2 || W_HEIGHT<2) return;
 xl=W_WIDTH;
 yl=W_HEIGHT;
 xs=ys=0;
 if (posx>clip.right || posy>clip.bottom || posx+xl<clip.left || posy+yl<clip.top)
   return;
 if (posx+xl-1 > clip.right) xl=clip.right-posx+1;
 if (posy+yl-1 > clip.bottom) yl=clip.bottom-posy+1;
 if (posx<clip.left)
  {
   xs=clip.left-posx;
   xl-=xs;
   posx=clip.left;

   xc+=cosas*xs; // woo!
   yc-=sinas*xs;
  }
 if (posy<clip.top)
  {
   ys=clip.top-posy;
   yl-=ys;
   posy=clip.top;

   xc+=sinas*ys; // woo!
   yc+=cosas*ys;
  }


 if (bpp==1)
  {
   byte *s8,*d8;
   d8=screen+posx+posy*scrx;
   s8=src;
   for (y=0; y<yl; y++)
    {
     srcx=xc;
     srcy=yc;

     for (x=0; x<xl; x++)
      {
       tempx=(srcx>>16);
       tempy=(srcy>>16);

       if (tempx>=0 && tempx<width && tempy>=0 && tempy<height)
        {
         pt=s8[tempx+tempy*width];
         if (pt)
          d8[x]=(byte)pt;//dest[x]=translucency_table[(pt<<8)|dest[x]];
        }

       srcx+=cosas;
       srcy-=sinas;
      }

     d8+=scrx;

     xc+=sinas;
     yc+=cosas;
    }
  }
 else
  {
   word *s16,*d16;
   d16=(word*)screen+posx+posy*scrx;
   s16=(word*)src;
   for (y=0; y<yl; y++)
    {
     srcx=xc;
     srcy=yc;

     for (x=0; x<xl; x++)
      {
       tempx=(srcx>>16);
       tempy=(srcy>>16);

       if (tempx>=0 && tempx<width && tempy>=0 && tempy<height)
        {
         pt=s16[tempx+tempy*width];
         if (pt!=trans_mask)
          d16[x]=pt;//dest[x]=translucency_table[(pt<<8)|dest[x]];
        }

       srcx+=cosas;
       srcy-=sinas;
      }

     d16+=scrx;

     xc+=sinas;
     yc+=cosas;
    }
  }
}

void GrDriver::WrapBlit(int x,int y,int width,int height, byte* src)
{
 int cur_x, sign_y;

 if (width < 1 || height < 1)
  return;

 x %= width, y %= height;
 sign_y = 0 - y;

 for (; sign_y < scry; sign_y += height)
  {
   for (cur_x = 0 - x; cur_x < scrx; cur_x += width)
    CopySprite(cur_x, sign_y, width, height, src);
  }
}

void GrDriver::TWrapBlit(int x,int y,int width,int height, byte* src)
{
 int cur_x, sign_y;

 if (width < 1 || height < 1)
  return;

 x %= width, y %= height;
 sign_y = 0 - y;

 for (; sign_y < scry; sign_y += height)
  {
   for (cur_x = 0 - x; cur_x < scrx; cur_x += width)
    TCopySprite(cur_x, sign_y, width, height, src);
  }
}

// ===================================== TRANSLUCENT BLITS =====================================

inline void GrDriver::SetPixelLucent(word* dest,int c,int lucentmode)
// only for hicolour.  It should be noted that this simply accepts a pointer to 
// dest, insted of x and y coordinates.  This is simply because the blits 
// themselves can calculate dest more efficiently themselves.  (speed is good)
// TODO: ASM!  Everywhere, anywhere!
{
 int  rs,gs,bs; // rgb of source pixel
 int  rd,gd,bd; // rgb of dest pixel

 switch (lucentmode)
  {
   case 0: *dest=c; return; // wtf? oh well, return it anyway
   case 1: *dest=((*dest&lucentmask)+(c&lucentmask))>>1; return; // ooh easy stuff.
   case 2: // variable lucency
           UnPackPixel(*dest,rs,gs,bs);
           UnPackPixel(c,rd,gd,bd);
           rd=lucentlut16[rd<<8|rs];
           gd=lucentlut16[gd<<8|gs];
           bd=lucentlut16[bd<<8|bs];
           *dest=PackPixel(rd,gd,bd);         
           return;
   case 3: // addition
           UnPackPixel(*dest,rs,gs,bs);
           UnPackPixel(c,rd,gd,bd);
           rs+=rd; gs+=gd; bs+=bd;
           if (rs>255) rs=255;
           if (gs>255) gs=255;
           if (bs>255) bs=255;
           *dest=PackPixel(rs,gs,bs);
           return;
   case 4: // subtraction
           UnPackPixel(*dest,rs,gs,bs);
           UnPackPixel(c,rd,gd,bd);
           rs-=rd; gs-=gd; bs-=bd;
           if (rs<0) rs=0;
           if (gs<0) gs=0;
           if (bs<0) bs=0;
           *dest=PackPixel(rs,gs,bs);
           return;
   /* this is quasi-slick, IMO.
      Instead of dividing the colour values, it simply blends them with black,
      using the variable lucency table.  In effect, doing the same thing, but fasta! --tSB*/
   case 5: UnPackPixel(*dest,rs,gs,bs);
           UnPackPixel(c,rd,gd,bd);
           rd=lucentlut16[rd<<8];
           gd=lucentlut16[gd<<8];
           bd=lucentlut16[bd<<8];
           rs+=rd; gs+=gd; bs+=bd;
           if (rs>255) rs=255;
           if (gs>255) gs=255;
           if (bs>255) bs=255;
           *dest=PackPixel(rs,gs,bs);        
           return;
           
   case 6: UnPackPixel(*dest,rs,gs,bs);
           UnPackPixel(c,rd,gd,bd);
           rd=lucentlut16[rd<<8];
           gd=lucentlut16[gd<<8];
           bd=lucentlut16[bd<<8];
           rs-=rd; gs-=gd; bs-=bd;
           if (rs<0) rs=0;
           if (gs<0) gs=0;
           if (bs<0) bs=0;
           *dest=PackPixel(rs,gs,bs);        
           return;
  }
}

void GrDriver::CopySpriteLucent(int x,int y,int width,int height,byte* src,int lucentmode)
{
  int xl,yl,xs,ys;
  int a;

  if (screen==NULL) return;

  cpubyte=RENDER;
  xl=width;
  yl=height;
  xs=ys=0;
  if (x>clip.right || y>clip.bottom || x+xl<clip.left || y+yl<clip.top)
    return;

  if (x+xl-1 > clip.right ) xl=clip.right -x+1;
  if (y+yl-1 > clip.bottom) yl=clip.bottom-y+1;
  if (x<clip.left) { xs=clip.left-x;   xl-=xs;   x=clip.left; }
  if (y<clip.top ) { ys=clip.top -y;   yl-=ys;   y=clip.top ; }

  if (bpp>1) // blit for hicolour
   {
    word *s16,*d16;

    s16=(word*)(src)+ys*width+xs;
    d16=(word*)(screen)+y*scrx+x;
    for (; yl; yl--) // TODO: asm
     {
      for (a=0; a<xl; a++)
       SetPixelLucent(&d16[a],s16[a],lucentmode);
      s16+=width;
      d16+=scrx;
     }
   }
  else // 8bit blit
   {
    byte *s8,*d8;
    s8=src+ys*width+xs;
    d8=screen+y*scrx+x;

    for (; yl; yl--) // TODO: asm
     {
      for (a=0; a<xl; a++)
       d8[a]=lucentlut8[d8[a]|(s8[a]<<8)]; // wee
      s8+=width;
      d8+=scrx;
     }
   }
  cpubyte=ETC;
}

void GrDriver::TCopySpriteLucent(int x,int y,int width,int height,byte* src,int lucentmode)
{
  int xl,yl,xs,ys;
  int a;

  if (screen==NULL) return;

  cpubyte=RENDER;
  xl=width;
  yl=height;
  xs=ys=0;
  if (x>clip.right || y>clip.bottom || x+xl<clip.left || y+yl<clip.top)
    return;

  if (x+xl-1 > clip.right ) xl=clip.right -x+1;
  if (y+yl-1 > clip.bottom) yl=clip.bottom-y+1;
  if (x<clip.left) { xs=clip.left-x;   xl-=xs;   x=clip.left; }
  if (y<clip.top ) { ys=clip.top -y;   yl-=ys;   y=clip.top ; }

  if (bpp>1) // blit for hicolour
   {
    word *s16,*d16;

    s16=(word*)(src)+ys*width+xs;
    d16=(word*)(screen)+y*scrx+x;
//    xl=xl*2;
    for (; yl; yl--) // TODO: asm
     {
      for (a=0; a<xl; a++)
       if (s16[a]!=trans_mask)
        SetPixelLucent(&d16[a],s16[a],lucentmode);
      s16+=width;
      d16+=scrx;
     }
   }
  else // 8bit blit
   {
    byte *s8,*d8;
    s8=src+ys*width+xs;
    d8=screen+y*scrx+x;

    for (; yl; yl--) // TODO: asm
     {
      for (a=0; a<xl; a++)
       if (s8[a])
        d8[a]=lucentlut8[d8[a]|(s8[a]<<8)]; // wee
      s8+=width;
      d8+=scrx;
     }
   }
  cpubyte=ETC;
}

void GrDriver::ScaleSpriteLucent(int x,int y, int iwidth, int iheight,int dwidth,int dheight,byte* src,int lucent)
{
 int i, j;
 int xerr, yerr;
 int xerr_start, yerr_start;
 int xadj, yadj;
 int xl, yl, xs, ys;
 int c;

 cpubyte = RENDER;

 if (dwidth < 1 || dheight < 1)
  return;

 xl = dwidth;
 yl = dheight;
 xs = ys = 0;
 if (x > clip.right || y > clip.bottom
     || x + xl < clip.left || y + yl < clip.top)
  return;

 if (x+xl-1 > clip.right)   xl = clip.right - x + 1;
 if (y+yl-1 > clip.bottom)  yl = clip.bottom - y + 1;

 if (x < clip.left) { xs = clip.left - x; xl -= xs; x = clip.left; }
 if (y < clip.top) { ys = clip.top - y; yl -= ys; y = clip.top; }

 xadj = (iwidth << 16)/dwidth;
 yadj = (iheight << 16)/dheight;
 xerr_start = xadj * xs;
 yerr_start = yadj * ys;

 yerr = yerr_start & 0xffff;

 if (bpp==1)
  {
   byte *s8,*d8;

   s8=src+(ys*iwidth);
   d8=screen+y*scrx+x;

   for (i = 0; i < yl; i += 1)
    {
     xerr = xerr_start;
     for (j = 0; j < xl; j += 1)
      {
       c = s8[(xerr >> 16)];
       d8[j]=lucentlut8[d8[j] | (c << 8)];
       xerr += xadj;
      }
     d8   += scrx;
     yerr += yadj;
     s8   += (yerr >> 16)*iwidth;
     yerr &= 0xffff;
    }
  }
 else
  {
   word *s16,*d16;

   s16=(word*)(src)+(yerr_start>>16)*iwidth;
   d16=(word*)(screen)+y*scrx+x;

   for (i = 0; i < yl; i += 1)
    {
     xerr = xerr_start;
     for (j = 0; j < xl; j += 1)
      {
       c = s16[(xerr >> 16)];
//       if (c!=trans_mask)
        SetPixelLucent(&d16[j],c,lucent);//d16[j] = c;
       xerr += xadj;
      }
     d16  += scrx;
     yerr += yadj;
     s16  += (yerr >> 16)*iwidth;
     yerr &= 0xffff;
    }
  }
}

void GrDriver::TScaleSpriteLucent(int x,int y, int iwidth, int iheight,int dwidth,int dheight,byte* src,int lucent)
{
 int i, j;
 int xerr, yerr;
 int xerr_start, yerr_start;
 int xadj, yadj;
 int xl, yl, xs, ys;
 int c;

 cpubyte = RENDER;

 if (dwidth < 1 || dheight < 1)
  return;

 xl = dwidth;
 yl = dheight;
 xs = ys = 0;
 if (x > clip.right || y > clip.bottom
     || x + xl < clip.left || y + yl < clip.top)
  return;

 if (x+xl-1 > clip.right)   xl = clip.right - x + 1;
 if (y+yl-1 > clip.bottom)  yl = clip.bottom - y + 1;

 if (x < clip.left) { xs = clip.left - x; xl -= xs; x = clip.left; }
 if (y < clip.top) { ys = clip.top - y; yl -= ys; y = clip.top; }

 xadj = (iwidth << 16)/dwidth;
 yadj = (iheight << 16)/dheight;
 xerr_start = xadj * xs;
 yerr_start = yadj * ys;

 yerr = yerr_start & 0xffff;

 if (bpp==1)
  {
   byte *s8,*d8;

   s8=src+(ys*iwidth);
   d8=screen+y*scrx+x;

   for (i = 0; i < yl; i += 1)
    {
     xerr = xerr_start;
     for (j = 0; j < xl; j += 1)
      {
       c = s8[(xerr >> 16)];
       if (c)
        d8[j] = c;//translucency_table[d[j] | (c << 8)];
       xerr += xadj;
      }
     d8   += scrx;
     yerr += yadj;
     s8   += (yerr >> 16)*iwidth;
     yerr &= 0xffff;
    }
  }
 else
  {
   word *s16,*d16;

   s16=(word*)(src)+(yerr_start>>16)*iwidth;
   d16=(word*)(screen)+y*scrx+x;

   for (i = 0; i < yl; i += 1)
    {
     xerr = xerr_start;
     for (j = 0; j < xl; j += 1)
      {
       c = s16[(xerr >> 16)];
       if (c!=trans_mask)
        SetPixelLucent(&d16[j],c,lucent);//d16[j] = c;
       xerr += xadj;
      }
     d16  += scrx;
     yerr += yadj;
     s16  += (yerr >> 16)*iwidth;
     yerr &= 0xffff;
    }
  }
}

void GrDriver::RotScaleLucent(int posx,int posy,int width,int height,float angle,float scale, byte* src,int lucent)
{
 // new! shamelessly ripped off from alias.zip
 // except the atan2 stuff which i had to make up myself AEN so there :p

 int xs,ys,xl,yl;
 int sinas,cosas,xc,yc,srcx,srcy,x,y,tempx,tempy,T_WIDTH_CENTER,T_HEIGHT_CENTER,W_WIDTH_CENTER,W_HEIGHT_CENTER,W_HEIGHT,W_WIDTH;
 word pt;
 float ft;

 ft=(float)atan2((float)width,(float)height);

 T_WIDTH_CENTER=width>>1;
 T_HEIGHT_CENTER=height>>1;
 W_WIDTH=(int)((float)width/scale*sin(ft) + (float)height/scale*cos(ft));
 W_HEIGHT=W_WIDTH;
 W_HEIGHT_CENTER=W_HEIGHT>>1;
 W_WIDTH_CENTER=W_HEIGHT_CENTER; //W_WIDTH/2;

 sinas=(int)(sin(-angle)*65536*scale);
 cosas=(int)(cos(-angle)*65536*scale);

 xc=T_WIDTH_CENTER*65536 - (W_HEIGHT_CENTER*(cosas+sinas));
 yc=T_HEIGHT_CENTER*65536 - (W_WIDTH_CENTER*(cosas-sinas));
 posx-=W_WIDTH_CENTER;
 posy-=W_HEIGHT_CENTER;

 // clipping
 if (W_WIDTH<2 || W_HEIGHT<2) return;
 xl=W_WIDTH;
 yl=W_HEIGHT;
 xs=ys=0;
 if (posx>clip.right || posy>clip.bottom || posx+xl<clip.left || posy+yl<clip.top)
   return;
 if (posx+xl-1 > clip.right) xl=clip.right-posx+1;
 if (posy+yl-1 > clip.bottom) yl=clip.bottom-posy+1;
 if (posx<clip.left)
  {
   xs=clip.left-posx;
   xl-=xs;
   posx=clip.left;

   xc+=cosas*xs; // woo!
   yc-=sinas*xs;
  }
 if (posy<clip.top)
  {
   ys=clip.top-posy;
   yl-=ys;
   posy=clip.top;

   xc+=sinas*ys; // woo!
   yc+=cosas*ys;
  }


 if (bpp==1)
  {
   byte *s8,*d8;
   d8=screen+posx+posy*scrx;
   s8=src;
   for (y=0; y<yl; y++)
    {
     srcx=xc;
     srcy=yc;

     for (x=0; x<xl; x++)
      {
       tempx=(srcx>>16);
       tempy=(srcy>>16);

       if (tempx>=0 && tempx<width && tempy>=0 && tempy<height)
        {
         pt=s8[tempx+tempy*width];
         if (pt)
          d8[x]=lucentlut8[(pt<<8)|d8[x]];
        }

       srcx+=cosas;
       srcy-=sinas;
      }

     d8+=scrx;

     xc+=sinas;
     yc+=cosas;
    }
  }
 else
  {
   word *s16,*d16;
   d16=(word*)(screen)+posy*scrx+posx;
   s16=(word*)src;
   for (y=0; y<yl; y++)
    {
     srcx=xc;
     srcy=yc;

     for (x=0; x<xl; x++)
      {
       tempx=(srcx>>16);
       tempy=(srcy>>16);

       if (tempx>=0 && tempx<width && tempy>=0 && tempy<height)
        {
         pt=s16[tempx+tempy*width];
         if (pt)
          SetPixelLucent(&d16[x],pt,lucent);//d16[x]=pt;//dest[x]=translucency_table[(pt<<8)|dest[x]];
        }

       srcx+=cosas;
       srcy-=sinas;
      }

     d16+=scrx;

     xc+=sinas;
     yc+=cosas;
    }
  }
}

void GrDriver::BlitStipple(int x,int y,int colour)
{
  int xl,yl,xs,ys;
  int ax,ay;

  xl=16;
  yl=16;
  xs=ys=0;
  if (x>clip.right || y>clip.bottom || x+xl<clip.left || y+yl<clip.top)
    return;
  if (x+xl-1 > clip.right)  xl=clip.right-x+1;
  if (y+yl-1 > clip.bottom) yl=clip.bottom-y+1;
  if (x<clip.left) { xl+=x-clip.left;   x=clip.left; }
  if (y<clip.top ) { yl+=y-clip.top;   y=clip.top ; }

 if (bpp==1)
  {
   byte* d8;
   d8=screen+y*scrx+x;
   for (ay=0; ay<yl; ay++)
    {
     for (ax=0; ax<xl; ax++)
      if ((ax+ay)&1) d8[ax]=colour;
     d8+=scrx;
    }
  }
 else
  {
   word* d16;
   d16=(word*)(screen)+y*scrx+x;
   for (ay=0; ay<yl; ay++)
    {
     ax=xl;
     while (ax--)
      SetPixelLucent(d16++,colour,1);
     d16+=scrx-xl;
    }
  }
}

void GrDriver::WrapBlitLucent(int x,int y,int width,int height, byte* src,int lucent)
{
 int cur_x, sign_y;

 if (width < 1 || height < 1)
  return;

 x %= width, y %= height;
 sign_y = 0 - y;

 for (; sign_y < scry; sign_y += height)
  {
   for (cur_x = 0 - x; cur_x < scrx; cur_x += width)
    CopySpriteLucent(cur_x, sign_y, width, height, src,lucent);
  }
}

void GrDriver::TWrapBlitLucent(int x,int y,int width,int height, byte* src,int lucent)
{
 int cur_x, sign_y;

 if (width < 1 || height < 1)
  return;

 x %= width, y %= height;
 sign_y = 0 - y;

 for (; sign_y < scry; sign_y += height)
  {
   for (cur_x = 0 - x; cur_x < scrx; cur_x += width)
    TCopySpriteLucent(cur_x, sign_y, width, height, src,lucent);
  }
}


// primitives

void GrDriver::SetPixel(int x,int y,int colour,int lucent)
{
 int ofs;
 ofs=y*scrx+x;
 
 if (x<clip.left || y<clip.top || x>clip.right || y>clip.bottom) return;

 if (bpp==1)
  {
   byte* p=screen+ofs;
   if (lucent)
    *p=lucentlut8[colour || *p<<8];
   else
    *p=colour;
  }
 else 
  {
   word* p=(word*)screen+ofs;
   if (lucent)
    SetPixelLucent(p,colour,lucent);
   else
    *p=colour;
  }
}

int  GrDriver::GetPixel(int x,int y)
{
 if (x<clip.left || y<clip.top || x>clip.right || y>clip.bottom) return 0;
 
 if (bpp==1)
  {
   byte *c;
   c=screen+y*scrx+x;
   return *c;
  }
 else
  {  
   word *w;
   w=(word*)screen+y*scrx+x;
   return *w;
  }

}

void GrDriver::HLine(int x,int y,int x2,int colour,int lucent)
{
 int xl;
// range checking
 if (x>x2)
  {
   int a=x2;
   x2=x;
   x=a;
  } // swap 'em

 if (x<clip.left) x=clip.left;
 if (x2>clip.right) x2=clip.right;

 // Is the line even onscreen?
 if (y<clip.top)  return;
 if (y>clip.bottom) return;
 if (x>clip.right) return;
 if (x2<clip.left) return;

 if (bpp==1)
  {
   byte *d8;
   d8=screen+(y*scrx+x);
   xl=x2-x+1;
   if (lucent)
    while (xl--)
     *d8=lucentlut8[*d8++|(colour<<8)];
   else
    memset(d8,(char)colour,xl); // woo. hard
  }
 else
  {
   word* d16;
   d16=(word*)(screen)+y*scrx+x;
   xl=x2-x+1;
   if (lucent)
    while (xl--)
     SetPixelLucent(d16++,colour,lucent);
   else
    while (xl--)
     *d16++=(word)colour; // woo. also hard.
  }
}

void GrDriver::VLine(int x,int y,int y2,int colour,int lucent)
{
 if (y>y2)
  {
   int a=y2;
   y2=y;
   y=a;
  } // swap 'em

 if (y<clip.top) y=clip.top;
 if (y2>clip.bottom) y2=clip.bottom;

 // Is the line even onscreen?
 if (y2<clip.top)  return;
 if (y>clip.bottom) return;
 if (x>clip.right) return;
 if (x<clip.left) return;

 if (bpp==1)
  {
   byte *d8;
   d8=screen+(y*scrx+x);
   int yl=y2-y+1;
   if (lucent)
    while (yl--)
     {
      *d8=lucentlut8[*d8|(colour<<8)];
      d8+=scrx;
     }
   else
    while (yl--)
     {
      *d8=(byte)colour;
      d8+=scrx;
     }
  }
 else
  {
   word* d16;
   d16=(word*)(screen)+y*scrx+x;
   int yl=y2-y+1;
   if (lucent)
    while (yl--)
     {
      SetPixelLucent(d16,colour,lucent);
      d16+=scrx;
     }
   else
    while (yl--)
     {
      *d16=(word)colour;
      d16+=scrx;
     }
  }
}

void swap(int& a,int& b)
{
 int c;
 c=a;
 a=b;
 b=c;
}

void GrDriver::Line(int x1,int y1,int x2,int y2,int colour,int lucent)
{
  short i,xc,yc,er,n,m,xi,yi,xcxi,ycyi,xcyi;
  unsigned dcy,dcx;

  cpubyte=RENDER;
  // check to see if the line is completly clipped off
  if ((x1<clip.left && x2<clip.left) || (x1>clip.right && x2>clip.right)
  || (y1<clip.top && y2<clip.top) || (y1>clip.bottom && y2>clip.bottom))
  {
    cpubyte=ETC;
    return;
  }

  if (x1>x2)
  {
    swap(x1,x2);
    swap(y1,y2);
  }

  // clip the left side
  if (x1<clip.left)
  { int myy=(y2-y1);
    int mxx=(x2-x1),b;
    if (!mxx)
    {
      cpubyte=ETC;
      return;
    }
    if (myy)
    {
      b=y1-(y2-y1)*x1/mxx;
      y1=myy*clip.left/mxx+b;
      x1=clip.left;
    }
    else x1=clip.left;
  }

  // clip the right side
  if (x2>clip.right)
  { int myy=(y2-y1);
    int mxx=(x2-x1),b;
    if (!mxx)
    {
      cpubyte=ETC;
      return;
    }
    if (myy)
    {
      b=y1-(y2-y1)*x1/mxx;
      y2=myy*clip.right/mxx+b;
      x2=clip.right;
    }
    else x2=clip.right;
  }

  if (y1>y2)
  {
    swap(x1,x2);
    swap(y1,y2);
  }

  // clip the bottom
  if (y2>clip.bottom)
  { int mxx=(x2-x1);
    int myy=(y2-y1),b;
    if (!myy)
    {
      cpubyte=ETC;
      return;
    }
    if (mxx)
    {
      b=y1-(y2-y1)*x1/mxx;
      x2=(clip.bottom-b)*mxx/myy;
      y2=clip.bottom;
    }
    else y2=clip.bottom;
  }

  // clip the top
  if (y1<clip.top)
  { int mxx=(x2-x1);
    int myy=(y2-y1),b;
    if (!myy)
    {
      cpubyte=ETC;
      return;
    }
    if (mxx)
    {
      b=y1-(y2-y1)*x1/mxx;
      x1=(clip.top-b)*mxx/myy;
      y1=clip.top;
    }
    else y1=clip.top;
  }

  // ???
  // see if it got cliped into the box, out out
  if (x1<clip.left || x2<clip.left || x1>clip.right || x2>clip.right || y1<clip.top || y2 <clip.top || y1>clip.bottom || y2>clip.bottom)
  {
    cpubyte=ETC;
    return;
  }

  if (x1>x2)
  { xc=x2; xi=x1; }
  else { xi=x2; xc=x1; }

  // assume y1<=y2 from above swap operation
  yi=y2; yc=y1;

  dcx=x1; dcy=y1;
  xc=(x2-x1); yc=(y2-y1);
  if (xc<0) xi=-1; else xi=1;
  if (yc<0) yi=-1; else yi=1;
  n=abs(xc); m=abs(yc);
  ycyi=abs(2*yc*xi);
  er=0;

  if (bpp==1)
   {
    if (n>m)
    {
      xcxi=abs(2*xc*xi);
      if (lucent)
       for (i=0;i<=n;i++)
       {
         screen[(dcy*scrx)+dcx]=lucentlut8[colour|screen[(dcy*scrx)+dcx]<<8];
         if (er>0)
         { dcy+=yi;
           er-=xcxi;
         }
         er+=ycyi;
         dcx+=xi;
       }    
      else
       for (i=0;i<=n;i++)
       {
        screen[(dcy*scrx)+dcx]=colour;
        if (er>0)
        { dcy+=yi;
          er-=xcxi;
        }
        er+=ycyi;
        dcx+=xi;
      }
    }
    else
    {
      xcyi=abs(2*xc*yi);
      if (lucent)
       for (i=0;i<=m;i++)
       {
         screen[(dcy*scrx)+dcx]=lucentlut8[colour|screen[(dcy*scrx)+dcx]<<8];
         if (er>0)
         { dcx+=xi;
           er-=ycyi;
         }
         er+=xcyi;
         dcy+=yi;
       }
      else
       for (i=0;i<=m;i++)
       {
         screen[(dcy*scrx)+dcx]=colour;
         if (er>0)
         { dcx+=xi;
           er-=ycyi;
         }
         er+=xcyi;
         dcy+=yi;
       }
    }
   }
  else
   {
    word* s16=(word*) screen;
    if (n>m)
    {
      xcxi=abs(2*xc*xi);
      if (lucent)
       for (i=0;i<=n;i++)
       {
//         s16[(dcy*scrx)+dcx]=colour;
         SetPixelLucent(&s16[(dcy*scrx)+dcx],colour,lucent);
         if (er>0)
         { dcy+=yi;
           er-=xcxi;
         }
         er+=ycyi;
         dcx+=xi;
       }
      else
       for (i=0;i<=n;i++)
       {
         s16[(dcy*scrx)+dcx]=colour;
         if (er>0)
         { dcy+=yi;
           er-=xcxi;
         }
         er+=ycyi;
         dcx+=xi;
       }
    }
    else
    {
      xcyi=abs(2*xc*yi);
      for (i=0;i<=m;i++)
      {
        s16[(dcy*scrx)+dcx]=colour;
        if (er>0)
        { dcx+=xi;
          er-=ycyi;
        }
        er+=xcyi;
        dcy+=yi;
      }
    }
   }
  cpubyte=ETC;
  return;
}

void GrDriver::Rect(int x1,int y1,int x2,int y2,int colour,int lucent)
{
 int a;
 if (x1>x2) { a=x1; x1=x2; x2=a; } // swap 'em
 if (y1>y2) { a=y1; y1=y2; y2=a; }

 // range checking.  Bleh, hline and vline do it anyway. --tSB
/* if (x1<clip.left) x1=clip.left;
 if (x2>clip.right) x2=clip.right;
 if (y1<clip.top) y1=clip.top;
 if (y2>clip.bottom) y2=clip.bottom;*/

 VLine(x1,y1,y2,colour,lucent);
 VLine(x2,y1,y2,colour,lucent);
 HLine(x1,y1,x2,colour,lucent);
 HLine(x1,y2,x2,colour,lucent);
}

void GrDriver::RectFill(int x1,int y1,int x2,int y2,int colour,int lucent)
{
 int a;
 if (x1>x2) { a=x1; x1=x2; x2=a; } // swap 'em
 if (y1>y2) { a=y1; y1=y2; y2=a; }

 // range checking
 if (x1<clip.left) x1=clip.left;
 if (x2>clip.right) x2=clip.right;
 if (y1<clip.top) y1=clip.top;
 if (y2>clip.bottom) y2=clip.bottom;


 for (a=y1; a<=y2; a++)
  HLine(x1,a,x2,colour,lucent);
}

void GrDriver::Circle(int x,int y,int radius,int colour,int lucent)
{
 int cx = 0;
 int cy = radius;
 int df = 1 - radius;
 int d_e = 3;
 int d_se = -2*radius + 5;

 cpubyte=RENDER;
    
 do
  {
   SetPixel(x + cx, y + cy, colour, lucent);
   if (cx) SetPixel(x - cx, y + cy, colour, lucent);
   if (cy) SetPixel(x + cx, y - cy, colour, lucent);
   if (cx && cy) SetPixel(x - cx, y - cy, colour, lucent);

   if (cx != cy)
    {
     SetPixel(x + cy, y + cx, colour, lucent);
     if (cx) SetPixel(x + cy, y - cx, colour, lucent);
     if (cy) SetPixel(x - cy, y + cx, colour, lucent);
     if (cx && cy) SetPixel(x - cy, y - cx, colour, lucent);
    }

   if (df < 0)
    {
     df += d_e;
     d_e += 2;
     d_se += 2;
    }
   else
    {
     df += d_se;
     d_e += 2;
     d_se += 4;
     cy -= 1;
    }
   cx += 1;
  }
 while (cx <= cy);

 cpubyte = ETC;
}




void GrDriver::CircleFill(int x,int y,int radius,int colour,int lucent)
{
 int cx = 0;
 int cy = radius;
 int df = 1 - radius;
 int d_e = 3;
 int d_se = -2*radius + 5;

 cpubyte = RENDER;

 do
  {
   HLine(x - cy, y - cx, x + cy, colour, lucent);
   if (cx) HLine(x - cy, y + cx, x + cy, colour, lucent);
   
   if (df < 0)
    {
     df += d_e;
     d_e += 2;
     d_se += 2;
    }
   else
    {
     if (cx != cy)
      {
       HLine(x - cx, y - cy, x + cx, colour, lucent);
       if (cy) HLine(x - cx, y + cy, x + cx, colour, lucent);
      }
     df += d_se;
     d_e += 2;
     d_se += 4;
     cy -= 1;
    }
   cx += 1;
  }
 while (cx <= cy);

 cpubyte = ETC;
}

void GrDriver::FlatPoly(int x1,int y1,int x2,int y2,int x3,int y3,int colour)
{
  int xstep,xstep2;
  int xval,xval2;
  int yon;
  int swaptemp;

  if (y1 > y3)
  {
    swaptemp=x1; x1=x3; x3=swaptemp;
    swaptemp=y1; y1=y3; y3=swaptemp;
  }
  if (y2 > y3)
  {
    swaptemp=x2; x2=x3; x3=swaptemp;
    swaptemp=y2; y2=y3; y3=swaptemp;
  }
  if (y1 > y2)
  {
    swaptemp=x1; x1=x2; x2=swaptemp;
    swaptemp=y1; y1=y2; y2=swaptemp;
  }

  xstep2=((x3-x1) << 16) / (y3-y1);
  xval2=x1 << 16;

  if (y1 != y2)
  {
    xstep = ((x2-x1) << 16) / (y2-y1);
    xval = x1 << 16;
    for (yon=y1;yon < y2; yon++)
    {
      if ((yon > clip.top) && (yon < clip.bottom))
      {
        HLine(xval>>16,yon,xval2>>16,colour,0);
      }
      xval+=xstep;
      xval2+=xstep2;
    }
  }
  if (y2 != y3)
  {
    xstep = ((x3-x2) << 16) / (y3-y2);
    xval = x2 << 16;
    for (yon=y2;yon < y3; yon++)
    {
      if ((yon > clip.top) && (yon < clip.bottom))
      {
        HLine(xval>>16,yon,xval2>>16,colour,0);
      }
      xval+=xstep;
      xval2+=xstep2;
    }
  }
}

inline void GrDriver::tmaphline(int x1, int x2, int y, int tx1, int tx2, int ty1, int ty2,int texw,int texh,byte* image)
{
  int i;
  int txstep,txval;
  int tystep,tyval;

  if (x1 != x2)
  {
    if (x2 < x1)
    {
      i=x1; x1=x2; x2=i;
      i=tx1; tx1=tx2; tx2=i;
      i=ty1; ty1=ty2; ty2=i;
    }
    if ((x1 > scrx) || (x2 < 0)) return;
    txstep=((tx2-tx1)<<16)/(x2-x1);
    tystep=((ty2-ty1)<<16)/(x2-x1);
    txval=tx1<<16;
    tyval=ty1<<16;
    word* s16=(word*)screen;
    word* d16=(word*)image;

    if (bpp==1)
     for (i=x1;i<x2;i++)
      {
       screen[y*scrx+i] = image[(tyval>>16)*texw+(txval>>16)];
       txval+=txstep;
       tyval+=tystep;
      }
    else
     for (i=x1;i<x2;i++)
      {
       s16[y*scrx+i] = d16[(tyval>>16)*texw+(txval>>16)];
       txval+=txstep;
       tyval+=tystep;
      }
    }
}

void GrDriver::TMapPoly(int x1, int y1, int x2, int y2, int x3, int y3,
              int tx1, int ty1, int tx2, int ty2, int tx3, int ty3,
              int tw, int th, byte *img)
{
  int xstep,xstep2;
  int xval,xval2;
  int txstep,txstep2;
  int tystep,tystep2;
  int txval,txval2;
  int tyval,tyval2;
  int yon;
  int swaptemp;

  byte* image;
  int texw,texh;

  image=img; texw=tw; texh=th;
  if (y1 > y3)
  {
    swaptemp=x1; x1=x3; x3=swaptemp;
    swaptemp=y1; y1=y3; y3=swaptemp;
    swaptemp=tx1; tx1=tx3; tx3=swaptemp;
    swaptemp=ty1; ty1=ty3; ty3=swaptemp;
  }
  if (y2 > y3)
  {
    swaptemp=x2; x2=x3; x3=swaptemp;
    swaptemp=y2; y2=y3; y3=swaptemp;
    swaptemp=tx2; tx2=tx3; tx3=swaptemp;
    swaptemp=ty2; ty2=ty3; ty3=swaptemp;
  }
  if (y1 > y2)
  {
    swaptemp=x1; x1=x2; x2=swaptemp;
    swaptemp=y1; y1=y2; y2=swaptemp;
    swaptemp=tx1; tx1=tx2; tx2=swaptemp;
    swaptemp=ty1; ty1=ty2; ty2=swaptemp;
  }
  xstep2=((x3-x1) << 16) / (y3-y1);
  xval2=x1 << 16;
  txstep2=((tx3-tx1) << 16) / (y3-y1);
  txval2=tx1 << 16;
  tystep2=((ty3-ty1) << 16) / (y3-y1);
  tyval2=ty1 << 16;

  if (y1 != y2)
  {
    xstep = ((x2-x1) << 16) / (y2-y1);
    xval = x1 << 16;
    txstep = ((tx2-tx1) << 16) / (y2-y1);
    txval = tx1 << 16;
    tystep = ((ty2-ty1) << 16) / (y2-y1);
    tyval = ty1 << 16;

    for (yon=y1;yon < y2; yon++)
    {
      if ((yon > clip.top) && (yon < clip.bottom))
       tmaphline(xval>>16,xval2>>16,yon,txval>>16,txval2>>16,
                  tyval>>16,tyval2>>16,
                  texw,texh,image);
      
      xval+=xstep;
      xval2+=xstep2;
      txval+=txstep;
      txval2+=txstep2;
      tyval+=tystep;
      tyval2+=tystep2;
    }
  }
  if (y2 != y3)
  {
    xstep = ((x3-x2) << 16) / (y3-y2);
    xval = x2 << 16;
    txstep = ((tx3-tx2) << 16) / (y3-y2);
    txval = tx2 << 16;
    tystep = ((ty3-ty2) << 16) / (y3-y2);
    tyval = ty2 << 16;

    for (yon=y2;yon < y3; yon++)
    {
      if ((yon > clip.top) && (yon < clip.bottom))
      {
        tmaphline(xval>>16,xval2>>16,yon,txval>>16,txval2>>16,
                  tyval>>16,tyval2>>16,
                  texw,texh,image);
      }
      xval+=xstep;
      xval2+=xstep2;
      txval+=txstep;
      txval2+=txstep2;
      tyval+=tystep;
      tyval2+=tystep2;
    }
  }
}


void GrDriver::Mask(byte* src,byte* mask, int width,int height, byte* dest)
{
 int i=width*height;
 if (bpp==2)
  {
   word* s16=(word*)src;
   word* m16=(word*)mask;
   word* d16=(word*)dest; // bleh, makes my life easier
   
   while (i--)
    {
     *d16=(*s16&*m16);
     d16++; s16++; m16++;
    }
   // hmm.. simple enough. ;)
  }
 else
  {
   while (i--)
    {
     *dest=(*src&*mask);
     dest++; mask++; src++;
    }
  }
}

void GrDriver::Silhouette(int width,int height,byte* src,byte* dest,int colour)
{
 width*=height;
 if (bpp==1)
  {
   while (width--)
    {
     if (*src) 
      *dest=colour;
     src++; dest++;
    }
  }
 else 
  {
   word* s16=(word*)src;
   word* d16=(word*)dest;
   while (width--)
    {
     if (*s16!=trans_mask)
      *d16=colour;
     s16++; d16++;
    }
  }
}

void GrDriver::ChangeAll(int width,int height,byte* src,int srccolour,int destcolour)
{
 width*=height;
 if (bpp==1)
  {
   while (width--)
    {
     if (*src==srccolour)
      *src=destcolour;
     src++;
    }
  }
 else
  {
   word* s16=(word*)src;
   while (width--)
    {
     if (*s16==srccolour)
      *s16=destcolour;
     s16++;
    }   
  }
}

// Pixel/palette crap

void GrDriver::GetPixelFormat(void)
{
 DDPIXELFORMAT ddpf;
 int r,g,b;

 memset(&ddpf,0,sizeof ddpf);
 ddpf.dwSize=sizeof(DDPIXELFORMAT);
 offsurf->GetPixelFormat(&ddpf);
 rpos=0;  gpos=0;  bpos=0;
 rsize=0; gsize=0; bsize=0;

 r=ddpf.dwRBitMask;
 g=ddpf.dwGBitMask;
 b=ddpf.dwBBitMask;

 while ((r&1)==0 && rpos<32)  {   r>>=1; rpos++;   }
 while ((g&1)==0 && gpos<32)  {   g>>=1; gpos++;   }
 while ((b&1)==0 && bpos<32)  {   b>>=1; bpos++;   }
 while ((r&1)==1)  {   r>>=1;    rsize++;  }
 while ((g&1)==1)  {   g>>=1;    gsize++;  }
 while ((b&1)==1)  {   b>>=1;    bsize++;  }

 bpp=ddpf.dwRGBBitCount/8;

 lucentmask=~((1<<rpos)|(1<<gpos)|(1<<bpos));
}

// converts an 8 bit palettized pixel to the current bit depth
unsigned int GrDriver::Conv8(int c)
{
 int i,r,g,b;
 static int lowestsofar,highestsofar;

 if (bpp==1) return c; // we'll assume it's the same palette for now.

 i=c*3; // pedal to the metal! ;D
 r=gamepal[i++];
 g=gamepal[i++];
 b=gamepal[i++];
 if (!c)
  return trans_mask;
 else
  return PackPixel(r<<2,g<<2,b<<2);
}

unsigned int GrDriver::Conv16(int c)
// converts a 5:6:5 hicolour pixel to the current bit depth
{
 int r,g,b;

 b=c&31;
 g=(c>>5)&63;
 r=(c>>11)&31;
// b<<=1; r<<=1;

 return PackPixel(r<<3,g<<2,b<<3);
}

unsigned int GrDriver::PackPixel(int r,int g,int b)
{
 if (rsize<8)  r>>=8-rsize;
 if (gsize<8)  g>>=8-gsize;
 if (bsize<8)  b>>=8-bsize;

 return (r<<rpos) | (g<<gpos) | (b<<bpos);
 // TODO: add closest-matching-colour function for 8bit modes.
}

void GrDriver::UnPackPixel(int c,int& r,int& g,int& b)
{
 r=c>>rpos;
 g=c>>gpos;
 b=c>>bpos;

 if (rsize<8)  r=(r<<(8-rsize))&255;
 if (gsize<8)  g=(g<<(8-gsize))&255;
 if (bsize<8)  b=(b<<(8-bsize))&255;
}

int GrDriver::SetPalette(byte* p) // p is a char[768]
{
 HRESULT result;
 PALETTEENTRY ddp[256];
 int i;
 static int beenhere=0;

 if (!beenhere)
  {
   memcpy(gamepal,p,768);       // one time only, copy the default VERGE palette
   beenhere=1;
  }

 if (p!=pal)
  memcpy(pal,p,768);            // we'll keep the pal member updated while we're at it.

 if (bpp>1) return 1; // non-palettized graphics mode

 // copy the pal array into ddp
 for (i=0; i<256; i++)
  {
   ddp[i].peRed  = pal[i*3  ]<<2;
   ddp[i].peGreen= pal[i*3+1]<<2;
   ddp[i].peBlue = pal[i*3+2]<<2;
   ddp[i].peFlags= 0;
  }

 if (ddpal==NULL) return 0;
 result=ddpal->SetEntries(0,0,256,ddp);
 if (result!=DD_OK)
  {
   Log("uh oh, palette problem");
   LogDDErr(result);
   return 0;
  }
 return 1;
}

int GrDriver::GetPalette(byte* p) // p is a char[768]
{
 HRESULT result;
 PALETTEENTRY ddp[256];
 int i;

 if (bpp>1) return 1;

 result=ddpal->GetEntries(0,0,256,ddp);
 if (result!=DD_OK)
  {
   LogDDErr(result);
   return 0;
  }

 for (i=0; i<256; i++)
  {
   pal[i*3  ]=ddp[i].peRed  >>2;
   pal[i*3+1]=ddp[i].peGreen>>2;
   pal[i*3+2]=ddp[i].peBlue >>2;
  }

 memcpy(p,pal,768);
 return 1;
}

int GrDriver::InitLucentLUT(byte* data)
// this just copies the data into lucentlut8 so the lucent stuff can work in 8bit mode.
{
 if (lucentlut8!=NULL) delete lucentlut8;
 lucentlut8=new char[256*256];
 memcpy(lucentlut8,data,256*256);
 return 1; // bleh, simple
}

void GrDriver::CalcLucentLUT(int lucency)
{
 int biggest;
 
 biggest=rsize;
 biggest=biggest<gsize?gsize:biggest;
 biggest=biggest<bsize?bsize:biggest;

 if (lucentlut16==NULL)
  lucentlut16=new word[65535]; // o_O --tSB
  
 int i,j;
 
 for (i=0; i<256; i++)
  for (j=0; j<256; j++)
  {
   lucentlut16[i*256+j]=(i*lucency+j*(255-lucency))/256;
   //i*32+j == (i*tlevel+j*(255-tlevel))/256;
  }
  
}

void GrDriver::SetClipRect(RECT newrect)
{
 memcpy(&clip,&newrect,sizeof clip);
}

void GrDriver::SetRenderDest(int x,int y,byte* dest)
{
 scrx=x; scry=y;
 clip.top=clip.left=0;
 clip.right=x-1; clip.bottom=y-1;
 screen=dest;
}

void GrDriver::RestoreRenderSettings()
{
 scrx=xres; scry=yres;
 clip.top=clip.left=0;
 clip.right=xres-1; clip.bottom=yres-1;
 screen=truescreen;
}

inline int GrDriver::morph_step(int S, int D, int mix, int light)
{
	return (mix*(S - D) + (100*D))*light/100/64;
}

void GrDriver::PaletteMorph(int mr,int mg,int mb,int percent,int intensity)
{
 int rgb[3],n;
 byte pmorph_palette[3*256];
 
 int i;
 int wr,wg,wb;
 int r,g,b;

 if (bpp==2)
  {
   if (percent && intensity==63)
    {
     if (morphlut) delete morphlut;
     morphlut=NULL;
     return;
    }
   if (morphlut==NULL)  morphlut=new word[65536]; // YES, I like look-up tables, thank you very much. --tSB
   for (i=0; i<65535; i++)
    {
     UnPackPixel(i,r,g,b);

     wr=morph_step(r,mr,percent,intensity);
     wg=morph_step(g,mg,percent,intensity);
     wb=morph_step(b,mb,percent,intensity);

     morphlut[i]=PackPixel(wr,wg,wb);
    }
  } 
 else
  {   
   rgb[0]=mr; rgb[1]=mg; rgb[2]=mb;

   for (n = 0; n < 3; n += 1)
    {
     if (rgb[n] < 0)
      rgb[n] = 0;
     else if (rgb[n] > 63)
      rgb[n] = 63;
    }

// pull the colors
   for (n = 0; n < 3*256; n += 1)
    {
     pmorph_palette[n] = (unsigned char) morph_step(gfx.gamepal[n], rgb[n % 3],percent, intensity);
    }

// enforce new palette
   SetPalette(pmorph_palette);
  }
}

bool __cdecl IsMMX(void)
/*
  I didn't write this.  I got it from:
  http://gamedev.net/reference/programming/features/mmxblend/
  By John Hebert

  --tSB
*/
{
    SYSTEM_INFO si;
    int nCPUFeatures=0;
    GetSystemInfo(&si);
    if (si.dwProcessorType != PROCESSOR_INTEL_386 && si.dwProcessorType != PROCESSOR_INTEL_486)
    {
        try
        {
            __asm
            {
                ; we must push/pop the registers << CPUID>>  writes to, as the
				; optimiser doesn't know about << CPUID>> , and so doesn't expect
				; these registers to change.
                push eax
                push ebx
                push ecx
                push edx

                ; << CPUID>> 
                ; eax=0,1,2 -> CPU info in eax,ebx,ecx,edx
                mov eax,1
                _emit 0x0f
                _emit 0xa2
                mov nCPUFeatures,edx

                pop edx
                pop ecx
                pop ebx
                pop eax
            }
        }
        catch(...) // just to be sure...
        {
			return false;
        }
    }
    return (nCPUFeatures & 0x00800000) != 0;
}
