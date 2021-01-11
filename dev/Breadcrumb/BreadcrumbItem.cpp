// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "BreadcrumbItem.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

#include "ItemTemplateWrapper.h"
#include "Breadcrumb.h"

namespace winrt::Microsoft::UI::Xaml::Controls
{
    CppWinRTActivatableClassWithBasicFactory(BreadcrumbItem)
}

#include "BreadcrumbItem.g.cpp"

BreadcrumbItem::BreadcrumbItem()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_BreadcrumbItem);

    SetDefaultStyleKey(this);
}

BreadcrumbItem::~BreadcrumbItem()
{
    RevokeListeners();
}

void BreadcrumbItem::RevokeListeners()
{
    m_breadcrumbItemButtonLoadedRevoker.revoke();
}

void BreadcrumbItem::OnApplyTemplate()
{
    __super::OnApplyTemplate();

    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement
    m_breadcrumbItemButton.set(GetTemplateChildT<winrt::Button>(L"PART_BreadcrumbItemButton", controlProtected));

    if (const auto& breadcrumbItemButton = m_breadcrumbItemButton.get())
    {
        m_breadcrumbItemButtonLoadedRevoker = breadcrumbItemButton.Loaded(winrt::auto_revoke, { this, &BreadcrumbItem::OnLoadedEvent });
    }
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

    if (const auto& itemsRepeater = m_ellipsisItemsRepeater.get())
    {
        m_flyoutRepeaterElementPreparedRevoker = itemsRepeater.ElementPrepared(winrt::auto_revoke, { this, &BreadcrumbItem::OnFlyoutElementPreparedEvent });
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

void BreadcrumbItem::SetItemsRepeater(const winrt::Breadcrumb& parent)
{
    m_parentBreadcrumb.set(parent);
}

void BreadcrumbItem::SetFlyoutDataTemplate(const winrt::IInspectable& newDataTemplate)
{
    if (auto const& dataTemplate = newDataTemplate.try_as<winrt::DataTemplate>())
    {
        m_ellipsisDataTemplate.set(dataTemplate);
    }
}

void BreadcrumbItem::OnBreadcrumbItemClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    if (const auto& breadcrumb = m_parentBreadcrumb.get())
    {
        auto breadcrumbImpl = winrt::get_self<Breadcrumb>(breadcrumb);
        breadcrumbImpl->RaiseItemClickedEvent(Content());
    }
}

void BreadcrumbItem::OnFlyoutElementPreparedEvent(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args)
{
    const auto& element = args.Element();

    element.AddHandler(winrt::UIElement::PointerPressedEvent(),
        winrt::box_value<winrt::PointerEventHandler>({this, &BreadcrumbItem::OnFlyoutElementClickEvent}),
        true);
    
    element.PreviewKeyDown({ this, &BreadcrumbItem::OnFlyoutElementKeyDownEvent });    
}

void BreadcrumbItem::OnFlyoutElementKeyDownEvent(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args)
{
    if (args.Key() == winrt::VirtualKey::Enter)
    {
        this->OnFlyoutElementClickEvent(sender, nullptr);
        args.Handled(true);
    }
    else
    {
        args.Handled(false);
    }
}

void BreadcrumbItem::OnFlyoutElementClickEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs&)
{
    if (const auto& breadcrumb = m_parentBreadcrumb.get())
    {
        const auto& breadcrumbImpl = winrt::get_self<Breadcrumb>(breadcrumb);
        const auto& senderAsContentControl = sender.try_as<winrt::ContentControl>();

        // Once an element has been clicked, close the flyout
        CloseFlyout();

        breadcrumbImpl->RaiseItemClickedEvent(senderAsContentControl.Content());
    }
}

winrt::IInspectable BreadcrumbItem::CloneEllipsisItemSource(const winrt::Collections::IVector<winrt::IInspectable>& ellipsisItemsSource)
{
    // A copy of the hidden elements array in BreadcrumbLayout is created
    // to avoid getting a Layout cycle exception
    auto newItemsSource = winrt::make<Vector<winrt::IInspectable>>();

    // The new list contains all the elements in reverse order
    const int itemsSourceSize = ellipsisItemsSource.Size();
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

void BreadcrumbItem::OnEllipsisItemClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    if (const auto& breadcrumb = m_parentBreadcrumb.get())
    {
        if (const auto& breadcrumbImpl = breadcrumb.try_as<Breadcrumb>())
        {
            const auto& hiddenElements = CloneEllipsisItemSource(breadcrumbImpl->HiddenElements());

            if (const auto& flyoutRepeater = m_ellipsisItemsRepeater.get())
            {
                flyoutRepeater.ItemTemplate(m_ellipsisDataTemplate.get());
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

    winrt::VisualStateManager::GoToState(*this, L"LastNode", false);
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

    winrt::VisualStateManager::GoToState(*this, L"Normal", false);
}

void BreadcrumbItem::InstantiateFlyout()
{
    // Only if the element has been created visually, instantiate the flyout
    if (const auto& breadcrumbItemButton = m_breadcrumbItemButton.get())
    {
        // Load the DataTemplate for the ItemsRepeater
        const auto& loadedDataTemplate = winrt::XamlReader::Load(
            L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> \
                <Button Content='{Binding}' Background='Transparent' />                        \
              </DataTemplate>");

        // Create ItemsRepeater and set the DataTemplate 
        const auto& ellipsisItemsRepeater = winrt::ItemsRepeater();
        ellipsisItemsRepeater.HorizontalAlignment(winrt::HorizontalAlignment::Stretch);
        ellipsisItemsRepeater.ItemTemplate(loadedDataTemplate);
        m_ellipsisItemsRepeater.set(ellipsisItemsRepeater);

        // Create the Flyout and add the ItemsRepeater as content
        const auto& ellipsisFlyout = winrt::Flyout();
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

    winrt::VisualStateManager::GoToState(*this, L"Ellipsis", false);
}
