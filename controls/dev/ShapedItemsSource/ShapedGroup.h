// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <vector>

#include "pch.h"
#include "common.h"
#include "ShapingHelpers.h"

#include <winrt/Microsoft.UI.Xaml.Data.h>

// A bucket produced by shaping: a group key, a stable string identity for that key, and the items
// that fell into it.
//
// It presents itself through ICollectionViewGroup — Group() is the key, GroupItems() the items —
// because that is the DECLARED contract layer 3 and the grouped adapter consume. The previous
// design published the same two facts as ICustomPropertyProvider properties named
// "__TableViewSourceGroupIdentity" and "__TableViewSourceGroupKey", which meant every consumer
// below the control had to know a TableView-specific magic string to read a group at all — the
// layering violation that made the grouped adapter un-reusable. Reading a group is now a QI, not a
// string probe, so an app-authored ICollectionViewGroup works with the same code path.
//
// ICustomPropertyProvider is still implemented, but only for GetStringRepresentation: XAML asks
// for it when a header template displays the group directly. It exposes no properties.
class ShapedGroup : public winrt::implements<
    ShapedGroup,
    winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup,
    winrt::Windows::Foundation::Collections::IIterable<winrt::IInspectable>,
    winrt::Microsoft::UI::Xaml::Data::ICustomPropertyProvider,
    winrt::Windows::Foundation::IStringable,
    TabularShapingHelpers::ITabularGroupIdentity>
{
public:
    ShapedGroup(winrt::IInspectable const& key, winrt::hstring const& groupKey);

    // ICollectionViewGroup — the contract consumers below layer 4 are allowed to know about.
    winrt::IInspectable Group() const;
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> GroupItems() const;

    // IIterable — a group is also directly enumerable, which is what the generic
    // "each element of the source is itself a collection" path expects.
    winrt::Windows::Foundation::Collections::IIterator<winrt::IInspectable> First();

    // IStringable / ICustomPropertyProvider
    winrt::hstring ToString();
    winrt::TypeName Type();
    winrt::Microsoft::UI::Xaml::Data::ICustomProperty GetCustomProperty(winrt::hstring const& name);
    winrt::Microsoft::UI::Xaml::Data::ICustomProperty GetIndexedProperty(winrt::hstring const& name, winrt::TypeName const& type);
    winrt::hstring GetStringRepresentation();

    // The stable string identity of this group, unique across the projection. Distinct from the
    // key object: two runs of a key selector can produce equal-but-not-identical key objects, and
    // expansion state has to survive that.
    winrt::hstring GroupKey() const;

    // ITabularGroupIdentity — the layer-1 contract through which layer 3 reads the identity above
    // without depending on this type.
    winrt::hstring StableGroupIdentity() const override;

    void GroupKey(winrt::hstring const& value);
    void Key(winrt::IInspectable const& value);
    void SetItems(std::vector<winrt::IInspectable> const& items);

private:
    winrt::IInspectable m_key{ nullptr };
    winrt::hstring m_groupKey;
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> m_items{ nullptr };
};
