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

fn read_int<T>(f: &mut File) -> std::io::Result<T>
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

fn read_string(f: &mut File, len: usize) -> std::io::Result<String> {
    let mut buf = vec![0; len];
    f.read_exact(&mut buf)?;

    for i in 0..len {
        if buf[i] == 0 {
            return Ok(String::from_utf8_lossy(&buf[0..i]).to_string())
        }
    }

    Ok(String::from_utf8_lossy(&buf).to_string())
}

fn read_vec(f: &mut File, len: usize) -> std::io::Result<Vec<u8>> {
    let mut result = Vec::new();
    result.resize(len, 0);
    f.read_exact(result.as_mut_slice())?;
    Ok(result)
}

struct VCVar {
    name: String,
    start_ofs: u32,
    array_len: u32,
}

struct VCFunction {
    name: String,
    arg_type: String, // ???
    num_args: u32,
    num_locals: u32,
    return_type: u32,
    sys_code_ofs: u32,
}

struct VCString {
    name: String,
    start_ofs: u32,
    array_len: u32,
}

struct SystemVC {
    variables: Vec<VCVar>,
    functions: Vec<VCFunction>,
    strings: Vec<VCString>,
}

fn read_system_vc_index(index_filename: &str) -> std::io::Result<SystemVC> {
    let f = &mut File::open(index_filename)?;
    let mut variables = Vec::new();
    let num_vars = read_int::<u32>(f)?;
    for _ in 0..num_vars {
        let name = read_string(f, 40)?;
        let start_ofs = read_int(f)?;
        let array_len = read_int(f)?;
        // println!("var int {} array len = {}", name, array_len);
        variables.push(VCVar {
            name,
            start_ofs,
            array_len,
        });
    }
    let mut functions = Vec::new();
    let num_functions = read_int::<u32>(f)?;
    for _ in 0..num_functions {
        let name = read_string(f, 40)?;
        let arg_type = read_string(f, 20)?;
        let num_args = read_int(f)?;
        let num_locals = read_int(f)?;
        let return_type = read_int(f)?;
        let sys_code_ofs = read_int(f)?;
        // println!("function {} arg count = {}", name, num_args);
        functions.push(VCFunction {
            name,
            arg_type,
            num_args,
            num_locals,
            return_type,
            sys_code_ofs,
        });
    }
    let mut strings = Vec::new();
    let num_strings = read_int::<u32>(f)?;
    for _ in 0..num_strings {
        let name = read_string(f, 40)?;
        let start_ofs = read_int(f)?;
        let array_len = read_int(f)?;
        // println!("var string {} array len = {}", name, array_len);
        strings.push(VCString {
            name,
            start_ofs,
            array_len,
        });
    }

    Ok(SystemVC {
        variables,
        functions,
        strings,
    })
}

struct MapEvent {
    bytecode: Vec<u8>,
    code_ofs: usize,
}

struct MapCode {
    events: Vec<MapEvent>,
}

fn read_code(name: &str) -> std::io::Result<MapCode> {
    let f = &mut File::open(name)?;

    f.seek(SeekFrom::Current(6))?; // Skip signature

    let offset = read_int::<u32>(f)? as usize;

    println!("Offset is {}", offset);

    f.seek(SeekFrom::Start(offset as u64))?;

    let num_map_events = read_int::<u32>(f)? as usize;
    println!("Map events {}", num_map_events);
    let mut event_offsets: Vec<usize> = Vec::with_capacity(num_map_events as usize + 1);

    for _ in 0..num_map_events {
        event_offsets.push(read_int::<u32>(f)? as usize);
    }

    let code_size = read_int::<u32>(f)? as usize;

    let real_code_start_ofs = f.seek(SeekFrom::Current(0))? as usize;

    event_offsets.push(code_size);

    let mut map_events = Vec::new();

    for evt in 0..num_map_events {
        let code_ofs = event_offsets[evt];
        let end_ofs = event_offsets[evt + 1];
        let event_code_size = end_ofs - code_ofs;
        let bytecode = read_vec(f, event_code_size)?;

        println!("Event {} size: {} bytes", evt, event_code_size);

        map_events.push(MapEvent {
            bytecode,
            code_ofs: real_code_start_ofs + code_ofs,
        });
    }

    let pos = f.seek(SeekFrom::Current(0))? as usize;
    f.seek(SeekFrom::End(0))?;
    let file_size = f.seek(SeekFrom::Current(0))? as usize;

    if real_code_start_ofs + code_size != pos {
        println!("Filesize is {}", file_size);
        println!(
            "Last pos is {}.  Current pos is {}.  Diff is {}",
            real_code_start_ofs + code_size,
            pos,
            pos - real_code_start_ofs - code_size
        );
    }

    Ok(MapCode { events: map_events })
}

struct State<'a> {
    bytecode: &'a Vec<u8>,
    file_ofs: usize,
    pos: usize,
}

