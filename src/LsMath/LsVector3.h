//===============================================================================
// @ LsVector3.h
// 
// Description
// ------------------------------------------------------------------------------
// Copyright (C) 2004 by Elsevier, Inc. All rights reserved.
//
//
//
//===============================================================================

#pragma once

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

//#include "IvWriter.h"

//-------------------------------------------------------------------------------
//-- Typedefs, Structs ----------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//-- Classes --------------------------------------------------------------------
//-------------------------------------------------------------------------------

class LsVector3
{
public:
    // constructor/destructor
    inline LsVector3() {}
    inline LsVector3( float _x, float _y, float _z ) :
        x(_x), y(_y), z(_z)
    {
    }
    inline ~LsVector3() {}

    // copy operations
    LsVector3(const LsVector3& other);
    LsVector3& operator=(const LsVector3& other);

    // text output (for debugging)
    //friend IvWriter& operator<<(IvWriter& out, const LsVector3& source);

    // accessors
    inline float& operator[]( unsigned int i ) { return (&x)[i]; }
    inline float operator[]( unsigned int i ) const { return (&x)[i]; }

    float Length() const;
    float LengthSquared() const;

    friend float Distance( const LsVector3& p0, const LsVector3& p1 );
    friend float DistanceSquared( const LsVector3& p0, const LsVector3& p1 );

    // comparison
    bool operator==( const LsVector3& other ) const;
    bool operator!=( const LsVector3& other ) const;
    bool IsZero() const;
    bool IsUnit() const;

    // manipulators
    inline void Set( float _x, float _y, float _z );
    void Clean();       // sets near-zero elements to 0
    inline void Zero(); // sets all elements to 0
    void Normalize();   // sets to unit vector

    // operators

    // addition/subtraction
    LsVector3 operator+( const LsVector3& other ) const;
    friend LsVector3& operator+=( LsVector3& vector, const LsVector3& other );
    LsVector3 operator-( const LsVector3& other ) const;
    friend LsVector3& operator-=( LsVector3& vector, const LsVector3& other );

    LsVector3 operator-() const;

    // scalar multiplication
    LsVector3   operator*( float scalar );
    friend LsVector3    operator*( float scalar, const LsVector3& vector );
    LsVector3&          operator*=( float scalar );
    LsVector3   operator/( float scalar );
    LsVector3&          operator/=( float scalar );

    // dot product/cross product
    float               Dot( const LsVector3& vector ) const;
    friend float        Dot( const LsVector3& vector1, const LsVector3& vector2 );
    LsVector3           Cross( const LsVector3& vector ) const;
    friend LsVector3    Cross( const LsVector3& vector1, const LsVector3& vector2 );

    // useful defaults
    static LsVector3    xAxis;
    static LsVector3    yAxis;
    static LsVector3    zAxis;
    static LsVector3    origin;

    // member variables
    float x, y, z;

protected:

private:
};

//-------------------------------------------------------------------------------
//-- Inlines --------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// @ LsVector3::Set()
//-------------------------------------------------------------------------------
// Set vector elements
//-------------------------------------------------------------------------------
inline void 
LsVector3::Set( float _x, float _y, float _z )
{
    x = _x; y = _y; z = _z;
}   // End of LsVector3::Set()

//-------------------------------------------------------------------------------
// @ LsVector3::Zero()
//-------------------------------------------------------------------------------
// Zero all elements
//-------------------------------------------------------------------------------
inline void 
LsVector3::Zero()
{
    x = y = z = 0.0f;
}   // End of LsVector3::Zero()

//-------------------------------------------------------------------------------
//-- Externs --------------------------------------------------------------------
//-------------------------------------------------------------------------------
