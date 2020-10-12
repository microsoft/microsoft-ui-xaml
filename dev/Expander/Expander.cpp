// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Expander.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "ExpanderAutomationPeer.h"
#include "Utils.h"
#include "winnls.h"

Expander::Expander()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_Expander);

    SetDefaultStyleKey(this);
}

winrt::AutomationPeer Expander::OnCreateAutomationPeer()
{
    return winrt::make<ExpanderAutomationPeer>(*this);
}

void Expander::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    if (ExpanderProperties::IsExpanded())
    {
        winrt::VisualStateManager::GoToState(*this, L"Expanded", true);
        this->RaiseExpandingEvent(*this);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"Collapsed", true);
        this->RaiseCollapsedEvent(*this);
    }
}

void Expander::OnKeyDown(winrt::KeyRoutedEventArgs const& e)
{
    if (m_hasFocus &&
        e.Key() == winrt::VirtualKey::Space)
    {
        if (ExpanderProperties::IsExpanded())
        {
            // If it's currently expanded, we will collapse
            winrt::VisualStateManager::GoToState(*this, L"Collapsed", true);
            ExpanderProperties::IsExpanded(false);
            this->RaiseExpandingEvent(*this);
        }
        else
        {
            // If it's currently collapsed, we will expand
            winrt::VisualStateManager::GoToState(*this, L"Expanded", true);
            ExpanderProperties::IsExpanded(true);
            this->RaiseCollapsedEvent(*this);
        }
        return;
    }

    __super::OnKeyDown(e);
}

void Expander::OnGotFocus(winrt::RoutedEventArgs const& e)
{
    __super::OnGotFocus(e);
    m_hasFocus = true;
}

void Expander::OnLostFocus(winrt::RoutedEventArgs const& e)
{
    __super::OnLostFocus(e);
    m_hasFocus = false;
}

void Expander::RaiseExpandingEvent(const winrt::Expander& container)
{
}

void Expander::RaiseCollapsedEvent(const winrt::Expander& container)
{
}

void Expander::OnIsExpandedPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    bool newValue = unbox_value<bool>(args.NewValue());

    if (newValue)
    {
        winrt::VisualStateManager::GoToState(*this, L"Expanded", true);
        this->RaiseExpandingEvent(*this);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"Collapsed", true);
        this->RaiseCollapsedEvent(*this);
    }
}

void Expander::OnExpandDirectionPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    // TODO: Implement
}