impl<'a> State<'a> {
    fn u8(&mut self) -> u8 {
        if self.pos + 1 > self.bytecode.len() {
            panic!("Unexpected EOF!!");
        }

        let result = self.bytecode[self.pos];
        self.pos += 1;
        result
    }

    fn u16(&mut self) -> u16 {
        if self.pos + 2 > self.bytecode.len() {
            panic!("Unexpected EOF reading u16");
        }

        let b = [self.bytecode[self.pos], self.bytecode[self.pos + 1]];
        let result = u16::from_le_bytes(b);
        self.pos += 2;
        result
    }

    fn u32(&mut self) -> u32 {
        if self.pos + 4 > self.bytecode.len() {
            panic!("Unexpected EOF reading u32");
        }

        let b = [
            self.bytecode[self.pos],
            self.bytecode[self.pos + 1],
            self.bytecode[self.pos + 2],
            self.bytecode[self.pos + 3],
        ];
        let result = u32::from_le_bytes(b);
        self.pos += 4;
        result
    }

    fn i32(&mut self) -> i32 {
        if self.pos + 4 > self.bytecode.len() {
            panic!("Unexpected EOF reading u32");
        }

        let b = [
            self.bytecode[self.pos],
            self.bytecode[self.pos + 1],
            self.bytecode[self.pos + 2],
            self.bytecode[self.pos + 3],
        ];
        let result = i32::from_le_bytes(b);
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

mod Operand {
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

mod Expr {
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

mod StdLib {
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

    pub const funcs: &'static [Func] = &[
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
            args: &[],
        },
        Func {
            name: "stopmusic",
            args: &[],
        },
        Func {
            name: "palettemorph",
            args: &[I, I, I, I],
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
        if 0 == func || func >= funcs.len() {
            panic!("Bad function index {}", func);
        }

        &funcs[func]
    }

    pub fn to_string(func: u8) -> &'static str {
        get(func).name
    }
}

fn decode_event(index: &SystemVC, event_idx: usize, code: &MapCode) {
    let event = &code.events[event_idx];
    let mut state = State {
        bytecode: &event.bytecode,
        file_ofs: event.code_ofs,
        pos: 0,
    };

    while state.pos < state.bytecode.len() {
        decode_statement(index, &mut state);
    }
}

fn emit(offset: usize, bytes: &[u8], detail: String) {
    let mut s = String::new();
    for b in bytes {
        if !s.is_empty() {
            s += " ";
        }
        s += &format!("{:02X}", b);
    }

    println!("{:04X}:\t{}\t{}", offset, s, detail);
}

fn decode_statement(index: &SystemVC, state: &mut State) {
    let start_index = state.pos;
    let op = state.u8();

    match op {
        Op::stdlib => {
            let func_idx = state.u8();
            let func = StdLib::get(func_idx);
            emit(
                start_index,
                &[op, func_idx],
                format!("stdlib    \t{} (argcount={})", func.name, func.args.len()),
            );

            for arg in func.args {
                match arg {
                    StdLib::Arg::Int => decode_int_expression(index, state),
                    StdLib::Arg::Str => decode_string_expression(index, state),
                }
            }
        }
        Op::externfunc => {
            let func_idx = state.u16() as usize;
            if func_idx > index.functions.len() {
                panic!("Bad extern function index {}", func_idx);
            }

            emit(
                start_index,
                &[op, (func_idx / 256) as u8, (func_idx & 255) as u8],
                format!(
                    "externfunc {}\t{}",
                    func_idx,
                    index.functions[func_idx].name
                )
            );
        }
        _ => {
            emit(start_index, &[op], "Unknown!!".to_string());
        }
    }
}

fn decode_int_operand(index: &SystemVC, state: &mut State) {
    let start_offset = state.pos;
    let op = state.u8();

    match op {
        Operand::IMMEDIATE => {
            let value = state.i32();
            emit(start_offset, &[
                    op,
                    ((value >> 24) & 255) as u8,
                    ((value >> 16) & 255) as u8,
                    ((value >> 8) & 255) as u8,
                    (value & 255) as u8
                ],
                format!("literal {}", value)
            );
        },
        _ =>
            emit(start_offset, &[op], "UNKNOWN!".to_string())
    }
}

fn decode_int_expression(index: &SystemVC, state: &mut State) {
    let start_offset = state.pos;

    decode_int_operand(index, state);

    let op = state.u8();

    match op {
        Expr::END =>
            emit(start_offset, &[op], "end expression".to_string()),
        _ =>
            emit(start_offset, &[op], "unknown int expression!".to_string()),
    }
}

fn decode_string_expression(index: &SystemVC, state: &mut State) {

}

fn main() -> Result<(), std::io::Error> {
    let args: std::vec::Vec<String> = std::env::args().collect();
    let map_events = read_code(args[1].as_ref())?;

    let system_index = read_system_vc_index("../tcod/system.idx")?;

    for evt in 0..map_events.events.len() {
        decode_event(&system_index, evt, &map_events);
    }

    Ok(())
}
