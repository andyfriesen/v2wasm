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

int lucentmode = 0;

void vc_UnPress() {
    int n;
    n = ResolveOperand();

    input.UnPress(n);
}

void vc_Exit_() {
    string_k message;

    message = ResolveString();

    Sys_Error(message.c_str());
}

void vc_Message() {
    string_k message;
    int n;

    message = ResolveString();
    n = ResolveOperand();

    Message_Send(message, n);
}

void vc_Malloc() {
    int n;

    n = ResolveOperand();

    vcreturn = (int)valloc(n, "vcreturn", OID_VC);

    Log(va("VC allocating %u bytes, ptr at 0x%08X.", n, vcreturn));

    if (!vcreturn)
        Message_Send("Warning: VC failed malloc", 750);
}

void vc_Free() {
    int ptr;

    ptr = ResolveOperand();

    if (0 == ptr) {
        return;
    }

    vfree((void*)ptr);

    Log(va("VC freeing allocated heap at 0x%08X.", ptr));
}

void vc_pow() {
    int a, b;

    a = ResolveOperand();
    b = ResolveOperand();

    vcreturn = (int)pow(a, b);
}

void vc_loadimage() {
    string_k filename;

    filename = ResolveString();

    vcreturn = (int)Image_LoadBuf(filename.c_str());
    if (!vcreturn) {
        Sys_Error("vc_loadimage: %s: unable to open", filename.c_str());
    }
}

void vc_copysprite() {
    int x, y, width, length;
    int ptr;

    x = ResolveOperand();
    y = ResolveOperand();
    width = ResolveOperand();
    length = ResolveOperand();
    ptr = ResolveOperand();

    if (lucentmode)
        gfx.CopySpriteLucent(x, y, width, length, (byte*)ptr, lucentmode);
    else
        gfx.CopySprite(x, y, width, length, (byte*)ptr);
}

void vc_tcopysprite() {
    int x, y, width, length;
    int ptr;

    x = ResolveOperand();
    y = ResolveOperand();
    width = ResolveOperand();
    length = ResolveOperand();
    ptr = ResolveOperand();

    if (lucentmode)
        gfx.TCopySpriteLucent(x, y, width, length, (byte*)ptr, lucentmode);
    else
        gfx.TCopySprite(x, y, width, length, (byte*)ptr);
}

void vc_EntitySpawn() {
    int tilex, tiley;
    string_k chr_filename;

    tilex = ResolveOperand();
    tiley = ResolveOperand();
    chr_filename = ResolveString();

    vcreturn = AllocateEntity(tilex, tiley, chr_filename.c_str());
}

void vc_SetPlayer() {
    int n;

    n = ResolveOperand();
    if (n < 0 || n >= entities) {
        Sys_Error("vc_SetPlayer: entity index out of range (attempted %d)", n);
    }

    player = &entity[n];
    playernum = (byte)n;

    entity[n].moving = 0;
}

void vc_Map() {
    hookretrace = 0;
    hooktimer = 0;
    kill = 1;

    startmap = ResolveString();
    printf("vc_Map %s\n", startmap.c_str());
}

void vc_LoadFont() {
    string_k filename;

    filename = ResolveString();

    vcreturn = Font_Load(filename.c_str());
}

void vc_PlayFLI() {
    Log("vc_PlayFLI disabled.");

    ResolveString(); // FLI filename

    /*
    string_k fli_filename=ResolveString();

    BITMAP flibuf;
    flibuf.w=screen_width;
    flibuf.h=screen_length;
    flibuf.data=screen;

    VFILE* f=vopen((const char*)fli_filename);
    if (!f)
    {
            Sys_Error("vc_PlayFLI: could not open %s.", (const
    char*)fli_filename);
    }

    unsigned int n=filesize(f);
    byte* data=(byte *)valloc(n, "vc_PlayFLI:data", 0);
    if (!data)
    {
            vclose(f);
            Sys_Error("vc_PlayFLI: Not enough memory to play FLI.");
    }
    vread(data, n, f);
    vclose(f);

    play_memory_fli(data, &flibuf, 0, ShowPage);

    vfree(data);

    timer_count=0;
    set_intensity(63);
    */
}

void vc_PrintString() {
    string_k text;
    int font_slot;

    font_slot = ResolveOperand();
    text = ResolveString();

    Font_Print(font_slot, text.c_str());
}

void vc_LoadRaw() {
    string_k raw_filename;
    VFILE* vf;
    int n;
    char* ptr;

    raw_filename = ResolveString();

    vf = vopen(raw_filename.c_str());
    if (!vf) {
        Sys_Error("vc_LoadRaw: could not open file %s", raw_filename.c_str());
    }
    n = filesize(vf);
    ptr = (char*)valloc(n, "LoadRaw:t", OID_VC);
    if (!ptr) {
        Sys_Error("vc_LoadRaw: memory exhausted on ptr");
    }
    vread(ptr, n, vf);
    vclose(vf);

    vcreturn = (int)ptr;
}

