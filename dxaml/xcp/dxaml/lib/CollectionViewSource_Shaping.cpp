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
#include "CollectionViewRefreshDeferral.h"

using namespace DirectUI;

namespace
{
    // Descriptor collection shared by a CollectionViewSource and its shaping view. A change is reported to the
    // view while there is one, else to the CollectionViewSource.
    template <typename T>
    class __declspec(novtable) ShapingDescriptionCollection : public ObservableTrackerCollection<T>
    {
    public:
        _Check_return_ HRESULT SetOwner(_In_ CollectionViewSource* pOwner)
        {
            m_isOwnerView = false;
            return ctl::AsWeak(pOwner, &m_ownerRef);
        }

        _Check_return_ HRESULT SetOwner(_In_ VectorCollectionView* pOwner)
        {
            m_isOwnerView = true;
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

            if (m_isOwnerView)
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
        ctl::WeakRefPtr m_ownerRef;
        bool m_isOwnerView = false;
        bool m_isSortDescriptions = false;
    };

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
    ctl::ComPtr<PropertyGroupDescription> spDescription;

    IFC_RETURN(ctl::make(&spDescription));
    IFC_RETURN(spDescription->put_PropertyName(propertyName));
    IFC_RETURN(spDescription.CopyTo(ppInstance));
    return S_OK;
}

_Check_return_ HRESULT PropertyGroupDescriptionFactory::CreateInstanceWithConverterImpl(
    _In_opt_ HSTRING propertyName,
    _In_opt_ xaml_data::IValueConverter* pConverter,
    _Outptr_ xaml_data::IPropertyGroupDescription** ppInstance)
{
    ctl::ComPtr<PropertyGroupDescription> spDescription;

    IFC_RETURN(ctl::make(&spDescription));
    IFC_RETURN(spDescription->put_PropertyName(propertyName));
    IFC_RETURN(spDescription->put_Converter(pConverter));
    IFC_RETURN(spDescription.CopyTo(ppInstance));
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
    IFC_RETURN(pView->ApplyShapingRules(m_tpSortDescriptions.Get(), m_tpGroupDescriptions.Get(), spFilter.Get(), spCustomSort.Get()));

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

_Check_return_ HRESULT CollectionViewSource::OnShapingViewGroupingChanged(_In_ VectorCollectionView* pView, _Out_ bool* pIsHandled)
{
    ctl::ComPtr<xaml_data::ICollectionView> spView;
    ctl::ComPtr<IInspectable> spSource;

    *pIsHandled = false;

    IFC_RETURN(get_View(spView.ReleaseAndGetAddressOf()));
    if (!m_tpShapingView || spView.Get() != m_tpShapingView.Get() || AsShapingView(spView.Get()) != pView)
    {
        return S_OK;
    }

    IFC_RETURN(get_Source(spSource.ReleaseAndGetAddressOf()));
    IFC_RETURN(EnsureView(spSource.Get()));
    *pIsHandled = true;
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

    if (sortCount == 0 && groupCount == 0 && !m_tpFilter && !m_tpCustomSort)
    {
        return S_OK;
    }

    // IsSourceGrouped and ICollectionViewFactory sources keep their existing behavior.
    IFC_RETURN(get_IsSourceGrouped(&isSourceGrouped));
    *pIsRequested = !isSourceGrouped && !ctl::is<xaml_data::ICollectionViewFactory>(pSource);
    return S_OK;
}
