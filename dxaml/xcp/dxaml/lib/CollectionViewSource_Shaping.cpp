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
#include "GroupDescription.g.h"
#include "VectorCollectionView.g.h"
#include "FilterEventArgs.g.h"
#include "CollectionViewManager.h"
#include "CollectionViewRefreshDeferral.h"
#include "CollectionViewLiveShapingProperties.h"

using namespace DirectUI;

namespace
{
    enum class DescriptionsKind { SortDescriptions, GroupDescriptions, GroupNames };

    // Descriptor collection shared by a CollectionViewSource and its shaping view. A change is reported to the
    // view while there is one, else to the CollectionViewSource. A group description's SortDescriptions and
    // GroupNames report to the group description. As in WPF (SortDescriptionCollection), a SortDescriptions
    // collection seals the descriptions it holds.
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

        _Check_return_ HRESULT SetOwner(_In_ GroupDescription* pOwner)
        {
            m_ownerKind = OwnerKind::GroupDescription;
            return ctl::AsWeak(pOwner, &m_ownerRef);
        }

        void SetKind(DescriptionsKind kind)
        {
            m_kind = kind;
        }

    protected:
        ShapingDescriptionCollection() = default;

        _Check_return_ HRESULT RaiseVectorChanged(_In_ wfc::CollectionChange action, UINT index) override
        {
            if (m_kind == DescriptionsKind::SortDescriptions)
            {
                IFC_RETURN(SealSortDescriptions(action, index));
            }

            IFC_RETURN(ObservableTrackerCollection<T>::RaiseVectorChanged(action, index));

            if (!m_ownerRef)
            {
                return S_OK;
            }

            const bool isSortDescriptions = m_kind == DescriptionsKind::SortDescriptions;
            if (m_ownerKind == OwnerKind::GroupDescription)
            {
                ctl::ComPtr<GroupDescription> spOwner;
                IFC_RETURN(m_ownerRef.As(&spOwner));
                if (spOwner)
                {
                    IFC_RETURN(isSortDescriptions ? spOwner->OnSortDescriptionsChanged() : spOwner->NotifyOwners());
                }
            }
            else if (m_ownerKind == OwnerKind::View)
            {
                ctl::ComPtr<xaml_data::ICollectionView> spView;
                IFC_RETURN(m_ownerRef.As(&spView));
                if (spView)
                {
                    auto pView = static_cast<VectorCollectionView*>(static_cast<CollectionView*>(spView.Get()));
                    IFC_RETURN(isSortDescriptions ? pView->OnSortDescriptionsChanged() : pView->OnShapingRulesChanged());
                }
            }
            else
            {
                ctl::ComPtr<xaml_data::ICollectionViewSource> spOwner;
                IFC_RETURN(m_ownerRef.As(&spOwner));
                if (spOwner)
                {
                    auto pOwner = spOwner.Cast<CollectionViewSource>();
                    IFC_RETURN(isSortDescriptions ? pOwner->OnSortDescriptionsChanged() : pOwner->OnShapingDescriptionsChanged());
                }
            }

            return S_OK;
        }

    private:
        enum class OwnerKind { Source, View, GroupDescription };

        _Check_return_ HRESULT SealSortDescriptions(_In_ wfc::CollectionChange action, UINT index)
        {
            UINT first = index;
            UINT last = index + 1;

            if (action == wfc::CollectionChange_Reset)
            {
                first = 0;
                IFC_RETURN(this->get_Size(&last));
            }
            else if (action != wfc::CollectionChange_ItemInserted && action != wfc::CollectionChange_ItemChanged)
            {
                return S_OK;
            }

            for (UINT i = first; i < last; ++i)
            {
                ctl::ComPtr<IInspectable> spItem;
                IFC_RETURN(this->UntypedGetAt(i, spItem.ReleaseAndGetAddressOf()));
                auto spDescription = ctl::query_interface_cast<SortDescription>(spItem.Get());
                if (spDescription)
                {
                    spDescription->Seal();
                }
            }
            return S_OK;
        }

