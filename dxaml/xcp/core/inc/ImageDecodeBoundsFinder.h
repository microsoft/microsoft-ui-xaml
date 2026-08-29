// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Helper class to compute the bounds to decode images to.
// This class is specifically used when images are culled by the RenderWalk.
// Its purpose is to compute the desired decode width/height that would have
// been computed by the RenderWalk.

class ImageDecodeBoundsFinder
{
public:
    // For each element/brush we test, compute a result
    enum Result
    {
        FoundBounds,            // Good to go!  We found a bounds to decode to
        EmptyBounds,            // The bounds is 0 in either dimension and indicates not ready yet
        FallbackToNaturalSize,  // We found a compatibility reason to fall back to decode to natural size
    };

    ImageDecodeBoundsFinder(_In_ CImageSource* pImageSource);

    // For each element/brush we find a result for, add the result to a collection of results for this image.
    void AddResult(Result result);

    // At the end of the process, go through all the results for all parent element/brushes and determine
    // the final result to base our decode size request on.
    Result GetFinalResult();

    // Helper function to determine if this element requires the software rendering code path
    bool UsesSoftwareRendering(_In_ CUIElement* pUIElement);

    // Main function to run the various checks and compute a desired decode bounds
    _Check_return_ HRESULT ComputeDecodeBoundsForUIElement(
        _In_ CUIElement* pUIElement,
        _In_ CImageBrush* pImageBrush);

    // Primary entry point, drives the overall process of finding the bounds to decode to
    _Check_return_ HRESULT FindReasonableDecodeBounds();

    enum FallbackReason
    {
        SynchronousDecode,
        NotInLiveTree,
        BitmapIcon,
        SoftwareRendering,
        UsesImageTiling,
        EmptyBoundsPostRenderWalk,
        NineGrid,
        DragAndDrop,
        DecodeSizeSpecified,
        AnimatedGIF,
    };

    // These enum values can't change since these are passed into the DecodeToRenderSizeDisabled ETW event. If adding
    // another enum value, add another static_assert here. Also, discuss with the owners of App Analysis as the tool may
    // need to be updated to understand the new fallback reason
    static_assert(FallbackReason::SynchronousDecode == 0u, "Can't change FallbackReason::SynchronousDecode enum value");
    static_assert(FallbackReason::NotInLiveTree == 1u, "Can't change FallbackReason::NotInLiveTree enum value");
    static_assert(FallbackReason::BitmapIcon == 2u, "Can't change FallbackReason::BitmapIcon enum value");
    static_assert(FallbackReason::SoftwareRendering == 3u, "Can't change FallbackReason::SoftwareRendering enum value");
    static_assert(FallbackReason::UsesImageTiling == 4u, "Can't change FallbackReason::UsesImageTiling enum value");
    static_assert(FallbackReason::EmptyBoundsPostRenderWalk == 5u, "Can't change FallbackReason::EmptyBoundsPostRenderWalk enum value");
    static_assert(FallbackReason::NineGrid == 6u, "Can't change FallbackReason::NineGrid enum value");
    static_assert(FallbackReason::DragAndDrop == 7u, "Can't change FallbackReason::DragAndDrop enum value");
    static_assert(FallbackReason::DecodeSizeSpecified == 8u, "Can't change FallbackReason::DecodeSizeSpecified enum value");
    static_assert(FallbackReason::AnimatedGIF == 9u, "Can't change FallbackReason::AnimatedGIF enum value");

public:
    CImageSource* m_pImageSource;   // The CImageSource we're finding the bounds for
    std::vector<Result> m_results;  // Stores the results for all possible parents of an ImageBrush
    bool m_continueSearch;          // bail-out flag
    bool m_skipDecode;              // lets the caller know we found empty bounds and should not decode at all
    UINT32 m_width;                 // The desired width to decode to
    UINT32 m_height;                // The desired height to decode to
};

