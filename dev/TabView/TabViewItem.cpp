// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TabView.h"
#include "TabViewItem.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "SharedHelpers.h"

TabViewItem::TabViewItem()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_TabViewItem);

    SetDefaultStyleKey(this);

    SetValue(s_TabViewTemplateSettingsProperty, winrt::make<TabViewItemTemplateSettings>());

    RegisterPropertyChangedCallback(winrt::SelectorItem::IsSelectedProperty(), { this, &TabViewItem::OnIsSelectedPropertyChanged });

    Loaded({ this, &TabViewItem::OnLoaded });
}

void TabViewItem::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    m_closeButton.set([this, controlProtected]() {
        auto closeButton = GetTemplateChildT<winrt::Button>(L"CloseButton", controlProtected);
        if (closeButton)
        {
            m_closeButtonClickRevoker = closeButton.Click(winrt::auto_revoke, { this, &TabViewItem::OnCloseButtonClick });
        }
        return closeButton;
    }());

    OnIconSourceChanged();

    if (auto tabView = SharedHelpers::GetAncestorOfType<winrt::TabView>(winrt::VisualTreeHelper::GetParent(*this)))
    {
        if (SharedHelpers::IsThemeShadowAvailable())
        {
            if (auto internalTabView = winrt::get_self<TabView>(tabView))
            {
                winrt::ThemeShadow shadow;
                shadow.Receivers().Append(internalTabView->GetShadowReceiver());
                m_shadow = shadow;

                double shadowDepth = unbox_value<double>(SharedHelpers::FindResource(c_tabViewShadowDepthName, winrt::Application::Current().Resources(), box_value(c_tabShadowDepth)));

                auto currentTranslation = Translation();
                auto translation = winrt::float3{ currentTranslation.x, currentTranslation.y, (float)shadowDepth };
                Translation(translation);

                UpdateShadow();
            }
        }

        m_tabDragStartingRevoker = tabView.TabDragStarting(winrt::auto_revoke, { this, &TabViewItem::OnTabDragStarting });
        m_tabDragCompletedRevoker = tabView.TabDragCompleted(winrt::auto_revoke, { this, &TabViewItem::OnTabDragCompleted });
    }
}

void TabViewItem::OnIsSelectedPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    UpdateShadow();
}

void TabViewItem::UpdateShadow()
{
    if (SharedHelpers::IsThemeShadowAvailable())
    {
        if (IsSelected() && !m_isDragging)
        {
            Shadow(m_shadow.as<winrt::ThemeShadow>());
        }
        else
        {
            Shadow(nullptr);
        }
    }
}

void TabViewItem::OnTabDragStarting(const winrt::IInspectable& sender, const winrt::TabViewTabDragStartingEventArgs& args)
{
    m_isDragging = true;
    UpdateShadow();
}

void TabViewItem::OnTabDragCompleted(const winrt::IInspectable& sender, const winrt::TabViewTabDragCompletedEventArgs& args)
{
    m_isDragging = false;
    UpdateShadow();
}

winrt::AutomationPeer TabViewItem::OnCreateAutomationPeer()
{
    return winrt::make<TabViewItemAutomationPeer>(*this);
}

void TabViewItem::OnLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    UpdateCloseButton();
}

void TabViewItem::UpdateCloseButton()
{
    if (auto && closeButton = m_closeButton.get())
    {
        closeButton.Visibility(IsClosable() ? winrt::Visibility::Visible : winrt::Visibility::Collapsed);
    }
}

void TabViewItem::RequestClose()
{
    if (auto tabView = SharedHelpers::GetAncestorOfType<winrt::TabView>(winrt::VisualTreeHelper::GetParent(*this)))
    {
        if (auto internalTabView = winrt::get_self<TabView>(tabView))
        {
            internalTabView->RequestCloseTab(*this);
        }
    }
}

void TabViewItem::RaiseRequestClose(TabViewTabCloseRequestedEventArgs const& args)
{
    // This should only be called from TabView, to ensure that both this event and the TabView TabRequestedClose event are raised
    m_closeRequestedEventSource(*this, args);
}

void TabViewItem::OnCloseButtonClick(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    RequestClose();
}

void TabViewItem::OnIsClosablePropertyChanged(const winrt::DependencyPropertyChangedEventArgs&)
{
    UpdateCloseButton();
}

void TabViewItem::OnHeaderPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (m_firstTimeSettingToolTip)
    {
        m_firstTimeSettingToolTip = false;

        if (!winrt::ToolTipService::GetToolTip(*this))
        {
            // App author has not specified a tooltip; use our own
            m_toolTip.set([this]() {
                auto toolTip = winrt::ToolTip();
                toolTip.Placement(winrt::Controls::Primitives::PlacementMode::Mouse);
                winrt::ToolTipService::SetToolTip(*this, toolTip);
                return toolTip;
            }());
        }
    }

    if (auto toolTip = m_toolTip.get())
    {
        // Update tooltip text to new header text
        auto headerContent = Header();
        auto potentialString = headerContent.try_as<winrt::IPropertyValue>();

        if (potentialString && potentialString.Type() == winrt::PropertyType::String)
        {
            toolTip.Content(headerContent);
        }
        else
        {
            toolTip.Content(nullptr);
        }
    }
}

void TabViewItem::OnPointerPressed(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerPressed(args);

    if (args.GetCurrentPoint(nullptr).Properties().PointerUpdateKind() == winrt::PointerUpdateKind::MiddleButtonPressed)
    {
        if (CapturePointer(args.Pointer()))
        {
            m_hasPointerCapture = true;
            m_isMiddlePointerButtonPressed = true;
        }
    }
}

void TabViewItem::OnPointerReleased(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerReleased(args);

    if (m_hasPointerCapture)
    {
        if (args.GetCurrentPoint(nullptr).Properties().PointerUpdateKind() == winrt::PointerUpdateKind::MiddleButtonReleased)
        {
            bool wasPressed = m_isMiddlePointerButtonPressed;
            m_isMiddlePointerButtonPressed = false;
            ReleasePointerCapture(args.Pointer());

            if (wasPressed)
            {
                if (IsClosable())
                {
                    RequestClose();
                }
            }
        }
    }
}

void TabViewItem::OnPointerEntered(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerEntered(args);

    if (m_hasPointerCapture)
    {
        m_isMiddlePointerButtonPressed = true;
    }
}

void TabViewItem::OnPointerExited(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerExited(args);

    m_isMiddlePointerButtonPressed = false;
}

void TabViewItem::OnPointerCanceled(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerCanceled(args);

    if (m_hasPointerCapture)
    {
        ReleasePointerCapture(args.Pointer());
        m_isMiddlePointerButtonPressed = false;
    }
}

void TabViewItem::OnPointerCaptureLost(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerCaptureLost(args);

    m_hasPointerCapture = false;
    m_isMiddlePointerButtonPressed = false;
}

void TabViewItem::OnIconSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    OnIconSourceChanged();
}

void TabViewItem::OnIconSourceChanged()
{
    auto const templateSettings = winrt::get_self<TabViewItemTemplateSettings>(TabViewTemplateSettings());
    if (auto const source = IconSource())
    {
        templateSettings->IconElement(SharedHelpers::MakeIconElementFrom(source));
        winrt::VisualStateManager::GoToState(*this, L"Icon"sv, false);
    }
    else
    {
        templateSettings->IconElement(nullptr);
        winrt::VisualStateManager::GoToState(*this, L"NoIcon"sv, false);
    }
}
