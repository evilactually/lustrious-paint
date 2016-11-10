//===============================================================================
// @ LsVector3.cpp
// 
// 3D vector class
// ------------------------------------------------------------------------------
// Copyright (C) 2004 by Elsevier, Inc. All rights reserved.
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <LsVector3.h>
#include <LsMath.h>

//-------------------------------------------------------------------------------
//-- Static Members -------------------------------------------------------------
//-------------------------------------------------------------------------------

LsVector3 LsVector3::xAxis( 1.0f, 0.0f, 0.0f );
LsVector3 LsVector3::yAxis( 0.0f, 1.0f, 0.0f );
LsVector3 LsVector3::zAxis( 0.0f, 0.0f, 1.0f );
LsVector3 LsVector3::origin( 0.0f, 0.0f, 0.0f );

//-------------------------------------------------------------------------------
//-- Methods --------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// @ LsVector3::LsVector3()
//-------------------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------------------
LsVector3::LsVector3(const LsVector3& other) : 
    x( other.x ),
    y( other.y ),
    z( other.z )
{

}   // End of LsVector3::LsVector3()


//-------------------------------------------------------------------------------
// @ LsVector3::operator=()
//-------------------------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------------------
LsVector3&
LsVector3::operator=(const LsVector3& other)
{
    // if same object
    if ( this == &other )
        return *this;
        
    x = other.x;
    y = other.y;
    z = other.z;
    
    return *this;

}   // End of LsVector3::operator=()


//-------------------------------------------------------------------------------
// @ operator<<()
//-------------------------------------------------------------------------------
// Text output for debugging
//-------------------------------------------------------------------------------
// IvWriter& 
// operator<<(IvWriter& out, const LsVector3& source)
// {
//     out << '<' << source.x << ',' << source.y << ',' << source.z << '>';
//
//     return out;
//    
// }   // End of operator<<()
    

//-------------------------------------------------------------------------------
// @ LsVector3::Length()
//-------------------------------------------------------------------------------
// Vector length
//-------------------------------------------------------------------------------
float 
LsVector3::Length() const
{
    return IvSqrt( x*x + y*y + z*z );

}   // End of LsVector3::Length()


//-------------------------------------------------------------------------------
// @ LsVector3::LengthSquared()
//-------------------------------------------------------------------------------
// Vector length squared (avoids square root)
//-------------------------------------------------------------------------------
float 
LsVector3::LengthSquared() const
{
    return (x*x + y*y + z*z);

}   // End of LsVector3::LengthSquared()


//-------------------------------------------------------------------------------
// @ ::Distance()
//-------------------------------------------------------------------------------
// Point distance
//-------------------------------------------------------------------------------
float 
Distance( const LsVector3& p0, const LsVector3& p1 )
{
    float x = p0.x - p1.x;
    float y = p0.y - p1.y;
    float z = p0.z - p1.z;

    return ::IvSqrt( x*x + y*y + z*z );

}   // End of LsVector3::Length()


//-------------------------------------------------------------------------------
// @ ::DistanceSquared()
//-------------------------------------------------------------------------------
// Point distance
//-------------------------------------------------------------------------------
float 
DistanceSquared( const LsVector3& p0, const LsVector3& p1 )
{
    float x = p0.x - p1.x;
    float y = p0.y - p1.y;
    float z = p0.z - p1.z;

    return ( x*x + y*y + z*z );

}   // End of ::DistanceSquared()


//-------------------------------------------------------------------------------
// @ LsVector3::operator==()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
LsVector3::operator==( const LsVector3& other ) const
{
    if ( ::IvAreEqual( other.x, x )
        && ::IvAreEqual( other.y, y )
        && ::IvAreEqual( other.z, z ) )
        return true;

    return false;   
}   // End of LsVector3::operator==()


//-------------------------------------------------------------------------------
// @ LsVector3::operator!=()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
LsVector3::operator!=( const LsVector3& other ) const
{
    if ( ::IvAreEqual( other.x, x )
        && ::IvAreEqual( other.y, y )
        && ::IvAreEqual( other.z, z ) )
        return false;

    return true;
}   // End of LsVector3::operator!=()


//-------------------------------------------------------------------------------
// @ LsVector3::IsZero()
//-------------------------------------------------------------------------------
// Check for zero vector
//-------------------------------------------------------------------------------
bool 
LsVector3::IsZero() const
{
    return ::IsZero(x*x + y*y + z*z);

}   // End of LsVector3::IsZero()


//-------------------------------------------------------------------------------
// @ LsVector3::IsUnit()
//-------------------------------------------------------------------------------
// Check for unit vector
//-------------------------------------------------------------------------------
bool 
LsVector3::IsUnit() const
{
    return ::IsZero(1.0f - x*x - y*y - z*z);

}   // End of LsVector3::IsUnit()


//-------------------------------------------------------------------------------
// @ LsVector3::Clean()
//-------------------------------------------------------------------------------
// Set elements close to zero equal to zero
//-------------------------------------------------------------------------------
void
LsVector3::Clean()
{
    if ( ::IsZero( x ) )
        x = 0.0f;
    if ( ::IsZero( y ) )
        y = 0.0f;
    if ( ::IsZero( z ) )
        z = 0.0f;

}   // End of LsVector3::Clean()


//-------------------------------------------------------------------------------
// @ LsVector3::Normalize()
//-------------------------------------------------------------------------------
// Set to unit vector
//-------------------------------------------------------------------------------
void
LsVector3::Normalize()
{
    float lengthsq = x*x + y*y + z*z;

    if ( ::IsZero( lengthsq ) )
    {
        Zero();
    }
    else
    {
        float factor = IvInvSqrt( lengthsq );
        x *= factor;
        y *= factor;
        z *= factor;
    }

}   // End of LsVector3::Normalize()


