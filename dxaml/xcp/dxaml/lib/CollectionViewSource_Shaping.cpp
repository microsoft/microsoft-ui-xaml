// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Experimental WPF-style shaping (sort, filter, group) for CollectionViewSource. The CollectionViewSource is
//      a proxy: its rules are applied to View, a VectorCollectionView that owns the engine
//      (VectorCollectionView_Shaping.cpp). See docs\design-notes\CollectionViewSourceShaping-POC.md.

#include "precomp.h"
#include "CollectionViewSource.g.h"
#include "SortDescription.g.h"
#include "PropertyGroupDescription.g.h"
#include "VectorCollectionView.g.h"
#include "FilterEventArgs.g.h"
#include "CollectionViewManager.h"
#include "CollectionViewRefreshDeferral.h"
#include "CollectionViewLiveShapingProperties.h"

using namespace DirectUI;

namespace
{
    // Descriptor collection shared by a CollectionViewSource and its shaping view. A change is reported to the
    // view while there is one, else to the CollectionViewSource. A group description's SortDescriptions report
    // to the group description.
    template <typename T>
    class __declspec(novtable) ShapingDescriptionCollection : public ObservableTrackerCollection<T>
    {
    public:
        _Check_return_ HRESULT SetOwner(_In_ CollectionViewSource* pOwner)
        {
            m_ownerKind = OwnerKind::Source;
            return ctl::AsWeak(pOwner, &m_ownerRef);
        }

        _Check_return_ HRESULT SetOwner(_In_ VectorCollectionView* pOwner)
        {
            m_ownerKind = OwnerKind::View;
            return ctl::AsWeak(pOwner, &m_ownerRef);
        }

        _Check_return_ HRESULT SetOwner(_In_ PropertyGroupDescription* pOwner)
        {
            m_ownerKind = OwnerKind::GroupDescription;
            return ctl::AsWeak(pOwner, &m_ownerRef);
        }

        void SetIsSortDescriptions()
        {
            m_isSortDescriptions = true;
        }

    protected:
        ShapingDescriptionCollection() = default;

        _Check_return_ HRESULT RaiseVectorChanged(_In_ wfc::CollectionChange action, UINT index) override
        {
            IFC_RETURN(ObservableTrackerCollection<T>::RaiseVectorChanged(action, index));

            if (!m_ownerRef)
            {
                return S_OK;
            }

            if (m_ownerKind == OwnerKind::GroupDescription)
            {
                ctl::ComPtr<PropertyGroupDescription> spOwner;
                IFC_RETURN(m_ownerRef.As(&spOwner));
                if (spOwner)
                {
                    IFC_RETURN(spOwner->OnSortDescriptionsChanged());
                }
            }
            else if (m_ownerKind == OwnerKind::View)
            {
                ctl::ComPtr<xaml_data::ICollectionView> spView;
                IFC_RETURN(m_ownerRef.As(&spView));
                if (spView)
                {
                    auto pView = static_cast<VectorCollectionView*>(static_cast<CollectionView*>(spView.Get()));
                    IFC_RETURN(m_isSortDescriptions ? pView->OnSortDescriptionsChanged() : pView->OnShapingRulesChanged());
                }
            }
            else
            {
                ctl::ComPtr<xaml_data::ICollectionViewSource> spOwner;
                IFC_RETURN(m_ownerRef.As(&spOwner));
                if (spOwner)
                {
                    auto pOwner = spOwner.Cast<CollectionViewSource>();
                    IFC_RETURN(m_isSortDescriptions ? pOwner->OnSortDescriptionsChanged() : pOwner->OnShapingDescriptionsChanged());
                }
            }

            return S_OK;
        }

    private:
        enum class OwnerKind { Source, View, GroupDescription };

        ctl::WeakRefPtr m_ownerRef;
        OwnerKind m_ownerKind = OwnerKind::Source;
        bool m_isSortDescriptions = false;
    };

    bool IsValidStringComparison(xaml_data::CollectionViewStringComparison value)
    {
        return value >= xaml_data::CollectionViewStringComparison_CurrentCulture && value <= xaml_data::CollectionViewStringComparison_OrdinalIgnoreCase;
    }

    // One shared default view per list source, held weakly (WPF: ViewManager's default views). Views are per
    // thread; a view removes its entry when it is destroyed.
    struct DefaultViewEntry
    {
        IUnknown* pSourceIdentity = nullptr;
        VectorCollectionView* pView = nullptr;
        ctl::WeakRefPtr viewRef;
    };

