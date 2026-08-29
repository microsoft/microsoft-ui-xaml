// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Enums for AutomationPeer
//  Synopsis:
//      The set of Enums defined by AutomationPeer
//      Will be mapped to corresponding UIA Ids

#ifndef __INCLUDE_UIAENUMS__
#define __INCLUDE_UIAENUMS__

namespace UIAXcp
{
#include "UIACoreEnums.g.h"

enum TextPatternRangeEndpoint
{   
    TextPatternRangeEndpoint_Start  = 0,
    TextPatternRangeEndpoint_End    = 1
} ;

enum SupportedTextSelection
{   
    SupportedTextSelection_None = 0,
    SupportedTextSelection_Single   = 1,
    SupportedTextSelection_Multiple = 2
} ;

enum CapStyle
{
    CapStyle_None   = 0,
    CapStyle_SmallCap   = 1,
    CapStyle_AllCap = 2,
    CapStyle_AllPetiteCaps  = 3,
    CapStyle_PetiteCaps = 4,
    CapStyle_Unicase    = 5,
    CapStyle_Titling    = 6,
    CapStyle_Other  = -1
} ;

enum HorizontalTextAlignment
{   
    HorizontalTextAlignment_Left    = 0,
    HorizontalTextAlignment_Centered    = 1,
    HorizontalTextAlignment_Right   = 2,
    HorizontalTextAlignment_Justified   = 3
} ;

enum TextUnit
{   
    TextUnit_Character = 0,
    TextUnit_Format = 1,
    TextUnit_Word = 2,
    TextUnit_Line = 3,
    TextUnit_Paragraph = 4,
    TextUnit_Page = 5,
    TextUnit_Document = 6,
};

//------------------------------------------------------------------------
// AutomationPeer Property enums -- Internal
//------------------------------------------------------------------------

#include "UIAEnums.g.h"

}; // namespace UIAXcp

#endif //__INCLUDE_UIAENUMS__
