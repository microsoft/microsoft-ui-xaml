// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <corep.h>
#include <FocusProperties.h>

namespace Focus { namespace XYFocusPrivate {
    template<class Element>
    bool IsValidCandidate(_In_ Element* const element)
    {
        const bool isFocusable = FocusProperties::IsFocusable(element, false /*ignoreOffScreenPosition*/);
        const bool isGamepadFocusCandidate = FocusProperties::IsGamepadFocusCandidate(element);
        const bool isRootScrollViewer = element->template OfTypeByIndex<KnownTypeIndex::RootScrollViewer>();
        const bool isValidTabStop = FocusProperties::IsPotentialTabStop(element);

        return isFocusable &&
            isGamepadFocusCandidate &&
            !isRootScrollViewer &&
            isValidTabStop;
    }

    extern template bool IsValidCandidate<CDependencyObject>(_In_ CDependencyObject* const object);
}}