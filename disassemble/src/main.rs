use std::fs::File;
use std::io::{Read, Seek, SeekFrom};
use std::mem::size_of;

trait FromLE
where
    Self: std::marker::Sized,
{
    fn from_le(i: Self) -> Self;
}

impl FromLE for u32 {
    fn from_le(i: u32) -> u32 {
        u32::from_le(i)
    }
}

impl FromLE for u16 {
    fn from_le(i: u16) -> u16 {
        u16::from_le(i)
    }
}

impl FromLE for u8 {
    fn from_le(i: u8) -> u8 {
        i
    }
}

fn decode<T>(f: &mut File) -> std::io::Result<T>
where
    T: std::marker::Sized,
    T: std::default::Default,
    T: FromLE,
{
    // let mut buf = [0; size_of::<T>()]; // Rust :(
    let r: T = Default::default();

    unsafe {
        let buf: *mut u8 = std::mem::transmute(&r);
        f.read_exact(std::slice::from_raw_parts_mut(buf, size_of::<T>()))?;
    }

    Ok(FromLE::from_le(r))
}

struct MapEvent {
    bytecode: Vec<u8>,
}

fn read_code(name: &str) -> std::io::Result<Vec<MapEvent>> {
    let f = &mut File::open(name)?;

    f.seek(SeekFrom::Current(6))?;
    let offset = decode::<u32>(f)? as usize;

    println!("Offset is {}", offset);

    f.seek(SeekFrom::Start(offset as u64))?;

    let num_map_events = decode::<u32>(f)? as usize;
    println!("Map events {}", num_map_events);
    let mut event_offsets: Vec<usize> = Vec::with_capacity(num_map_events as usize + 1);

    for _ in 0..num_map_events {
        event_offsets.push(decode::<u32>(f)? as usize);
    }

    let code_size = decode::<u32>(f)? as usize;

    event_offsets.push(code_size);

    let mut map_events = Vec::new();

    for evt in 0..num_map_events {
        let event_code_size = event_offsets[evt + 1] - event_offsets[evt];
        let mut bytecode = Vec::new();
        bytecode.resize(event_code_size, 0);
        f.read_exact(bytecode.as_mut_slice())?;

        println!("Event {} size: {} bytes", evt, event_code_size);

        map_events.push(MapEvent { bytecode: bytecode });
    }

    let pos = f.seek(SeekFrom::Current(0))? as usize;
    f.seek(SeekFrom::End(0));
    let file_size = f.seek(SeekFrom::Current(0))? as usize;

    if offset + code_size != pos {
        println!("Filesize is {}", file_size);
        println!(
            "Last pos is {}.  Current pos is {}.  Diff is {}",
            offset + code_size,
            pos,
            pos - offset - code_size
        );
    }

    Ok(map_events)
}

struct State<'a> {
    evt: &'a MapEvent,
    pos: usize,
}

impl<'a> State<'a> {
    fn u8(&mut self) -> u8 {
        if self.pos + 1 > self.evt.bytecode.len() {
            panic!("Unexpected EOF!!");
        }

        let result = self.evt.bytecode[self.pos];
        self.pos += 1;
        result
    }

    fn u16(&mut self) -> u16 {
        if self.pos + 2 > self.evt.bytecode.len() {
            panic!("Unexpected EOF reading u16");
        }

        let b = [self.evt.bytecode[self.pos], self.evt.bytecode[self.pos + 1]];
        let result = u16::from_le_bytes(b);
        self.pos += 2;
        result
    }

    fn u32(&mut self) -> u32 {
        if self.pos + 4 > self.evt.bytecode.len() {
            panic!("Unexpected EOF reading u32");
        }

        let b = [
            self.evt.bytecode[self.pos],
            self.evt.bytecode[self.pos + 1],
            self.evt.bytecode[self.pos + 2],
            self.evt.bytecode[self.pos + 3],
        ];
        let result = u32::from_le_bytes(b);
        self.pos += 4;
        result
    }
}