// TODO: rename the layer[] and layers[] arrays less-confusingly. ;P
void vc_SetTile() {
    int x, y, lay, value;

    x = ResolveOperand();
    y = ResolveOperand();
    lay = ResolveOperand();
    value = ResolveOperand();

    // ensure all arguments are valid
    if (x < 0 || y < 0)
        return;
    if (lay == 6 || lay == 7) {
        if (x >= layer[0].sizex || y >= layer[0].sizey)
            return;
    } else {
        if ((lay >= 0 && lay < 6) &&
            (x >= layer[lay].sizex || y >= layer[lay].sizey)) {
            return;
        }
    }

    // determine action
    switch (lay) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        layers[lay][y * layer[lay].sizex + x] = (short)value;
        break;

    case 6:
        obstruct[y * layer[0].sizex + x] = (byte)value;
        break;
    case 7:
        zone[y * layer[0].sizex + x] = (byte)value;
        break;

    default:
        Sys_Error("vc_SetTile: invalid layer value (");
    }
}

void vc_ScaleSprite() {
    int x, y, source_width, source_length, dest_width, dest_length;
    int ptr;

    x = ResolveOperand();
    y = ResolveOperand();
    source_width = ResolveOperand();
    source_length = ResolveOperand();
    dest_width = ResolveOperand();
    dest_length = ResolveOperand();
    ptr = ResolveOperand();

    if (!lucentmode)
        gfx.ScaleSprite(x, y, source_width, source_length, dest_width,
            dest_length, (byte*)ptr);
    else
        gfx.ScaleSpriteLucent(x, y, source_width, source_length, dest_width,
            dest_length, (byte*)ptr, lucentmode);
}

void vc_EntityMove() {
    int ent;
    string_k movescript;

    ent = ResolveOperand();
    movescript = ResolveString();

    if (ent < 0 || ent >= entities) {
        Log(va("vc_EntityMove: no such entity %d", ent));
        return;
    }

    entity[ent].moving = 0;
    entity[ent].speedct = 0;

    entity[ent].delayct = 0;
    entity[ent].mode = 0;
    entity[ent].data1 = 0;

    strncpy(movescriptbuf + 256 * ent, movescript.c_str(), 255);

    entity[ent].scriptofs = movescriptbuf + 256 * ent;
    entity[ent].movecode = 4;
}

void vc_HLine() {
    int x, y, xe, color;

    x = ResolveOperand();
    y = ResolveOperand();
    xe = ResolveOperand();
    color = ResolveOperand();

    gfx.HLine(x, y, xe, color, lucentmode);
}

void vc_VLine() {
    int x, y, ye, color;

    x = ResolveOperand();
    y = ResolveOperand();
    ye = ResolveOperand();
    color = ResolveOperand();

    gfx.VLine(x, y, ye, color, lucentmode);
}

void vc_Line() {
    int x, y, xe, ye, color;

    x = ResolveOperand();
    y = ResolveOperand();
    xe = ResolveOperand();
    ye = ResolveOperand();
    color = ResolveOperand();

    gfx.Line(x, y, xe, ye, color, lucentmode);
}

void vc_Circle() {
    int x, y, radius, color;

    x = ResolveOperand();
    y = ResolveOperand();
    radius = ResolveOperand();
    color = ResolveOperand();

    gfx.Circle(x, y, radius, color, lucentmode);
}

void vc_CircleFill() {
    int x, y, radius, color;

    x = ResolveOperand();
    y = ResolveOperand();
    radius = ResolveOperand();
    color = ResolveOperand();

    gfx.CircleFill(x, y, radius, color, lucentmode);
}

void vc_Rect() {
    int x, y, xe, ye, color;

    x = ResolveOperand();
    y = ResolveOperand();
    xe = ResolveOperand();
    ye = ResolveOperand();
    color = ResolveOperand();

    gfx.Rect(x, y, xe, ye, color, lucentmode);
}

void vc_RectFill() {
    int x, y, xe, ye, color;

    x = ResolveOperand();
    y = ResolveOperand();
    xe = ResolveOperand();
    ye = ResolveOperand();
    color = ResolveOperand();

    gfx.RectFill(x, y, xe, ye, color, lucentmode);
}

void vc_strlen() {
    string_k text;

    text = ResolveString();

    vcreturn = text.length();
}

void vc_strcmp() {
    string_k a, b;

    a = ResolveString();
    b = ResolveString();

    if (a < b)
        vcreturn = -1;
    else if (a > b)
        vcreturn = +1;
    else
        vcreturn = 0;
}

void vc_FontWidth() {
    int n;

    n = ResolveOperand(); // slot

    vcreturn = Font_GetWidth(n);
}

void vc_FontHeight() {
    int n;

    n = ResolveOperand();

    vcreturn = Font_GetLength(n);
}

void vc_SetPixel() {
    int x, y, color;

    x = ResolveOperand();
    y = ResolveOperand();
    color = ResolveOperand();

    gfx.SetPixel(x, y, color, lucentmode);
}

void vc_GetPixel() {
    int x, y;

    x = ResolveOperand();
    y = ResolveOperand();

    vcreturn = 0;
    vcreturn = gfx.GetPixel(x, y);
}

void vc_EntityOnScreen() {
    int find, n;

    find = ResolveOperand();
    for (n = 0; n < cc; n++) {
        if (entidx[n] == find) {
            vcreturn = 1;
            return;
        }
    }

    vcreturn = 0;
}

