#pragma once

#include "vulkan/vk_cpp.hpp"
#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace ls {
    extern bool dialogShowing;
    extern HWND windowHandle;
    void Abort(std::string& msg) {
        dialogShowing = true;
        MessageBox(windowHandle,
                   msg.c_str(),
                   "Error",
                   MB_OK | MB_ICONERROR);
		throw 1;
    }

    void Abort(const char* msg) {
        Abort(std::string(msg));
    }

    void Abort() {
        //exit(1);
		throw 1;
    }

    void Error() {
        Abort(std::string("Error occurred, view log for details."));   
    }

    void ExitFatal(std::string message, std::string caption) 
    {
        MessageBox(NULL, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
		throw 1;
    }

    std::vector<char> GetBinaryFileContents( std::string const &filename ) {
        std::ifstream file( filename, std::ios::binary );
        if( file.fail() ) {
                std::cout << "Could not open \"" << filename << "\" file!" << std::endl;
                return std::vector<char>();
        }

        std::streampos begin, end;
        begin = file.tellg();
        file.seekg( 0, std::ios::end );
        end = file.tellg();

        std::vector<char> result( static_cast<size_t>(end - begin) );
        file.seekg( 0, std::ios::beg );
        file.read( &result[0], end - begin );
        file.close();

        return result;
    }

    inline __m128 _mm_floor_ps2(const __m128& x) {
        __m128i v0 = _mm_setzero_si128();                                          // zeroes
        __m128i v1 = _mm_cmpeq_epi32(v0, v0);                                      // ones
        __m128i ji = _mm_srli_epi32(v1, 25);                                       // shift packed 32-bit integers by 25 each, bringing in zeroes
        __m128 j = *(__m128*)&_mm_slli_epi32(ji, 23); //create vector 1.0f         // left shift by 23 to left, bring zeroes (j == 1.0f is not used below this line) xmm3
        __m128i i = _mm_cvttps_epi32(x);                                           // truncate source floats into integers
        __m128 fi = _mm_cvtepi32_ps(i);                                            // convert truncated integers back to floats
        __m128 igx = _mm_cmpgt_ps(fi, x);                                          // if fi > x then 0xffffffff else 0 (fi > x if x was a negative number)
        j = _mm_and_ps(igx, j);                                                    // mask 1.0f values stored in j if NOT (fi > x); negative x will unmask 1.0f
        return _mm_sub_ps(fi, j);                                                  // 1.0f is subtructed from fi; fi was truncated version of x(but we need a floor)
    }

#ifdef _DEBUG
#include <stdlib.h>
#include <stdarg.h>
#endif

    static void tracef(LPCTSTR lpszFormat, ...)
    {
#ifdef _DEBUG
        static const int BUFFERSIZE = 0x800;
        TCHAR    lpszBuffer[BUFFERSIZE];
        va_list  fmtList;

        va_start(fmtList, lpszFormat);
        vsprintf_s(lpszBuffer, lpszFormat, fmtList);
        va_end(fmtList);

        ::OutputDebugString(lpszBuffer);
#endif
    }
}