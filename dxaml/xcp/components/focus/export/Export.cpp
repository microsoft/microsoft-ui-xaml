// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <uielement.h>
#include <DOPointerCast.h>

#include "Focusability.h"
#include "FocusProperties.h"

#include <FxCallbacks.h>

// This file represents a convinient place to explicitly declare template specializations that should be linked
// outside the focus directory. The base templates in this file have been marked extern, meaning that they will not
// be compiled. This allows us to override their functionality in unittests (similar to how stubs work). However, this also means
// that we need to explicitly declare it so that MUX can successfully link.

// XYFocus Focusability
template bool Focus::XYFocusPrivate::IsValidCandidate<CDependencyObject>(_In_ CDependencyObject* const object);

void FocusProperties::IsScrollable(
    _In_ CDependencyObject* const element,
    _Out_ bool* const horizontally,
    _Out_ bool* const vertically)
{
    *horizontally = false;
    *vertically = false;

    if (element->OfTypeByIndex<KnownTypeIndex::ScrollContentPresenter>())
    {
        VERIFYHR(FxCallbacks::UIElement_IsScrollViewerContentScrollable(
            static_cast<CUIElement*>(element),
            horizontally,
            vertically));
    }
    else
    {
        auto uielement = do_pointer_cast<CUIElement>(element);
        if (uielement && uielement->IsScroller())
        {
            const auto desiredSize = uielement->DesiredSize;
            const auto unclippedDesiredSize = uielement->UnclippedDesiredSize;

            *horizontally = unclippedDesiredSize.width > desiredSize.width;
            *vertically = unclippedDesiredSize.height > desiredSize.height;
        }
    }
}