void vc_GetTile() {
    int x, y, lay;

    x = ResolveOperand();
    y = ResolveOperand();
    lay = ResolveOperand();

    vcreturn = 0;

    // ensure all arguments are valid
    if (x < 0 || y < 0)
        return;
    if (lay == 6 || lay == 7) {
        if (x >= layer[0].sizex || y >= layer[0].sizey)
            return;
    } else {
        if ((lay >= 0 && lay < 6) &&
            (x >= layer[lay].sizex || y >= layer[lay].sizey)) {
            return;
        }
    }

    switch (lay) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        vcreturn = (int)layers[lay][y * layer[lay].sizex + x];
        break;
    case 6:
        vcreturn = (int)obstruct[y * layer[0].sizex + x];
        break;
    case 7:
        vcreturn = (int)zone[y * layer[0].sizex + x];
        break;

    default:
        Sys_Error("vc_GetTile: invalid layer value");
    }
}

void vc_SetResolution() {
    int xres, yres;

    xres = ResolveOperand();
    yres = ResolveOperand();

    vcreturn = gfx.SetMode(xres, yres);
    if (!vcreturn)
        Sys_Error("vc_SetResolution failed");
    gfx.Clear();
    input.ClipMouse(0, 0, xres, yres);
}

void vc_SetRString() {
    // TODO: some validity checks would be nice
    rstring = ResolveString();
}

void vc_SetClipRect() {
    int bogus;
    RECT clip;

    clip.left = ResolveOperand();
    clip.top = ResolveOperand();
    clip.right = ResolveOperand();
    clip.bottom = ResolveOperand();

    bogus = 0;

    // ensure arguments stay valid
    if (clip.left < 0)
        clip.left = 0, bogus++;
    else if (clip.left >= gfx.scrx)
        clip.left = gfx.scrx - 1, bogus++;

    if (clip.top < 0)
        clip.top = 0, bogus++;
    else if (clip.top >= gfx.scry)
        clip.top = gfx.scry - 1, bogus++;

    if (clip.right < 0)
        clip.right = 0, bogus++;
    else if (clip.right >= gfx.scrx)
        clip.right = gfx.scrx - 1, bogus++;

    if (clip.bottom < 0)
        clip.bottom = 0, bogus++;
    else if (clip.bottom >= gfx.scry)
        clip.bottom = gfx.scry - 1, bogus++;

    if (bogus)
        Log(va("vc_SetClipRect: %d bogus args", bogus));
    gfx.SetClipRect(clip);
}

void vc_SetRenderDest() {
    byte* scr;
    int x, y;

    x = ResolveOperand();
    y = ResolveOperand();
    scr = (byte*)ResolveOperand();
    gfx.SetRenderDest(x, y, scr);
}

void vc_RestoreRenderSettings() { gfx.RestoreRenderSettings(); }

void vc_PartyMove() {
    int fudge;

    player = 0;

    entity[playernum].moving = 0;
    entity[playernum].speedct = 0;
    entity[playernum].delayct = 0;
    entity[playernum].mode = 0;
    entity[playernum].data1 = 0;

    vcpush(cameratracking);
    vcpush(tracker);

    if (1 == cameratracking) {
        cameratracking = 2;
        tracker = playernum;
    }

    // ResolveString((char *) (int) movescriptbuf + (int) (playernum*256));
    string_k movescript = ResolveString();
    strncpy(movescriptbuf + 256 * playernum, movescript.c_str(), 255);
    movescriptbuf[256 * playernum + 255] = '\0';

    entity[playernum].scriptofs = movescriptbuf + 256 * playernum;
    entity[playernum].movecode = 4;

    // aen <3feb2000> Now works independent of engine timer 'timer_count'.
    fudge = systemtime;
    while (entity[playernum].movecode) {
        fudge = systemtime - fudge;
        while (fudge) {
            fudge -= 1;
            ProcessEntities();
        }
        fudge = systemtime;

        Render();
        gfx.ShowPage();

        /* To prevent crashing, we have to update the controls, and check to see
           if
           alt-x is pressed.
           Otherwise, if the user accidently directs the player into an
           obstruction,
           the player is
           forced to reset their computer. (ctrl-alt-del doesn't even work!) */
        input.Update();
        if (input.key[DIK_LMENU] && input.key[DIK_X])
            Sys_Error("");
    }

    tracker = (byte)vcpop();
    cameratracking = (byte)vcpop();

    player = &entity[playernum];
    timer_count = 0;
}

void vc_WrapBlit() {
    int offsetx, offsety, width, length;
    int ptr;

    offsetx = ResolveOperand();
    offsety = ResolveOperand();
    width = ResolveOperand();
    length = ResolveOperand();
    ptr = ResolveOperand();

    if (!lucentmode)
        gfx.WrapBlit(offsetx, offsety, width, length, (byte*)ptr);
    else
        gfx.WrapBlitLucent(
            offsetx, offsety, width, length, (byte*)ptr, lucentmode);
}

void vc_TWrapBlit() {
    int offsetx, offsety, width, length;
    int ptr;

    offsetx = ResolveOperand();
    offsety = ResolveOperand();
    width = ResolveOperand();
    length = ResolveOperand();
    ptr = ResolveOperand();

    if (!lucentmode)
        gfx.TWrapBlit(offsetx, offsety, width, length, (byte*)ptr);
    else
        gfx.TWrapBlitLucent(
            offsetx, offsety, width, length, (byte*)ptr, lucentmode);
}

