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

static void Conlib_ConsoleBG_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("consolebg <image$>");
    Console_Printf("");
}

static void Conlib_ConsoleBG() {
    switch (Con_NumArgs()) {
    case 2: {
        byte *pic = 0;

        pic = Image_LoadBuf(Con_GetArg(1).c_str());
        // if we couldn't find it, notify user
        if (!pic) {
            Console_Printf(
                va("%s: error opening console graphic", Con_GetArg(1).c_str()));
            Console_Printf("");
            return;
        }

        // we found it, but it's not the correct size
        if (Image_Width() != 320 || Image_Length() != 120) {
            vfree(pic);
            Console_Printf(
                va("%s: invalid console graphic", Con_GetArg(1).c_str()));
            Console_Printf("");
            return;
        }

        // anything else is success; free old, set loaded as new
        if (consolebg)  // first time it'll be nothing; don't make mem routines
                        // dump
                        // a report
            vfree(consolebg);
        consolebg = pic;
    } break;

    default:
        Conlib_ConsoleBG_Syntax();
        break;
    }
}

static void Conlib_ListMounts_Syntax() {
    Console_Printf("");
    Console_Printf("syntax:");
    Console_Printf("listmounts");
}

static void Conlib_ListMounts() {
    switch (Con_NumArgs()) {
    case 1: {
        Console_Printf(va("There are %d files mounted.", filesmounted));
        for (int n = 0; n < filesmounted; n++) {
            Console_Printf(va("File %s contains %d files.", pack[n].mountname,
                pack[n].numfiles));
        }
        Console_Printf("");
    } break;

    default:
        Conlib_ListMounts_Syntax();
        break;
    }
}

static void Conlib_PackInfo_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("packinfo <mount$>");
    Console_Printf("");
}

static void Conlib_PackInfo() {
    switch (Con_NumArgs()) {
    case 2: {
        int mount = atoi(Con_GetArg(1).c_str());
        if (mount < 0 || mount >= filesmounted) {
            Console_Printf("Invalid mount request.");
            Console_Printf("");
            return;
        }

        Console_Printf(va("Files in %s:", pack[mount].mountname));

        for (int n = 0; n < pack[mount].numfiles; n++) {
            static char temp[1024] = {0};
            sprintf(temp, "%s              ", pack[mount].files[n].fname);
            sprintf(temp + 20, "%d bytes", pack[mount].files[n].size);
            Console_Printf(temp);
        }
        Console_Printf("");
    } break;

    default:
        Conlib_PackInfo_Syntax();
        break;
    }
}

static void Conlib_ListCmds_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("listcmds");
    Console_Printf("");
}

static void Conlib_ListCmds() {
    int idx = 0;

    switch (Con_NumArgs()) {
    case 1:
        do {
            Console_Printf(concmds[idx].name());

            idx++;
        } while (idx < concmds.size());
        Console_Printf("");
        break;

    default:
        Conlib_ListCmds_Syntax();
        break;
    }
}

static void Conlib_CD_Play_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("cd_play <track#>");
    Console_Printf("");
}

static void Conlib_CD_Play() {
    switch (Con_NumArgs()) {
    case 2: {
        /*
        int track=atoi(Con_GetArg(1));
        CD_Play(track);
        */
    } break;

    default:
        Conlib_CD_Play_Syntax();
        break;
    }
}

static void Conlib_CD_Stop_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("cd_stop");
    Console_Printf("");
}

static void Conlib_CD_Stop() {
    switch (Con_NumArgs()) {
    case 1:
        // CD_Stop();
        break;

    default:
        Conlib_CD_Stop_Syntax();
        break;
    }
}

static void Conlib_CD_Open_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("cd_open");
    Console_Printf("");
}

static void Conlib_CD_Open() {
    switch (Con_NumArgs()) {
    case 1:
        // CD_Open_Door();
        break;

    default:
        Conlib_CD_Open_Syntax();
        break;
    }
}

static void Conlib_CD_Close_Syntax() {
    Console_Printf("syntax");
    Console_Printf("cd_close");
    Console_Printf("");
}

static void Conlib_CD_Close() {
    switch (Con_NumArgs()) {
    case 1:
        // CD_Close_Door();
        break;

    default:
        Conlib_CD_Close_Syntax();
        break;
    }
}

