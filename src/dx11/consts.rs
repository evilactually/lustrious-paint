
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

#[repr(C)]
pub enum D3D_DRIVER_TYPE {
    D3D_DRIVER_TYPE_UNKNOWN     = 0,
    D3D_DRIVER_TYPE_HARDWARE    = 1,
    D3D_DRIVER_TYPE_REFERENCE   = 2,
    D3D_DRIVER_TYPE_NULL        = 3,
    D3D_DRIVER_TYPE_SOFTWARE    = 4,
    D3D_DRIVER_TYPE_WARP        = 5 
}