        ctl::WeakRefPtr m_ownerRef;
        OwnerKind m_ownerKind = OwnerKind::Source;
        DescriptionsKind m_kind = DescriptionsKind::GroupDescriptions;
    };

    template <typename T, typename TOwner>
    _Check_return_ HRESULT CreateShapingDescriptionCollection(_In_ TOwner* pOwner, DescriptionsKind kind, _Outptr_ wfc::IVector<T>** ppCollection)
    {
        ctl::ComPtr<ShapingDescriptionCollection<T>> spCollection;
        IFC_RETURN(ctl::ComObject<ShapingDescriptionCollection<T>>::CreateInstance(spCollection.ReleaseAndGetAddressOf()));
        spCollection->SetKind(kind);
        IFC_RETURN(spCollection->SetOwner(pOwner));
        *ppCollection = spCollection.Detach();
        return S_OK;
    }

    bool IsValidStringComparison(xaml_data::CollectionViewStringComparison value)
    {
        return value >= xaml_data::CollectionViewStringComparison_CurrentCulture && value <= xaml_data::CollectionViewStringComparison_OrdinalIgnoreCase;
    }

    bool IsValidSortDirection(xaml_data::CollectionViewSortDirection value)
    {
        return value == xaml_data::CollectionViewSortDirection_Ascending || value == xaml_data::CollectionViewSortDirection_Descending;
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

_Check_return_ HRESULT SortDescriptionFactory::CreateInstanceWithPropertyNameAndDirectionImpl(
    _In_opt_ HSTRING propertyName,
    xaml_data::CollectionViewSortDirection direction,
    _Outptr_ xaml_data::ISortDescription** ppInstance)
{
    ctl::ComPtr<SortDescription> spDescription;

    IFCEXPECTRC_RETURN(IsValidSortDirection(direction), E_INVALIDARG);

    IFC_RETURN(ctl::make(&spDescription));
    IFC_RETURN(spDescription->put_PropertyName(propertyName));
    IFC_RETURN(spDescription->put_Direction(direction));
    IFC_RETURN(spDescription.CopyTo(ppInstance));
    return S_OK;
}

_Check_return_ HRESULT SortDescription::CheckNotSealed()
{
    if (m_isSealed)
    {
        static constexpr WCHAR c_message[] = L"A SortDescription cannot be changed once a SortDescriptions collection holds it.";
        IFC_RETURN(ErrorHelper::OriginateError(E_ILLEGAL_METHOD_CALL, ARRAYSIZE(c_message) - 1, c_message));
    }
    return S_OK;
}

_Check_return_ HRESULT SortDescription::get_PropertyNameImpl(_Out_ HSTRING* pValue)
{
    IFC_RETURN(GetValueByKnownIndex(KnownPropertyIndex::SortDescription_PropertyName, pValue));
    return S_OK;
}

_Check_return_ HRESULT SortDescription::put_PropertyNameImpl(_In_opt_ HSTRING value)
{
    IFC_RETURN(CheckNotSealed());
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::SortDescription_PropertyName, value));
    return S_OK;
}

_Check_return_ HRESULT SortDescription::get_DirectionImpl(_Out_ xaml_data::CollectionViewSortDirection* pValue)
{
    IFC_RETURN(GetValueByKnownIndex(KnownPropertyIndex::SortDescription_Direction, pValue));
    return S_OK;
}

_Check_return_ HRESULT SortDescription::put_DirectionImpl(xaml_data::CollectionViewSortDirection value)
{
    IFCEXPECTRC_RETURN(IsValidSortDirection(value), E_INVALIDARG);
    IFC_RETURN(CheckNotSealed());
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::SortDescription_Direction, value));
    return S_OK;
}