static void Conlib_Exit_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("exit");
    Console_Printf("");
}

static void Conlib_Exit() {
    switch (Con_NumArgs()) {
    case 1:
        Sys_Error("");
        break;

    default:
        Conlib_Exit_Syntax();
        break;
    }
}

static void Conlib_Vid_Mode_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("vid_mode <xres#> <yres#>");
    Console_Printf("");
}

static void Conlib_Vid_Mode() {
    switch (Con_NumArgs()) {
    case 3:
        if (gfx.SetMode(Con_GetArg(1).toint(), Con_GetArg(2).toint(),
                gfx.bpp * 8, gfx.IsFullScreen())) {
            input.ClipMouse(0, 0, Con_GetArg(1).toint(), Con_GetArg(2).toint());
            Console_Printf("{||||||||||||}");
            Console_Printf("Loading new video driver...");
            Console_Printf(gfx.DriverDesc());
            Console_Printf("{||||||||||||}");

            Con_SetViewablePixelRows(Con_Length());
        } else
            Sys_Error("Unsupported/unknown video mode.");
        Console_Printf("");
        break;

    default:
        Conlib_Vid_Mode_Syntax();
        break;
    }
}

static void Conlib_Cpu_Usage_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("cpu_usage [1|0]");
    Console_Printf("");
}

static void Conlib_Cpu_Usage() {
    switch (Con_NumArgs()) {
    case 1:
        Console_Printf(va("cpu_usage is %d", cpu_watch));
        Console_Printf("");
        break;

    case 2:
        cpu_watch = Con_GetArg(1).toint() ? 1 : 0;
        break;

    default:
        Conlib_Cpu_Usage_Syntax();
        break;
    }
}

static void Conlib_Mount_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("mount <filename$>");
    Console_Printf("");
}

static void Conlib_Mount() {
    switch (Con_NumArgs()) {
    case 2:
        MountVFile(Con_GetArg(1).c_str());
        Console_Printf(va("%s mounted.", Con_GetArg(1).c_str()));
        Console_Printf("");
        break;

    default:
        Conlib_Mount_Syntax();
        break;
    }
}

static void Conlib_Map_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("map [filename$]");
    Console_Printf("");
}

static void Conlib_Map() {
    switch (Con_NumArgs()) {
    case 1:
        Console_Printf("{||||||||||||||||||||}");

        // sprintf(strbuf,"MAP stats for Ä%s~ - Å%d~ layers",mapname,numlayers);
        // Console_Printf(strbuf);
        Console_Printf(va("MAP stats for %s - %d layers", mapname, numlayers));

        // sprintf(strbuf,"Base dimensions Å%d~ x Å%d~", layer[0].sizex,
        // layer[0].sizey);
        // Console_Printf(strbuf);
        Console_Printf(
            va("Base dimensions %dx%d", layer[0].sizex, layer[0].sizey));

        // a=layer[0].sizex*layer[0].sizey;
        // sprintf(strbuf,"MAP using Å%d~ bytes of memory",
        //	a*(2+(numlayers*2))); Console_Printf(strbuf);
        Console_Printf(va("MAP using %d bytes of memory",
            (layer[0].sizex * layer[0].sizey) * (2 + (numlayers * 2))));

        // sprintf(strbuf,"Å%d~ active zones.",numzones);
        // Console_Printf(strbuf);
        Console_Printf(va("%d active zones.", numzones));

        Console_Printf("{||||||||||||||||||||}");

        // sprintf(strbuf,"VSP file: Ä%s~",vspname); Console_Printf(strbuf);
        Console_Printf(va("VSP file: %s", vspname));

        // sprintf(strbuf,"VSP has Å%d~ tiles using Å%d~ bytes",numtiles,
        //	(numtiles*256)+800+(numtiles*3)); Console_Printf(strbuf);
        Console_Printf(va("VSP has %d tiles using %d bytes", numtiles,
            (numtiles * 256) + 800 + (numtiles * 3)));

        Console_Printf("{||||||||||||||||||||}");
        Console_Printf("");
        break;

    // do a map switch
    case 2:
        hookretrace = 0;
        hooktimer = 0;
        kill = 1;

        startmap = Con_GetArg(1);
        Log(va("MAPswitch=%s", startmap));

        // key[SCAN_RQUOTA]=1;
        // key_press(SCAN_RQUOTA);
        break;

    default:
        Conlib_Map_Syntax();
        break;
    }
}