    thread_local std::vector<DefaultViewEntry> t_defaultViews;

    template <typename T, typename TOwner>
    _Check_return_ HRESULT SetDescriptionsOwner(_In_ wfc::IVector<T>* pCollection, _In_ TOwner* pOwner)
    {
        IFC_RETURN(static_cast<ShapingDescriptionCollection<T>*>(pCollection)->SetOwner(pOwner));
        return S_OK;
    }

    VectorCollectionView* AsShapingView(_In_ xaml_data::ICollectionView* pView)
    {
        return static_cast<VectorCollectionView*>(static_cast<CollectionView*>(pView));
    }
}

_Check_return_ HRESULT SortDescriptionFactory::CreateInstanceImpl(
    _In_opt_ HSTRING propertyName,
    xaml_data::ListSortDirection direction,
    _Outptr_ xaml_data::ISortDescription** ppInstance)
{
    ctl::ComPtr<SortDescription> spDescription;

    IFCEXPECTRC_RETURN(direction == xaml_data::ListSortDirection_Ascending || direction == xaml_data::ListSortDirection_Descending, E_INVALIDARG);

    IFC_RETURN(ctl::make(&spDescription));
    IFC_RETURN(spDescription->put_PropertyName(propertyName));
    IFC_RETURN(spDescription->put_Direction(direction));
    IFC_RETURN(spDescription.CopyTo(ppInstance));
    return S_OK;
}

_Check_return_ HRESULT PropertyGroupDescriptionFactory::CreateInstanceImpl(
    _In_opt_ HSTRING propertyName,
    _Outptr_ xaml_data::IPropertyGroupDescription** ppInstance)
{
    IFC_RETURN(CreateInstanceWithConverterAndStringComparisonImpl(propertyName, nullptr /* pConverter */, xaml_data::CollectionViewStringComparison_Ordinal, ppInstance));
    return S_OK;
}

_Check_return_ HRESULT PropertyGroupDescriptionFactory::CreateInstanceWithConverterImpl(
    _In_opt_ HSTRING propertyName,
    _In_opt_ xaml_data::IValueConverter* pConverter,
    _Outptr_ xaml_data::IPropertyGroupDescription** ppInstance)
{
    IFC_RETURN(CreateInstanceWithConverterAndStringComparisonImpl(propertyName, pConverter, xaml_data::CollectionViewStringComparison_Ordinal, ppInstance));
    return S_OK;
}

_Check_return_ HRESULT PropertyGroupDescriptionFactory::CreateInstanceWithConverterAndStringComparisonImpl(
    _In_opt_ HSTRING propertyName,
    _In_opt_ xaml_data::IValueConverter* pConverter,
    xaml_data::CollectionViewStringComparison stringComparison,
    _Outptr_ xaml_data::IPropertyGroupDescription** ppInstance)
{
    ctl::ComPtr<PropertyGroupDescription> spDescription;

    IFCEXPECTRC_RETURN(IsValidStringComparison(stringComparison), E_INVALIDARG);

    IFC_RETURN(ctl::make(&spDescription));
    IFC_RETURN(spDescription->put_PropertyName(propertyName));
    IFC_RETURN(spDescription->put_Converter(pConverter));
    IFC_RETURN(spDescription->put_StringComparison(stringComparison));
    IFC_RETURN(spDescription.CopyTo(ppInstance));
    return S_OK;
}

_Check_return_ HRESULT PropertyGroupDescription::get_SortDescriptionsImpl(_Outptr_ wfc::IObservableVector<xaml_data::SortDescription*>** ppValue)
{
    if (!m_tpSortDescriptions)
    {
        ctl::ComPtr<ShapingDescriptionCollection<xaml_data::SortDescription*>> spCollection;
        IFC_RETURN(ctl::ComObject<ShapingDescriptionCollection<xaml_data::SortDescription*>>::CreateInstance(spCollection.ReleaseAndGetAddressOf()));
        IFC_RETURN(spCollection->SetOwner(this));
        SetPtrValue(m_tpSortDescriptions, spCollection.Get());
    }

    IFC_RETURN(m_tpSortDescriptions.CopyTo(ppValue));
    return S_OK;
}

_Check_return_ HRESULT PropertyGroupDescription::get_CustomSortImpl(_Outptr_result_maybenull_ xaml_data::ICollectionViewComparer** ppValue)
{
    IFC_RETURN(m_tpCustomSort.CopyTo(ppValue));
    return S_OK;
}

