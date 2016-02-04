
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