static void Conlib_Ver_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("ver");
    Console_Printf("");
}

static void Conlib_Ver() {
    switch (Con_NumArgs()) {
    case 1:
        Console_Printf("{|||||||||||||||||}");
        Console_Printf(va("VERGE v%s", VERSION));
        Console_Printf("Copyright (C)1998, 2000 vecna");
#ifdef __GNUC__
        Console_Printf("All rights reserved. MING/Win32 build.");
#else
        Console_Printf("All rights reserved. MSVC/Win32 build.");
#endif
        Console_Printf(va("Timestamp %s at %s.", __DATE__, __TIME__));
        Console_Printf("Options: none");
        Console_Printf("{|||||||||||||||||}");
        Console_Printf("");
        break;

    default:
        Conlib_Ver_Syntax();
        break;
    }
}

static void Conlib_BrowseTiles_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("browsetiles");
    Console_Printf("");
}

// TODO: clean this bitch
static void Conlib_BrowseTiles() {
    switch (Con_NumArgs()) {
    case 1: {
        /*
        int x,y,n,k=0,a=0;

        while (key_last()!=SCAN_Q)
        {
                ClearScreen();
                UpdateControls();
                CheckMessages();
                if(key_last()==SCAN_A) { if(a) a=0; else a=1; key_set_last(0); }
                if(key_last()==SCAN_DOWN&&(k+(ty-3)*(tx-3)-tx+4)<numtiles)
                {
                        k+=tx-4;
                        key_set_last(0);
                }
                if(key_last()==SCAN_UP&&k>0)
                {
                        k-=tx-4;
                        key_set_last(0);
                }
                for(y=1; y<ty-2; y++)
                {
                        for(x=1; x<tx-3; x++)
                        {
                                n=((y-1)*(tx-4)+x-1+k);
                                if (n<numtiles)
                                {
                                        if (!a)
        CopySpriteClip(x*16,y*16,16,16,(unsigned char*)((unsigned
        int)vsp+((y-1)*(tx-4)+x-1+k)*256));
                                        else
        CopySpriteClip(x*16,y*16,16,16,(unsigned char*)((unsigned
        int)vsp+tileidx[(y-1)*(tx-4)+x-1+k]*256));
                                }
                        }
                        Font_GotoXY((tx-2)*16-8,y*16+5);
                        sprintf(strbuf,"%i",(y-1)*(tx-4)+k);
                        Font_Print(0,strbuf);
                }
                Font_GotoXY(16,(ty-1)*16-8);
                Font_Print(0,"Hit Q to quit, A to toggle anim,");
                Font_GotoXY(16,(ty-1)*16);
                Font_Print(0,"up/down to change tiles");
                ShowPage();
        }
        */
    } break;

    default:
        Conlib_BrowseTiles_Syntax();
        break;
    }
}

static void Conlib_Warp_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("warp <x#> <y#>");
    Console_Printf("");
}

static void Conlib_Warp() {
    int warpx, warpy;

    switch (Con_NumArgs()) {
    case 3:
        warpx = Con_GetArg(1).toint();
        warpy = Con_GetArg(2).toint();
        if (warpx < 0 || warpx >= layer[0].sizex || warpy < 0 ||
            warpy >= layer[0].sizey) {
            Console_Printf(va("Coordinates out of bounds (%d,%d)",
                layer[0].sizex, layer[0].sizey));
            return;
        }
        player->tx = (short)warpx;
        player->ty = (short)warpy;
        player->x = player->tx * 16;
        player->y = player->ty * 16;
        break;

    default:
        Conlib_Warp_Syntax();
        break;
    }
}

static void Conlib_CameraTracking_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("cameratracking [mode#]");
    Console_Printf("");
}

static void Conlib_CameraTracking() {
    switch (Con_NumArgs()) {
    case 1:
        Console_Printf(va("cameratracking is %d", cameratracking));
        Console_Printf("");
        break;

    case 2:
        cameratracking = (byte)Con_GetArg(1).toint();
        break;

    default:
        Conlib_CameraTracking_Syntax();
        break;
    }
}