// As in WPF, a CustomSort replaces the SortDescriptions, and adding a SortDescription drops the CustomSort.
_Check_return_ HRESULT PropertyGroupDescription::put_CustomSortImpl(_In_opt_ xaml_data::ICollectionViewComparer* pValue)
{
    UINT sortCount = 0;

    SetPtrValue(m_tpCustomSort, pValue);
    if (m_tpSortDescriptions)
    {
        IFC_RETURN(m_tpSortDescriptions->get_Size(&sortCount));
    }

    if (sortCount > 0)
    {
        m_isApplyingCustomSort = true;
        HRESULT hr = m_tpSortDescriptions->Clear();
        m_isApplyingCustomSort = false;
        IFC_RETURN(hr);
    }

    IFC_RETURN(NotifyOwners());
    return S_OK;
}

_Check_return_ HRESULT PropertyGroupDescription::OnSortDescriptionsChanged()
{
    UINT sortCount = 0;

    if (m_isApplyingCustomSort)
    {
        return S_OK;
    }

    IFC_RETURN(m_tpSortDescriptions->get_Size(&sortCount));
    if (sortCount > 0)
    {
        m_tpCustomSort.Clear();
    }

    IFC_RETURN(NotifyOwners());
    return S_OK;
}

_Check_return_ HRESULT PropertyGroupDescription::AddOwner(_In_ VectorCollectionView* pView)
{
    bool isOwner = false;

    m_owners.erase(std::remove_if(m_owners.begin(), m_owners.end(), [pView, &isOwner](ctl::WeakRefPtr& ownerRef)
    {
        ctl::ComPtr<xaml_data::ICollectionView> spOwner;
        if (FAILED(ownerRef.As(&spOwner)) || !spOwner)
        {
            return true;
        }
        isOwner = isOwner || (static_cast<CollectionView*>(spOwner.Get()) == static_cast<CollectionView*>(pView));
        return false;
    }), m_owners.end());

    if (!isOwner)
    {
        ctl::WeakRefPtr ownerRef;
        IFC_RETURN(ctl::AsWeak(static_cast<CollectionView*>(pView), &ownerRef));
        m_owners.push_back(std::move(ownerRef));
    }

    return S_OK;
}

