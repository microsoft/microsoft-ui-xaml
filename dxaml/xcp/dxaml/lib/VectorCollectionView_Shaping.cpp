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
#include "PropertyGroupDescription.g.h"
#include "VectorCollectionView.g.h"
#include "GroupedDataCollectionView.g.h"
#include "PropertyChangedEventArgs.g.h"
#include "FrameworkElement.g.h"
#include "CollectionViewRefreshDeferral.h"
#include "PropertyPathParser.h"
#include "PropertyPath.h"
#include "Callback.h"
#include "CollectionViewLiveShapingProperties.h"

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

    // A computed group (WPF: MS.Internal.Data.CollectionViewGroupInternal). As a vector it holds the group's
    // leaves (the items of all its subgroups, in order), so a top-level group is the GroupItems of its
    // ICollectionViewGroup. As an ICollectionViewGroup itself, Group is the group and GroupItems is WPF's
    // CollectionViewGroup.Items: the subgroups, or the items at the bottom level. Bindings see Key (the
    // IGrouping<K,T> idiom), Name, Items, ItemCount and IsBottomLevel; as in WPF, a change to ItemCount raises
    // PropertyChanged.
    class __declspec(novtable) CollectionViewGroupInternal :
        public InternalList,
        public xaml_data::ICustomPropertyProvider,
        public xaml_data::ICollectionViewGroup,
        public xaml_data::INotifyPropertyChanged
    {
        BEGIN_INTERFACE_MAP(CollectionViewGroupInternal, InternalList)
            INTERFACE_ENTRY(CollectionViewGroupInternal, xaml_data::ICustomPropertyProvider)
            INTERFACE_ENTRY(CollectionViewGroupInternal, xaml_data::ICollectionViewGroup)
            INTERFACE_ENTRY(CollectionViewGroupInternal, xaml_data::INotifyPropertyChanged)
        END_INTERFACE_MAP(CollectionViewGroupInternal, InternalList)

    public:
        _Check_return_ HRESULT InitializeGroup(_In_opt_ IInspectable* pKey, _In_opt_ CollectionViewGroupInternal* pParent, bool isBottomLevel)
        {
            SetPtrValue(m_tpKey, pKey);
            m_pParent = pParent;
            if (!isBottomLevel)
            {
                ctl::ComPtr<InternalList> spSubgroupList;
                IFC_RETURN(ctl::ComObject<InternalList>::CreateInstance(spSubgroupList.ReleaseAndGetAddressOf()));
                SetPtrValue(m_tpSubgroupList, spSubgroupList.Get());
            }
            return S_OK;
        }

        IInspectable* GetKeyNoRef() const
        {
            return m_tpKey.Get();
        }

        CollectionViewGroupInternal* GetParentNoRef() const
        {
            return m_pParent;
        }

        void ClearParent()
        {
            m_pParent = nullptr;
        }

        bool IsBottomLevel() const
        {
            return !m_tpSubgroupList.Get();
        }

        virtual bool IsRoot() const
        {
            return false;
        }

        UINT LeafCount() const
        {
            return Size();
        }

        const std::vector<ctl::ComPtr<CollectionViewGroupInternal>>& Subgroups() const
        {
            return m_subgroups;
        }

        // Bottom level: the source index of each item.
        std::vector<UINT>& SourceIndices()
        {
            return m_sourceIndices;
        }

        UINT FirstSourceIndex() const
        {
            const CollectionViewGroupInternal* pGroup = this;
            while (!pGroup->IsBottomLevel() && !pGroup->m_subgroups.empty())
            {
                pGroup = pGroup->m_subgroups.front().Get();
            }
            return pGroup->m_sourceIndices.empty() ? 0 : pGroup->m_sourceIndices.front();
        }

        UINT IndexOfSubgroup(_In_ const CollectionViewGroupInternal* pSubgroup) const
        {
            for (size_t i = 0; i < m_subgroups.size(); ++i)
            {
                if (m_subgroups[i].Get() == pSubgroup)
                {
                    return static_cast<UINT>(i);
                }
            }
            return static_cast<UINT>(m_subgroups.size());
        }

        // The number of leaves in the subgroups before pSubgroup (WPF: LeafIndexFromItem, one level).
        UINT LeafCountBefore(_In_ const CollectionViewGroupInternal* pSubgroup) const
        {
            UINT count = 0;
            for (const auto& spSubgroup : m_subgroups)
            {
                if (spSubgroup.Get() == pSubgroup)
                {
                    break;
                }
                count += spSubgroup->LeafCount();
            }
            return count;
        }

        _Check_return_ HRESULT InsertSubgroup(UINT index, _In_ CollectionViewGroupInternal* pSubgroup, bool isRaised)
        {
            m_subgroups.insert(m_subgroups.begin() + index, ctl::ComPtr<CollectionViewGroupInternal>(pSubgroup));
            auto pSubgroupItem = ctl::as_iinspectable(pSubgroup);
            if (isRaised)
            {
                IFC_RETURN(m_tpSubgroupList->InternalInsertAt(index, pSubgroupItem));
            }
            else
            {
                IFC_RETURN(m_tpSubgroupList->TrackerCollection<IInspectable*>::InsertAt(index, pSubgroupItem));
            }
            return S_OK;
        }

        _Check_return_ HRESULT RemoveSubgroup(UINT index)
        {
            m_subgroups[index]->ClearParent();
            m_subgroups.erase(m_subgroups.begin() + index);
            IFC_RETURN(m_tpSubgroupList->InternalRemoveAt(index));
            return S_OK;
        }

        _Check_return_ HRESULT InsertLeaf(UINT index, _In_opt_ IInspectable* pItem, bool isRaised)
        {
            if (isRaised)
            {
                IFC_RETURN(InternalInsertAt(index, pItem));
                IFC_RETURN(RaiseItemCountChanged());
            }
            else
            {
                IFC_RETURN(TrackerCollection<IInspectable*>::InsertAt(index, pItem));
            }
            return S_OK;
        }

        _Check_return_ HRESULT RemoveLeaf(UINT index)
        {
            IFC_RETURN(InternalRemoveAt(index));
            IFC_RETURN(RaiseItemCountChanged());
            return S_OK;
        }

        // INotifyPropertyChanged
        IFACEMETHODIMP add_PropertyChanged(_In_ xaml_data::IPropertyChangedEventHandler* pHandler, _Out_ EventRegistrationToken* pToken) override
        {
            ARG_NOTNULL_RETURN(pHandler, "handler");
            ARG_VALIDRETURNPOINTER(pToken);
            m_propertyChangedHandlers.AddHandler(pHandler, pToken);
            return S_OK;
        }

        IFACEMETHODIMP remove_PropertyChanged(EventRegistrationToken token) override
        {
            IFC_RETURN(m_propertyChangedHandlers.RemoveHandler(token));
            return S_OK;
        }

        _Check_return_ HRESULT SetLeaf(UINT index, _In_opt_ IInspectable* pItem)
        {
            IFC_RETURN(InternalSetAt(index, pItem));
            return S_OK;
        }

        // ICollectionViewGroup
        IFACEMETHODIMP get_Group(_Outptr_ IInspectable** ppValue) override
        {
            *ppValue = ctl::as_iinspectable(this);
            (*ppValue)->AddRef();
            return S_OK;
        }

        IFACEMETHODIMP get_GroupItems(_Outptr_ wfc::IObservableVector<IInspectable*>** ppValue) override
        {
            ctl::ComPtr<IInspectable> spItems = m_tpSubgroupList ? ctl::as_iinspectable(m_tpSubgroupList.Get()) : ctl::as_iinspectable(this);
            ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spVector;
            IFC_RETURN(spItems.As(&spVector));
            *ppValue = spVector.Detach();
            return S_OK;
        }

        // ICustomPropertyProvider
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
                IFC_RETURN(CustomProperty::CreateObjectProperty(name, [](IInspectable* pTarget, IInspectable** ppResult)
                {
                    return FromTarget(pTarget)->m_tpKey.CopyTo(ppResult);
                }, ppValue));
            }
            else if (wrl_wrappers::HStringReference(L"Items") == name)
            {
                IFC_RETURN(CustomProperty::CreateObjectProperty(name, [](IInspectable* pTarget, IInspectable** ppResult)
                {
                    return FromTarget(pTarget)->get_GroupItems(reinterpret_cast<wfc::IObservableVector<IInspectable*>**>(ppResult));
                }, ppValue));
            }
            else if (wrl_wrappers::HStringReference(L"ItemCount") == name)
            {
                IFC_RETURN(CustomProperty::CreateInt32Property(name, [](IInspectable* pTarget, IInspectable** ppResult)
                {
                    return DirectUI::PropertyValue::CreateFromInt32(static_cast<INT32>(FromTarget(pTarget)->LeafCount()), ppResult);
                }, ppValue));
            }
            else if (wrl_wrappers::HStringReference(L"IsBottomLevel") == name)
            {
                IFC_RETURN(CustomProperty::CreateBooleanProperty(name, [](IInspectable* pTarget, IInspectable** ppResult)
                {
                    return DirectUI::PropertyValue::CreateFromBoolean(FromTarget(pTarget)->IsBottomLevel(), ppResult);
                }, ppValue));
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
            if (m_tpKey)
            {
                IFC_RETURN(FrameworkElement::GetStringFromObject(m_tpKey.Get(), pValue));
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
            else if (InlineIsEqualGUID(iid, __uuidof(xaml_data::ICollectionViewGroup)))
            {
                *ppObject = static_cast<xaml_data::ICollectionViewGroup*>(this);
            }
            else if (InlineIsEqualGUID(iid, __uuidof(xaml_data::INotifyPropertyChanged)))
            {
                *ppObject = static_cast<xaml_data::INotifyPropertyChanged*>(this);
            }
            else
            {
                return InternalList::QueryInterfaceImpl(iid, ppObject);
            }

            AddRefOuter();
            return S_OK;
        }

        // The subgroups, typed, and as the observable list GroupItems presents (the root's is the InternalList).
        // A tracked reference, so the reference tracker walk reaches nested groups (their keys and handlers).
        std::vector<ctl::ComPtr<CollectionViewGroupInternal>> m_subgroups;
        TrackerPtr<InternalList> m_tpSubgroupList;

    private:
        static CollectionViewGroupInternal* FromTarget(_In_ IInspectable* pTarget)
        {
            ctl::ComPtr<IInspectable> spTarget(pTarget);
            ctl::ComPtr<xaml_data::ICustomPropertyProvider> spProvider;
            VERIFYHR(spTarget.As(&spProvider));
            return spProvider.Cast<CollectionViewGroupInternal>();
        }

        void OnReferenceTrackerWalk(INT walkType) override
        {
            m_propertyChangedHandlers.ReferenceTrackerWalk(static_cast<EReferenceTrackerWalkType>(walkType));
            InternalList::OnReferenceTrackerWalk(walkType);
        }

        _Check_return_ HRESULT RaiseItemCountChanged()
        {
            if (m_propertyChangedHandlers.Empty())
            {
                return S_OK;
            }

            ctl::ComPtr<DirectUI::PropertyChangedEventArgs> spArgs;
            IFC_RETURN(ctl::make(&spArgs));
            IFC_RETURN(spArgs->put_PropertyName(wrl_wrappers::HStringReference(L"ItemCount").Get()));
            IFC_RETURN(m_propertyChangedHandlers.Raise(ctl::as_iinspectable(this), spArgs.Get()));
            return S_OK;
        }

        // A key can be an app object (a Converter result), so it is walked.
        TrackerPtr<IInspectable> m_tpKey;
        TrackerEventSource<xaml_data::IPropertyChangedEventHandler, IInspectable, xaml_data::IPropertyChangedEventArgs> m_propertyChangedHandlers;
        CollectionViewGroupInternal* m_pParent = nullptr;
        std::vector<UINT> m_sourceIndices;
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

    // Integral values compare exactly (a double holds only 53 bits).
    bool TryGetIntegerValue(_In_ wf::IPropertyValue* pValue, wf::PropertyType type, _Out_ bool* pIsNegative, _Out_ UINT64* pMagnitude)
    {
        INT64 signedValue = 0;
        UINT64 unsignedValue = 0;
        bool isSigned = true;
        HRESULT hr = S_OK;

        switch (type)
        {
        case wf::PropertyType_UInt8:  { BYTE v = 0;   hr = pValue->GetUInt8(&v);  unsignedValue = v; isSigned = false; break; }
        case wf::PropertyType_Char16: { WCHAR v = 0;  hr = pValue->GetChar16(&v); unsignedValue = v; isSigned = false; break; }
        case wf::PropertyType_UInt16: { UINT16 v = 0; hr = pValue->GetUInt16(&v); unsignedValue = v; isSigned = false; break; }
        case wf::PropertyType_UInt32: { UINT32 v = 0; hr = pValue->GetUInt32(&v); unsignedValue = v; isSigned = false; break; }
        case wf::PropertyType_UInt64: { hr = pValue->GetUInt64(&unsignedValue); isSigned = false; break; }
        case wf::PropertyType_Int16:  { INT16 v = 0;  hr = pValue->GetInt16(&v);  signedValue = v; break; }
        case wf::PropertyType_Int32:  { INT32 v = 0;  hr = pValue->GetInt32(&v);  signedValue = v; break; }
        case wf::PropertyType_Int64:  { hr = pValue->GetInt64(&signedValue); break; }
        default: return false;
        }

        if (FAILED(hr))
        {
            return false;
        }

        *pIsNegative = isSigned && signedValue < 0;
        *pMagnitude = !isSigned ? unsignedValue : (*pIsNegative ? (0 - static_cast<UINT64>(signedValue)) : static_cast<UINT64>(signedValue));
        return true;
    }

    // Values that cannot be ordered compare equal, so the stable sort keeps their source order. Strings compare
    // in the view's culture (WPF: CollectionView.Culture), else the user's.
    int CompareKeys(_In_opt_ IInspectable* pLeft, _In_opt_ IInspectable* pRight, _In_z_ LPCWSTR localeName)
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
            const int result = CompareStringEx(localeName, 0, szLeft, static_cast<int>(leftLength), szRight, static_cast<int>(rightLength), nullptr, nullptr, 0);
            return (result == 0) ? 0 : result - CSTR_EQUAL;
        }

        if (DirectUI::PropertyValue::IsNumericPropertyType(leftType) && DirectUI::PropertyValue::IsNumericPropertyType(rightType))
        {
            bool isLeftNegative = false;
            bool isRightNegative = false;
            UINT64 leftMagnitude = 0;
            UINT64 rightMagnitude = 0;
            if (TryGetIntegerValue(spLeft.Get(), leftType, &isLeftNegative, &leftMagnitude) &&
                TryGetIntegerValue(spRight.Get(), rightType, &isRightNegative, &rightMagnitude))
            {
                if (isLeftNegative != isRightNegative)
                {
                    return isLeftNegative ? -1 : 1;
                }
                const int magnitudeOrder = (leftMagnitude < rightMagnitude) ? -1 : ((leftMagnitude > rightMagnitude) ? 1 : 0);
                return isLeftNegative ? -magnitudeOrder : magnitudeOrder;
            }

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
            _In_opt_ xaml_data::ICollectionViewComparer* pCustomSort,
            _In_opt_ HSTRING culture)
        {
            UINT count = 0;
            UINT32 cultureLength = 0;
            LPCWSTR szCulture = WindowsGetStringRawBuffer(culture, &cultureLength);

            m_fields.clear();
            m_spCustomSort = pCustomSort;
            m_localeName = (cultureLength > 0) ? std::wstring(szCulture, cultureLength) : std::wstring();
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

                    const int result = CompareKeys(leftKeys[i].Get(), spRightKey.Get(), m_localeName.empty() ? LOCALE_NAME_USER_DEFAULT : m_localeName.c_str());
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
        std::wstring m_localeName;
    };

    _Check_return_ HRESULT TryGetString(_In_opt_ IInspectable* pValue, _Out_ wrl_wrappers::HString* pString, _Out_ bool* pIsString)
    {
        auto spValue = ctl::query_interface_cast<wf::IPropertyValue>(pValue);
        wf::PropertyType type = wf::PropertyType_Empty;

        *pIsString = false;
        if (spValue && SUCCEEDED(spValue->get_Type(&type)) && type == wf::PropertyType_String)
        {
            pString->Release();
            IFC_RETURN(spValue->GetString(pString->GetAddressOf()));
            *pIsString = true;
        }
        return S_OK;
    }

    // WPF: PropertyGroupDescription.NamesMatch, String.Equals(groupName, itemName, StringComparison).
    _Check_return_ HRESULT GroupNamesMatch(xaml_data::CollectionViewStringComparison comparison, _In_opt_ IInspectable* pGroupName, _In_opt_ IInspectable* pItemName, _Out_ bool* pMatch)
    {
        wrl_wrappers::HString groupName;
        wrl_wrappers::HString itemName;
        bool isGroupNameString = false;
        bool isItemNameString = false;

        IFC_RETURN(TryGetString(pGroupName, &groupName, &isGroupNameString));
        IFC_RETURN(TryGetString(pItemName, &itemName, &isItemNameString));
        if (!isGroupNameString || !isItemNameString)
        {
            IFC_RETURN(DirectUI::PropertyValue::AreEqual(pGroupName, pItemName, pMatch));
            return S_OK;
        }

        UINT32 groupLength = 0;
        UINT32 itemLength = 0;
        LPCWSTR szGroup = groupName.GetRawBuffer(&groupLength);
        LPCWSTR szItem = itemName.GetRawBuffer(&itemLength);
        int result = 0;

        switch (comparison)
        {
        case xaml_data::CollectionViewStringComparison_Ordinal:
        case xaml_data::CollectionViewStringComparison_OrdinalIgnoreCase:
            result = CompareStringOrdinal(szGroup, static_cast<int>(groupLength), szItem, static_cast<int>(itemLength), comparison == xaml_data::CollectionViewStringComparison_OrdinalIgnoreCase);
            break;

        default:
        {
            const bool isInvariant = comparison == xaml_data::CollectionViewStringComparison_InvariantCulture || comparison == xaml_data::CollectionViewStringComparison_InvariantCultureIgnoreCase;
            const bool isIgnoreCase = comparison == xaml_data::CollectionViewStringComparison_CurrentCultureIgnoreCase || comparison == xaml_data::CollectionViewStringComparison_InvariantCultureIgnoreCase;
            result = CompareStringEx(isInvariant ? LOCALE_NAME_INVARIANT : LOCALE_NAME_USER_DEFAULT, isIgnoreCase ? NORM_IGNORECASE : 0, szGroup, static_cast<int>(groupLength), szItem, static_cast<int>(itemLength), nullptr, nullptr, 0);
            break;
        }
        }

        *pMatch = (result == CSTR_EQUAL);
        return S_OK;
    }

    // Each group above pChild, up to the root, gets the change at the item's index among its leaves.
    template <typename TChange>
    _Check_return_ HRESULT ChangeAncestorLeaves(_In_ CollectionViewGroupInternal* pChild, UINT offset, const TChange& changeLeaf)
    {
        for (auto pGroup = pChild->GetParentNoRef(); pGroup && !pGroup->IsRoot(); pChild = pGroup, pGroup = pGroup->GetParentNoRef())
        {
            offset += pGroup->LeafCountBefore(pChild);
            IFC_RETURN(changeLeaf(pGroup, offset));
        }
        return S_OK;
    }

    // One level of grouping: the group name of an item (WPF: PropertyGroupDescription.GroupNameFromItem / NamesMatch).
    struct GroupLevel
    {
        bool hasDescription = false;
        ShapingKeySelector selector;
        ctl::ComPtr<xaml_data::IValueConverter> spConverter;
        xaml_data::CollectionViewStringComparison stringComparison = xaml_data::CollectionViewStringComparison_Ordinal;
        // Group order (WPF: GroupDescription.SortDescriptions / CustomSort); else first appearance.
        bool hasGroupOrder = false;
        SortFieldComparer groupSort;
        ctl::ComPtr<xaml_data::ICollectionViewComparer> spGroupCustomSort;
    };

    // The top of the group tree (WPF: MS.Internal.Data.CollectionViewGroupRoot). Its subgroups are the view's
    // InternalList; its own leaves are not kept.
    class __declspec(novtable) CollectionViewGroupRoot : public CollectionViewGroupInternal
    {
    public:
        _Check_return_ HRESULT InitializeRoot(_In_ wfc::IVector<xaml_data::PropertyGroupDescription*>* pDescriptions, _In_ VectorCollectionView* pOwner, _In_opt_ HSTRING culture)
        {
            UINT count = 0;

            IFC_RETURN(InitializeGroup(nullptr, nullptr, false /* isBottomLevel */));
            IFC_RETURN(m_culture.Set(culture));
            IFC_RETURN(pDescriptions->get_Size(&count));
            m_levels.clear();
            m_levels.resize(count);
            for (UINT i = 0; i < count; ++i)
            {
                ctl::ComPtr<xaml_data::IPropertyGroupDescription> spDescription;
                ctl::ComPtr<PropertyGroupDescription> spImpl;
                auto& level = m_levels[i];
                IFC_RETURN(pDescriptions->GetAt(i, spDescription.ReleaseAndGetAddressOf()));
                if (!spDescription)
                {
                    continue;
                }

                wrl_wrappers::HString path;
                IFC_RETURN(spDescription->get_PropertyName(path.GetAddressOf()));
                IFC_RETURN(CreateKeySelector(path.Get(), &level.selector));
                IFC_RETURN(spDescription->get_Converter(level.spConverter.ReleaseAndGetAddressOf()));
                level.hasDescription = true;

                IFC_RETURN(spDescription.As(&spImpl));
                IFC_RETURN(spImpl->get_StringComparison(&level.stringComparison));
                IFC_RETURN(spImpl->get_CustomSortImpl(level.spGroupCustomSort.ReleaseAndGetAddressOf()));
                IFC_RETURN(level.groupSort.Initialize(spImpl->GetSortDescriptionsNoRef(), nullptr /* pCustomSort */, culture));
                level.hasGroupOrder = level.spGroupCustomSort || !level.groupSort.IsEmpty();
                IFC_RETURN(spImpl->AddOwner(pOwner));
            }

            return S_OK;
        }

        bool IsRoot() const override
        {
            return true;
        }

        UINT LevelCount() const
        {
            return static_cast<UINT>(m_levels.size());
        }

        // After a recompute: the subgroups are presented through pList, which already holds them.
        void PresentSubgroups(_In_ InternalList* pList)
        {
            SetPtrValue(m_tpSubgroupList, pList);
        }

        _Check_return_ HRESULT GetGroupNames(_In_opt_ IInspectable* pItem, _Out_ std::vector<ctl::ComPtr<IInspectable>>* pNames)
        {
            wrl_wrappers::HStringReference objectTypeName(L"Object");
            wxaml_interop::TypeName objectType = { objectTypeName.Get(), wxaml_interop::TypeKind_Primitive };

            pNames->clear();
            pNames->resize(m_levels.size());
            for (size_t level = 0; level < m_levels.size(); ++level)
            {
                const auto& groupLevel = m_levels[level];
                if (!groupLevel.hasDescription)
                {
                    continue;
                }

                IFC_RETURN(EvaluateKey(groupLevel.selector, pItem, (*pNames)[level].ReleaseAndGetAddressOf()));
                if (groupLevel.spConverter)
                {
                    ctl::ComPtr<IInspectable> spConverted;
                    IFC_RETURN(groupLevel.spConverter->Convert((*pNames)[level].Get(), objectType, nullptr /* parameter */, m_culture.Get(), spConverted.ReleaseAndGetAddressOf()));
                    (*pNames)[level] = std::move(spConverted);
                }
            }

            return S_OK;
        }

        _Check_return_ HRESULT NamesMatch(UINT level, _In_opt_ IInspectable* pGroupName, _In_opt_ IInspectable* pItemName, _Out_ bool* pMatch) const
        {
            IFC_RETURN(GroupNamesMatch(m_levels[level].stringComparison, pGroupName, pItemName, pMatch));
            return S_OK;
        }

        bool HasGroupOrder(UINT level) const
        {
            return m_levels[level].hasGroupOrder;
        }

        // Negative, zero or positive as group x sorts before, with or after group y (WPF: the group comparer
        // CollectionViewGroupInternal.FindIndex uses). CustomSort compares the group names.
        _Check_return_ HRESULT CompareGroups(UINT level, _In_ CollectionViewGroupInternal* pLeft, _In_ CollectionViewGroupInternal* pRight, _Out_ int* pResult) const
        {
            const auto& groupLevel = m_levels[level];
            *pResult = 0;

            if (groupLevel.spGroupCustomSort)
            {
                INT32 result = 0;
                IFC_RETURN(groupLevel.spGroupCustomSort->Invoke(pLeft->GetKeyNoRef(), pRight->GetKeyNoRef(), &result));
                *pResult = (result > 0) - (result < 0);
                return S_OK;
            }

            std::vector<ctl::ComPtr<IInspectable>> leftKeys;
            IFC_RETURN(groupLevel.groupSort.EvaluateKeys(ctl::as_iinspectable(pLeft), &leftKeys));
            IFC_RETURN(groupLevel.groupSort.Compare(ctl::as_iinspectable(pLeft), leftKeys, 0, ctl::as_iinspectable(pRight), nullptr /* pRightKeys */, 0, pResult));
            return S_OK;
        }

        // With a group order, the index of the first sibling that sorts after the new group; ties go after.
        _Check_return_ HRESULT FindOrderedSubgroupPosition(_In_ CollectionViewGroupInternal* pParent, UINT level, _In_ CollectionViewGroupInternal* pSubgroup, _Out_ UINT* pPosition) const
        {
            const auto& subgroups = pParent->Subgroups();
            UINT position = 0;
            for (; position < subgroups.size(); ++position)
            {
                int result = 0;
                IFC_RETURN(CompareGroups(level, pSubgroup, subgroups[position].Get(), &result));
                if (result < 0)
                {
                    break;
                }
            }
            *pPosition = position;
            return S_OK;
        }

        // The subgroup of pGroup named pName, or null.
        _Check_return_ HRESULT FindSubgroup(_In_ CollectionViewGroupInternal* pGroup, UINT level, _In_opt_ IInspectable* pName, _Outptr_result_maybenull_ CollectionViewGroupInternal** ppSubgroup) const
        {
            *ppSubgroup = nullptr;
            for (const auto& spSubgroup : pGroup->Subgroups())
            {
                bool isMatch = false;
                IFC_RETURN(NamesMatch(level, spSubgroup->GetKeyNoRef(), pName, &isMatch));
                if (isMatch)
                {
                    *ppSubgroup = spSubgroup.Get();
                    break;
                }
            }

            return S_OK;
        }

        // Whether names (one per level) lead to the bottom-level group pGroup.
        _Check_return_ HRESULT IsGroupPath(_In_ CollectionViewGroupInternal* pGroup, const std::vector<ctl::ComPtr<IInspectable>>& names, _Out_ bool* pIsMatch) const
        {
            UINT level = LevelCount();
            *pIsMatch = true;
            for (auto pCurrent = pGroup; pCurrent && !pCurrent->IsRoot() && level > 0; pCurrent = pCurrent->GetParentNoRef())
            {
                bool isMatch = false;
                --level;
                IFC_RETURN(NamesMatch(level, pCurrent->GetKeyNoRef(), names[level].Get(), &isMatch));
                if (!isMatch)
                {
                    *pIsMatch = false;
                    break;
                }
            }
            return S_OK;
        }

        // A recompute adds the items in sorted order; a new group goes after the existing ones (WPF: loading).
        _Check_return_ HRESULT AddLoading(_In_opt_ IInspectable* pItem, const std::vector<ctl::ComPtr<IInspectable>>& names, UINT sourceIndex)
        {
            CollectionViewGroupInternal* pGroup = this;
            for (UINT level = 0; level < LevelCount(); ++level)
            {
                CollectionViewGroupInternal* pSubgroup = nullptr;
                IFC_RETURN(FindSubgroup(pGroup, level, names[level].Get(), &pSubgroup));
                if (!pSubgroup)
                {
                    ctl::ComPtr<CollectionViewGroupInternal> spSubgroup;
                    UINT position = static_cast<UINT>(pGroup->Subgroups().size());
                    IFC_RETURN(CreateGroup(names[level].Get(), pGroup, level, spSubgroup.ReleaseAndGetAddressOf()));
                    if (HasGroupOrder(level))
                    {
                        IFC_RETURN(FindOrderedSubgroupPosition(pGroup, level, spSubgroup.Get(), &position));
                    }
                    IFC_RETURN(pGroup->InsertSubgroup(position, spSubgroup.Get(), false /* isRaised */));
                    pSubgroup = spSubgroup.Get();
                }
                pGroup = pSubgroup;
            }

            const UINT position = pGroup->LeafCount();
            auto insertLeaf = [pItem](CollectionViewGroupInternal* pAncestor, UINT index) { return pAncestor->InsertLeaf(index, pItem, false /* isRaised */); };
            pGroup->SourceIndices().push_back(sourceIndex);
            IFC_RETURN(insertLeaf(pGroup, position));
            IFC_RETURN(ChangeAncestorLeaves(pGroup, position, insertLeaf));

            return S_OK;
        }

        _Check_return_ HRESULT CreateGroup(_In_opt_ IInspectable* pName, _In_ CollectionViewGroupInternal* pParent, UINT level, _Outptr_ CollectionViewGroupInternal** ppGroup) const
        {
            ctl::ComPtr<CollectionViewGroupInternal> spGroup;
            IFC_RETURN(ctl::ComObject<CollectionViewGroupInternal>::CreateInstance(spGroup.ReleaseAndGetAddressOf()));
            IFC_RETURN(spGroup->InitializeGroup(pName, pParent, level + 1 == LevelCount()));
            *ppGroup = spGroup.Detach();
            return S_OK;
        }

        // The bottom-level group and position that hold the item at sourceIndex.
        bool FindLeaf(UINT sourceIndex, _Outptr_result_maybenull_ CollectionViewGroupInternal** ppGroup, _Out_ UINT* pPosition)
        {
            return FindLeafIn(this, sourceIndex, ppGroup, pPosition);
        }

        // A source insert (delta 1) or remove (delta -1) at sourceIndex shifts the later source indices.
        void ShiftSourceIndices(UINT sourceIndex, int delta)
        {
            ShiftSourceIndicesIn(this, sourceIndex, delta);
        }

    protected:
        CollectionViewGroupRoot() = default;

    private:
        static bool FindLeafIn(_In_ CollectionViewGroupInternal* pGroup, UINT sourceIndex, _Outptr_result_maybenull_ CollectionViewGroupInternal** ppGroup, _Out_ UINT* pPosition)
        {
            *ppGroup = nullptr;
            *pPosition = 0;
            if (pGroup->IsBottomLevel())
            {
                auto& sourceIndices = pGroup->SourceIndices();
                auto found = std::find(sourceIndices.begin(), sourceIndices.end(), sourceIndex);
                if (found != sourceIndices.end())
                {
                    *ppGroup = pGroup;
                    *pPosition = static_cast<UINT>(found - sourceIndices.begin());
                    return true;
                }
                return false;
            }

            for (const auto& spSubgroup : pGroup->Subgroups())
            {
                if (FindLeafIn(spSubgroup.Get(), sourceIndex, ppGroup, pPosition))
                {
                    return true;
                }
            }
            return false;
        }

        static void ShiftSourceIndicesIn(_In_ CollectionViewGroupInternal* pGroup, UINT sourceIndex, int delta)
        {
            if (pGroup->IsBottomLevel())
            {
                for (auto& index : pGroup->SourceIndices())
                {
                    if (delta > 0 && index >= sourceIndex)
                    {
                        ++index;
                    }
                    else if (delta < 0 && index > sourceIndex)
                    {
                        --index;
                    }
                }
                return;
            }

            for (const auto& spSubgroup : pGroup->Subgroups())
            {
                ShiftSourceIndicesIn(spSubgroup.Get(), sourceIndex, delta);
            }
        }

        std::vector<GroupLevel> m_levels;
        wrl_wrappers::HString m_culture;
    };

    // Where an item joins the group tree (WPF: CollectionViewGroupRoot.AddToSubgroups).
    struct GroupedInsertion
    {
        bool isAccepted = false;
        // The deepest existing group on the item's path.
        CollectionViewGroupInternal* pGroup = nullptr;
        // The first group the item creates on its path, already holding the item; null if its group exists.
        ctl::ComPtr<CollectionViewGroupInternal> spNewGroup;
        // spNewGroup's index among pGroup's subgroups, else the item's index among pGroup's items.
        UINT position = 0;
    };

    // Where an item belongs in a sorted list of size items, without the item's own slot (excludedPosition, or
    // UINT_MAX). Items isSkipped marks are not compared (WPF: LiveShapingBlock.FindPosition ignores IsSortDirty
    // items, whose place is stale). The item is in order anywhere in [*pFirst, *pLast]; *pPosition is the upper
    // bound, the place a full recompute gives it when nothing is skipped.
    template <typename TItemAt, typename TSourceIndexAt, typename TIsSkipped>
    _Check_return_ HRESULT FindSortedRange(
        UINT size,
        UINT excludedPosition,
        const TItemAt& itemAt,
        const TSourceIndexAt& sourceIndexAt,
        const TIsSkipped& isSkipped,
        const SortFieldComparer& comparer,
        _In_opt_ IInspectable* pItem,
        const std::vector<ctl::ComPtr<IInspectable>>& keys,
        UINT sourceIndex,
        _Out_ UINT* pPosition,
        _Out_opt_ UINT* pFirst = nullptr,
        _Out_opt_ UINT* pLast = nullptr)
    {
        const UINT count = size - ((excludedPosition < size) ? 1 : 0);
        auto actual = [excludedPosition](UINT position) { return (position >= excludedPosition) ? position + 1 : position; };
        auto skipped = [&](UINT position) { return isSkipped(sourceIndexAt(actual(position))); };
        UINT low = 0;
        UINT high = count;

        while (low < high)
        {
            const UINT middle = low + (high - low) / 2;
            UINT probe = middle;
            while (probe < high && skipped(probe))
            {
                ++probe;
            }

            if (probe == high)
            {
                high = middle;
                continue;
            }

            ctl::ComPtr<IInspectable> spOther;
            int result = 0;
            IFC_RETURN(itemAt(actual(probe), spOther.ReleaseAndGetAddressOf()));
            IFC_RETURN(comparer.Compare(pItem, keys, sourceIndex, spOther.Get(), nullptr /* pRightKeys */, sourceIndexAt(actual(probe)), &result));
            if (result < 0)
            {
                high = middle;
            }
            else
            {
                low = probe + 1;
            }
        }

        *pPosition = low;
        if (pFirst)
        {
            UINT first = low;
            while (first > 0 && skipped(first - 1))
            {
                --first;
            }
            *pFirst = first;
        }
        if (pLast)
        {
            UINT last = low;
            while (last < count && skipped(last))
            {
                ++last;
            }
            *pLast = last;
        }
        return S_OK;
    }

    // The item's index in a bottom-level group: after the items that sort before or with it. excludedPosition is
    // a replaced item still in the group, or UINT_MAX. pIsSkipped marks (by source index) items not to compare.
    _Check_return_ HRESULT FindLeafPosition(
        _In_ CollectionViewGroupInternal* pGroup,
        const SortFieldComparer& comparer,
        _In_opt_ IInspectable* pItem,
        const std::vector<ctl::ComPtr<IInspectable>>& keys,
        UINT sourceIndex,
        UINT excludedPosition,
        _Out_ UINT* pPosition,
        _In_opt_ const std::vector<bool>* pIsSkipped = nullptr,
        _Out_opt_ UINT* pFirst = nullptr,
        _Out_opt_ UINT* pLast = nullptr)
    {
        const auto& sourceIndices = pGroup->SourceIndices();
        IFC_RETURN(FindSortedRange(
            pGroup->LeafCount(),
            excludedPosition,
            [pGroup](UINT position, IInspectable** ppItem) { return pGroup->GetAt(position, ppItem); },
            [&sourceIndices](UINT position) { return sourceIndices[position]; },
            [pIsSkipped](UINT index) { return pIsSkipped && index < pIsSkipped->size() && (*pIsSkipped)[index]; },
            comparer, pItem, keys, sourceIndex, pPosition, pFirst, pLast));
        return S_OK;
    }

    // A new subgroup's index (WPF: CollectionViewGroupInternal.FindIndex): before the first subgroup whose first
    // item sorts after the new group's item.
    _Check_return_ HRESULT FindSubgroupPosition(
        _In_ CollectionViewGroupRoot* pRoot,
        _In_ CollectionViewGroupInternal* pParent,
        UINT level,
        _In_ CollectionViewGroupInternal* pSubgroup,
        const SortFieldComparer& comparer,
        _In_opt_ IInspectable* pItem,
        const std::vector<ctl::ComPtr<IInspectable>>& keys,
        UINT sourceIndex,
        _Out_ UINT* pPosition)
    {
        const auto& subgroups = pParent->Subgroups();
        UINT position = 0;

        if (pRoot->HasGroupOrder(level))
        {
            IFC_RETURN(pRoot->FindOrderedSubgroupPosition(pParent, level, pSubgroup, pPosition));
            return S_OK;
        }

        for (; position < subgroups.size(); ++position)
        {
            ctl::ComPtr<IInspectable> spSeed;
            int result = 0;
            if (subgroups[position]->LeafCount() == 0)
            {
                continue;
            }
            IFC_RETURN(subgroups[position]->GetAt(0, spSeed.ReleaseAndGetAddressOf()));
            IFC_RETURN(comparer.Compare(pItem, keys, sourceIndex, spSeed.Get(), nullptr /* pRightKeys */, subgroups[position]->FirstSourceIndex(), &result));
            if (result < 0)
            {
                break;
            }
        }

        *pPosition = position;
        return S_OK;
    }

    // Runs app code (property getters, converters, comparers) and changes nothing that is presented.
    _Check_return_ HRESULT PlanGroupedInsertion(
        _In_ CollectionViewGroupRoot* pRoot,
        const SortFieldComparer& comparer,
        _In_opt_ IInspectable* pItem,
        const std::vector<ctl::ComPtr<IInspectable>>& keys,
        UINT sourceIndex,
        _In_opt_ CollectionViewGroupInternal* pExcludedGroup,
        UINT excludedPosition,
        _Out_ GroupedInsertion* pInsertion,
        _In_opt_ const std::vector<bool>* pIsSkipped = nullptr)
    {
        std::vector<ctl::ComPtr<IInspectable>> names;
        CollectionViewGroupInternal* pGroup = pRoot;

        IFC_RETURN(pRoot->GetGroupNames(pItem, &names));
        pInsertion->isAccepted = true;

        for (UINT level = 0; level < pRoot->LevelCount(); ++level)
        {
            CollectionViewGroupInternal* pSubgroup = nullptr;
            IFC_RETURN(pRoot->FindSubgroup(pGroup, level, names[level].Get(), &pSubgroup));
            if (pSubgroup)
            {
                pGroup = pSubgroup;
                continue;
            }

            // The item starts a branch: build it, holding the item, before it is presented.
            CollectionViewGroupInternal* pParent = pGroup;
            for (UINT newLevel = level; newLevel < pRoot->LevelCount(); ++newLevel)
            {
                ctl::ComPtr<CollectionViewGroupInternal> spNewGroup;
                IFC_RETURN(pRoot->CreateGroup(names[newLevel].Get(), pParent, newLevel, spNewGroup.ReleaseAndGetAddressOf()));
                IFC_RETURN(spNewGroup->InsertLeaf(0, pItem, false /* isRaised */));
                if (newLevel == level)
                {
                    pInsertion->spNewGroup = spNewGroup;
                }
                else
                {
                    IFC_RETURN(pParent->InsertSubgroup(0, spNewGroup.Get(), false /* isRaised */));
                }
                if (newLevel + 1 == pRoot->LevelCount())
                {
                    spNewGroup->SourceIndices().push_back(sourceIndex);
                }
                pParent = spNewGroup.Get();
            }

            pInsertion->pGroup = pGroup;
            IFC_RETURN(FindSubgroupPosition(pRoot, pGroup, level, pInsertion->spNewGroup.Get(), comparer, pItem, keys, sourceIndex, &pInsertion->position));
            return S_OK;
        }

        pInsertion->pGroup = pGroup;
        IFC_RETURN(FindLeafPosition(pGroup, comparer, pItem, keys, sourceIndex, (pGroup == pExcludedGroup) ? excludedPosition : UINT_MAX, &pInsertion->position, pIsSkipped));
        return S_OK;
    }

    _Check_return_ HRESULT ApplyGroupedInsertion(const GroupedInsertion& insertion, _In_opt_ IInspectable* pItem, UINT sourceIndex)
    {
        auto insertLeaf = [pItem](CollectionViewGroupInternal* pGroup, UINT index) { return pGroup->InsertLeaf(index, pItem, true /* isRaised */); };

        if (insertion.spNewGroup)
        {
            IFC_RETURN(insertion.pGroup->InsertSubgroup(insertion.position, insertion.spNewGroup.Get(), true /* isRaised */));
            IFC_RETURN(ChangeAncestorLeaves(insertion.spNewGroup.Get(), 0, insertLeaf));
            return S_OK;
        }

        auto& sourceIndices = insertion.pGroup->SourceIndices();
        sourceIndices.insert(sourceIndices.begin() + insertion.position, sourceIndex);
        IFC_RETURN(insertLeaf(insertion.pGroup, insertion.position));
        IFC_RETURN(ChangeAncestorLeaves(insertion.pGroup, insertion.position, insertLeaf));
        return S_OK;
    }

    // An item leaves its group (WPF: RemoveFromSubgroups); the groups it was the last item of go with it.
    _Check_return_ HRESULT ApplyGroupedRemoval(_In_ CollectionViewGroupInternal* pGroup, UINT position)
    {
        auto removeLeaf = [](CollectionViewGroupInternal* pAncestor, UINT index) { return pAncestor->RemoveLeaf(index); };
        CollectionViewGroupInternal* pEmptied = nullptr;

        if (pGroup->LeafCount() == 1)
        {
            pEmptied = pGroup;
            while (pEmptied->GetParentNoRef() && !pEmptied->GetParentNoRef()->IsRoot() && pEmptied->GetParentNoRef()->LeafCount() == 1)
            {
                pEmptied = pEmptied->GetParentNoRef();
            }
        }

        if (pEmptied)
        {
            auto pParent = pEmptied->GetParentNoRef();
            const UINT offset = pParent->IsRoot() ? 0 : pParent->LeafCountBefore(pEmptied);
            const UINT index = pParent->IndexOfSubgroup(pEmptied);
            IFC_RETURN(pParent->RemoveSubgroup(index));
            if (!pParent->IsRoot())
            {
                IFC_RETURN(pParent->RemoveLeaf(offset));
                IFC_RETURN(ChangeAncestorLeaves(pParent, offset, removeLeaf));
            }
            return S_OK;
        }

        auto& sourceIndices = pGroup->SourceIndices();
        sourceIndices.erase(sourceIndices.begin() + position);
        IFC_RETURN(pGroup->RemoveLeaf(position));
        IFC_RETURN(ChangeAncestorLeaves(pGroup, position, removeLeaf));
        return S_OK;
    }

    _Check_return_ HRESULT ApplyGroupedReplace(_In_ CollectionViewGroupInternal* pGroup, UINT position, _In_opt_ IInspectable* pItem)
    {
        auto setLeaf = [pItem](CollectionViewGroupInternal* pAncestor, UINT index) { return pAncestor->SetLeaf(index, pItem); };
        IFC_RETURN(setLeaf(pGroup, position));
        IFC_RETURN(ChangeAncestorLeaves(pGroup, position, setLeaf));
        return S_OK;
    }
}

