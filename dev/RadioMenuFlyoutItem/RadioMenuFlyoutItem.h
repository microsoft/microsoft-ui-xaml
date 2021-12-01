// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "RadioMenuFlyoutItem.g.h"
#include "RadioMenuFlyoutItem.properties.h"


// This type exists for RadioMenuFlyoutItem to derive publically from MenuFlyoutItem, but secretly from ToggleMenuFlyoutItem.
template <typename D, typename T, typename ... I>
struct __declspec(empty_bases) DeriveFromToggleMenuFlyoutItemHelper_base : winrt::Windows::UI::Xaml::Controls::ToggleMenuFlyoutItemT<D, winrt::default_interface<T>, winrt::composable, I...>
{
    using composable = T;
    using class_type = typename T;

    operator class_type() const noexcept
    {
        return static_cast<winrt::IInspectable>(*this).as<class_type>();
    }

    hstring GetRuntimeClassName() const
    {
        return hstring{ winrt::name_of<T>() };
    }

    bool InternalIsChecked()
    {
        return this->IsChecked();
    }

    void InternalIsChecked(bool value)
    {
        this->IsChecked(value);
    }
};

class RadioMenuFlyoutItem :
    public ReferenceTracker<RadioMenuFlyoutItem, DeriveFromToggleMenuFlyoutItemHelper_base, winrt::RadioMenuFlyoutItem>,
    public RadioMenuFlyoutItemProperties
{

public:
    RadioMenuFlyoutItem();
    ~RadioMenuFlyoutItem();

    // IsChecked property is ambiguous with ToggleMenuFlyoutItem, lift up RadioMenuFlyoutItem::IsChecked to disambiguate.
    using RadioMenuFlyoutItemProperties::IsChecked;

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    static void OnAreCheckStatesEnabledPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyPropertyChangedEventArgs& args);

private:
    void OnInternalIsCheckedChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);

    void UpdateCheckedItemInGroup();

    // Copies of IsChecked & GroupName to avoid using those dependency properties in the ~RadioMenuFlyoutItem() destructor which would lead to crashes.
    bool m_isChecked{ false };
    winrt::hstring m_groupName{ L"" };

    bool m_isSafeUncheck{ false };

    PropertyChanged_revoker m_InternalIsCheckedChangedRevoker{};

    static thread_local std::unique_ptr<std::map<winrt::hstring, winrt::weak_ref<RadioMenuFlyoutItem>>> s_selectionMap;
};
