mod constants;
mod decode;

use std::fs::File;
use std::io::{Seek, SeekFrom};

use constants::*;

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
    let num_vars = decode::int::<u32>(f)?;
    for _ in 0..num_vars {
        let name = decode::string(f, 40)?;
        let start_ofs = decode::int(f)?;
        let array_len = decode::int(f)?;
        // println!("var int {} array len = {}", name, array_len);
        variables.push(VCVar {
            name,
            start_ofs,
            array_len,
        });
    }
    let mut functions = Vec::new();
    let num_functions = decode::int::<u32>(f)?;
    for _ in 0..num_functions {
        let name = decode::string(f, 40)?;
        let arg_type = decode::string(f, 20)?;
        let num_args = decode::int(f)?;
        let num_locals = decode::int(f)?;
        let return_type = decode::int(f)?;
        let sys_code_ofs = decode::int(f)?;
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
    let num_strings = decode::int::<u32>(f)?;
    for _ in 0..num_strings {
        let name = decode::string(f, 40)?;
        let start_ofs = decode::int(f)?;
        let array_len = decode::int(f)?;
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

    let offset = decode::int::<u32>(f)? as usize;

    println!("Offset is {}", offset);

    f.seek(SeekFrom::Start(offset as u64))?;

    let num_map_events = decode::int::<u32>(f)? as usize;
    println!("Map events {}", num_map_events);
    let mut event_offsets: Vec<usize> = Vec::with_capacity(num_map_events as usize + 1);

    for _ in 0..num_map_events {
        event_offsets.push(decode::int::<u32>(f)? as usize);
    }

    let code_size = decode::int::<u32>(f)? as usize;

    let real_code_start_ofs = f.seek(SeekFrom::Current(0))? as usize;

    event_offsets.push(code_size);

    let mut map_events = Vec::new();

    for evt in 0..num_map_events {
        let code_ofs = event_offsets[evt];
        let end_ofs = event_offsets[evt + 1];
        let event_code_size = end_ofs - code_ofs;
        let bytecode = decode::vec(f, event_code_size)?;

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

    fn emit(&self, offset: usize, detail: String) {
        let mut s = String::new();
        for b in offset..self.pos {
            if !s.is_empty() {
                s += " ";
            }
            s += &format!("{:02X}", self.bytecode[b]);
        }
    
        const TARGET_COLUMN: usize = 40;
        let padding = " ".repeat(TARGET_COLUMN - s.len());
    
        println!("{:04X}:    {bytes}{padding}{detail}", offset + self.file_ofs, bytes=s, padding=padding, detail=detail);    
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

fn decode_statement(index: &SystemVC, state: &mut State) {
    let start_index = state.pos;
    let op = state.u8();

    match op {
        op::STDLIB => {
            let func_idx = state.u8();
            let func = std_lib::get(func_idx);
            state.emit(
                start_index,
                format!("stdlib    \t{} (argcount={})", func.name, func.args.len()),
            );

            for arg in func.args {
                match arg {
                    std_lib::Arg::Int => decode_int_expression(state),
                    std_lib::Arg::Str => decode_string_expression(index, state),
                }
            }
        }
        op::EXTERNFUNC => {
            let func_idx = state.u16() as usize;
            if func_idx > index.functions.len() {
                panic!("Bad extern function index {}", func_idx);
            }

            state.emit(
                start_index,
                format!(
                    "externfunc {}\t{}",
                    func_idx,
                    index.functions[func_idx].name
                )
            );
        }
        op::IF_ => {
            state.emit(
                start_index,
                "begin if statement".to_string()
            );

            decode_if_expression(index, state);

            let pos = state.pos;
            let jump_dest = state.u32();
            state.emit(
                pos,
                format!("jump dest {:08X}", jump_dest)
            );
        }
        _ => {
            panic!("Unknown statement op {:02X}", op);
        }
    }
}

fn decode_if_expression(index: &SystemVC, state: &mut State) {
    decode_int_operand(state);

    let pos = state.pos;
    let op = state.u8();
    match op {
        if_op::ZERO =>
            state.emit(
                pos,
                "nonzero".to_string()
            ),

        _ =>
            panic!("Unknown if op {:02X}", op)
    }
}

// ResolveOperand
fn decode_int_operand(state: &mut State) {
    let start_offset = state.pos;
    let op = state.u8();

    match op {
        operand::IMMEDIATE => {
            let value = state.i32();
            state.emit(start_offset,
                format!("literal {}", value)
            );
        },
        _ =>
            state.emit(start_offset, "UNKNOWN!".to_string())
    }
}

fn decode_int_expression(state: &mut State) {
    let start_offset = state.pos;

    decode_int_operand(state);

    let op = state.u8();

    match op {
        expr::END =>
            state.emit(start_offset, "end expression".to_string()),
        _ =>
            state.emit(start_offset, "unknown int expression!".to_string()),
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
