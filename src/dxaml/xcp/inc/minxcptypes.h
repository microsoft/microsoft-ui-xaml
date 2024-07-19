// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// A bare set of xcp typedefs needed to work effectively with the
// to the XAML framework. This include file
// is designed to be included from unit tests and other places where we'd
// like to build isolated classes.

// DO NOT USE THE TYPEDEFS HERE IN NEW CODE. THEY ARE KEPT AROUND MERELY
// TO SCOPE THE WORK REQUIRED TO WORK WITH CLASSES IN THE PLATFORM WITHOUT
// HAVING TO CONVERT LARGE NUMERS OF AUXILIARY FILES.

#pragma once

#include "minerror.h"

#pragma region XType Definitions

// Basic signed integer types
typedef signed char XINT8;
typedef signed short XINT16;
typedef unsigned short XCHAR;
typedef signed int XINT32;
typedef signed long long XINT64;
typedef XINT64 XLONG_PTR;

// Basic unsigned integer types
typedef unsigned char XUINT8;
typedef unsigned short XUINT16;
typedef unsigned int XUINT32;

typedef unsigned long long XUINT64;

// Other integer types
typedef long XLONG;
typedef unsigned long XDWORD;
typedef unsigned char XBYTE;

typedef float XFLOAT;
typedef double XDOUBLE;

// Terminated string type
typedef _Null_terminated_ XCHAR* XSZSTR;

typedef void *XHANDLE;

struct XPOINT
{
    int x;
    int y;

    XPOINT operator-(_In_ const XPOINT &pt) const
    {
        XPOINT ptResult;

        ptResult.x = x - pt.x;
        ptResult.y = y - pt.y;

        return ptResult;
    }
};

struct XPOINTF
{
    float x;
    float y;

    XPOINTF operator+(_In_ const XPOINTF &vec) const
    {
        XPOINTF ptResult;

        ptResult.x = x + vec.x;
        ptResult.y = y + vec.y;

        return ptResult;
    }

    float operator*(_In_ const XPOINTF &pt) const
    {
        // Dot Product
        return (x*pt.x + y*pt.y);
    }

    XPOINTF operator*(_In_ const double rScale) const
    {
        XPOINTF ptResult;

        ptResult.x = x * (float)rScale;
        ptResult.y = y * (float)rScale;

        return ptResult;
    }

    void operator/=(_In_ const double rScale)
    {
        x /= (float)rScale;
        y /= (float)rScale;
    }

    void operator*=(_In_ const float rScale)
    {
        x *= rScale;
        y *= rScale;
    }

    XPOINTF operator-(_In_ const XPOINTF &pt) const
    {
        XPOINTF ptResult;

        ptResult.x = x - pt.x;
        ptResult.y = y - pt.y;

        return ptResult;
    }

    void operator-=(_In_ const XPOINTF &vec)
    {
        x -= vec.x;
        y -= vec.y;
    }

    void operator+=(_In_ const XPOINTF &vec)
    {
        x += vec.x;
        y += vec.y;
    }

    XPOINTF operator / (const float scaleFactor) const
    {
        XPOINTF result = *this;
        result.x /= scaleFactor;
        result.y /= scaleFactor;
        return result;
    }

    XPOINTF operator * (const float scaleFactor) const
    {
        XPOINTF result = *this;
        result.x *= scaleFactor;
        result.y *= scaleFactor;
        return result;
    }
};

inline bool operator == (_In_ const XPOINTF &op1, _In_ const XPOINTF &op2)
{
    return op1.x == op2.x &&
           op1.y == op2.y;
}

// A struct to store (x,y) points with a coordinate space annotation. Used during hit testing.
// The plan is to move all hit testing to this struct so that we'd have coordinate-space safety. The switch will be made incrementally.
struct XPOINTF_COORDS
{
    float x;
    float y;

    //
    // There are multiple sets of coordinate spaces, with different points as the origin and different measures of size. The common ones are:
    // - The top-left of a monitor at (0, 0), and measured in physical pixels.
    // - The top-left of a window at (0, 0), and measured in physical pixels.
    // - The top-left of a window at (0, 0), and measured in logical pixels. This means the display scale (e.g. 150%, 200%) has been divided out.
    //
    // This flag tracks whether we're working in physical pixels or logical pixels.
    //
    bool isPhysicalPixels { false };

    // Converts back to XPOINTF to be fed into old hit testing code.
    XPOINTF ToXPointF_NoDivide() const
    {
        // We might be passed to a function expecting logical or physical. Either can be returned as-is.
        return { x, y };
    }

    // Converts to logical pixels. Divides out a scale if we're in physical pixels.
    XPOINTF ToXPointF_Logical(float scale) const
    {
        if (isPhysicalPixels)
        {
            return { x / scale, y / scale };
        }
        else
        {
            return { x, y };
        }
    }
};

struct XSIZEF
{
    float width;
    float height;
};

inline bool operator==(_In_ const XSIZEF &op1, _In_ const XSIZEF &op2)
{
    return op1.width == op2.width &&
           op1.height == op2.height;
}

struct XSIZE
{
    int Width;
    int Height;
};

struct XRECTF_WH
{
    float X;
    float Y;
    float Width;
    float Height;

