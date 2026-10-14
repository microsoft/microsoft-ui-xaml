// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CppWinRTHelpers.h"

class TableViewContextFlyout : public winrt::implements<TableViewContextFlyout, winrt::IInspectable>
{
public:
    TableViewContextFlyout(
        winrt::FrameworkElement const& target,
        winrt::TableViewColumn const& column,
        winrt::DependencyProperty const& property)
        : m_target(winrt::make_weak(target)), m_column(winrt::make_weak(column)), m_property(property)
    {
    }

    static winrt::com_ptr<TableViewContextFlyout> Create(
        winrt::FrameworkElement const& target,
        winrt::TableViewColumn const& column,
        winrt::DependencyProperty const& property)
    {
        auto subscription = winrt::make_self<TableViewContextFlyout>(target, column, property);
        subscription->m_propertyChangedRevoker = RegisterPropertyChanged(
            column, property,
            [weak = subscription->get_weak()](auto const&, auto const&)
            {
                if (auto const strong = weak.get())
                {
                    strong->Update();
                }
            });
        subscription->Update();
        return subscription;
    }

private:
    void Update()
    {
        auto const target = m_target.get();
        auto const column = m_column.get();
        if (!target || !column)
        {
            return;
        }

        auto const property = winrt::UIElement::ContextFlyoutProperty();
        if (target.GetBindingExpression(property))
        {
            m_ownedFlyout = nullptr;
            return;
        }

        auto const localValue = target.ReadLocalValue(property);
        auto const owned = m_ownedFlyout.get();
        const bool ownsLocalValue = owned && localValue.try_as<winrt::FlyoutBase>() == owned;
        if (!ownsLocalValue)
        {
            m_ownedFlyout = nullptr;
            if (localValue != winrt::DependencyProperty::UnsetValue())
            {
                // The app replaced our value (including with null or a binding).
                return;
            }
        }

        if (auto const flyout = column.GetValue(m_property).try_as<winrt::FlyoutBase>())
        {
            m_ownedFlyout = winrt::make_weak(flyout);
            target.ContextFlyout(flyout);
        }
        else if (ownsLocalValue)
        {
            // Null must restore style/ancestor fallback, not mask a style with a local null.
            m_ownedFlyout = nullptr;
            target.ClearValue(property);
        }
    }

    winrt::weak_ref<winrt::FrameworkElement> m_target;
    winrt::weak_ref<winrt::TableViewColumn> m_column;
    winrt::weak_ref<winrt::FlyoutBase> m_ownedFlyout;
    winrt::DependencyProperty m_property{ nullptr };
    PropertyChanged_revoker m_propertyChangedRevoker{};
};
