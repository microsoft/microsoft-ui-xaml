// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Vector.h"
#include "Breadcrumb.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

#include "BreadcrumbItem.h"

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
        breadcrumbItemsRepeater.ItemTemplate(*m_breadcrumbElementFactory);

        m_itemRepeaterElementPreparedRevoker = breadcrumbItemsRepeater.ElementPrepared(winrt::auto_revoke, { this, &Breadcrumb::OnElementPreparedEvent });
        m_itemRepeaterElementClearingRevoker = breadcrumbItemsRepeater.ElementClearing(winrt::auto_revoke, { this, &Breadcrumb::OnElementClearingEvent });
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
    /*
    else if (property == s_SelectedIndexProperty)
    {
        UpdateSelectedIndex();
    }
    else if (property == s_SelectedItemProperty)
    {
        UpdateSelectedItem();
    }*/
    else if (property == s_ItemTemplateProperty)
    {
        UpdateItemTemplate();
    }
}

void Breadcrumb::OnElementPreparedEvent(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args)
{
    if (auto item = args.Element().try_as<winrt::BreadcrumbItem>())
    {
        int itemIndex = args.Index();

        auto itemImpl = winrt::get_self<BreadcrumbItem>(item);
        // item.m_parentContainer = this;

        auto itemsSourceAsList = this->ItemsSource().as<winrt::Collections::IVector<winrt::IInspectable>>();
        if (itemIndex == 0)
        {
            auto content = item.Content();
            auto firstElement = itemsSourceAsList.GetAt(0);
            item.Content(firstElement);
        }

        item.ContentTemplate(this->ItemTemplate().as<winrt::DataTemplate>());

        //node.GotFocus += Node_GotFocus;
        
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

void Breadcrumb::UpdateItemTemplate()
{
    m_breadcrumbElementFactory->UserElementFactory(ItemTemplate());
}

void Breadcrumb::UpdateItemsSource()
{
    m_itemsSourceChanged.revoke();
    m_itemsSourceChanged2.revoke();

    if (auto const breadcrumbItemRepeater = m_breadcrumbItemRepeater.get())
    {
        breadcrumbItemRepeater.ItemsSource(GenerateInternalItemsSource());

        if (auto const itemsSource = this->ItemsSource())
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
            
            /*
            if (auto collection = itemsSource.as<winrt::ObservableCollection<IInspectable>>())
            {
                m_itemsSourceChanged2 = collection.VectorChanged(winrt::auto_revoke, { this, &Breadcrumb::OnRepeaterCollectionChanged });
            }
            */
        }

        if (auto const itemsSourceView = breadcrumbItemRepeater.ItemsSourceView())
        {
            m_itemsSourceChanged = itemsSourceView.CollectionChanged(winrt::auto_revoke, { this, &Breadcrumb::OnRepeaterCollectionChanged });
        }
    }
}

void Breadcrumb::OnRepeaterCollectionChanged(const winrt::IInspectable&, const winrt::IInspectable& args)
{
    if (auto evento = args.try_as<winrt::NotifyCollectionChangedEventArgs>())
    {
        UpdateItemsSource();
    }

    return;
    /*
    if (auto const breadcrumbItemRepeater = m_breadcrumbItemRepeater.get())
    {
        if (auto const itemSourceView = breadcrumbItemRepeater.ItemsSourceView())
        {
            auto const count = itemSourceView.Count();
            for (auto index = 0; index < count; index++)
            {
                if (auto const element = breadcrumbItemRepeater.TryGetElement(index))
                {
                    element.SetValue(winrt::AutomationProperties::SizeOfSetProperty(), box_value(count));
                }
            }
        }
    }
    */
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
