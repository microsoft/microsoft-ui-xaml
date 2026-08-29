// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <PALTypes.h>
#include <PALGfx.h>

struct SharedRenderParams;
struct NWRenderParams;
class CBrush;
class CPlainPen;
class CMILMatrix;
class CCoreServices;
template <class T> class xvector;
template <class T> class xref_ptr;

// Fillmode

enum XcpFillMode : uint8_t
{
    XcpFillModeAlternate           = 0,
    XcpFillModeWinding             = 1,
    XcpFillModeAlternateClipped    = 3, // = alternate fill with clipping edges
    XcpFillModeWindingClipped      = 4, // = winding fill with clipping edges
};

enum XcpMappingMode : uint8_t
{
    XcpAbsolute = 0,
    XcpRelative = 1
};

enum XcpPenCap : uint8_t
{
    XcpPenCapFlat = 0,
    XcpPenCapSquare = 1,
    XcpPenCapRound = 2,
    XcpPenCapTriangle = 3,
};

enum XcpLineJoin : uint8_t
{
    XcpLineJoinMiter        = 0,
    XcpLineJoinBevel        = 1,
    XcpLineJoinRound        = 2,
    XcpLineJoinMiterClipped = 3,
};

enum XcpDashStyle : uint8_t
{
    XcpDashStyleSolid      = 0,
    XcpDashStyleDash       = 1,
    XcpDashStyleDot        = 2,
    XcpDashStyleDashDot    = 3,
    XcpDashStyleDashDotDot = 4,
    XcpDashStyleCustom     = 5,
};

enum XcpShapeMaskMode : uint8_t
{
    XcpShapeMaskModeDefault = 0,
    XcpShapeMaskModeStroke = 1,
    XcpShapeMaskModeFill = 2,
};

//-----------------------------------------------------------------------------
//
// Various enums/structs for brushes
//
//-----------------------------------------------------------------------------

enum XcpGradientWrapMode : uint8_t
{
    XcpGradientWrapModeExtend  = 0,     // Pad
    XcpGradientWrapModeFlip    = 1,     // Reflect
    XcpGradientWrapModeTile    = 2      // Repeat
};

enum XcpColorInterpolationMode : uint8_t
{
    // Physically Linear 1.0 Gamma Space (scRGB)
    XcpColorInterpolationModeScRgbLinearInterpolation   = 0,

    // Perceptually Linear 2.2 Gamma Space (sRGB)
    XcpColorInterpolationModeSRgbLinearInterpolation    = 1
};

struct XcpGradientStop
{
    XFLOAT  rPosition;
    XCOLORF color;
};

typedef XUINT8 XPATHTYPE;

//-------------------------------------------------------------------------
//
//  Subpixel resolution constants.
//
//  We need to support 2 modes:
//     4x4  - for vector graphics
//     16x1 - for text
//
//  So, our subpixel coordinate system is 16x4 internally and we drop
//  data to downsample to another mode.
//
//-------------------------------------------------------------------------

const XINT32 c_nAAShiftX = 4;
const XINT32 c_nAAShiftY = 2;
const XFLOAT c_fAAFloatToInt = 16.0f; //this is the number we use to convert float point to 28.4 int value
const XINT32 c_nAAFloatToIntShift = 4; //Shift to convert back from 28.4 int to subpixel coordinates

#define AASIZE(shiftsize)  (1 << (shiftsize))
#define AAMASK(shiftsize)  (AASIZE(shiftsize)-1)