void vc_SetMousePos() {
    int x, y;

    x = ResolveOperand();
    y = ResolveOperand();

    input.MoveMouse(x, y);
}

void vc_HookRetrace() {
    int script;

    script = 0;
    switch (GrabC()) {
    case 1:
        script = ResolveOperand();
        break;
    case 2:
        script = USERFUNC_MARKER + GrabD();
        break;
    }
    hookretrace = script;
}

void vc_HookTimer() {
    int script;

    script = 0;
    switch (GrabC()) {
    case 1:
        script = ResolveOperand();
        break;
    case 2:
        script = USERFUNC_MARKER + GrabD();
        break;
    }
    hooktimer = script;
}

void vc_HookKey() {
    int key, script;

    key = ResolveOperand();
    if (key < 0)
        key = 0;
    if (key > 127)
        key = 127;
    //	key = scantokey[key];

    script = 0;
    switch (GrabC()) {
    case 1:
        script = ResolveOperand();
        break;
    case 2:
        script = USERFUNC_MARKER + GrabD();
        break;
    }
    bindarray[key] = script;
}

void vc_PlayMusic() {
    string_k filename;

    filename = ResolveString();

    PlayMusic(filename.c_str());
}

int morph_step(int S, int D, int mix, int light) {
    return (mix * (S - D) + (100 * D)) * light / 100 / 64;
}

void vc_PaletteMorph() {
    int r, g, b;
    int percent, intensity;

    r = ResolveOperand(); // red
    g = ResolveOperand(); // green
    b = ResolveOperand(); // blue

    percent = 100 - ResolveOperand();
    intensity = ResolveOperand();

    gfx.PaletteMorph(r, g, b, percent, intensity);
}

string_k EnforceNoDirectories(string_k s) {
    int n;
    /* rewritten - tSB
       if the first char is a backslash, the second char is a colon, or
       there are two or more consecutive periods anywhere in the string,
       then bomb out with an error.  They deserve it anyway. D:<           */
    n = 0;

    if (s[0] == '/' || s[0] == '\\')
        Sys_Error(
            va("EnforceNoDirectories: Invalid file name (%s)", s.c_str()));

    if (s[1] == ':') // second char a colon?  (eg C:autoexec.bat)
        Sys_Error(
            va("EnforceNoDirectories: Invalid file name (%s)", s.c_str()));
    n = 0;
    while (n < s.length() - 1) {
        if (s[n] == '.' &&
            s[n + 1] ==
                '.') // two (or more) consective periods?  (eg ..\autoexec.bat)
            Sys_Error(
                va("EnforceNoDirectories: Invalid file name (%s)", s.c_str()));
        n++;
    }

    return s; // We're clean! - tSB
}

void vc_OpenFile() {
    string_k filename;
    //	VFILE*	vf;

    /*	filename	= ResolveString();
            filename	= EnforceNoDirectories(filename);

            vf = vopen((const char*)filename);
            vcreturn = (quad)vf;*/

    filename = EnforceNoDirectories(ResolveString());

    int idx = OpenVCFile(filename.c_str());
    vcreturn = (quad)idx;

    Log(va(" --> VC opened file %s, Handle %i", filename.c_str(), idx));
}

void vc_CloseFile() {
    /*	VFILE*	vf;

            vf	=(VFILE *)ResolveOperand();
            vclose(vf);*/

    int idx = ResolveOperand();
    CloseVCFile(idx);

    Log(va(" --> VC closed file %i", idx));
}

void vc_QuickRead() {
    /*
    int		temp;

    Log("vc_QuickRead disabled.");

    ResolveString();	// filename

    temp=GrabC();			// code
    GrabW();				// offset
    if (op_SARRAY==temp)
            ResolveOperand();	// offset
    ResolveOperand();		// seek line
    */

    string_k filename, ret;
    int offset, seekline;

    filename = ResolveString();
    filename = EnforceNoDirectories(filename);

    // which string are we reading into?
    char code = GrabC();
    if (code == op_STRING) {
        offset = GrabW();
    }
    if (code == op_SARRAY) {
        offset = GrabW();
        offset += ResolveOperand();
    }

    // which line are we reading from the file?
    seekline = ResolveOperand();
    if (seekline < 1)
        seekline = 1;

    // open the file
    VFILE* f = vopen(filename.c_str());
    if (!f) {
        Sys_Error("vc_QuickRead: could not open %s", filename.c_str());
    }

    // seek to the line of interest
    char temp[256] = {0};
    for (int n = 0; n < seekline; n++) {
        vgets(temp, 255, f);
    }
    // suppress trailing CR/LF
    char* p = temp;
    while (*p) {
        if ('\n' == *p || '\r' == *p)
            *p = '\0';
        p++;
    }

    // assign to vc string
    if (offset >= 0 && offset < stralloc) {
        vc_strings[offset] = temp;
    }

    vclose(f);
}

void vc_AddFollower() {
    Log("vc_AddFollower disabled.");

    ResolveOperand(); // entity

    /*
    int n;

    n=ResolveOperand();
    if (n<0 || n>=entities)
    {
            Sys_Error("vc_AddFollower: Not a valid entity index. (%d)", n);
    }

    follower[(int)numfollowers++]=n;
    ResetFollowers();
    */
}

