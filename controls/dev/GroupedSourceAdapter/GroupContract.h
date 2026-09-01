// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include "ShapingHelpers.h"

#include <string_view>

#include <winrt/Microsoft.UI.Xaml.Data.h>

// How a grouped structure is READ, expressed once.
//
// A group used to have to answer ICustomPropertyProvider probes for properties named
// "__TableViewSourceGroupIdentity" and "__TableViewSourceGroupKey" before anything could read its
// key — a control-specific magic string baked into every consumer, which is precisely what made
// the grouped stack unusable outside that one control. The contract is now
// ICollectionViewGroup: declared, implementable by an app, and checkable by the compiler.
//
// Layer 3 owns this because layer 3 is the layer that CONSUMES the contract. Layer 2 implements it
// without depending on this header, which keeps the two siblings rather than a stack.
namespace TabularShapingHelpers
{
    // The group's key. A group that declares no key is its own key, which keeps a plain
    // collection-of-collections source working unchanged.
    inline winrt::IInspectable GetGroupKeyObject(winrt::IInspectable const& group)
    {
        if (auto const collectionViewGroup = group.try_as<winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup>())
        {
            if (auto const key = collectionViewGroup.Group())
            {
                return key;
            }
        }

        return group;
    }

    // A stable string form of the group's key, or empty when the key has no value representation.
    //
    // Empty is meaningful: it says "this key cannot be compared by value, so compare the group
    // objects instead". Callers must not treat it as a valid key, or two unrelated groups with
    // non-value keys would collapse onto one another.
    inline winrt::hstring GetGroupKeyIdentity(winrt::IInspectable const& group)
    {
        // A group that minted its own identity wins. Re-deriving one from the key object would
        // discard the app's group identity selector, and would produce nothing at all for a
        // reference-typed key.
        if (group)
        {
            if (auto const identityProvider = group.try_as<TabularShapingHelpers::ITabularGroupIdentity>())
            {
                if (auto const identity = identityProvider->StableGroupIdentity(); !identity.empty())
                {
                    return identity;
                }
            }
        }

        const auto key = TabularShapingHelpers::TabularValueKey::ToObjectLookupKey(GetGroupKeyObject(group), true);

        constexpr std::wstring_view valuePrefix{ L"value:" };
        const std::wstring_view keyView{ key.c_str(), key.size() };
        if (keyView.size() > valuePrefix.size() && keyView.substr(0, valuePrefix.size()) == valuePrefix)
        {
            return winrt::hstring{ keyView.substr(valuePrefix.size()) };
        }

        return L"";
    }
}
