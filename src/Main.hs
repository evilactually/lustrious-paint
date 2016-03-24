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
import Foreign.Marshal.Alloc(alloca)

type HANDLE = Ptr Void

type HINSTANCE = HANDLE

type HMODULE = HANDLE

type HWND = HANDLE

type HICON = HANDLE

type HCURSOR = HICON

type HBRUSH = HANDLE

type LPCTSTR = CString

type HGDIOBJ = HANDLE

type LONG_PTR = LONG

type LRESULT = LONG_PTR

type LPARAM = LONG_PTR

type WPARAM = LONG_PTR

type LONG = CLong

type INT = CInt

type UINT = CUInt

type BOOL = CInt

data WNDCLASSEX = WNDCLASSEX { wcSize :: UINT,
                               wcStyle :: ClassStyle,
                               wcWindowProcedure :: FunPtr (WindowProcedure), 
                               wcClassExtra :: INT,
                               wcWindowExtra :: INT,
                               wcInstance :: HINSTANCE,
                               wcIcon :: HICON,
                               wcCursor :: HCURSOR,
                               wcBackground :: HBRUSH,
                               wcMenuName :: LPCTSTR,
                               wcClassName:: LPCTSTR,
                               wcIconSmall :: HICON }

type PWNDCLASSEX = Ptr WNDCLASSEX

#if ARCH == ARCH_x86
instance Storable WNDCLASSEX where
        alignment _ = 4
        sizeOf _    = 48
        peek ptr    = WNDCLASSEX
            <$> peekByteOff ptr 0  -- wcSize :: UINT
            <*> peekByteOff ptr 4  -- wcStyle :: ClassStyle
            <*> peekByteOff ptr 8  -- wcWindowProcedure :: FunPtr (Ptr WindowProcedure)
            <*> peekByteOff ptr 12 -- wcClassExtra :: INT
            <*> peekByteOff ptr 16 -- wcWindowExtra :: INT
            <*> peekByteOff ptr 20 -- wcInstance :: HINSTANCE
            <*> peekByteOff ptr 24 -- wcIcon :: HCURSOR
            <*> peekByteOff ptr 28 -- wcCursor :: HCURSOR
            <*> peekByteOff ptr 32 -- wcBackground :: HBRUSH
            <*> peekByteOff ptr 36 -- wcMenuName :: LPCTSTR
            <*> peekByteOff ptr 40 -- wcClassName :: LPCTSTR
            <*> peekByteOff ptr 44 -- wcIconSmall :: HICON
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
#endif

data Rectangle = Rectangle { reLeft   :: LONG,
                             reTop    :: LONG,
                             reRight  :: LONG,
                             reBottom :: LONG }

#if ARCH == ARCH_x86
instance Storable Rectangle where
  alignment _ = 4
  sizeOf _    = 16
  peek ptr    = Rectangle
    <$> peekByteOff ptr 0   -- reLeft   :: LONG
    <*> peekByteOff ptr 4   -- reTop    :: LONG
    <*> peekByteOff ptr 8   -- reRight  :: LONG
    <*> peekByteOff ptr 12  -- reBottom :: LONG
  poke ptr poked = do
    pokeByteOff ptr 0  (reLeft poked)
    pokeByteOff ptr 4  (reTop poked)
    pokeByteOff ptr 8  (reRight poked)
    pokeByteOff ptr 12 (reBottom poked)
#endif

newtype ClassStyle = ClassStyle UINT
  deriving (Eq, Storable, Bits, Show)

pattern CS_HREDRAW = ClassStyle 0x0002
pattern CS_VREDRAW = ClassStyle 0x0001

newtype IconId = IconId INT
  deriving (Eq, Storable, Show)

pattern IDI_APPLICATION = IconId 32512

newtype CursorId = CursorId INT
  deriving (Eq, Storable, Show)

pattern IDC_ARROW = CursorId 32512

-- GDI Stock Objects
pattern BLACK_BRUSH = 4

newtype WindowMessage = WindowMessage UINT
  deriving (Eq, Storable, Show)

pattern WM_NCHITTEST = WindowMessage 0x0084
pattern WM_NCCALCSIZE = WindowMessage 0x0083
pattern WM_DESTROY = WindowMessage 0x0002

class ReturnValue v where
    toLResult :: v -> LRESULT

newtype HitTestResult = HitTestResult LONG

pattern HTBORDER      = HitTestResult 18
pattern HTBOTTOM      = HitTestResult 15
pattern HTBOTTOMLEFT  = HitTestResult 16
pattern HTBOTTOMRIGHT = HitTestResult 17
pattern HTCAPTION     = HitTestResult 2
pattern HTCLIENT      = HitTestResult 1
pattern HTCLOSE       = HitTestResult 20
pattern HTERROR       = HitTestResult (-2)
pattern HTGROWBOX     = HitTestResult 4
pattern HTHELP        = HitTestResult 21
pattern HTHSCROLL     = HitTestResult 6
pattern HTLEFT        = HitTestResult 10
pattern HTMENU        = HitTestResult 5
pattern HTMAXBUTTON   = HitTestResult 9
pattern HTMINBUTTON   = HitTestResult 8
pattern HTNOWHERE     = HitTestResult 0
pattern HTREDUCE      = HitTestResult 8
pattern HTRIGHT       = HitTestResult 11
pattern HTSIZE        = HitTestResult 4
pattern HTSYSMENU     = HitTestResult 3
pattern HTTOP         = HitTestResult 12
pattern HTTOPLEFT     = HitTestResult 13
pattern HTTOPRIGHT    = HitTestResult 14
pattern HTTRANSPARENT = HitTestResult (-1)
pattern HTVSCROLL     = HitTestResult 7
pattern HTZOOM        = HitTestResult 9

instance ReturnValue HitTestResult where
  toLResult (HitTestResult v) = v

-- type Compare = Int -> Int -> Bool
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
  c_GetStockObject :: INT -> HGDIOBJ

foreign import stdcall "DefWindowProcA"
  c_DefWindowProcA :: WindowProcedure

foreign import stdcall "GetWindowRect"
 c_GetWindowRect :: HWND -> Ptr(Rectangle) -> IO(BOOL)

getWindowRectangle :: HWND -> IO(Maybe(Rectangle))
getWindowRectangle hwnd = alloca $ \ptr -> do 
  success <- c_GetWindowRect hwnd ptr
  case success of
    0 -> return Nothing
    _ -> Just <$> peek ptr

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

getXLParam :: LPARAM -> LONG
getXLParam p = fromIntegral x
  where
    (_,x) = split $ fromIntegral p
#if ARCH == ARCH_x86
    split = splitWord32
#else
    split = splitWord64
#endif

getYLParam :: LPARAM -> LONG
getYLParam p = fromIntegral y
  where
    (y,_) = split $ fromIntegral p
#if ARCH == ARCH_x86
    split = splitWord32
#else
    split = splitWord64
#endif

type WindowProcedure = HWND -> WindowMessage -> WPARAM -> LPARAM -> IO(LRESULT)

windowProcedure :: WindowProcedure
windowProcedure hwnd WM_NCHITTEST wparam lparam = return $ toLResult HTCLIENT
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