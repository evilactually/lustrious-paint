
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

use ::types::*;
use ::consts::*;
use ::win32::{HRESULT, HMODULE, UINT};

#[allow(non_snake_case)]
#[link(name = "D3D11")]
extern "system" {
    pub fn GetDevice() -> *const ID3D11DeviceVtbl;
    pub fn D3D11CreateDeviceAndSwapChain(pAdapter: *const IDXGIAdapter, 
                                         DriverType: D3D_DRIVER_TYPE,
                                         Software: HMODULE,
                                         Flags: UINT,
                                         pFeatureLevels: *const D3D_FEATURE_LEVEL,
                                         FeatureLevels: UINT,
                                         SDKVersion: UINT,
                                         pSwapChainDesc: *const DXGI_SWAP_CHAIN_DESC,
                                         ppSwapChain: *mut *const IDXGISwapChain,
                                         ppDevice: *mut *const ID3D11Device,
                                         pFeatureLevel: *mut *const D3D_FEATURE_LEVEL,
                                         ppImmediateContext: *mut *const ID3D11DeviceContext) -> HRESULT;
}