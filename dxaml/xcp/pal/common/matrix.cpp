// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MinMath.h"
#include <CValue.h>
#include <algorithm>

#undef min
#undef max

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Set to empty
//
//-------------------------------------------------------------------------
void
CMILMatrix::SetToEmpty()
{
    this->_11 = 0.0f;
    this->_12 = 0.0f;
    this->_21 = 0.0f;
    this->_22 = 0.0f;
    this->_31 = 0.0f;
    this->_32 = 0.0f;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Set to identity
//
//-------------------------------------------------------------------------
void
CMILMatrix::SetToIdentity()
{
    this->_11 = 1.0f;
    this->_12 = 0.0f;
    this->_21 = 0.0f;
    this->_22 = 1.0f;
    this->_31 = 0.0f;
    this->_32 = 0.0f;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Set to rotation
//
//-------------------------------------------------------------------------
void
CMILMatrix::SetToRotation(XFLOAT rAngle)
{
    XFLOAT rCos = MathCosDegrees(rAngle);
    XFLOAT rSin = MathSinDegrees(rAngle);

    _11 = rCos;
    _12 = -rSin;

    _21 = rSin;
    _22 = rCos;

    _31 = 0.0f;
    _32 = 0.0f;
}

// Set the overall matrix to a rotation matrix with given angle, about given center point
void CMILMatrix::SetToRotationAboutCenter(XFLOAT angle, XFLOAT centerX, XFLOAT centerY)
{
    XFLOAT cosAngle = MathCosDegrees(angle);
    XFLOAT sinAngle = MathSinDegrees(angle);

    _11 = cosAngle;
    _12 = -sinAngle;

    _21 = sinAngle;
    _22 = cosAngle;

    _31 = -centerX * cosAngle - centerY * sinAngle + centerX;
    _32 = sinAngle * centerX - cosAngle * centerY + centerY;
}

// Set the overall matrix to a scale matrix with given scale factor, about given center point
void CMILMatrix::SetToScaleAboutCenter(XFLOAT scaleX, XFLOAT scaleY, XFLOAT centerX, XFLOAT centerY)
{
    _11 = scaleX;
    _12 = 0.0f;

    _21 = 0.0f;
    _22 = scaleY;

    _31 = -centerX * scaleX + centerX;
    _32 = -centerY * scaleY + centerY;
}

//-------------------------------------------------------------------------
//
//  Method: Append
//
//  Synopsis:
//      Replaces the current matrix with the product of itself and the incoming
//  matrix.
//
//-------------------------------------------------------------------------

void
CMILMatrix::Append(_In_ const CMILMatrix& that)
{
    CMILMatrix temp;

    temp = *this;

    this->_11 = temp._11 * that._11 + temp._12 * that._21;
    this->_12 = temp._11 * that._12 + temp._12 * that._22;
    this->_21 = temp._21 * that._11 + temp._22 * that._21;
    this->_22 = temp._21 * that._12 + temp._22 * that._22;
    this->_31 = temp._31 * that._11 + temp._32 * that._21 + that._31;
    this->_32 = temp._31 * that._12 + temp._32 * that._22 + that._32;
}

//-------------------------------------------------------------------------
//
//  Method: Prepend
//
//  Synopsis:
//      Replaces the current matrix with the product of itself and the incoming
//  matrix.
//
//-------------------------------------------------------------------------

void
CMILMatrix::Prepend(_In_ const CMILMatrix& that)
{
    CMILMatrix temp;

    temp = *this;

    this->_11 = that._11 * temp._11 + that._12 * temp._21;
    this->_12 = that._11 * temp._12 + that._12 * temp._22;
    this->_21 = that._21 * temp._11 + that._22 * temp._21;
    this->_22 = that._21 * temp._12 + that._22 * temp._22;
    this->_31 = that._31 * temp._11 + that._32 * temp._21 + temp._31;
    this->_32 = that._31 * temp._12 + that._32 * temp._22 + temp._32;
}

void CMILMatrix::Transform3DPoints_PreserveW(const gsl::span<XPOINTF4>& points) const
{
    //            [ _11 _12  0   0 ]
    // [ x y z w ][ _21 _22  0   0 ] = [ x_11+y_21+w_31  x_12+y_22+w_32  z  w ]
    //            [  0   0   1   0 ]
    //            [ _31 _32  0   1 ]
    //
    for (XPOINTF4& point : points)
    {
        float x = point.x;
        float y = point.y;
        float w = point.w;

        point.x = (_11 * x) + (_21 * y) + (_31 * w);
        point.y = (_12 * x) + (_22 * y) + (_32 * w);
        // z and w are unchanged
    }
}

//-------------------------------------------------------------------------
//
//  Member:
//      CMILMatrix::TransformAsVectors
//
//  Synopsis:
//      Transform the specified array of vectors using the current matrix.
//
//  Implementation details:
//      This function assumes two things
//          1) There is always at least 1 vector
//          2) The source array DOES NOT overlap the destination array unless
//              the target address is less then or equal to the source address.
//      However, since this component can't access the PAL or the OS we can't
//      assert this behave so we have to programatically guarantee it.
//
//      The function performs the following computation
//
//      p' = p M:
//                                  ( M11 M12 0 )
//      (px', py', 0) = (px, py, 0) ( M21 M22 0 )
//                                  ( dx  dy  1 )
//
//-------------------------------------------------------------------------

void
CMILMatrix::TransformAsVectors(
    _In_reads_(count) const XPOINTF *srcPoints,    // Array of source points to transform
    _Out_writes_(count) XPOINTF *destPoints,        // Destination for transformed points.
    _In_ XUINT32 count                              // Number of points to transform
) const
{
// Validate the parameters.

    if (!count || ((srcPoints < destPoints) && (srcPoints + count > destPoints)))
        return;

// We use locals to allow in-place transforms of the array.
    XFLOAT x;
    XFLOAT y;

    do
    {
        x = srcPoints->x;
        y = srcPoints->y;

        destPoints->x = (_11 * x) + (_21 * y);
        destPoints->y = (_12 * x) + (_22 * y);
    }
    while (destPoints++, srcPoints++, --count != 0);
}


//-------------------------------------------------------------------------
//
//  Member:
//      CMILMatrix::TransformBounds
//
//  Synopsis:
//      Transform the specified Rect using the current matrix.
//
//  Implementation details:
//      The function performs the following computation
//
//      p' = p M:
//                                  ( M11 M12 0 )
//      (px', py', 0) = (px, py, 0) ( M21 M22 0 )
//                                  ( dx  dy  1 )
//
//      ... for all 4 points in a rectangle.
//
//      We then take a bounding box of the resulting transformed points.
//
//-------------------------------------------------------------------------

void
CMILMatrix::TransformBounds(
    _In_ const XRECTF *pSource,
    _Out_ XRECTF *pTarget) const
{
    // Get the corners
    XPOINTF ptCorners[4];
    XPOINTF ptMin;
    XPOINTF ptMax;

    ptCorners[0].x = pSource->X;
    ptCorners[0].y = pSource->Y;
    ptCorners[1].x = pSource->X + pSource->Width;
    ptCorners[1].y = pSource->Y;
    ptCorners[2].x = pSource->X;
    ptCorners[2].y = pSource->Y + pSource->Height;
    ptCorners[3].x = pSource->X + pSource->Width;
    ptCorners[3].y = pSource->Y + pSource->Height;

    // Transform all four
    Transform( ptCorners, ptCorners, 4 );

    // do a min/max to find the bounds
    ptMin.x = XFLOAT_MAX;
    ptMin.y = XFLOAT_MAX;
    ptMax.x = XFLOAT_MIN;
    ptMax.y = XFLOAT_MIN;
    for ( int i=0; i<4; i++ )
    {
        ptMin.x = std::min(ptMin.x, ptCorners[i].x);
        ptMin.y = std::min(ptMin.y, ptCorners[i].y);

        ptMax.x = std::max(ptMax.x, ptCorners[i].x);
        ptMax.y = std::max(ptMax.y, ptCorners[i].y);
    }

    // Now reconstruct new bounds
    pTarget->X = ptMin.x;
    pTarget->Y = ptMin.y;
    pTarget->Width = ptMax.x - ptMin.x;
    pTarget->Height = ptMax.y - ptMin.y;
}

//-------------------------------------------------------------------------
//
//  Member:
//      CMILMatrix::TransformBounds
//
//  Synopsis:
//      Transform the specified Rect using the current matrix.
//
//  Implementation details:
//      The function performs the following computation
//
//      p' = p M:
//                                  ( M11 M12 0 )
//      (px', py', 0) = (px, py, 0) ( M21 M22 0 )
//                                  ( dx  dy  1 )
//
//      ... for all 4 points in a rectangle.
//
//      We then take a bounding box of the resulting transformed points.
//
//-------------------------------------------------------------------------

void
CMILMatrix::TransformBounds(
    _In_ const XRECTF_RB *pSource,
    _Out_ XRECTF_RB *pTarget) const
{
    // Get the corners
    XPOINTF ptCorners[4];
    XPOINTF ptMin;
    XPOINTF ptMax;

    ptCorners[0].x = pSource->left;
    ptCorners[0].y = pSource->top;
    ptCorners[1].x = pSource->right;
    ptCorners[1].y = pSource->top;
    ptCorners[2].x = pSource->left;
    ptCorners[2].y = pSource->bottom;
    ptCorners[3].x = pSource->right;
    ptCorners[3].y = pSource->bottom;

    // Transform all four
    Transform(ptCorners, ptCorners, 4);

    // do a min/max to find the bounds
    ptMin.x = XFLOAT_MAX;
    ptMin.y = XFLOAT_MAX;
    ptMax.x = XFLOAT_MIN;
    ptMax.y = XFLOAT_MIN;
    for (int i = 0; i < 4; i++)
    {
        ptMin.x = std::min(ptMin.x, ptCorners[i].x);
        ptMin.y = std::min(ptMin.y, ptCorners[i].y);

        ptMax.x = std::max(ptMax.x, ptCorners[i].x);
        ptMax.y = std::max(ptMax.y, ptCorners[i].y);
    }

    // Now reconstruct new bounds
    pTarget->left = ptMin.x;
    pTarget->top = ptMin.y;
    pTarget->right = ptMax.x;
    pTarget->bottom = ptMax.y;
}


//+------------------------------------------------------------------------
//
//  Member:
//      CMILMatrix::Scale
//
//  Synopsis:
//      Appends a scale matrix
//
//  Notes:
//      The function performs the following computation
//
//            ( M11 M12 0 ) ( sX  0  0 )   (sX*M11  sY*M12  0)
//            ( M21 M22 0 ) ( 0   sY 0 ) = (sX*M21  sY*M22  0)
//            ( dx  dy  1 ) ( 0   0  1 )   (sX*dx   sY*dy   0)
//
//-------------------------------------------------------------------------
void
CMILMatrix::Scale(_In_ XFLOAT rScaleX, _In_ XFLOAT rScaleY)
{
    _11 *= rScaleX;
    _21 *= rScaleX;
    _31 *= rScaleX;

    _12 *= rScaleY;
    _22 *= rScaleY;
    _32 *= rScaleY;
}

//+-----------------------------------------------------------------------
//
//  Member:    CMILMatrix::GetMaxFactor
//
//  Synopsis:  Get maximum of |Transformed V| / |V| for this matrix
//
//  Return:    The maximum
//
//  Notes: When a vector (x,y) is transformed with the matrix M then the
//  square of its length is:
//                               (x,y) /M11 M12\ /M11 M21\ /x\
//                                     \M21 M22/ \M12 M22/ \y/
//  This is a quadratic form:
//                           f(x, y) = (x, y)/a b\ /x\
//                                           \b c/ \y/
//  where:
//         a = M11*M11 + M12*M12,
//         b = M11*M21 + M12*M22,
//         c = M21*M21 + M22*M22,
//
//  The min and max of its values on a unit vector are eigenvalues
//  of  the matrix /a, b\.
//                 \b, c/
//
//  Eigenvalues are the roots of the characteristic equation:
//
//               |a-x  b | = 0,
//               | b  c-x|
//
//  where | | stands for the determinant. The equation is:
//
//               (a-x)(c-x) - b^2 = 0
//  or
//               x^2 - (a+c)x + ac - b^2 = 0.
//                         _____________________                  ______________
//  The roots are (a+c +-\/(a+c)^2 - 4(ac - b^2) ) / 2 = (a+c +-\/(a-c)^2 + 4b^2 ) / 2
//
//  The maximal factor is the one with greater absolute value
//
//------------------------------------------------------------------------------
XFLOAT
CMILMatrix::GetMaxFactor() const
{
    XFLOAT r;

    XFLOAT a = _11 * _11 + _12 * _12;
    XFLOAT b = _11 * _21 + _12 * _22;
    XFLOAT c = _21 * _21 + _22 * _22;

    r = a - c;
    r = sqrtf(r * r + 4 * b * b);

    // The roots are eigenvalues of a positive semi-definite quadratic form, so they
    // are non-negative.  They are a + c +- r.  For best accuracy we want to get the
    // larger one first. Both a and c are sums of squares, so a + c > 0. For the larger
    // root we therefore need to choose +:

    // Ignore NaNs
    r = sqrtf((a + c + r) * .5f);

    return r;
}


//-------------------------------------------------------------------------
//
//  Member:
//      CMILMatrix::InferAffineMatrix
//
//  Synopsis:
//
//      Infer an affine transformation matrix from a rectangle-to-rectangle
//   mapping
//
//-------------------------------------------------------------------------
void
CMILMatrix::InferAffineMatrix(
    _In_reads_(3) const XPOINTF *pTargetPoints,
    _In_reads_(1) const XRECTF &rcSource
    )
{
    XFLOAT x0, y0, x1, y1, x2, y2;
    XFLOAT u0, v0, u1, v1, u2, v2;
    XFLOAT d;

    x0 = pTargetPoints[0].x;
    y0 = pTargetPoints[0].y;
    x1 = pTargetPoints[1].x;
    y1 = pTargetPoints[1].y;
    x2 = pTargetPoints[2].x;
    y2 = pTargetPoints[2].y;

    u0 = rcSource.X;
    v0 = rcSource.Y;
    u1 = u0 + rcSource.Width;
    v1 = v0;
    u2 = u0;
    v2 = v0 + rcSource.Height;

    d = u0*(v1-v2) - v0*(u1-u2) + (u1*v2-u2*v1);

    SetToIdentity();

    // Division by zero is okay
    d = 1.0f / d;

    XFLOAT t0, t1, t2;

    t0 = v1-v2;
    t1 = v2-v0;
    t2 = v0-v1;
    _11 = d * (x0*t0 + x1*t1 + x2*t2);
    _12 = d * (y0*t0 + y1*t1 + y2*t2);

    t0 = u2-u1;
    t1 = u0-u2;
    t2 = u1-u0;
    _21 = d * (x0*t0 + x1*t1 + x2*t2);
    _22 = d * (y0*t0 + y1*t1 + y2*t2);

    t0 = u1*v2-u2*v1;
    t1 = u2*v0-u0*v2;
    t2 = u2*v1-u1*v0;
    _31 = d * (x0*t0 + x1*t1 + x2*t2);
    _32 = d * (y0*t0 + y1*t1 + y2*t2);
}

//+-----------------------------------------------------------------------------
//
//  Member:    GetScaleDimensions
//
//  Synopsis:  Extracts the absolute scale factors, ignoring translation,
//             rotation, flipping, skewing. This is useful in prefiltering an
//             image for display. (The reconstruction filter will handle
//             translation, rotation, flipping, and skewing).
//
//------------------------------------------------------------------------------
void
CMILMatrix::GetScaleDimensions(
    _Out_writes_(1) XFLOAT *prScaleX,
    _Out_writes_(1) XFLOAT *prScaleY
    ) const
{
    ASSERT(prScaleX);
    ASSERT(prScaleY);

    // rScaleX is the length of the transform of the vector (1,0).
    // rScaleY is the length of the transform of the vector (0,1).

    XFLOAT rScaleX = sqrtf(_11 * _11 + _12 * _12);
    XFLOAT rScaleY = sqrtf(_21 * _21 + _22 * _22);

    // Convert NaN to zero.
    if (!(rScaleX == rScaleX))
    {
        rScaleX = 0;
    }
    if (!(rScaleY == rScaleY))
    {
        rScaleY = 0;
    }

    // Postconditions

    ASSERT(rScaleX >= 0);
    ASSERT(rScaleY >= 0);

    *prScaleX = rScaleX;
    *prScaleY = rScaleY;
}


//------------------------------------------------------------------------------
//
//  Member:    operator==
//
//  Synopsis:  Equality operator overload
//
//------------------------------------------------------------------------------
bool CMILMatrix::operator==(const CMILMatrix& other) const
{
    return (   _11 == other._11 && _12 == other._12
        && _21 == other._21 && _22 == other._22
        && _31 == other._31 && _32 == other._32);
}


//------------------------------------------------------------------------------
//
//  Member:    operator!=
//
//  Synopsis:  Inequality operator overload
//
//------------------------------------------------------------------------------
bool CMILMatrix::operator!=(const CMILMatrix& other) const
{
    return !(*this == other);
}

//------------------------------------------------------------------------------
//
//  Member:    IsAxisAlignedScaleOrTranslationReal
//
//  Synopsis:  Determines if the matrix has only scale or translation.
//                  Uses floating point comparisons.
//
//------------------------------------------------------------------------------
bool CMILMatrix::IsAxisAlignedScaleOrTranslationReal() const
{
    return (IsCloseReal(_12, 0.0f) && IsCloseReal(_21, 0.0f))
        || (IsCloseReal(_11, 0.0f) && IsCloseReal(_22, 0.0f));
}


//-------------------------------------------------------------------------
//
//  Synopsis:
//      Copy constructor from a 2D matrix
//
//      Ignore the third row and column in the 3D matrix. The projected
//      contents will get flattened.
//
//      [ _11 _12  0 ]    [ _11, _12,   0,   0 ]
//      [ _21 _22  0 ] -> [ _21, _22,   0,   0 ]
//      [ _31 _32  1 ]    [   0,   0,   0,   0 ]
//                        [ _31, _32,   0,   1 ]
//
//-------------------------------------------------------------------------
CMILMatrix4x4::CMILMatrix4x4(_In_ const CMILMatrix *pOther)
    : _11(pOther->_11)
    , _12(pOther->_12)
    , _13(0.0f)
    , _14(0.0f)
    , _21(pOther->_21)
    , _22(pOther->_22)
    , _23(0.0f)
    , _24(0.0f)
    , _31(0.0f)
    , _32(0.0f)
    , _33(0.0f)
    , _34(0.0f)
    , _41(pOther->_31)
    , _42(pOther->_32)
    , _43(0.0f)
    , _44(1.0f)
{
}

CMILMatrix4x4::CMILMatrix4x4(_In_ const wfn::Matrix4x4& matrix4x4)
{
    ASSERT(sizeof(CMILMatrix4x4) == sizeof(wfn::Matrix4x4));
    memcpy(this, &matrix4x4, sizeof(wfn::Matrix4x4));
}

CMILMatrix4x4 CMILMatrix4x4::FromFloatArray(_In_ CValue& value)
{
    // We expect exactly 16 floats in the array
    XCP_FAULT_ON_FAILURE(value.GetType() == valueFloatArray && value.GetArrayElementCount() == 16);

    XFLOAT* createValueArray = value.AsFloatArray();
    CMILMatrix4x4 matrix;

    matrix._11 = createValueArray[0];
    matrix._12 = createValueArray[1];
    matrix._13 = createValueArray[2];
    matrix._14 = createValueArray[3];

    matrix._21 = createValueArray[4];
    matrix._22 = createValueArray[5];
    matrix._23 = createValueArray[6];
    matrix._24 = createValueArray[7];

    matrix._31 = createValueArray[8];
    matrix._32 = createValueArray[9];
    matrix._33 = createValueArray[10];
    matrix._34 = createValueArray[11];

    matrix._41 = createValueArray[12];
    matrix._42 = createValueArray[13];
    matrix._43 = createValueArray[14];
    matrix._44 = createValueArray[15];

    return matrix;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Set to a perspective projection
//
//-------------------------------------------------------------------------
void
CMILMatrix4x4::SetToPerspective(
    XFLOAT rNear,
    XFLOAT rFar,
    XFLOAT rFieldOfView,
    XFLOAT rAspectRatio,
    XUINT32 bRightHanded
    )
{
    //
    // Using the math from D3DXMatrixPerspectiveFovLH/RH
    //

    XFLOAT rYScale =  1.0f / MathTanDegrees( rFieldOfView/2.0f ); // 1 / tan( fovY / 2 )
    XFLOAT rXScale = rYScale / rAspectRatio;
    XFLOAT rDeltaZ = rFar - rNear;
    XFLOAT rRightHandedInvert = bRightHanded ? -1.0f : 1.0f;

    _11 = rXScale;
    _12 = 0.0f;
    _13 = 0.0f;
    _14 = 0.0f;

    _21 = 0.0f;
    _22 = rYScale;
    _23 = 0.0f;
    _24 = 0.0f;

    _31 = 0.0f;
    _32 = 0.0f;
    _33 = rFar / rDeltaZ * rRightHandedInvert;
    _34 = rRightHandedInvert;

    _41 = 0.0f;
    _42 = 0.0f;
    _43 = -rNear * rFar / rDeltaZ;
    _44 = 0.0f;

}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Set to a compound 2D projection
//
//-------------------------------------------------------------------------
void
CMILMatrix4x4::SetTo2DTransform( _In_ const CMILMatrix *pmat2D )
{
    _11 = pmat2D->_11;
    _12 = pmat2D->_12;
    _13 = 0.0f;
    _14 = 0.0f;

    _21 = pmat2D->_21;
    _22 = pmat2D->_22;
    _23 = 0.0f;
    _24 = 0.0f;

    _31 = 0.0f;
    _32 = 0.0f;
    _33 = 1.0f;
    _34 = 0.0f;

    _41 = pmat2D->_31;
    _42 = pmat2D->_32;
    _43 = 0.0f;
    _44 = 1.0f;
}

CMILMatrix CMILMatrix4x4::Get2DRepresentation() const
{
    ASSERT(Is2D());
    return Extract2DComponents();
}

CMILMatrix CMILMatrix4x4::Extract2DComponents() const
{
    CMILMatrix temp;
    temp._11 = _11;
    temp._12 = _12;
    temp._21 = _21;
    temp._22 = _22;
    temp._31 = _41;
    temp._32 = _42;
    return temp;
}

bool CMILMatrix4x4::Is2D() const
{
    return (                          _13 == 0.0f && _14 == 0.0f &&
                                      _23 == 0.0f && _24 == 0.0f &&
        _31 == 0.0f && _32 == 0.0f && _33 == 1.0f && _34 == 0.0f &&
                                      _43 == 0.0f && _44 == 1.0f);
}

bool Matrix4x4Is2D(const  wfn::Matrix4x4& m)
{
    return (                          m.M13 == 0.0f && m.M14 == 0.0f &&
                                      m.M23 == 0.0f && m.M24 == 0.0f &&
    m.M31 == 0.0f && m.M32 == 0.0f && m.M33 == 1.0f && m.M34 == 0.0f &&
                                      m.M43 == 0.0f && m.M44 == 1.0f);
}

bool CMILMatrix4x4::IsScaleOrTranslationOnly() const
{
    return (
                       _12 == 0.0f && _13 == 0.0f && _14 == 0.0f &&
        _21 == 0.0f                && _23 == 0.0f && _24 == 0.0f &&
        _31 == 0.0f && _32 == 0.0f                && _34 == 0.0f &&
                                                     _44 == 1.0f);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the effective 2D scaling done by this transform as a 2D
//      matrix.
//
//-------------------------------------------------------------------------
void
CMILMatrix4x4::Get2DScaleDimensions(
    _In_ const XSIZEF &elementSize,
    XFLOAT minScaleX,
    XFLOAT minScaleY,
    _Out_ XFLOAT *pScaleX,
    _Out_ XFLOAT *pScaleY
    ) const
{
    //
    // If this projection contains a Z offset or a rotation around the X or Y axes, then it is
    // effectively scaling the contents of the projection by bring it closer to or farther away
    // from the camera. We need to take this scaling into account for things like text
    // rasterization to avoid blurry or grainy text when an element is projected towards or
    // away from the camera.
    //
    // We later use the 2D scale to transform the X and Y axes, so we calculate it here by passing
    // the points (x, y), (x+1, y), and (x, y+1) through the projection. We then take the
    // transformed (1, 0) and (0, 1) vectors and use that as the size of the scale.
    //
    // Note that the values we use for (x, y) (i.e. for the origin) matters. If this projection
    // contains a large rotation along X or Y, then one side of the element will have a much
    // larger scale than the opposite side. We need to choose the side of the element with the
    // larger scale, otherwise we'll still end up with blurry text (possibly even more blurry
    // than if we just ignored the scale in the first place).
    //
    // It's difficult to tell what the original 3D transform of the projection was from looking
    // at the matrix, especially since it can be an arbitrary matrix from a Matrix3DTransform. So
    // we just try using all 4 corners of the element as the origin, then take the biggest scale
    // from all of them.
    //

    // 0-2 for the top left points, 3-5 for top right, 6-8 for bottom left, 9-11 for bottom right
    const XUINT32 pointCount = 12;
    XPOINTF points[pointCount];

    points[0].x = points[6].x = 0;  // top left & bottom left
    points[0].y = points[3].y = 0;  // top left & top right
    points[3].x = points[9].x = elementSize.width;  // top right & bottom right
    points[6].y = points[9].y = elementSize.height; // bottom left & bottom right

    for (XUINT32 i = 0; i < pointCount; i+=3)
    {
        points[i+1].x = points[i].x + 1;
        points[i+1].y = points[i].y;

        points[i+2].x = points[i].x;
        points[i+2].y = points[i].y + 1;
    }

    const gsl::span<XPOINTF> pointsSpan(points, pointCount);
    Transform2DPoints_DivideW(pointsSpan, pointsSpan);

    //
    // Since everything is positive, a larger scale is equivalent to a larger square of the
    // scale, so we can skip the square roots and just use (Xx^2 + Xy^2) & (Yx^2 * Yy^2) when
    // looking for the largest scale. We'll take the square root once at the end before returning
    // the scale.
    //
    // We also don't attempt to account for near plane clipping. A corner of the element may be
    // clipped off by the near plane and won't be rendered, but its transformed X and Y axes are
    // still good (after dividing by w) and can still be used. The alternative is to drop these
    // points and not consider them, but that can result in blurry text when the closest corner
    // is just barely clipped off.
    //
    // We clamp the scale at the specified minimum value. There are cases where a projection
    // starts animating at something close to 90, which produces an effective 2D scale very close
    // to 0 in one dimension. The contents of the projection will then be realized at the small
    // scale and will be blurry for the duration of the animation. This is a temporary workaround.
    // The real fix would be to take the entire duration of the animation into account when
    // calculating the effective scale, or to occasionally update the realization during the
    // animation.
    //

    XFLOAT largestScaleXSquared = minScaleX * minScaleX;
    XFLOAT largestScaleYSquared = minScaleY * minScaleY;
    for (XUINT32 i = 0; i < pointCount; i+=3)
    {
        XPOINTF xAxis = points[i+1] - points[i];
        XFLOAT xScaleSquared = (xAxis.x * xAxis.x + xAxis.y * xAxis.y);
        if (xScaleSquared > largestScaleXSquared)
        {
            largestScaleXSquared = xScaleSquared;
        }

        XPOINTF yAxis = points[i+2] - points[i];
        XFLOAT yScaleSquared = (yAxis.x * yAxis.x + yAxis.y * yAxis.y);
        if (yScaleSquared > largestScaleYSquared)
        {
            largestScaleYSquared = yScaleSquared;
        }
    }

    *pScaleX = sqrtf(largestScaleXSquared);
    *pScaleY = sqrtf(largestScaleYSquared);
}

// Note: This assumes the Z position of a 2D point is 0.0 and the W value is 1.0
bool CMILMatrix4x4::Transform2DPoints_DivideW(const gsl::span<const XPOINTF>& source, const gsl::span<XPOINTF>& destination) const
{
    ASSERT(source.size() == destination.size());

    bool areAllWValuesPositive = true;

    for (int i = 0; i < source.size(); i++)
    {
        float x = source[i].x;
        float y = source[i].y;

        // Given that z == 0.0, w == 1.0, compute only the necessary
        // components
        destination[i].x = (_11 * x) + (_21 * y) + (_41);
        destination[i].y = (_12 * x) + (_22 * y) + (_42);
        float w = (_14 * x) + (_24 * y) + (_44);

        if (w != 0)
        {
            destination[i] /= w;
        }

        areAllWValuesPositive = areAllWValuesPositive && (w > 0);
    }

    return areAllWValuesPositive;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Transform the specified Rect using the current matrix.
//
//  Implementation details:
//      The function transforms all 4 points in the rectangle through this
//      matrix, using z=0. We then take a bounding box of the resulting
//      transformed points, ignoring their z values.
//
//-------------------------------------------------------------------------
void
CMILMatrix4x4::TransformBoundsIgnoreZ(
    _In_ const XRECTF *pSource,
    _Out_ XRECTF *pTarget) const
{
    // Get the corners
    XPOINTF4 ptCorners[4];

    ptCorners[0].x = pSource->X;
    ptCorners[0].y = pSource->Y;
    ptCorners[0].z = 0.0f;
    ptCorners[0].w = 1.0f;
    ptCorners[1].x = pSource->X + pSource->Width;
    ptCorners[1].y = pSource->Y;
    ptCorners[1].z = 0.0f;
    ptCorners[1].w = 1.0f;
    ptCorners[2].x = pSource->X;
    ptCorners[2].y = pSource->Y + pSource->Height;
    ptCorners[2].z = 0.0f;
    ptCorners[2].w = 1.0f;
    ptCorners[3].x = pSource->X + pSource->Width;
    ptCorners[3].y = pSource->Y + pSource->Height;
    ptCorners[3].z = 0.0f;
    ptCorners[3].w = 1.0f;

    // Transform all four
    Transform_PreserveW(ptCorners, ptCorners);

    // do a min/max to find the bounds
    XPOINTF ptMin;
    XPOINTF ptMax;
    ptMin.x = XFLOAT_MAX;
    ptMin.y = XFLOAT_MAX;
    ptMax.x = XFLOAT_MIN;
    ptMax.y = XFLOAT_MIN;
    for (XUINT32 i = 0; i < 4; i++)
    {
        ptCorners[i].x = ptCorners[i].x / ptCorners[i].w;
        ptCorners[i].y = ptCorners[i].y / ptCorners[i].w;

        ptMin.x = MIN(ptMin.x, ptCorners[i].x);
        ptMin.y = MIN(ptMin.y, ptCorners[i].y);

        ptMax.x = MAX(ptMax.x, ptCorners[i].x);
        ptMax.y = MAX(ptMax.y, ptCorners[i].y);
    }

    // Now reconstruct new bounds
    pTarget->X = ptMin.x;
    pTarget->Y = ptMin.y;
    pTarget->Width = ptMax.x - ptMin.x;
    pTarget->Height = ptMax.y - ptMin.y;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Replaces the current matrix with the product of another matrix
//      and itself. Treats the incoming 3x3 matrix as a 4x4 matrix with
//      only x and y components.
//
//-------------------------------------------------------------------------
void
CMILMatrix4x4::Append(_In_ const CMILMatrix& that)
{
    CMILMatrix4x4 temp(*this);

    // [ 11 12 13 14 ][ _11 _12  0   0 ]   [ 11_11 + 12_21 + 14_31,  11_12 + 12_22 + 14_32,  13,  14 ]
    // [ 21 22 23 24 ][ _21 _22  0   0 ] = [ 21_11 + 22_21 + 24_31,  21_12 + 22_22 + 24_32,  23,  24 ]
    // [ 31 32 33 34 ][  0   0   1   0 ]   [ 31_11 + 32_21 + 34_31,  31_12 + 32_22 + 34_32,  33,  34 ]
    // [ 41 42 43 44 ][ _31 _32  0   1 ]   [ 41_11 + 42_21 + 44_31,  41_12 + 42_22 + 44_32,  43,  44 ]

    for (XUINT32 row = 0; row < 4; row++)
    {
        m[row][0] = temp.m[row][0] * that._11 + temp.m[row][1] * that._21 + temp.m[row][3] * that._31;
        m[row][1] = temp.m[row][0] * that._12 + temp.m[row][1] * that._22 + temp.m[row][3] * that._32;
        // Column 3 doesn't change
        // Column 4 doesn't change
    }
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Replaces the current matrix with the product of itself and another
//      matrix. Treats the incoming 3x3 matrix as a 4x4 matrix with only
//      x and y components. Ignores the z values.
//
//-------------------------------------------------------------------------
void
CMILMatrix4x4::Prepend_DropZ(_In_ const CMILMatrix& that)
{
    CMILMatrix4x4 temp(*this);
    //
    // [ _11 _12  0   0 ][ 11 12 13 14 ]   [ 11_11 + 21_12,       12_11 + 22_12,       13_11 + 23_12,       14_11 + 24_12      ]
    // [ _21 _22  0   0 ][ 21 22 23 24 ] = [ 11_21 + 21_22,       12_21 + 22_22,       13_21 + 23_22,       14_21 + 24_22      ]
    // [  0   0   0   0 ][ 31 32 33 34 ]   [                  0,                   0,                   0,                   0 ]
    // [ _31 _32  0   1 ][ 41 42 43 44 ]   [ 11_31 + 21_32 + 41,  12_31 + 22_32 + 42,  13_31 + 23_32 + 43,  14_31 + 24_32 + 44 ]
    //
    // first row:
    //   column 1 = 11_11 + 21_12
    //   column 2 = 12_11 + 22_12
    //   column 3 = 13_11 + 23_12
    //   column n = 1n_11 + 2n_12
    //
    // second row:
    //   column 1 = 11_21 + 21_22
    //   column 2 = 12_21 + 22_22
    //   column 3 = 13_21 + 23_22
    //   column n = 1n_21 + 2n_22
    //
    // third row:
    //   all 0
    //
    // fourth row:
    // column 1 = 11_31 + 21_32 + 41
    // column 2 = 12_31 + 22_32 + 42
    // column 3 = 13_31 + 23_32 + 43
    // column n = 1n_31 + 2n_32 + 4n
    //
    for (XUINT32 column = 0; column < 4; column++)
    {
        m[0][column] = temp.m[0][column] * that._11 + temp.m[1][column] * that._12;
        m[1][column] = temp.m[0][column] * that._21 + temp.m[1][column] * that._22;
        m[2][column] = 0;
        m[3][column] = temp.m[0][column] * that._31 + temp.m[1][column] * that._32 + temp.m[3][column];
    }
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Equality operator overload
//
//------------------------------------------------------------------------------
bool
CMILMatrix4x4::operator==(const CMILMatrix4x4& other) const
{
    for (XUINT32 row = 0; row < 4; row++)
    {
        for (XUINT32 column = 0; column < 4; column++)
        {
            if (m[row][column] != other.m[row][column])
            {
                return false;
            }
        }
    }
    return true;
}


//------------------------------------------------------------------------------
//
//  Member:    operator!=
//
//  Synopsis:  Inequality operator overload
//
//------------------------------------------------------------------------------
bool
CMILMatrix4x4::operator!=(const CMILMatrix4x4& other) const
{
    return !(*this == other);
}
