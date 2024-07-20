// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CDependencyObject.h>
#include <VisualTree.h>
#include <XYFocus.h>

namespace Focus { namespace XYFocusPrivate {

    XRECTF_RB GetBoundsForRanking(
        _In_ CDependencyObject* element,
        _In_ bool ignoreClipping);

    bool IsOccluded(
        _In_ CDependencyObject* element,
        _In_ const XRECTF_RB& elementBounds);

    std::vector<Focus::XYFocus::XYFocusParams> FindElements(
        _In_ CDependencyObject* startRoot,
        _In_ const CDependencyObject* currentElement,
        _In_ const CDependencyObject* const activeScroller,
        _In_ bool ignoreClipping,
        _In_ bool shouldConsiderXYFocusKeyboardNavigation);

    bool IsValidFocusSubtree(
        _In_ CDependencyObject* const element,
        _In_ bool shouldConsiderXYFocusKeyboardNavigation);

    bool IsDirectionalRegion(
        _In_ CDependencyObject* element);

    //Checks to see if the first found Scroller ancestor is currently processing scroll
    bool IsCandidateParticipatingInScroll(
        _In_ CDependencyObject* const candidate,
        _In_ const CDependencyObject* const activeScroller);

    //Walks up the tree from the active scrolling surface to find another scrolling surface that can potentially contain a candidate
    //We do this to consider elements that are currently scrolled out of view (wrt the parent scrolling surface), hence occluded
    bool IsCandidateChildOfAncestorScroller(
        _In_ CDependencyObject* candidate,
        _In_ const CDependencyObject* activeScroller);
}}