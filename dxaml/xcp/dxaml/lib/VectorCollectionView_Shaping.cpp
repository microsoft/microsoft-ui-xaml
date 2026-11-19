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

    // A computed group (WPF: MS.Internal.Data.CollectionViewGroupInternal). As a vector it holds the group's
    // leaves (the items of all its subgroups, in order), so a top-level group is the GroupItems of its
    // ICollectionViewGroup. As an ICollectionViewGroup itself, Group is the group and GroupItems is WPF's
    // CollectionViewGroup.Items: the subgroups, or the items at the bottom level. Bindings see Key (the
    // IGrouping<K,T> idiom), Name, Items, ItemCount and IsBottomLevel.
    class __declspec(novtable) CollectionViewGroupInternal :
        public InternalList,
        public xaml_data::ICustomPropertyProvider,
        public xaml_data::ICollectionViewGroup
    {
        BEGIN_INTERFACE_MAP(CollectionViewGroupInternal, InternalList)
            INTERFACE_ENTRY(CollectionViewGroupInternal, xaml_data::ICustomPropertyProvider)
            INTERFACE_ENTRY(CollectionViewGroupInternal, xaml_data::ICollectionViewGroup)
        END_INTERFACE_MAP(CollectionViewGroupInternal, InternalList)

    public:
        _Check_return_ HRESULT InitializeGroup(_In_opt_ IInspectable* pKey, _In_opt_ CollectionViewGroupInternal* pParent, bool isBottomLevel)
        {
            m_spKey = pKey;
            m_pParent = pParent;
            if (!isBottomLevel)
            {
                IFC_RETURN(ctl::ComObject<InternalList>::CreateInstance(m_spSubgroupList.ReleaseAndGetAddressOf()));
            }
            return S_OK;
        }

        IInspectable* GetKeyNoRef() const
        {
            return m_spKey.Get();
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
            return !m_spSubgroupList;
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
                IFC_RETURN(m_spSubgroupList->InternalInsertAt(index, pSubgroupItem));
            }
            else
            {
                IFC_RETURN(m_spSubgroupList->TrackerCollection<IInspectable*>::InsertAt(index, pSubgroupItem));
            }
            return S_OK;
        }

        _Check_return_ HRESULT RemoveSubgroup(UINT index)
        {
            m_subgroups[index]->ClearParent();
            m_subgroups.erase(m_subgroups.begin() + index);
            IFC_RETURN(m_spSubgroupList->InternalRemoveAt(index));
            return S_OK;
        }

        _Check_return_ HRESULT InsertLeaf(UINT index, _In_opt_ IInspectable* pItem, bool isRaised)
        {
            if (isRaised)
            {
                IFC_RETURN(InternalInsertAt(index, pItem));
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
            ctl::ComPtr<IInspectable> spItems = m_spSubgroupList ? ctl::as_iinspectable(m_spSubgroupList.Get()) : ctl::as_iinspectable(this);
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
                    return FromTarget(pTarget)->m_spKey.CopyTo(ppResult);
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
            else if (InlineIsEqualGUID(iid, __uuidof(xaml_data::ICollectionViewGroup)))
            {
                *ppObject = static_cast<xaml_data::ICollectionViewGroup*>(this);
            }
            else
            {
                return InternalList::QueryInterfaceImpl(iid, ppObject);
            }

            AddRefOuter();
            return S_OK;
        }

        // The subgroups, typed, and as the observable list GroupItems presents (the root's is the InternalList).
        std::vector<ctl::ComPtr<CollectionViewGroupInternal>> m_subgroups;
        ctl::ComPtr<InternalList> m_spSubgroupList;

    private:
        static CollectionViewGroupInternal* FromTarget(_In_ IInspectable* pTarget)
        {
            ctl::ComPtr<IInspectable> spTarget(pTarget);
            ctl::ComPtr<xaml_data::ICustomPropertyProvider> spProvider;
            VERIFYHR(spTarget.As(&spProvider));
            return spProvider.Cast<CollectionViewGroupInternal>();
        }

        // Not a TrackerPtr: ObservableTrackerCollection::OnReferenceTrackerWalk is final.
        ctl::ComPtr<IInspectable> m_spKey;
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

    // One level of grouping: the group name of an item (WPF: PropertyGroupDescription.GroupNameFromItem / NamesMatch).
    struct GroupLevel
    {
        bool hasDescription = false;
        ShapingKeySelector selector;
        ctl::ComPtr<xaml_data::IValueConverter> spConverter;
    };

    // The top of the group tree (WPF: MS.Internal.Data.CollectionViewGroupRoot). Its subgroups are the view's
    // InternalList; its own leaves are not kept.
    class __declspec(novtable) CollectionViewGroupRoot : public CollectionViewGroupInternal
    {
    public:
        _Check_return_ HRESULT InitializeRoot(_In_ wfc::IVector<xaml_data::PropertyGroupDescription*>* pDescriptions)
        {
            UINT count = 0;

            IFC_RETURN(InitializeGroup(nullptr, nullptr, false /* isBottomLevel */));
            IFC_RETURN(pDescriptions->get_Size(&count));
            m_levels.clear();
            m_levels.resize(count);
            for (UINT i = 0; i < count; ++i)
            {
                ctl::ComPtr<xaml_data::IPropertyGroupDescription> spDescription;
                IFC_RETURN(pDescriptions->GetAt(i, spDescription.ReleaseAndGetAddressOf()));
                if (spDescription)
                {
                    wrl_wrappers::HString path;
                    IFC_RETURN(spDescription->get_PropertyName(path.GetAddressOf()));
                    IFC_RETURN(CreateKeySelector(path.Get(), &m_levels[i].selector));
                    IFC_RETURN(spDescription->get_Converter(m_levels[i].spConverter.ReleaseAndGetAddressOf()));
                    m_levels[i].hasDescription = true;
                }
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
            m_spSubgroupList = pList;
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
                    IFC_RETURN(groupLevel.spConverter->Convert((*pNames)[level].Get(), objectType, nullptr /* parameter */, nullptr /* language */, spConverted.ReleaseAndGetAddressOf()));
                    (*pNames)[level] = std::move(spConverted);
                }
            }

            return S_OK;
        }

        _Check_return_ HRESULT NamesMatch(UINT level, _In_opt_ IInspectable* pGroupName, _In_opt_ IInspectable* pItemName, _Out_ bool* pMatch) const
        {
            UNREFERENCED_PARAMETER(level);
            IFC_RETURN(DirectUI::PropertyValue::AreEqual(pGroupName, pItemName, pMatch));
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
                    IFC_RETURN(CreateGroup(names[level].Get(), pGroup, level, spSubgroup.ReleaseAndGetAddressOf()));
                    IFC_RETURN(pGroup->InsertSubgroup(static_cast<UINT>(pGroup->Subgroups().size()), spSubgroup.Get(), false /* isRaised */));
                    pSubgroup = spSubgroup.Get();
                }
                pGroup = pSubgroup;
            }

            pGroup->SourceIndices().push_back(sourceIndex);
            for (auto pAncestor = pGroup; pAncestor && !pAncestor->IsRoot(); pAncestor = pAncestor->GetParentNoRef())
            {
                IFC_RETURN(pAncestor->InsertLeaf(pAncestor->LeafCount(), pItem, false /* isRaised */));
            }

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

    // The item's index in a bottom-level group: after the items that sort before or with it. excludedPosition is
    // a replaced item still in the group, or UINT_MAX.
    _Check_return_ HRESULT FindLeafPosition(
        _In_ CollectionViewGroupInternal* pGroup,
        const SortFieldComparer& comparer,
        _In_opt_ IInspectable* pItem,
        const std::vector<ctl::ComPtr<IInspectable>>& keys,
        UINT sourceIndex,
        UINT excludedPosition,
        _Out_ UINT* pPosition)
    {
        const auto& sourceIndices = pGroup->SourceIndices();
        const UINT count = pGroup->LeafCount() - ((excludedPosition < pGroup->LeafCount()) ? 1 : 0);
        UINT low = 0;
        UINT high = count;

        while (low < high)
        {
            const UINT middle = low + (high - low) / 2;
            const UINT position = (middle >= excludedPosition) ? middle + 1 : middle;
            ctl::ComPtr<IInspectable> spOther;
            int result = 0;
            IFC_RETURN(pGroup->GetAt(position, spOther.ReleaseAndGetAddressOf()));
            IFC_RETURN(comparer.Compare(pItem, keys, sourceIndex, spOther.Get(), nullptr /* pRightKeys */, sourceIndices[position], &result));
            if (result < 0)
            {
                high = middle;
            }
            else
            {
                low = middle + 1;
            }
        }

        *pPosition = low;
        return S_OK;
    }

    // A new subgroup's index (WPF: CollectionViewGroupInternal.FindIndex): before the first subgroup whose first
    // item sorts after the new group's item.
    _Check_return_ HRESULT FindSubgroupPosition(
        _In_ CollectionViewGroupInternal* pParent,
        const SortFieldComparer& comparer,
        _In_opt_ IInspectable* pItem,
        const std::vector<ctl::ComPtr<IInspectable>>& keys,
        UINT sourceIndex,
        _Out_ UINT* pPosition)
    {
        const auto& subgroups = pParent->Subgroups();
        UINT position = 0;

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
        _Out_ GroupedInsertion* pInsertion)
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
            IFC_RETURN(FindSubgroupPosition(pGroup, comparer, pItem, keys, sourceIndex, &pInsertion->position));
            return S_OK;
        }

        pInsertion->pGroup = pGroup;
        IFC_RETURN(FindLeafPosition(pGroup, comparer, pItem, keys, sourceIndex, (pGroup == pExcludedGroup) ? excludedPosition : UINT_MAX, &pInsertion->position));
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
        std::vector<ctl::ComPtr<IInspectable>> groupNames;
        UINT sourceIndex = 0;
    };

    SortFieldComparer sortComparer;
    ctl::ComPtr<CollectionViewGroupRoot> spRoot;
    std::vector<Entry> entries;
    std::vector<ctl::ComPtr<IInspectable>> outputItems;
    std::vector<UINT> outputSourceIndices;
    const UINT sourceChangeCount = m_sourceListChangeCount;
    UINT size = 0;

    IFC_RETURN(sortComparer.Initialize(m_tpSortDescriptions.Get(), m_tpCustomSort.Get()));

    if (m_isGrouping)
    {
        IFC_RETURN(ctl::ComObject<CollectionViewGroupRoot>::CreateInstance(spRoot.ReleaseAndGetAddressOf()));
        IFC_RETURN(spRoot->InitializeRoot(m_tpGroupDescriptions.Get()));
    }

    IFC_RETURN(m_tpSourceList->get_Size(&size));
    entries.reserve(size);

    for (UINT i = 0; i < size; ++i)
    {
        Entry entry;
        bool isAccepted = true;
        entry.sourceIndex = i;
        IFC_RETURN(m_tpSourceList->GetAt(i, entry.item.ReleaseAndGetAddressOf()));

        IFC_RETURN(PassesFilter(entry.item.Get(), &isAccepted));
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

    // Recorded before ReplaceAll raises, so a handler that changes the source sees consistent state.
    m_sourceIndices = std::move(outputSourceIndices);
    m_spGroupRoot = spRoot ? static_cast<xaml_data::ICollectionViewGroup*>(spRoot.Get()) : nullptr;
    m_sourceListSize = size;
    m_areSourceIndicesValid = (sourceChangeCount == m_sourceListChangeCount);
    IFC_RETURN(static_cast<InternalList*>(m_tpInternalList.Get())->ReplaceAll(outputItems));
    return S_OK;
}

