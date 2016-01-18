use std::fmt;
use std::convert::{From};

fn foo<'a, 'b>(x: &'a u32, y: &'b u32) -> &'a u32 {
    x
}

pub fn fff() {
    let x = 12;

    let z: &u32 = {
        let y = 42;
        foo(&x, &y)
    };
}

// struct Foo<'a> {
//     x: &'a i32,
// }

// pub fn kkk() {
//     let f : Foo;
//     {
//         let y = &5;
//         f = Foo { x: y };
//     };
//     println!("{}", f.x);
// }


#[repr(u8)]
pub enum c_void {
    // Two dummy variants so the #[repr] attribute can be used.
    #[doc(hidden)]
    __variant1,
    #[doc(hidden)]
    __variant2,
}

macro_rules! DECLARE_HANDLE {
    ($name:ident, $inner:ident) => {
        #[repr(C)] #[allow(missing_copy_implementations)] struct $inner { unused: ::c_void }
        pub type $name = *mut $inner;
    };
}

DECLARE_HANDLE!(HINSTANCE, HINSTANCE__);
DECLARE_HANDLE!(HICON, HICON__);
DECLARE_HANDLE!(HCURSOR, HCURSOR__);
DECLARE_HANDLE!(HBRUSH, HBRUSH__);
DECLARE_HANDLE!(LPCWSTR, LPCWSTR__);
DECLARE_HANDLE!(HWND, HWND__);

pub type c_uint = u32;
pub type c_int = i32;
pub type c_long = i64; // is this correct?

pub type UINT = c_uint;

pub type UINT_PTR = c_uint;
pub type LONG_PTR = c_long;

pub type WPARAM = UINT_PTR;
pub type LPARAM = LONG_PTR;
type LRESULT = LONG_PTR;

pub type WNDPROC = Option<unsafe  extern "system" fn(HWND, UINT, WPARAM, LPARAM) -> LRESULT>;

// type HINSTANCE = *mut HINSTANCE__;
// type HICON = *mut HICON__;
// type HCURSOR = *mut HCURSOR__;
// type HBRUSH = *mut HBRUSH__;
// type LPCWSTR = *mut LPCWSTR__;

#[repr(C)] #[derive(Copy)]
pub struct WNDCLASSEXW {
    pub cbSize: ::UINT,
    pub style: ::UINT,
	pub lpfnWndProc: WNDPROC,
    pub cbClsExtra: ::c_int,
    pub cbWndExtra: ::c_int,
    pub hInstance: ::HINSTANCE,
    pub hIcon: ::HICON,
    pub hCursor: ::HCURSOR,
    pub hbrBackground: ::HBRUSH,
    pub lpszMenuName: ::LPCWSTR,
    pub lpszClassName: ::LPCWSTR,
    pub hIconSm: ::HICON,
}

impl Clone for WNDCLASSEXW { fn clone(&self) -> WNDCLASSEXW { *self } }

//extern {

//}


struct Complex(f32,f32);

impl fmt::Display for Complex {
	fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
		write!(f, "{} + {}i", self.0, self.1)
	}
}

#[derive(Debug)]
struct Matrix(f32, f32, f32, f32);


impl fmt::Display for Matrix {
	fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
		write!(f, "( {} {} )\n( {} {} ) ", self.0, self.1, self.2, self.3)
	}
}

impl std::convert::From<Complex> for Matrix {
    fn from(f: Complex) -> Self {
        Matrix(f.0,f.0,f.0,f.0)
    }
}

fn main() {
    println!("hello world");
    println!("{subject} {verb} {predicate}",
             predicate="over the lazy dog",
             subject="the quick brown fox",
             verb="jumps");

    #[derive(Debug)]
    #[derive(Clone)]
    struct Structure(i32);
    println!("{:?}", Structure(3));

    // However, custom types such as this structure require more complicated
    // handling. This will not work.
    println!("{:.3}", 22.0/7.0);
    let s = Structure(2);
    s.clone();

    println!("{}", Complex(1.0,2.0));

    let v: Vec<i32> = Vec::new();

    let j = 10u32;

    let (a,) = (10,);

    let m = Matrix(1.2f32,2.1,3.0,4.0);
    println!("{}", m);
    //let ccc = Complex(2.0f32,5f32);
    let mmm : Matrix = Matrix::from(Complex(2.0f32,5f32));
    let yyy = 10 as f32;
    let aaaaa = u16::from(10u8);
    let uu = 1212usize;
    //let fff : f32 = 10;
}

// df + df + df + df
//