void vc_FlatPoly() {
    int a, b, c, d, e, f, g;

    a = ResolveOperand(); // a
    b = ResolveOperand(); // b
    c = ResolveOperand(); // c
    d = ResolveOperand(); // d
    e = ResolveOperand(); // e
    f = ResolveOperand(); // f
    g = ResolveOperand(); // g

    gfx.FlatPoly(a, b, c, d, e, f, g);
}

void vc_TMapPoly() {
    int a, b, c, d, e, f, g, h, i, j, k, l, m, n, o;

    a = ResolveOperand();
    b = ResolveOperand();
    c = ResolveOperand();
    d = ResolveOperand();
    e = ResolveOperand();
    f = ResolveOperand();
    g = ResolveOperand();
    h = ResolveOperand();
    i = ResolveOperand();
    j = ResolveOperand();
    k = ResolveOperand();
    l = ResolveOperand();
    m = ResolveOperand();
    n = ResolveOperand();
    o = ResolveOperand();
    gfx.TMapPoly(a, b, c, d, e, f, g, h, i, j, k, l, m, n, (byte*)o);
}

void vc_CacheSound() {
    string_k filename;

    filename = ResolveString();

    vcreturn = CacheSound(filename.c_str());
}

void vc_PlaySound() {
    int slot, volume, pan;

    slot = ResolveOperand();
    volume = ResolveOperand();
    pan = ResolveOperand();

    PlaySFX(slot, volume, pan);
}

void vc_RotScale() {
    int x, y, width, length, angle, scale;
    int ptr;

    x = ResolveOperand();
    y = ResolveOperand();
    width = ResolveOperand();
    length = ResolveOperand();
    angle = ResolveOperand();
    scale = ResolveOperand();
    ptr = ResolveOperand();

    if (!lucentmode)
        gfx.RotScale(x, y, width, length, (float)(angle * 3.14159 / 180.0),
            (float)(scale / 1000.0), (byte*)ptr);
    else
        gfx.RotScaleLucent(x, y, width, length,
            (float)(angle * 3.14159 / 180.0), (float)(scale / 1000.0),
            (byte*)ptr, lucentmode);
}

void vc_MapLine()
// blah.  poo.
// The graphics routines are separate from the rest of VERGE, so it's all gotta
// be here. :P
{
    // x        = the offset of xwin that we'll use
    // y        = the y coord that we'll draw on
    // y_offset = the y coord that we'll be rendering
    // lay      = the layer number.
    // the line is always the whole screen wide, but at any height and any
    // distortion level.
    int x, y, x_map, y_map, x_sub, y_sub, y_offset, lay;

    x = ResolveOperand();
    y = ResolveOperand();
    y_offset = ResolveOperand();
    lay = ResolveOperand();

    if (lay < 0 || lay >= numlayers)
        return; // validate arguments
    if (!layertoggle[lay])
        return; // is this layer visible?

    // This is the location of the first tile we draw, it'll be at the left hand
    // edge of the screen
    x_map = (xwin + x) * layer[lay].pmultx / layer[lay].pdivx;
    y_map = (ywin + y_offset) * layer[lay].pmulty / layer[lay].pdivy;

    y_offset &= 15;

    if (x_map < 0)
        x_map = 0; // make my life easier; don't allow scrolling past map edges
    if (y_map < 0)
        y_map = 0;

    x_sub = -(
        x_map & 15); // get subtile position while we still have pixel precision
    y_sub = (y_map & 15);

    x_map >>= 4; // determine upper left tile coords of camera
    y_map >>= 4;

    word* source = layers[lay] + y_map * layer[lay].sizex + x_map; // ew
    // TODO: GetTile function (inline!)
    x = x_sub;

    do {
        if (*source)
            gfx.CopySprite(x, y, 16, 1,
                (byte*)(vsp) +
                    (16 * 16 * tileidx[*source] + (16 * y_sub)) * gfx.bpp);

        source += 1;
        x += 16;
    } while (x < gfx.XRes());

    /*	LFB_BlitMapLine(x_sub, y_offset, y_sub,
                    (unsigned short *) (layers[lay] + (y_map*layer[lay].sizex) +
       x_map), 0, 0);*/
    /*
    int x, y, screen_length, l;
    int xtc, ytc, xofs, yofs;


    xtc=x >> 4;
    ytc=y >> 4;
    xofs= -(x&15);
    yofs=  (y&15);

    MapLine(xofs, screen_length, yofs, (word *)
    (layers[l]+((ytc*layer[l].sizex)+xtc)));
    */
}