    XSIZEF& Size() { return (XSIZEF&)Width; }
    const XSIZEF& Size() const { return (const XSIZEF&)Width; }
    XPOINTF& Point() { return (XPOINTF&)X; }
    float Right() const { return X + Width; }
    float Bottom() const { return Y + Height; }
};
typedef XRECTF_WH XRECTF;

inline XRECTF operator / (const XRECTF& rect, float scaleFactor)
{
    XRECTF result = rect;
    result.X /= scaleFactor;
    result.Y /= scaleFactor;
    result.Width /= scaleFactor;
    result.Height /= scaleFactor;
    return result;
}

inline XRECTF operator * (const XRECTF& rect, float scaleFactor)
{
    XRECTF result = rect;
    result.X *= scaleFactor;
    result.Y *= scaleFactor;
    result.Width *= scaleFactor;
    result.Height *= scaleFactor;
    return result;
}

struct XRECTF_RB
{
    float left;
    float top;
    float right;
    float bottom;

    bool IsUniform() const { return left == top && left == right && left == bottom; }
};
typedef XRECTF_RB XTHICKNESS;

struct XRECT_WH
{
    int X;
    int Y;
    int Width;
    int Height;
};
typedef XRECT_WH XRECT;

struct XRECT_RB
{
    int left;
    int top;
    int right;
    int bottom;
};

struct XRECTF_CR
{
    XFLOAT topLeft;
    XFLOAT topRight;
    XFLOAT bottomRight;
    XFLOAT bottomLeft;
};

inline bool operator==(_In_ const XRECTF_CR &op1, _In_ const XRECTF_CR &op2)
{
    return op1.topLeft == op2.topLeft &&
           op1.topRight == op2.topRight &&
           op1.bottomRight == op2.bottomRight &&
           op1.bottomLeft == op2.bottomLeft;
}

typedef XRECTF_CR XCORNERRADIUS;

struct XPOINTF4
{
    float x;
    float y;
    float z;
    float w;

    void operator*=(_In_ const double rScale)
    {
        x *= (float)rScale;
        y *= (float)rScale;
        z *= (float)rScale;
        w *= (float)rScale;
    }

    void operator/=(_In_ const double rScale)
    {
        x /= (float)rScale;
        y /= (float)rScale;
        z /= (float)rScale;
        w /= (float)rScale;
    }

    XPOINTF4 operator+(_In_ const XPOINTF4 &vec) const
    {
        XPOINTF4 ptResult;

        ptResult.x = x + vec.x;
        ptResult.y = y + vec.y;
        ptResult.z = z + vec.z;
        ptResult.w = w + vec.w;

        return ptResult;
    }

    XPOINTF4 operator-(_In_ const XPOINTF4 &vec) const
    {
        XPOINTF4 ptResult;

        ptResult.x = x - vec.x;
        ptResult.y = y - vec.y;
        ptResult.z = z - vec.z;
        ptResult.w = w - vec.w;

        return ptResult;
    }

    XPOINTF4 operator*(_In_ const double rScale) const
    {
        XPOINTF4 ptResult;

        ptResult.x = x * static_cast<float>(rScale);
        ptResult.y = y * static_cast<float>(rScale);
        ptResult.z = z * static_cast<float>(rScale);
        ptResult.w = w * static_cast<float>(rScale);

        return ptResult;
    }
};

struct PointWithAAMasks
{
    float x;
    float y;
    float z;
    float w;

    // 1 iff the edge is collapsed inward, 0 iff no change, -1 iff edge is expanded outward
    int aaMaskInteriorToPreviousPoint;
    int aaMaskInteriorToNextPoint;

    // -1 iff the edge is collapsed inward, 0 iff no change, 1 iff the edge is expanded outward
    int aaMaskExteriorToNextPoint;
    int aaMaskExteriorToPreviousPoint;
};

struct XMATRIX
{
    float _11;
    float _12;
    float _21;
    float _22;
    float _31;
    float _32;
};

struct XPOINTD
{
    double x;
    double y;
};

namespace DirectUI { enum class GridUnitType : uint8_t; }

struct XGRIDLENGTH
{
    DirectUI::GridUnitType type;
    char padding[3] { };    // XBF writer will stamp this struct onto disk and compare to the XBF master which has zeros in these 3 bytes
    XFLOAT value;

    static XFLOAT Default()
    {
        return 1.0f;
    }
};

static_assert(sizeof(XGRIDLENGTH) == 8, "XGRIDLENGTH is written by XBF");

inline bool operator==(_In_ const XGRIDLENGTH &op1, _In_ const XGRIDLENGTH &op2)
{
    return op1.type == op2.type &&
           op1.value == op2.value;
}

struct XNAME
{
    XUINT32 cNamespace;
    XUINT32 cName;
    const XCHAR  *pNamespace;
    const XCHAR  *pName;
};

struct TextRangeData
{
    XINT32 startIndex;
    XINT32 length;
};

inline bool operator==(const TextRangeData& lhs, const TextRangeData& rhs)
{
    return
        lhs.startIndex == rhs.startIndex &&
        lhs.length == rhs.length;
}

#pragma endregion
