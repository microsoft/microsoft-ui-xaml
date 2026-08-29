// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "Bubbling.h"

#include <CValueBoxer.h>
#include <MetadataAPI.h>
#include <cvalue.h>

#include <FocusProperties.h>
#include "Focusability.h"
#include "FocusableHelper.h"

using namespace Focus;
using namespace DirectUI;

CDependencyObject* XYFocusPrivate::GetDirectionOverride(
    _In_ CDependencyObject* element,
    _In_opt_ CDependencyObject* searchRoot,
    _In_ DirectUI::FocusNavigationDirection direction,
    _In_ bool ignoreFocusabililty)
{
    CDependencyObject* overrideElement = nullptr;

    const KnownPropertyIndex index = GetXYFocusPropertyIndex(element, direction);

    if (index != KnownPropertyIndex::UnknownType_UnknownProperty)
    {
        CValue value;

        if (FAILED(element->GetValueByIndex(index, &value))) { return nullptr; }
        if (FAILED(DirectUI::CValueBoxer::UnwrapWeakRef(&value, DirectUI::MetadataAPI::GetDependencyPropertyByIndex(index), &overrideElement))) { return nullptr; }
        if (overrideElement && (ignoreFocusabililty == false && IsValidCandidate(overrideElement) == false)) { return nullptr; }

        // If an override was specified, but it is located outside the searchRoot, don't use it as the candidate.
        if (searchRoot != nullptr &&
            overrideElement != nullptr &&
            searchRoot->IsAncestorOf(overrideElement) == false) { return nullptr; }
    }

    return overrideElement;
}

CDependencyObject* XYFocusPrivate::TryXYFocusBubble(
    _In_ CDependencyObject* element,
    _In_opt_ CDependencyObject* candidate,
    _In_opt_ CDependencyObject* searchRoot,
    _In_ DirectUI::FocusNavigationDirection direction)
{
    if (candidate == nullptr) { return nullptr; }

    CDependencyObject* nextFocusableElement = candidate;
    auto directionOverrideRoot = GetDirectionOverrideRoot(element, searchRoot, direction);

    if (directionOverrideRoot != nullptr)
    {
        const bool isAncestor = directionOverrideRoot->IsAncestorOf(candidate);
        auto rootOverride = GetDirectionOverride(directionOverrideRoot, searchRoot, direction);

        if (rootOverride != nullptr && !isAncestor)
        {
            nextFocusableElement = rootOverride;
        }
    }

    return nextFocusableElement;
}

CDependencyObject* XYFocusPrivate::GetDirectionOverrideRoot(
    _In_ CDependencyObject* const element,
    _In_opt_ CDependencyObject* searchRoot,
    _In_ const DirectUI::FocusNavigationDirection direction)
{
    CDependencyObject* root = element;

    while (root != nullptr && GetDirectionOverride(root, searchRoot, direction) == nullptr)
    {
        root = root->GetParent();
    }

    return root;
}

DirectUI::XYFocusNavigationStrategy XYFocusPrivate::GetStrategy(
    _In_ CDependencyObject* element,
    _In_ DirectUI::FocusNavigationDirection direction,
    _In_ DirectUI::XYFocusNavigationStrategyOverride navigationStrategyOverride)
{
    static_assert(static_cast<int>(XYFocusNavigationStrategy::Auto) == (static_cast<int>(XYFocusNavigationStrategyOverride::Auto) - 1), "Ensuring XYFocusStrategy enum values are as expected");
    static_assert(static_cast<int>(XYFocusNavigationStrategy::Projection) == (static_cast<int>(XYFocusNavigationStrategyOverride::Projection) - 1), "Ensuring XYFocusStrategy enum values are as expected");
    static_assert(static_cast<int>(XYFocusNavigationStrategy::NavigationDirectionDistance) == (static_cast<int>(XYFocusNavigationStrategyOverride::NavigationDirectionDistance) - 1), "Ensuring XYFocusStrategy enum values are as expected");
    static_assert(static_cast<int>(XYFocusNavigationStrategy::RectilinearDistance) == (static_cast<int>(XYFocusNavigationStrategyOverride::RectilinearDistance) - 1), "Ensuring XYFocusStrategy enum values are as expected");

    const bool isAutoOverride = (navigationStrategyOverride == XYFocusNavigationStrategyOverride::Auto);
    const bool isNavigationStrategySpecified = (navigationStrategyOverride != XYFocusNavigationStrategyOverride::None) && !isAutoOverride;

    if (isNavigationStrategySpecified)
    {
        //We can cast just by offsetting values because we have ensured that the XYFocusStrategy enums offset as expected
        return static_cast<XYFocusNavigationStrategy>(static_cast<int>(navigationStrategyOverride) - 1);
    }
    else if (isAutoOverride)
    {
        //Skip the element if we have an auto override and look at its parent's strategy
        element = element->GetParent();
    }

    if (element->OfTypeByIndex<KnownTypeIndex::UIElement>() == false) { return DirectUI::XYFocusNavigationStrategy::Projection; }

    CValue value;
    KnownPropertyIndex index = GetXYFocusNavigationStrategyPropertyIndex(element, direction);

    // Even though this is an inherited property, we still want to walk up the tree ourselves to check verify that a strategy was not set. We use
    // GetValueInternal instead of GetValueByIndex to ensure we do no go through the inherited code path (this code path will walk up the tree).

    while(element && SUCCEEDED(element->GetValueInternal(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(index), &value)))
    {
        const DirectUI::XYFocusNavigationStrategy mode = static_cast<DirectUI::XYFocusNavigationStrategy>(value.AsEnum());

        if (mode != DirectUI::XYFocusNavigationStrategy::Auto) { return mode; }

        element = element->GetParent();
    }

    // If we fall though here, return the default strategy mode
    return DirectUI::XYFocusNavigationStrategy::Projection;
}

