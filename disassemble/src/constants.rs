
pub mod op {
    pub const STDLIB: u8 = 1;
    pub const LOCALFUNC: u8 = 2;
    pub const EXTERNFUNC: u8 = 3;
    pub const IF_: u8 = 4;
    pub const ELSE_: u8 = 5;
    pub const GOTO: u8 = 6;
    pub const SWITCH: u8 = 7;
    pub const CASE: u8 = 8;
    pub const RETURN_: u8 = 9;
    pub const ASSIGN: u8 = 10;
    pub const SETRETVAL: u8 = 11;
    pub const SETLOCALSTACK: u8 = 12;
    pub const SETRETSTRING: u8 = 13;
}

pub mod operand {
    pub const IMMEDIATE: u8 = 1;
    pub const HVAR0: u8 = 2;
    pub const HVAR1: u8 = 3;
    pub const UVAR: u8 = 4;
    pub const UVARRAY: u8 = 5;
    pub const LVAR: u8 = 6;
    pub const BFUNC: u8 = 7;
    pub const UFUNC: u8 = 8;
    pub const GROUP: u8 = 9;
    pub const STRING: u8 = 10;
    pub const SARRAY: u8 = 11;
    pub const SLOCAL: u8 = 12;
}

pub mod if_op {
    pub const ZERO: u8 = 1;
    pub const NONZERO: u8 = 2;
    pub const EQUALTO: u8 = 3;
    pub const NOTEQUAL: u8 = 4;
    pub const GREATERTHAN: u8 = 5;
    pub const LESSTHAN: u8 = 6;
    pub const GREATERTHANOREQUAL: u8 = 7;
    pub const LESSTHANOREQUAL: u8 = 8;
    pub const GROUP: u8 = 9;
    pub const UNGROUP: u8 = 10;
    pub const AND: u8 = 11;
    pub const OR: u8 = 12;
}

pub mod expr {
    pub const ADD: u8 = 1;
    pub const SUB: u8 = 2;
    pub const MULT: u8 = 3;
    pub const DIV: u8 = 4;
    pub const MOD: u8 = 5;
    pub const SHL: u8 = 6;
    pub const SHR: u8 = 7;
    pub const AND: u8 = 8;
    pub const OR: u8 = 9;
    pub const XOR : u8= 10;
    pub const END : u8= 11;
}

pub mod string_expr {
    pub const IMMEDIATE: u8 = 1;
    pub const GLOBAL: u8 = 2;
    pub const ARRAY: u8 = 3;
    pub const NUMSTR: u8 = 4;
    pub const LEFT: u8 = 5;
    pub const RIGHT: u8 = 6;
    pub const MID: u8 = 7;
    pub const LOCAL: u8 = 8;
    pub const ADD: u8 = 9;
    pub const CHR: u8 = 10;
    pub const END: u8 = 11;
    pub const UFUNC: u8 = 12;
    pub const BFUNC: u8 = 13;
}

pub mod std_lib {
    pub enum Arg {
        Int,
        Str,
    }

    const I: Arg = Arg::Int;
    const S: Arg = Arg::Str;

    pub struct Func {
        pub name: &'static str,
        pub args: &'static [Arg],
    }

