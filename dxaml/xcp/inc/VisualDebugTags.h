// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//
// An enum for debug tags that we put in a Visual's property set to identify well-known Visuals produced by Xaml.
//
// This used to be done with strings that we kept synchronized between product and test code (look for
// L"_XAML_DEBUG_TAG_PrependVisual" as an example). This header provides some strongly typed values that we can just
// include from both product and test code.
//
enum class VisualDebugTags : uint8_t
{
    WindowedPopup_ContentIslandRootVisual,
    WindowedPopup_DebugVisual,
    WindowedPopup_AnimationRootVisual,
    WindowedPopup_SystemBackdropPlacementVisual,
    WindowedPopup_PublicRootVisual,
    CompNode_PrependVisual,
    CompNode_PrimaryVisual,
    CompNode_ContentVisual,
    CompNode_RoundedCornerClipVisual,
    CompNode_DropShadowVisual,
    // Note: Manually keep this in sync with the VisualDebugTagsNames array in VisualTreeVerifier.cpp and
    // WinRTMockDComp.cpp.
};

// Use this property to set the debug tag. A tagged Visual will have a PropertySet with this property in it, and the
// scalar value will be a VisualDebugTags value.
#ifndef debugTagPropertyName
#define debugTagPropertyName L"_XAML_DEBUG_TAG"
#endif