_Check_return_ HRESULT PropertyGroupDescription::NotifyOwners()
{
    std::vector<ctl::ComPtr<xaml_data::ICollectionView>> owners;

    for (auto& ownerRef : m_owners)
    {
        ctl::ComPtr<xaml_data::ICollectionView> spOwner;
        IFC_RETURN(ownerRef.As(&spOwner));
        if (spOwner)
        {
            owners.push_back(std::move(spOwner));
        }
    }

    for (const auto& spOwner : owners)
    {
        IFC_RETURN(static_cast<VectorCollectionView*>(static_cast<CollectionView*>(spOwner.Get()))->OnGroupDescriptionChanged(this));
    }

    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::EnsureShapingDescriptions()
{
    if (!m_tpSortDescriptions)
    {
        ctl::ComPtr<ShapingDescriptionCollection<xaml_data::SortDescription*>> spCollection;
        IFC_RETURN(ctl::ComObject<ShapingDescriptionCollection<xaml_data::SortDescription*>>::CreateInstance(spCollection.ReleaseAndGetAddressOf()));
        spCollection->SetIsSortDescriptions();
        IFC_RETURN(spCollection->SetOwner(this));
        SetPtrValue(m_tpSortDescriptions, spCollection.Get());
    }

    if (!m_tpGroupDescriptions)
    {
        ctl::ComPtr<ShapingDescriptionCollection<xaml_data::PropertyGroupDescription*>> spCollection;
        IFC_RETURN(ctl::ComObject<ShapingDescriptionCollection<xaml_data::PropertyGroupDescription*>>::CreateInstance(spCollection.ReleaseAndGetAddressOf()));
        IFC_RETURN(spCollection->SetOwner(this));
        SetPtrValue(m_tpGroupDescriptions, spCollection.Get());
    }

    return S_OK;
}

// WPF: CollectionViewSource.ApplyPropertiesToView. The view takes the rules and owns them while it is View.
_Check_return_ HRESULT CollectionViewSource::ApplyPropertiesToView(_In_ VectorCollectionView* pView)
{
    ctl::ComPtr<xaml_data::ICollectionViewFilter> spFilter = m_tpFilter.Get();
    ctl::ComPtr<xaml_data::ICollectionViewComparer> spCustomSort = m_tpCustomSort.Get();

    IFC_RETURN(EnsureShapingDescriptions());
    IFC_RETURN(SetDescriptionsOwner(m_tpSortDescriptions.Get(), pView));
    IFC_RETURN(SetDescriptionsOwner(m_tpGroupDescriptions.Get(), pView));
    SetPtrValue(m_tpShapingView, static_cast<xaml_data::ICollectionView*>(pView));
    m_tpFilter.Clear();
    m_tpCustomSort.Clear();

    IFC_RETURN(pView->EnableShaping(this));
    IFC_RETURN(pView->ApplyLiveShaping(m_isLiveSortingRequested, m_isLiveFilteringRequested, m_isLiveGroupingRequested,
        m_spLiveSortingProperties.Get(), m_spLiveFilteringProperties.Get(), m_spLiveGroupingProperties.Get()));
    IFC_RETURN(pView->ApplyShapingRules(m_tpSortDescriptions.Get(), m_tpGroupDescriptions.Get(), spFilter.Get(), spCustomSort.Get(), m_culture.Get()));
    m_culture.Release();

    if (m_deferRefreshLevel > 0)
    {
        IFC_RETURN(pView->DeferRefresh(m_spViewDeferral.ReleaseAndGetAddressOf()));
    }

    return S_OK;
}

// Takes the rules back from the shaping view before View is replaced.
_Check_return_ HRESULT CollectionViewSource::DetachShapingView()
{
    ctl::ComPtr<xaml_data::ICollectionView> spView = m_tpShapingView.Get();
    ctl::ComPtr<xaml_data::ICollectionViewFilter> spFilter;
    ctl::ComPtr<xaml_data::ICollectionViewComparer> spCustomSort;

    if (!spView)
    {
        return S_OK;
    }

    auto pView = AsShapingView(spView.Get());
    IFC_RETURN(pView->get_Filter(spFilter.ReleaseAndGetAddressOf()));
    IFC_RETURN(pView->get_CustomSort(spCustomSort.ReleaseAndGetAddressOf()));
    m_culture.Release();
    IFC_RETURN(pView->get_Culture(m_culture.GetAddressOf()));
    pView->DetachShaping();

    m_tpShapingView.Clear();
    SetPtrValue(m_tpFilter, spFilter.Get());
    SetPtrValue(m_tpCustomSort, spCustomSort.Get());
    IFC_RETURN(SetDescriptionsOwner(m_tpSortDescriptions.Get(), this));
    IFC_RETURN(SetDescriptionsOwner(m_tpGroupDescriptions.Get(), this));

    if (m_spViewDeferral)
    {
        auto spViewDeferral = std::move(m_spViewDeferral);
        IFC_RETURN(spViewDeferral->Close());
    }

    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::get_SortDescriptionsImpl(_Outptr_ wfc::IObservableVector<xaml_data::SortDescription*>** ppValue)
{
    IFC_RETURN(EnsureShapingDescriptions());
    IFC_RETURN(m_tpSortDescriptions.CopyTo(ppValue));
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::get_GroupDescriptionsImpl(_Outptr_ wfc::IObservableVector<xaml_data::PropertyGroupDescription*>** ppValue)
{
    IFC_RETURN(EnsureShapingDescriptions());
    IFC_RETURN(m_tpGroupDescriptions.CopyTo(ppValue));
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::get_FilterImpl(_Outptr_result_maybenull_ xaml_data::ICollectionViewFilter** ppValue)
{
    if (m_tpShapingView)
    {
        IFC_RETURN(AsShapingView(m_tpShapingView.Get())->get_Filter(ppValue));
        return S_OK;
    }

    IFC_RETURN(m_tpFilter.CopyTo(ppValue));
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::put_FilterImpl(_In_opt_ xaml_data::ICollectionViewFilter* pValue)
{
    if (m_tpShapingView)
    {
        IFC_RETURN(AsShapingView(m_tpShapingView.Get())->put_Filter(pValue));
        return S_OK;
    }

    SetPtrValue(m_tpFilter, pValue);
    IFC_RETURN(OnShapingDescriptionsChanged());
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::get_CustomSortImpl(_Outptr_result_maybenull_ xaml_data::ICollectionViewComparer** ppValue)
{
    if (m_tpShapingView)
    {
        IFC_RETURN(AsShapingView(m_tpShapingView.Get())->get_CustomSort(ppValue));
        return S_OK;
    }

    IFC_RETURN(m_tpCustomSort.CopyTo(ppValue));
    return S_OK;
}

// As in WPF, a CustomSort replaces the SortDescriptions, and adding a SortDescription drops the CustomSort.
_Check_return_ HRESULT CollectionViewSource::put_CustomSortImpl(_In_opt_ xaml_data::ICollectionViewComparer* pValue)
{
    UINT sortCount = 0;

    if (m_tpShapingView)
    {
        IFC_RETURN(AsShapingView(m_tpShapingView.Get())->put_CustomSort(pValue));
        return S_OK;
    }

    SetPtrValue(m_tpCustomSort, pValue);

    if (pValue && m_tpSortDescriptions)
    {
        IFC_RETURN(m_tpSortDescriptions->get_Size(&sortCount));
    }

    if (sortCount > 0)
    {
        m_isApplyingCustomSort = true;
        HRESULT hr = m_tpSortDescriptions->Clear();
        m_isApplyingCustomSort = false;
        IFC_RETURN(hr);
    }

    IFC_RETURN(OnShapingDescriptionsChanged());
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::get_CultureImpl(_Out_ HSTRING* pValue)
{
    if (m_tpShapingView)
    {
        IFC_RETURN(AsShapingView(m_tpShapingView.Get())->get_Culture(pValue));
        return S_OK;
    }

    IFC_RETURN(m_culture.CopyTo(pValue));
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::put_CultureImpl(_In_opt_ HSTRING value)
{
    if (m_tpShapingView)
    {
        IFC_RETURN(AsShapingView(m_tpShapingView.Get())->put_Culture(value));
        return S_OK;
    }

    IFC_RETURN(VectorCollectionView::ValidateCulture(value));
    IFC_RETURN(m_culture.Set(value));
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::get_IsLiveSortingRequestedImpl(_Out_ BOOLEAN* pValue)
{
    *pValue = m_isLiveSortingRequested;
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::put_IsLiveSortingRequestedImpl(BOOLEAN value)
{
    m_isLiveSortingRequested = !!value;
    IFC_RETURN(ApplyLiveShapingToView());
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::get_IsLiveFilteringRequestedImpl(_Out_ BOOLEAN* pValue)
{
    *pValue = m_isLiveFilteringRequested;
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::put_IsLiveFilteringRequestedImpl(BOOLEAN value)
{
    m_isLiveFilteringRequested = !!value;
    IFC_RETURN(ApplyLiveShapingToView());
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::get_IsLiveGroupingRequestedImpl(_Out_ BOOLEAN* pValue)
{
    *pValue = m_isLiveGroupingRequested;
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::put_IsLiveGroupingRequestedImpl(BOOLEAN value)
{
    m_isLiveGroupingRequested = !!value;
    IFC_RETURN(ApplyLiveShapingToView());
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::get_LiveSortingPropertiesImpl(_Outptr_ wfc::IObservableVector<HSTRING>** ppValue)
{
    IFC_RETURN(EnsureLiveShapingProperties());
    IFC_RETURN(m_spLiveSortingProperties.CopyTo(ppValue));
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::get_LiveFilteringPropertiesImpl(_Outptr_ wfc::IObservableVector<HSTRING>** ppValue)
{
    IFC_RETURN(EnsureLiveShapingProperties());
    IFC_RETURN(m_spLiveFilteringProperties.CopyTo(ppValue));
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::get_LiveGroupingPropertiesImpl(_Outptr_ wfc::IObservableVector<HSTRING>** ppValue)
{
    IFC_RETURN(EnsureLiveShapingProperties());
    IFC_RETURN(m_spLiveGroupingProperties.CopyTo(ppValue));
    return S_OK;
}

// WPF: OnForwardedCollectionChanged; a change to a property list is applied to View.
_Check_return_ HRESULT CollectionViewSource::EnsureLiveShapingProperties()
{
    if (m_spLiveSortingProperties)
    {
        return S_OK;
    }

    ctl::WeakRefPtr weakThis;
    IFC_RETURN(ctl::AsWeak(this, &weakThis));
    auto onChanged = [weakThis]() mutable -> HRESULT
    {
        ctl::ComPtr<xaml_data::ICollectionViewSource> spThis;
        IFC_RETURN(weakThis.As(&spThis));
        if (spThis)
        {
            IFC_RETURN(spThis.Cast<CollectionViewSource>()->ApplyLiveShapingToView());
        }
        return S_OK;
    };

    for (auto* pspProperties : { std::addressof(m_spLiveSortingProperties), std::addressof(m_spLiveFilteringProperties), std::addressof(m_spLiveGroupingProperties) })
    {
        wrl::ComPtr<LiveShapingPropertyCollection> spCollection;
        IFC_RETURN(wrl::MakeAndInitialize<LiveShapingPropertyCollection>(&spCollection));
        spCollection->SetChangedCallback(onChanged);
        IFC_RETURN(spCollection.CopyTo(pspProperties->ReleaseAndGetAddressOf()));
    }

    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::ApplyLiveShapingToView()
{
    if (m_tpShapingView)
    {
        IFC_RETURN(AsShapingView(m_tpShapingView.Get())->ApplyLiveShaping(m_isLiveSortingRequested, m_isLiveFilteringRequested, m_isLiveGroupingRequested,
            m_spLiveSortingProperties.Get(), m_spLiveFilteringProperties.Get(), m_spLiveGroupingProperties.Get()));
    }

    return S_OK;
}

#if WI_IS_FEATURE_PRESENT(Feature_ExperimentalApi)
_Check_return_ HRESULT STDMETHODCALLTYPE CollectionViewSourceGenerated::add_FilterRequested(_In_ xaml_data::IFilterEventHandler* pValue, _Out_ EventRegistrationToken* pToken)
{
    IFC_RETURN(CheckThread());
    IFC_RETURN(static_cast<CollectionViewSource*>(this)->AddFilterRequestedHandler(pValue, pToken));
    return S_OK;
}

_Check_return_ HRESULT STDMETHODCALLTYPE CollectionViewSourceGenerated::remove_FilterRequested(EventRegistrationToken token)
{
    IFC_RETURN(CheckThread());
    IFC_RETURN(static_cast<CollectionViewSource*>(this)->RemoveFilterRequestedHandler(token));
    return S_OK;
}
#endif

// As in WPF, adding or removing a handler refilters the view.
_Check_return_ HRESULT CollectionViewSource::AddFilterRequestedHandler(_In_ xaml_data::IFilterEventHandler* pHandler, _Out_ EventRegistrationToken* pToken)
{
    ARG_NOTNULL_RETURN(pHandler, "value");
    ARG_VALIDRETURNPOINTER(pToken);

    if (!m_spFilterRequestedEventSource)
    {
        IFC_RETURN(ctl::ComObject<FilterRequestedEventSource>::CreateInstance(m_spFilterRequestedEventSource.ReleaseAndGetAddressOf()));
    }

    IFC_RETURN(m_spFilterRequestedEventSource->AddHandler(pHandler));
    pToken->value = reinterpret_cast<INT64>(pHandler);
    IFC_RETURN(OnShapingDescriptionsChanged());
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::RemoveFilterRequestedHandler(EventRegistrationToken token)
{
    auto pHandler = reinterpret_cast<xaml_data::IFilterEventHandler*>(token.value);

    if (!m_spFilterRequestedEventSource || !pHandler)
    {
        return S_OK;
    }

    IFC_RETURN(m_spFilterRequestedEventSource->RemoveHandler(pHandler));
    IFC_RETURN(OnShapingDescriptionsChanged());
    return S_OK;
}

bool CollectionViewSource::HasFilterRequestedHandlers() const
{
    return m_spFilterRequestedEventSource && m_spFilterRequestedEventSource->HasHandlers();
}

// WPF: CollectionViewSource.WrapFilter. Accepted starts true; each handler may change it.
_Check_return_ HRESULT CollectionViewSource::RaiseFilterRequested(_In_opt_ IInspectable* pItem, _Out_ bool* pIsAccepted)
{
    ctl::ComPtr<FilterEventArgs> spArgs;
    BOOLEAN isAccepted = TRUE;

    *pIsAccepted = true;
    if (!HasFilterRequestedHandlers())
    {
        return S_OK;
    }

    IFC_RETURN(ctl::make(&spArgs));
    IFC_RETURN(spArgs->put_Item(pItem));
    IFC_RETURN(spArgs->put_Accepted(TRUE));
    IFC_RETURN(m_spFilterRequestedEventSource->Raise(ctl::as_iinspectable(this), ctl::interface_cast<xaml_data::IFilterEventArgs>(spArgs.Get())));
    IFC_RETURN(spArgs->get_Accepted(&isAccepted));
    *pIsAccepted = !!isAccepted;
    return S_OK;
}

// WPF: CollectionViewSource.GetDefaultView. The view is shared per source while anything holds it, and no
// CollectionViewSource owns it.
_Check_return_ HRESULT CollectionViewSourceFactory::GetDefaultViewImpl(_In_opt_ IInspectable* pSource, _Outptr_result_maybenull_ xaml_data::ICollectionView** ppReturnValue)
{
    ctl::ComPtr<IUnknown> spIdentity;
    ctl::ComPtr<xaml_data::ICollectionView> spView;
    ctl::ComPtr<xaml_data::ICollectionViewFactory> spFactory;
    bool isShapeable = false;

    *ppReturnValue = nullptr;
    if (!pSource)
    {
        return S_OK;
    }

    if (ctl::is<xaml_data::ICollectionView>(pSource))
    {
        IFC_RETURN(ctl::do_query_interface(*ppReturnValue, pSource));
        return S_OK;
    }

    IFC_RETURN(pSource->QueryInterface(IID_PPV_ARGS(spIdentity.ReleaseAndGetAddressOf())));

    t_defaultViews.erase(std::remove_if(t_defaultViews.begin(), t_defaultViews.end(), [&spView, &spIdentity](DefaultViewEntry& entry)
    {
        ctl::ComPtr<xaml_data::ICollectionView> spCandidate;
        IGNOREHR(entry.viewRef.As(&spCandidate));
        if (!spCandidate)
        {
            return true;
        }
        if (entry.pSourceIdentity == spIdentity.Get())
        {
            spView = std::move(spCandidate);
        }
        return false;
    }), t_defaultViews.end());

    if (!spView)
    {
        if (SUCCEEDED(pSource->QueryInterface(IID_PPV_ARGS(spFactory.ReleaseAndGetAddressOf()))))
        {
            IFC_RETURN(spFactory->CreateView(spView.ReleaseAndGetAddressOf()));
        }
        else
        {
            IFC_RETURN(CollectionViewManager::CreateNewView(pSource, false /* isShapingRequested */, spView.ReleaseAndGetAddressOf(), &isShapeable));
        }

        BOOLEAN isCurrentValid = FALSE;
        IFC_RETURN(spView->MoveCurrentToFirst(&isCurrentValid));

        // Only a list source's view (WPF: ListCollectionView) is shared and shapeable.
        if (isShapeable)
        {
            auto pView = AsShapingView(spView.Get());
            pView->SetSourceCollection(pSource);
            IFC_RETURN(pView->EnableDefaultViewShaping());

            DefaultViewEntry entry;
            entry.pSourceIdentity = spIdentity.Get();
            entry.pView = pView;
            IFC_RETURN(ctl::AsWeak(spView.Get(), &entry.viewRef));
            t_defaultViews.push_back(std::move(entry));
        }
    }

    *ppReturnValue = spView.Detach();
    return S_OK;
}

void DirectUI::UnregisterDefaultView(_In_ VectorCollectionView* pView)
{
    t_defaultViews.erase(std::remove_if(t_defaultViews.begin(), t_defaultViews.end(), [pView](const DefaultViewEntry& entry)
    {
        return entry.pView == pView;
    }), t_defaultViews.end());
}

// Collections for a view no CollectionViewSource owns (a default view).
_Check_return_ HRESULT VectorCollectionView::CreateShapingDescriptions(
    _Outptr_ wfc::IVector<xaml_data::SortDescription*>** ppSortDescriptions,
    _Outptr_ wfc::IVector<xaml_data::PropertyGroupDescription*>** ppGroupDescriptions)
{
    ctl::ComPtr<ShapingDescriptionCollection<xaml_data::SortDescription*>> spSortDescriptions;
    ctl::ComPtr<ShapingDescriptionCollection<xaml_data::PropertyGroupDescription*>> spGroupDescriptions;

    IFC_RETURN(ctl::ComObject<ShapingDescriptionCollection<xaml_data::SortDescription*>>::CreateInstance(spSortDescriptions.ReleaseAndGetAddressOf()));
    spSortDescriptions->SetIsSortDescriptions();
    IFC_RETURN(spSortDescriptions->SetOwner(this));
    IFC_RETURN(ctl::ComObject<ShapingDescriptionCollection<xaml_data::PropertyGroupDescription*>>::CreateInstance(spGroupDescriptions.ReleaseAndGetAddressOf()));
    IFC_RETURN(spGroupDescriptions->SetOwner(this));

    *ppSortDescriptions = spSortDescriptions.Detach();
    *ppGroupDescriptions = spGroupDescriptions.Detach();
    return S_OK;
}

// Reached only while there is no shaping view; otherwise the view hears the change.
_Check_return_ HRESULT CollectionViewSource::OnSortDescriptionsChanged()
{
    if (m_isApplyingCustomSort)
    {
        return S_OK;
    }

    if (m_tpCustomSort)
    {
        UINT sortCount = 0;
        IFC_RETURN(m_tpSortDescriptions->get_Size(&sortCount));
        if (sortCount > 0)
        {
            m_tpCustomSort.Clear();
        }
    }

    IFC_RETURN(OnShapingDescriptionsChanged());
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::RefreshImpl()
{
    if (m_tpShapingView)
    {
        IFC_RETURN(AsShapingView(m_tpShapingView.Get())->Refresh());
    }

    return S_OK;
}

// Deferrals taken here and on View share one level: the first one also defers View.
_Check_return_ HRESULT CollectionViewSource::DeferRefreshImpl(_Outptr_ wf::IClosable** ppReturnValue)
{
    Microsoft::WRL::ComPtr<CollectionViewRefreshDeferral<CollectionViewSource>> spDeferral;
    IFC_RETURN(wrl::MakeAndInitialize<CollectionViewRefreshDeferral<CollectionViewSource>>(&spDeferral, this));

    if (m_deferRefreshLevel == 0 && m_tpShapingView)
    {
        IFC_RETURN(AsShapingView(m_tpShapingView.Get())->DeferRefresh(m_spViewDeferral.ReleaseAndGetAddressOf()));
    }

    ++m_deferRefreshLevel;
    *ppReturnValue = spDeferral.Detach();
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::EndDeferRefresh()
{
    ASSERT(m_deferRefreshLevel > 0);
    if (m_deferRefreshLevel == 0 || --m_deferRefreshLevel > 0)
    {
        return S_OK;
    }

    if (m_spViewDeferral)
    {
        auto spViewDeferral = std::move(m_spViewDeferral);
        IFC_RETURN(spViewDeferral->Close());
    }

    if (!m_isRefreshPending)
    {
        return S_OK;
    }

    m_isRefreshPending = false;
    IFC_RETURN(OnShapingDescriptionsChanged());
    return S_OK;
}

// A rule changed while there is no shaping view: a View that cannot shape (IIterable or IVectorView source)
// is replaced by one that can once a rule is set.
_Check_return_ HRESULT CollectionViewSource::OnShapingDescriptionsChanged()
{
    ctl::ComPtr<IInspectable> spSource;
    bool isShapingRequested = false;

    if (m_deferRefreshLevel > 0)
    {
        m_isRefreshPending = true;
        return S_OK;
    }

    if (m_tpShapingView)
    {
        IFC_RETURN(AsShapingView(m_tpShapingView.Get())->OnShapingRulesChanged());
        return S_OK;
    }

    IFC_RETURN(get_Source(spSource.ReleaseAndGetAddressOf()));
    if (!spSource)
    {
        return S_OK;
    }

    IFC_RETURN(IsShapingRequested(spSource.Get(), &isShapingRequested));
    if (!isShapingRequested)
    {
        return S_OK;
    }

    IFC_RETURN(EnsureView(spSource.Get()));
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::IsShapingRequested(_In_ IInspectable* pSource, _Out_ bool* pIsRequested)
{
    BOOLEAN isSourceGrouped = FALSE;
    UINT sortCount = 0;
    UINT groupCount = 0;

    *pIsRequested = false;

    if (m_tpSortDescriptions)
    {
        IFC_RETURN(m_tpSortDescriptions->get_Size(&sortCount));
    }

    if (m_tpGroupDescriptions)
    {
        IFC_RETURN(m_tpGroupDescriptions->get_Size(&groupCount));
    }

    if (sortCount == 0 && groupCount == 0 && !m_tpFilter && !m_tpCustomSort && !HasFilterRequestedHandlers())
    {
        return S_OK;
    }

    // IsSourceGrouped and ICollectionViewFactory sources keep their existing behavior.
    IFC_RETURN(get_IsSourceGrouped(&isSourceGrouped));
    *pIsRequested = !isSourceGrouped && !ctl::is<xaml_data::ICollectionViewFactory>(pSource);
    return S_OK;
}