_Check_return_ HRESULT PropertyGroupDescriptionFactory::CreateInstanceWithPropertyNameImpl(
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

// WPF: PropertyGroupDescription.PropertyName / Converter / StringComparison raise PropertyChanged, which regroups.
_Check_return_ HRESULT PropertyGroupDescription::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(PropertyGroupDescriptionGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::PropertyGroupDescription_StringComparison:
        {
            xaml_data::CollectionViewStringComparison value = xaml_data::CollectionViewStringComparison_Ordinal;
            IFC_RETURN(get_StringComparison(&value));
            IFCEXPECTRC_RETURN(IsValidStringComparison(value), E_INVALIDARG);
            IFC_RETURN(NotifyOwners());
        }
        break;

    case KnownPropertyIndex::PropertyGroupDescription_PropertyName:
    case KnownPropertyIndex::PropertyGroupDescription_Converter:
        IFC_RETURN(NotifyOwners());
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT PropertyGroupDescription::GroupNameFromItemImpl(_In_opt_ IInspectable* pItem, INT level, _In_opt_ HSTRING culture, _Outptr_result_maybenull_ IInspectable** ppResult)
{
    wrl_wrappers::HString propertyName;
    ctl::ComPtr<xaml_data::IValueConverter> spConverter;

    IFC_RETURN(get_PropertyName(propertyName.GetAddressOf()));
    IFC_RETURN(get_Converter(spConverter.ReleaseAndGetAddressOf()));
    IFC_RETURN(ShapingGroupNameFromProperty(propertyName.Get(), spConverter.Get(), pItem, level, culture, ppResult));
    return S_OK;
}

_Check_return_ HRESULT PropertyGroupDescription::NamesMatchImpl(_In_opt_ IInspectable* pGroupName, _In_opt_ IInspectable* pItemName, _Out_ BOOLEAN* pResult)
{
    xaml_data::CollectionViewStringComparison comparison = xaml_data::CollectionViewStringComparison_Ordinal;
    bool isMatch = false;

    IFC_RETURN(get_StringComparison(&comparison));
    IFC_RETURN(ShapingNamesMatch(comparison, pGroupName, pItemName, &isMatch));
    *pResult = isMatch;
    return S_OK;
}

_Check_return_ HRESULT GroupDescription::GetDefaultValue2(_In_ const CDependencyProperty* pDP, _Out_ CValue* pValue)
{
    if (pDP->GetIndex() == KnownPropertyIndex::GroupDescription_SortDescriptions)
    {
        ctl::ComPtr<wfc::IObservableVector<xaml_data::SortDescription*>> spValue;
        IFC_RETURN(get_SortDescriptionsImpl(spValue.ReleaseAndGetAddressOf()));
        pValue->SetIInspectableAddRef(spValue.Get());
        return S_OK;
    }

    IFC_RETURN(GroupDescriptionGenerated::GetDefaultValue2(pDP, pValue));
    return S_OK;
}

_Check_return_ HRESULT GroupDescription::get_GroupNamesImpl(_Outptr_ wfc::IObservableVector<IInspectable*>** ppValue)
{
    if (!m_tpGroupNames)
    {
        ctl::ComPtr<wfc::IVector<IInspectable*>> spCollection;
        IFC_RETURN(CreateShapingDescriptionCollection<IInspectable*>(this, DescriptionsKind::GroupNames, spCollection.ReleaseAndGetAddressOf()));
        SetPtrValue(m_tpGroupNames, spCollection.Get());
    }

    IFC_RETURN(m_tpGroupNames.CopyTo(ppValue));
    return S_OK;
}

_Check_return_ HRESULT GroupDescription::get_SortDescriptionsImpl(_Outptr_ wfc::IObservableVector<xaml_data::SortDescription*>** ppValue)
{
    if (!m_tpSortDescriptions)
    {
        ctl::ComPtr<wfc::IVector<xaml_data::SortDescription*>> spCollection;
        IFC_RETURN(CreateShapingDescriptionCollection<xaml_data::SortDescription*>(this, DescriptionsKind::SortDescriptions, spCollection.ReleaseAndGetAddressOf()));
        SetPtrValue(m_tpSortDescriptions, spCollection.Get());
    }

    IFC_RETURN(m_tpSortDescriptions.CopyTo(ppValue));
    return S_OK;
}

_Check_return_ HRESULT GroupDescription::get_CustomSortImpl(_Outptr_result_maybenull_ xaml_data::ICollectionViewComparer** ppValue)
{
    IFC_RETURN(m_tpCustomSort.CopyTo(ppValue));
    return S_OK;
}

// As in WPF, a CustomSort replaces the SortDescriptions, and adding a SortDescription drops the CustomSort.
_Check_return_ HRESULT GroupDescription::put_CustomSortImpl(_In_opt_ xaml_data::ICollectionViewComparer* pValue)
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

_Check_return_ HRESULT GroupDescription::GroupNameFromItemImpl(_In_opt_ IInspectable* pItem, INT level, _In_opt_ HSTRING culture, _Outptr_result_maybenull_ IInspectable** ppResult)
{
    *ppResult = nullptr;
    return S_OK;
}

_Check_return_ HRESULT GroupDescription::NamesMatchImpl(_In_opt_ IInspectable* pGroupName, _In_opt_ IInspectable* pItemName, _Out_ BOOLEAN* pResult)
{
    bool isMatch = false;
    IFC_RETURN(ShapingNamesMatch(xaml_data::CollectionViewStringComparison_Ordinal, pGroupName, pItemName, &isMatch));
    *pResult = isMatch;
    return S_OK;
}

_Check_return_ HRESULT GroupDescription::OnSortDescriptionsChanged()
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

_Check_return_ HRESULT GroupDescription::AddOwner(_In_ VectorCollectionView* pView)
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

_Check_return_ HRESULT GroupDescription::NotifyOwners()
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
        ctl::ComPtr<wfc::IVector<xaml_data::SortDescription*>> spCollection;
        IFC_RETURN(CreateShapingDescriptionCollection<xaml_data::SortDescription*>(this, DescriptionsKind::SortDescriptions, spCollection.ReleaseAndGetAddressOf()));
        SetPtrValue(m_tpSortDescriptions, spCollection.Get());
    }

    if (!m_tpGroupDescriptions)
    {
        ctl::ComPtr<wfc::IVector<xaml_data::GroupDescription*>> spCollection;
        IFC_RETURN(CreateShapingDescriptionCollection<xaml_data::GroupDescription*>(this, DescriptionsKind::GroupDescriptions, spCollection.ReleaseAndGetAddressOf()));
        SetPtrValue(m_tpGroupDescriptions, spCollection.Get());
    }

    return S_OK;
}

