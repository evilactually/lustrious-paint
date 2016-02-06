
#![crate_name = "win32"]
#![crate_type = "rlib"]

extern crate ctypes;

mod types;
mod imports;
mod consts;
mod wrappers;

pub use consts::*;
pub use types::*;
pub use wrappers::*;
pub use imports::{DefWindowProcA};

pub fn low_word(x: LONG) -> LONG {
    x & 0x0000ffff
}

#[allow(overflowing_literals)] 
pub fn high_word(x: LONG) -> LONG {
    (x & 0xffff0000) >> 16
}

pub fn get_x_lparam(lp: LONG) -> LONG {
    low_word(lp)
}

pub fn get_y_lparam(lp: LONG) -> LONG {
    high_word(lp)
}