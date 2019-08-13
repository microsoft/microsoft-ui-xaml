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

    if (auto tabView = SharedHelpers::GetAncestorOfType<winrt::TabView>(winrt::VisualTreeHelper::GetParent(*this)))
    {
        m_CanCloseTabsChangedRevoker = RegisterPropertyChanged(tabView, winrt::TabView::CanCloseTabsProperty(), { this, &TabViewItem::OnCloseButtonPropertyChanged });
    }

    m_dragStartingRevoker = DragStarting(winrt::auto_revoke, { this, &TabViewItem::OnDragStarting });
    m_dragOverRevoker = DragOver(winrt::auto_revoke, { this, &TabViewItem::OnDragOver });
    m_dropRevoker = Drop(winrt::auto_revoke, { this, &TabViewItem::OnDrop });
}

void TabViewItem::OnDragOver(const winrt::IInspectable& sender, const winrt::DragEventArgs& args)
{
    args.AcceptedOperation( winrt::DataPackageOperation::Move);
}

void TabViewItem::OnDrop(const winrt::IInspectable& sender, const winrt::DragEventArgs& args)
{
    
}


winrt::AutomationPeer TabViewItem::OnCreateAutomationPeer()
{
    return winrt::make<TabViewItemAutomationPeer>(*this);
}

void TabViewItem::OnLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    UpdateCloseButton();
}

bool TabViewItem::CanClose()
{
    bool canClose = false;
    if (auto tabView = SharedHelpers::GetAncestorOfType<winrt::TabView>(winrt::VisualTreeHelper::GetParent(*this)))
    {
        // IsCloseable defaults to true, but if it hasn't been set then CanCloseTabs should override it.
        canClose =
            IsCloseable()
            && (ReadLocalValue(IsCloseableProperty()) != winrt::DependencyProperty::UnsetValue()
                || tabView.CanCloseTabs());
    }
    return canClose;
}

void TabViewItem::UpdateCloseButton()
{
    if (auto&& closeButton = m_closeButton.get())
    {
        closeButton.Visibility(CanClose() ? winrt::Visibility::Visible : winrt::Visibility::Collapsed);
    }
}

void TabViewItem::TryClose()
{
    if (auto tabView = SharedHelpers::GetAncestorOfType<winrt::TabView>(winrt::VisualTreeHelper::GetParent(*this)))
    {
        auto args = winrt::make_self<TabViewTabClosingEventArgs>(*this);
        m_tabClosingEventSource(*this, *args);

        if (!args->Cancel())
        {
            auto internalTabView = winrt::get_self<TabView>(tabView);
            internalTabView->CloseTab(*this);
        }
    }
}

void TabViewItem::OnCloseButtonClick(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    TryClose();
}

void TabViewItem::OnCloseButtonPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    UpdateCloseButton();
}

void TabViewItem::OnIsCloseablePropertyChanged(const winrt::DependencyPropertyChangedEventArgs&)
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
    ItemContainer::OnPointerPressed(args);

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
    ItemContainer::OnPointerReleased(args);

    if (m_hasPointerCapture)
    {
        if (args.GetCurrentPoint(nullptr).Properties().PointerUpdateKind() == winrt::PointerUpdateKind::MiddleButtonReleased)
        {
            bool wasPressed = m_isMiddlePointerButtonPressed;
            m_isMiddlePointerButtonPressed = false;
            ReleasePointerCapture(args.Pointer());

            if (wasPressed)
            {
                if (CanClose())
                {
                    TryClose();
                }
            }
        }
    }
}

void TabViewItem::OnPointerEntered(winrt::PointerRoutedEventArgs const& args)
{
    ItemContainer::OnPointerEntered(args);

    if (m_hasPointerCapture)
    {
        m_isMiddlePointerButtonPressed = true;
    }
}

void TabViewItem::OnPointerExited(winrt::PointerRoutedEventArgs const& args)
{
    ItemContainer::OnPointerExited(args);

    m_isMiddlePointerButtonPressed = false;
}

void TabViewItem::OnPointerCanceled(winrt::PointerRoutedEventArgs const& args)
{
    ItemContainer::OnPointerCanceled(args);

    if (m_hasPointerCapture)
    {
        ReleasePointerCapture(args.Pointer());
        m_isMiddlePointerButtonPressed = false;
    }
}

void TabViewItem::OnPointerCaptureLost(winrt::PointerRoutedEventArgs const& args)
{
    ItemContainer::OnPointerCaptureLost(args);

    m_hasPointerCapture = false;
    m_isMiddlePointerButtonPressed = false;
}

void TabViewItem::OnDragStarting(const winrt::UIElement& sender, const winrt::DragStartingEventArgs& args)
{
    if (auto tabView = SharedHelpers::GetAncestorOfType<winrt::TabView>(winrt::VisualTreeHelper::GetParent(*this)))
    {
        auto internalTabView = winrt::get_self<TabView>(tabView);
        internalTabView->OnItemDragStarting(*this, args);
    }
}