// The descriptor collections are read-only DependencyProperties whose value is created on first use, so markup can
// fill them (Hub.SectionHeaders does the same).
_Check_return_ HRESULT CollectionViewSource::GetDefaultValue2(_In_ const CDependencyProperty* pDP, _Out_ CValue* pValue)
{
    ctl::ComPtr<IInspectable> spValue;

    switch (pDP->GetIndex())
    {
    case KnownPropertyIndex::CollectionViewSource_SortDescriptions:
        {
            ctl::ComPtr<wfc::IObservableVector<xaml_data::SortDescription*>> spCollection;
            IFC_RETURN(get_SortDescriptionsImpl(spCollection.ReleaseAndGetAddressOf()));
            IFC_RETURN(spCollection.As(&spValue));
        }
        break;

    case KnownPropertyIndex::CollectionViewSource_GroupDescriptions:
        {
            ctl::ComPtr<wfc::IObservableVector<xaml_data::GroupDescription*>> spCollection;
            IFC_RETURN(get_GroupDescriptionsImpl(spCollection.ReleaseAndGetAddressOf()));
            IFC_RETURN(spCollection.As(&spValue));
        }
        break;

    case KnownPropertyIndex::CollectionViewSource_LiveSortingProperties:
    case KnownPropertyIndex::CollectionViewSource_LiveFilteringProperties:
    case KnownPropertyIndex::CollectionViewSource_LiveGroupingProperties:
        {
            IFC_RETURN(EnsureLiveShapingProperties());
            auto pIndex = pDP->GetIndex();
            auto& spCollection = (pIndex == KnownPropertyIndex::CollectionViewSource_LiveSortingProperties) ? m_spLiveSortingProperties
                : (pIndex == KnownPropertyIndex::CollectionViewSource_LiveFilteringProperties) ? m_spLiveFilteringProperties : m_spLiveGroupingProperties;
            IFC_RETURN(spCollection.As(&spValue));
        }
        break;

    default:
        IFC_RETURN(CollectionViewSourceGenerated::GetDefaultValue2(pDP, pValue));
        return S_OK;
    }

    pValue->SetIInspectableAddRef(spValue.Get());
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::GetLiveShapingRequests(_Out_ bool* pIsLiveSorting, _Out_ bool* pIsLiveFiltering, _Out_ bool* pIsLiveGrouping)
{
    BOOLEAN isLiveSorting = FALSE;
    BOOLEAN isLiveFiltering = FALSE;
    BOOLEAN isLiveGrouping = FALSE;

    IFC_RETURN(get_IsLiveSortingRequested(&isLiveSorting));
    IFC_RETURN(get_IsLiveFilteringRequested(&isLiveFiltering));
    IFC_RETURN(get_IsLiveGroupingRequested(&isLiveGrouping));
    *pIsLiveSorting = !!isLiveSorting;
    *pIsLiveFiltering = !!isLiveFiltering;
    *pIsLiveGrouping = !!isLiveGrouping;
    return S_OK;
}

// WPF: CollectionViewSource.ApplyPropertiesToView. The view takes the rules and owns them while it is View.
_Check_return_ HRESULT CollectionViewSource::ApplyPropertiesToView(_In_ VectorCollectionView* pView)
{
    ctl::ComPtr<xaml_data::ICollectionViewComparer> spCustomSort = m_tpCustomSort.Get();

    IFC_RETURN(EnsureShapingDescriptions());
    IFC_RETURN(SetDescriptionsOwner(m_tpSortDescriptions.Get(), pView));
    IFC_RETURN(SetDescriptionsOwner(m_tpGroupDescriptions.Get(), pView));
    SetPtrValue(m_tpShapingView, static_cast<xaml_data::ICollectionView*>(pView));
    m_tpCustomSort.Clear();

    IFC_RETURN(pView->EnableShaping(this));
    bool isLiveSorting = false;
    bool isLiveFiltering = false;
    bool isLiveGrouping = false;
    IFC_RETURN(GetLiveShapingRequests(&isLiveSorting, &isLiveFiltering, &isLiveGrouping));
    IFC_RETURN(pView->ApplyLiveShaping(isLiveSorting, isLiveFiltering, isLiveGrouping,
        m_spLiveSortingProperties.Get(), m_spLiveFilteringProperties.Get(), m_spLiveGroupingProperties.Get()));
    IFC_RETURN(pView->ApplyShapingRules(m_tpSortDescriptions.Get(), m_tpGroupDescriptions.Get(), nullptr /* pFilter */, spCustomSort.Get(), m_culture.Get()));
    m_culture.Release();

    if (m_deferRefreshLevel > 0)
    {
        IFC_RETURN(pView->DeferRefresh(m_spViewDeferral.ReleaseAndGetAddressOf()));
    }

    return S_OK;
}

// Takes the rules back from the shaping view before View is replaced. The view's Filter delegate stays with it.
_Check_return_ HRESULT CollectionViewSource::DetachShapingView()
{
    ctl::ComPtr<xaml_data::ICollectionView> spView = m_tpShapingView.Get();
    ctl::ComPtr<xaml_data::ICollectionViewComparer> spCustomSort;

    if (!spView)
    {
        return S_OK;
    }

    auto pView = AsShapingView(spView.Get());
    IFC_RETURN(pView->get_CustomSort(spCustomSort.ReleaseAndGetAddressOf()));
    m_culture.Release();
    IFC_RETURN(pView->get_Culture(m_culture.GetAddressOf()));
    IFC_RETURN(pView->DetachFromHost());

    m_tpShapingView.Clear();
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

_Check_return_ HRESULT CollectionViewSource::get_GroupDescriptionsImpl(_Outptr_ wfc::IObservableVector<xaml_data::GroupDescription*>** ppValue)
{
    IFC_RETURN(EnsureShapingDescriptions());
    IFC_RETURN(m_tpGroupDescriptions.CopyTo(ppValue));
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
        bool isLiveSorting = false;
        bool isLiveFiltering = false;
        bool isLiveGrouping = false;
        IFC_RETURN(GetLiveShapingRequests(&isLiveSorting, &isLiveFiltering, &isLiveGrouping));
        IFC_RETURN(AsShapingView(m_tpShapingView.Get())->ApplyLiveShaping(isLiveSorting, isLiveFiltering, isLiveGrouping,
            m_spLiveSortingProperties.Get(), m_spLiveFilteringProperties.Get(), m_spLiveGroupingProperties.Get()));
    }

    return S_OK;
}

#if WI_IS_FEATURE_PRESENT(Feature_ExperimentalApi)
_Check_return_ HRESULT STDMETHODCALLTYPE CollectionViewSourceGenerated::add_Filter(_In_ xaml_data::IFilterEventHandler* pValue, _Out_ EventRegistrationToken* pToken)
{
    IFC_RETURN(CheckThread());
    IFC_RETURN(static_cast<CollectionViewSource*>(this)->AddFilterEventHandler(pValue, pToken));
    return S_OK;
}

_Check_return_ HRESULT STDMETHODCALLTYPE CollectionViewSourceGenerated::remove_Filter(EventRegistrationToken token)
{
    IFC_RETURN(CheckThread());
    IFC_RETURN(static_cast<CollectionViewSource*>(this)->RemoveFilterEventHandler(token));
    return S_OK;
}
#endif

// As in WPF, adding or removing a handler refilters the view.
_Check_return_ HRESULT CollectionViewSource::AddFilterEventHandler(_In_ xaml_data::IFilterEventHandler* pHandler, _Out_ EventRegistrationToken* pToken)
{
    ARG_NOTNULL_RETURN(pHandler, "value");
    ARG_VALIDRETURNPOINTER(pToken);

    if (!m_spFilterEventSource)
    {
        IFC_RETURN(ctl::ComObject<FilterEventSource>::CreateInstance(m_spFilterEventSource.ReleaseAndGetAddressOf()));
    }

    IFC_RETURN(m_spFilterEventSource->AddHandler(pHandler));
    pToken->value = reinterpret_cast<INT64>(pHandler);
    IFC_RETURN(OnFilterEventHandlersChanged());
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::RemoveFilterEventHandler(EventRegistrationToken token)
{
    auto pHandler = reinterpret_cast<xaml_data::IFilterEventHandler*>(token.value);

    if (!m_spFilterEventSource || !pHandler)
    {
        return S_OK;
    }

    IFC_RETURN(m_spFilterEventSource->RemoveHandler(pHandler));
    IFC_RETURN(OnFilterEventHandlersChanged());
    return S_OK;
}

// WPF wraps the event in a filter delegate set on the view, so the view keeps the CollectionViewSource alive
// while the event has handlers; an app that keeps only View keeps filtering.
_Check_return_ HRESULT CollectionViewSource::OnFilterEventHandlersChanged()
{
    if (m_tpShapingView)
    {
        AsShapingView(m_tpShapingView.Get())->SetFilterHost(HasFilterEventHandlers() ? this : nullptr);
    }

    IFC_RETURN(OnShapingDescriptionsChanged());
    return S_OK;
}

bool CollectionViewSource::HasFilterEventHandlers() const
{
    return m_spFilterEventSource && m_spFilterEventSource->HasHandlers();
}

// WPF: CollectionViewSource.WrapFilter. Accepted starts true; each handler may change it.
_Check_return_ HRESULT CollectionViewSource::RaiseFilterEvent(_In_opt_ IInspectable* pItem, _Out_ bool* pIsAccepted)
{
    ctl::ComPtr<FilterEventArgs> spArgs;
    BOOLEAN isAccepted = TRUE;

    *pIsAccepted = true;
    if (!HasFilterEventHandlers())
    {
        return S_OK;
    }

    IFC_RETURN(ctl::make(&spArgs));
    IFC_RETURN(spArgs->put_Item(pItem));
    IFC_RETURN(spArgs->put_Accepted(TRUE));
    IFC_RETURN(m_spFilterEventSource->Raise(ctl::as_iinspectable(this), ctl::interface_cast<xaml_data::IFilterEventArgs>(spArgs.Get())));
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
    _Outptr_ wfc::IVector<xaml_data::GroupDescription*>** ppGroupDescriptions)
{
    ctl::ComPtr<wfc::IVector<xaml_data::SortDescription*>> spSortDescriptions;
    ctl::ComPtr<wfc::IVector<xaml_data::GroupDescription*>> spGroupDescriptions;

    IFC_RETURN(CreateShapingDescriptionCollection<xaml_data::SortDescription*>(this, DescriptionsKind::SortDescriptions, spSortDescriptions.ReleaseAndGetAddressOf()));
    IFC_RETURN(CreateShapingDescriptionCollection<xaml_data::GroupDescription*>(this, DescriptionsKind::GroupDescriptions, spGroupDescriptions.ReleaseAndGetAddressOf()));

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

    IFC_RETURN(CheckShapingSupported(spSource.Get()));
    IFC_RETURN(IsShapingRequested(spSource.Get(), &isShapingRequested));
    if (!isShapingRequested)
    {
        return S_OK;
    }

    IFC_RETURN(EnsureView(spSource.Get()));
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::HasShapingRules(_Out_ bool* pHasRules)
{
    UINT sortCount = 0;
    UINT groupCount = 0;

    if (m_tpSortDescriptions)
    {
        IFC_RETURN(m_tpSortDescriptions->get_Size(&sortCount));
    }

    if (m_tpGroupDescriptions)
    {
        IFC_RETURN(m_tpGroupDescriptions->get_Size(&groupCount));
    }

    *pHasRules = sortCount > 0 || groupCount > 0 || m_tpCustomSort || HasFilterEventHandlers();
    return S_OK;
}

// IsSourceGrouped and ICollectionViewFactory sources keep their existing behavior: their view cannot shape.
_Check_return_ HRESULT CollectionViewSource::IsShapeableSource(_In_ IInspectable* pSource, _Out_ bool* pIsShapeable)
{
    BOOLEAN isSourceGrouped = FALSE;
    IFC_RETURN(get_IsSourceGrouped(&isSourceGrouped));
    *pIsShapeable = !isSourceGrouped && !ctl::is<xaml_data::ICollectionViewFactory>(pSource);
    return S_OK;
}

_Check_return_ HRESULT CollectionViewSource::IsShapingRequested(_In_ IInspectable* pSource, _Out_ bool* pIsRequested)
{
    bool hasRules = false;

    *pIsRequested = false;
    IFC_RETURN(HasShapingRules(&hasRules));
    if (hasRules)
    {
        IFC_RETURN(IsShapeableSource(pSource, pIsRequested));
    }
    return S_OK;
}

// As in WPF (CollectionViewSource.ApplyPropertiesToView: CannotSortView, CannotGroupView, CannotFilterView), a
// rule the view cannot apply is an error rather than ignored.
_Check_return_ HRESULT CollectionViewSource::CheckShapingSupported(_In_ IInspectable* pSource)
{
    bool hasRules = false;
    bool isShapeable = true;

    IFC_RETURN(HasShapingRules(&hasRules));
    if (hasRules)
    {
        IFC_RETURN(IsShapeableSource(pSource, &isShapeable));
    }

    if (!isShapeable)
    {
        static constexpr WCHAR c_message[] = L"A CollectionViewSource with IsSourceGrouped set, or over an ICollectionViewFactory, cannot sort, filter or group its View.";
        IFC_RETURN(ErrorHelper::OriginateError(E_ILLEGAL_METHOD_CALL, ARRAYSIZE(c_message) - 1, c_message));
    }
    return S_OK;
}
