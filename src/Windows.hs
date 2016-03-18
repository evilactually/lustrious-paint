{-# LANGUAGE ForeignFunctionInterface #-}

module Windows where

-- import Foreign.Ptr(Ptr)
-- import Data.Void(Void)
import Foreign
import Foreign.C.Types
import Data.Void(Void)
import Foreign.C.String(CString)

foreign import ccall "sin"
  c_sin :: CDouble -> CDouble

foreign import stdcall "GetModuleHandleA"
  c_GetModuleHandleA :: CString -> IO(Ptr Void)

newtype HWND = Ptr Void

c_WndProc :: HWND -> Word32 -> Word32 -> Word32 -> Word32
c_WndProc _ _ _ x = x