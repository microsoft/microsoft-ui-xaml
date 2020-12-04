// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Breadcrumb.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

#include "BreadcrumbItem.h"

Breadcrumb::Breadcrumb()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_Breadcrumb);

    SetDefaultStyleKey(this);
}

void Breadcrumb::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement
    m_breadcrumbItemRepeater.set(GetTemplateChildT<winrt::ItemsRepeater>(L"BreadcrumbItemRepeater", controlProtected));

    // This has to go into the layout sample
    /*
    m_ellipsisButton = GetTemplateChild("PART_EllipsisButton") as Windows.UI.Xaml.Controls.SplitButton;
    (BreadCrumbItemsRepeater.Layout as BreadCrumbLayout).EllipsisButton = m_ellipsisButton;
    m_ellipsisButton.Click += EllipsisButton_Click;
    m_ellipsisButton.Loaded += M_ellipsisButton_Loaded;

    Microsoft.UI.Xaml.Controls.ItemsRepeater ellipsisFlyoutItemsRepeater = (m_ellipsisButton.Flyout as Flyout).Content as Microsoft.UI.Xaml.Controls.ItemsRepeater;
    ellipsisFlyoutItemsRepeater.ElementPrepared += EllipsisFlyoutItemsRepeater_ElementPrepared;
    */

    if (auto breadcrumbItemsRepeater = m_breadcrumbItemRepeater.get())
    {
        m_itemRepeaterElementPreparedRevoker = breadcrumbItemsRepeater.ElementPrepared(winrt::auto_revoke, { this, &Breadcrumb::OnElementPreparedEvent });
        m_itemRepeaterElementClearingRevoker = breadcrumbItemsRepeater.ElementClearing(winrt::auto_revoke, { this, &Breadcrumb::OnElementClearingEvent });
    }

}

void Breadcrumb::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    // TODO: Implement
}

void Breadcrumb::OnElementPreparedEvent(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args)
{
    if (auto item = args.Element().try_as<winrt::BreadcrumbItem>())
    {
        auto itemImpl = winrt::get_self<BreadcrumbItem>(item);
        // item.m_parentContainer = this;
        item.ContentTemplate(this->ItemTemplate().as<winrt::DataTemplate>());

        //node.GotFocus += Node_GotFocus;
        auto itemsSourceAsList = sender.ItemsSource().as<winrt::Collections::IVector<winrt::IInspectable>>();
        if ((uint32_t)args.Index() == itemsSourceAsList.Size() - 1)
        {
            // LastItem = node; // do I need this
            itemImpl->SetPropertiesForLastNode();
        }
        else
        {
            itemImpl->ResetVisualProperties();
        }
    }
}

void Breadcrumb::OnElementClearingEvent(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementClearingEventArgs args)
{
    if (auto item = args.Element().try_as<winrt::BreadcrumbItem>())
    {
        auto const itemImpl = winrt::get_self<BreadcrumbItem>(item);
        itemImpl->ResetVisualProperties();
        itemImpl->RevokeListeners();
    }
}
