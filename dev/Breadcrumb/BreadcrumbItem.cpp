// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "BreadcrumbItem.h"
#include "RuntimeProfiler.h"
#include "ItemTemplateWrapper.h"
#include "Breadcrumb.h"
#include "BreadcrumbDropDownItem.h"
#include "BreadcrumbItemAutomationPeer.h"

namespace winrt::Microsoft::UI::Xaml::Controls
{
    CppWinRTActivatableClassWithBasicFactory(BreadcrumbItem)
}

#include "BreadcrumbItem.g.cpp"

BreadcrumbItem::BreadcrumbItem()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_BreadcrumbItem);

    SetDefaultStyleKey(this);
    HookListeners();
}

BreadcrumbItem::~BreadcrumbItem()
{
    RevokeListeners();
}

void BreadcrumbItem::HookListeners()
{
    if (auto const& thisAsIUIElement7 = this->try_as<winrt::IUIElement7>())
    {
        thisAsIUIElement7.PreviewKeyDown({ this, &BreadcrumbItem::OnChildPreviewKeyDown });
    }
    else if (auto const& thisAsUIElement = this->try_as<winrt::UIElement>())
    {
        m_keyDownRevoker = AddRoutedEventHandler<RoutedEventType::KeyDown>(thisAsUIElement,
            { this, &BreadcrumbItem::OnChildPreviewKeyDown },
            true /*handledEventsToo*/);
    }

    RegisterPropertyChangedCallback(winrt::FrameworkElement::FlowDirectionProperty(), { this, &BreadcrumbItem::OnFlowDirectionChanged });
}

void BreadcrumbItem::RevokeListeners()
{
    m_keyDownRevoker.revoke();
}

void BreadcrumbItem::RevokePartsListeners()
{
    m_breadcrumbItemButtonLoadedRevoker.revoke();
    m_breadcrumbItemButtonClickRevoker.revoke();
    m_ellipsisRepeaterElementPreparedRevoker.revoke();
    m_ellipsisRepeaterElementIndexChangedRevoker.revoke();
    m_isPressedButtonRevoker.revoke();
    m_isPointerOverButtonRevoker.revoke();
    m_isEnabledButtonRevoker.revoke();
}

void BreadcrumbItem::OnApplyTemplate()
{
    RevokePartsListeners();

    __super::OnApplyTemplate();

    winrt::IControlProtected controlProtected{ *this };

    m_breadcrumbItemButton.set(GetTemplateChildT<winrt::Button>(s_breadcrumbItemButtonPartName, controlProtected));

    if (const auto& breadcrumbItemButton = m_breadcrumbItemButton.get())
    {
        m_breadcrumbItemButtonLoadedRevoker = breadcrumbItemButton.Loaded(winrt::auto_revoke, { this, &BreadcrumbItem::OnLoadedEvent });

        m_isPressedButtonRevoker = RegisterPropertyChanged(breadcrumbItemButton, winrt::ButtonBase::IsPressedProperty(), { this, &BreadcrumbItem::OnVisualPropertyChanged });
        m_isPointerOverButtonRevoker = RegisterPropertyChanged(breadcrumbItemButton, winrt::ButtonBase::IsPointerOverProperty(), { this, &BreadcrumbItem::OnVisualPropertyChanged });
        m_isEnabledButtonRevoker = RegisterPropertyChanged(breadcrumbItemButton, winrt::Control::IsEnabledProperty(), { this, &BreadcrumbItem::OnVisualPropertyChanged });
    }

    UpdateCommonVisualState(false /*useTransitions*/);
    UpdateItemTypeVisualState(false /*useTransitions*/);
}

