{-# LANGUAGE ForeignFunctionInterface #-}
{-# LANGUAGE PatternSynonyms #-}
-- {-# LANGUAGE GeneralizedNewtypeDeriving #-}

module Main where

import Data.Char(toUpper)
import Foreign.C.Types
import Foreign.Ptr
import Data.Void
import Foreign.C.String
import Foreign.Storable
import Data.Bits((.|.))
import Data.Int(Int32)

type HANDLE = Ptr Void

type HINSTANCE = HANDLE

type HMODULE = HANDLE

type HICON = HANDLE

type HCURSOR = HICON

type HBRUSH = HANDLE

type LPCTSTR = CString

type HGDIOBJ = HANDLE

data WNDCLASSEX = WNDCLASSEX { cbSize :: CUInt,
                               style :: CUInt,
                               wndProc :: FunPtr (Ptr Void), 
                               cbClsExtra :: CInt,
                               cbWndExtra :: CInt,
                               hInstance :: HINSTANCE,
                               hIcon :: HICON,
                               hCursor :: HCURSOR,
                               hbrBackground :: HBRUSH,
                               lpszMenuName :: LPCTSTR,
                               lpszClassName:: LPCTSTR,
                               hIconSm :: HICON }

type PWNDCLASSEX = Ptr WNDCLASSEX

instance Storable WNDCLASSEX where
        alignment _ = 4
        sizeOf _    = 48
        peek ptr    = WNDCLASSEX
            <$> peekByteOff ptr 0  -- cbSize :: CUInt
            <*> peekByteOff ptr 4  -- style :: CUInt
            <*> peekByteOff ptr 8  -- wndProc :: FunPtr (Ptr Void)
            <*> peekByteOff ptr 12 -- cbClsExtra :: CInt 
            <*> peekByteOff ptr 16 -- cbWndExtra :: CInt
            <*> peekByteOff ptr 20 -- hInstance :: CInt
            <*> peekByteOff ptr 24 -- hIcon :: CInt
            <*> peekByteOff ptr 28 -- hCursor :: CInt
            <*> peekByteOff ptr 32 -- hbrBackground :: CInt
            <*> peekByteOff ptr 36 -- lpszMenuName :: CInt
            <*> peekByteOff ptr 40 -- lpszClassName :: CInt
            <*> peekByteOff ptr 44 -- hIconSm :: CInt
        poke ptr poked = do
            pokeByteOff ptr 0  (cbSize poked)
            pokeByteOff ptr 4  (style poked)
            pokeByteOff ptr 8  (wndProc poked)
            pokeByteOff ptr 12 (cbClsExtra poked)
            pokeByteOff ptr 16 (cbWndExtra poked)
            pokeByteOff ptr 20 (hInstance poked)
            pokeByteOff ptr 24 (hIcon poked)
            pokeByteOff ptr 28 (hCursor poked)
            pokeByteOff ptr 32 (hbrBackground poked)
            pokeByteOff ptr 36 (lpszMenuName poked)
            pokeByteOff ptr 40 (lpszClassName poked)
            pokeByteOff ptr 44 (hIconSm poked)


-- Class styles
pattern CS_HREDRAW = 0x0002
pattern CS_VREDRAW = 0x0001

-- Icons
pattern IDI_APPLICATION = 32512

-- Cursors
pattern IDC_ARROW = 32512

-- GDI Stock Objects
pattern BLACK_BRUSH = 4

--type Compare = Int -> Int -> Bool
--foreign import ccall "wrapper"
--  mkCompare :: Compare -> IO (FunPtr Compare)

foreign import stdcall "LoadIconA"
  c_LoadIconByName :: HINSTANCE -> LPCTSTR -> IO(HICON)

foreign import stdcall "LoadIconA"
  c_LoadIconById :: HINSTANCE -> CInt -> IO(HICON)

foreign import stdcall "LoadCursorA"
  c_LoadCursorByName :: HINSTANCE -> LPCTSTR -> IO(HCURSOR)

foreign import stdcall "LoadCursorA"
  c_LoadCursorById :: HINSTANCE -> CInt -> IO(HCURSOR)

foreign import stdcall "GetModuleHandleA"
  c_GetModuleHandleA :: CString -> IO(HMODULE)

foreign import stdcall "GetStockObject"
  c_GetStockObject :: CInt -> HGDIOBJ

main = do
    instance_handle <- c_GetModuleHandleA nullPtr
    icon <- c_LoadIconById instance_handle IDI_APPLICATION
    cursor <- c_LoadCursorById instance_handle IDC_ARROW
    class_name <- newCString "WindowClass"
    let wndclass = WNDCLASSEX {
        cbSize = fromIntegral $ sizeOf wndclass,
        style = CS_HREDRAW .|. CS_VREDRAW,
        wndProc = nullFunPtr, --                              <-- WndProc goes here
        cbClsExtra = 0,
        cbWndExtra = 0,
        hInstance = instance_handle,
        hIcon = icon,
        hCursor = cursor,
        hbrBackground = c_GetStockObject BLACK_BRUSH,
        lpszMenuName = nullPtr,
        lpszClassName = class_name,
        hIconSm = icon
    }
    putStrLn $ show CS_HREDRAW
    -- interact (unlines . filter (elem 'a') . lines)