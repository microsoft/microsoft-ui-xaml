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
    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement
    m_breadcrumbItemButton.set(GetTemplateChildT<winrt::Button>(L"BreadcrumbItemButton", controlProtected));
    m_flyout.set(GetTemplateChildT<winrt::FlyoutBase>(L"EllipsisButtonFlyout", controlProtected));
    m_flyoutRepeater.set(GetTemplateChildT<winrt::ItemsRepeater>(L"FlyoutRepeater", controlProtected));

    if (auto breadcrumbItemButton = m_breadcrumbItemButton.get())
    {
        m_breadcrumbItemButtonLoadedRevoker = breadcrumbItemButton.Loaded(winrt::auto_revoke, { this, &BreadcrumbItem::OnLoadedEvent });
    }
}

void BreadcrumbItem::OnLoadedEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    // m_rootGrid.set(winrt::VisualTreeHelper::GetChild(m_breadcrumbItemButton.get(), 0).as<winrt::Grid>());

    if (auto rootGrid = m_rootGrid.get())
    {
        m_secondaryButtonGrid.set(winrt::VisualTreeHelper::GetChild(m_rootGrid.get(), 1).as<winrt::Grid>());
        m_splitButtonBorder.set(winrt::VisualTreeHelper::GetChild(m_rootGrid.get(), 2).as<winrt::Grid>());
        m_primaryButton.set(winrt::VisualTreeHelper::GetChild(m_rootGrid.get(), 3).as<winrt::Button>());
        m_secondaryButton.set(winrt::VisualTreeHelper::GetChild(m_rootGrid.get(), 4).as<winrt::Button>());

        if (auto secondaryButton = m_secondaryButton.get())
        {
            secondaryButton.IsEnabled(false);
        }
    }

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

    if (const auto& flyoutRepeater = m_flyoutRepeater.get())
    {
        m_flyoutRepeaterElementPreparedRevoker = flyoutRepeater.ElementPrepared(winrt::auto_revoke, { this, &BreadcrumbItem::OnFlyoutElementPreparedEvent });
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
        m_flyoutDataTemplate.set(dataTemplate);
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
    if (auto flyout = m_flyout.get())
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
    if (auto flyout = m_flyout.get())
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

            if (const auto& flyoutRepeater = m_flyoutRepeater.get())
            {
                flyoutRepeater.ItemTemplate(m_flyoutDataTemplate.get());
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

    m_breadcrumbItemButton.get().Flyout(nullptr);

    winrt::VisualStateManager::GoToState(*this, L"Normal", false);
}

void BreadcrumbItem::InstantiateFlyout()
{
    /*
    const auto& ellipsisFlyout = winrt::make<Flyout>();

    const auto& loadedFlyout = winrt::XamlReader::Load(
        L"<Flyout Placement='Bottom' x:Name='EllipsisButtonFlyout'                      \
           xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'            \
           xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'                       \
           xmlns:local='using:Microsoft.UI.Xaml.Controls'>                              \
                                                                  \
        </Flyout>");

    const auto& loadedItemsRepeater = winrt::XamlReader::Load(
        L"<local:ItemsRepeater x:Name='FlyoutRepeater' HorizontalAlignment='Stretch' \
           xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'         \
           xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'                    \
           xmlns:local='using:Microsoft.UI.Xaml.Controls'>                           \
            <local:ItemsRepeater.ItemTemplate>                                       \
                <DataTemplate>                                                       \
                    <Button Content='{Binding}' Background='Transparent' / >         \
                </DataTemplate>                                                     \
            </local:ItemsRepeater.ItemTemplate>                                     \
          </local:ItemsRepeater>");
          */
}

void BreadcrumbItem::SetPropertiesForEllipsisNode()
{
    m_isEllipsisNode = true;
    m_isLastNode = false;

    

    // m_breadcrumbItemButton.get().Flyout(loadedFlyout.as<winrt::FlyoutBase>());

    winrt::VisualStateManager::GoToState(*this, L"Ellipsis", false);
}
