// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Experimental WPF-style shaping (sort, filter, group): the engine of the VectorCollectionView a
//      CollectionViewSource creates over a vector (WPF: ListCollectionView). See
//      docs\design-notes\CollectionViewSourceShaping-POC.md.

#include "precomp.h"
#include <algorithm>
#include <vector>
#include "CollectionViewSource.g.h"
#include "VectorCollectionView.g.h"
#include "GroupedDataCollectionView.g.h"
#include "FrameworkElement.g.h"
#include "CollectionViewRefreshDeferral.h"
#include "PropertyPathParser.h"
#include "PropertyPath.h"

using namespace DirectUI;

namespace
{
    // Listeners are created with fListenToChanges = false, so SourceChanged is never called.
    struct ShapingPropertyPathHost final : public IPropertyPathListenerHost
    {
        _Check_return_ HRESULT GetTraceString(_Outptr_result_z_ const WCHAR **pszTraceString) override
        {
            *pszTraceString = L"";
            return S_OK;
        }

        _Check_return_ HRESULT SourceChanged() override
        {
            return S_OK;
        }
    };

    ShapingPropertyPathHost s_shapingPropertyPathHost;

    // The shaped list the view wraps (WPF: ListCollectionView.InternalList). Read-only; a recompute
    // replaces its contents wholesale and announces a single Reset.
    class __declspec(novtable) InternalList : public ReadOnlyObservableTrackerCollection<IInspectable*>
    {
    public:
        _Check_return_ HRESULT ReplaceAll(_In_ const std::vector<ctl::ComPtr<IInspectable>>& items)
        {
            IFC_RETURN(TrackerCollection<IInspectable*>::Clear());
            Reserve(static_cast<uint32_t>(items.size()));
            for (const auto& item : items)
            {
                IFC_RETURN(TrackerCollection<IInspectable*>::Append(item.Get()));
            }

            IFC_RETURN(RaiseVectorChanged(wfc::CollectionChange_Reset, 0));
            return S_OK;
        }

    protected:
        InternalList() = default;
    };

