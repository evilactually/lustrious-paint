
module Main where 

import Windows
import Data.ByteString
import Data.String
import Foreign.Ptr

--p = intPtrToPtr (fromInteger 42 :: IntPtr)
--main = print p

main = print =<< (c_GetModuleHandleA nullPtr)

--main = do
-- p <- (c_GetModuleHandleA nullPtr)
-- print p