static void Conlib_RString_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("rstring [order-list$]");
    Console_Printf("");
}

static void Conlib_RString() {
    switch (Con_NumArgs()) {
    case 1:
        Console_Printf(va("Renderstring: %s", rstring));
        Console_Printf("");
        break;

    case 2:
        rstring = Con_GetArg(1);
        break;

    default:
        Conlib_RString_Syntax();
        break;
    }
}

static void Conlib_ShowObs_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("showobs [1|0]");
    Console_Printf("");
}

static void Conlib_ShowObs() {
    switch (Con_NumArgs()) {
    case 1:
        Console_Printf(va("showobs is %d", showobs));
        Console_Printf("");
        break;

    case 2:
        showobs = Con_GetArg(1).toint() ? 1 : 0;
        break;

    default:
        Conlib_ShowObs_Syntax();
        break;
    }
}

static void Conlib_Phantom_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("phantom [1|0]");
    Console_Printf("");
}

static void Conlib_Phantom() {
    switch (Con_NumArgs()) {
    case 1:
        Console_Printf(va("phantom is %d", phantom));
        Console_Printf("");
        break;

    case 2:
        phantom = Con_GetArg(1).toint() ? 1 : 0;
        break;

    default:
        Conlib_Phantom_Syntax();
        break;
    }
}

static void Conlib_EntityStat_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("entitystat");
    Console_Printf("");
}

static void Conlib_EntityStat() {
    switch (Con_NumArgs()) {
    case 1:
        // sprintf(strbuf,"There are Ç%d~ entities on this map.",entities);
        // Console_Printf(strbuf);
        Console_Printf(va("There are %d entities on this map.", entities));

        // sprintf(strbuf,"Ç%d~ of those are onscreen / active.",cc);
        // Console_Printf(strbuf);
        Console_Printf(va("%d of those are onscreen/active.", cc));
        Console_Printf("");
        break;

    default:
        Conlib_EntityStat_Syntax();
        break;
    }
}

static void Conlib_ActiveEnts_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("activeents");
    Console_Printf("");
}

static void Conlib_ActiveEnts() {
    switch (Con_NumArgs()) {
    case 1: {
        Console_Printf("Active entities:");
        for (int n = 0; n < cc; n++) {
            // sprintf(strbuf,"Ç%d~",entidx[i]);
            // Console_Printf(strbuf);
            Console_Printf(va("%d", entidx[n]));
        }
        Console_Printf("");
    } break;

    default:
        Conlib_ActiveEnts_Syntax();
        break;
    }
}

static void Conlib_Entity_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("entity <slot#>");
    Console_Printf("");
}

static void Conlib_Entity() {
    switch (Con_NumArgs()) {
    case 2: {
        int n = Con_GetArg(1).toint();
        if (n < 0 || n >= entities) {
            Console_Printf("No such entity.");
            Console_Printf("");
            return;
        }

        // sprintf(strbuf,"Desc: Ç%s~", entity[i].desc); Console_Printf(strbuf);
        Console_Printf(va("Desc: %s", entity[n].desc));
        // sprintf(strbuf,"tx: Ç%d~ ty: Ç%d~", entity[i].tx, entity[i].ty);
        // Console_Printf(strbuf);
        Console_Printf(va("tx: %d ty: %d", entity[n].tx, entity[n].ty));
        // sprintf(strbuf,"CHR index: %d", entity[i].chrindex);
        // Console_Printf(strbuf);
        Console_Printf(va("CHR index: %d", entity[n].chrindex));
        Console_Printf("");
    } break;

    default:
        Conlib_Entity_Syntax();
        break;
    }
}

static void Conlib_CurPos_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("curpos");
    Console_Printf("");
}

static void Conlib_CurPos() {
    switch (Con_NumArgs()) {
    case 1:
        // sprintf(strbuf,"xwc: Ç%d~ ywc: Ç%d~", player->x, player->y);
        // Console_Printf(strbuf);
        Console_Printf(va("xwc: %d ywc: %d", player->x, player->y));
        // sprintf(strbuf,"xtc: Ç%d~ ytc: Ç%d~", player->x>>4, player->y>>4);
        // Console_Printf(strbuf);
        Console_Printf(va("xtc: %d ytc: %d", player->x / 16, player->y / 16));
        Console_Printf("");
        break;

    default:
        Conlib_CurPos_Syntax();
        break;
    }
}