void BreadcrumbItem::OnLoadedEvent(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    if (auto breadcrumbItemButton = m_breadcrumbItemButton.get())
    {
        m_breadcrumbItemButtonClickRevoker.revoke();
        if (m_isEllipsisNode)
        {
            m_breadcrumbItemButtonClickRevoker = breadcrumbItemButton.Click(winrt::auto_revoke, { this, &BreadcrumbItem::OnEllipsisItemClick });
        }
        else
        {
            m_breadcrumbItemButtonClickRevoker = breadcrumbItemButton.Click(winrt::auto_revoke, { this, &BreadcrumbItem::OnBreadcrumbItemClick });
        }
    }

    if (m_isEllipsisNode)
    {
        SetPropertiesForEllipsisNode();
    }
    else if (m_isLastNode)
    {
        SetPropertiesForLastNode();
    }
    else
    {
        ResetVisualProperties();
    }
}

void BreadcrumbItem::SetParentBreadcrumb(const winrt::Breadcrumb& parent)
{
    m_parentBreadcrumb.set(parent);
}

void BreadcrumbItem::SetDropDownItemDataTemplate(const winrt::IInspectable& newDataTemplate)
{
    if (auto const& dataTemplate = newDataTemplate.try_as<winrt::DataTemplate>())
    {
        m_dropDownItemDataTemplate.set(dataTemplate);
    }
    else if (!newDataTemplate)
    {
        m_dropDownItemDataTemplate.set(nullptr);
    }
}

void BreadcrumbItem::SetIndex(const uint32_t index)
{
    m_index = index;
}

void BreadcrumbItem::RaiseItemClickedEvent(const winrt::IInspectable& content, const uint32_t index)
{
    if (const auto& breadcrumb = m_parentBreadcrumb.get())
    {
        auto breadcrumbImpl = winrt::get_self<Breadcrumb>(breadcrumb);
        breadcrumbImpl->RaiseItemClickedEvent(content, index);
    }
}

void BreadcrumbItem::OnBreadcrumbItemClick(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    RaiseItemClickedEvent(Content(), m_index - 1);
}

void BreadcrumbItem::OnFlyoutElementPreparedEvent(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args)
{
    UpdateFlyoutIndex(args.Element(), args.Index());
}

void BreadcrumbItem::OnFlyoutElementIndexChangedEvent(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementIndexChangedEventArgs& args)
{
    UpdateFlyoutIndex(args.Element(), args.NewIndex());
}

void BreadcrumbItem::OnFlowDirectionChanged(winrt::DependencyObject const&, winrt::DependencyProperty const&)
{
    UpdateItemTypeVisualState(true /*useTransitions*/);
}

void BreadcrumbItem::OnChildPreviewKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args)
{
    if (args.Key() == winrt::VirtualKey::Enter || args.Key() == winrt::VirtualKey::Space)
    {
        if (m_isEllipsisNode)
        {
            OnEllipsisItemClick(nullptr, nullptr);
        }
        else
        {
            OnBreadcrumbItemClick(nullptr, nullptr);
        }
        args.Handled(true);
    }
}

void BreadcrumbItem::UpdateFlyoutIndex(const winrt::UIElement& element, const uint32_t index)
{
    if (auto const& itemsRepeater = m_ellipsisItemsRepeater.get())
    {
        if (auto const& itemSourceView = itemsRepeater.ItemsSourceView())
        {
            const uint32_t itemCount = itemSourceView.Count();

            if (const auto& dropDownItemImpl = element.try_as<BreadcrumbDropDownItem>())
            {
                dropDownItemImpl->SetEllipsisBreadcrumbItem(*this);
                dropDownItemImpl->SetIndex(itemCount - index);
            }

            hstring name = s_ellipsisItemAutomationName + winrt::to_hstring(index + 1);
            winrt::AutomationProperties::SetName(element, name);

            element.SetValue(winrt::AutomationProperties::PositionInSetProperty(), box_value(index + 1));
            element.SetValue(winrt::AutomationProperties::SizeOfSetProperty(), box_value(itemCount));
        }
    }
}