mod Op {
    pub const stdlib: u8 = 1;
    pub const localfunc: u8 = 2;
    pub const externfunc: u8 = 3;
    pub const if_: u8 = 4;
    pub const else_: u8 = 5;
    pub const goto: u8 = 6;
    pub const switch: u8 = 7;
    pub const case: u8 = 8;
    pub const return_: u8 = 9;
    pub const assign: u8 = 10;
    pub const setretval: u8 = 11;
    pub const setlocalstack: u8 = 12;
    pub const setretstring: u8 = 13;
}

mod StdLib {
    pub const names: &'static [&'static str] = &[
        "exit",
        "message",
        "malloc",
        "free",
        "pow",
        "loadimage",
        "copysprite",
        "tcopysprite",
        "render",
        "showpage",
        "entityspawn",
        "setplayer",
        "map",
        "loadfont",
        "playfli",
        // B
        "gotoxy",
        "printstring",
        "loadraw",
        "settile",
        "allowconsole",
        "scalesprite",
        "processentities",
        "updatecontrols",
        "unpress",
        "entitymove",
        "hline",
        "vline",
        "line",
        "circle",
        "circlefill", // 30
        // C
        "rect",
        "rectfill",
        "strlen",
        "strcmp",
        "cd_stop",
        "cd_play",
        "fontwidth",
        "fontheight",
        "setpixel",
        "getpixel",
        "entityonscreen",
        "random",
        "gettile",
        "hookretrace",
        "hooktimer",
        // D
        "setresolution",
        "setrstring",
        "setcliprect",
        "setrenderdest",
        "restorerendersettings",
        "partymove",
        "sin",
        "cos",
        "tan",
        "readmouse",
        "setclip",
        "setlucent",
        "wrapblit",
        "twrapblit",
        "setmousepos", // 60
        // E
        "hookkey",
        "playmusic",
        "stopmusic",
        "palettemorph",
        "fopen",
        "fclose",
        "quickread",
        "addfollower",
        "killfollower",
        "killallfollowers",
        "resetfollowers",
        "flatpoly",
        "tmappoly",
        "cachesound",
        "freeallsounds",
        // F
        "playsound",
        "rotscale",
        "mapline",
        "tmapline",
        "val",
        "tscalesprite",
        "grabregion",
        "log",
        "fseekline",
        "fseekpos",
        "fread",
        "fgetbyte",
        "fgetword",
        "fgetquad",
        "fgetline", // 90
        // G
        "fgettoken",
        "fwritestring",
        "fwrite",
        "frename",
        "fdelete",
        "fwopen",
        "fwclose",
        "memcpy",
        "memset",
        "silhouette",
        "initmosaictable",
        "mosaic",
        "writevars",
        "readvars",
        "callevent", // 105
        // H
        "asc",
        "callscript",
        "numforscript",
        "filesize",
        "ftell",
        "changechr",
        "rgb",
        "getr",
        "getg",
        "getb",
        "mask",
        "changeall",
        "sqrt",
        "fwritebyte",
        "fwriteword", // 120
        // I
        "fwritequad",
        "calclucent",
        "imagesize",
    ];

    pub fn to_string(func: u8) -> &'static str {
        let func = func as usize;
        if func >= names.len() {
            panic!("Bad function index {}", func);
        }
        return names[func];
    }
}

fn decode_event(evt: &MapEvent) {
    let mut state = State { evt: evt, pos: 0 };

    while state.pos < evt.bytecode.len() {
        decode_statement(&mut state);
    }
}

fn decode_statement(state: &mut State) {
    let op = state.u8();

    let a = match op {
        Op::stdlib => {
            let func_idx = state.u8();
            format!(
                "stdlib    \t{:X}\t{}",
                func_idx,
                StdLib::to_string(func_idx)
            )
        }
        Op::externfunc => {
            let func_idx = state.u16();
            format!("externfunc\t{:X}", func_idx)
        }
        _ => "".to_string(),
    };

    println!("\t{:X}\t{}", op, a);
}

fn main() -> Result<(), std::io::Error> {
    let args: std::vec::Vec<String> = std::env::args().collect();
    let map_events = read_code(args[1].as_ref())?;

    for evt in map_events {
        decode_event(&evt);
    }

    Ok(())
}