//-------------------------------------------------------------------------------
// @ LsVector3::operator+()
//-------------------------------------------------------------------------------
// Add vector to self and return
//-------------------------------------------------------------------------------
LsVector3 
LsVector3::operator+( const LsVector3& other ) const
{
    return LsVector3( x + other.x, y + other.y, z + other.z );

}   // End of LsVector3::operator+()


//-------------------------------------------------------------------------------
// @ LsVector3::operator+=()
//-------------------------------------------------------------------------------
// Add vector to self, store in self
//-------------------------------------------------------------------------------
LsVector3& 
operator+=( LsVector3& self, const LsVector3& other )
{
    self.x += other.x;
    self.y += other.y;
    self.z += other.z;

    return self;

}   // End of LsVector3::operator+=()


//-------------------------------------------------------------------------------
// @ LsVector3::operator-()
//-------------------------------------------------------------------------------
// Subtract vector from self and return
//-------------------------------------------------------------------------------
LsVector3 
LsVector3::operator-( const LsVector3& other ) const
{
    return LsVector3( x - other.x, y - other.y, z - other.z );

}   // End of LsVector3::operator-()


//-------------------------------------------------------------------------------
// @ LsVector3::operator-=()
//-------------------------------------------------------------------------------
// Subtract vector from self, store in self
//-------------------------------------------------------------------------------
LsVector3& 
operator-=( LsVector3& self, const LsVector3& other )
{
    self.x -= other.x;
    self.y -= other.y;
    self.z -= other.z;

    return self;

}   // End of LsVector3::operator-=()

//-------------------------------------------------------------------------------
// @ LsVector3::operator-=() (unary)
//-------------------------------------------------------------------------------
// Negate self and return
//-------------------------------------------------------------------------------
LsVector3
LsVector3::operator-() const
{
    return LsVector3(-x, -y, -z);
}    // End of LsVector3::operator-()


//-------------------------------------------------------------------------------
// @ operator*()
//-------------------------------------------------------------------------------
// Scalar multiplication
//-------------------------------------------------------------------------------
LsVector3   
LsVector3::operator*( float scalar )
{
    return LsVector3( scalar*x, scalar*y, scalar*z );

}   // End of operator*()


//-------------------------------------------------------------------------------
// @ operator*()
//-------------------------------------------------------------------------------
// Scalar multiplication
//-------------------------------------------------------------------------------
LsVector3   
operator*( float scalar, const LsVector3& vector )
{
    return LsVector3( scalar*vector.x, scalar*vector.y, scalar*vector.z );

}   // End of operator*()


//-------------------------------------------------------------------------------
// @ LsVector3::operator*()
//-------------------------------------------------------------------------------
// Scalar multiplication by self
//-------------------------------------------------------------------------------
LsVector3&
LsVector3::operator*=( float scalar )
{
    x *= scalar;
    y *= scalar;
    z *= scalar;

    return *this;

}   // End of LsVector3::operator*=()


//-------------------------------------------------------------------------------
// @ operator/()
//-------------------------------------------------------------------------------
// Scalar division
//-------------------------------------------------------------------------------
LsVector3   
LsVector3::operator/( float scalar )
{
    return LsVector3( x/scalar, y/scalar, z/scalar );

}   // End of operator/()


//-------------------------------------------------------------------------------
// @ LsVector3::operator/=()
//-------------------------------------------------------------------------------
// Scalar division by self
//-------------------------------------------------------------------------------
LsVector3&
LsVector3::operator/=( float scalar )
{
    x /= scalar;
    y /= scalar;
    z /= scalar;

    return *this;

}   // End of LsVector3::operator/=()


//-------------------------------------------------------------------------------
// @ LsVector3::Dot()
//-------------------------------------------------------------------------------
// Dot product by self
//-------------------------------------------------------------------------------
float               
LsVector3::Dot( const LsVector3& vector ) const
{
    return (x*vector.x + y*vector.y + z*vector.z);

}   // End of LsVector3::Dot()


//-------------------------------------------------------------------------------
// @ Dot()
//-------------------------------------------------------------------------------
// Dot product friend operator
//-------------------------------------------------------------------------------
float               
Dot( const LsVector3& vector1, const LsVector3& vector2 )
{
    return (vector1.x*vector2.x + vector1.y*vector2.y + vector1.z*vector2.z);

}   // End of Dot()


//-------------------------------------------------------------------------------
// @ LsVector3::Cross()
//-------------------------------------------------------------------------------
// Cross product by self
//-------------------------------------------------------------------------------
LsVector3   
LsVector3::Cross( const LsVector3& vector ) const
{
    return LsVector3( y*vector.z - z*vector.y,
                      z*vector.x - x*vector.z,
                      x*vector.y - y*vector.x );

}   // End of LsVector3::Cross()


//-------------------------------------------------------------------------------
// @ Cross()
//-------------------------------------------------------------------------------
// Cross product friend operator
//-------------------------------------------------------------------------------
LsVector3               
Cross( const LsVector3& vector1, const LsVector3& vector2 )
{
    return LsVector3( vector1.y*vector2.z - vector1.z*vector2.y,
                      vector1.z*vector2.x - vector1.x*vector2.z,
                      vector1.x*vector2.y - vector1.y*vector2.x );

}   // End of Cross()