void vc_TMapLine()
// Directly copy/pasted from vc_MapLine.
// I suck and I know it! --tSB
{
    // x        = the offset of xwin that we'll use
    // y        = the y coord that we'll draw on
    // y_offset = the y coord that we'll be rendering
    // lay      = the layer number.
    // the line is always the whole screen wide, but at any height and any
    // distortion level.
    int x, y, x_map, y_map, x_sub, y_sub, y_offset, lay;

    x = ResolveOperand();
    y = ResolveOperand();
    y_offset = ResolveOperand();
    lay = ResolveOperand();

    if (lay < 0 || lay >= numlayers)
        return; // validate arguments
    if (!layertoggle[lay])
        return; // is this layer visible?

    // This is the location of the first tile we draw, it'll be at the left hand
    // edge of the screen
    x_map = (xwin + x) * layer[lay].pmultx / layer[lay].pdivx;
    y_map = (ywin + y_offset) * layer[lay].pmulty / layer[lay].pdivy;

    y_offset &= 15;

    if (x_map < 0)
        x_map = 0; // make my life easier; don't allow scrolling past map edges
    if (y_map < 0)
        y_map = 0;

    x_sub = -(
        x_map & 15); // get subtile position while we still have pixel precision
    y_sub = (y_map & 15);

    x_map >>= 4; // determine upper left tile coords of camera
    y_map >>= 4;

    word* source = layers[lay] + y_map * layer[lay].sizex + x_map; // ew
    // TODO: GetTile function (inline!)
    x = x_sub;

    do {
        if (*source)
            gfx.TCopySprite(x, y, 16, 1,
                (byte*)(vsp) +
                    (16 * 16 * tileidx[*source] + (16 * y_sub)) * gfx.bpp);

        source += 1;
        x += 16;
    } while (x < gfx.XRes());

    /*	LFB_BlitMapLine(x_sub, y_offset, y_sub,
                    (unsigned short *) (layers[lay] + (y_map*layer[lay].sizex) +
       x_map), 0, 0);*/
    /*
    int x, y, screen_length, l;
    int xtc, ytc, xofs, yofs;


    xtc=x >> 4;
    ytc=y >> 4;
    xofs= -(x&15);
    yofs=  (y&15);

    MapLine(xofs, screen_length, yofs, (word *)
    (layers[l]+((ytc*layer[l].sizex)+xtc)));
    */
}

void vc_val() {
    string_k s;

    s = ResolveString();

    vcreturn = s.toint();
}

void vc_TScaleSprite() {
    int x, y, source_width, source_length, dest_width, dest_length;
    int ptr;

    x = ResolveOperand();
    y = ResolveOperand();
    source_width = ResolveOperand();
    source_length = ResolveOperand();
    dest_width = ResolveOperand();
    dest_length = ResolveOperand();
    ptr = ResolveOperand();

    if (lucentmode)
        gfx.TScaleSpriteLucent(x, y, source_width, source_length, dest_width,
            dest_length, (byte*)ptr, lucentmode);
    else
        gfx.TScaleSprite(x, y, source_width, source_length, dest_width,
            dest_length, (byte*)ptr);
}

void vc_GrabRegion() {
    int x, y, xe, ye, bogus;

    x = ResolveOperand();
    y = ResolveOperand();
    xe = ResolveOperand();
    ye = ResolveOperand();

    bogus = 0;

    // ensure arguments stay valid
    if (x < 0)
        x = 0, bogus++;
    else if (x >= gfx.scrx)
        x = gfx.scrx - 1, bogus++;
    if (y < 0)
        y = 0, bogus++;
    else if (y >= gfx.scry)
        y = gfx.scry - 1, bogus++;

    if (xe < 0)
        xe = 0, bogus++;
    else if (xe >= gfx.scrx)
        xe = gfx.scrx - 1, bogus++;
    if (ye < 0)
        ye = 0, bogus++;
    else if (ye >= gfx.scry)
        ye = gfx.scry - 1, bogus++;

    if (bogus)
        Log(va("vc_GrabRegion: %d bogus args", bogus));

    // swap?
    if (xe < x) {
        int t = x;
        x = xe;
        xe = t;
    }
    if (ye < y) {
        int t = ye;
        y = ye;
        ye = t;
    }

    xe = xe - x + 1;
    ye = ye - y + 1;

    if (gfx.bpp > 1) {
        unsigned short* source;
        unsigned short* ptr;
        int n;

        source = ((unsigned short*)gfx.screen) + (y * gfx.scrx) + x;
        ptr = (unsigned short*)ResolveOperand();

        while (ye) {
            for (n = 0; n < xe; n += 1)
                ptr[n] = source[n];

            ptr += xe;
            source += gfx.scrx;
            ye -= 1;
        }
    } else {
        unsigned char* source;
        unsigned char* ptr;

        source = gfx.screen + (y * gfx.scrx) + x;
        ptr = (unsigned char*)ResolveOperand();

        while (ye) {
            V_memcpy(ptr, source, xe);

            ptr += xe;
            source += gfx.scrx;
            ye -= 1;
        }
    }
}

void vc_Log() {
    string_k message;

    message = ResolveString();

    Log(message.c_str());
}

void vc_fseekline() {
    int line;
    VFILE* vf;

    line = ResolveOperand() - 1; // POO!  The -1 makes it work like it did in v2
    vf = GetReadFilePtr(ResolveOperand());

    vseek(vf, 0, SEEK_SET);

    // 1 will yield first line
    char temp[256 + 1];
    do {
        vgets(temp, 256, vf);
    } while (--line > 0);
}

void vc_fseekpos() {
    int pos;
    VFILE* vf;

    pos = ResolveOperand();
    vf = GetReadFilePtr(ResolveOperand());

    vseek(vf, pos, 0);
}

void vc_fread() {
    char* buffer;
    int len;
    VFILE* vf;

    buffer = (char*)ResolveOperand();
    len = ResolveOperand();
    vf = GetReadFilePtr(ResolveOperand());

    vread(buffer, len, vf);
}