    // A computed group (WPF: MS.Internal.Data.CollectionViewGroupInternal). It is its own item list
    // (as with IsSourceGrouped and no ItemsPath) and exposes Key (IGrouping<K,T> idiom) and Name
    // (WPF CollectionViewGroup) to bindings.
    class __declspec(novtable) CollectionViewGroupInternal :
        public InternalList,
        public xaml_data::ICustomPropertyProvider
    {
        BEGIN_INTERFACE_MAP(CollectionViewGroupInternal, InternalList)
            INTERFACE_ENTRY(CollectionViewGroupInternal, xaml_data::ICustomPropertyProvider)
        END_INTERFACE_MAP(CollectionViewGroupInternal, InternalList)

    public:
        void SetKey(_In_opt_ IInspectable* pKey)
        {
            m_spKey = pKey;
        }

        IInspectable* GetKeyNoRef() const
        {
            return m_spKey.Get();
        }

        IFACEMETHODIMP get_Type(_Out_ wxaml_interop::TypeName* pValue) override
        {
            wxaml_interop::TypeName typeName = {};
            typeName.Kind = wxaml_interop::TypeKind_Primitive;
            IFC_RETURN(WindowsCreateString(STR_LEN_PAIR(L"Object"), &typeName.Name));
            *pValue = typeName;
            return S_OK;
        }

        IFACEMETHODIMP GetCustomProperty(_In_ HSTRING name, _Outptr_ xaml_data::ICustomProperty** ppValue) override
        {
            *ppValue = nullptr;
            if (wrl_wrappers::HStringReference(L"Key") == name || wrl_wrappers::HStringReference(L"Name") == name)
            {
                IFC_RETURN(CustomProperty::CreateObjectProperty(name, &CollectionViewGroupInternal::GetKeyPropertyValue, ppValue));
            }
            return S_OK;
        }

        IFACEMETHODIMP GetIndexedProperty(_In_ HSTRING name, _In_ wxaml_interop::TypeName type, _Outptr_ xaml_data::ICustomProperty** ppValue) override
        {
            *ppValue = nullptr;
            return S_OK;
        }

        IFACEMETHODIMP GetStringRepresentation(_Out_ HSTRING* pValue) override
        {
            *pValue = nullptr;
            if (m_spKey)
            {
                IFC_RETURN(FrameworkElement::GetStringFromObject(m_spKey.Get(), pValue));
            }
            return S_OK;
        }

    protected:
        CollectionViewGroupInternal() = default;

        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(xaml_data::ICustomPropertyProvider)))
            {
                *ppObject = static_cast<xaml_data::ICustomPropertyProvider*>(this);
            }
            else
            {
                return InternalList::QueryInterfaceImpl(iid, ppObject);
            }

            AddRefOuter();
            return S_OK;
        }

    private:
        static _Check_return_ HRESULT GetKeyPropertyValue(_In_ IInspectable* pTarget, _Outptr_ IInspectable** ppValue)
        {
            ctl::ComPtr<IInspectable> spTarget(pTarget);
            ctl::ComPtr<xaml_data::ICustomPropertyProvider> spProvider;
            IFC_RETURN(spTarget.As(&spProvider));
            IFC_RETURN(spProvider.Cast<CollectionViewGroupInternal>()->m_spKey.CopyTo(ppValue));
            return S_OK;
        }

        // Not a TrackerPtr: ObservableTrackerCollection::OnReferenceTrackerWalk is final.
        ctl::ComPtr<IInspectable> m_spKey;
    };

    struct ShapingKeySelector
    {
        ctl::ComPtr<PropertyPathListener> listener; // Null means "the item itself", as in WPF.
        bool isDescending = false;
    };

    _Check_return_ HRESULT CreateKeySelector(_In_opt_ HSTRING path, _Out_ ShapingKeySelector* pSelector)
    {
        UINT32 length = 0;
        LPCWSTR szPath = WindowsGetStringRawBuffer(path, &length);

        if (length == 0)
        {
            return S_OK;
        }

        PropertyPathParser parser{};
        IFC_RETURN(parser.SetSource(szPath, nullptr));
        IFC_RETURN(ctl::make<PropertyPathListener>(&s_shapingPropertyPathHost, &parser, false /* fListenToChanges */, false /* fUseWeakReferenceForSource */, &pSelector->listener));
        return S_OK;
    }

    _Check_return_ HRESULT EvaluateKey(_In_ const ShapingKeySelector& selector, _In_opt_ IInspectable* pItem, _Outptr_result_maybenull_ IInspectable** ppKey)
    {
        *ppKey = nullptr;

        if (!selector.listener)
        {
            *ppKey = pItem;
            ctl::addref_interface(pItem);
            return S_OK;
        }

        HRESULT hr = selector.listener->SetSource(pItem);
        if (SUCCEEDED(hr) && selector.listener->FullPathExists())
        {
            hr = selector.listener->GetValue(ppKey);
        }

        // Do not keep the item alive through the listener.
        IGNOREHR(selector.listener->SetSource(nullptr));
        return hr;
    }

    _Check_return_ HRESULT GetNumericValue(_In_ wf::IPropertyValue* pValue, wf::PropertyType type, _Out_ DOUBLE* pResult)
    {
        switch (type)
        {
        case wf::PropertyType_UInt8:  { BYTE v = 0;   IFC_RETURN(pValue->GetUInt8(&v));  *pResult = v; break; }
        case wf::PropertyType_Char16: { WCHAR v = 0;  IFC_RETURN(pValue->GetChar16(&v)); *pResult = v; break; }
        case wf::PropertyType_Int16:  { INT16 v = 0;  IFC_RETURN(pValue->GetInt16(&v));  *pResult = v; break; }
        case wf::PropertyType_UInt16: { UINT16 v = 0; IFC_RETURN(pValue->GetUInt16(&v)); *pResult = v; break; }
        case wf::PropertyType_Int32:  { INT32 v = 0;  IFC_RETURN(pValue->GetInt32(&v));  *pResult = v; break; }
        case wf::PropertyType_UInt32: { UINT32 v = 0; IFC_RETURN(pValue->GetUInt32(&v)); *pResult = v; break; }
        case wf::PropertyType_Int64:  { INT64 v = 0;  IFC_RETURN(pValue->GetInt64(&v));  *pResult = static_cast<DOUBLE>(v); break; }
        case wf::PropertyType_UInt64: { UINT64 v = 0; IFC_RETURN(pValue->GetUInt64(&v)); *pResult = static_cast<DOUBLE>(v); break; }
        case wf::PropertyType_Single: { FLOAT v = 0;  IFC_RETURN(pValue->GetSingle(&v)); *pResult = v; break; }
        case wf::PropertyType_Double: { IFC_RETURN(pValue->GetDouble(pResult)); break; }
        default: return E_INVALIDARG;
        }

        return S_OK;
    }

    // Values that cannot be ordered compare equal, so the stable sort keeps their source order.
    int CompareKeys(_In_opt_ IInspectable* pLeft, _In_opt_ IInspectable* pRight)
    {
        if (!pLeft || !pRight)
        {
            return (pLeft ? 1 : 0) - (pRight ? 1 : 0);
        }

        auto spLeft = ctl::query_interface_cast<wf::IPropertyValue>(pLeft);
        auto spRight = ctl::query_interface_cast<wf::IPropertyValue>(pRight);
        wf::PropertyType leftType = wf::PropertyType_Empty;
        wf::PropertyType rightType = wf::PropertyType_Empty;

        if (!spLeft || !spRight || FAILED(spLeft->get_Type(&leftType)) || FAILED(spRight->get_Type(&rightType)))
        {
            return 0;
        }

        if (leftType == wf::PropertyType_String && rightType == wf::PropertyType_String)
        {
            wrl_wrappers::HString left;
            wrl_wrappers::HString right;
            UINT32 leftLength = 0;
            UINT32 rightLength = 0;

            if (FAILED(spLeft->GetString(left.GetAddressOf())) || FAILED(spRight->GetString(right.GetAddressOf())))
            {
                return 0;
            }

            LPCWSTR szLeft = left.GetRawBuffer(&leftLength);
            LPCWSTR szRight = right.GetRawBuffer(&rightLength);
            const int result = CompareStringEx(LOCALE_NAME_USER_DEFAULT, 0, szLeft, static_cast<int>(leftLength), szRight, static_cast<int>(rightLength), nullptr, nullptr, 0);
            return (result == 0) ? 0 : result - CSTR_EQUAL;
        }

        if (DirectUI::PropertyValue::IsNumericPropertyType(leftType) && DirectUI::PropertyValue::IsNumericPropertyType(rightType))
        {
            DOUBLE left = 0;
            DOUBLE right = 0;

            if (FAILED(GetNumericValue(spLeft.Get(), leftType, &left)) || FAILED(GetNumericValue(spRight.Get(), rightType, &right)))
            {
                return 0;
            }

            return (left < right) ? -1 : ((left > right) ? 1 : 0);
        }

        if (leftType == rightType)
        {
            switch (leftType)
            {
            case wf::PropertyType_Boolean:
            {
                BOOLEAN left = FALSE;
                BOOLEAN right = FALSE;
                if (SUCCEEDED(spLeft->GetBoolean(&left)) && SUCCEEDED(spRight->GetBoolean(&right)))
                {
                    return static_cast<int>(!!left) - static_cast<int>(!!right);
                }
                break;
            }

            case wf::PropertyType_DateTime:
            {
                wf::DateTime left = {};
                wf::DateTime right = {};
                if (SUCCEEDED(spLeft->GetDateTime(&left)) && SUCCEEDED(spRight->GetDateTime(&right)))
                {
                    return (left.UniversalTime < right.UniversalTime) ? -1 : ((left.UniversalTime > right.UniversalTime) ? 1 : 0);
                }
                break;
            }

            case wf::PropertyType_TimeSpan:
            {
                wf::TimeSpan left = {};
                wf::TimeSpan right = {};
                if (SUCCEEDED(spLeft->GetTimeSpan(&left)) && SUCCEEDED(spRight->GetTimeSpan(&right)))
                {
                    return (left.Duration < right.Duration) ? -1 : ((left.Duration > right.Duration) ? 1 : 0);
                }
                break;
            }

            default:
                break;
            }
        }

        return 0;
    }

    // Orders items by the SortDescriptions, or by the CustomSort when set (WPF: MS.Internal.Data.SortFieldComparer).
    // Ties fall back to the source index, so the full sort is stable and the incremental placement
    // matches it.
    class SortFieldComparer
    {
    public:
        _Check_return_ HRESULT Initialize(
            _In_opt_ wfc::IVector<xaml_data::SortDescription*>* pDescriptions,
            _In_opt_ xaml_data::ICollectionViewComparer* pCustomSort)
        {
            UINT count = 0;

            m_fields.clear();
            m_spCustomSort = pCustomSort;
            if (m_spCustomSort || !pDescriptions)
            {
                return S_OK;
            }

            IFC_RETURN(pDescriptions->get_Size(&count));
            for (UINT i = 0; i < count; ++i)
            {
                ctl::ComPtr<xaml_data::ISortDescription> spDescription;
                IFC_RETURN(pDescriptions->GetAt(i, spDescription.ReleaseAndGetAddressOf()));
                if (!spDescription)
                {
                    continue;
                }

                wrl_wrappers::HString path;
                xaml_data::ListSortDirection direction = xaml_data::ListSortDirection_Ascending;
                ShapingKeySelector field;
                IFC_RETURN(spDescription->get_PropertyName(path.GetAddressOf()));
                IFC_RETURN(spDescription->get_Direction(&direction));
                IFC_RETURN(CreateKeySelector(path.Get(), &field));
                field.isDescending = (direction == xaml_data::ListSortDirection_Descending);
                m_fields.push_back(std::move(field));
            }

            return S_OK;
        }

        bool IsEmpty() const
        {
            return !m_spCustomSort && m_fields.empty();
        }

        // The sort keys Compare reads for an item; none with a CustomSort.
        _Check_return_ HRESULT EvaluateKeys(_In_opt_ IInspectable* pItem, _Out_ std::vector<ctl::ComPtr<IInspectable>>* pKeys) const
        {
            pKeys->clear();
            pKeys->reserve(m_fields.size());
            for (const auto& field : m_fields)
            {
                ctl::ComPtr<IInspectable> spKey;
                IFC_RETURN(EvaluateKey(field, pItem, spKey.ReleaseAndGetAddressOf()));
                pKeys->push_back(std::move(spKey));
            }

            return S_OK;
        }

        // pRightKeys null: the right item's keys are evaluated as needed.
        _Check_return_ HRESULT Compare(
            _In_opt_ IInspectable* pLeft,
            const std::vector<ctl::ComPtr<IInspectable>>& leftKeys,
            UINT leftSourceIndex,
            _In_opt_ IInspectable* pRight,
            _In_opt_ const std::vector<ctl::ComPtr<IInspectable>>* pRightKeys,
            UINT rightSourceIndex,
            _Out_ int* pResult) const
        {
            *pResult = 0;

            if (m_spCustomSort)
            {
                INT32 result = 0;
                IFC_RETURN(m_spCustomSort->Invoke(pLeft, pRight, &result));
                *pResult = (result > 0) - (result < 0);
            }
            else
            {
                for (size_t i = 0; i < m_fields.size() && *pResult == 0; ++i)
                {
                    ctl::ComPtr<IInspectable> spRightKey;
                    if (pRightKeys)
                    {
                        spRightKey = (*pRightKeys)[i];
                    }
                    else
                    {
                        IFC_RETURN(EvaluateKey(m_fields[i], pRight, spRightKey.ReleaseAndGetAddressOf()));
                    }

                    const int result = CompareKeys(leftKeys[i].Get(), spRightKey.Get());
                    *pResult = m_fields[i].isDescending ? -result : result;
                }
            }

            if (*pResult == 0)
            {
                *pResult = (leftSourceIndex < rightSourceIndex) ? -1 : ((leftSourceIndex > rightSourceIndex) ? 1 : 0);
            }

            return S_OK;
        }

    private:
        std::vector<ShapingKeySelector> m_fields;
        ctl::ComPtr<xaml_data::ICollectionViewComparer> m_spCustomSort;
    };

    // Builds the top-level groups in order of first appearance of their key (WPF: MS.Internal.Data.CollectionViewGroupRoot).
    class CollectionViewGroupRoot
    {
    public:
        _Check_return_ HRESULT AddToSubgroups(_In_opt_ IInspectable* pItem, _In_opt_ IInspectable* pKey)
        {
            size_t groupIndex = m_groups.size();
            for (size_t g = 0; g < m_groups.size(); ++g)
            {
                bool areEqual = false;
                IFC_RETURN(DirectUI::PropertyValue::AreEqual(m_groups[g]->GetKeyNoRef(), pKey, &areEqual));
                if (areEqual)
                {
                    groupIndex = g;
                    break;
                }
            }

            if (groupIndex == m_groups.size())
            {
                ctl::ComPtr<CollectionViewGroupInternal> spGroup;
                IFC_RETURN(ctl::ComObject<CollectionViewGroupInternal>::CreateInstance(spGroup.ReleaseAndGetAddressOf()));
                spGroup->SetKey(pKey);
                m_groups.push_back(std::move(spGroup));
                m_groupItems.emplace_back();
            }

            m_groupItems[groupIndex].emplace_back(pItem);
            return S_OK;
        }

        // Fills each group with its items and returns the groups, in order.
        _Check_return_ HRESULT GetGroups(_Out_ std::vector<ctl::ComPtr<IInspectable>>* pGroups)
        {
            pGroups->clear();
            pGroups->reserve(m_groups.size());
            for (size_t g = 0; g < m_groups.size(); ++g)
            {
                IFC_RETURN(m_groups[g]->ReplaceAll(m_groupItems[g]));
                pGroups->push_back(ctl::as_iinspectable(m_groups[g].Get()));
            }

            return S_OK;
        }

    private:
        std::vector<ctl::ComPtr<CollectionViewGroupInternal>> m_groups;
        std::vector<std::vector<ctl::ComPtr<IInspectable>>> m_groupItems;
    };

}