HRESULT VectorCollectionView::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(xaml_data::ICollectionViewShaping)) && m_isShapingEnabled)
    {
        *ppObject = static_cast<xaml_data::ICollectionViewShaping*>(this);
        AddRefOuter();
        return S_OK;
    }

    if (InlineIsEqualGUID(iid, __uuidof(xaml_data::ICollectionViewLiveShaping)) && m_isShapingEnabled)
    {
        *ppObject = static_cast<xaml_data::ICollectionViewLiveShaping*>(this);
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
    SetFilterHost(pHost->HasFilterRequestedHandlers() ? pHost : nullptr);
    return S_OK;
}

void VectorCollectionView::SetFilterHost(_In_opt_ CollectionViewSource* pHost)
{
    if (pHost)
    {
        SetPtrValue(m_tpFilterHost, static_cast<xaml_data::ICollectionViewSource*>(pHost));
    }
    else
    {
        m_tpFilterHost.Clear();
    }
}

_Check_return_ HRESULT VectorCollectionView::ApplyShapingRules(
    _In_ wfc::IVector<xaml_data::SortDescription*>* pSortDescriptions,
    _In_ wfc::IVector<xaml_data::PropertyGroupDescription*>* pGroupDescriptions,
    _In_opt_ xaml_data::ICollectionViewFilter* pFilter,
    _In_opt_ xaml_data::ICollectionViewComparer* pCustomSort,
    _In_opt_ HSTRING culture)
{
    SetPtrValue(m_tpSortDescriptions, pSortDescriptions);
    SetPtrValue(m_tpGroupDescriptions, pGroupDescriptions);
    SetPtrValue(m_tpFilter, pFilter);
    SetPtrValue(m_tpCustomSort, pCustomSort);
    IFC_RETURN(m_culture.Set(culture));
    IFC_RETURN(OnShapingRulesChanged());
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::EnableDefaultViewShaping()
{
    ctl::ComPtr<wfc::IVector<xaml_data::SortDescription*>> spSortDescriptions;
    ctl::ComPtr<wfc::IVector<xaml_data::PropertyGroupDescription*>> spGroupDescriptions;

    m_isShapingEnabled = true;
    m_isDefaultView = true;
    SetPtrValue(m_tpSourceList, m_tpSource.Get());
    IFC_RETURN(CreateShapingDescriptions(spSortDescriptions.ReleaseAndGetAddressOf(), spGroupDescriptions.ReleaseAndGetAddressOf()));
    IFC_RETURN(ApplyShapingRules(spSortDescriptions.Get(), spGroupDescriptions.Get(), nullptr /* pFilter */, nullptr /* pCustomSort */, nullptr /* culture */));
    return S_OK;
}

void VectorCollectionView::SetSourceCollection(_In_opt_ IInspectable* pSource)
{
    SetPtrValue(m_tpSourceCollection, pSource);
}

_Check_return_ HRESULT VectorCollectionView::ValidateCulture(_In_opt_ HSTRING value)
{
    UINT32 length = 0;
    LPCWSTR szValue = WindowsGetStringRawBuffer(value, &length);
    IFCEXPECTRC_RETURN(length == 0 || (length < LOCALE_NAME_MAX_LENGTH && IsValidLocaleName(szValue)), E_INVALIDARG);
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::OnGroupDescriptionChanged(_In_ PropertyGroupDescription* pDescription)
{
    UINT index = 0;
    BOOLEAN isFound = FALSE;

    if (!m_tpGroupDescriptions || !m_isGrouping)
    {
        return S_OK;
    }

    IFC_RETURN(m_tpGroupDescriptions->IndexOf(static_cast<xaml_data::IPropertyGroupDescription*>(ctl::interface_cast<xaml_data::IPropertyGroupDescription>(pDescription)), &index, &isFound));
    if (isFound)
    {
        IFC_RETURN(OnShapingRulesChanged());
    }

    return S_OK;
}

// The descriptions are copied into collections of the view's own; the host keeps the ones it shared. The
// copy changes no rule, so it recomputes nothing.
_Check_return_ HRESULT VectorCollectionView::DetachFromHost()
{
    ctl::ComPtr<wfc::IVector<xaml_data::SortDescription*>> spSortDescriptions;
    ctl::ComPtr<wfc::IVector<xaml_data::PropertyGroupDescription*>> spGroupDescriptions;
    const bool wasRefreshPending = m_isRefreshPending;
    HRESULT hr = S_OK;

    m_hostRef = ctl::WeakRefPtr();
    m_tpFilterHost.Clear();
    if (!m_tpSortDescriptions || !m_tpGroupDescriptions)
    {
        return S_OK;
    }

    IFC_RETURN(CreateShapingDescriptions(spSortDescriptions.ReleaseAndGetAddressOf(), spGroupDescriptions.ReleaseAndGetAddressOf()));

    ++m_deferRefreshLevel;
    hr = [&]() -> HRESULT
    {
        UINT count = 0;
        IFC_RETURN(m_tpSortDescriptions->get_Size(&count));
        for (UINT i = 0; i < count; ++i)
        {
            ctl::ComPtr<xaml_data::ISortDescription> spDescription;
            IFC_RETURN(m_tpSortDescriptions->GetAt(i, spDescription.ReleaseAndGetAddressOf()));
            IFC_RETURN(spSortDescriptions->Append(spDescription.Get()));
        }

        IFC_RETURN(m_tpGroupDescriptions->get_Size(&count));
        for (UINT i = 0; i < count; ++i)
        {
            ctl::ComPtr<xaml_data::IPropertyGroupDescription> spDescription;
            IFC_RETURN(m_tpGroupDescriptions->GetAt(i, spDescription.ReleaseAndGetAddressOf()));
            IFC_RETURN(spGroupDescriptions->Append(spDescription.Get()));
        }
        return S_OK;
    }();
    --m_deferRefreshLevel;
    m_isRefreshPending = wasRefreshPending;
    IFC_RETURN(hr);

    SetPtrValue(m_tpSortDescriptions, spSortDescriptions.Get());
    SetPtrValue(m_tpGroupDescriptions, spGroupDescriptions.Get());
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_SortDescriptions(_Outptr_ wfc::IObservableVector<xaml_data::SortDescription*>** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    IFCEXPECT_RETURN(m_tpSortDescriptions);
    IFC_RETURN(m_tpSortDescriptions.CopyTo(ppValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_GroupDescriptions(_Outptr_ wfc::IObservableVector<xaml_data::PropertyGroupDescription*>** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    IFCEXPECT_RETURN(m_tpGroupDescriptions);
    IFC_RETURN(m_tpGroupDescriptions.CopyTo(ppValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_Filter(_Outptr_result_maybenull_ xaml_data::ICollectionViewFilter** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    IFC_RETURN(m_tpFilter.CopyTo(ppValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::put_Filter(_In_opt_ xaml_data::ICollectionViewFilter* pValue)
{
    IFC_RETURN(CheckThread());
    SetPtrValue(m_tpFilter, pValue);
    IFC_RETURN(OnShapingRulesChanged());
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_CustomSort(_Outptr_result_maybenull_ xaml_data::ICollectionViewComparer** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    IFC_RETURN(m_tpCustomSort.CopyTo(ppValue));
    return S_OK;
}

// As in WPF, a CustomSort replaces the SortDescriptions, and adding a SortDescription drops the CustomSort.
IFACEMETHODIMP VectorCollectionView::put_CustomSort(_In_opt_ xaml_data::ICollectionViewComparer* pValue)
{
    UINT sortCount = 0;

    IFC_RETURN(CheckThread());
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

IFACEMETHODIMP VectorCollectionView::get_SourceCollection(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    IFC_RETURN(m_tpSourceCollection.CopyTo(ppValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_Culture(_Out_ HSTRING* pValue)
{
    ARG_VALIDRETURNPOINTER(pValue);
    IFC_RETURN(CheckThread());
    IFC_RETURN(m_culture.CopyTo(pValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::put_Culture(_In_opt_ HSTRING value)
{
    IFC_RETURN(CheckThread());
    IFC_RETURN(ValidateCulture(value));
    IFC_RETURN(m_culture.Set(value));
    IFC_RETURN(OnShapingRulesChanged());
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::Refresh()
{
    IFC_RETURN(CheckThread());
    IFC_RETURN(RefreshOrDefer());
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::DeferRefresh(_Outptr_ wf::IClosable** ppReturnValue)
{
    Microsoft::WRL::ComPtr<CollectionViewRefreshDeferral<VectorCollectionView>> spDeferral;

    ARG_VALIDRETURNPOINTER(ppReturnValue);
    *ppReturnValue = nullptr;
    IFC_RETURN(CheckThread());

    IFC_RETURN(wrl::MakeAndInitialize<CollectionViewRefreshDeferral<VectorCollectionView>>(&spDeferral, this));
    ++m_deferRefreshLevel;
    *ppReturnValue = spDeferral.Detach();
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::EndDeferRefresh()
{
    ASSERT(m_deferRefreshLevel > 0);
    if (m_deferRefreshLevel == 0 || --m_deferRefreshLevel > 0)
    {
        return S_OK;
    }

    if (!m_isRefreshPending)
    {
        // Live changes made while deferred are restored now, incrementally.
        IFC_RETURN(RestoreLiveShaping());
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

    *pHasRules = sortCount > 0 || groupCount > 0 || m_tpFilter || m_tpCustomSort || m_tpFilterHost;
    return S_OK;
}

// A rule changed (WPF: RefreshOrDefer after a SortDescriptions, GroupDescriptions or Filter change).
_Check_return_ HRESULT VectorCollectionView::OnShapingRulesChanged()
{
    UINT groupCount = 0;
    bool hasRules = false;

    if (!m_isShapingEnabled)
    {
        return S_OK;
    }

    if (m_shapeBusyDepth > 0)
    {
        m_isRecomputePending = true;
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

    IFC_RETURN(StartShaping(isGrouping));
    IFC_RETURN(RunPendingRecompute());
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::RunPendingRecompute()
{
    // A handler that changes the view on every change of it would never finish (in WPF it overflows the stack).
    constexpr UINT c_maxPendingRecomputes = 64;

    if (!m_isRecomputePending || m_shapeBusyDepth > 0 || m_isRunningPendingRecompute)
    {
        return S_OK;
    }

    m_isRunningPendingRecompute = true;
    auto done = wil::scope_exit([this]() noexcept { m_isRunningPendingRecompute = false; });
    for (UINT pass = 0; m_isRecomputePending; ++pass)
    {
        m_isRecomputePending = false;
        IFCEXPECTRC_RETURN(pass < c_maxPendingRecomputes, E_ILLEGAL_METHOD_CALL);
        IFC_RETURN(OnShapingRulesChanged());
    }
    return S_OK;
}

// Computes the output into the view's flat or group list and presents it, announced as one Reset; currency
// follows the item. As in WPF, starting or stopping grouping keeps the view and its CollectionGroups object:
// CollectionGroups turns non-null or null, PropertyChanged("CollectionGroups") is raised, then the Reset.
_Check_return_ HRESULT VectorCollectionView::StartShaping(bool isGrouping)
{
    ctl::ComPtr<wfc::IVector<IInspectable*>> spPreviousOutput = m_tpInternalList.Get();
    const bool wasGrouping = m_isGrouping;
    HRESULT hr = S_OK;

    if (isGrouping && !m_tpGroupList)
    {
        ctl::ComPtr<InternalList> spGroupList;
        ctl::ComPtr<xaml_data::ICollectionView> spGrouped;
        IFC_RETURN(ctl::ComObject<InternalList>::CreateInstance(spGroupList.ReleaseAndGetAddressOf()));
        IFC_RETURN(GroupedDataCollectionView::CreateInstance(spGroupList.Get(), nullptr /* pItemsPath */, spGrouped.ReleaseAndGetAddressOf()));
        static_cast<GroupedDataCollectionView*>(spGrouped.Get())->SetRaisesSingleGroupsReset();
        SetPtrValue(m_tpGroupList, static_cast<wfc::IVector<IInspectable*>*>(spGroupList.Get()));
        SetPtrValue(m_tpGroupedPresentation, spGrouped.Get());
    }
    else if (!isGrouping && !m_tpFlatList)
    {
        ctl::ComPtr<InternalList> spFlatList;
        IFC_RETURN(ctl::ComObject<InternalList>::CreateInstance(spFlatList.ReleaseAndGetAddressOf()));
        SetPtrValue(m_tpFlatList, static_cast<wfc::IVector<IInspectable*>*>(spFlatList.Get()));
    }

    m_isGrouping = isGrouping;
    SetPtrValue(m_tpInternalList, isGrouping ? m_tpGroupList.Get() : m_tpFlatList.Get());

    // Followed before the first compute, so a change the app makes while it runs is not missed.
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

    m_isRecomputePending = false;
    ++m_shapeBusyDepth;
    hr = RefreshOverride();
    if (FAILED(hr))
    {
        --m_shapeBusyDepth;
        m_isGrouping = wasGrouping;
        SetPtrValue(m_tpInternalList, spPreviousOutput.Get());
        m_areSourceIndicesValid = false;
        IFC_RETURN(hr);
    }

    // The output is read-only, so index-based writes through the view fail instead of reordering the source.
    hr = [&]() -> HRESULT
    {
        if (isGrouping)
        {
            ctl::ComPtr<wfc::IVector<IInspectable*>> spGroupedItems;
            IFC_RETURN(m_tpGroupedPresentation.As(&spGroupedItems));
            IFC_RETURN(RetargetSource(spGroupedItems.Get(), !wasGrouping));
        }
        else
        {
            IFC_RETURN(RetargetSource(m_tpFlatList.Get(), wasGrouping));
        }

        // The output no longer presented lets go of its items.
        if (spPreviousOutput && spPreviousOutput.Get() != m_tpInternalList.Get())
        {
            IFC_RETURN(static_cast<InternalList*>(spPreviousOutput.Get())->ReplaceAll({}));
        }
        return S_OK;
    }();
    --m_shapeBusyDepth;
    IFC_RETURN(hr);

    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::get_CollectionGroupsImpl(_Out_ wfc::IObservableVector<IInspectable *> **value)
{
    if (m_isGrouping && m_tpGroupedPresentation)
    {
        IFC_RETURN(m_tpGroupedPresentation->get_CollectionGroups(value));
        return S_OK;
    }

    IFC_RETURN(CollectionView::get_CollectionGroupsImpl(value));
    return S_OK;
}

// Full recompute (WPF: ListCollectionView.RefreshOverride): filter, then stable multi-key sort, then group by
// first appearance. isSkippedWhenUnchanged: a flat output equal to the current one raises nothing.
_Check_return_ HRESULT VectorCollectionView::RefreshOverride(bool isSkippedWhenUnchanged)
{
    struct Entry
    {
        ctl::ComPtr<IInspectable> item;
        std::vector<ctl::ComPtr<IInspectable>> sortKeys;
        std::vector<ctl::ComPtr<IInspectable>> groupNames;
        UINT sourceIndex = 0;
    };

    SortFieldComparer sortComparer;
    ctl::ComPtr<CollectionViewGroupRoot> spRoot;
    ctl::ComPtr<CollectionViewSource> spFilterHost;
    std::vector<Entry> entries;
    std::vector<ctl::ComPtr<IInspectable>> outputItems;
    std::vector<UINT> outputSourceIndices;
    const UINT sourceChangeCount = m_sourceListChangeCount;
    UINT size = 0;

    IFC_RETURN(sortComparer.Initialize(m_tpSortDescriptions.Get(), m_tpCustomSort.Get(), m_culture.Get()));

    if (m_isGrouping)
    {
        IFC_RETURN(ctl::ComObject<CollectionViewGroupRoot>::CreateInstance(spRoot.ReleaseAndGetAddressOf()));
        IFC_RETURN(spRoot->InitializeRoot(m_tpGroupDescriptions.Get(), this, m_culture.Get()));
    }

    IFC_RETURN(GetFilterHost(spFilterHost.ReleaseAndGetAddressOf()));
    IFC_RETURN(m_tpSourceList->get_Size(&size));
    entries.reserve(size);

    for (UINT i = 0; i < size; ++i)
    {
        Entry entry;
        bool isAccepted = true;
        entry.sourceIndex = i;
        IFC_RETURN(m_tpSourceList->GetAt(i, entry.item.ReleaseAndGetAddressOf()));

        IFC_RETURN(PassesFilter(entry.item.Get(), spFilterHost.Get(), &isAccepted));
        if (!isAccepted)
        {
            continue;
        }

        IFC_RETURN(sortComparer.EvaluateKeys(entry.item.Get(), &entry.sortKeys));

        if (spRoot)
        {
            IFC_RETURN(spRoot->GetGroupNames(entry.item.Get(), &entry.groupNames));
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

    if (!spRoot)
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
        for (const auto& entry : entries)
        {
            IFC_RETURN(spRoot->AddLoading(entry.item.Get(), entry.groupNames, entry.sourceIndex));
        }

        outputItems.reserve(spRoot->Subgroups().size());
        for (const auto& spGroup : spRoot->Subgroups())
        {
            outputItems.push_back(ctl::as_iinspectable(spGroup.Get()));
        }
        spRoot->PresentSubgroups(static_cast<InternalList*>(m_tpInternalList.Get()));
    }

    IFC_RETURN(RebuildLiveShapingItems());

    bool isUnchanged = isSkippedWhenUnchanged && !spRoot && !m_spGroupRoot && m_areSourceIndicesValid && outputSourceIndices == m_sourceIndices;
    for (UINT i = 0; isUnchanged && i < outputItems.size(); ++i)
    {
        ctl::ComPtr<IInspectable> spPresented;
        IFC_RETURN(m_tpInternalList->GetAt(i, spPresented.ReleaseAndGetAddressOf()));
        isUnchanged = spPresented.Get() == outputItems[i].Get();
    }

    // Recorded before ReplaceAll raises, so a handler that changes the source sees consistent state.
    m_sourceIndices = std::move(outputSourceIndices);
    m_spGroupRoot = spRoot ? static_cast<xaml_data::ICollectionViewGroup*>(spRoot.Get()) : nullptr;
    m_sourceListSize = size;
    m_areSourceIndicesValid = (sourceChangeCount == m_sourceListChangeCount);
    if (!isUnchanged)
    {
        IFC_RETURN(static_cast<InternalList*>(m_tpInternalList.Get())->ReplaceAll(outputItems));
    }
    return S_OK;
}

// The CollectionViewSource whose FilterRequested handlers filter the view, if any. As in WPF, its event replaces
// the Filter delegate while it has handlers.
_Check_return_ HRESULT VectorCollectionView::GetFilterHost(_Outptr_result_maybenull_ CollectionViewSource** ppHost)
{
    ctl::ComPtr<xaml_data::ICollectionViewSource> spHost = m_tpFilterHost.Get();

    *ppHost = nullptr;
    if (spHost && static_cast<CollectionViewSource*>(spHost.Get())->HasFilterRequestedHandlers())
    {
        *ppHost = static_cast<CollectionViewSource*>(spHost.Detach());
    }

    return S_OK;
}

// Whether the view shows the item (WPF: CollectionView.PassesFilter).
_Check_return_ HRESULT VectorCollectionView::PassesFilter(_In_opt_ IInspectable* pItem, _In_opt_ CollectionViewSource* pFilterHost, _Out_ bool* pIsAccepted)
{
    *pIsAccepted = true;
    if (pFilterHost)
    {
        IFC_RETURN(pFilterHost->RaiseFilterRequested(pItem, pIsAccepted));
    }
    else if (m_tpFilter)
    {
        boolean accepted = true;
        IFC_RETURN(m_tpFilter->Invoke(pItem, &accepted));
        *pIsAccepted = !!accepted;
    }
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::RefreshOrDefer(bool isSkippedWhenUnchanged)
{
    ctl::ComPtr<IInspectable> spCurrentItem;
    BOOLEAN wasBeforeFirst = FALSE;
    BOOLEAN wasAfterLast = FALSE;
    BOOLEAN isCurrentValid = FALSE;
    HRESULT hr = S_OK;

    if (!m_tpInternalList)
    {
        return S_OK;
    }

    // Asked for while app code runs or the view raises its changes: done once that unwinds.
    if (m_shapeBusyDepth > 0)
    {
        m_isRecomputePending = true;
        return S_OK;
    }

    if (m_deferRefreshLevel > 0)
    {
        m_isRefreshPending = true;
        return S_OK;
    }

    IFC_RETURN(get_CurrentItemImpl(spCurrentItem.ReleaseAndGetAddressOf()));
    IFC_RETURN(get_IsCurrentBeforeFirstImpl(&wasBeforeFirst));
    IFC_RETURN(get_IsCurrentAfterLastImpl(&wasAfterLast));

    m_isRecomputePending = false;
    ++m_shapeBusyDepth;
    hr = RefreshOverride(isSkippedWhenUnchanged);
    --m_shapeBusyDepth;
    IFC_RETURN(hr);

    // WPF (ListCollectionView.RefreshOverride): currency stays before the first or after the last item, else
    // follows the item, else goes to the first.
    if (wasAfterLast || wasBeforeFirst)
    {
        UINT size = 0;
        BOOLEAN isBeforeFirst = FALSE;
        BOOLEAN isAfterLast = FALSE;
        IFC_RETURN(get_Size(&size));
        IFC_RETURN(get_IsCurrentBeforeFirstImpl(&isBeforeFirst));
        IFC_RETURN(get_IsCurrentAfterLastImpl(&isAfterLast));
        if (wasAfterLast ? !isAfterLast : !isBeforeFirst)
        {
            IFC_RETURN(MoveCurrentToPositionImpl(wasAfterLast ? static_cast<INT>(size) : -1, &isCurrentValid));
        }
    }
    else
    {
        ctl::ComPtr<IInspectable> spNowCurrent;
        IFC_RETURN(get_CurrentItemImpl(spNowCurrent.ReleaseAndGetAddressOf()));
        if (!spCurrentItem || spNowCurrent.Get() != spCurrentItem.Get())
        {
            IFC_RETURN(MoveCurrentToImpl(spCurrentItem.Get(), &isCurrentValid));
            if (!isCurrentValid)
            {
                IFC_RETURN(MoveCurrentToFirstImpl(&isCurrentValid));
            }
        }
    }

    IFC_RETURN(RunPendingRecompute());
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
    ClearLiveShapingItems();
}

// A source change (WPF: ListCollectionView.ProcessCollectionChanged).
_Check_return_ HRESULT VectorCollectionView::ProcessCollectionChanged(_In_ wfc::IVectorChangedEventArgs* pArgs)
{
    wfc::CollectionChange change = wfc::CollectionChange_Reset;
    UINT index = 0;
    bool isApplied = false;

    ++m_sourceListChangeCount;

    if (!m_tpInternalList)
    {
        m_areSourceIndicesValid = false;
        return S_OK;
    }

    // A change made while app code runs (a Filter, a getter) or while the view raises its changes (a handler)
    // is applied once that unwinds, by a recompute.
    if (m_shapeBusyDepth > 0)
    {
        m_areSourceIndicesValid = false;
        m_isRecomputePending = true;
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

    if (m_areSourceIndicesValid && change != wfc::CollectionChange_Reset && !HasSortDirtyLiveItems())
    {
        IFC_RETURN(ProcessCollectionChangedWithAdjustedIndex(change, index, &isApplied));
    }

    if (isApplied)
    {
        IFC_RETURN(UpdateLiveShapingItems(change, index));
    }
    else
    {
        IFC_RETURN(RefreshOrDefer());
    }

    IFC_RETURN(RunPendingRecompute());
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

    if (m_isGrouping)
    {
        IFC_RETURN(ProcessGroupedCollectionChanged(change, index, pIsApplied));
        return S_OK;
    }

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

        ++m_shapeBusyDepth;
        hr = [&]() -> HRESULT
        {
            SortFieldComparer comparer;
            std::vector<ctl::ComPtr<IInspectable>> keys;
            bool isAccepted = true;

            ctl::ComPtr<CollectionViewSource> spFilterHost;
            IFC_RETURN(GetFilterHost(spFilterHost.ReleaseAndGetAddressOf()));
            IFC_RETURN(PassesFilter(spItem.Get(), spFilterHost.Get(), &isAccepted));
            if (!isAccepted)
            {
                return S_OK;
            }

            IFC_RETURN(comparer.Initialize(m_tpSortDescriptions.Get(), m_tpCustomSort.Get(), m_culture.Get()));
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
        --m_shapeBusyDepth;
        IFC_RETURN(hr);

        if (sourceChangeCount != m_sourceListChangeCount)
        {
            return S_OK;
        }
    }

    // Bookkeeping first, then the notifications, each matching the output at the time it is raised.
    ++m_shapeBusyDepth;
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
    --m_shapeBusyDepth;
    IFC_RETURN(hr);

    // A handler changed the source while the change was raised: recompute fully.
    *pIsApplied = (sourceChangeCount == m_sourceListChangeCount);
    return S_OK;
}

// A source change on a grouped view (WPF: ListCollectionView.AddItemToGroups / RemoveItemFromGroups): the item
// joins or leaves its bottom-level group at its sorted position, a group it starts is inserted among its siblings,
// and a group it empties is removed. Each vector of the group tree raises its own change.
_Check_return_ HRESULT VectorCollectionView::ProcessGroupedCollectionChanged(wfc::CollectionChange change, UINT index, _Out_ bool* pIsApplied)
{
    auto pRoot = static_cast<CollectionViewGroupRoot*>(static_cast<CollectionViewGroupInternal*>(m_spGroupRoot.Get()));
    const UINT sourceChangeCount = m_sourceListChangeCount;
    CollectionViewGroupInternal* pOldGroup = nullptr;
    UINT oldPosition = 0;
    GroupedInsertion insertion;
    ctl::ComPtr<IInspectable> spItem;
    SortFieldComparer comparer;
    std::vector<ctl::ComPtr<IInspectable>> keys;
    HRESULT hr = S_OK;

    *pIsApplied = false;
    if (!pRoot)
    {
        return S_OK;
    }

    const bool hasOld = (change != wfc::CollectionChange_ItemInserted) && pRoot->FindLeaf(index, &pOldGroup, &oldPosition);
    if (change == wfc::CollectionChange_ItemInserted)
    {
        pRoot->ShiftSourceIndices(index, 1);
    }

    // App code (Filter, property getters, converters, CustomSort) runs only here and in a re-plan below.
    if (change != wfc::CollectionChange_ItemRemoved)
    {
        IFC_RETURN(m_tpSourceList->GetAt(index, spItem.ReleaseAndGetAddressOf()));

        ++m_shapeBusyDepth;
        hr = [&]() -> HRESULT
        {
            bool isAccepted = true;
            ctl::ComPtr<CollectionViewSource> spFilterHost;
            IFC_RETURN(GetFilterHost(spFilterHost.ReleaseAndGetAddressOf()));
            IFC_RETURN(PassesFilter(spItem.Get(), spFilterHost.Get(), &isAccepted));
            if (!isAccepted)
            {
                return S_OK;
            }

            IFC_RETURN(comparer.Initialize(m_tpSortDescriptions.Get(), m_tpCustomSort.Get(), m_culture.Get()));
            IFC_RETURN(comparer.EvaluateKeys(spItem.Get(), &keys));
            IFC_RETURN(PlanGroupedInsertion(pRoot, comparer, spItem.Get(), keys, index, hasOld ? pOldGroup : nullptr, oldPosition, &insertion));
            return S_OK;
        }();
        --m_shapeBusyDepth;
        IFC_RETURN(hr);

        if (sourceChangeCount != m_sourceListChangeCount)
        {
            return S_OK;
        }
    }

    ++m_shapeBusyDepth;
    hr = [&]() -> HRESULT
    {
        if (hasOld && insertion.isAccepted && !insertion.spNewGroup && insertion.pGroup == pOldGroup && insertion.position == oldPosition)
        {
            IFC_RETURN(ApplyGroupedReplace(pOldGroup, oldPosition, spItem.Get()));
            return S_OK;
        }

        if (hasOld)
        {
            IFC_RETURN(ApplyGroupedRemoval(pOldGroup, oldPosition));
        }

        if (change == wfc::CollectionChange_ItemRemoved)
        {
            pRoot->ShiftSourceIndices(index, -1);
        }

        if (insertion.isAccepted)
        {
            // The removal may have changed the groups the plan used.
            if (hasOld)
            {
                insertion = GroupedInsertion();
                IFC_RETURN(PlanGroupedInsertion(pRoot, comparer, spItem.Get(), keys, index, nullptr /* pExcludedGroup */, UINT_MAX, &insertion));
            }
            IFC_RETURN(ApplyGroupedInsertion(insertion, spItem.Get(), index));
        }

        return S_OK;
    }();
    --m_shapeBusyDepth;
    IFC_RETURN(hr);

    // A handler changed the source while the change was raised: recompute fully.
    *pIsApplied = (sourceChangeCount == m_sourceListChangeCount);
    return S_OK;
}

// ---- Live shaping (WPF: ICollectionViewLiveShaping on ListCollectionView, MS.Internal.Data.LiveShapingList) ----

namespace
{
    enum LiveShapingFlag : UINT
    {
        LiveShapingFlag_Sorting = 0x1,
        LiveShapingFlag_Filtering = 0x2,
        LiveShapingFlag_Grouping = 0x4,
    };

    struct LivePath
    {
        wrl_wrappers::HString path;
        UINT flags = 0;
    };

    _Check_return_ HRESULT AddLivePath(std::vector<LivePath>& paths, _In_opt_ HSTRING path, UINT flag)
    {
        if (WindowsGetStringLen(path) == 0)
        {
            return S_OK;
        }

        for (auto& existing : paths)
        {
            INT32 result = 1;
            IFC_RETURN(WindowsCompareStringOrdinal(existing.path.Get(), path, &result));
            if (result == 0)
            {
                existing.flags |= flag;
                return S_OK;
            }
        }

        LivePath added;
        IFC_RETURN(added.path.Set(path));
        added.flags = flag;
        paths.push_back(std::move(added));
        return S_OK;
    }

    _Check_return_ HRESULT AddLivePaths(std::vector<LivePath>& paths, _In_opt_ wfc::IObservableVector<HSTRING>* pProperties, UINT flag, _Out_ bool* pHasAny)
    {
        *pHasAny = false;
        if (!pProperties)
        {
            return S_OK;
        }

        const auto& properties = static_cast<LiveShapingPropertyCollection*>(pProperties)->Paths();
        for (const auto& path : properties)
        {
            IFC_RETURN(AddLivePath(paths, path.Get(), flag));
        }

        *pHasAny = !properties.empty();
        return S_OK;
    }

    // Both collections are LiveShapingPropertyCollections (the CVS's and the view's).
    _Check_return_ HRESULT CopyLivePaths(_In_opt_ wfc::IObservableVector<HSTRING>* pFrom, bool isCopied, _In_ wfc::IObservableVector<HSTRING>* pTo)
    {
        LiveShapingPropertyCollection::Strings empty;
        const auto& paths = (isCopied && pFrom) ? static_cast<LiveShapingPropertyCollection*>(pFrom)->Paths() : empty;
        IFC_RETURN(static_cast<LiveShapingPropertyCollection*>(pTo)->SetPaths(paths));
        return S_OK;
    }
}

// Follows one live property of one source item, through the binding engine's property path listener, so a
// Microsoft.UI.Xaml.Data.INotifyPropertyChanged item, an ICustomPropertyProvider item and a DependencyObject
// all work, as for {Binding} (WPF: a LiveShapingItem binds to each live property).
class VectorCollectionView::LivePropertyObserver final : public IPropertyPathListenerHost
{
public:
    LivePropertyObserver(_In_ VectorCollectionView* pView, _In_ LiveShapingItem* pItem, UINT flags)
        : m_pView(pView), m_pItem(pItem), m_flags(flags)
    {
    }

    ~LivePropertyObserver()
    {
        Disconnect();
    }

    _Check_return_ HRESULT Connect(_In_ HSTRING path, _In_opt_ IInspectable* pSource)
    {
        UINT32 length = 0;
        LPCWSTR szPath = WindowsGetStringRawBuffer(path, &length);
        PropertyPathParser parser{};
        ctl::ComPtr<PropertyPathListener> spListener;
        IFC_RETURN(parser.SetSource(szPath, nullptr));
        IFC_RETURN(ctl::make<PropertyPathListener>(this, &parser, true /* fListenToChanges */, false /* fUseWeakReferenceForSource */, &spListener));
        m_listener.Append(spListener.Get());
        IFC_RETURN(spListener->SetSource(pSource));
        return S_OK;
    }

    // The listener's steps detach their PropertyChanged handlers when they are released.
    void Disconnect()
    {
        if (!m_listener.Empty())
        {
            m_listener.Begin()->Get()->ClearOwner();
            m_listener.Clear();
        }
        m_pView = nullptr;
    }

    // The listener holds the item (an app object), so the view walks it (VectorCollectionView::OnReferenceTrackerWalk).
    void ReferenceTrackerWalk(EReferenceTrackerWalkType walkType)
    {
        m_listener.ReferenceTrackerWalk(walkType);
    }

    LiveShapingItem* Item() const
    {
        return m_pItem;
    }

    UINT Flags() const
    {
        return m_flags;
    }

    _Check_return_ HRESULT GetTraceString(_Outptr_result_z_ const WCHAR** pszTraceString) override
    {
        *pszTraceString = L"";
        return S_OK;
    }

    // As for {Binding}, PropertyChanged must be raised on the view's thread; off it the change is an error,
    // not a change the view silently misses.
    _Check_return_ HRESULT SourceChanged() override
    {
        if (m_pView)
        {
            IFC_RETURN(m_pView->CheckThread());
            IFC_RETURN(m_pView->OnLivePropertyChanged(this));
        }
        return S_OK;
    }

private:
    VectorCollectionView* m_pView;
    LiveShapingItem* m_pItem;
    UINT m_flags;
    // At most one; a TrackerPtrVector so the view can walk it.
    TrackerPtrVector<PropertyPathListener> m_listener;
};

// One per source item, in source order (WPF: LiveShapingItem). Dirty flags mark a live property that changed.
struct VectorCollectionView::LiveShapingItem
{
    std::vector<std::unique_ptr<LivePropertyObserver>> observers;
    UINT dirtyFlags = 0;
};

struct VectorCollectionView::LiveShapingState
{
    std::vector<LivePath> paths;
    std::vector<std::unique_ptr<LiveShapingItem>> items;
    bool isRestoreQueued = false;
};

bool VectorCollectionView::IsLiveShaping() const
{
    return m_isLiveSorting || m_isLiveFiltering || m_isLiveGrouping;
}

// An item whose sort keys changed is out of place until the next restore, so a binary search over the output
// cannot place a new item; the source change is applied by a recompute instead.
bool VectorCollectionView::HasSortDirtyLiveItems() const
{
    if (!m_spLiveShaping || !m_isLiveSorting)
    {
        return false;
    }

    return std::any_of(m_spLiveShaping->items.begin(), m_spLiveShaping->items.end(),
        [](const std::unique_ptr<LiveShapingItem>& spItem) { return (spItem->dirtyFlags & LiveShapingFlag_Sorting) != 0; });
}

IFACEMETHODIMP VectorCollectionView::get_CanChangeLiveSorting(_Out_ BOOLEAN* pValue)
{
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue = TRUE;
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_CanChangeLiveFiltering(_Out_ BOOLEAN* pValue)
{
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue = TRUE;
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_CanChangeLiveGrouping(_Out_ BOOLEAN* pValue)
{
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue = TRUE;
    return S_OK;
}

namespace
{
    _Check_return_ HRESULT BoxIsLive(bool value, _Outptr_ wf::IReference<bool>** ppValue)
    {
        ctl::ComPtr<IInspectable> spValue;
        ctl::ComPtr<wf::IReference<bool>> spReference;
        IFC_RETURN(DirectUI::PropertyValue::CreateFromBoolean(value, &spValue));
        IFC_RETURN(spValue.As(&spReference));
        *ppValue = spReference.Detach();
        return S_OK;
    }
}

IFACEMETHODIMP VectorCollectionView::get_IsLiveSorting(_Outptr_result_maybenull_ wf::IReference<bool>** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    IFC_RETURN(BoxIsLive(m_isLiveSorting, ppValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::put_IsLiveSorting(_In_opt_ wf::IReference<bool>* pValue)
{
    IFC_RETURN(SetIsLive(&m_isLiveSorting, pValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_IsLiveFiltering(_Outptr_result_maybenull_ wf::IReference<bool>** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    IFC_RETURN(BoxIsLive(m_isLiveFiltering, ppValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::put_IsLiveFiltering(_In_opt_ wf::IReference<bool>* pValue)
{
    IFC_RETURN(SetIsLive(&m_isLiveFiltering, pValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_IsLiveGrouping(_Outptr_result_maybenull_ wf::IReference<bool>** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    IFC_RETURN(BoxIsLive(m_isLiveGrouping, ppValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::put_IsLiveGrouping(_In_opt_ wf::IReference<bool>* pValue)
{
    IFC_RETURN(SetIsLive(&m_isLiveGrouping, pValue));
    return S_OK;
}

// As in WPF's ListCollectionView, null is rejected: this view always knows whether it is live.
_Check_return_ HRESULT VectorCollectionView::SetIsLive(_In_ bool* pIsLive, _In_opt_ wf::IReference<bool>* pValue)
{
    boolean value = false;

    IFC_RETURN(CheckThread());
    ARG_NOTNULL_RETURN(pValue, "value");
    IFC_RETURN(pValue->get_Value(&value));
    if (*pIsLive != !!value)
    {
        const UINT flag = (pIsLive == &m_isLiveSorting) ? LiveShapingFlag_Sorting
            : ((pIsLive == &m_isLiveFiltering) ? LiveShapingFlag_Filtering : LiveShapingFlag_Grouping);
        *pIsLive = !!value;
        IFC_RETURN(OnIsLiveChanged(value ? flag : 0, value ? 0 : flag));
    }
    return S_OK;
}

// WPF (ListCollectionView.IsLiveSorting and the others): the view catches up on changes it did not follow
// (RebuildLocalArray), then raises PropertyChanged. Here the catch-up raises a Reset only if the order changed.
_Check_return_ HRESULT VectorCollectionView::OnIsLiveChanged(UINT turnedOn, UINT turnedOff)
{
    const UINT changed = turnedOn | turnedOff;

    IFC_RETURN(OnLiveShapingChanged());

    if (changed & LiveShapingFlag_Sorting)
    {
        IFC_RETURN(RaisePropertyChanged(STR_LEN_PAIR(L"IsLiveSorting")));
    }
    if (changed & LiveShapingFlag_Filtering)
    {
        IFC_RETURN(RaisePropertyChanged(STR_LEN_PAIR(L"IsLiveFiltering")));
    }
    if (changed & LiveShapingFlag_Grouping)
    {
        IFC_RETURN(RaisePropertyChanged(STR_LEN_PAIR(L"IsLiveGrouping")));
    }
    return S_OK;
}

void VectorCollectionView::OnReferenceTrackerWalk(INT walkType)
{
    if (m_spLiveShaping)
    {
        const auto walkTypeAsEnum = static_cast<EReferenceTrackerWalkType>(walkType);
        for (const auto& spItem : m_spLiveShaping->items)
        {
            for (const auto& spObserver : spItem->observers)
            {
                spObserver->ReferenceTrackerWalk(walkTypeAsEnum);
            }
        }
    }

    VectorCollectionViewGenerated::OnReferenceTrackerWalk(walkType);
}

IFACEMETHODIMP VectorCollectionView::get_LiveSortingProperties(_Outptr_ wfc::IObservableVector<HSTRING>** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    IFC_RETURN(EnsureLiveShapingProperties());
    IFC_RETURN(m_spLiveSortingProperties.CopyTo(ppValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_LiveFilteringProperties(_Outptr_ wfc::IObservableVector<HSTRING>** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    IFC_RETURN(EnsureLiveShapingProperties());
    IFC_RETURN(m_spLiveFilteringProperties.CopyTo(ppValue));
    return S_OK;
}

IFACEMETHODIMP VectorCollectionView::get_LiveGroupingProperties(_Outptr_ wfc::IObservableVector<HSTRING>** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());
    IFC_RETURN(EnsureLiveShapingProperties());
    IFC_RETURN(m_spLiveGroupingProperties.CopyTo(ppValue));
    return S_OK;
}

// WPF: OnLivePropertyListChanged. The collections can outlive the view, so they reach it weakly.
_Check_return_ HRESULT VectorCollectionView::EnsureLiveShapingProperties()
{
    if (m_spLiveSortingProperties)
    {
        return S_OK;
    }

    ctl::WeakRefPtr weakThis;
    IFC_RETURN(ctl::AsWeak(static_cast<CollectionView*>(this), &weakThis));
    auto onChanged = [weakThis]() mutable -> HRESULT
    {
        ctl::ComPtr<xaml_data::ICollectionView> spView;
        IFC_RETURN(weakThis.As(&spView));
        if (spView)
        {
            auto pView = static_cast<VectorCollectionView*>(static_cast<CollectionView*>(spView.Get()));
            if (!pView->m_isApplyingLiveShaping && pView->IsLiveShaping())
            {
                IFC_RETURN(pView->OnLiveShapingChanged());
            }
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

// WPF: the live shaping part of CollectionViewSource.ApplyPropertiesToView.
_Check_return_ HRESULT VectorCollectionView::ApplyLiveShaping(
    bool isLiveSorting,
    bool isLiveFiltering,
    bool isLiveGrouping,
    _In_opt_ wfc::IObservableVector<HSTRING>* pSortingProperties,
    _In_opt_ wfc::IObservableVector<HSTRING>* pFilteringProperties,
    _In_opt_ wfc::IObservableVector<HSTRING>* pGroupingProperties)
{
    HRESULT hr = S_OK;
    const UINT before = (m_isLiveSorting ? LiveShapingFlag_Sorting : 0) | (m_isLiveFiltering ? LiveShapingFlag_Filtering : 0) | (m_isLiveGrouping ? LiveShapingFlag_Grouping : 0);
    const UINT after = (isLiveSorting ? LiveShapingFlag_Sorting : 0) | (isLiveFiltering ? LiveShapingFlag_Filtering : 0) | (isLiveGrouping ? LiveShapingFlag_Grouping : 0);

    IFC_RETURN(EnsureLiveShapingProperties());

    m_isApplyingLiveShaping = true;
    m_isLiveSorting = isLiveSorting;
    m_isLiveFiltering = isLiveFiltering;
    m_isLiveGrouping = isLiveGrouping;
    hr = [&]() -> HRESULT
    {
        IFC_RETURN(CopyLivePaths(pSortingProperties, isLiveSorting, m_spLiveSortingProperties.Get()));
        IFC_RETURN(CopyLivePaths(pFilteringProperties, isLiveFiltering, m_spLiveFilteringProperties.Get()));
        IFC_RETURN(CopyLivePaths(pGroupingProperties, isLiveGrouping, m_spLiveGroupingProperties.Get()));
        return S_OK;
    }();
    m_isApplyingLiveShaping = false;
    IFC_RETURN(hr);

    IFC_RETURN(OnIsLiveChanged(after & ~before, before & ~after));
    return S_OK;
}

// A live feature or a property list changed (WPF: RebuildLocalArray): what is observed changes, and the view
// catches up on the changes it did not follow while they were not observed.
_Check_return_ HRESULT VectorCollectionView::OnLiveShapingChanged()
{
    if (m_shapeBusyDepth > 0)
    {
        m_isRecomputePending = true;
        return S_OK;
    }

    if (IsLiveShaping() && m_tpInternalList)
    {
        IFC_RETURN(RefreshOrDefer(true /* isSkippedWhenUnchanged */));
        return S_OK;
    }

    IFC_RETURN(RebuildLiveShapingItems());
    return S_OK;
}

// The properties to observe (WPF: LiveShapingList.SetLiveShapingProperties). Sorting and grouping default to
// the SortDescriptions' and GroupDescriptions' properties; filtering observes only what it is told to.
_Check_return_ HRESULT VectorCollectionView::RebuildLiveShapingItems()
{
    std::vector<LivePath> paths;
    UINT size = 0;
    bool hasAny = false;

    ClearLiveShapingItems();

    if (!IsLiveShaping() || !m_tpInternalList || !m_tpSourceList)
    {
        return S_OK;
    }

    IFC_RETURN(EnsureLiveShapingProperties());

    if (m_isLiveSorting)
    {
        IFC_RETURN(AddLivePaths(paths, m_spLiveSortingProperties.Get(), LiveShapingFlag_Sorting, &hasAny));
        if (!hasAny && m_tpSortDescriptions && !m_tpCustomSort)
        {
            UINT count = 0;
            IFC_RETURN(m_tpSortDescriptions->get_Size(&count));
            for (UINT i = 0; i < count; ++i)
            {
                ctl::ComPtr<xaml_data::ISortDescription> spDescription;
                wrl_wrappers::HString path;
                IFC_RETURN(m_tpSortDescriptions->GetAt(i, spDescription.ReleaseAndGetAddressOf()));
                if (spDescription)
                {
                    IFC_RETURN(spDescription->get_PropertyName(path.GetAddressOf()));
                    IFC_RETURN(AddLivePath(paths, path.Get(), LiveShapingFlag_Sorting));
                }
            }
        }
    }

    if (m_isLiveFiltering)
    {
        IFC_RETURN(AddLivePaths(paths, m_spLiveFilteringProperties.Get(), LiveShapingFlag_Filtering, &hasAny));
    }

    if (m_isLiveGrouping)
    {
        IFC_RETURN(AddLivePaths(paths, m_spLiveGroupingProperties.Get(), LiveShapingFlag_Grouping, &hasAny));
        if (!hasAny && m_tpGroupDescriptions)
        {
            UINT count = 0;
            IFC_RETURN(m_tpGroupDescriptions->get_Size(&count));
            for (UINT i = 0; i < count; ++i)
            {
                ctl::ComPtr<xaml_data::IPropertyGroupDescription> spDescription;
                wrl_wrappers::HString path;
                IFC_RETURN(m_tpGroupDescriptions->GetAt(i, spDescription.ReleaseAndGetAddressOf()));
                if (spDescription)
                {
                    IFC_RETURN(spDescription->get_PropertyName(path.GetAddressOf()));
                    IFC_RETURN(AddLivePath(paths, path.Get(), LiveShapingFlag_Grouping));
                }
            }
        }
    }

    if (paths.empty())
    {
        return S_OK;
    }

    auto spState = std::make_shared<LiveShapingState>();
    spState->paths = std::move(paths);

    IFC_RETURN(m_tpSourceList->get_Size(&size));
    spState->items.reserve(size);
    for (UINT i = 0; i < size; ++i)
    {
        std::unique_ptr<LiveShapingItem> spItem;
        IFC_RETURN(CreateLiveShapingItem(*spState, i, &spItem));
        spState->items.push_back(std::move(spItem));
    }

    // A rebuild that app code (a getter) caused while this one ran is newer.
    if (m_spLiveShaping || !IsLiveShaping())
    {
        DisconnectLiveShapingItems(*spState);
        return S_OK;
    }

    // The tracker walk reads the items under this lock.
    AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());
    m_spLiveShaping = std::move(spState);
    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::CreateLiveShapingItem(_In_ const LiveShapingState& state, UINT sourceIndex, _Out_ std::unique_ptr<LiveShapingItem>* pItem)
{
    ctl::ComPtr<IInspectable> spSourceItem;
    auto spItem = std::make_unique<LiveShapingItem>();

    IFC_RETURN(m_tpSourceList->GetAt(sourceIndex, spSourceItem.ReleaseAndGetAddressOf()));
    for (const auto& livePath : state.paths)
    {
        auto spObserver = std::make_unique<LivePropertyObserver>(this, spItem.get(), livePath.flags);
        // A path the item does not have is observed as a binding to it would be: it never changes.
        if (SUCCEEDED(spObserver->Connect(livePath.path.Get(), spSourceItem.Get())))
        {
            spItem->observers.push_back(std::move(spObserver));
        }
    }

    *pItem = std::move(spItem);
    return S_OK;
}

void VectorCollectionView::DisconnectLiveShapingItems(_In_ LiveShapingState& state)
{
    for (auto& spItem : state.items)
    {
        for (auto& spObserver : spItem->observers)
        {
            spObserver->Disconnect();
        }
    }
}

void VectorCollectionView::ClearLiveShapingItems()
{
    std::shared_ptr<LiveShapingState> spState;
    {
        AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());
        spState = std::move(m_spLiveShaping);
    }

    if (spState)
    {
        DisconnectLiveShapingItems(*spState);
    }
}

// The source changed and the change was applied incrementally: observe the new item, stop observing the old.
_Check_return_ HRESULT VectorCollectionView::UpdateLiveShapingItems(wfc::CollectionChange change, UINT index)
{
    if (!m_spLiveShaping)
    {
        return S_OK;
    }

    auto spState = m_spLiveShaping;
    auto& items = spState->items;
    const UINT expectedSize = static_cast<UINT>(items.size())
        + ((change == wfc::CollectionChange_ItemInserted) ? 1 : 0)
        - ((change == wfc::CollectionChange_ItemRemoved) ? 1 : 0);

    if (expectedSize != m_sourceListSize || index > items.size() || (change != wfc::CollectionChange_ItemInserted && index >= items.size()))
    {
        IFC_RETURN(RebuildLiveShapingItems());
        return S_OK;
    }

    if (change != wfc::CollectionChange_ItemInserted)
    {
        std::unique_ptr<LiveShapingItem> spRemoved;
        {
            AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());
            spRemoved = std::move(items[index]);
            items.erase(items.begin() + index);
        }
        for (auto& spObserver : spRemoved->observers)
        {
            spObserver->Disconnect();
        }
    }

    if (change != wfc::CollectionChange_ItemRemoved)
    {
        std::unique_ptr<LiveShapingItem> spItem;
        IFC_RETURN(CreateLiveShapingItem(*spState, index, &spItem));
        if (m_spLiveShaping == spState)
        {
            AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());
            items.insert(items.begin() + index, std::move(spItem));
        }
        else
        {
            for (auto& spObserver : spItem->observers)
            {
                spObserver->Disconnect();
            }
        }
    }

    return S_OK;
}

// A live property of an item changed (WPF: LiveShapingList.OnItemPropertyChanged). As in WPF, the item is
// marked and the view is restored once, at the next dispatcher turn, for every item that changed meanwhile.
_Check_return_ HRESULT VectorCollectionView::OnLivePropertyChanged(_In_ LivePropertyObserver* pObserver)
{
    auto spState = m_spLiveShaping;
    if (!spState)
    {
        return S_OK;
    }

    pObserver->Item()->dirtyFlags |= pObserver->Flags();

    if (!spState->isRestoreQueued && m_deferRefreshLevel == 0)
    {
        auto pCore = DXamlCore::GetCurrent();
        if (pCore && pCore->GetXamlDispatcherNoRef())
        {
            ctl::ComPtr<VectorCollectionView> spThis(this);
            spState->isRestoreQueued = true;
            IFC_RETURN(pCore->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(spThis, &VectorCollectionView::OnLiveShapingRestoreCallback)));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT VectorCollectionView::OnLiveShapingRestoreCallback()
{
    if (m_spLiveShaping)
    {
        m_spLiveShaping->isRestoreQueued = false;
    }

    IFC_RETURN(RestoreLiveShaping());
    return S_OK;
}

// WPF: ListCollectionView.RestoreLiveShaping. Each changed item is moved to where it now belongs, filtered in
// or out, or moved to its new group, with an ItemRemoved and an ItemInserted (WinRT vectors have no Move); an
// item still in order raises nothing. Like WPF's FindPosition, placing one item ignores the changed items not
// yet placed, whose positions are stale.
_Check_return_ HRESULT VectorCollectionView::RestoreLiveShaping()
{
    auto spState = m_spLiveShaping;
    ctl::ComPtr<IInspectable> spCurrentItem;
    ctl::ComPtr<CollectionViewSource> spFilterHost;
    SortFieldComparer comparer;
    std::vector<bool> isSortDirty;
    const UINT sourceChangeCount = m_sourceListChangeCount;
    bool hasDirty = false;
    bool isApplied = true;

    if (!spState || !m_tpInternalList || m_deferRefreshLevel > 0)
    {
        return S_OK;
    }

    if (m_shapeBusyDepth > 0)
    {
        m_isRecomputePending = true;
        return S_OK;
    }

    for (const auto& spItem : spState->items)
    {
        hasDirty = hasDirty || spItem->dirtyFlags != 0;
    }

    if (!hasDirty)
    {
        return S_OK;
    }

    // A source change that could not be followed is pending: recompute, which observes the items afresh.
    if (!m_areSourceIndicesValid || spState->items.size() != m_sourceListSize)
    {
        IFC_RETURN(RefreshOrDefer());
        return S_OK;
    }

    IFC_RETURN(get_CurrentItemImpl(spCurrentItem.ReleaseAndGetAddressOf()));
    IFC_RETURN(comparer.Initialize(m_tpSortDescriptions.Get(), m_tpCustomSort.Get(), m_culture.Get()));
    IFC_RETURN(GetFilterHost(spFilterHost.ReleaseAndGetAddressOf()));

    const bool hasFilter = !!spFilterHost || !!m_tpFilter;
    const bool isSorting = m_isLiveSorting && !comparer.IsEmpty();
    isSortDirty.resize(spState->items.size());
    for (size_t i = 0; i < spState->items.size(); ++i)
    {
        isSortDirty[i] = isSorting && (spState->items[i]->dirtyFlags & LiveShapingFlag_Sorting) != 0;
    }

    auto isSkipped = [&isSortDirty](UINT sourceIndex) { return sourceIndex < isSortDirty.size() && isSortDirty[sourceIndex]; };

    // A flat view: the item's place in the InternalList.
    auto restoreFlat = [&](UINT sourceIndex, UINT flags, _In_opt_ IInspectable* pItem, _Out_ bool* pIsItemApplied) -> HRESULT
    {
        auto pOutput = static_cast<InternalList*>(m_tpInternalList.Get());
        auto found = std::find(m_sourceIndices.begin(), m_sourceIndices.end(), sourceIndex);
        const bool isInView = found != m_sourceIndices.end();
        const UINT oldPosition = isInView ? static_cast<UINT>(found - m_sourceIndices.begin()) : UINT_MAX;
        const bool isFiltering = (flags & LiveShapingFlag_Filtering) && m_isLiveFiltering && hasFilter;
        const bool isResorting = (flags & LiveShapingFlag_Sorting) && isSorting;
        bool isAccepted = isInView;
        bool isMoving = false;
        UINT newPosition = 0;
        HRESULT hr = S_OK;

        *pIsItemApplied = true;
        ++m_shapeBusyDepth;
        hr = [&]() -> HRESULT
        {
            if (isFiltering)
            {
                IFC_RETURN(PassesFilter(pItem, spFilterHost.Get(), &isAccepted));
            }

            if (!isAccepted || (isInView && !isResorting))
            {
                return S_OK;
            }

            std::vector<ctl::ComPtr<IInspectable>> keys;
            UINT first = 0;
            UINT last = 0;
            IFC_RETURN(comparer.EvaluateKeys(pItem, &keys));
            IFC_RETURN(FindSortedRange(
                static_cast<UINT>(m_sourceIndices.size()),
                oldPosition,
                [pOutput](UINT position, IInspectable** ppOther) { return pOutput->GetAt(position, ppOther); },
                [this](UINT position) { return m_sourceIndices[position]; },
                isSkipped,
                comparer, pItem, keys, sourceIndex, &newPosition, &first, &last));
            isMoving = isInView && (oldPosition < first || oldPosition > last);
            return S_OK;
        }();
        --m_shapeBusyDepth;
        IFC_RETURN(hr);

        if (sourceChangeCount != m_sourceListChangeCount)
        {
            *pIsItemApplied = false;
            return S_OK;
        }

        const bool isRemoved = isInView && (!isAccepted || isMoving);
        const bool isInserted = isAccepted && (!isInView || isMoving);
        if (!isRemoved && !isInserted)
        {
            return S_OK;
        }

        ++m_shapeBusyDepth;
        hr = [&]() -> HRESULT
        {
            if (isRemoved)
            {
                m_sourceIndices.erase(m_sourceIndices.begin() + oldPosition);
                IFC_RETURN(pOutput->InternalRemoveAt(oldPosition));
            }

            if (isInserted)
            {
                m_sourceIndices.insert(m_sourceIndices.begin() + newPosition, sourceIndex);
                IFC_RETURN(pOutput->InternalInsertAt(newPosition, pItem));
            }
            return S_OK;
        }();
        --m_shapeBusyDepth;
        IFC_RETURN(hr);
        return S_OK;
    };

    // A grouped view (WPF: CollectionViewGroupRoot.RestoreGrouping and MoveItemWithinGroups).
    auto restoreGrouped = [&](UINT sourceIndex, UINT flags, _In_opt_ IInspectable* pItem, _Out_ bool* pIsItemApplied) -> HRESULT
    {
        auto pRoot = static_cast<CollectionViewGroupRoot*>(static_cast<CollectionViewGroupInternal*>(m_spGroupRoot.Get()));
        CollectionViewGroupInternal* pOldGroup = nullptr;
        UINT oldPosition = 0;
        const bool isInView = pRoot && pRoot->FindLeaf(sourceIndex, &pOldGroup, &oldPosition);
        const bool isFiltering = (flags & LiveShapingFlag_Filtering) && m_isLiveFiltering && hasFilter;
        const bool isResorting = (flags & LiveShapingFlag_Sorting) && isSorting;
        const bool isRegrouping = (flags & LiveShapingFlag_Grouping) && m_isLiveGrouping;
        std::vector<ctl::ComPtr<IInspectable>> keys;
        GroupedInsertion insertion;
        bool isAccepted = isInView;
        bool isMoving = false;
        HRESULT hr = S_OK;

        *pIsItemApplied = true;
        if (!pRoot)
        {
            return S_OK;
        }

        ++m_shapeBusyDepth;
        hr = [&]() -> HRESULT
        {
            if (isFiltering)
            {
                IFC_RETURN(PassesFilter(pItem, spFilterHost.Get(), &isAccepted));
            }

            if (!isAccepted || (isInView && !isResorting && !isRegrouping))
            {
                return S_OK;
            }

            IFC_RETURN(comparer.EvaluateKeys(pItem, &keys));
            if (!isInView)
            {
                IFC_RETURN(PlanGroupedInsertion(pRoot, comparer, pItem, keys, sourceIndex, nullptr /* pExcludedGroup */, UINT_MAX, &insertion, &isSortDirty));
                return S_OK;
            }

            bool isInGroup = true;
            if (isRegrouping)
            {
                std::vector<ctl::ComPtr<IInspectable>> names;
                IFC_RETURN(pRoot->GetGroupNames(pItem, &names));
                IFC_RETURN(pRoot->IsGroupPath(pOldGroup, names, &isInGroup));
            }

            if (!isInGroup)
            {
                // Planned once it has left its group, which may go with it.
                isMoving = true;
                return S_OK;
            }

            if (isResorting)
            {
                UINT position = 0;
                UINT first = 0;
                UINT last = 0;
                IFC_RETURN(FindLeafPosition(pOldGroup, comparer, pItem, keys, sourceIndex, oldPosition, &position, &isSortDirty, &first, &last));
                if (oldPosition < first || oldPosition > last)
                {
                    isMoving = true;
                    insertion.isAccepted = true;
                    insertion.pGroup = pOldGroup;
                    insertion.position = position;
                }
            }
            return S_OK;
        }();
        --m_shapeBusyDepth;
        IFC_RETURN(hr);

        if (sourceChangeCount != m_sourceListChangeCount)
        {
            *pIsItemApplied = false;
            return S_OK;
        }

        const bool isRemoved = isInView && (!isAccepted || isMoving);
        const bool isInserted = isAccepted && (!isInView || isMoving);
        if (!isRemoved && !isInserted)
        {
            return S_OK;
        }

        ++m_shapeBusyDepth;
        hr = [&]() -> HRESULT
        {
            if (isRemoved)
            {
                IFC_RETURN(ApplyGroupedRemoval(pOldGroup, oldPosition));
            }

            if (isInserted)
            {
                if (!insertion.pGroup)
                {
                    IFC_RETURN(PlanGroupedInsertion(pRoot, comparer, pItem, keys, sourceIndex, nullptr /* pExcludedGroup */, UINT_MAX, &insertion, &isSortDirty));
                }
                IFC_RETURN(ApplyGroupedInsertion(insertion, pItem, sourceIndex));
            }
            return S_OK;
        }();
        --m_shapeBusyDepth;
        IFC_RETURN(hr);
        return S_OK;
    };

    for (UINT i = 0; i < isSortDirty.size() && isApplied; ++i)
    {
        // A handler of a change raised here may have refreshed the view, which observes the items afresh.
        if (m_spLiveShaping != spState || spState->items.size() != isSortDirty.size())
        {
            break;
        }

        const UINT flags = std::exchange(spState->items[i]->dirtyFlags, 0u);
        if (flags != 0)
        {
            ctl::ComPtr<IInspectable> spItem;
            IFC_RETURN(m_tpSourceList->GetAt(i, spItem.ReleaseAndGetAddressOf()));
            IFC_RETURN(m_isGrouping ? restoreGrouped(i, flags, spItem.Get(), &isApplied) : restoreFlat(i, flags, spItem.Get(), &isApplied));
        }
        isSortDirty[i] = false;
        isApplied = isApplied && (sourceChangeCount == m_sourceListChangeCount);
    }

    if (!isApplied)
    {
        m_areSourceIndicesValid = false;
        IFC_RETURN(RefreshOrDefer());
        return S_OK;
    }

    // Currency follows the item, as after a recompute.
    if (spCurrentItem)
    {
        ctl::ComPtr<IInspectable> spNowCurrent;
        IFC_RETURN(get_CurrentItemImpl(spNowCurrent.ReleaseAndGetAddressOf()));
        if (spNowCurrent.Get() != spCurrentItem.Get())
        {
            BOOLEAN isCurrentValid = FALSE;
            IFC_RETURN(MoveCurrentToImpl(spCurrentItem.Get(), &isCurrentValid));
            if (!isCurrentValid)
            {
                IFC_RETURN(MoveCurrentToFirstImpl(&isCurrentValid));
            }
        }
    }

    IFC_RETURN(RunPendingRecompute());
    return S_OK;
}