KnownPropertyIndex XYFocusPrivate::GetXYFocusPropertyIndex(
    _In_ CDependencyObject* const element,
    _In_ DirectUI::FocusNavigationDirection direction)
{
    KnownPropertyIndex prop = KnownPropertyIndex::UnknownType_UnknownProperty;

    if (element->IsRightToLeft())
    {
        if (direction == DirectUI::FocusNavigationDirection::Left) { direction = DirectUI::FocusNavigationDirection::Right; }
        else if (direction == DirectUI::FocusNavigationDirection::Right) { direction = DirectUI::FocusNavigationDirection::Left; }
    }

    if (direction == DirectUI::FocusNavigationDirection::Left)
    {
        if (element->OfTypeByIndex<KnownTypeIndex::UIElement>()) { prop = KnownPropertyIndex::UIElement_XYFocusLeft; }
        else if (IFocusable* ifocusable = CFocusableHelper::GetIFocusableForDO(element)) { prop = ifocusable->GetXYFocusLeftPropertyIndex(); }
    }
    else if (direction == DirectUI::FocusNavigationDirection::Right)
    {
        if (element->OfTypeByIndex<KnownTypeIndex::UIElement>()) { prop = KnownPropertyIndex::UIElement_XYFocusRight; }
        else if (IFocusable* ifocusable = CFocusableHelper::GetIFocusableForDO(element)) { prop = ifocusable->GetXYFocusRightPropertyIndex(); }
    }
    else if (direction == DirectUI::FocusNavigationDirection::Up)
    {
        if (element->OfTypeByIndex<KnownTypeIndex::UIElement>()) { prop = KnownPropertyIndex::UIElement_XYFocusUp; }
        else if (IFocusable* ifocusable = CFocusableHelper::GetIFocusableForDO(element)) { prop = ifocusable->GetXYFocusUpPropertyIndex(); }
    }
    else if (direction == DirectUI::FocusNavigationDirection::Down)
    {
        if (element->OfTypeByIndex<KnownTypeIndex::UIElement>()) { prop = KnownPropertyIndex::UIElement_XYFocusDown; }
        else if (IFocusable* ifocusable = CFocusableHelper::GetIFocusableForDO(element)) { prop = ifocusable->GetXYFocusDownPropertyIndex(); }
    }

    return prop;
}

KnownPropertyIndex XYFocusPrivate::GetXYFocusNavigationStrategyPropertyIndex(
    _In_ CDependencyObject* const element,
    _In_ DirectUI::FocusNavigationDirection direction)
{
    KnownPropertyIndex prop = KnownPropertyIndex::UnknownType_UnknownProperty;

    if (element->IsRightToLeft())
    {
        if (direction == DirectUI::FocusNavigationDirection::Left) { direction = DirectUI::FocusNavigationDirection::Right; }
        else if (direction == DirectUI::FocusNavigationDirection::Right) { direction = DirectUI::FocusNavigationDirection::Left; }
    }

    switch (direction)
    {
        case DirectUI::FocusNavigationDirection::Left:
            if (element->OfTypeByIndex<KnownTypeIndex::UIElement>()) { prop = KnownPropertyIndex::UIElement_XYFocusLeftNavigationStrategy; }
            else if (IFocusable* ifocusable = CFocusableHelper::GetIFocusableForDO(element)) { prop = ifocusable->GetXYFocusLeftNavigationStrategyPropertyIndex(); }
            break;
        case DirectUI::FocusNavigationDirection::Right:
            if (element->OfTypeByIndex<KnownTypeIndex::UIElement>()) { prop = KnownPropertyIndex::UIElement_XYFocusRightNavigationStrategy; }
            else if (IFocusable* ifocusable = CFocusableHelper::GetIFocusableForDO(element)) { prop = ifocusable->GetXYFocusRightNavigationStrategyPropertyIndex(); }
            break;
        case DirectUI::FocusNavigationDirection::Up:
            if (element->OfTypeByIndex<KnownTypeIndex::UIElement>()) { prop = KnownPropertyIndex::UIElement_XYFocusUpNavigationStrategy; }
            else if (IFocusable* ifocusable = CFocusableHelper::GetIFocusableForDO(element)) { prop = ifocusable->GetXYFocusUpNavigationStrategyPropertyIndex(); }
            break;
        case DirectUI::FocusNavigationDirection::Down:
            if (element->OfTypeByIndex<KnownTypeIndex::UIElement>()) { prop = KnownPropertyIndex::UIElement_XYFocusDownNavigationStrategy; }
            else if (IFocusable* ifocusable = CFocusableHelper::GetIFocusableForDO(element)) { prop = ifocusable->GetXYFocusDownNavigationStrategyPropertyIndex(); }
            break;
    }

    return prop;
}
