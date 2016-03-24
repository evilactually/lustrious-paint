{-# LANGUAGE ForeignFunctionInterface #-}
{-# LANGUAGE PatternSynonyms #-}
{-# LANGUAGE GeneralizedNewtypeDeriving #-}
{-# LANGUAGE CPP #-}

module Main where

import Data.Char(toUpper)
import Foreign.C.Types
import Foreign.Ptr
import Data.Void
import Foreign.C.String
import Foreign.Storable
import Data.Bits((.|.), (.&.), shiftL, shiftR, Bits)
import Data.Int(Int32)
import Data.Word

type HANDLE = Ptr Void

type HINSTANCE = HANDLE

type HMODULE = HANDLE

type HWND = HANDLE

type HICON = HANDLE

type HCURSOR = HICON

type HBRUSH = HANDLE

type LPCTSTR = CString

type HGDIOBJ = HANDLE

type LONG_PTR = CLong

type LRESULT = LONG_PTR

type LPARAM = LONG_PTR

type WPARAM = LONG_PTR

data WNDCLASSEX = WNDCLASSEX { wcSize :: CUInt,
                               wcStyle :: ClassStyle,
                               wcWindowProcedure :: FunPtr (WindowProcedure), 
                               wcClassExtra :: CInt,
                               wcWindowExtra :: CInt,
                               wcInstance :: HINSTANCE,
                               wcIcon :: HICON,
                               wcCursor :: HCURSOR,
                               wcBackground :: HBRUSH,
                               wcMenuName :: LPCTSTR,
                               wcClassName:: LPCTSTR,
                               wcIconSmall :: HICON }

type PWNDCLASSEX = Ptr WNDCLASSEX

instance Storable WNDCLASSEX where
        alignment _ = 4
        sizeOf _    = 48
        peek ptr    = WNDCLASSEX
            <$> peekByteOff ptr 0  -- wcSize :: CUInt
            <*> peekByteOff ptr 4  -- wcStyle :: CUInt
            <*> peekByteOff ptr 8  -- wcWindowProcedure :: FunPtr (Ptr Void)
            <*> peekByteOff ptr 12 -- wcClassExtra :: CInt 
            <*> peekByteOff ptr 16 -- wcWindowExtra :: CInt
            <*> peekByteOff ptr 20 -- wcInstance :: CInt
            <*> peekByteOff ptr 24 -- wcIcon :: CInt
            <*> peekByteOff ptr 28 -- wcCursor :: CInt
            <*> peekByteOff ptr 32 -- wcBackground :: CInt
            <*> peekByteOff ptr 36 -- wcMenuName :: CInt
            <*> peekByteOff ptr 40 -- wcClassName :: CInt
            <*> peekByteOff ptr 44 -- wcIconSmall :: CInt
        poke ptr poked = do
            pokeByteOff ptr 0  (wcSize poked)
            pokeByteOff ptr 4  (wcStyle poked)
            pokeByteOff ptr 8  (wcWindowProcedure poked)
            pokeByteOff ptr 12 (wcClassExtra poked)
            pokeByteOff ptr 16 (wcWindowExtra poked)
            pokeByteOff ptr 20 (wcInstance poked)
            pokeByteOff ptr 24 (wcIcon poked)
            pokeByteOff ptr 28 (wcCursor poked)
            pokeByteOff ptr 32 (wcBackground poked)
            pokeByteOff ptr 36 (wcMenuName poked)
            pokeByteOff ptr 40 (wcClassName poked)
            pokeByteOff ptr 44 (wcIconSmall poked)


newtype ClassStyle = ClassStyle CUInt
  deriving (Eq, Storable, Bits, Show)

pattern CS_HREDRAW = ClassStyle 0x0002
pattern CS_VREDRAW = ClassStyle 0x0001

newtype IconId = IconId CInt
  deriving (Eq, Storable, Show)

pattern IDI_APPLICATION = IconId 32512

newtype CursorId = CursorId CInt
  deriving (Eq, Storable, Show)

pattern IDC_ARROW = CursorId 32512

-- GDI Stock Objects
pattern BLACK_BRUSH = 4

--type Compare = Int -> Int -> Bool
--foreign import ccall "wrapper"
--  mkCompare :: Compare -> IO (FunPtr Compare)

foreign import stdcall "LoadIconA"
  c_LoadIconByName :: HINSTANCE -> LPCTSTR -> IO(HICON)

foreign import stdcall "LoadIconA"
  c_LoadIconById :: HINSTANCE -> IconId -> IO(HICON)

foreign import stdcall "LoadCursorA"
  c_LoadCursorByName :: HINSTANCE -> LPCTSTR -> IO(HCURSOR)

foreign import stdcall "LoadCursorA"
  c_LoadCursorById :: HINSTANCE -> CursorId -> IO(HCURSOR)

foreign import stdcall "GetModuleHandleA"
  c_GetModuleHandleA :: CString -> IO(HMODULE)

foreign import stdcall "GetStockObject"
  c_GetStockObject :: CInt -> HGDIOBJ

foreign import stdcall "DefWindowProcA"
  c_DefWindowProcA :: WindowProcedure

mkMask mask_width = (0x1 `shiftL` mask_width) - 1

splitWord64 :: Word64 -> (Word32, Word32)
splitWord64 word = (fromIntegral $ (word .&. hi_mask) `shiftR` mask_width, fromIntegral $ word .&. lo_mask)
  where
    mask_width = 32
    lo_mask = mkMask mask_width
    hi_mask = lo_mask `shiftL` mask_width

splitWord32 :: Word32 -> (Word16, Word16)
splitWord32 word = (fromIntegral $ (word .&. hi_mask) `shiftR` mask_width, fromIntegral $ word .&. lo_mask)
  where
    mask_width = 16
    lo_mask = mkMask mask_width
    hi_mask = lo_mask `shiftL` mask_width

getXLParam :: LPARAM -> CLong
getXLParam p = fromIntegral x
  where
#if ARCH == ARCH_x86
    (_,x) = splitWord32 $ fromIntegral p
#else
    (_,x) = splitWord64 $ fromIntegral p
#endif

getYLParam :: LPARAM -> CLong
getYLParam p = fromIntegral y
  where
#if ARCH == ARCH_x86
    (y,_) = splitWord32 $ fromIntegral p
#else
    (y,_) = splitWord64 $ fromIntegral p
#endif

type WindowProcedure = HWND -> CUInt -> WPARAM -> LPARAM -> IO(LRESULT)

windowProcedure :: WindowProcedure
windowProcedure hwnd msg wparam lparam = c_DefWindowProcA hwnd msg wparam lparam

foreign import ccall "wrapper" mkWindowProcedurePtr :: WindowProcedure -> IO (FunPtr (WindowProcedure))

main = do
  instance_handle <- c_GetModuleHandleA nullPtr
  icon <- c_LoadIconById instance_handle IDI_APPLICATION
  cursor <- c_LoadCursorById instance_handle IDC_ARROW
  class_name <- newCString "WindowClass"
  proc <- mkWindowProcedurePtr windowProcedure
  let wndclass = WNDCLASSEX {
    wcSize = fromIntegral $ sizeOf wndclass,
    wcStyle = CS_HREDRAW .|. CS_VREDRAW,
    wcWindowProcedure = proc,
    wcClassExtra = 0,
    wcWindowExtra = 0,
    wcInstance = instance_handle,
    wcIcon = icon,
    wcCursor = cursor,
    wcBackground = c_GetStockObject BLACK_BRUSH,
    wcMenuName = nullPtr,
    wcClassName = class_name,
    wcIconSmall = icon
  }
  putStrLn $ show CS_HREDRAW
  putStrLn $ show (0xdeaff001, 0x12345678)
  putStrLn $ show $ wcStyle wndclass
  -- interact (unlines . filter (elem 'a') . lines)