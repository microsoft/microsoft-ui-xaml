// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FocusChildrenIteratorWrapper.h"

class CDOCollection;

namespace FocusProperties
{
    template<class Collection, class DependencyObject>
    Collection* GetFocusChildren(_In_ DependencyObject* const object)
    {
        Collection* collection = nullptr;
        IGNOREHR(object->GetFocusableChildren(&collection));

        return collection;
    }

    template<class DependencyObject>
    bool IsVisible(_In_ DependencyObject* const object)
    {
        return object->IsVisible();
    }

    template<class DependencyObject>
    bool AreAllAncestorsVisible(_In_ DependencyObject* const object)
    {
        return object->AreAllAncestorsVisible();
    }

    template<class DependencyObject>
    bool IsEnabled(_In_ DependencyObject* const object)
    {
        return object->IsEnabled() == TRUE;
    }

    template<class DependencyObject>
    bool IsFocusable(_In_ DependencyObject* const object, bool /*ignoreOffScreenPosition*/)
    {
        return object->IsFocusable() == TRUE;
    }

    template<class DependencyObject>
    bool IsPotentialTabStop(_In_ DependencyObject* const object)
    {
        return false;
    }

    template<class DependencyObject>
    bool CanHaveFocusableChildren(_In_ DependencyObject* const object)
    {
        return false;
    }

    template<class DependencyObject>
    bool IsFocusEngagementEnabled(_In_ DependencyObject* const control)
    {
        return control->IsFocusEngagementEnabled() == TRUE;
    }

    template<class DependencyObject>
    bool IsFocusEngaged(_In_ DependencyObject* const control)
    {
        return control->IsFocusEngaged() == TRUE;
    }

    template<class DependencyObject>
    bool IsGamepadFocusCandidate(_In_ DependencyObject* const candidate)
    {
        return candidate->IsGamepadFocusCandidate();
    }

    template<class DependencyObject>
    bool ShouldSkipFocusSubTree(_In_ DependencyObject* const parent)
    {
        return false;
    }

    template<class DependencyObject>
    bool HasFocusedElement(_In_ DependencyObject* const element)
    {
        return false;
    }

    void IsScrollable(_In_ CDependencyObject* const element, _Out_ bool* const horizontally, _Out_ bool* const vertically);

    FocusChildrenIteratorWrapper GetFocusChildrenInTabOrderIterator(_In_ CDependencyObject* const parent);

    template<> CDOCollection* GetFocusChildren<CDOCollection, CDependencyObject>(_In_ CDependencyObject* const object);
    template<> bool IsVisible<CDependencyObject>(_In_ CDependencyObject* const object);
    template<> bool AreAllAncestorsVisible<CDependencyObject>(_In_ CDependencyObject* const object);
    template<> bool IsEnabled<CDependencyObject>(_In_ CDependencyObject* const object);
    template<> bool IsFocusable<CDependencyObject>(_In_ CDependencyObject* const object, bool ignoreOffScreenPosition);
    template<> bool IsPotentialTabStop<CDependencyObject>(_In_ CDependencyObject* const object);
    template<> bool CanHaveFocusableChildren<CDependencyObject>(_In_ CDependencyObject* const parent);
    template<> bool IsFocusEngagementEnabled<CDependencyObject>(_In_ CDependencyObject* const object);
    template<> bool IsFocusEngaged<CDependencyObject>(_In_ CDependencyObject* const object);
    template<> bool IsGamepadFocusCandidate<CDependencyObject>(_In_ CDependencyObject* const object);
    template<> bool ShouldSkipFocusSubTree<CDependencyObject>(_In_ CDependencyObject* const parent);
    template<> bool HasFocusedElement<CDependencyObject>(_In_ CDependencyObject* const element);
}