    pub const FUNCS: &'static [Func] = &[
        Func {
            name: "$$$ILLEGAL_ZERO$$$",
            args: &[],
        },
        Func {
            name: "exit",
            args: &[],
        },
        Func {
            name: "message",
            args: &[],
        },
        Func {
            name: "malloc",
            args: &[I],
        },
        Func {
            name: "free",
            args: &[I],
        },
        Func {
            name: "pow",
            args: &[I, I],
        },
        Func {
            name: "loadimage",
            args: &[],
        },
        Func {
            name: "copysprite",
            args: &[I, I, I, I, I],
        },
        Func {
            name: "tcopysprite",
            args: &[I, I, I, I, I],
        },
        Func {
            name: "render",
            args: &[],
        },
        Func {
            name: "showpage",
            args: &[],
        },
        Func {
            name: "entityspawn",
            args: &[],
        },
        Func {
            name: "setplayer",
            args: &[I],
        },
        Func {
            name: "map",
            args: &[],
        },
        Func {
            name: "loadfont",
            args: &[],
        },
        Func {
            name: "playfli",
            args: &[],
        },
        // B
        Func {
            name: "gotoxy",
            args: &[I, I],
        },
        Func {
            name: "printstring",
            args: &[],
        },
        Func {
            name: "loadraw",
            args: &[],
        },
        Func {
            name: "settile",
            args: &[I, I, I, I],
        },
        Func {
            name: "allowconsole",
            args: &[I],
        },
        Func {
            name: "scalesprite",
            args: &[I, I, I, I, I, I, I],
        },
        Func {
            name: "processentities",
            args: &[],
        },
        Func {
            name: "updatecontrols",
            args: &[],
        },
        Func {
            name: "unpress",
            args: &[I],
        },
        Func {
            name: "entitymove",
            args: &[],
        },
        Func {
            name: "hline",
            args: &[I, I, I, I],
        },
        Func {
            name: "vline",
            args: &[I, I, I, I],
        },
        Func {
            name: "line",
            args: &[I, I, I, I, I],
        },
        Func {
            name: "circle",
            args: &[I, I, I, I],
        },
        Func {
            name: "circlefill",
            args: &[I, I, I, I],
        }, // 30
        // C
        Func {
            name: "rect",
            args: &[I, I, I, I, I],
        },
        Func {
            name: "rectfill",
            args: &[I, I, I, I, I],
        },
        Func {
            name: "strlen",
            args: &[],
        },
        Func {
            name: "strcmp",
            args: &[],
        },
        Func {
            name: "cd_stop",
            args: &[],
        },
        Func {
            name: "cd_play",
            args: &[I],
        },
        Func {
            name: "fontwidth",
            args: &[I],
        },
        Func {
            name: "fontheight",
            args: &[I],
        },
        Func {
            name: "setpixel",
            args: &[I, I, I],
        },
        Func {
            name: "getpixel",
            args: &[I, I],
        },
        Func {
            name: "entityonscreen",
            args: &[I],
        },
        Func {
            name: "random",
            args: &[I],
        },
        Func {
            name: "gettile",
            args: &[I, I, I],
        },
        Func {
            name: "hookretrace",
            args: &[],
        },
        Func {
            name: "hooktimer",
            args: &[],
        },
        // D
        Func {
            name: "setresolution",
            args: &[I, I],
        },
        Func {
            name: "setrstring",
            args: &[],
        },
        Func {
            name: "setcliprect",
            args: &[I, I, I, I],
        },
        Func {
            name: "setrenderdest",
            args: &[I, I, I],
        },
        Func {
            name: "restorerendersettings",
            args: &[],
        },
        Func {
            name: "partymove",
            args: &[],
        },
        Func {
            name: "sin",
            args: &[I],
        },
        Func {
            name: "cos",
            args: &[I],
        },
        Func {
            name: "tan",
            args: &[I],
        },
        Func {
            name: "readmouse",
            args: &[],
        },
        Func {
            name: "setclip",
            args: &[I],
        },
        Func {
            name: "setlucent",
            args: &[],
        },
        Func {
            name: "wrapblit",
            args: &[],
        },
        Func {
            name: "twrapblit",
            args: &[],
        },
        Func {
            name: "setmousepos",
            args: &[],
        }, // 60
        // E
        Func {
            name: "hookkey",
            args: &[],
        },
        Func {
            name: "playmusic",
            args: &[S],
        },
        Func {
            name: "stopmusic",
            args: &[],
        },
        Func {
            name: "palettemorph",
            args: &[I, I, I, I, I],
        },
        Func {
            name: "fopen",
            args: &[],
        },
        Func {
            name: "fclose",
            args: &[],
        },
        Func {
            name: "quickread",
            args: &[],
        },
        Func {
            name: "addfollower",
            args: &[],
        },
        Func {
            name: "killfollower",
            args: &[],
        },
        Func {
            name: "killallfollowers",
            args: &[],
        },
        Func {
            name: "resetfollowers",
            args: &[],
        },
        Func {
            name: "flatpoly",
            args: &[],
        },
        Func {
            name: "tmappoly",
            args: &[],
        },
        Func {
            name: "cachesound",
            args: &[],
        },
        Func {
            name: "freeallsounds",
            args: &[],
        },
        // F
        Func {
            name: "playsound",
            args: &[],
        },
        Func {
            name: "rotscale",
            args: &[],
        },
        Func {
            name: "mapline",
            args: &[],
        },
        Func {
            name: "tmapline",
            args: &[],
        },
        Func {
            name: "val",
            args: &[],
        },
        Func {
            name: "tscalesprite",
            args: &[],
        },
        Func {
            name: "grabregion",
            args: &[],
        },
        Func {
            name: "log",
            args: &[],
        },
        Func {
            name: "fseekline",
            args: &[],
        },
        Func {
            name: "fseekpos",
            args: &[],
        },
        Func {
            name: "fread",
            args: &[],
        },
        Func {
            name: "fgetbyte",
            args: &[],
        },
        Func {
            name: "fgetword",
            args: &[],
        },
        Func {
            name: "fgetquad",
            args: &[],
        },
        Func {
            name: "fgetline",
            args: &[],
        }, // 90
        // G
        Func {
            name: "fgettoken",
            args: &[],
        },
        Func {
            name: "fwritestring",
            args: &[],
        },
        Func {
            name: "fwrite",
            args: &[],
        },
        Func {
            name: "frename",
            args: &[],
        },
        Func {
            name: "fdelete",
            args: &[],
        },
        Func {
            name: "fwopen",
            args: &[],
        },
        Func {
            name: "fwclose",
            args: &[],
        },
        Func {
            name: "memcpy",
            args: &[],
        },
        Func {
            name: "memset",
            args: &[],
        },
        Func {
            name: "silhouette",
            args: &[],
        },
        Func {
            name: "initmosaictable",
            args: &[],
        },
        Func {
            name: "mosaic",
            args: &[],
        },
        Func {
            name: "writevars",
            args: &[],
        },
        Func {
            name: "readvars",
            args: &[],
        },
        Func {
            name: "callevent",
            args: &[],
        }, // 105
        // H
        Func {
            name: "asc",
            args: &[],
        },
        Func {
            name: "callscript",
            args: &[],
        },
        Func {
            name: "numforscript",
            args: &[],
        },
        Func {
            name: "filesize",
            args: &[],
        },
        Func {
            name: "ftell",
            args: &[],
        },
        Func {
            name: "changechr",
            args: &[],
        },
        Func {
            name: "rgb",
            args: &[],
        },
        Func {
            name: "getr",
            args: &[],
        },
        Func {
            name: "getg",
            args: &[],
        },
        Func {
            name: "getb",
            args: &[],
        },
        Func {
            name: "mask",
            args: &[],
        },
        Func {
            name: "changeall",
            args: &[],
        },
        Func {
            name: "sqrt",
            args: &[],
        },
        Func {
            name: "fwritebyte",
            args: &[],
        },
        Func {
            name: "fwriteword",
            args: &[],
        }, // 120
        // I
        Func {
            name: "fwritequad",
            args: &[],
        },
        Func {
            name: "calclucent",
            args: &[],
        },
        Func {
            name: "imagesize",
            args: &[],
        },
    ];

    pub fn get(func: u8) -> &'static Func {
        let func = func as usize;
        if 0 == func || func >= FUNCS.len() {
            panic!("Bad function index {}", func);
        }

        &FUNCS[func]
    }

    pub fn to_string(func: u8) -> &'static str {
        get(func).name
    }
}

pub mod hardvar0 {
    pub const VARS: &'static [&'static str] = &[
        "xwin", "ywin", "cameratracking", "timer", "up", "down", "left", "right",
        "b1", "b2", "b3", "b4", "screenx", "screeny",
        "player", // 15

        "numentsonscreen", "tracker", "mx", "my", "mb", "vctrace", "image_width",
        "image_height", "music_volume", "vsp", "lastent", "last_pressed",
        "map_width", "map_height", "vsync", "numents", "mask_color",
        "bitdepth" // 30
    ];
}

pub mod hardvar1 {
    pub const VARS: &'static [&'static str] = &[
        "screen", "entity.x", "entity.y", "entity.tx", "entity.ty", "entity.facing",
        "entity.moving", "entity.specframe", "entity.speed", "entity.movecode",
        "entsonscreen", "key", "layer.hline", "byte",
        "word", // 15

        "quad", "pal", "sbyte", "sword", "squad", "entity.isob", "entity.canob",
        "entity.autoface", "entity.visible", "entity.on", "chr_data",
        "entity.width", "entity.height", "entity.chrindex",
        "zone.event", // 30

        "zone.chance",
    ];
}
