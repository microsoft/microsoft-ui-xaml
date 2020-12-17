// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Vector.h"
#include "Breadcrumb.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

#include "BreadcrumbItem.h"
#include "BreadcrumbLayout.h"
#include "BreadcrumbItemClickedEventArgs.h"

Breadcrumb::Breadcrumb()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_Breadcrumb);

    SetDefaultStyleKey(this);
    m_breadcrumbElementFactory = winrt::make_self<BreadcrumbElementFactory>();
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
        breadcrumbItemsRepeater.ItemsSource(winrt::make<Vector<IInspectable>>());
        breadcrumbItemsRepeater.ItemTemplate(*m_breadcrumbElementFactory);
        
        m_itemRepeaterElementPreparedRevoker = breadcrumbItemsRepeater.ElementPrepared(winrt::auto_revoke, { this, &Breadcrumb::OnElementPreparedEvent });
        m_itemRepeaterElementClearingRevoker = breadcrumbItemsRepeater.ElementClearing(winrt::auto_revoke, { this, &Breadcrumb::OnElementClearingEvent });

        m_breadcrumbItemRepeaterLoadedRevoker = breadcrumbItemsRepeater.Loaded(winrt::auto_revoke, { this, &Breadcrumb::OnBreadcrumbItemRepeaterLoaded });
    }

    UpdateItemsSource();
}

void Breadcrumb::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    // TODO: Implement
    if (property == s_ItemsSourceProperty)
    {
        UpdateItemsSource();
    }
    else if (property == s_ItemTemplateProperty)
    {
        UpdateItemTemplate();
    }
    else if (property == s_DropdownItemTemplateProperty)
    {
        UpdateDropdownItemTemplate();
    }
}

void Breadcrumb::OnBreadcrumbItemRepeaterLoaded(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    if (auto const breadcrumbItemRepeater = m_breadcrumbItemRepeater.get())
    {
        OnRepeaterCollectionChanged(nullptr, nullptr);
    }
}

void Breadcrumb::UpdateItemTemplate()
{
    const winrt::IInspectable& newItemTemplate = ItemTemplate();
    m_breadcrumbElementFactory->UserElementFactory(newItemTemplate);
}

void Breadcrumb::UpdateDropdownItemTemplate()
{
    const winrt::IInspectable& newItemTemplate = DropdownItemTemplate();

    // Copy the item template to the ellipsis button too
    if (const auto& ellipsisBreadcrumbItem = m_ellipsisBreadcrumbItem.get())
    {
        if (const auto& itemImpl = winrt::get_self<BreadcrumbItem>(ellipsisBreadcrumbItem))
        {
            itemImpl->SetFlyoutDataTemplate(newItemTemplate);
        }
    }
}

void Breadcrumb::UpdateItemsSource()
{
    m_itemsSourceChanged.revoke();
    m_itemsSourceChanged2.revoke();

    auto const itemsSource = this->ItemsSource();
    m_breadcrumbItemsRepeaterItemsSource = winrt::ItemsSourceView(itemsSource);
    m_itemsSourceChanged = m_breadcrumbItemsRepeaterItemsSource.CollectionChanged(winrt::auto_revoke, { this, &Breadcrumb::OnRepeaterCollectionChanged });

    if (const auto breadcrumbItemRepeater = m_breadcrumbItemRepeater.get())
    {
        if (const auto itemsSource = this->ItemsSource())
        {
            const auto incc = [this]() {
                if (this->ItemsSource())
                {
                    return this->ItemsSource().try_as<winrt::INotifyCollectionChanged>();
                }
                else
                {
                    return this->ItemsSource().try_as<winrt::INotifyCollectionChanged>();
                }
            }();

            if (incc)
            {
                m_eventToken = incc.CollectionChanged({ this, &Breadcrumb::OnRepeaterCollectionChanged });
                m_notifyCollectionChanged.set(incc);
            }

            if (auto collection = itemsSource.try_as<winrt::IObservableVector<IInspectable>>())
            {
                m_itemsSourceChanged2 = collection.VectorChanged(winrt::auto_revoke, { this, &Breadcrumb::OnRepeaterCollectionChanged });
            }
        }
    }
}

void Breadcrumb::OnRepeaterCollectionChanged(const winrt::IInspectable&, const winrt::IInspectable& args)
{
    if (const auto& breadcrumbItemRepeater = m_breadcrumbItemRepeater.get())
    {
        // breadcrumbItemRepeater.ItemsSource(winrt::make<Vector<IInspectable>>());
        // breadcrumbItemRepeater.UpdateLayout();

        auto newItemsSource = GenerateInternalItemsSource();
        breadcrumbItemRepeater.ItemsSource(newItemsSource);

        breadcrumbItemRepeater.UpdateLayout();
    }

    return;
}

void Breadcrumb::OnElementPreparedEvent(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args)
{
    if (auto item = args.Element().try_as<winrt::BreadcrumbItem>())
    {
        const uint32_t itemIndex = args.Index();

        auto itemsSourceAsList = m_breadcrumbItemRepeater.get().ItemsSource().as<winrt::Collections::IVector<winrt::IInspectable>>();
        const uint32_t itemCount = itemsSourceAsList.Size();

        auto itemImpl = winrt::get_self<BreadcrumbItem>(item);
        itemImpl->SetItemsRepeater(*this);

        if (itemIndex == 0)
        {
            itemImpl->SetPropertiesForEllipsisNode();
            itemImpl->SetFlyoutDataTemplate(DropdownItemTemplate());

            m_ellipsisBreadcrumbItem.set(item);
        }
        else
        {
            if (itemIndex == (itemCount - 1))
            {
                if (const auto& lastItem = m_lastBreadcrumbItem.get())
                {
                    auto lastItemImpl = winrt::get_self<BreadcrumbItem>(lastItem);
                    lastItemImpl->ResetVisualProperties();
                }

                itemImpl->SetPropertiesForLastNode();
                m_lastBreadcrumbItem.set(item);
            }
            else
            {
                itemImpl->ResetVisualProperties();
            }
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

winrt::IInspectable Breadcrumb::GenerateInternalItemsSource()
{
    auto itemsSource = this->ItemsSource();

    auto newItemsSource = winrt::make<Vector<IInspectable>>();

    auto ellipsisItem = winrt::make<BreadcrumbItem>();
    ellipsisItem.Content(winrt::box_value(L"..."));

    newItemsSource.Append(ellipsisItem);

    if (auto itemsSourceAsList = itemsSource.try_as<winrt::Collections::IVector<winrt::IInspectable>>())
    {
        for (const auto item : itemsSourceAsList)
        {
            newItemsSource.Append(item);
        }
    }

    return newItemsSource;
}


void Breadcrumb::RaiseItemClickedEvent(const winrt::IInspectable& content)
{
    auto eventArgs = winrt::make_self<BreadcrumbItemClickedEventArgs>();
    eventArgs->Item(content);
    m_itemClickedEventSource(*this, *eventArgs);
}

winrt::Collections::IVector<winrt::IInspectable> Breadcrumb::HiddenElements()
{
    if (const auto& breadcrumbItemRepeater = m_breadcrumbItemRepeater.get())
    {
        if (const auto& breadcrumbLayout = breadcrumbItemRepeater.Layout().try_as<BreadcrumbLayout>())
        {
            return breadcrumbLayout->HiddenElements();
        }
    }

    return winrt::make<Vector<winrt::IInspectable>>();
}