HRESULT VectorCollectionView::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(xaml_data::ICollectionViewShaping)) && m_isShapingEnabled)
    {
        *ppObject = static_cast<xaml_data::ICollectionViewShaping*>(this);
        AddRefOuter();
        return S_OK;
    }

    return VectorCollectionViewGenerated::QueryInterfaceImpl(iid, ppObject);
}

_Check_return_ HRESULT VectorCollectionView::EnableShaping(_In_ CollectionViewSource* pHost)
{
    m_isShapingEnabled = true;
    SetPtrValue(m_tpSourceList, m_tpSource.Get());
    IFC_RETURN(ctl::AsWeak(pHost, &m_hostRef));
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::ApplyShapingRules(
    _In_ wfc::IVector<xaml_data::SortDescription*>* pSortDescriptions,
    _In_ wfc::IVector<xaml_data::PropertyGroupDescription*>* pGroupDescriptions,
    _In_opt_ xaml_data::ICollectionViewFilter* pFilter,
    _In_opt_ xaml_data::ICollectionViewComparer* pCustomSort)
{
    SetPtrValue(m_tpSortDescriptions, pSortDescriptions);
    SetPtrValue(m_tpGroupDescriptions, pGroupDescriptions);
    SetPtrValue(m_tpFilter, pFilter);
    SetPtrValue(m_tpCustomSort, pCustomSort);
    IFC_RETURN(OnShapingRulesChanged());
    return S_OK;
}

void VectorCollectionView::DetachShaping()
{
    m_isShapingDetached = true;
    DetachSourceList();
}

// A view its CollectionViewSource replaced keeps answering ICollectionViewShaping through that source, so it
// still shapes the source's current View.
_Check_return_ HRESULT VectorCollectionView::GetDetachedHost(_Outptr_result_maybenull_ CollectionViewSource** ppHost)
{
    *ppHost = nullptr;
    IFC_RETURN(CheckThread());

    if (m_isShapingDetached && m_hostRef)
    {
        ctl::ComPtr<xaml_data::ICollectionViewSource> spHost;
        IFC_RETURN(m_hostRef.As(&spHost));
        if (spHost)
        {
            *ppHost = static_cast<CollectionViewSource*>(spHost.Detach());
        }
    }

    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_SortDescriptions(_Outptr_ wfc::IObservableVector<xaml_data::SortDescription*>** ppValue)
{
    ctl::ComPtr<CollectionViewSource> spHost;
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(GetDetachedHost(spHost.ReleaseAndGetAddressOf()));
    if (spHost)
    {
        IFC_RETURN(spHost->get_SortDescriptionsImpl(ppValue));
        return S_OK;
    }

    IFCEXPECT_RETURN(m_tpSortDescriptions);
    IFC_RETURN(m_tpSortDescriptions.CopyTo(ppValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_GroupDescriptions(_Outptr_ wfc::IObservableVector<xaml_data::PropertyGroupDescription*>** ppValue)
{
    ctl::ComPtr<CollectionViewSource> spHost;
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(GetDetachedHost(spHost.ReleaseAndGetAddressOf()));
    if (spHost)
    {
        IFC_RETURN(spHost->get_GroupDescriptionsImpl(ppValue));
        return S_OK;
    }

    IFCEXPECT_RETURN(m_tpGroupDescriptions);
    IFC_RETURN(m_tpGroupDescriptions.CopyTo(ppValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_Filter(_Outptr_result_maybenull_ xaml_data::ICollectionViewFilter** ppValue)
{
    ctl::ComPtr<CollectionViewSource> spHost;
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(GetDetachedHost(spHost.ReleaseAndGetAddressOf()));
    if (spHost)
    {
        IFC_RETURN(spHost->get_FilterImpl(ppValue));
        return S_OK;
    }

    IFC_RETURN(m_tpFilter.CopyTo(ppValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::put_Filter(_In_opt_ xaml_data::ICollectionViewFilter* pValue)
{
    ctl::ComPtr<CollectionViewSource> spHost;
    IFC_RETURN(GetDetachedHost(spHost.ReleaseAndGetAddressOf()));
    if (spHost)
    {
        IFC_RETURN(spHost->put_FilterImpl(pValue));
        return S_OK;
    }

    SetPtrValue(m_tpFilter, pValue);
    IFC_RETURN(OnShapingRulesChanged());
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_CustomSort(_Outptr_result_maybenull_ xaml_data::ICollectionViewComparer** ppValue)
{
    ctl::ComPtr<CollectionViewSource> spHost;
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(GetDetachedHost(spHost.ReleaseAndGetAddressOf()));
    if (spHost)
    {
        IFC_RETURN(spHost->get_CustomSortImpl(ppValue));
        return S_OK;
    }

    IFC_RETURN(m_tpCustomSort.CopyTo(ppValue));
    return S_OK;
}

// As in WPF, a CustomSort replaces the SortDescriptions, and adding a SortDescription drops the CustomSort.
IFACEMETHODIMP VectorCollectionView::put_CustomSort(_In_opt_ xaml_data::ICollectionViewComparer* pValue)
{
    ctl::ComPtr<CollectionViewSource> spHost;
    UINT sortCount = 0;

    IFC_RETURN(GetDetachedHost(spHost.ReleaseAndGetAddressOf()));
    if (spHost)
    {
        IFC_RETURN(spHost->put_CustomSortImpl(pValue));
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

    IFC_RETURN(OnShapingRulesChanged());
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::Refresh()
{
    ctl::ComPtr<CollectionViewSource> spHost;
    IFC_RETURN(GetDetachedHost(spHost.ReleaseAndGetAddressOf()));
    if (spHost)
    {
        IFC_RETURN(spHost->RefreshImpl());
        return S_OK;
    }

    IFC_RETURN(RefreshOrDefer());
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::DeferRefresh(_Outptr_ wf::IClosable** ppReturnValue)
{
    ctl::ComPtr<CollectionViewSource> spHost;
    Microsoft::WRL::ComPtr<CollectionViewRefreshDeferral<VectorCollectionView>> spDeferral;

    ARG_VALIDRETURNPOINTER(ppReturnValue);
    *ppReturnValue = nullptr;
    IFC_RETURN(GetDetachedHost(spHost.ReleaseAndGetAddressOf()));
    if (spHost)
    {
        IFC_RETURN(spHost->DeferRefreshImpl(ppReturnValue));
        return S_OK;
    }

    IFC_RETURN(wrl::MakeAndInitialize<CollectionViewRefreshDeferral<VectorCollectionView>>(&spDeferral, this));
    ++m_deferRefreshLevel;
    *ppReturnValue = spDeferral.Detach();
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::EndDeferRefresh()
{
    ASSERT(m_deferRefreshLevel > 0);
    if (m_deferRefreshLevel == 0 || --m_deferRefreshLevel > 0 || !m_isRefreshPending)
    {
        return S_OK;
    }

    m_isRefreshPending = false;
    IFC_RETURN(OnShapingRulesChanged());
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::OnSortDescriptionsChanged()
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

    IFC_RETURN(OnShapingRulesChanged());
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::HasShapingRules(_Out_ bool* pHasRules)
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

    *pHasRules = sortCount > 0 || groupCount > 0 || m_tpFilter || m_tpCustomSort;
    return S_OK;
}

// A rule changed (WPF: RefreshOrDefer after a SortDescriptions, GroupDescriptions or Filter change).
_Check_return_ HRESULT VectorCollectionView::OnShapingRulesChanged()
{
    UINT groupCount = 0;
    bool hasRules = false;

    if (!m_isShapingEnabled || m_isShapingDetached || m_isComputingShape)
    {
        return S_OK;
    }

    if (m_deferRefreshLevel > 0)
    {
        m_isRefreshPending = true;
        return S_OK;
    }

    if (m_tpGroupDescriptions)
    {
        IFC_RETURN(m_tpGroupDescriptions->get_Size(&groupCount));
    }
    const bool isGrouping = (groupCount > 0);

    // Reshaped in place, so a consumer holding the view keeps a live view.
    if (m_tpInternalList && isGrouping == m_isGrouping)
    {
        IFC_RETURN(RefreshOrDefer());
        return S_OK;
    }

    // An unshaped view stays untouched until a rule is set.
    IFC_RETURN(HasShapingRules(&hasRules));
    if (!m_tpInternalList && !hasRules)
    {
        return S_OK;
    }

    // Controls read CollectionGroups once per ItemsSource, so when a CollectionViewSource publishes this view,
    // switching between flat and grouped output gives it a new View, as changing IsSourceGrouped does.
    if (isGrouping != m_isGrouping && m_hostRef)
    {
        ctl::ComPtr<xaml_data::ICollectionViewSource> spHost;
        bool isHandled = false;
        IFC_RETURN(m_hostRef.As(&spHost));
        if (spHost)
        {
            IFC_RETURN(static_cast<CollectionViewSource*>(spHost.Get())->OnShapingViewGroupingChanged(this, &isHandled));
            if (isHandled)
            {
                return S_OK;
            }
        }
    }

    IFC_RETURN(StartShaping(isGrouping));
    return S_OK;
}

// Computes the output into a new InternalList and presents it: flat, or through a GroupedDataCollectionView.
// Announced as one Reset; currency follows the item.
_Check_return_ HRESULT VectorCollectionView::StartShaping(bool isGrouping)
{
    ctl::ComPtr<InternalList> spOutput;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spPreviousOutput = m_tpInternalList.Get();
    const bool wasGrouping = m_isGrouping;
    HRESULT hr = S_OK;

    IFC_RETURN(ctl::ComObject<InternalList>::CreateInstance(spOutput.ReleaseAndGetAddressOf()));

    m_isGrouping = isGrouping;
    SetPtrValue(m_tpInternalList, static_cast<wfc::IVector<IInspectable*>*>(spOutput.Get()));

    m_isComputingShape = true;
    hr = RefreshOverride();
    m_isComputingShape = false;
    if (FAILED(hr))
    {
        m_isGrouping = wasGrouping;
        SetPtrValue(m_tpInternalList, spPreviousOutput.Get());
        m_areSourceIndicesValid = false;
        IFC_RETURN(hr);
    }

    // The output is read-only, so index-based writes through the view fail instead of reordering the source.
    if (isGrouping)
    {
        ctl::ComPtr<xaml_data::ICollectionView> spGrouped;
        ctl::ComPtr<wfc::IVector<IInspectable*>> spGroupedItems;
        IFC_RETURN(GroupedDataCollectionView::CreateInstance(spOutput.Get(), nullptr /* pItemsPath */, spGrouped.ReleaseAndGetAddressOf()));
        IFC_RETURN(spGrouped.As(&spGroupedItems));
        SetPtrValue(m_tpGroupedPresentation, spGrouped.Get());
        IFC_RETURN(RetargetSource(spGroupedItems.Get()));
    }
    else
    {
        m_tpGroupedPresentation.Clear();
        IFC_RETURN(RetargetSource(spOutput.Get()));
    }

    if (!m_epSourceListChangedHandler)
    {
        auto spObservable = m_tpSourceList.AsOrNull<wfc::IObservableVector<IInspectable*>>();
        if (spObservable)
        {
            SetPtrValue(m_tpSourceListObservable, spObservable.Get());
            IFC_RETURN(m_epSourceListChangedHandler.AttachEventHandler(m_tpSourceListObservable.Get(),
                [this](wfc::IObservableVector<IInspectable*>* pSender, wfc::IVectorChangedEventArgs* pArgs)
                {
                    auto pegThis = ctl::try_make_autopeg(this);
                    if (pegThis)
                    {
                        IFC_RETURN(ProcessCollectionChanged(pArgs));
                    }
                    return S_OK;
                }));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::get_CollectionGroupsImpl(_Out_ wfc::IObservableVector<IInspectable *> **value)
{
    if (m_tpGroupedPresentation)
    {
        IFC_RETURN(m_tpGroupedPresentation->get_CollectionGroups(value));
        return S_OK;
    }

    IFC_RETURN(CollectionView::get_CollectionGroupsImpl(value));
    return S_OK;
}

// Full recompute (WPF: ListCollectionView.RefreshOverride): filter, then stable multi-key sort, then group by
// first appearance.
_Check_return_ HRESULT VectorCollectionView::RefreshOverride()
{
    struct Entry
    {
        ctl::ComPtr<IInspectable> item;
        std::vector<ctl::ComPtr<IInspectable>> sortKeys;
        ctl::ComPtr<IInspectable> groupKey;
        UINT sourceIndex = 0;
    };

    SortFieldComparer sortComparer;
    ShapingKeySelector groupSelector;
    bool hasGroupSelector = false;
    ctl::ComPtr<xaml_data::IValueConverter> spGroupConverter;
    wrl_wrappers::HStringReference groupKeyTypeName(L"Object");
    wxaml_interop::TypeName groupKeyType = { groupKeyTypeName.Get(), wxaml_interop::TypeKind_Primitive };
    ctl::ComPtr<xaml_data::ICollectionViewFilter> spFilter = m_tpFilter.Get();
    std::vector<Entry> entries;
    std::vector<ctl::ComPtr<IInspectable>> outputItems;
    std::vector<UINT> outputSourceIndices;
    const UINT sourceChangeCount = m_sourceListChangeCount;
    UINT size = 0;

    IFC_RETURN(sortComparer.Initialize(m_tpSortDescriptions.Get(), m_tpCustomSort.Get()));

    // ICollectionViewGroup is single level, so only the first group description is used.
    if (m_isGrouping)
    {
        UINT count = 0;
        IFC_RETURN(m_tpGroupDescriptions->get_Size(&count));
        for (UINT i = 0; i < count && !hasGroupSelector; ++i)
        {
            ctl::ComPtr<xaml_data::IPropertyGroupDescription> spDescription;
            IFC_RETURN(m_tpGroupDescriptions->GetAt(i, spDescription.ReleaseAndGetAddressOf()));
            if (spDescription)
            {
                wrl_wrappers::HString path;
                IFC_RETURN(spDescription->get_PropertyName(path.GetAddressOf()));
                IFC_RETURN(CreateKeySelector(path.Get(), &groupSelector));
                IFC_RETURN(spDescription->get_Converter(spGroupConverter.ReleaseAndGetAddressOf()));
                hasGroupSelector = true;
            }
        }
    }

    IFC_RETURN(m_tpSourceList->get_Size(&size));
    entries.reserve(size);

    for (UINT i = 0; i < size; ++i)
    {
        Entry entry;
        entry.sourceIndex = i;
        IFC_RETURN(m_tpSourceList->GetAt(i, entry.item.ReleaseAndGetAddressOf()));

        if (spFilter)
        {
            boolean accepted = true;
            IFC_RETURN(spFilter->Invoke(entry.item.Get(), &accepted));
            if (!accepted)
            {
                continue;
            }
        }

        IFC_RETURN(sortComparer.EvaluateKeys(entry.item.Get(), &entry.sortKeys));

        if (hasGroupSelector)
        {
            IFC_RETURN(EvaluateKey(groupSelector, entry.item.Get(), entry.groupKey.ReleaseAndGetAddressOf()));
            if (spGroupConverter)
            {
                ctl::ComPtr<IInspectable> spConverted;
                IFC_RETURN(spGroupConverter->Convert(entry.groupKey.Get(), groupKeyType, nullptr /* parameter */, nullptr /* language */, spConverted.ReleaseAndGetAddressOf()));
                entry.groupKey = std::move(spConverted);
            }
        }

        entries.push_back(std::move(entry));
    }

    if (!sortComparer.IsEmpty())
    {
        // A CustomSort that fails ends the sort and the recompute; the previous output stays.
        HRESULT hrCompare = S_OK;
        std::stable_sort(entries.begin(), entries.end(), [&sortComparer, &hrCompare](const Entry& left, const Entry& right)
        {
            int result = 0;
            if (SUCCEEDED(hrCompare))
            {
                hrCompare = sortComparer.Compare(left.item.Get(), left.sortKeys, left.sourceIndex, right.item.Get(), &right.sortKeys, right.sourceIndex, &result);
            }
            return SUCCEEDED(hrCompare) && result < 0;
        });
        IFC_RETURN(hrCompare);
    }

    if (!m_isGrouping)
    {
        outputItems.reserve(entries.size());
        outputSourceIndices.reserve(entries.size());
        for (auto& entry : entries)
        {
            outputItems.push_back(std::move(entry.item));
            outputSourceIndices.push_back(entry.sourceIndex);
        }
    }
    else
    {
        CollectionViewGroupRoot root;
        for (const auto& entry : entries)
        {
            IFC_RETURN(root.AddToSubgroups(entry.item.Get(), entry.groupKey.Get()));
        }

        IFC_RETURN(root.GetGroups(&outputItems));
    }

    // Recorded before ReplaceAll raises, so a handler that changes the source sees consistent state.
    m_sourceIndices = std::move(outputSourceIndices);
    m_sourceListSize = size;
    m_areSourceIndicesValid = !m_isGrouping && (sourceChangeCount == m_sourceListChangeCount);
    IFC_RETURN(static_cast<InternalList*>(m_tpInternalList.Get())->ReplaceAll(outputItems));
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::RefreshOrDefer()
{
    ctl::ComPtr<IInspectable> spCurrentItem;
    BOOLEAN isCurrentValid = FALSE;
    HRESULT hr = S_OK;

    // Changes raised while computing (for example from a Filter callback) are ignored.
    if (!m_tpInternalList || m_isComputingShape)
    {
        return S_OK;
    }

    if (m_deferRefreshLevel > 0)
    {
        m_isRefreshPending = true;
        return S_OK;
    }

    IFC_RETURN(get_CurrentItemImpl(spCurrentItem.ReleaseAndGetAddressOf()));

    m_isComputingShape = true;
    hr = RefreshOverride();
    m_isComputingShape = false;
    IFC_RETURN(hr);

    if (spCurrentItem)
    {
        IFC_RETURN(MoveCurrentToImpl(spCurrentItem.Get(), &isCurrentValid));
    }

    if (!isCurrentValid)
    {
        IFC_RETURN(MoveCurrentToFirstImpl(&isCurrentValid));
    }

    return S_OK;
}

void VectorCollectionView::DetachSourceList()
{
    if (m_epSourceListChangedHandler)
    {
        auto spSource = m_tpSourceListObservable.GetSafeReference();
        if (spSource)
        {
            VERIFYHR(m_epSourceListChangedHandler.DetachEventHandler(spSource.Get()));
        }
        else
        {
            m_epSourceListChangedHandler = ctl::EventPtr<VectorChangedEventCallback>();
        }
    }

    m_tpSourceListObservable.Clear();
    m_areSourceIndicesValid = false;
}

// A source change (WPF: ListCollectionView.ProcessCollectionChanged).
_Check_return_ HRESULT VectorCollectionView::ProcessCollectionChanged(_In_ wfc::IVectorChangedEventArgs* pArgs)
{
    wfc::CollectionChange change = wfc::CollectionChange_Reset;
    UINT index = 0;
    bool isApplied = false;

    ++m_sourceListChangeCount;

    // A change raised while computing (for example from a Filter callback) is not applied, and
    // the next change recomputes fully.
    if (!m_tpInternalList || m_isComputingShape)
    {
        m_areSourceIndicesValid = false;
        return S_OK;
    }

    if (m_deferRefreshLevel > 0)
    {
        m_areSourceIndicesValid = false;
        m_isRefreshPending = true;
        return S_OK;
    }

    IFC_RETURN(pArgs->get_CollectionChange(&change));

    // Some vectors (C++/CX Platform::Collections) fail Index on a Reset.
    if (change != wfc::CollectionChange_Reset)
    {
        IFC_RETURN(pArgs->get_Index(&index));
    }

    if (m_areSourceIndicesValid && !m_isGrouping && change != wfc::CollectionChange_Reset)
    {
        IFC_RETURN(ProcessCollectionChangedWithAdjustedIndex(change, index, &isApplied));
    }

    if (!isApplied)
    {
        IFC_RETURN(RefreshOrDefer());
    }

    return S_OK;
}

// Applies one Add, Remove or Replace from the source to a flat shaped view as the matching vector
// change, at the position a full recompute would give the item. Leaves *pIsApplied false when a
// full recompute is needed instead.
_Check_return_ HRESULT VectorCollectionView::ProcessCollectionChangedWithAdjustedIndex(wfc::CollectionChange change, UINT index, _Out_ bool* pIsApplied)
{
    auto pOutput = static_cast<InternalList*>(m_tpInternalList.Get());
    const UINT sourceChangeCount = m_sourceListChangeCount;
    const UINT notFound = static_cast<UINT>(-1);
    UINT sourceSize = 0;
    UINT oldPosition = notFound;
    UINT newPosition = notFound;
    ctl::ComPtr<IInspectable> spItem;
    HRESULT hr = S_OK;

    *pIsApplied = false;

    IFC_RETURN(m_tpSourceList->get_Size(&sourceSize));
    if (change != wfc::CollectionChange_ItemInserted && change != wfc::CollectionChange_ItemRemoved && change != wfc::CollectionChange_ItemChanged)
    {
        return S_OK;
    }

    // The source has already changed; a size or index that does not match ours means a missed change.
    const UINT expectedSize = m_sourceListSize
        + ((change == wfc::CollectionChange_ItemInserted) ? 1 : 0)
        - ((change == wfc::CollectionChange_ItemRemoved) ? 1 : 0);
    if (sourceSize != expectedSize || index >= ((sourceSize > m_sourceListSize) ? sourceSize : m_sourceListSize))
    {
        return S_OK;
    }
    m_sourceListSize = sourceSize;

    auto found = std::find(m_sourceIndices.begin(), m_sourceIndices.end(), index);
    if (change != wfc::CollectionChange_ItemInserted && found != m_sourceIndices.end())
    {
        oldPosition = static_cast<UINT>(found - m_sourceIndices.begin());
    }

    if (change == wfc::CollectionChange_ItemInserted)
    {
        for (auto& sourceIndex : m_sourceIndices)
        {
            if (sourceIndex >= index)
            {
                ++sourceIndex;
            }
        }
    }

    // App code (Filter, property getters, CustomSort) runs only here.
    if (change != wfc::CollectionChange_ItemRemoved)
    {
        IFC_RETURN(m_tpSourceList->GetAt(index, spItem.ReleaseAndGetAddressOf()));

        m_isComputingShape = true;
        hr = [&]() -> HRESULT
        {
            ctl::ComPtr<xaml_data::ICollectionViewFilter> spFilter = m_tpFilter.Get();
            SortFieldComparer comparer;
            std::vector<ctl::ComPtr<IInspectable>> keys;

            if (spFilter)
            {
                boolean accepted = true;
                IFC_RETURN(spFilter->Invoke(spItem.Get(), &accepted));
                if (!accepted)
                {
                    return S_OK;
                }
            }

            IFC_RETURN(comparer.Initialize(m_tpSortDescriptions.Get(), m_tpCustomSort.Get()));
            IFC_RETURN(comparer.EvaluateKeys(spItem.Get(), &keys));

            // Upper bound over the output without the replaced item.
            const UINT count = static_cast<UINT>(m_sourceIndices.size()) - ((oldPosition != notFound) ? 1 : 0);
            UINT low = 0;
            UINT high = count;
            while (low < high)
            {
                const UINT middle = low + (high - low) / 2;
                const UINT position = (oldPosition != notFound && middle >= oldPosition) ? middle + 1 : middle;
                ctl::ComPtr<IInspectable> spOther;
                int result = 0;
                IFC_RETURN(pOutput->GetAt(position, spOther.ReleaseAndGetAddressOf()));
                IFC_RETURN(comparer.Compare(spItem.Get(), keys, index, spOther.Get(), nullptr /* pRightKeys */, m_sourceIndices[position], &result));
                if (result < 0)
                {
                    high = middle;
                }
                else
                {
                    low = middle + 1;
                }
            }

            newPosition = low;
            return S_OK;
        }();
        m_isComputingShape = false;
        IFC_RETURN(hr);

        if (sourceChangeCount != m_sourceListChangeCount)
        {
            return S_OK;
        }
    }

    // Bookkeeping first, then the notifications, each matching the output at the time it is raised.
    m_isComputingShape = true;
    hr = [&]() -> HRESULT
    {
        if (oldPosition != notFound && oldPosition == newPosition)
        {
            IFC_RETURN(pOutput->InternalSetAt(oldPosition, spItem.Get()));
            return S_OK;
        }

        if (oldPosition != notFound)
        {
            m_sourceIndices.erase(m_sourceIndices.begin() + oldPosition);
        }

        if (change == wfc::CollectionChange_ItemRemoved)
        {
            for (auto& sourceIndex : m_sourceIndices)
            {
                if (sourceIndex > index)
                {
                    --sourceIndex;
                }
            }
        }

        if (oldPosition != notFound)
        {
            IFC_RETURN(pOutput->InternalRemoveAt(oldPosition));
        }

        if (newPosition != notFound)
        {
            m_sourceIndices.insert(m_sourceIndices.begin() + newPosition, index);
            IFC_RETURN(pOutput->InternalInsertAt(newPosition, spItem.Get()));
        }

        return S_OK;
    }();
    m_isComputingShape = false;
    IFC_RETURN(hr);

    // A handler changed the source while the change was raised: recompute fully.
    *pIsApplied = (sourceChangeCount == m_sourceListChangeCount);
    return S_OK;
}
