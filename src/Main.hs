{-# LANGUAGE ForeignFunctionInterface #-}

module Main where

--main :: IO ()
--main = do
--  putStrLn "hello world..."

import Data.Char(toUpper)
import Foreign.C.Types
import Foreign.Ptr
import Data.Void
import Foreign.C.String

-- WNDCLASSEX
-- main = interact (map toUpper . (++) "Your data, in uppercase, is:\n\n")

data AAA = FunPtr Int

type HANDLE = Ptr Void

type HINSTANCE = HANDLE

type HICON = HANDLE

type HCURSOR = HICON

type HBRUSH = HANDLE

type LPCTSTR = CString

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

main = interact (unlines . filter (elem 'a') . lines)