static void Conlib_PlayerSpeed_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("playerspeed [speed#]");
    Console_Printf("");
}

static void Conlib_PlayerSpeed() {
    if (!player) {
        Console_Printf("No player.");
        Console_Printf("");
        return;
    }

    switch (Con_NumArgs()) {
    case 1:
        // sprintf(strbuf,"speed is is Å%d~", player->speed);
        // Console_Printf(strbuf);
        Console_Printf(va("speed is is %d", player->speed));
        Console_Printf("");
        break;

    case 2:
        player->speed = (byte)Con_GetArg(1).toint();
        player->speedct = 0;
        break;

    default:
        Conlib_PlayerSpeed_Syntax();
        break;
    }
}

static void Conlib_SpeedDemon_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("speeddemon [1|0]");
    Console_Printf("");
}

static void Conlib_SpeedDemon() {
    switch (Con_NumArgs()) {
    case 1:
        Console_Printf(va("speeddemon is %d", speeddemon));
        Console_Printf("");
        break;

    case 2:
        speeddemon = Con_GetArg(1).toint() ? 1 : 0;
        break;

    default:
        Conlib_SpeedDemon_Syntax();
        break;
    }
}

/*
static void Conlib_RV_Syntax()
{
        Console_Printf("syntax:");
        Console_Printf("rv <variable$>");
        Console_Printf("");
}
*/

static void Conlib_RV() { ReadVCVar(); }

/*
static void Conlib_SV_Syntax()
{
        Console_Printf("syntax:");
        Console_Printf("sv <variable$> <value>");
        Console_Printf("");
}
*/

static void Conlib_SV() { WriteVCVar(); }

static void Conlib_Player_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("player <slot#>");
    Console_Printf("");
}

static void Conlib_Player() {
    switch (Con_NumArgs()) {
    case 2: {
        int n = Con_GetArg(1).toint();
        if (n < 0 || n >= entities) {
            Console_Printf("No such entity.");
            Console_Printf("");
            return;
        }

        player = entity + n;
        playernum = (byte)n;
        //	entity[n].movecode=0;
        entity[n].moving = 0;
        SiftEntities();

        Console_Printf("Player updated.");
        Console_Printf("");
    } break;

    default:
        Conlib_Player_Syntax();
        break;
    }
}

static void Conlib_SpawnEntity_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("spawnentity <tile-x#> <tile-y#> <chr-filename$>");
    Console_Printf("");
}

static void Conlib_SpawnEntity() {
    switch (Con_NumArgs()) {
    case 4: {
        int n = AllocateEntity(
            // atoi((char *) args[1]), atoi((char *) args[2]), (char *)
            // args[3]);
            Con_GetArg(1).toint(), Con_GetArg(2).toint(),
            Con_GetArg(3).c_str());

        // sprintf(strbuf,"Entity %d allocated.",n);
        // Console_Printf(strbuf);
        Console_Printf(va("Entity %d allocated.", n));
        Console_Printf("");
    } break;

    default:
        Conlib_SpawnEntity_Syntax();
        break;
    }
}

static void Conlib_ShowZones_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("showzones [1|0]");
    Console_Printf("");
}

static void Conlib_ShowZones() {
    switch (Con_NumArgs()) {
    case 1:
        // sprintf(strbuf,"showzones is Å%d~", showzone);
        // Console_Printf(strbuf);
        Console_Printf(va("showzones is %d", showzone));
        Console_Printf("");
        break;

    case 2:
        showzone = Con_GetArg(1).toint() ? 1 : 0;
        break;

    default:
        Conlib_ShowZones_Syntax();
        break;
    }
}

#include "vcedit.h"

static void Conlib_EditScript_Syntax() {
    Console_Printf("syntax:");
    Console_Printf("edit <script-filename$>");
    Console_Printf("");
}

static void Conlib_EditScript() {
    switch (Con_NumArgs()) {
    case 2:
        //		key_dest=key_editor;
        V2SE_Main();
        //		key_dest=key_console;
        break;

    default:
        Conlib_EditScript_Syntax();
        break;
    }
}