// Whether the view shows the item (WPF: CollectionView.PassesFilter).
_Check_return_ HRESULT VectorCollectionView::PassesFilter(_In_opt_ IInspectable* pItem, _Out_ bool* pIsAccepted)
{
    *pIsAccepted = true;
    if (m_tpFilter)
    {
        boolean accepted = true;
        IFC_RETURN(m_tpFilter->Invoke(pItem, &accepted));
        *pIsAccepted = !!accepted;
    }
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

    if (m_areSourceIndicesValid && change != wfc::CollectionChange_Reset)
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

        m_isComputingShape = true;
        hr = [&]() -> HRESULT
        {
            SortFieldComparer comparer;
            std::vector<ctl::ComPtr<IInspectable>> keys;
            bool isAccepted = true;

            IFC_RETURN(PassesFilter(spItem.Get(), &isAccepted));
            if (!isAccepted)
            {
                return S_OK;
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

        m_isComputingShape = true;
        hr = [&]() -> HRESULT
        {
            bool isAccepted = true;
            IFC_RETURN(PassesFilter(spItem.Get(), &isAccepted));
            if (!isAccepted)
            {
                return S_OK;
            }

            IFC_RETURN(comparer.Initialize(m_tpSortDescriptions.Get(), m_tpCustomSort.Get()));
            IFC_RETURN(comparer.EvaluateKeys(spItem.Get(), &keys));
            IFC_RETURN(PlanGroupedInsertion(pRoot, comparer, spItem.Get(), keys, index, hasOld ? pOldGroup : nullptr, oldPosition, &insertion));
            return S_OK;
        }();
        m_isComputingShape = false;
        IFC_RETURN(hr);

        if (sourceChangeCount != m_sourceListChangeCount)
        {
            return S_OK;
        }
    }

    m_isComputingShape = true;
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
    m_isComputingShape = false;
    IFC_RETURN(hr);

    // A handler changed the source while the change was raised: recompute fully.
    *pIsApplied = (sourceChangeCount == m_sourceListChangeCount);
    return S_OK;
}
