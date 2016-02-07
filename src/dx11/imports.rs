
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

use ::types::*;
use ::consts::*;
use ::win32::{HRESULT};

#[allow(non_snake_case)]
extern "system" {
	pub fn GetDevice() -> *const ID3D11DeviceVtbl;
	pub fn D3D11CreateDeviceAndSwapChain(pAdapter: *const IDXGIAdapter, DriverType: D3D_DRIVER_TYPE) -> HRESULT;

	// pub fn D3D11CreateDeviceAndSwapChain(IDXGIAdapter* pAdapter,
 //    									 DriverType: D3D_DRIVER_TYPE,
 //     									 Software: HMODULE,
 //    									 Flags: UINT,
 //    									 pFeatureLevels: *const D3D_FEATURE_LEVEL,
 //    								     FeatureLevels: UINT,
 //    									 SDKVersion: UINT,
 //    									 pSwapChainDesc: *const DXGI_SWAP_CHAIN_DESC,
 //    									 ppSwapChain: **const IDXGISwapChain,
 //     									 ppDevice: **ID3D11Device,
 //    									 pFeatureLevel: *D3D_FEATURE_LEVEL,
 //    									 ppImmediateContext: **ID3D11DeviceContext) -> HRESULT;
}