void vc_fgetbyte() {
    VFILE* vf = 0;
    byte b = 0;

    // vf	=(VFILE *)ResolveOperand();
    vf = GetReadFilePtr(ResolveOperand());
    vread(&b, 1, vf);

    vcreturn = b;
}

void vc_fgetword() {
    VFILE* vf = 0;
    word w = 0;

    // vf	=(VFILE *)ResolveOperand();
    vf = GetReadFilePtr(ResolveOperand());
    vread(&w, 2, vf);

    vcreturn = w;
}

void vc_fgetquad() {
    VFILE* vf = 0;
    quad q = 0;

    // vf	=(VFILE *)ResolveOperand();
    vf = GetReadFilePtr(ResolveOperand());
    vread(&q, 4, vf);

    vcreturn = q;
}

void vc_fgetline() {
    char temp[256 + 1];
    char* p;
    int code, offset;
    VFILE* vf;

    // which global vc string do we read into?
    code = GrabC();
    offset = GrabW();
    if (op_SARRAY == code) {
        offset += ResolveOperand();
    }

    // file pointer; blegh
    // vf	=(VFILE *)ResolveOperand();
    vf = GetReadFilePtr(ResolveOperand()); // Better? ;)  --tSB

    // read line into temp buffer
    vgets(temp, 256, vf);
    temp[256] = '\0';

    // suppress trailing CR/LF
    p = temp;
    while (*p) {
        if ('\n' == *p || '\r' == *p)
            *p = '\0';
        p++;
    }

    // assign to vc string
    if (offset >= 0 && offset < stralloc) {
        vc_strings[offset] = temp;
    }
}

void vc_fgettoken() {
    char temp[256];
    int code, offset;
    VFILE* vf;

    // which global vc string do we read into?
    code = GrabC();
    offset = GrabW();
    if (code == op_SARRAY) {
        offset += ResolveOperand();
    }

    // file pointer; blegh
    //	vf	=(VFILE *)ResolveOperand();
    vf = GetReadFilePtr(ResolveOperand());

    // read into temp buffer
    vscanf(vf, "%s", temp);

    // assign to vc string
    if (offset >= 0 && offset < stralloc) {
        vc_strings[offset] = temp;
    }
}

void vc_fwritestring() {
    FILE* f;
    string_k temp;

    temp = ResolveString();
    // f		=(FILE *)ResolveOperand();
    f = GetWriteFilePtr(ResolveOperand());

    fprintf(f, "%s\n", temp.c_str());
}

void vc_fwrite() {
    char* buffer;
    int length;
    FILE* f;

    buffer = (char*)ResolveOperand();
    length = ResolveOperand();
    // f		=(FILE *)ResolveOperand();
    f = GetWriteFilePtr(ResolveOperand());

    fwrite(buffer, 1, length, f);
}

void vc_frename() {
    string_k a, b;

    a = ResolveString();
    b = ResolveString();
    a = EnforceNoDirectories(a);
    b = EnforceNoDirectories(b);

    rename(a.c_str(), b.c_str());

    Log(va(" --> VC renamed %s to %s.", a.c_str(), b.c_str()));
}

void vc_fdelete() {
    string_k filename;

    filename = ResolveString();
    filename = EnforceNoDirectories(filename);

    remove(filename.c_str());

    Log(va(" --> VC deleted %s.", filename.c_str()));
}

void vc_fwopen() {
    string_k filename;

    filename = ResolveString();
    filename = EnforceNoDirectories(filename);

    vcreturn = OpenWriteVCFile(filename.c_str());

    Log(va(" --> VC opened %s for writing, handle %i.", filename.c_str(),
        vcreturn));
}

void vc_fwclose() {
    int idx = ResolveOperand();
    CloseVCFile(idx);

    Log(va(" --> VC close file opened for writing, handle %i.", idx));
}

void vc_memcpy() {
    char *source, *dest;
    int length;

    dest = (char*)ResolveOperand();
    source = (char*)ResolveOperand();
    length = ResolveOperand();

    memcpy(dest, source, length);
}

void vc_memset() {
    char* dest;
    int color, length;

    dest = (char*)ResolveOperand();
    color = ResolveOperand();
    length = ResolveOperand();

    V_memset(dest, color, length);
}

// <aen, may 5>
// + modified to use new silhouette vdriver routines
// + added checks for ClipOn
void vc_Silhouette() {
    /*	int		x, y, width, length, color;
            int		ptr;

            x		=ResolveOperand();
            y		=ResolveOperand();
            width	=ResolveOperand();
            length	=ResolveOperand();
            ptr		=ResolveOperand();
            color	=ResolveOperand();

            //LFB_BlitBop(x, y, width, length, color, (byte *)ptr, LucentOn);*/

    int width, height, src, dest, colour;
    width = ResolveOperand();
    height = ResolveOperand();
    src = ResolveOperand();
    dest = ResolveOperand();
    colour = ResolveOperand();

    gfx.Silhouette(width, height, (byte*)src, (byte*)dest, colour);
}

