// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <TreeWalker.h>

#include "Focusability.h"
#include <Hyperlink.h>

#include "FocusProperties.h"

using namespace Focus;

std::vector<Focus::XYFocus::XYFocusParams> XYFocusPrivate::FindElements(
    _In_ CDependencyObject* startRoot,
    _In_ const CDependencyObject* currentElement,
    _In_ const CDependencyObject* const activeScroller,
    _In_ bool ignoreClipping,
    _In_ bool shouldConsiderXYFocusKeyboardNavigation)
{
    const bool isScrolling = (activeScroller != nullptr);
    std::vector<Focus::XYFocus::XYFocusParams> focusList;
    const auto& collection = FocusProperties::GetFocusChildren<CDOCollection>(startRoot);

    if (!collection || collection->IsLeaving()) { return focusList; }

    const unsigned int kidCount = collection->GetCount();

    //Iterate though every node in the tree that is focusable
    for (unsigned int i = 0; i < kidCount; i++)
    {
        xref_ptr<CDependencyObject> child;
        child.attach(static_cast<CDependencyObject*>(collection->GetItemWithAddRef(i)));

        if (child.get() == nullptr) { continue; }

        const bool isEngagementEnabledButNotEngaged = FocusProperties::IsFocusEngagementEnabled(child.get()) && !FocusProperties::IsFocusEngaged(child.get());

        //This is an element that can be focused
        if (child.get() != currentElement && IsValidCandidate(child.get()))
        {
            Focus::XYFocus::XYFocusParams params;

            if (isScrolling)
            {
                auto scrollCandidate = child.get();
                auto bounds = GetBoundsForRanking(scrollCandidate, ignoreClipping);

                //Include all elements participating in scrolling or
                //elements that are currently not occluded (in view) or
                //elements that are currently occluded but part of a parent scrolling surface.
                if (IsCandidateParticipatingInScroll(scrollCandidate, activeScroller) ||
                    !IsOccluded(scrollCandidate, bounds) ||
                    IsCandidateChildOfAncestorScroller(scrollCandidate, activeScroller))
                {
                    params.element = scrollCandidate;
                    params.bounds = bounds;

                    focusList.push_back(params);
                }
            }
            else
            {
                auto bounds = GetBoundsForRanking(child.get(), ignoreClipping);

                params.element = child.get();
                params.bounds = bounds;

                focusList.push_back(params);
            }
        }

        if (IsValidFocusSubtree(child.get(), shouldConsiderXYFocusKeyboardNavigation) && !isEngagementEnabledButNotEngaged)
        {
            const auto subFocusList = FindElements(child.get(), currentElement, activeScroller, ignoreClipping, shouldConsiderXYFocusKeyboardNavigation);
            focusList.insert(focusList.end(), subFocusList.begin(), subFocusList.end());
        }
    }

    return focusList;
}

//Evaluate if the Sub-tree under the current element potentially can contain focusable items
//Note: This isn't the same as IsValidCandidate because a sub-tree under a valid element might not be focusable.
bool XYFocusPrivate::IsValidFocusSubtree(_In_ CDependencyObject* const element, _In_ bool shouldConsiderXYFocusKeyboardNavigation)
{
    const bool isDirectionalRegion =
        shouldConsiderXYFocusKeyboardNavigation &&
        element->OfTypeByIndex<KnownTypeIndex::UIElement>() &&
        IsDirectionalRegion(element);

    return FocusProperties::IsVisible(element) &&
        FocusProperties::IsEnabled(element) &&
        !FocusProperties::ShouldSkipFocusSubTree(element) &&
        (!shouldConsiderXYFocusKeyboardNavigation || isDirectionalRegion);

}

bool XYFocusPrivate::IsCandidateParticipatingInScroll(
    _In_ CDependencyObject* const candidate,
    _In_ const CDependencyObject* const activeScroller)
{
    if (activeScroller == nullptr)
    {
        return false;
    }

    CDependencyObject* parent = candidate;
    while (parent)
    {
        const auto element = do_pointer_cast<CUIElement>(parent);
        if (element && element->IsScroller())
        {
            return (parent == activeScroller);
        }
        parent = parent->GetParent();
    }
    return false;
}

//Walks up the tree from the active scrolling surface to find another scrolling surface that can potentially contain a candidate
//We do this to consider elements that are currently scrolled out of view (wrt the parent scrolling surface), hence occluded.
bool XYFocusPrivate::IsCandidateChildOfAncestorScroller(
    _In_ CDependencyObject* candidate,
    _In_ const CDependencyObject* activeScroller)
{
    if (activeScroller == nullptr)
    {
        return false;
    }

    CDependencyObject* parent = activeScroller->GetParent();
    while (parent)
    {
        const auto element = do_pointer_cast<CUIElement>(parent);
        if (element && element->IsScroller())
        {
            if (parent->IsAncestorOf(candidate))
            {
                return true;
            }
            //We want to continue walking up the tree to look for more scrolling surfaces
            //who could scroll our candidate into view
        }
        parent = parent->GetParent();
    }
    return false;
}

bool XYFocusPrivate::IsDirectionalRegion(_In_ CDependencyObject* element)
{
    if (element->OfTypeByIndex<KnownTypeIndex::UIElement>() == false) { return false; }

    CValue value;
    if (SUCCEEDED(element->GetValueByIndex(KnownPropertyIndex::UIElement_XYFocusKeyboardNavigation, &value)))
    {
        DirectUI::XYFocusKeyboardNavigationMode mode = static_cast<DirectUI::XYFocusKeyboardNavigationMode>(value.AsEnum());
        return mode != DirectUI::XYFocusKeyboardNavigationMode::Disabled;
    }

    return false;
}

XRECTF_RB Focus::XYFocusPrivate::GetBoundsForRanking(
    _In_ CDependencyObject* element,
    _In_ bool ignoreClipping)
{
    XRECTF_RB bounds;
    InvalidRectF(&bounds);

    if (element->OfTypeByIndex<KnownTypeIndex::Hyperlink>())
    {
        element = static_cast<CHyperlink*>(element)->GetContainingFrameworkElement();
    }

    VERIFYHR(static_cast<CUIElement*>(element)->GetGlobalBoundsLogical(&bounds, ignoreClipping));
    return bounds;
}

bool Focus::XYFocusPrivate::IsOccluded(
    _In_ CDependencyObject* element,
    _In_ const XRECTF_RB& elementBounds)
{
    bool isOccluded = false;

    if (element->OfTypeByIndex<KnownTypeIndex::Hyperlink>())
    {
        element = static_cast<CHyperlink*>(element)->GetContainingFrameworkElement();
    }

    auto root = VisualTree::GetRootOrIslandForElement(element);

    if (FAILED(root->IsOccluded(static_cast<CUIElement*>(element), elementBounds, &isOccluded)))
    {
        return true; //Ignore element if it fails Occlusivity Testing
    }

    return isOccluded;
}

