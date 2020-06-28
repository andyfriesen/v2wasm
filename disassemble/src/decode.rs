use std::mem::size_of;
use std::fs::File;
use std::io::Read;

pub trait FromLE
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

pub fn int<T>(f: &mut File) -> std::io::Result<T>
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

pub fn string(f: &mut File, len: usize) -> std::io::Result<String> {
    let mut buf = vec![0; len];
    f.read_exact(&mut buf)?;

    for i in 0..len {
        if buf[i] == 0 {
            return Ok(String::from_utf8_lossy(&buf[0..i]).to_string())
        }
    }

    Ok(String::from_utf8_lossy(&buf).to_string())
}

pub fn vec(f: &mut File, len: usize) -> std::io::Result<Vec<u8>> {
    let mut result = vec![0; len];
    f.read_exact(result.as_mut_slice())?;
    Ok(result)
}