winrt::IInspectable BreadcrumbItem::CloneEllipsisItemSource(const winrt::Collections::IVector<winrt::IInspectable>& ellipsisItemsSource)
{
    // A copy of the hidden elements array in BreadcrumbLayout is created
    // to avoid getting a Layout cycle exception
    auto newItemsSource = winrt::make<Vector<winrt::IInspectable>>();

    // The new list contains all the elements in reverse order
    const int itemsSourceSize = ellipsisItemsSource.Size();

    // The itemsSourceSize should always be at least 1 as it must always contain the ellipsis item
    assert(itemsSourceSize > 0);

    for (int i = itemsSourceSize - 1; i >= 0; --i)
    {
        const auto& item = ellipsisItemsSource.GetAt(i);
        newItemsSource.Append(item);
    }

    return newItemsSource;
}

void BreadcrumbItem::OpenFlyout()
{
    if (auto flyout = m_ellipsisFlyout.get())
    {
        if (SharedHelpers::IsFlyoutShowOptionsAvailable())
        {
            winrt::FlyoutShowOptions options{};
            flyout.ShowAt(*this, options);
        }
        else
        {
            flyout.ShowAt(*this);
        }
    }
}

void BreadcrumbItem::CloseFlyout()
{
    if (auto flyout = m_ellipsisFlyout.get())
    {
        flyout.Hide();
    }
}

void BreadcrumbItem::OnVisualPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    UpdateCommonVisualState(true /*useTransitions*/);
}

void BreadcrumbItem::UpdateItemTypeVisualState(bool useTransitions)
{
    const bool isLeftToRight = (FlowDirection() == winrt::FlowDirection::LeftToRight);
    hstring visualStateName;

    if (m_isEllipsisNode)
    {
        if (isLeftToRight)
        {
            visualStateName = s_ellipsisStateName;
        }
        else
        {
            visualStateName = s_ellipsisRTLStateName;
        }
    }
    else if (m_isLastNode)
    {
        visualStateName = s_lastItemStateName;
    }
    else if (isLeftToRight)
    {
        visualStateName = s_defaultStateName;
    }
    else
    {
        visualStateName = s_defaultRTLStateName;
    }

    winrt::VisualStateManager::GoToState(*this, visualStateName, useTransitions);
}

void BreadcrumbItem::UpdateCommonVisualState(bool useTransitions)
{
    if (const auto& breadcrumbItemButton = m_breadcrumbItemButton.get())
    {
        hstring commonVisualStateName = L"";

        // If is last item: place Current as prefix for visual state
        if (m_isLastNode)
        {
            commonVisualStateName = s_currentStateName;
        }

        if (!breadcrumbItemButton.IsEnabled())
        {
            commonVisualStateName = commonVisualStateName + s_disabledStateName;
        }
        else if (breadcrumbItemButton.IsPressed())
        {
            commonVisualStateName = commonVisualStateName + s_pressedStateName;
        }
        else if (breadcrumbItemButton.IsPointerOver())
        {
            commonVisualStateName = commonVisualStateName + s_pointerOverStateName;
        }
        else
        {
            commonVisualStateName = commonVisualStateName + s_normalStateName;
        }

        winrt::VisualStateManager::GoToState(breadcrumbItemButton, commonVisualStateName, useTransitions);
    }
}

void BreadcrumbItem::OnEllipsisItemClick(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    if (const auto& breadcrumb = m_parentBreadcrumb.get())
    {
        if (const auto& breadcrumbImpl = breadcrumb.try_as<Breadcrumb>())
        {
            const auto& hiddenElements = CloneEllipsisItemSource(breadcrumbImpl->HiddenElements());

            if (const auto& dataTemplate = m_dropDownItemDataTemplate.get())
            {
                m_ellipsisElementFactory->UserElementFactory(dataTemplate);
            }

            if (const auto& flyoutRepeater = m_ellipsisItemsRepeater.get())
            {
                flyoutRepeater.ItemsSource(hiddenElements);
            }

            OpenFlyout();
        }
    }
}

