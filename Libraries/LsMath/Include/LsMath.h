//===============================================================================
// @ LsMath.h
// 
// Base math macros and functions
// ------------------------------------------------------------------------------
// Copyright (C) 2004 by Elsevier, Inc. All rights reserved.
//
//===============================================================================

#pragma once

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//-- Typedefs, Structs ----------------------------------------------------------
//-------------------------------------------------------------------------------

#define kEpsilon    1.0e-6f

#define kPI         3.1415926535897932384626433832795f
#define kHalfPI     1.5707963267948966192313216916398f
#define kTwoPI      2.0f*kPI

#ifdef PLATFORM_OSX
#define sqrtf sqrt
#define sinf sin
#define cosf cos
#define fabsf fabs
#define tanf tan
#define atan2f atan2
#define acosf acos
#endif

inline float IvSqrt( float val );
inline float IvInvSqrt( float val );
inline float IvAbs( float f );

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------
extern void IvFastSinCos( float a, float& sina, float& cosa );

//-------------------------------------------------------------------------------
//-- Inlines --------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// @ IsZero()
//-------------------------------------------------------------------------------
// Is this floating point value close to zero?
//-------------------------------------------------------------------------------
inline bool IsZero( float a );

//-------------------------------------------------------------------------------
// @ IvAreEqual()
//-------------------------------------------------------------------------------
// Are these floating point values close to equal?
//-------------------------------------------------------------------------------
inline bool IvAreEqual( float a, float b );

//-------------------------------------------------------------------------------
// @ LsAreEqualRelative()
//-------------------------------------------------------------------------------
// Same as IvAreEqual, but using more advanced method.
//-------------------------------------------------------------------------------
inline bool LsAreEqualRelative(float a, float b);

//-------------------------------------------------------------------------------
// @ IvSin()
//-------------------------------------------------------------------------------
// Returns the floating-point sine of the argument
//-------------------------------------------------------------------------------
inline float IvSin( float a );

//-------------------------------------------------------------------------------
// @ IvCos()
//-------------------------------------------------------------------------------
// Returns the floating-point cosine of the argument
//-------------------------------------------------------------------------------
inline float IvCos( float a );

//-------------------------------------------------------------------------------
// @ IvTan()
//-------------------------------------------------------------------------------
// Returns the floating-point tangent of the argument
//-------------------------------------------------------------------------------
inline float IvTan( float a );

//-------------------------------------------------------------------------------
// @ IvSinCos()
//-------------------------------------------------------------------------------
// Returns the floating-point sine and cosine of the argument
//-------------------------------------------------------------------------------
inline void IvSinCos( float a, float& sina, float& cosa );

inline bool LsOdd(int x);

inline bool LsEven(int x);

//-------------------------------------------------------------------------------
// @ LsOddCount()
//-------------------------------------------------------------------------------
// Returns a count of odd numbers on the closed interval [min, max]
//-------------------------------------------------------------------------------
inline int LsOddCount(int min, int max);

//-------------------------------------------------------------------------------
// @ LsEvenCount()
//-------------------------------------------------------------------------------
// Returns a count of even numbers on the closed interval [min, max]
//-------------------------------------------------------------------------------
inline int LsEvenCount(int min, int max);

//-------------------------------------------------------------------------------
//-- Externs --------------------------------------------------------------------
//-------------------------------------------------------------------------------
