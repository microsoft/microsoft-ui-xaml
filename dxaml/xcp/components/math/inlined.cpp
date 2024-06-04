// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "real.h"

//+------------------------------------------------------------------------
//
//  Function:   XcpFloor
//
//  Synopsis:   Floor version of XcpRound that handles all values
//
//-------------------------------------------------------------------------
XINT32
XcpFloor(_In_ XDOUBLE x)
{
    XINT32 result;

    // XINT32_MAX/XINT32_MIN constants duplicated below so that real.h is usable anywhere without
    // header dependencies

    const int nMaxInt = 2147483647;
    const int nMinInt = (-2147483647 - 1);
    const float rMaxInt = (float)nMaxInt;
    const float rMinInt = (float)nMinInt;

    if (x >= rMaxInt)
    {
        result = nMaxInt;
    }
    else if (x <= rMinInt)
    {
        result = nMinInt;
    }
    else
    {
        union {
            double d;
#if BIGENDIAN
            struct {
                int msb;
                unsigned int lsb;
            } i;
#else
            struct {
                unsigned int lsb;
                int msb;
            } i;
#endif
        } di;

        di.d = (x-.25) + (0x000C000000000000LL);
        result = (di.i.lsb >> 1) | (di.i.msb << 31);
        // TODO: 100953 - fix underlying fp precision issues causing
        // this assert to fail in powerpoint hosted apps.
        //ASSERT(x >= result && x < result + 1);
    }

    return result;
}

//+------------------------------------------------------------------------
//
//  Function:   XcpFloor64
//
//  Synopsis:   Floor version of XcpRound that handles all values
//
//-------------------------------------------------------------------------
XINT64
XcpFloor64(_In_ XDOUBLE x)
{
    XINT64 result;

    // XINT64_MAX/XINT64_MIN constants duplicated below so that real.h is usable anywhere without
    // header dependencies
    // We can only represent a number that is exactly represented by a double's mantissa
    // so we set the max and min to be 2^52

    const XINT64 nMaxInt = 1125899906842623i64;
    const XINT64 nMinInt = (-1125899906842623i64 - 1);

    const double rMaxInt = (double)nMaxInt;
    const double rMinInt = (double)nMinInt;

    if (x >= rMaxInt)
    {
        result = nMaxInt;
    }
    else if (x <= rMinInt)
    {
        result = nMinInt;
    }
    else
    {
        union {
            double d;
#if BIGENDIAN
            struct {
                int msb;
                unsigned int lsb;
            } i;
#else
            struct {
                unsigned int lsb;
                int msb;
            } i;
#endif
        } di;

        di.d = (x-.25) + (0x000C000000000000LL);
        result = ((XINT64) di.i.lsb >> 1) | ((XINT64) di.i.msb << 31);
        result &= 0x0003FFFFFFFFFFFFLL;
    }

    ASSERT(x >= result && x < result + 1);
    return result;
}

//+------------------------------------------------------------------------
//
//  Function:   XcpRound
//
//  Synopsis:   Faster cross-platform version of round than CRT
//
//-------------------------------------------------------------------------
XINT32
XcpRound(_In_ XDOUBLE x)
{
    return XcpFloor(x+0.5);
}

//+------------------------------------------------------------------------
//
//  Function:   XcpRound64
//
//  Synopsis:   Faster cross-platform version of round than CRT
//
//-------------------------------------------------------------------------
XINT64
XcpRound64(_In_ XDOUBLE x)
{
    return XcpFloor64(x+0.5);
}

//+------------------------------------------------------------------------
//
//  Function:   XcpCeiling
//
//  Synopsis:   Faster cross-platform version of ceiling than CRT
//
//-------------------------------------------------------------------------
XINT32
XcpCeiling(_In_ XDOUBLE x)
{
    return -XcpFloor(-x);
}

//+------------------------------------------------------------------------
//
//  Method:
//      XcpNextSmaller
//
//  Synopsis:
//      Compute max float less than given.
//
//  Note:
//      This routine works only for positive given numbers.
//      Negatives, zeros, infinity and NaN are asserted but not handled;
//
//-------------------------------------------------------------------------

XFLOAT
XcpNextSmaller(_In_ XFLOAT x)
{
    FI fi;
    fi.f = x;

    // This routine works only for positive given numbers.
    // Following assertion detects infinity and NaNs.
    ASSERT(fi.i > 0 && fi.i < 0x7F800000);

    fi.i--;
    return fi.f;
}

//------------------------------------------------------------------------------
//
//  A helper function that checks if a given XFLOAT represents a NaN.
//
//------------------------------------------------------------------------------
bool
IsNanF(
    XFLOAT value
    )
{
    // Take a short-cut per the IEEE 754 standard and test for NaNs by
    // comparing the values to themselves. This will save us a lot of
    // work on a fairly frequently used code path.
    return (value != value);
}


//------------------------------------------------------------------------------
//
//  A helper function that checks if a given XFLOAT represents +INF or -INF.
//
//------------------------------------------------------------------------------
bool
IsInfiniteF(
    XFLOAT value
    )
{
    // Test for infinity by looking at the exponent, which will have
    // all bits set for values denoting infinity and mantissa, which
    // will have all of its bits reset if we're dealing with an infinity.
    // Note that we ignore the sign and return TRUE for both positive
    // and negative infinities.
    const XUINT32 valueAsBits = *reinterpret_cast<XUINT32 *>(&value);
    return (valueAsBits & 0x7FFFFFFF) == 0x7F800000;
}