void vc_Mosaic() {
    Log("vc_Mosaic disabled.");

    ResolveOperand(); // a
    ResolveOperand(); // b
    ResolveOperand(); // c
    ResolveOperand(); // d
    ResolveOperand(); // e
    ResolveOperand(); // f
    ResolveOperand(); // g

    /*
  int a,b,c,d,e,f,g;

  a=ResolveOperand();
  b=ResolveOperand();
  c=ResolveOperand();
  d=ResolveOperand();
  e=ResolveOperand();
  f=ResolveOperand();
  g=ResolveOperand();
  Mosaic(a,b,(byte *) c,d,e,f,g);
    */
}

void vc_WriteVars() {
    FILE* f;

    // f	=(FILE *)ResolveOperand();
    f = GetWriteFilePtr(ResolveOperand());

    fwrite(globalint, 1, 4 * maxint, f);

    for (int n = 0; n < stralloc; n++) {
        int z = vc_strings[n].length();

        fwrite(&z, 1, 4, f);
        fwrite(vc_strings[n].c_str(), 1, z, f);
        //		fputc(0, f); // x_X --tSB
    }
}

void vc_ReadVars() {
    VFILE* f;

    // f	=(VFILE *)ResolveOperand();
    f = GetReadFilePtr(ResolveOperand());

    vread(globalint, 4 * maxint, f);

    for (int n = 0; n < stralloc; n++) {
        int z;
        vread(&z, 4, f);

        char* temp = new char[z + 1];
        if (!temp)
            Sys_Error("vc_Readars: memory exhausted on %d bytes.", z);
        vread(temp, z, f);
        temp[z] = '\0';
        vc_strings[n] = temp;

        delete[] temp;
        temp = 0;
    }
}

void vc_Asc() { vcreturn = ResolveString()[0]; }

void vc_NumForScript() { vcreturn = GrabD(); }

void vc_Filesize() {
    string_k filename;
    VFILE* vf;

    filename = ResolveString();

    vf = vopen(filename.c_str());
    vcreturn = filesize(vf);
    vclose(vf);
}

void vc_FTell() {
    VFILE* vf;

    // vf	=(VFILE *)ResolveOperand();
    vf = GetReadFilePtr(ResolveOperand());

    vcreturn = vtell(vf);
}

void vc_CheckCorrupt() {
    //	Log("vc_CheckCorrupt disabled.");

    //*
    Log("checking for corruption...");
    CheckCorruption();
    //*/
}

void vc_ChangeCHR() {
    int who;
    string_k chrname;

    who = ResolveOperand();
    chrname = ResolveString();

    ChangeCHR(who, chrname.c_str());
}

void vc_RGB() {
    int r, g, b;

    r = ResolveOperand();
    g = ResolveOperand();
    b = ResolveOperand();

    vcreturn = gfx.PackPixel(r, g, b);
}

// Gah!  replace these with vc_unpackpixel! --tSB
void vc_GetR() {
    int color;
    int r, g, b;

    color = ResolveOperand();
    gfx.UnPackPixel(color, r, g, b);

    vcreturn = r;
}

void vc_GetG() {
    int color;
    int r, g, b;

    color = ResolveOperand();
    gfx.UnPackPixel(color, r, g, b);

    vcreturn = g;
}

void vc_GetB() {
    int color;
    int r, g, b;

    color = ResolveOperand();
    gfx.UnPackPixel(color, r, g, b);

    vcreturn = b;
}

void vc_Mask() {
    int source, mask, width, length, dest;

    source = ResolveOperand();
    mask = ResolveOperand();
    width = ResolveOperand();
    length = ResolveOperand();
    dest = ResolveOperand();

    gfx.Mask((byte*)source, (byte*)dest, width, length, (byte*)mask);
    /*LFB_BlitMask((unsigned char *) source, (unsigned char *) mask, width,
       length,
            (unsigned char *) dest);  */
}

void vc_ChangeAll() {
    int source, width, length, source_color, dest_color;

    source = ResolveOperand();
    width = ResolveOperand();
    length = ResolveOperand();
    source_color = ResolveOperand();
    dest_color = ResolveOperand();

    gfx.ChangeAll(width, length, (byte*)source, source_color, dest_color);
    // eAll((unsigned char *) source, width, length, source_color, dest_color);
}

//- tSB
void vc_fwritebyte() {
    char b;
    FILE* f;

    b = (char)ResolveOperand();
    // f=(FILE*)ResolveOperand();
    f = GetWriteFilePtr(ResolveOperand());
    fwrite(&b, 1, 1, f);
}

void vc_fwriteword() {
    word b;
    FILE* f;

    b = (word)ResolveOperand();
    // f=(FILE*)ResolveOperand();
    f = GetWriteFilePtr(ResolveOperand());
    fwrite(&b, 1, 2, f);
}

void vc_fwritequad() {
    int b;
    FILE* f;

    b = ResolveOperand();
    // f=(FILE*)ResolveOperand();
    f = GetWriteFilePtr(ResolveOperand());
    fwrite(&b, 1, 4, f);
}

void vc_ImageSize() {
    int x1, x2, y1, y2;

    x1 = ResolveOperand();
    y1 = ResolveOperand();
    x2 = ResolveOperand();
    y2 = ResolveOperand();

    if (x1 > x2) {
        int i = x1;
        x1 = x2;
        x2 = i;
    }
    if (y1 > y2) {
        int i = y1;
        y1 = y2;
        y2 = i;
    }

    x2 -= x1;
    y2 -= y1;
    vcreturn = x2 * y2 * gfx.bpp;
}
//- end
