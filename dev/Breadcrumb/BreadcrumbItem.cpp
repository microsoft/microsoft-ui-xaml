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
    m_splitButtonLoadedRevoker.revoke();
}

void BreadcrumbItem::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement
    m_splitButton.set(GetTemplateChildT<winrt::SplitButton>(L"SplitButton", controlProtected));
    m_flyout.set(GetTemplateChildT<winrt::FlyoutBase>(L"SplitButtonFlyout", controlProtected));
    m_flyoutRepeater.set(GetTemplateChildT<winrt::ItemsRepeater>(L"FlyoutRepeater", controlProtected));

    if (auto splitButton = m_splitButton.get())
    {
        m_splitButtonLoadedRevoker = splitButton.Loaded(winrt::auto_revoke, { this, &BreadcrumbItem::OnLoadedEvent });
    }
}

void BreadcrumbItem::OnLoadedEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    m_rootGrid.set(winrt::VisualTreeHelper::GetChild(m_splitButton.get(), 0).as<winrt::Grid>());

    if (auto rootGrid = m_rootGrid.get())
    {
        m_secondaryButtonGrid.set(winrt::VisualTreeHelper::GetChild(m_rootGrid.get(), 1).as<winrt::Grid>());
        m_splitButtonBorder.set(winrt::VisualTreeHelper::GetChild(m_rootGrid.get(), 2).as<winrt::Grid>());
        m_primaryButton.set(winrt::VisualTreeHelper::GetChild(m_rootGrid.get(), 3).as<winrt::Button>());
        m_secondaryButton.set(winrt::VisualTreeHelper::GetChild(m_rootGrid.get(), 4).as<winrt::Button>());

        if (auto secondaryButton = m_secondaryButton.get())
        {
            secondaryButton.IsEnabled(false);
            SetSecondaryButtonText(true);
            SetSecondaryButtonVisibility(m_isChevronVisible);
        }
    }

    if (auto splitButton = m_splitButton.get())
    {
        m_splitButtonClickRevoker.revoke();
        if (m_isEllipsisNode)
        {
            m_splitButtonClickRevoker = splitButton.Click(winrt::auto_revoke, { this, &BreadcrumbItem::OnEllipsisItemClick });
        }
        else
        {
            m_splitButtonClickRevoker = splitButton.Click(winrt::auto_revoke, { this, &BreadcrumbItem::OnBreadcrumbItemClick });
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

void BreadcrumbItem::OnBreadcrumbItemClick(const winrt::IInspectable& sender, const winrt::SplitButtonClickEventArgs& args)
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

    // m_pointerPressedRevoker = element.PointerPressed(winrt::auto_revoke, { this, &BreadcrumbItem::OnFlyoutElementClickEvent });
    
    if (const auto& button = element.try_as<winrt::ButtonBase>())
    {
        button.Click({ this, &BreadcrumbItem::OnFlyoutElementClickEvent });
    }
    
}

void BreadcrumbItem::OnFlyoutElementClickEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    if (const auto& breadcrumb = m_parentBreadcrumb.get())
    {
        const auto& breadcrumbImpl = winrt::get_self<Breadcrumb>(breadcrumb);
        const auto& senderAsContentControl = sender.try_as<winrt::ContentControl>();
        breadcrumbImpl->RaiseItemClickedEvent(senderAsContentControl.Content());

        // this should also update the hidden elements
        if (const auto& splitButton = m_splitButton.get())
        {
            if (const auto& splitButtonImpl = winrt::get_self<SplitButton>(splitButton))
            {
                splitButtonImpl->CloseFlyout();
            }
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
    for (int i = itemsSourceSize - 1; i >= 0; --i)
    {
        const auto& item = ellipsisItemsSource.GetAt(i);
        newItemsSource.Append(item);
    }

    return newItemsSource;
}

void BreadcrumbItem::OnEllipsisItemClick(const winrt::IInspectable& sender, const winrt::SplitButtonClickEventArgs& args)
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

            if (const auto& splitButton = m_splitButton.get())
            {
                if (const auto& splitButtonImpl = winrt::get_self<SplitButton>(splitButton))
                {
                    splitButtonImpl->OpenFlyout();
                }
            }
        }
    }
}

void BreadcrumbItem::SetPropertiesForLastNode()
{
    m_isEllipsisNode = false;
    m_isLastNode = true;

    SetPrimaryButtonBoldFontWeight(true);
    SetSecondaryButtonVisibility(false);
}

void BreadcrumbItem::ResetVisualProperties()
{
    m_isEllipsisNode = false;
    m_isLastNode = false;

    if (auto secondaryButton = m_secondaryButton.get())
    {
        SetPrimaryButtonBoldFontWeight(false);
        SetSecondaryButtonVisibility(true);
        SetSecondaryButtonText(true);
    }
}

void BreadcrumbItem::SetPropertiesForEllipsisNode()
{
    m_isEllipsisNode = true;
    m_isLastNode = false;

    if (auto primaryButton = m_primaryButton.get())
    {
        primaryButton.IsEnabled(true);
    }

    if (auto secondaryButton = m_secondaryButton.get())
    {
        SetPrimaryButtonBoldFontWeight(false);
        SetSecondaryButtonVisibility(true);
    }
}

void BreadcrumbItem::SetPrimaryButtonBoldFontWeight(bool mustBeBold)
{
    if (auto primaryButton = m_primaryButton.get())
    {
        primaryButton.FontWeight(mustBeBold ? winrt::FontWeights::Bold() : winrt::FontWeights::Normal());
    }
}

void BreadcrumbItem::SetSecondaryButtonVisibility(bool isVisible)
{
    m_isChevronVisible = isVisible;

    if (auto secondaryButton = m_secondaryButton.get())
    {
        secondaryButton.Visibility(m_isChevronVisible ? winrt::Visibility::Visible : winrt::Visibility::Collapsed);
    }

    if (auto secondaryButtonGrid = m_secondaryButtonGrid.get())
    {
        secondaryButtonGrid.Visibility(m_isChevronVisible ? winrt::Visibility::Visible : winrt::Visibility::Collapsed);
    }

    if (auto rootGrid = m_rootGrid.get())
    {
        int columnDefinitionCount = rootGrid.ColumnDefinitions().Size();

        auto lastColumnDefinition = rootGrid.ColumnDefinitions().GetAt(columnDefinitionCount - 1);

        if (lastColumnDefinition.Width().Value != 0.0)
        {
            m_chevronOriginalWidth = lastColumnDefinition.Width();
        }

        winrt::GridLength hiddenGridLength;
        hiddenGridLength.GridUnitType = winrt::GridUnitType::Pixel;
        hiddenGridLength.Value = 0.0;

        lastColumnDefinition.Width(isVisible ? m_chevronOriginalWidth : hiddenGridLength);

        if (auto splitButtonBorder = m_splitButtonBorder.get())
        {
            winrt::Grid::SetColumnSpan(splitButtonBorder, columnDefinitionCount - 1);
        }
    }
}

void BreadcrumbItem::SetSecondaryButtonText(bool isCollapsed)
{
    if (auto secondaryButton = m_secondaryButton.get())
    {
        winrt::TextBlock secondaryButtonContent = secondaryButton.Content().as<winrt::TextBlock>();

        secondaryButton.IsEnabled(false);
        if (secondaryButtonContent)
        {
            secondaryButtonContent.Text(isCollapsed ? L"\xE76C" : L"\xE70D");
        }
    }
}