//------------------------------------------------------------------------------
//
//  A helper function that checks if a given XFLOAT represents finite number only.
//
//------------------------------------------------------------------------------
bool
IsFiniteF(
    XFLOAT value
    )
{
    return !IsInfiniteF(value) && !IsNanF(value);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the absolute value
//
//------------------------------------------------------------------------
XINT32
XcpAbs(_In_ XINT32 nValue)
{
    if (nValue < 0)
    {
        return -nValue;
    }

    return nValue;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the absolute value
//
//------------------------------------------------------------------------
XFLOAT
XcpAbsF(_In_ XFLOAT rValue)
{
    if (rValue < 0)
    {
        return -rValue;
    }

    return rValue;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the absolute value
//
//------------------------------------------------------------------------
XDOUBLE
XcpAbsD(_In_ XDOUBLE rValue)
{
    if (rValue < 0)
    {
        return -rValue;
    }

    return rValue;
}

//+------------------------------------------------------------------------
//
//  Synopsis:  Clamps an integer to the specified range.
//
//-------------------------------------------------------------------------
int
ClampInteger(_In_ XINT32 nValue, _In_ XINT32 nMin, _In_ XINT32 nMax)
{
    return ClampValue<XINT32>(nValue, nMin, nMax);
}

//+------------------------------------------------------------------------
//
//  Synopsis:  Clamps a float to the specified range.
//
//-------------------------------------------------------------------------
XFLOAT
ClampReal(_In_ XFLOAT rValue, _In_ XFLOAT rMin, _In_ XFLOAT rMax)
{
    return ClampValue<XFLOAT>(rValue, rMin, rMax);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Removes the integral component and returns just the fractional
//      component of the argument float.
//
//-------------------------------------------------------------------------
XFLOAT
FractionReal(XFLOAT val)
{
    return val - static_cast<XFLOAT>(XINT32(floorf(val)));
}

//-------------------------------------------------------------------------
//
//   Return TRUE if two points are close. Close is defined as near enough
//   that the rounding to 32bit float precision could have resulted in the
//   difference. We define an arbitrary number of allowed rounding errors (10).
//   We divide by b to normalize the difference. It doesn't matter which point
//   we divide by - if they're significantly different, we'll return true, and
//   if they're really close, then a==b (almost).
//
// Arguments:
//
//   a, b - input numbers to compare.
//
// Return Value:
//
//   TRUE if the numbers are close enough.
//
//-------------------------------------------------------------------------
bool
IsCloseReal(_In_ const XFLOAT a, _In_ const XFLOAT b)
{
    // if b == 0.0f we don't want to divide by zero. If this happens
    // it's sufficient to use 1.0 as the divisor because REAL_EPSILON
    // should be good enough to test if a number is close enough to zero.

    // NOTE: if b << a, this could cause an FP overflow. Currently we mask
    // these exceptions, but if we unmask them, we should probably check
    // the divide.

    // We assume we can generate an overflow exception without taking down
    // the system. We will still get the right results based on the FPU
    // default handling of the overflow.

    // Ensure that anyone clearing the overflow mask comes and revisits this
    // assumption. If you hit this Assert, it means that the #O exception mask
    // has been cleared. Go check c_wFPCtrlExceptions.

    return( XcpAbsF( (a-b) / ((b==0.0f)?1.0f:b) ) < 10.0f*REAL_EPSILON );
}

bool
IsLessThanReal(_In_ const XFLOAT a, _In_ const XFLOAT b)
{
    return (a < b) && !IsCloseReal(a,b);
}

//-------------------------------------------------------------------------
//
//  Synopsis:  Returns the determinant
//
//-------------------------------------------------------------------------
XFLOAT
Determinant(
    _In_ const XPOINTF &a,
    _In_ const XPOINTF &b)
{
    return (a.x * b.y - a.y * b.x);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Right turn in a left handed coordinate system (left otherwise)
//
//-------------------------------------------------------------------------
XPOINTF
TurnRight(_In_ const XPOINTF &pt)
{
    XPOINTF ptResult;

    ptResult.x = -pt.y;
    ptResult.y = pt.x;

    return ptResult;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Does the rectangle contain the point?
//
//------------------------------------------------------------------------
bool
DoesRectContainPoint(
    _In_ const XRECTF &rect,
    _In_ const XPOINTF &point
    )
{
    return ((point.x >= rect.X) &&
            (point.y >= rect.Y) &&
            (point.x < (rect.X + rect.Width)) &&
            (point.y < (rect.Y + rect.Height)));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Does the rectangle contain the point?
//
//------------------------------------------------------------------------
bool
DoesRectContainPoint(
    _In_ const XRECTF_RB &rect,
    _In_ const XPOINTF &point
    )
{
    return ((point.x >= rect.left) &&
            (point.y >= rect.top) &&
            (point.x < rect.right) &&
            (point.y < rect.bottom));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Does the rectangle contain the point inclusively?
//
//------------------------------------------------------------------------
bool
DoesRectContainPointInclusive(
    _In_ const XRECTF_RB &rect,
    _In_ const XPOINTF &point
    )
{
    return ((point.x >= rect.left) &&
        (point.y >= rect.top) &&
        (point.x <= rect.right) &&
        (point.y <= rect.bottom));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if two rectangles intersect
//
//------------------------------------------------------------------------
bool
DoRectsIntersect(
    _In_ const XRECT &rc1,
    _In_ const XRECT &rc2
    )
{
    // Empty rects have negative or zero width or height. An empty rect
    // never intersects another rect.
    if ( rc1.Height <= 0 || rc1.Width <= 0 ||
         rc2.Height <= 0 || rc2.Width <= 0 )
    {
        return false;
    }

    // It is important to use < rather than <= (and > rather than >=) here to match
    // the other overload of DoRectsIntersect.
    return ((rc1.X < (rc2.X + rc2.Width)) &&
           ((rc1.X + rc1.Width) > rc2.X) &&
           (rc1.Y < (rc2.Y + rc2.Height)) &&
           ((rc1.Y + rc1.Height) > rc2.Y));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if two rectangles intersect
//
//------------------------------------------------------------------------
bool
DoRectsIntersect(
    _In_ const XRECTF_RB &a,
    _In_ const XRECTF_RB &b
    )
{
    return((a.left < b.right) &&
           (a.top < b.bottom) &&
           (a.right > b.left) &&
           (a.bottom > b.top));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if two rectangles intersect inclusively
//
//------------------------------------------------------------------------
bool
DoRectsIntersectInclusive(
    _In_ const XRECTF_RB &a,
    _In_ const XRECTF_RB &b
    )
{
    return((a.left <= b.right) &&
        (a.top <= b.bottom) &&
        (a.right >= b.left) &&
        (a.bottom >= b.top));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if two rectangles intersect
//
//------------------------------------------------------------------------
bool
DoRectsIntersect(
    _In_ const XRECTF &rc1,
    _In_ const XRECTF &rc2
    )
{
    // Empty rects have negative or zero width or height. An empty rect
    // never intersects another rect.
    if ( rc1.Height <= 0 || rc1.Width <= 0 ||
         rc2.Height <= 0 || rc2.Width <= 0 )
    {
        return false;
    }

    // It is important to use < rather than <= (and > rather than >=) here to match
    // the other overload of DoRectsIntersect and to ensure that tiled objects (VSIS and tiled images)
    // can be batched together.
    return ((rc1.X < (rc2.X + rc2.Width)) &&
           ((rc1.X + rc1.Width) > rc2.X) &&
           (rc1.Y < (rc2.Y + rc2.Height)) &&
           ((rc1.Y + rc1.Height) > rc2.Y));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if two rectangles intersect or exactly touch
//
//------------------------------------------------------------------------
bool
DoRectsIntersectInclusive(
    _In_ const XRECTF &rc1,
    _In_ const XRECTF &rc2
    )
{
    // Empty rects have negative or zero width or height. An empty rect
    // never intersects another rect.
    if ( rc1.Height <= 0 || rc1.Width <= 0 ||
         rc2.Height <= 0 || rc2.Width <= 0 )
    {
        return false;
    }

    return ((rc1.X <= (rc2.X + rc2.Width)) &&
           ((rc1.X + rc1.Width) >= rc2.X) &&
           (rc1.Y <= (rc2.Y + rc2.Height)) &&
           ((rc1.Y + rc1.Height) >= rc2.Y));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set empty rect
//
//------------------------------------------------------------------------
void
ZeroSize(
    _Out_ XSIZE *pSize
    )
{
    pSize->Width = 0;
    pSize->Height = 0;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks if a rectangle is empty.
//
//------------------------------------------------------------------------
bool
IsZeroSize(_In_ const XSIZE &size)
{
    return size.Width <= 0 || size.Height <= 0;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set empty rect
//
//------------------------------------------------------------------------
void
EmptyRect(
    _Out_ XRECT *pRect
    )
{
    pRect->X = 0;
    pRect->Y = 0;
    pRect->Width = 0;
    pRect->Height = 0;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks if a rectangle is empty.
//
//------------------------------------------------------------------------
bool
IsEmptyRect(_In_ const XRECT &rect)
{
    return rect.Width <= 0 || rect.Height <= 0;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set empty rect
//
//------------------------------------------------------------------------
void
EmptyRect(
    _Out_ XRECT_RB *pRect
    )
{
    pRect->left = 0;
    pRect->top = 0;
    pRect->right = 0;
    pRect->bottom = 0;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks if a rectangle is empty.
//
//------------------------------------------------------------------------
bool
IsEmptyRect(_In_ const XRECT_RB &rect)
{
    return (rect.right <= rect.left || rect.bottom <= rect.top);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set empty rect
//
//------------------------------------------------------------------------
void
EmptyRectF(
    _Out_ XRECTF *pRect
    )
{
    pRect->X = 0;
    pRect->Y = 0;
    pRect->Width = 0;
    pRect->Height = 0;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks if a rectangle is empty.
//
//------------------------------------------------------------------------
bool
IsEmptyRectF(_In_ const XRECTF &rect)
{
    return rect.Width <= 0.0f || rect.Height <= 0.0f;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set empty rect
//
//------------------------------------------------------------------------
void
EmptyRectF(
    _Out_ XRECTF_RB *pRect
    )
{
    pRect->left = 0;
    pRect->top = 0;
    pRect->right = 0;
    pRect->bottom = 0;
}

void
InvalidRectF(
    _Out_ XRECTF_RB *pRect
)
{
    const float inf = std::numeric_limits<float>::infinity();

    pRect->left = inf;
    pRect->top = inf;
    pRect->right = inf;
    pRect->bottom = inf;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks if a rectangle is empty.
//
//------------------------------------------------------------------------
bool
IsEmptyRectF(_In_ const XRECTF_RB& Rect)
{
    return (Rect.right <= Rect.left || Rect.bottom <= Rect.top);
}

bool IsInvalidRectF(_In_ const XRECTF_RB& rect)
{
    const float inf = std::numeric_limits<float>::infinity();

    return rect.left == inf && rect.right == inf && rect.top == inf && rect.bottom == inf;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets rect to 'empty' clip.  This clip uses extreme values to
//      ensure by default that nothing we can render is clipped.
//
//------------------------------------------------------------------------
void
SetInfiniteClip(_Out_ XRECTF *pClipRect)
{
    // TODO: MERGE: XFLOAT_INF is suspicious, X and Y should be -INF but it yields a compiler warning due to overflow.
    *pClipRect = GetInfiniteClip();
}

XRECTF
GetInfiniteClip()
{
    return {
        XFLOAT_MIN / 2.0f,
        XFLOAT_MIN / 2.0f,
        XFLOAT_MAX,
        XFLOAT_MAX};
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks if a rectangle is infinite.
//
//------------------------------------------------------------------------
bool
IsInfiniteRectF(_In_ const XRECTF& Rect)
{
    return Rect.X <= XFLOAT_MIN / 2.0f
        || Rect.Y <= XFLOAT_MIN / 2.0f
        || Rect.Width >= XFLOAT_MAX
        || Rect.Height >= XFLOAT_MAX;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Convert an XRECTF_RB to an XRECTF
//
//------------------------------------------------------------------------
XRECTF
ToXRectF(_In_ const XRECTF_RB& Rect)
{
    XRECTF Val =
    {
        Rect.left,
        Rect.top,
        Rect.right - Rect.left,
        Rect.bottom - Rect.top
    };

    return Val;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Convert an XRECTF to an XRECTF_RB
//
//------------------------------------------------------------------------
XRECTF_RB
ToXRectFRB(_In_ const XRECTF& Rect)
{
    XRECTF_RB Val =
    {
        Rect.X,
        Rect.Y,
        Rect.X + Rect.Width,
        Rect.Y + Rect.Height
    };

    return Val;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Convert an XRECT_RB to an XRECT
//
//------------------------------------------------------------------------
XRECT
ToXRect(_In_ const XRECT_RB& Rect)
{
    XRECT Val =
    {
        Rect.left,
        Rect.top,
        Rect.right - Rect.left,
        Rect.bottom - Rect.top
    };

    return Val;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Convert an XRECT to an XRECT_RB
//
//------------------------------------------------------------------------
XRECT_RB
ToXRectRB(_In_ const XRECT& Rect)
{
    XRECT_RB Val =
    {
        Rect.X,
        Rect.Y,
        Rect.X + Rect.Width,
        Rect.Y + Rect.Height
    };

    return Val;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Union 2 rectangles
//
//------------------------------------------------------------------------
void
UnionRect(
    _Inout_ XRECT *a,
    _In_ const XRECT *b
    )
{

    if (b->Width > 0 && b->Height > 0)
    {
        if (a->Width == 0 || a->Height == 0)
        {
            *a = *b;
        }
        else
        {
            XINT32 nRightEdge = MAX(a->X + a->Width, b->X + b->Width);
            XINT32 nBottomEdge = MAX(a->Y + a->Height, b->Y + b->Height);

            a->X = MIN(a->X, b->X);
            a->Y = MIN(a->Y, b->Y);
            a->Width = nRightEdge - a->X;
            a->Height = nBottomEdge - a->Y;
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Union 2 rectangles
//
//------------------------------------------------------------------------
void
UnionRect(
    _Inout_ XRECT_RB *a,
    _In_ const XRECT_RB *b
    )
{
    if (b->right > b->left && b->bottom > b->top)
    {
        if (a->right <= a->left || a->bottom <= a->top)
        {
            *a = *b;
        }
        else
        {
            a->left = MIN(a->left, b->left);
            a->right = MAX(a->right, b->right);

            a->top = MIN(a->top, b->top);
            a->bottom = MAX(a->bottom, b->bottom);
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Union 2 rectangles
//
//------------------------------------------------------------------------
void
UnionRectF(
    _Inout_ XRECTF *a,
    _In_ const XRECTF *b
    )
{
    if (b->Width > 0 && b->Height > 0)
    {
        if (a->Width == 0 || a->Height == 0)
        {
            *a = *b;
        }
        else
        {
            XFLOAT nRightEdge = MAX(a->X + a->Width, b->X + b->Width);
            XFLOAT nBottomEdge = MAX(a->Y + a->Height, b->Y + b->Height);

            //
            // If a rectangle was stretched to go from negative infinity to positive
            // infinity, the addition above will add negative infinity to positive
            // infinity and get NaN. In this case the right and bottom should be
            // at XFLOAT_MAX. The left may still be at negative infinity, which is
            // OK since XFLOAT_MAX - negative infinity produces positive infinity
            // for the width as expected.
            //
            if (IsNanF(nRightEdge))
            {
                nRightEdge = XFLOAT_MAX;
            }

            if (IsNanF(nBottomEdge))
            {
                nBottomEdge = XFLOAT_MAX;
            }

            a->X = MIN(a->X, b->X);
            a->Y = MIN(a->Y, b->Y);
            a->Width = nRightEdge - a->X;
            a->Height = nBottomEdge - a->Y;
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Union 2 rectangles
//
//------------------------------------------------------------------------
void
UnionRectF(
    _Inout_ XRECTF_RB *a,
    _In_ const XRECTF_RB *b
    )
{
    if (b->right > b->left && b->bottom > b->top)
    {
        if (a->right <= a->left || a->bottom <= a->top)
        {
            *a = *b;
        }
        else
        {
            a->left = MIN(a->left, b->left);
            a->right = MAX(a->right, b->right);

            a->top = MIN(a->top, b->top);
            a->bottom = MAX(a->bottom, b->bottom);
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Expand rectangle bounds - empty rectangles still have bounds.
//
//------------------------------------------------------------------------
void
UpdateRectBoundsF(
    _Inout_ XRECTF_RB *a,
    _In_ const XRECTF_RB *b
    )
{
    a->left = MIN(a->left, b->left);
    a->right = MAX(a->right, b->right);

    a->top = MIN(a->top, b->top);
    a->bottom = MAX(a->bottom, b->bottom);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Intersects 2 rectangles
//
//------------------------------------------------------------------------
bool
IntersectRect(
    _Inout_ XRECTF *a,
    _In_ const XRECTF *b
    )
{
    bool doRectanglesIntersect = false;

    XFLOAT right = MIN(a->X + a->Width, b->X + b->Width);
    XFLOAT bottom = MIN(a->Y + a->Height, b->Y + b->Height);

    //
    // If a rectangle was stretched to go from negative infinity to positive
    // infinity, the addition above will add negative infinity to positive
    // infinity and get NaN. In this case the right and bottom should be
    // at XFLOAT_MAX. The left may still be at negative infinity, which is
    // OK since XFLOAT_MAX - negative infinity produces positive infinity
    // for the width as expected.
    //
    if (IsNanF(right))
    {
        right = XFLOAT_MAX;
    }

    if (IsNanF(bottom))
    {
        bottom = XFLOAT_MAX;
    }

    a->X = MAX(a->X, b->X);
    a->Y = MAX(a->Y, b->Y);

    if (a->X < right && a->Y < bottom)
    {
        a->Width = right - a->X;
        a->Height = bottom - a->Y;

        doRectanglesIntersect = TRUE;
    }
    else
    {
        a->X = a->Y = a->Width = a->Height = 0.0f;
    }

    return doRectanglesIntersect;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Intersects 2 rectangles
//
//------------------------------------------------------------------------
bool
IntersectRect(_Inout_ XRECT* a, _In_ const XRECT* b)
{
    XINT32 right = MIN(a->X + a->Width, b->X + b->Width);
    XINT32 bottom = MIN(a->Y + a->Height, b->Y + b->Height);

    a->X = MAX(a->X, b->X);
    a->Y = MAX(a->Y, b->Y);

    if (a->X < right && a->Y < bottom)
    {
        a->Width = right - a->X;
        a->Height = bottom - a->Y;
        return true;
    }
    else
    {
        a->X = a->Y = a->Width = a->Height = 0;
        return false;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Intersects 2 rectangles
//
//------------------------------------------------------------------------
bool
IntersectRect(_Inout_ XRECT_RB* a, _In_ const XRECT_RB* b)
{
    XINT32 right = MIN(a->right, b->right);
    XINT32 bottom = MIN(a->bottom, b->bottom);

    a->left = MAX(a->left, b->left);
    a->top = MAX(a->top, b->top);

    if(a->left < right && a->top < bottom)
    {
        a->right = right;
        a->bottom = bottom;
        return true;
    }
    else
    {
        a->left = a->right = a->top = a->bottom = 0;
        return false;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Intersects 2 rectangles
//
//------------------------------------------------------------------------
bool
IntersectRect(_Inout_ XRECTF_RB* a, _In_ const XRECTF_RB* b)
{
    XFLOAT right = MIN(a->right, b->right);
    XFLOAT bottom = MIN(a->bottom, b->bottom);

    a->left = MAX(a->left, b->left);
    a->top = MAX(a->top, b->top);

    if(a->left < right && a->top < bottom)
    {
        a->right = right;
        a->bottom = bottom;
        return true;
    }
    else
    {
        a->left = a->right = a->top = a->bottom = 0.0f;
        return false;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if rect "contained" is contained inside of rect "container"
//      This is an inclusive comparison (a rect is contained inside itself)
//
//------------------------------------------------------------------------
bool
DoesRectContainRect(_In_ const XRECTF_RB* container, _In_ const XRECTF_RB* contained)
{
    return (container->left <= contained->left) &&
           (container->top <= contained->top) &&
           (container->right >= contained->right) &&
           (container->bottom >= contained->bottom);
}

bool
DoesRectContainRect(_In_ const XRECT_RB* container, _In_ const XRECT_RB* contained)
{
    return (container->left <= contained->left) &&
           (container->top <= contained->top) &&
           (container->right >= contained->right) &&
           (container->bottom >= contained->bottom);
}

bool
DoesRectContainRect(_In_ const XRECTF_WH *pContainer, _In_ const XRECTF_WH *pContained)
{
    XRECTF_RB container = ToXRectFRB(*pContainer);
    XRECTF_RB contained = ToXRectFRB(*pContained);

    return DoesRectContainRect(&container, &contained);
}

bool
DoesRectContainRect(_In_ const XRECT_WH *pContainer, _In_ const XRECT_WH *pContained)
{
    XRECT_RB container = ToXRectRB(*pContainer);
    XRECT_RB contained = ToXRectRB(*pContained);

    return DoesRectContainRect(&container, &contained);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Compares to rectangles for inequality
//
//------------------------------------------------------------------------
bool
operator !=(const XRECT_WH& lhs, const XRECT_WH& rhs)
{
    return ((lhs.X != rhs.X) ||
            (lhs.Y != rhs.Y) ||
            (lhs.Width != rhs.Width) ||
            (lhs.Height != rhs.Height));
}

bool
operator !=(const XRECT_RB& lhs, const XRECT_RB& rhs)
{
    return ((lhs.left != rhs.left) ||
            (lhs.top != rhs.top) ||
            (lhs.right != rhs.right) ||
            (lhs.bottom != rhs.bottom));
}

// NOTE: The floating-point rect comparisons do not account for rounding error.
bool
operator !=(const XRECTF_WH& lhs, const XRECTF_WH& rhs)
{
    return ((lhs.X != rhs.X) ||
            (lhs.Y != rhs.Y) ||
            (lhs.Width != rhs.Width) ||
            (lhs.Height != rhs.Height));
}

bool
operator !=(const XRECTF_RB& lhs, const XRECTF_RB& rhs)
{
    return ((lhs.left != rhs.left) ||
            (lhs.top != rhs.top) ||
            (lhs.right != rhs.right) ||
            (lhs.bottom != rhs.bottom));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Compares to rectangles for equality
//
//------------------------------------------------------------------------
bool
operator ==(const XRECT_WH& lhs, const XRECT_WH& rhs)
{
    return !(lhs != rhs);
}

bool
operator ==(const XRECT_RB& lhs, const XRECT_RB& rhs)
{
    return !(lhs != rhs);
}

bool
operator ==(const XRECTF_WH& lhs, const XRECTF_WH& rhs)
{
    return !(lhs != rhs);
}

bool
operator ==(const XRECTF_RB& lhs, const XRECTF_RB& rhs)
{
    return !(lhs != rhs);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Fills a 4-point array in counter-clockwise order with the 4
//      corner points form a rect.
//
//-------------------------------------------------------------------------
void FillPointsFromRectCCW(
    _Out_writes_(4) XPOINTF* pPoints,
    _In_ const XRECTF& rcRect
    )
{
    // Get the four corners of the hit rect in counter-clockwise order
    pPoints[0].x = rcRect.X + rcRect.Width;
    pPoints[0].y = rcRect.Y + rcRect.Height;
    pPoints[1].x = rcRect.X + rcRect.Width;
    pPoints[1].y = rcRect.Y;
    pPoints[2].x = rcRect.X;
    pPoints[2].y = rcRect.Y;
    pPoints[3].x = rcRect.X;
    pPoints[3].y = rcRect.Y + rcRect.Height;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Fills a 4-point array in counter-clockwise order with the 4
//      corner points form a rect.
//
//-------------------------------------------------------------------------
void FillPointsFromRectCCW(
    _Out_writes_(4) XPOINTF4* pPoints,
    _In_ const XRECTF& rcRect
    )
{
    // Get the four corners of the hit rect in counter-clockwise order
    pPoints[0].x = rcRect.X + rcRect.Width;
    pPoints[0].y = rcRect.Y + rcRect.Height;
    pPoints[1].x = rcRect.X + rcRect.Width;
    pPoints[1].y = rcRect.Y;
    pPoints[2].x = rcRect.X;
    pPoints[2].y = rcRect.Y;
    pPoints[3].x = rcRect.X;
    pPoints[3].y = rcRect.Y + rcRect.Height;

    pPoints[0].z = pPoints[1].z = pPoints[2].z = pPoints[3].z = 0.0f;
    pPoints[0].w = pPoints[1].w = pPoints[2].w = pPoints[3].w = 1.0f;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Fills a point with the AA offset or shift depending on the mask
//      00 - no AA, no shifting
//      11 - AA, shifts interior points inward and exterior points outward
//      01 - Shift inwards
//      10 - Shift outwards
//
//-------------------------------------------------------------------------

void FillPointWithAAOffset(
    XUINT32 aaMask,
    _Out_ XINT32* pAAMaskInterior,
    _Out_ XINT32* pAAMaskExterior
    )
{
    switch (aaMask)
    {
        case 3:
            *pAAMaskInterior = 1;
            *pAAMaskExterior = 1;
            break;
        case 2:
            *pAAMaskInterior = -1;
            *pAAMaskExterior = 1;
            break;
        case 1:
            *pAAMaskInterior = 1;
            *pAAMaskExterior = -1;
            break;
        case 0:
            *pAAMaskInterior = 0;
            *pAAMaskExterior = 0;
            break;
        default:
            // Unsupported AA state
            ASSERT(FALSE);
            break;
    }
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Fills a 4-point array in counter-clockwise order with the 4
//      corner points form a rect.
//
//-------------------------------------------------------------------------
void FillPointsFromRectCCW(
    _Out_writes_(4) PointWithAAMasks* pPoints,
    _In_ const XRECTF& rcRect,
    XUINT32 aaLeft,
    XUINT32 aaTop,
    XUINT32 aaRight,
    XUINT32 aaBottom
    )
{
    //
    // Get the four corners of the hit rect in counter-clockwise order
    // Point 0 is the bottom right, 1 is the top right, 2 is the top left, and 3 is the bottom left
    //

    pPoints[0].x = rcRect.X + rcRect.Width;
    pPoints[0].y = rcRect.Y + rcRect.Height;

    pPoints[1].x = rcRect.X + rcRect.Width;
    pPoints[1].y = rcRect.Y;

    pPoints[2].x = rcRect.X;
    pPoints[2].y = rcRect.Y;

    pPoints[3].x = rcRect.X;
    pPoints[3].y = rcRect.Y + rcRect.Height;

    pPoints[0].z = pPoints[1].z = pPoints[2].z = pPoints[3].z = 0.0f;
    pPoints[0].w = pPoints[1].w = pPoints[2].w = pPoints[3].w = 1.0f;

    // 3 to 0 is BL to BR - bottom edge
    FillPointWithAAOffset(
        aaBottom,
        &pPoints[0].aaMaskInteriorToPreviousPoint,
        &pPoints[0].aaMaskExteriorToPreviousPoint
        );

    // 0 to 1 is BR to TR - right edge
    FillPointWithAAOffset(
        aaRight,
        &pPoints[0].aaMaskInteriorToNextPoint,
        &pPoints[0].aaMaskExteriorToNextPoint
        );

    // 1 to 0 is TR to BR - right edge
    FillPointWithAAOffset(
        aaRight,
        &pPoints[1].aaMaskInteriorToPreviousPoint,
        &pPoints[1].aaMaskExteriorToPreviousPoint
        );

    // 1 to 2 is TR to TL - top edge
    FillPointWithAAOffset(
        aaTop,
        &pPoints[1].aaMaskInteriorToNextPoint,
        &pPoints[1].aaMaskExteriorToNextPoint
        );

    // 2 to 1 is TL to TR - top edge
    FillPointWithAAOffset(
        aaTop,
        &pPoints[2].aaMaskInteriorToPreviousPoint,
        &pPoints[2].aaMaskExteriorToPreviousPoint
        );

    // 2 to 3 is TL to BL - left edge
    FillPointWithAAOffset(
        aaLeft,
        &pPoints[2].aaMaskInteriorToNextPoint,
        &pPoints[2].aaMaskExteriorToNextPoint
        );

    // 3 to 2 is BL to TL - left edge
    FillPointWithAAOffset(
        aaLeft,
        &pPoints[3].aaMaskInteriorToPreviousPoint,
        &pPoints[3].aaMaskExteriorToPreviousPoint
        );

    // 3 to 0 is BL to BR - bottom edge
    FillPointWithAAOffset(
        aaBottom,
        &pPoints[3].aaMaskInteriorToNextPoint,
        &pPoints[3].aaMaskExteriorToNextPoint
        );
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Fills a rect from a 4-point array in counter-clockwise order and
//  returns true if the points form a rect.  Otherwise, returns false.
//
//-------------------------------------------------------------------------
bool FillRectFromPointsCCW(const gsl::span<const XPOINTF>& points, XRECTF& rect)
{
    if (points.size() == 4)
    {
        if ( (points[2].x == points[1].x && points[3].x == points[0].x &&
              points[2].y == points[3].y && points[1].y == points[0].y ) ||
             (points[2].y == points[1].y && points[3].y == points[0].y &&
              points[2].x == points[3].x && points[1].x == points[0].x ) )
        {
            rect.X = MIN(points[2].x, points[0].x);
            rect.Y = MIN(points[2].y, points[0].y);
            rect.Width = MAX(points[2].x, points[0].x) - rect.X;
            rect.Height = MAX(points[2].y, points[0].y) - rect.Y;

            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the Dot Product of two vectors ( point structs )
//
//-------------------------------------------------------------------------
XFLOAT DotProduct( _In_ XPOINTF& vecA, _In_ XPOINTF& vecB )
{
    return (vecA.x * vecB.x) + (vecA.y * vecB.y);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Extends the 2D vectors ( x,y ) to 3D ( x,y,z=0) and takes the cross product.
//      Returns the Z component of the resultant vector.
//
//-------------------------------------------------------------------------
XFLOAT CrossProductZ( _In_ XPOINTF& vecA, _In_ XPOINTF& vecB )
{
    return (vecA.x * vecB.y) - (vecA.y * vecB.x);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Rounds the number up to the next power of 2
//
//-------------------------------------------------------------------------
XUINT32 RoundUpToPow2(_In_ XUINT32 a)
{
    a--;
    a |= a >> 1;
    a |= a >> 2;
    a |= a >> 4;
    a |= a >> 8;
    a |= a >> 16;
    a++;
    return a;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Scans from MSB to LSB for the first set bit in the mask and returns
//      it through the first param. Returns 0 iff the mask is 0.
//
//      The function has a leading Xcp to not conflict with a VC++ intrinsic
//
//-------------------------------------------------------------------------

_Success_(return != 0) unsigned char XcpBitScanReverse(_Out_ unsigned long *index, _In_ unsigned long mask)
{
    unsigned long i = 31;
    unsigned long testMask = static_cast<unsigned long>(1) << 31;

    if (mask == 0)
    {
        return 0;
    }

    while (i > 0 && ((mask & testMask) == 0))
    {
        i--;
        testMask >>= 1;
    }
    *index = i;

    return 1;
}

//------------------------------------------------------------------------
//  Method: PullTo01
//  Synopsis:
//        Clips a parameter to the interval [0,1]
//-----------------------------------------------------------------------
bool PullTo01(// Return true if r is in [0,1] within tolerance
    XDOUBLE fuzz,    // In: Computational error tolerance
    _Inout_ XDOUBLE & t)     // In/out: Pulled into [0,1], if close enough
{
    if (t < -fuzz  ||  t > 1 + fuzz)
        return false;
    if (t < 0)
        t = 0;
    if (t > 1)
        t = 1;
    return true;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generates the AA masks for a point created by a clip.
//
//------------------------------------------------------------------------
void GenerateAAMasksForClipPoint(
    _In_ const PointWithAAMasks& ptA,
    _In_ const PointWithAAMasks& ptB,
    bool isPointBInsideClip,
    _Out_ PointWithAAMasks& clipPoint
    )
{
    //
    // For the following case, the polygon's edge XZ is clipped at point Y, and the polygon's
    // edge ZV is clipped at point W:
    //
    //                   clip edge
    //                    /
    //                   /
    //           [Z]---[Y]---------[X]
    //            |    /    edge
    //            |   /
    //            |  /
    //            | /
    //            |/
    //           [W]
    //           /|
    //          / |
    //         /  |
    //        /   |edge
    //       /    |
    //    clip    |
    //    edge   [V]
    //
    // The new edge XY should be antialiased iff the old edge XZ is antialiased. Similarly, WV
    // should be antialiased iff ZV is antialiased. For the new generated edge YW, we use the
    // heuristic that all edges introduced by clipping should be antialiased.
    //
    // This method calculates the intersection between a segment in the polygon and a clip edge,
    // so we're either calculating Y given XZ or W given ZV.
    //
    // If we're calculating W (Z is ptA and V is ptB),
    //   - aa mask to next point = V's aa mask to previous point
    //   - aa mask to previous point = 1 (the previous point was a generated point)
    //
    // And if we're calculating Y (X is ptA and Z is ptB),
    //   - aa mask to next point = 1 (the next point will be a generated point)
    //   - aa mask to previous point = X's aa mask to next point
    //
    // We differentiate these cases using the isPointBInside parameter.
    //
    if (isPointBInsideClip)
    {
        // ptA is Z and ptB is V from the diagram above, result is point W
        clipPoint.aaMaskInteriorToNextPoint = ptB.aaMaskInteriorToPreviousPoint;
        clipPoint.aaMaskExteriorToNextPoint = ptB.aaMaskExteriorToPreviousPoint;

        clipPoint.aaMaskInteriorToPreviousPoint = 1;
        clipPoint.aaMaskExteriorToPreviousPoint = 1;
    }
    else
    {
        // ptA is X and ptB is Z from the diagram above, result is point Y
        clipPoint.aaMaskInteriorToNextPoint = 1;
        clipPoint.aaMaskExteriorToNextPoint = 1;

        clipPoint.aaMaskInteriorToPreviousPoint = ptA.aaMaskInteriorToNextPoint;
        clipPoint.aaMaskExteriorToPreviousPoint = ptA.aaMaskExteriorToNextPoint;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generates the AA masks for a point created by a clip. No-op
//      overload for XPOINTF4.
//
//------------------------------------------------------------------------
void GenerateAAMasksForClipPoint(
    _In_ const XPOINTF4& ptA,
    _In_ const XPOINTF4& ptB,
    bool isPointBInsideClip,
    _In_ XPOINTF4& clipPoint
    )
{
    // Do nothing.
    UNREFERENCED_PARAMETER(ptA);
    UNREFERENCED_PARAMETER(ptB);
    UNREFERENCED_PARAMETER(isPointBInsideClip);
    UNREFERENCED_PARAMETER(clipPoint);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Lines AB and CD intersect at X. Find the scale AX/AB. This scale
//      can then be multiplied into AB and added to A to get the point
//      of intersection X.
//
//------------------------------------------------------------------------
bool GetScaleFactor(
    const XPOINTF& vecBA,
    const XPOINTF& vecCD,
    const XPOINTF& vecCA,
    XDOUBLE fuzz,
    _Inout_ XDOUBLE& scale
    )
{
    XFLOAT det = Determinant(vecBA, vecCD);

    if (det == 0.0f)
    {
        return false;
    }

    scale = Determinant(vecCA, vecCD) / det;

    if (!PullTo01(fuzz, scale))
    {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if a line segment intersects an infinite line and returns
//      the point of intersection.  Algorithm same as IntersectSegments, but
//      without clamping the second line to a segment.
//
//------------------------------------------------------------------------
_Success_(return != 0) bool IntersectSegmentWithLine( // Return true if there is an intersection
    const XPOINTF & A,    // In: Segment's start
    const XPOINTF & B,    // In: Segment's end
    const XPOINTF & C,    // In: A point on the infinite line
    const XPOINTF & D,    // In: A second point on the infinite line
    bool isPointBInside, // In: Whether point B (segment's end) is inside the clipped polygon
    XDOUBLE fuzz,         // In: Computational error tolerance
    _Out_ XPOINTF & result)   // Out: Point of intersection
{
    UNREFERENCED_PARAMETER(isPointBInside);

    XPOINTF BA = B - A;
    XPOINTF CD = C - D;
    XPOINTF CA = C - A;
    XDOUBLE s = 0.0f;

    if (!GetScaleFactor(BA, CD, CA, fuzz, s))
    {
        return false;
    }

    result = A + BA * s;
    return true;
}

XRECTF ConvertRectToXRectF(_In_ const wf::Rect& rect) 
{
    return XRECTF{rect.X, rect.Y, rect.Width, rect.Height};
}

XRECTF_RB ConvertRectToXRectFRB(
    _In_ wf::Rect& rect)
{
    XRECTF_RB coreRect;

    coreRect.left = rect.X;
    coreRect.top = rect.Y;
    coreRect.right = rect.X + rect.Width;
    coreRect.bottom = rect.Y + rect.Height;

    return coreRect;
}

wf::Rect ConvertXRectFRBToRect(
    _In_ XRECTF_RB& rect)
{
    wf::Rect out;

    out.X = rect.left;
    out.Y = rect.top;
    out.Width = rect.right - rect.left;
    out.Height = rect.bottom - rect.top;

    return out;
}

XRECTF ConvertRectToXRECTF(const wf::Rect& rect)
{
    XRECTF result;

    result.X = rect.X;
    result.Y = rect.Y;
    result.Width = rect.Width;
    result.Height = rect.Height;

    return result;
}

wf::Rect ConvertXRECTFToRect(const XRECTF& rect)
{
    wf::Rect result;

    result.X = rect.X;
    result.Y = rect.Y;
    result.Width = rect.Width;
    result.Height = rect.Height;

    return result;
}