void BreadcrumbItem::SetPropertiesForLastNode()
{
    m_isEllipsisNode = false;
    m_isLastNode = true;

    UpdateCommonVisualState(false /*useTransitions*/);
    UpdateItemTypeVisualState(false /*useTransitions*/);
}

void BreadcrumbItem::ResetVisualProperties()
{
    m_isEllipsisNode = false;
    m_isLastNode = false;

    if (const auto& breadcrumbItemButton = m_breadcrumbItemButton.get())
    {
        breadcrumbItemButton.Flyout(nullptr);
    }
    m_ellipsisFlyout.set(nullptr);
    m_ellipsisItemsRepeater.set(nullptr);
    m_ellipsisElementFactory = nullptr;

    UpdateCommonVisualState(false /*useTransitions*/);
    UpdateItemTypeVisualState(false /*useTransitions*/);
}

void BreadcrumbItem::InstantiateFlyout()
{
    // Only if the element has been created visually, instantiate the flyout
    if (const auto& breadcrumbItemButton = m_breadcrumbItemButton.get())
    {
        // Create ItemsRepeater and set the DataTemplate 
        const auto& ellipsisItemsRepeater = winrt::ItemsRepeater();
        ellipsisItemsRepeater.Name(s_ellipsisItemsRepeaterPartName);
        winrt::AutomationProperties::SetName(ellipsisItemsRepeater, s_ellipsisItemsRepeaterAutomationName);
        ellipsisItemsRepeater.HorizontalAlignment(winrt::HorizontalAlignment::Stretch);

        m_ellipsisElementFactory = winrt::make_self<BreadcrumbDropDownElementFactory>();
        ellipsisItemsRepeater.ItemTemplate(*m_ellipsisElementFactory);

        const auto& stackLayout = winrt::StackLayout();
        stackLayout.Orientation(winrt::Controls::Orientation::Vertical);
        ellipsisItemsRepeater.Layout(stackLayout);

        if (const auto& dataTemplate = m_dropDownItemDataTemplate.get())
        {
            m_ellipsisElementFactory->UserElementFactory(dataTemplate);
        }

        m_ellipsisRepeaterElementPreparedRevoker = ellipsisItemsRepeater.ElementPrepared(winrt::auto_revoke, { this, &BreadcrumbItem::OnFlyoutElementPreparedEvent });
        m_ellipsisRepeaterElementIndexChangedRevoker = ellipsisItemsRepeater.ElementIndexChanged(winrt::auto_revoke, { this, &BreadcrumbItem::OnFlyoutElementIndexChangedEvent });

        m_ellipsisItemsRepeater.set(ellipsisItemsRepeater);

        // Create the Flyout and add the ItemsRepeater as content
        const auto& ellipsisFlyout = winrt::Flyout();
        winrt::AutomationProperties::SetName(ellipsisFlyout, s_ellipsisFlyoutAutomationName);
        ellipsisFlyout.Content(ellipsisItemsRepeater);
        ellipsisFlyout.Placement(winrt::FlyoutPlacementMode::Bottom);

        m_ellipsisFlyout.set(ellipsisFlyout);

        // Set the Flyout to the ellipsis button
        breadcrumbItemButton.Flyout(ellipsisFlyout);
    }
}

void BreadcrumbItem::SetPropertiesForEllipsisNode()
{
    m_isEllipsisNode = true;
    m_isLastNode = false;

    InstantiateFlyout();

    UpdateCommonVisualState(false /*useTransitions*/);
    UpdateItemTypeVisualState(false /*useTransitions*/);
}

winrt::AutomationPeer BreadcrumbItem::OnCreateAutomationPeer()
{
    return winrt::make<BreadcrumbItemAutomationPeer>(*this);
}

void BreadcrumbItem::OnClickEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    if (m_isEllipsisNode)
    {
        OnEllipsisItemClick(nullptr, nullptr);
    }
    else
    {
        OnBreadcrumbItemClick(nullptr, nullptr);
    }
}
