// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      The internals of the CVS shaping engine (VectorCollectionView_Shaping.cpp, _SourceChanges.cpp,
//      _LiveShaping.cpp): sort keys, the group tree, failure containment. Private to those files. One named
//      namespace, so the inline thread_locals are one per thread across them.

#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>
#include "CollectionViewSource.g.h"
#include "PropertyGroupDescription.g.h"
#include "SortDescription.g.h"
#include "VectorCollectionView.g.h"
#include "GroupedDataCollectionView.g.h"
#include "PropertyChangedEventArgs.g.h"
#include "FrameworkElement.g.h"
#include "CollectionViewRefreshDeferral.h"
#include "PropertyPathParser.h"
#include "PropertyPath.h"
#include "Callback.h"
#include "CollectionViewLiveShapingProperties.h"

namespace DirectUI::ShapingEngine
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

    inline ShapingPropertyPathHost s_shapingPropertyPathHost;

    // While the engine applies a change, an app handler of a change it raises that fails does not stop it: the
    // change is finished, so the view stays consistent, and the first failure is returned afterwards.
    inline thread_local HRESULT* t_pRaiseFailure = nullptr;

    class RaiseFailureScope
    {
    public:
        RaiseFailureScope() : m_pPrevious(t_pRaiseFailure)
        {
            t_pRaiseFailure = &m_hr;
        }

        ~RaiseFailureScope()
        {
            t_pRaiseFailure = m_pPrevious;
        }

        RaiseFailureScope(const RaiseFailureScope&) = delete;
        RaiseFailureScope& operator=(const RaiseFailureScope&) = delete;

        HRESULT Result() const
        {
            return m_hr;
        }

    private:
        HRESULT m_hr = S_OK;
        HRESULT* m_pPrevious;
    };

    // Failures no app code can cause, which are not contained.
    inline bool IsUncontainedFailure(HRESULT hr)
    {
        return hr == E_OUTOFMEMORY || hr == RPC_E_WRONG_THREAD;
    }

    // A diagnostics event (VectorCollectionView_Shaping.cpp) for a contained failure that is not reported.
    void TraceContainedFailure(HRESULT hr, HRESULT capturedHr, bool hasErrorInfo);

    // A source change or a live restore no app caller waits for: the first failure of app code during it, with the
    // error info the app's code left (its message and stack), reported once the view is consistent again.
    class ContainedFailureReport
    {
    public:
        ContainedFailureReport() : m_pPrevious(s_pCurrent)
        {
            s_pCurrent = this;
        }

        ~ContainedFailureReport()
        {
            s_pCurrent = m_pPrevious;
        }

        ContainedFailureReport(const ContainedFailureReport&) = delete;
        ContainedFailureReport& operator=(const ContainedFailureReport&) = delete;

        static void Capture(HRESULT hr)
        {
            auto pCurrent = s_pCurrent;
            if (!pCurrent || !FAILED(hr) || FAILED(pCurrent->m_hr))
            {
                return;
            }

            pCurrent->m_hr = hr;
            Microsoft::WRL::ComPtr<IRestrictedErrorInfo> spErrorInfo;
            if (SUCCEEDED(::GetRestrictedErrorInfo(&spErrorInfo)) && spErrorInfo)
            {
                IGNOREHR(::SetRestrictedErrorInfo(spErrorInfo.Get()));
                BSTR description = nullptr;
                BSTR restrictedDescription = nullptr;
                BSTR capabilitySid = nullptr;
                HRESULT errorInfoHr = S_OK;
                if (SUCCEEDED(spErrorInfo->GetErrorDetails(&description, &errorInfoHr, &restrictedDescription, &capabilitySid)) && errorInfoHr == hr)
                {
                    pCurrent->m_spErrorInfo = spErrorInfo;
                }
                SysFreeString(description);
                SysFreeString(restrictedDescription);
                SysFreeString(capabilitySid);
            }
        }

        // Application.UnhandledException sees the failure, as for app code a dispatcher callback runs. Only a failure
        // that carries error info (an exception app code threw) is reported; a framework listener's bare HRESULT is
        // not turned into an app crash. The report runs synchronously inside the source's change notification, so an
        // UnhandledException handler that changes the same source re-enters it (an ObservableCollection throws then).
        void Report(HRESULT hr)
        {
            if (hr != m_hr || !m_spErrorInfo)
            {
                // Contained and not reported (no error info: most likely a framework listener's HRESULT).
                TraceContainedFailure(hr, m_hr, !!m_spErrorInfo);
                return;
            }
            IGNOREHR(::SetRestrictedErrorInfo(m_spErrorInfo.Get()));
            IGNOREHR(ErrorHelper::ReportUnhandledError(hr));
        }

    private:
        static inline thread_local ContainedFailureReport* s_pCurrent = nullptr;
        HRESULT m_hr = S_OK;
        Microsoft::WRL::ComPtr<IRestrictedErrorInfo> m_spErrorInfo;
        ContainedFailureReport* m_pPrevious;
    };

    inline HRESULT ContainRaiseFailure(HRESULT hr)
    {
        if (FAILED(hr) && t_pRaiseFailure && !IsUncontainedFailure(hr))
        {
            if (SUCCEEDED(*t_pRaiseFailure))
            {
                *t_pRaiseFailure = hr;
            }
            ContainedFailureReport::Capture(hr);
            return S_OK;
        }
        return hr;
    }

    // The first failure of hr and *pFirst.
    inline void KeepFirstFailure(_Inout_ HRESULT* pFirst, HRESULT hr)
    {
        if (SUCCEEDED(*pFirst) && FAILED(hr))
        {
            *pFirst = hr;
            ContainedFailureReport::Capture(hr);
        }
    }

    // App objects (comparers, converters, group descriptions, group names) the engine keeps between changes. They
    // are held through tracked references the view walks, so a delegate that captures its page does not root it.
    // The engine uses them through raw pointers that these keep valid.
    class AppReferences
    {
    public:
        template <typename T>
        T* Keep(_In_opt_ T* pValue)
        {
            if (pValue)
            {
                m_references.Append(ctl::iunknown_cast(pValue));
            }
            return pValue;
        }

        void ReferenceTrackerWalk(EReferenceTrackerWalkType walkType)
        {
            m_references.ReferenceTrackerWalk(walkType);
        }

    private:
        TrackerPtrVector<IUnknown> m_references;
    };

    // Nonzero while the engine runs app code or raises its changes (VectorCollectionView::m_shapeBusyDepth).
    class BusyScope
    {
    public:
        explicit BusyScope(UINT& depth) : m_depth(depth)
        {
            ++m_depth;
        }

        ~BusyScope()
        {
            --m_depth;
        }

        BusyScope(const BusyScope&) = delete;
        BusyScope& operator=(const BusyScope&) = delete;

    private:
        UINT& m_depth;
    };

    // The storage of the presented lists (a TrackerCollection storage, see TrackerPtrVector). A TrackerPtrVector moves
    // every later TrackerPtr on an insert or a remove, and each move takes the reference lock (about 14 ns a row, so
    // 1.4 ms a change at 100k rows). Here an item keeps the slot it was put in until it leaves, and the order is a
    // vector of slot numbers: an insert or a remove moves 4-byte integers and no TrackerPtr. The reference tracker
    // walk walks the slots; a free slot is empty.
    template <typename T>
    class StableTrackerPtrVector
    {
    public:
        class ConstIterator
        {
        public:
            ConstIterator(typename std::vector<TrackerPtr<T>>::const_iterator slots, UINT slotCount, std::vector<UINT>::const_iterator position)
                : m_slots(slots), m_slotCount(slotCount), m_position(position)
            {
            }

            const TrackerPtr<T>& operator*() const
            {
                // The order only names allocated slots (RemoveAt frees a slot before the item's release can run code).
                FAIL_FAST_ASSERT(*m_position < m_slotCount);
                return *(m_slots + *m_position);
            }

            ConstIterator& operator++()
            {
                ++m_position;
                return *this;
            }

            ConstIterator operator++(int)
            {
                ConstIterator previous(*this);
                ++m_position;
                return previous;
            }

            bool operator==(const ConstIterator& other) const
            {
                return m_position == other.m_position;
            }

            bool operator!=(const ConstIterator& other) const
            {
                return m_position != other.m_position;
            }

        private:
            typename std::vector<TrackerPtr<T>>::const_iterator m_slots;
            UINT m_slotCount = 0;
            std::vector<UINT>::const_iterator m_position;
        };

        // TrackerPtrVector only asserts its index, so every slot is checked here before it is used.
        _Check_return_ HRESULT GetAt(_In_ UINT index, _Outptr_ T** item)
        {
            UINT slot = 0;
            IFC_RETURN(SlotAt(index, &slot));
            IFC_RETURN(m_slots.GetAt(slot, item));
            return S_OK;
        }

        _Check_return_ HRESULT GetAsReferenceTrackerUnsafe(_In_ UINT index, _Outptr_ xaml_hosting::IReferenceTrackerInternal** itemNoRef)
        {
            UINT slot = 0;
            IFC_RETURN(SlotAt(index, &slot));
            IFC_RETURN(m_slots.GetAsReferenceTrackerUnsafe(slot, itemNoRef));
            return S_OK;
        }

        _Check_return_ HRESULT SetAt(_In_ UINT index, _In_opt_ T* item)
        {
            UINT slot = 0;
            IFC_RETURN(SlotAt(index, &slot));
            // The replaced item is released last, once the list is consistent (see RemoveAt).
            ctl::ComPtr<T> spReleasedLast;
            IFC_RETURN(m_slots.GetAt(slot, spReleasedLast.ReleaseAndGetAddressOf()));
            IFC_RETURN(m_slots.SetAt(slot, item));
            return S_OK;
        }

        _Check_return_ HRESULT InsertAt(_In_ UINT index, _In_opt_ T* item)
        {
            IFCEXPECTRC_RETURN(index <= m_order.size(), E_BOUNDS);
            UINT slot = 0;
            IFC_RETURN(AllocateSlot(item, &slot));
            m_order.insert(m_order.begin() + index, slot);
            return S_OK;
        }

        // TrackerCollection::Append returns nothing; AllocateSlot cannot fail once the slot is checked, so a failure
        // here is a broken invariant, not an error to recover from.
        void Append(_In_opt_ T* item)
        {
            UINT slot = 0;
            IFCFAILFAST(AllocateSlot(item, &slot));
            m_order.push_back(slot);
        }

        // The item leaves the order and its slot is freed before the item is released, as TrackerPtrVector::RemoveAt
        // releases it after the erase: the release may run code that reads or changes this list (even Clear it), and
        // the list is consistent by then. The release is the last thing this does.
        _Check_return_ HRESULT RemoveAt(_In_ UINT index)
        {
            UINT slot = 0;
            IFC_RETURN(SlotAt(index, &slot));
            ctl::ComPtr<T> spReleasedLast;
            IFC_RETURN(m_slots.GetAt(slot, spReleasedLast.ReleaseAndGetAddressOf()));
            m_order.erase(m_order.begin() + index);
            m_freeSlots.push_back(slot);
            IFC_RETURN(m_slots.SetAt(slot, nullptr));
            return S_OK;
        }

        void Clear()
        {
            m_order.clear();
            m_freeSlots.clear();
            m_slots.Clear();
        }

        void Reserve(_In_ UINT capacity)
        {
            m_slots.Reserve(capacity);
            m_order.reserve(capacity);
        }

        ConstIterator Begin() const
        {
            return ConstIterator(m_slots.Begin(), m_slots.Size(), m_order.cbegin());
        }

        ConstIterator End() const
        {
            return ConstIterator(m_slots.Begin(), m_slots.Size(), m_order.cend());
        }

        UINT Size() const
        {
            return static_cast<UINT>(m_order.size());
        }

        bool Empty() const
        {
            return m_order.empty();
        }

        void ReferenceTrackerWalk(_In_ EReferenceTrackerWalkType walkType)
        {
            m_slots.ReferenceTrackerWalk(walkType);
        }

    private:
        _Check_return_ HRESULT SlotAt(_In_ UINT index, _Out_ UINT* pSlot) const
        {
            *pSlot = 0;
            IFCCHECK_RETURN(index < m_order.size());
            IFCCHECK_RETURN(m_order[index] < m_slots.Size());
            *pSlot = m_order[index];
            return S_OK;
        }

        // A free slot past the end (none should be) is dropped rather than used.
        _Check_return_ HRESULT AllocateSlot(_In_opt_ T* item, _Out_ UINT* pSlot)
        {
            *pSlot = 0;
            while (!m_freeSlots.empty() && m_freeSlots.back() >= m_slots.Size())
            {
                ASSERT(false);
                m_freeSlots.pop_back();
            }

            if (m_freeSlots.empty())
            {
                m_slots.Append(item);
                *pSlot = m_slots.Size() - 1;
                return S_OK;
            }

            const UINT slot = m_freeSlots.back();
            IFC_RETURN(m_slots.SetAt(slot, item));
            m_freeSlots.pop_back();
            *pSlot = slot;
            return S_OK;
        }

        // Grows only by Append, so an item's TrackerPtr moves only when the vector reallocates (amortized O(1)).
        TrackerPtrVector<T> m_slots;
        // The slot of each item, in list order.
        std::vector<UINT> m_order;
        std::vector<UINT> m_freeSlots;
    };

    using PresentedListStorage = StableTrackerPtrVector<IInspectable>;

    // The shaped list the view wraps (WPF: ListCollectionView.InternalList). Read-only; a recompute
    // replaces its contents wholesale and announces a single Reset.
    class __declspec(novtable) InternalList : public ReadOnlyObservableTrackerCollection<IInspectable*, PresentedListStorage>
    {
    public:
        // The collection without the change notifications, for the engine's unraised edits.
        using StorageCollection = TrackerCollection<IInspectable*, PresentedListStorage>;

        _Check_return_ HRESULT ReplaceAll(_In_ const std::vector<ctl::ComPtr<IInspectable>>& items)
        {
            IFC_RETURN(StorageCollection::Clear());
            Reserve(static_cast<uint32_t>(items.size()));
            for (const auto& item : items)
            {
                IFC_RETURN(StorageCollection::Append(item.Get()));
            }

            IFC_RETURN(RaiseVectorChanged(wfc::CollectionChange_Reset, 0));
            return S_OK;
        }

    protected:
        InternalList() = default;

        _Check_return_ HRESULT RaiseVectorChanged(_In_ wfc::CollectionChange action, UINT index) override
        {
            return ContainRaiseFailure(ReadOnlyObservableTrackerCollection<IInspectable*, PresentedListStorage>::RaiseVectorChanged(action, index));
        }
    };

    // A sort key decoded once from a property value (WPF: SortFieldComparer's cached values), so a compare
    // queries no interface and copies no string. Strings compare in the view's culture (WPF: CollectionView.Culture),
    // else the user's; integral values compare exactly (a double holds only 53 bits). WPF's Comparer.Default throws
    // on values of different types; here the order stays total: null first, then by kind (Boolean, Number,
    // DateTime, TimeSpan, String, then values that cannot be ordered, which compare equal and so keep their source
    // order). NaN sorts before every other number, as Double.CompareTo does.
    struct SortKey
    {
        enum class Kind : UINT8 { Null, Unordered, String, Number, Boolean, DateTime, TimeSpan };

        Kind kind = Kind::Null;
        bool isInteger = false;
        bool isNegative = false;
        UINT64 magnitude = 0;
        DOUBLE number = 0;
        INT64 ticks = 0;
        std::string sortKey;
    };

    using SortKeys = std::vector<SortKey>;

    // A group name normalized so names that match (PropertyGroupDescription.NamesMatch) have equal keys. A custom
    // GroupDescription's names have none: its NamesMatch decides.
    struct GroupNameKey
    {
        UINT8 kind = 0;
        std::string bytes;

        bool operator==(const GroupNameKey& other) const
        {
            return kind == other.kind && bytes == other.bytes;
        }
    };

    struct GroupNameKeyHash
    {
        size_t operator()(const GroupNameKey& key) const
        {
            return std::hash<std::string>()(key.bytes) * 31 + key.kind;
        }
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
        public xaml_data::ICollectionViewGroup2,
        public xaml_data::INotifyPropertyChanged
    {
        BEGIN_INTERFACE_MAP(CollectionViewGroupInternal, InternalList)
            INTERFACE_ENTRY(CollectionViewGroupInternal, xaml_data::ICustomPropertyProvider)
            INTERFACE_ENTRY(CollectionViewGroupInternal, xaml_data::ICollectionViewGroup)
            INTERFACE_ENTRY(CollectionViewGroupInternal, xaml_data::ICollectionViewGroup2)
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

        // WPF: CollectionViewGroupInternal.IsExplicit, a group of GroupDescription.GroupNames. It is created with its
        // parent, stays when it empties, and the explicit groups come first, in GroupNames order.
        bool IsExplicit() const
        {
            return m_isExplicit;
        }

        void SetIsExplicit()
        {
            m_isExplicit = true;
        }

        UINT ExplicitSubgroupCount() const
        {
            return m_explicitSubgroupCount;
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

        // Bottom level: each item's sort key slot (SortKeyStore), parallel to SourceIndices.
        std::vector<UINT>& KeySlots()
        {
            return m_keySlots;
        }

        // The keys of the group's own SortDescriptions order, read when it was created.
        SortKeys& GroupSortKeys()
        {
            return m_groupSortKeys;
        }

        void SetNameKey(GroupNameKey&& key, bool isHashed)
        {
            m_nameKey = std::move(key);
            m_isNameHashed = isHashed;
        }

        // The subgroup with this name key (WPF: CollectionViewGroupInternal.GetSubgroupFromMap), or null.
        CollectionViewGroupInternal* FindSubgroupByKey(const GroupNameKey& key) const
        {
            auto found = m_subgroupsByName.find(key);
            return (found != m_subgroupsByName.end()) ? found->second : nullptr;
        }

        // The subgroups whose names have no key, found by NamesMatch.
        const std::vector<CollectionViewGroupInternal*>& UnhashedSubgroups() const
        {
            return m_unhashedSubgroups;
        }

        // The bottom-level group that holds this group's first leaf, or null. Empty (explicit) groups are passed over.
        CollectionViewGroupInternal* FirstBottomGroup()
        {
            CollectionViewGroupInternal* pGroup = this;
            while (!pGroup->IsBottomLevel())
            {
                CollectionViewGroupInternal* pNext = nullptr;
                for (const auto& spSubgroup : pGroup->m_subgroups)
                {
                    if (spSubgroup->LeafCount() > 0)
                    {
                        pNext = spSubgroup.Get();
                        break;
                    }
                }
                if (!pNext)
                {
                    return nullptr;
                }
                pGroup = pNext;
            }
            return pGroup->m_sourceIndices.empty() ? nullptr : pGroup;
        }

        // Loading: after the bottom-level groups have their items, each group above them gets its leaves once.
        _Check_return_ HRESULT CollectLeaves()
        {
            if (IsBottomLevel())
            {
                return S_OK;
            }

            UINT count = 0;
            for (const auto& spSubgroup : m_subgroups)
            {
                IFC_RETURN(spSubgroup->CollectLeaves());
                count += spSubgroup->LeafCount();
            }

            if (IsRoot())
            {
                return S_OK;
            }

            IFC_RETURN(StorageCollection::Clear());
            Reserve(count);
            for (const auto& spSubgroup : m_subgroups)
            {
                const UINT size = spSubgroup->LeafCount();
                for (UINT i = 0; i < size; ++i)
                {
                    ctl::ComPtr<IInspectable> spItem;
                    IFC_RETURN(spSubgroup->GetAt(i, spItem.ReleaseAndGetAddressOf()));
                    IFC_RETURN(StorageCollection::Append(spItem.Get()));
                }
            }
            return S_OK;
        }

        _Check_return_ HRESULT InsertSubgroup(UINT index, _In_ CollectionViewGroupInternal* pSubgroup, bool isRaised)
        {
            // A custom GroupDescription's NamesMatch may tell apart names with one key: the first keeps the entry.
            if (pSubgroup->m_isNameHashed && m_subgroupsByName.emplace(pSubgroup->m_nameKey, pSubgroup).second)
            {
                pSubgroup->m_isNameMapped = true;
            }
            else
            {
                m_unhashedSubgroups.push_back(pSubgroup);
            }
            if (pSubgroup->m_isExplicit)
            {
                ++m_explicitSubgroupCount;
            }
            m_subgroups.insert(m_subgroups.begin() + index, ctl::ComPtr<CollectionViewGroupInternal>(pSubgroup));
            auto pSubgroupItem = ctl::as_iinspectable(pSubgroup);
            if (isRaised)
            {
                IFC_RETURN(m_tpSubgroupList->InternalInsertAt(index, pSubgroupItem));
            }
            else
            {
                IFC_RETURN(m_tpSubgroupList->StorageCollection::InsertAt(index, pSubgroupItem));
            }
            return S_OK;
        }

        _Check_return_ HRESULT RemoveSubgroup(UINT index)
        {
            auto pSubgroup = m_subgroups[index].Get();
            if (pSubgroup->m_isNameMapped)
            {
                m_subgroupsByName.erase(pSubgroup->m_nameKey);
                pSubgroup->m_isNameMapped = false;
            }
            else
            {
                m_unhashedSubgroups.erase(std::remove(m_unhashedSubgroups.begin(), m_unhashedSubgroups.end(), pSubgroup), m_unhashedSubgroups.end());
            }
            if (pSubgroup->m_isExplicit)
            {
                --m_explicitSubgroupCount;
            }
            pSubgroup->ClearParent();
            m_subgroups.erase(m_subgroups.begin() + index);
            IFC_RETURN(m_tpSubgroupList->InternalRemoveAt(index));
            return S_OK;
        }

        // Loading: the subgroups in a new order, nothing raised.
        _Check_return_ HRESULT ReorderSubgroups(std::vector<ctl::ComPtr<CollectionViewGroupInternal>>&& subgroups)
        {
            m_subgroups = std::move(subgroups);
            IFC_RETURN(m_tpSubgroupList->StorageCollection::Clear());
            for (const auto& spSubgroup : m_subgroups)
            {
                IFC_RETURN(m_tpSubgroupList->StorageCollection::Append(ctl::as_iinspectable(spSubgroup.Get())));
            }
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
                IFC_RETURN(StorageCollection::InsertAt(index, pItem));
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

        // ICollectionViewGroup2 (WPF: CollectionViewGroup.ItemCount and IsBottomLevel; its Name and Items are
        // ICollectionViewGroup.Group and GroupItems)
        IFACEMETHODIMP get_ItemCount(_Out_ INT32* pValue) override
        {
            ARG_VALIDRETURNPOINTER(pValue);
            *pValue = static_cast<INT32>(LeafCount());
            return S_OK;
        }

        IFACEMETHODIMP get_IsBottomLevel(_Out_ BOOLEAN* pValue) override
        {
            ARG_VALIDRETURNPOINTER(pValue);
            *pValue = IsBottomLevel();
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
            else if (InlineIsEqualGUID(iid, __uuidof(xaml_data::ICollectionViewGroup2)))
            {
                *ppObject = static_cast<xaml_data::ICollectionViewGroup2*>(this);
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
            IFC_RETURN(ContainRaiseFailure(m_propertyChangedHandlers.Raise(ctl::as_iinspectable(this), spArgs.Get())));
            return S_OK;
        }

        // A key can be an app object (a Converter result), so it is walked.
        TrackerPtr<IInspectable> m_tpKey;
        TrackerEventSource<xaml_data::IPropertyChangedEventHandler, IInspectable, xaml_data::IPropertyChangedEventArgs> m_propertyChangedHandlers;
        CollectionViewGroupInternal* m_pParent = nullptr;
        std::vector<UINT> m_sourceIndices;
        std::vector<UINT> m_keySlots;
        SortKeys m_groupSortKeys;
        GroupNameKey m_nameKey;
        bool m_isNameHashed = false;
        bool m_isNameMapped = false;
        bool m_isExplicit = false;
        UINT m_explicitSubgroupCount = 0;
        std::unordered_map<GroupNameKey, CollectionViewGroupInternal*, GroupNameKeyHash> m_subgroupsByName;
        std::vector<CollectionViewGroupInternal*> m_unhashedSubgroups;
    };

    struct ShapingKeySelector
    {
        ctl::ComPtr<PropertyPathListener> listener; // Null means "the item itself", as in WPF.
        bool isDescending = false;
    };

    inline _Check_return_ HRESULT CreateKeySelector(_In_opt_ HSTRING path, _Out_ ShapingKeySelector* pSelector)
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

    inline _Check_return_ HRESULT EvaluateKey(_In_ const ShapingKeySelector& selector, _In_opt_ IInspectable* pItem, _Outptr_result_maybenull_ IInspectable** ppKey)
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

    inline _Check_return_ HRESULT GetNumericValue(_In_ wf::IPropertyValue* pValue, wf::PropertyType type, _Out_ DOUBLE* pResult)
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
    inline bool TryGetIntegerValue(_In_ wf::IPropertyValue* pValue, wf::PropertyType type, _Out_ bool* pIsNegative, _Out_ UINT64* pMagnitude)
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

    // LCMapStringEx rejects an empty string; this is the key of a string of ignorable characters, which
    // CompareStringEx finds equal to it.
    constexpr char c_emptySortKey[] = { 1, 1, 1, 1, 0 };

    // The LCMAP_SORTKEY bytes of a string: memcmp of two keys orders as CompareStringEx with the same locale
    // and flags.
    inline bool TryMakeStringSortKey(_In_opt_z_ LPCWSTR localeName, DWORD flags, _In_reads_(length) LPCWSTR szValue, UINT32 length, _Out_ std::string* pKey)
    {
        if (length == 0)
        {
            pKey->assign(c_emptySortKey, sizeof(c_emptySortKey));
            return true;
        }

        const int size = LCMapStringEx(localeName, LCMAP_SORTKEY | flags, szValue, static_cast<int>(length), nullptr, 0, nullptr, nullptr, 0);
        if (size <= 0)
        {
            return false;
        }

        pKey->resize(static_cast<size_t>(size));
        return LCMapStringEx(localeName, LCMAP_SORTKEY | flags, szValue, static_cast<int>(length), reinterpret_cast<LPWSTR>(pKey->data()), size, nullptr, nullptr, 0) == size;
    }

    inline _Check_return_ HRESULT MakeSortKey(_In_opt_ IInspectable* pValue, _In_z_ LPCWSTR localeName, _Out_ SortKey* pKey)
    {
        *pKey = SortKey();
        if (!pValue)
        {
            return S_OK;
        }

        pKey->kind = SortKey::Kind::Unordered;
        auto spValue = ctl::query_interface_cast<wf::IPropertyValue>(pValue);
        wf::PropertyType type = wf::PropertyType_Empty;
        if (!spValue || FAILED(spValue->get_Type(&type)))
        {
            return S_OK;
        }

        if (type == wf::PropertyType_String)
        {
            wrl_wrappers::HString value;
            UINT32 length = 0;
            if (SUCCEEDED(spValue->GetString(value.GetAddressOf())))
            {
                LPCWSTR szValue = value.GetRawBuffer(&length);
                if (TryMakeStringSortKey(localeName, 0, szValue, length, &pKey->sortKey))
                {
                    pKey->kind = SortKey::Kind::String;
                }
            }
        }
        else if (DirectUI::PropertyValue::IsNumericPropertyType(type))
        {
            if (TryGetIntegerValue(spValue.Get(), type, &pKey->isNegative, &pKey->magnitude))
            {
                pKey->isInteger = true;
                pKey->number = pKey->isNegative ? -static_cast<DOUBLE>(pKey->magnitude) : static_cast<DOUBLE>(pKey->magnitude);
                pKey->kind = SortKey::Kind::Number;
            }
            else if (SUCCEEDED(GetNumericValue(spValue.Get(), type, &pKey->number)))
            {
                pKey->kind = SortKey::Kind::Number;
            }
        }
        else if (type == wf::PropertyType_Boolean)
        {
            BOOLEAN value = FALSE;
            if (SUCCEEDED(spValue->GetBoolean(&value)))
            {
                pKey->ticks = !!value;
                pKey->kind = SortKey::Kind::Boolean;
            }
        }
        else if (type == wf::PropertyType_DateTime)
        {
            wf::DateTime value = {};
            if (SUCCEEDED(spValue->GetDateTime(&value)))
            {
                pKey->ticks = value.UniversalTime;
                pKey->kind = SortKey::Kind::DateTime;
            }
        }
        else if (type == wf::PropertyType_TimeSpan)
        {
            wf::TimeSpan value = {};
            if (SUCCEEDED(spValue->GetTimeSpan(&value)))
            {
                pKey->ticks = value.Duration;
                pKey->kind = SortKey::Kind::TimeSpan;
            }
        }

        return S_OK;
    }

    inline int SortKeyRank(SortKey::Kind kind)
    {
        switch (kind)
        {
        case SortKey::Kind::Null: return 0;
        case SortKey::Kind::Boolean: return 1;
        case SortKey::Kind::Number: return 2;
        case SortKey::Kind::DateTime: return 3;
        case SortKey::Kind::TimeSpan: return 4;
        case SortKey::Kind::String: return 5;
        default: return 6;
        }
    }

    // An integer (sign and magnitude) against a double that is not NaN, exactly: through a double, integers above 2^53
    // would compare equal to doubles they differ from, and the order would not be total.
    inline int CompareIntegerWithDouble(bool isNegative, UINT64 magnitude, DOUBLE value)
    {
        constexpr DOUBLE c_twoTo64 = 18446744073709551616.0;
        const bool isZero = magnitude == 0;
        if (value >= c_twoTo64)
        {
            return -1;
        }
        if (value <= -c_twoTo64)
        {
            return 1;
        }

        const DOUBLE truncated = std::trunc(value);
        const bool isValueNegative = truncated < 0;
        const UINT64 valueMagnitude = static_cast<UINT64>(isValueNegative ? -truncated : truncated);
        const bool isIntegerNegative = isNegative && !isZero;
        const bool isTruncatedNegative = isValueNegative && valueMagnitude != 0;
        int order = 0;
        if (isIntegerNegative != isTruncatedNegative)
        {
            order = isIntegerNegative ? -1 : 1;
        }
        else if (magnitude != valueMagnitude)
        {
            order = (magnitude < valueMagnitude) ? -1 : 1;
            order = isIntegerNegative ? -order : order;
        }
        if (order != 0)
        {
            return order;
        }

        const DOUBLE fraction = value - truncated;
        return (fraction > 0) ? -1 : ((fraction < 0) ? 1 : 0);
    }

    inline int CompareSortKeys(const SortKey& left, const SortKey& right)
    {
        if (left.kind != right.kind)
        {
            const int leftRank = SortKeyRank(left.kind);
            const int rightRank = SortKeyRank(right.kind);
            return (leftRank > rightRank) - (leftRank < rightRank);
        }

        if (left.kind == SortKey::Kind::Null || left.kind == SortKey::Kind::Unordered)
        {
            return 0;
        }

        switch (left.kind)
        {
        case SortKey::Kind::String:
        {
            const int result = left.sortKey.compare(right.sortKey);
            return (result > 0) - (result < 0);
        }

        case SortKey::Kind::Number:
            if (left.isInteger && right.isInteger)
            {
                if (left.isNegative != right.isNegative)
                {
                    return left.isNegative ? -1 : 1;
                }
                const int magnitudeOrder = (left.magnitude < right.magnitude) ? -1 : ((left.magnitude > right.magnitude) ? 1 : 0);
                return left.isNegative ? -magnitudeOrder : magnitudeOrder;
            }
            if (left.number != left.number || right.number != right.number)
            {
                const bool isLeftNaN = left.number != left.number;
                const bool isRightNaN = right.number != right.number;
                return (isLeftNaN ? 0 : 1) - (isRightNaN ? 0 : 1);
            }
            if (left.isInteger)
            {
                return CompareIntegerWithDouble(left.isNegative, left.magnitude, right.number);
            }
            if (right.isInteger)
            {
                return -CompareIntegerWithDouble(right.isNegative, right.magnitude, left.number);
            }
            return (left.number < right.number) ? -1 : ((left.number > right.number) ? 1 : 0);

        default:
            return (left.ticks < right.ticks) ? -1 : ((left.ticks > right.ticks) ? 1 : 0);
        }
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
            _In_opt_ HSTRING culture,
            _Inout_ AppReferences& references)
        {
            UINT count = 0;
            UINT32 cultureLength = 0;
            LPCWSTR szCulture = WindowsGetStringRawBuffer(culture, &cultureLength);

            m_fields.clear();
            m_pCustomSort = references.Keep(pCustomSort);
            m_localeName = (cultureLength > 0) ? std::wstring(szCulture, cultureLength) : std::wstring();
            if (m_pCustomSort || !pDescriptions)
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
                xaml_data::CollectionViewSortDirection direction = xaml_data::CollectionViewSortDirection_Ascending;
                ShapingKeySelector field;
                IFC_RETURN(spDescription->get_PropertyName(path.GetAddressOf()));
                IFC_RETURN(spDescription->get_Direction(&direction));
                IFC_RETURN(CreateKeySelector(path.Get(), &field));
                field.isDescending = (direction == xaml_data::CollectionViewSortDirection_Descending);
                m_fields.push_back(std::move(field));
            }

            return S_OK;
        }

        bool IsEmpty() const
        {
            return !m_pCustomSort && m_fields.empty();
        }

        bool HasCustomSort() const
        {
            return !!m_pCustomSort;
        }

        // The keys Compare reads per item; none with a CustomSort, which compares the items.
        UINT FieldCount() const
        {
            return m_pCustomSort ? 0 : static_cast<UINT>(m_fields.size());
        }

        _Check_return_ HRESULT EvaluateKeys(_In_opt_ IInspectable* pItem, _Out_ SortKeys* pKeys) const
        {
            LPCWSTR localeName = m_localeName.empty() ? LOCALE_NAME_USER_DEFAULT : m_localeName.c_str();
            pKeys->clear();
            pKeys->resize(FieldCount());
            for (UINT i = 0; i < FieldCount(); ++i)
            {
                ctl::ComPtr<IInspectable> spValue;
                IFC_RETURN(EvaluateKey(m_fields[i], pItem, spValue.ReleaseAndGetAddressOf()));
                IFC_RETURN(MakeSortKey(spValue.Get(), localeName, &(*pKeys)[i]));
            }

            return S_OK;
        }

        // The items are read only by a CustomSort; the keys (FieldCount each) only by the SortDescriptions.
        _Check_return_ HRESULT Compare(
            _In_opt_ IInspectable* pLeft,
            _In_opt_ const SortKey* pLeftKeys,
            UINT leftSourceIndex,
            _In_opt_ IInspectable* pRight,
            _In_opt_ const SortKey* pRightKeys,
            UINT rightSourceIndex,
            _Out_ int* pResult) const
        {
            *pResult = 0;

            if (m_pCustomSort)
            {
                INT32 result = 0;
                IFC_RETURN(m_pCustomSort->Invoke(pLeft, pRight, &result));
                *pResult = (result > 0) - (result < 0);
            }
            else
            {
                for (size_t i = 0; i < m_fields.size() && *pResult == 0; ++i)
                {
                    const int result = CompareSortKeys(pLeftKeys[i], pRightKeys[i]);
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
        xaml_data::ICollectionViewComparer* m_pCustomSort = nullptr;
        std::wstring m_localeName;
    };

    // A stable merge sort for orders that call app code (a CustomSort, a group's CustomSort). std::stable_sort assumes
    // a strict weak ordering and can step outside the range when the comparer is inconsistent (random results, items
    // that change while it runs); this sort reads and writes only inside [0, size) whatever the comparer returns, so
    // such a comparer gives some permutation of the items, never a corrupted range. isBefore(left, right, &result)
    // can fail: the first failure ends the sort. A final pass checks each pair of neighbours and reports an order it
    // finds inconsistent as E_INVALIDARG (WPF: InvalidOperationException from Array.Sort). On failure the items are
    // unchanged. Already-ordered runs are joined with one comparison, so a sorted input costs about 2n comparisons.
    template <typename T, typename IsBefore>
    _Check_return_ HRESULT SafeStableSort(_Inout_ std::vector<T>& items, IsBefore&& isBefore)
    {
        const size_t count = items.size();
        if (count < 2)
        {
            return S_OK;
        }

        std::vector<size_t> order(count);
        std::vector<size_t> merged(count);
        for (size_t i = 0; i < count; ++i)
        {
            order[i] = i;
        }

        for (size_t width = 1; width < count; width = (width <= count / 2) ? width * 2 : count)
        {
            for (size_t begin = 0; begin < count; begin += std::min(2 * width, count - begin))
            {
                const size_t middle = begin + std::min(width, count - begin);
                const size_t end = middle + std::min(width, count - middle);
                size_t left = begin;
                size_t right = middle;
                size_t out = begin;
                if (right < end)
                {
                    bool isOutOfOrder = false;
                    IFC_RETURN(isBefore(items[order[right]], items[order[right - 1]], &isOutOfOrder));
                    if (!isOutOfOrder)
                    {
                        left = middle;
                        right = end;
                        std::copy(order.begin() + begin, order.begin() + end, merged.begin() + begin);
                        out = end;
                    }
                }
                while (left < middle && right < end)
                {
                    bool isRightFirst = false;
                    IFC_RETURN(isBefore(items[order[right]], items[order[left]], &isRightFirst));
                    merged[out++] = isRightFirst ? order[right++] : order[left++];
                }
                while (left < middle)
                {
                    merged[out++] = order[left++];
                }
                while (right < end)
                {
                    merged[out++] = order[right++];
                }
            }
            std::swap(order, merged);
        }

        for (size_t i = 1; i < count; ++i)
        {
            bool isOutOfOrder = false;
            IFC_RETURN(isBefore(items[order[i]], items[order[i - 1]], &isOutOfOrder));
            if (isOutOfOrder)
            {
                static constexpr WCHAR c_message[] = L"The comparer returned inconsistent results: it does not define an order, or the items changed while the view was sorted.";
                IFC_RETURN(ErrorHelper::OriginateError(E_INVALIDARG, ARRAYSIZE(c_message) - 1, c_message));
            }
        }

        std::vector<T> sorted;
        sorted.reserve(count);
        for (const size_t index : order)
        {
            sorted.push_back(std::move(items[index]));
        }
        items = std::move(sorted);
        return S_OK;
    }

    // The decoded sort keys of the presented items, FieldCount per slot. An item keeps its slot while it moves,
    // so the incremental and live paths compare against cached keys and only the slot numbers move.
    class SortKeyStore
    {
    public:
        static constexpr UINT c_noSlot = UINT_MAX;

        void Reset(UINT fieldCount)
        {
            m_fieldCount = fieldCount;
            m_keys.clear();
            m_freeSlots.clear();
        }

        UINT FieldCount() const
        {
            return m_fieldCount;
        }

        UINT Allocate(const SortKeys& keys)
        {
            const UINT slot = AllocateSlot();
            Set(slot, keys);
            return slot;
        }

        UINT Allocate(SortKeys&& keys)
        {
            const UINT slot = AllocateSlot();
            for (UINT i = 0; i < m_fieldCount && i < keys.size(); ++i)
            {
                m_keys[static_cast<size_t>(slot) * m_fieldCount + i] = std::move(keys[i]);
            }
            return slot;
        }

        void Set(UINT slot, const SortKeys& keys)
        {
            for (UINT i = 0; i < m_fieldCount; ++i)
            {
                m_keys[static_cast<size_t>(slot) * m_fieldCount + i] = (i < keys.size()) ? keys[i] : SortKey();
            }
        }

        void Free(UINT slot)
        {
            if (m_fieldCount == 0 || slot == c_noSlot)
            {
                return;
            }

            for (UINT i = 0; i < m_fieldCount; ++i)
            {
                m_keys[static_cast<size_t>(slot) * m_fieldCount + i] = SortKey();
            }
            m_freeSlots.push_back(slot);
        }

        // Valid until the next Allocate.
        const SortKey* At(UINT slot) const
        {
            return (m_fieldCount == 0) ? nullptr : m_keys.data() + static_cast<size_t>(slot) * m_fieldCount;
        }

    private:
        UINT AllocateSlot()
        {
            if (m_fieldCount == 0)
            {
                return 0;
            }

            if (!m_freeSlots.empty())
            {
                const UINT slot = m_freeSlots.back();
                m_freeSlots.pop_back();
                return slot;
            }

            const UINT slot = static_cast<UINT>(m_keys.size() / m_fieldCount);
            m_keys.resize(m_keys.size() + m_fieldCount);
            return slot;
        }

        UINT m_fieldCount = 0;
        std::vector<SortKey> m_keys;
        std::vector<UINT> m_freeSlots;
    };

    inline _Check_return_ HRESULT TryGetString(_In_opt_ IInspectable* pValue, _Out_ wrl_wrappers::HString* pString, _Out_ bool* pIsString)
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
    inline _Check_return_ HRESULT GroupNamesMatch(xaml_data::CollectionViewStringComparison comparison, _In_opt_ IInspectable* pGroupName, _In_opt_ IInspectable* pItemName, _Out_ bool* pMatch)
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

    // The key under which a parent maps a group name (WPF: CollectionViewGroupInternal._nameToGroupMap), equal for
    // exactly the names GroupNamesMatch matches: a string as compared (ordinal, ordinal uppercase, or its sort
    // key), a scalar by its value (AreEqual), another object by identity. *pIsHashed is false for a name only
    // NamesMatch can match (a NaN, an enum box, an array).
    inline _Check_return_ HRESULT MakeGroupNameKey(xaml_data::CollectionViewStringComparison comparison, _In_opt_ IInspectable* pName, _Out_ GroupNameKey* pKey, _Out_ bool* pIsHashed)
    {
        constexpr UINT8 c_identityKind = 0xFF;
        auto setBytes = [pKey](const void* pData, size_t size)
        {
            if (size == 0)
            {
                pKey->bytes.clear();
                return;
            }
            pKey->bytes.assign(static_cast<const char*>(pData), size);
        };
        auto normalize = [](FLOAT value, bool* pIsNaN) { *pIsNaN = *pIsNaN || value != value; return (value == 0) ? 0.0f : value; };

        *pKey = GroupNameKey();
        *pIsHashed = true;
        if (!pName)
        {
            return S_OK;
        }

        auto spValue = ctl::query_interface_cast<wf::IPropertyValue>(pName);
        if (!spValue)
        {
            ctl::ComPtr<IUnknown> spIdentity;
            IFC_RETURN(pName->QueryInterface(IID_PPV_ARGS(&spIdentity)));
            IUnknown* pIdentity = spIdentity.Get();
            pKey->kind = c_identityKind;
            setBytes(&pIdentity, sizeof(pIdentity));
            return S_OK;
        }

        wf::PropertyType type = wf::PropertyType_Empty;
        IFC_RETURN(spValue->get_Type(&type));
        pKey->kind = static_cast<UINT8>(1 + type);
        bool isNaN = false;

        switch (type)
        {
        case wf::PropertyType_String:
        {
            wrl_wrappers::HString name;
            UINT32 length = 0;
            IFC_RETURN(spValue->GetString(name.GetAddressOf()));
            LPCWSTR szName = name.GetRawBuffer(&length);
            switch (comparison)
            {
            case xaml_data::CollectionViewStringComparison_Ordinal:
                setBytes(szName, length * sizeof(WCHAR));
                break;

            // CompareStringOrdinal ignoring case matches what the invariant uppercase mapping makes equal.
            case xaml_data::CollectionViewStringComparison_OrdinalIgnoreCase:
            {
                std::wstring upper(szName, length);
                if (length > 0 && LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_UPPERCASE, szName, static_cast<int>(length), upper.data(), static_cast<int>(length), nullptr, nullptr, 0) != static_cast<int>(length))
                {
                    *pIsHashed = false;
                }
                setBytes(upper.data(), length * sizeof(WCHAR));
                break;
            }

            default:
            {
                const bool isInvariant = comparison == xaml_data::CollectionViewStringComparison_InvariantCulture || comparison == xaml_data::CollectionViewStringComparison_InvariantCultureIgnoreCase;
                const bool isIgnoreCase = comparison == xaml_data::CollectionViewStringComparison_CurrentCultureIgnoreCase || comparison == xaml_data::CollectionViewStringComparison_InvariantCultureIgnoreCase;
                *pIsHashed = TryMakeStringSortKey(isInvariant ? LOCALE_NAME_INVARIANT : LOCALE_NAME_USER_DEFAULT, isIgnoreCase ? NORM_IGNORECASE : 0, szName, length, &pKey->bytes);
                break;
            }
            }
            break;
        }

        case wf::PropertyType_Boolean: { BOOLEAN v = FALSE; IFC_RETURN(spValue->GetBoolean(&v)); setBytes(&v, sizeof(v)); break; }
        case wf::PropertyType_Char16: { WCHAR v = 0; IFC_RETURN(spValue->GetChar16(&v)); setBytes(&v, sizeof(v)); break; }
        case wf::PropertyType_UInt8: { BYTE v = 0; IFC_RETURN(spValue->GetUInt8(&v)); setBytes(&v, sizeof(v)); break; }
        case wf::PropertyType_Int16: { INT16 v = 0; IFC_RETURN(spValue->GetInt16(&v)); setBytes(&v, sizeof(v)); break; }
        case wf::PropertyType_UInt16: { UINT16 v = 0; IFC_RETURN(spValue->GetUInt16(&v)); setBytes(&v, sizeof(v)); break; }
        case wf::PropertyType_Int32: { INT32 v = 0; IFC_RETURN(spValue->GetInt32(&v)); setBytes(&v, sizeof(v)); break; }
        case wf::PropertyType_UInt32: { UINT32 v = 0; IFC_RETURN(spValue->GetUInt32(&v)); setBytes(&v, sizeof(v)); break; }
        case wf::PropertyType_Int64: { INT64 v = 0; IFC_RETURN(spValue->GetInt64(&v)); setBytes(&v, sizeof(v)); break; }
        case wf::PropertyType_UInt64: { UINT64 v = 0; IFC_RETURN(spValue->GetUInt64(&v)); setBytes(&v, sizeof(v)); break; }
        case wf::PropertyType_Guid: { GUID v = {}; IFC_RETURN(spValue->GetGuid(&v)); setBytes(&v, sizeof(v)); break; }
        case wf::PropertyType_DateTime: { wf::DateTime v = {}; IFC_RETURN(spValue->GetDateTime(&v)); setBytes(&v.UniversalTime, sizeof(v.UniversalTime)); break; }
        case wf::PropertyType_TimeSpan: { wf::TimeSpan v = {}; IFC_RETURN(spValue->GetTimeSpan(&v)); setBytes(&v.Duration, sizeof(v.Duration)); break; }
        case wf::PropertyType_Single: { FLOAT v = 0; IFC_RETURN(spValue->GetSingle(&v)); v = normalize(v, &isNaN); setBytes(&v, sizeof(v)); break; }
        case wf::PropertyType_Double:
        {
            DOUBLE v = 0;
            IFC_RETURN(spValue->GetDouble(&v));
            isNaN = v != v;
            v = (v == 0) ? 0.0 : v;
            setBytes(&v, sizeof(v));
            break;
        }
        case wf::PropertyType_Point:
        {
            wf::Point v = {};
            IFC_RETURN(spValue->GetPoint(&v));
            const FLOAT values[] = { normalize(v.X, &isNaN), normalize(v.Y, &isNaN) };
            setBytes(values, sizeof(values));
            break;
        }
        case wf::PropertyType_Size:
        {
            wf::Size v = {};
            IFC_RETURN(spValue->GetSize(&v));
            const FLOAT values[] = { normalize(v.Width, &isNaN), normalize(v.Height, &isNaN) };
            setBytes(values, sizeof(values));
            break;
        }
        case wf::PropertyType_Rect:
        {
            wf::Rect v = {};
            IFC_RETURN(spValue->GetRect(&v));
            const FLOAT values[] = { normalize(v.X, &isNaN), normalize(v.Y, &isNaN), normalize(v.Width, &isNaN), normalize(v.Height, &isNaN) };
            setBytes(values, sizeof(values));
            break;
        }

        default:
            *pIsHashed = false;
            break;
        }

        *pIsHashed = *pIsHashed && !isNaN;
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

    inline _Check_return_ HRESULT AreGroupSortKeysFixed(_In_opt_ wfc::IVector<xaml_data::SortDescription*>* pDescriptions, _Out_ bool* pAreFixed)
    {
        UINT count = 0;
        *pAreFixed = true;
        if (pDescriptions)
        {
            IFC_RETURN(pDescriptions->get_Size(&count));
        }

        for (UINT i = 0; i < count && *pAreFixed; ++i)
        {
            ctl::ComPtr<xaml_data::ISortDescription> spDescription;
            wrl_wrappers::HString path;
            IFC_RETURN(pDescriptions->GetAt(i, spDescription.ReleaseAndGetAddressOf()));
            if (spDescription)
            {
                IFC_RETURN(spDescription->get_PropertyName(path.GetAddressOf()));
                *pAreFixed = wrl_wrappers::HStringReference(L"Key") == path.Get() || wrl_wrappers::HStringReference(L"Name") == path.Get();
            }
        }
        return S_OK;
    }

    // WPF: PropertyGroupDescription.GroupNameFromItem applies Converter.Convert(value, typeof(object), level, culture).
    inline _Check_return_ HRESULT ConvertGroupName(_In_opt_ xaml_data::IValueConverter* pConverter, INT level, _In_opt_ HSTRING culture, _Inout_ ctl::ComPtr<IInspectable>& spName)
    {
        if (!pConverter)
        {
            return S_OK;
        }

        wrl_wrappers::HStringReference objectTypeName(L"Object");
        wxaml_interop::TypeName objectType = { objectTypeName.Get(), wxaml_interop::TypeKind_Primitive };
        ctl::ComPtr<IInspectable> spLevel;
        ctl::ComPtr<IInspectable> spConverted;
        IFC_RETURN(DirectUI::PropertyValue::CreateFromInt32(level, spLevel.ReleaseAndGetAddressOf()));
        IFC_RETURN(pConverter->Convert(spName.Get(), objectType, spLevel.Get(), culture, spConverted.ReleaseAndGetAddressOf()));
        spName = std::move(spConverted);
        return S_OK;
    }

    // One level of grouping: the group name of an item (WPF: GroupDescription.GroupNameFromItem / NamesMatch). A
    // PropertyGroupDescription is read natively; any other GroupDescription is asked through its overridables.
    struct GroupLevel
    {
        bool hasDescription = false;
        ShapingKeySelector selector;
        xaml_data::IValueConverter* pConverter = nullptr;
        xaml_data::CollectionViewStringComparison stringComparison = xaml_data::CollectionViewStringComparison_Ordinal;
        GroupDescription* pCustomDescription = nullptr;
        // WPF: GroupDescription.GroupNames, the explicit groups of this level.
        std::vector<IInspectable*> explicitNames;
        // Group order (WPF: GroupDescription.SortDescriptions / CustomSort); else first appearance.
        bool hasGroupOrder = false;
        // The group SortDescriptions read only Key and Name, which a group keeps for its life, so each group
        // reads its sort keys once and siblings are found by binary search. Else (ItemCount) they are read
        // at each compare.
        bool areGroupSortKeysFixed = false;
        SortFieldComparer groupSort;
        xaml_data::ICollectionViewComparer* pGroupCustomSort = nullptr;
    };
}

using namespace DirectUI::ShapingEngine;


// The comparer the view sorts with and the decoded keys of the items it presents (flat output: one slot per
// output position, parallel to m_sourceIndices; grouped output: per bottom-level group).
struct DirectUI::ShapingSortState
{
    AppReferences references;
    SortFieldComparer comparer;
    SortKeyStore keys;
    std::vector<UINT> flatSlots;
};

namespace DirectUI::ShapingEngine
{
    using SortState = DirectUI::ShapingSortState;

    // The top of the group tree (WPF: MS.Internal.Data.CollectionViewGroupRoot). Its subgroups are the view's
    // InternalList; its own leaves are not kept.
    class __declspec(novtable) CollectionViewGroupRoot : public CollectionViewGroupInternal
    {
    public:
        _Check_return_ HRESULT InitializeRoot(_In_ wfc::IVector<xaml_data::GroupDescription*>* pDescriptions, _In_ VectorCollectionView* pOwner, _In_opt_ HSTRING culture, UINT sourceSize)
        {
            UINT count = 0;

            IFC_RETURN(InitializeGroup(nullptr, nullptr, false /* isBottomLevel */));
            IFC_RETURN(m_culture.Set(culture));
            m_leafGroups.assign(sourceSize, nullptr);
            IFC_RETURN(pDescriptions->get_Size(&count));
            m_levels.clear();
            m_levels.resize(count);
            for (UINT i = 0; i < count; ++i)
            {
                ctl::ComPtr<xaml_data::IGroupDescription> spDescription;
                auto& level = m_levels[i];
                IFC_RETURN(pDescriptions->GetAt(i, spDescription.ReleaseAndGetAddressOf()));
                if (!spDescription)
                {
                    continue;
                }

                auto spBase = ctl::query_interface_cast<GroupDescription>(spDescription.Get());
                if (!spBase)
                {
                    continue;
                }

                auto pBase = spBase.Get();
                auto spProperty = ctl::query_interface_cast<PropertyGroupDescription>(spDescription.Get());
                if (spProperty)
                {
                    wrl_wrappers::HString path;
                    IFC_RETURN(spProperty->get_PropertyName(path.GetAddressOf()));
                    IFC_RETURN(CreateKeySelector(path.Get(), &level.selector));
                    ctl::ComPtr<xaml_data::IValueConverter> spConverter;
                    IFC_RETURN(spProperty->get_Converter(spConverter.ReleaseAndGetAddressOf()));
                    level.pConverter = m_appReferences.Keep(spConverter.Get());
                    IFC_RETURN(spProperty->get_StringComparison(&level.stringComparison));
                }
                else
                {
                    level.pCustomDescription = m_appReferences.Keep(pBase);
                }
                level.hasDescription = true;

                ctl::ComPtr<xaml_data::ICollectionViewComparer> spGroupCustomSort;
                IFC_RETURN(pBase->get_CustomSortImpl(spGroupCustomSort.ReleaseAndGetAddressOf()));
                level.pGroupCustomSort = m_appReferences.Keep(spGroupCustomSort.Get());
                IFC_RETURN(level.groupSort.Initialize(pBase->GetSortDescriptionsNoRef(), nullptr /* pCustomSort */, culture, m_appReferences));
                level.hasGroupOrder = level.pGroupCustomSort || !level.groupSort.IsEmpty();
                IFC_RETURN(AreGroupSortKeysFixed(pBase->GetSortDescriptionsNoRef(), &level.areGroupSortKeysFixed));
                if (auto pNames = pBase->GetGroupNamesNoRef())
                {
                    UINT nameCount = 0;
                    IFC_RETURN(pNames->get_Size(&nameCount));
                    level.explicitNames.resize(nameCount);
                    for (UINT n = 0; n < nameCount; ++n)
                    {
                        ctl::ComPtr<IInspectable> spName;
                        IFC_RETURN(pNames->GetAt(n, spName.ReleaseAndGetAddressOf()));
                        level.explicitNames[n] = m_appReferences.Keep(spName.Get());
                    }
                }
                IFC_RETURN(pBase->AddOwner(pOwner));
            }

            IFC_RETURN(CreateExplicitSubgroups(this, 0));
            return S_OK;
        }

        // WPF: CollectionViewGroupRoot.InitializeGroup. A group gets the explicit subgroups of the next level (and
        // they theirs) when it is created; they are not presented until the group is.
        _Check_return_ HRESULT CreateExplicitSubgroups(_In_ CollectionViewGroupInternal* pGroup, UINT level)
        {
            if (level >= LevelCount())
            {
                return S_OK;
            }

            for (auto pName : m_levels[level].explicitNames)
            {
                ctl::ComPtr<CollectionViewGroupInternal> spSubgroup;
                IFC_RETURN(CreateGroupCore(pName, pGroup, level, true /* isExplicit */, spSubgroup.ReleaseAndGetAddressOf()));
                IFC_RETURN(pGroup->InsertSubgroup(pGroup->ExplicitSubgroupCount(), spSubgroup.Get(), false /* isRaised */));
            }
            return S_OK;
        }

        bool IsRoot() const override
        {
            return true;
        }

        // The view walks the app objects its levels keep (the root itself is not reached by the tracker walk).
        void WalkAppReferences(EReferenceTrackerWalkType walkType)
        {
            m_appReferences.ReferenceTrackerWalk(walkType);
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

        _Check_return_ HRESULT GetGroupNames(_In_opt_ IInspectable* pItem, _Out_ std::vector<ctl::ComPtr<IInspectable>>* pNames, _Out_opt_ bool* pHasCollectionName = nullptr)
        {
            pNames->clear();
            pNames->resize(m_levels.size());
            if (pHasCollectionName)
            {
                *pHasCollectionName = false;
            }
            for (size_t level = 0; level < m_levels.size(); ++level)
            {
                const auto& groupLevel = m_levels[level];
                if (!groupLevel.hasDescription)
                {
                    continue;
                }

                if (groupLevel.pCustomDescription)
                {
                    IFC_RETURN(groupLevel.pCustomDescription->GroupNameFromItemCoreProtected(pItem, static_cast<INT>(level), m_culture.Get(), (*pNames)[level].ReleaseAndGetAddressOf()));
                }
                else
                {
                    IFC_RETURN(EvaluateKey(groupLevel.selector, pItem, (*pNames)[level].ReleaseAndGetAddressOf()));
                    IFC_RETURN(ConvertGroupName(groupLevel.pConverter, static_cast<INT>(level), m_culture.Get(), (*pNames)[level]));
                }

                if (pHasCollectionName && IsCollectionName((*pNames)[level].Get()))
                {
                    *pHasCollectionName = true;
                }
            }

            return S_OK;
        }

        // WPF: a group name that is a collection (ICollection there; here an iterable that is not a string) puts the
        // item in a group for each of its names.
        static bool IsCollectionName(_In_opt_ IInspectable* pName)
        {
            return pName && (ctl::is<xaml_interop::IBindableIterable>(pName)
                || ctl::is<wfc::IIterable<IInspectable*>>(pName)
                || ctl::is<wfc::IIterable<HSTRING>>(pName));
        }

        static _Check_return_ HRESULT GetCollectionNames(_In_ IInspectable* pName, _Out_ std::vector<ctl::ComPtr<IInspectable>>* pNames)
        {
            pNames->clear();
            ctl::ComPtr<wfc::IIterable<IInspectable*>> spIterable;
            if (auto spBindable = ctl::query_interface_cast<xaml_interop::IBindableIterable>(pName))
            {
                // IBindableIterable is v-table compatible with IIterable<IInspectable*> (CollectionViewManager).
                spIterable = reinterpret_cast<wfc::IIterable<IInspectable*>*>(spBindable.Get());
            }
            else
            {
                spIterable = ctl::query_interface_cast<wfc::IIterable<IInspectable*>>(pName);
            }

            if (spIterable)
            {
                ctl::ComPtr<wfc::IIterator<IInspectable*>> spIterator;
                boolean hasCurrent = false;
                IFC_RETURN(spIterable->First(spIterator.ReleaseAndGetAddressOf()));
                IFC_RETURN(spIterator->get_HasCurrent(&hasCurrent));
                while (hasCurrent)
                {
                    ctl::ComPtr<IInspectable> spName;
                    IFC_RETURN(spIterator->get_Current(spName.ReleaseAndGetAddressOf()));
                    pNames->push_back(std::move(spName));
                    IFC_RETURN(spIterator->MoveNext(&hasCurrent));
                }
                return S_OK;
            }

            if (auto spStrings = ctl::query_interface_cast<wfc::IIterable<HSTRING>>(pName))
            {
                ctl::ComPtr<wfc::IIterator<HSTRING>> spIterator;
                boolean hasCurrent = false;
                IFC_RETURN(spStrings->First(spIterator.ReleaseAndGetAddressOf()));
                IFC_RETURN(spIterator->get_HasCurrent(&hasCurrent));
                while (hasCurrent)
                {
                    wrl_wrappers::HString value;
                    ctl::ComPtr<IInspectable> spName;
                    IFC_RETURN(spIterator->get_Current(value.GetAddressOf()));
                    IFC_RETURN(DirectUI::PropertyValue::CreateFromString(value.Get(), spName.ReleaseAndGetAddressOf()));
                    pNames->push_back(std::move(spName));
                    IFC_RETURN(spIterator->MoveNext(&hasCurrent));
                }
            }
            return S_OK;
        }

        // Some item is in more than one group. It has no single leaf group, so the incremental paths recompute instead
        // (WPF tracks each item's groups).
        bool HasMultiGroupItems() const
        {
            return m_hasMultiGroupItems;
        }

        _Check_return_ HRESULT NamesMatch(UINT level, _In_opt_ IInspectable* pGroupName, _In_opt_ IInspectable* pItemName, _Out_ bool* pMatch) const
        {
            const auto& groupLevel = m_levels[level];
            if (groupLevel.pCustomDescription)
            {
                BOOLEAN isMatch = FALSE;
                IFC_RETURN(groupLevel.pCustomDescription->NamesMatchCoreProtected(pGroupName, pItemName, &isMatch));
                *pMatch = !!isMatch;
                return S_OK;
            }

            IFC_RETURN(GroupNamesMatch(groupLevel.stringComparison, pGroupName, pItemName, pMatch));
            return S_OK;
        }

        // A name's key under the level's NamesMatch. A custom GroupDescription's names are keyed as its default
        // NamesMatch (ordinal) compares them; since an override may compare otherwise, FindSubgroup confirms a group
        // found by key with NamesMatch, and else asks NamesMatch of each subgroup.
        _Check_return_ HRESULT MakeNameKey(UINT level, _In_opt_ IInspectable* pName, _Out_ GroupNameKey* pKey, _Out_ bool* pIsHashed) const
        {
            *pIsHashed = false;
            const auto& groupLevel = m_levels[level];
            IFC_RETURN(MakeGroupNameKey(groupLevel.pCustomDescription ? xaml_data::CollectionViewStringComparison_Ordinal : groupLevel.stringComparison, pName, pKey, pIsHashed));
            return S_OK;
        }

        bool HasGroupOrder(UINT level) const
        {
            return m_levels[level].hasGroupOrder;
        }

        // Negative, zero or positive as group x sorts before, with or after group y (WPF: the group comparer
        // CollectionViewGroupInternal.FindIndex uses). CustomSort compares the group names; SortDescriptions compare
        // the keys each group read when it was created.
        _Check_return_ HRESULT CompareGroups(UINT level, _In_ CollectionViewGroupInternal* pLeft, _In_ CollectionViewGroupInternal* pRight, _Out_ int* pResult) const
        {
            const auto& groupLevel = m_levels[level];
            *pResult = 0;

            if (groupLevel.pGroupCustomSort)
            {
                INT32 result = 0;
                IFC_RETURN(groupLevel.pGroupCustomSort->Invoke(pLeft->GetKeyNoRef(), pRight->GetKeyNoRef(), &result));
                *pResult = (result > 0) - (result < 0);
                return S_OK;
            }

            if (groupLevel.areGroupSortKeysFixed)
            {
                IFC_RETURN(groupLevel.groupSort.Compare(ctl::as_iinspectable(pLeft), pLeft->GroupSortKeys().data(), 0, ctl::as_iinspectable(pRight), pRight->GroupSortKeys().data(), 0, pResult));
                return S_OK;
            }

            SortKeys leftKeys;
            SortKeys rightKeys;
            IFC_RETURN(groupLevel.groupSort.EvaluateKeys(ctl::as_iinspectable(pLeft), &leftKeys));
            IFC_RETURN(groupLevel.groupSort.EvaluateKeys(ctl::as_iinspectable(pRight), &rightKeys));
            IFC_RETURN(groupLevel.groupSort.Compare(ctl::as_iinspectable(pLeft), leftKeys.data(), 0, ctl::as_iinspectable(pRight), rightKeys.data(), 0, pResult));
            return S_OK;
        }

        // With a group order, the index of the first sibling that sorts after the new group; ties go after. The
        // siblings are in that order unless they are compared by what changes (ItemCount), so this is a binary
        // search, else a scan (WPF: CollectionViewGroupInternal.FindIndex). pRemoved is a subgroup a pending removal
        // takes away: it is passed over, and the position is the one once it is gone.
        _Check_return_ HRESULT FindOrderedSubgroupPosition(
            _In_ CollectionViewGroupInternal* pParent,
            UINT level,
            _In_ CollectionViewGroupInternal* pSubgroup,
            _In_opt_ CollectionViewGroupInternal* pRemoved,
            _Out_ UINT* pPosition) const
        {
            const auto& allSubgroups = pParent->Subgroups();
            const auto& groupLevel = m_levels[level];
            UINT removedIndex = UINT_MAX;
            for (UINT i = 0; pRemoved && i < allSubgroups.size(); ++i)
            {
                if (allSubgroups[i].Get() == pRemoved)
                {
                    removedIndex = i;
                    break;
                }
            }
            const UINT count = static_cast<UINT>(allSubgroups.size()) - ((removedIndex != UINT_MAX) ? 1 : 0);
            auto subgroupAt = [&allSubgroups, removedIndex](UINT i) { return allSubgroups[(i >= removedIndex) ? i + 1 : i].Get(); };

            if (!groupLevel.pGroupCustomSort && !groupLevel.areGroupSortKeysFixed)
            {
                UINT position = pParent->ExplicitSubgroupCount();
                for (; position < count; ++position)
                {
                    int result = 0;
                    IFC_RETURN(CompareGroups(level, pSubgroup, subgroupAt(position), &result));
                    if (result < 0)
                    {
                        break;
                    }
                }
                *pPosition = position;
                return S_OK;
            }

            UINT low = pParent->ExplicitSubgroupCount();
            UINT high = count;
            while (low < high)
            {
                const UINT middle = low + (high - low) / 2;
                int result = 0;
                IFC_RETURN(CompareGroups(level, pSubgroup, subgroupAt(middle), &result));
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

        // The subgroup of pGroup named pName, or null: by its name key (WPF: the name-to-group map), else by
        // NamesMatch among the subgroups whose names have no key.
        _Check_return_ HRESULT FindSubgroup(_In_ CollectionViewGroupInternal* pGroup, UINT level, _In_opt_ IInspectable* pName, _Outptr_result_maybenull_ CollectionViewGroupInternal** ppSubgroup) const
        {
            GroupNameKey key;
            bool isHashed = false;

            *ppSubgroup = nullptr;
            IFC_RETURN(MakeNameKey(level, pName, &key, &isHashed));
            auto pFound = isHashed ? pGroup->FindSubgroupByKey(key) : nullptr;
            if (!m_levels[level].pCustomDescription)
            {
                if (isHashed)
                {
                    *ppSubgroup = pFound;
                    return S_OK;
                }
            }
            else
            {
                bool isMatch = false;
                if (pFound)
                {
                    IFC_RETURN(NamesMatch(level, pFound->GetKeyNoRef(), pName, &isMatch));
                }
                if (isMatch)
                {
                    *ppSubgroup = pFound;
                    return S_OK;
                }

                for (const auto& spSubgroup : pGroup->Subgroups())
                {
                    if (spSubgroup.Get() == pFound)
                    {
                        continue;
                    }
                    IFC_RETURN(NamesMatch(level, spSubgroup->GetKeyNoRef(), pName, &isMatch));
                    if (isMatch)
                    {
                        *ppSubgroup = spSubgroup.Get();
                        break;
                    }
                }
                return S_OK;
            }

            for (auto pSubgroup : pGroup->UnhashedSubgroups())
            {
                bool isMatch = false;
                IFC_RETURN(NamesMatch(level, pSubgroup->GetKeyNoRef(), pName, &isMatch));
                if (isMatch)
                {
                    *ppSubgroup = pSubgroup;
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
        // Only the bottom-level groups get the item now; FinishLoading gives the groups above them their leaves.
        _Check_return_ HRESULT AddLoading(_In_opt_ IInspectable* pItem, const std::vector<ctl::ComPtr<IInspectable>>& names, UINT sourceIndex, UINT slot, bool hasCollectionName = false)
        {
            if (hasCollectionName)
            {
                UINT placements = 0;
                CollectionViewGroupInternal* pLastGroup = nullptr;
                IFC_RETURN(AddLoadingToSubgroups(this, 0, pItem, names, sourceIndex, slot, &placements, &pLastGroup));
                if (placements == 1)
                {
                    SetLeafGroup(sourceIndex, pLastGroup);
                }
                else if (placements > 1)
                {
                    // Every later source change and live restore of this root is then a full recompute (O(n log n)
                    // a change), until a recompute builds a root without such items.
                    m_hasMultiGroupItems = true;
                    SetLeafGroup(sourceIndex, MultiGroupLeaf());
                }
                return S_OK;
            }

            CollectionViewGroupInternal* pGroup = this;
            for (UINT level = 0; level < LevelCount(); ++level)
            {
                IFC_RETURN(FindOrCreateLoadingSubgroup(pGroup, level, names[level].Get(), &pGroup));
            }

            pGroup->SourceIndices().push_back(sourceIndex);
            pGroup->KeySlots().push_back(slot);
            IFC_RETURN(pGroup->InsertLeaf(pGroup->LeafCount(), pItem, false /* isRaised */));
            SetLeafGroup(sourceIndex, pGroup);
            return S_OK;
        }

        _Check_return_ HRESULT FindOrCreateLoadingSubgroup(_In_ CollectionViewGroupInternal* pGroup, UINT level, _In_opt_ IInspectable* pName, _Outptr_ CollectionViewGroupInternal** ppSubgroup)
        {
            CollectionViewGroupInternal* pSubgroup = nullptr;
            IFC_RETURN(FindSubgroup(pGroup, level, pName, &pSubgroup));
            if (!pSubgroup)
            {
                ctl::ComPtr<CollectionViewGroupInternal> spSubgroup;
                UINT position = static_cast<UINT>(pGroup->Subgroups().size());
                IFC_RETURN(CreateGroup(pName, pGroup, level, spSubgroup.ReleaseAndGetAddressOf()));
                if (HasGroupOrder(level) && !IsGroupOrderVariable(level))
                {
                    IFC_RETURN(FindOrderedSubgroupPosition(pGroup, level, spSubgroup.Get(), nullptr /* pRemoved */, &position));
                }
                IFC_RETURN(pGroup->InsertSubgroup(position, spSubgroup.Get(), false /* isRaised */));
                pSubgroup = spSubgroup.Get();
            }
            *ppSubgroup = pSubgroup;
            return S_OK;
        }

        // WPF: CollectionViewGroupRoot.AddToSubgroups while loading, with a name that is a collection at some level.
        // The bottom-level groups share the item's sort key slot.
        _Check_return_ HRESULT AddLoadingToSubgroups(
            _In_ CollectionViewGroupInternal* pGroup,
            UINT level,
            _In_opt_ IInspectable* pItem,
            const std::vector<ctl::ComPtr<IInspectable>>& names,
            UINT sourceIndex,
            UINT slot,
            _Inout_ UINT* pPlacements,
            _Inout_ CollectionViewGroupInternal** ppLastGroup)
        {
            if (level == LevelCount())
            {
                pGroup->SourceIndices().push_back(sourceIndex);
                pGroup->KeySlots().push_back(slot);
                IFC_RETURN(pGroup->InsertLeaf(pGroup->LeafCount(), pItem, false /* isRaised */));
                ++*pPlacements;
                *ppLastGroup = pGroup;
                return S_OK;
            }

            auto pName = names[level].Get();
            if (!IsCollectionName(pName))
            {
                CollectionViewGroupInternal* pSubgroup = nullptr;
                IFC_RETURN(FindOrCreateLoadingSubgroup(pGroup, level, pName, &pSubgroup));
                IFC_RETURN(AddLoadingToSubgroups(pSubgroup, level + 1, pItem, names, sourceIndex, slot, pPlacements, ppLastGroup));
                return S_OK;
            }

            std::vector<ctl::ComPtr<IInspectable>> collectionNames;
            IFC_RETURN(GetCollectionNames(pName, &collectionNames));
            for (const auto& spName : collectionNames)
            {
                CollectionViewGroupInternal* pSubgroup = nullptr;
                IFC_RETURN(FindOrCreateLoadingSubgroup(pGroup, level, spName.Get(), &pSubgroup));
                IFC_RETURN(AddLoadingToSubgroups(pSubgroup, level + 1, pItem, names, sourceIndex, slot, pPlacements, ppLastGroup));
            }
            return S_OK;
        }

        _Check_return_ HRESULT FinishLoading()
        {
            bool isReordered = false;
            IFC_RETURN(CollectLeaves());
            IFC_RETURN(SortVariableLevels(this, 0, &isReordered));
            if (isReordered)
            {
                IFC_RETURN(CollectLeaves());
            }
            return S_OK;
        }

        // A level ordered by what its groups' items change (ItemCount): its groups are ordered once their items are
        // known, and a change that leaves them out of order is applied by a recompute.
        bool IsGroupOrderVariable(UINT level) const
        {
            const auto& groupLevel = m_levels[level];
            return groupLevel.hasGroupOrder && !groupLevel.pGroupCustomSort && !groupLevel.areGroupSortKeysFixed;
        }

        bool HasVariableGroupOrder() const
        {
            for (UINT level = 0; level < LevelCount(); ++level)
            {
                if (IsGroupOrderVariable(level))
                {
                    return true;
                }
            }
            return false;
        }

        _Check_return_ HRESULT IsGroupOrderValid(_In_ CollectionViewGroupInternal* pGroup, UINT level, _Out_ bool* pIsValid) const
        {
            *pIsValid = true;
            if (pGroup->IsBottomLevel())
            {
                return S_OK;
            }

            const auto& subgroups = pGroup->Subgroups();
            if (IsGroupOrderVariable(level))
            {
                for (size_t i = pGroup->ExplicitSubgroupCount(); i + 1 < subgroups.size(); ++i)
                {
                    int result = 0;
                    IFC_RETURN(CompareGroups(level, subgroups[i].Get(), subgroups[i + 1].Get(), &result));
                    if (result > 0)
                    {
                        *pIsValid = false;
                        return S_OK;
                    }
                }
            }

            for (const auto& spSubgroup : subgroups)
            {
                IFC_RETURN(IsGroupOrderValid(spSubgroup.Get(), level + 1, pIsValid));
                if (!*pIsValid)
                {
                    break;
                }
            }
            return S_OK;
        }

        _Check_return_ HRESULT SortVariableLevels(_In_ CollectionViewGroupInternal* pGroup, UINT level, _Inout_ bool* pIsReordered)
        {
            if (pGroup->IsBottomLevel())
            {
                return S_OK;
            }

            for (const auto& spSubgroup : pGroup->Subgroups())
            {
                IFC_RETURN(SortVariableLevels(spSubgroup.Get(), level + 1, pIsReordered));
            }

            if (!IsGroupOrderVariable(level))
            {
                return S_OK;
            }

            struct Entry
            {
                ctl::ComPtr<CollectionViewGroupInternal> spGroup;
                SortKeys keys;
            };

            const auto& groupSort = m_levels[level].groupSort;
            const auto& subgroups = pGroup->Subgroups();
            const UINT explicitCount = pGroup->ExplicitSubgroupCount();
            std::vector<Entry> entries;
            entries.reserve(subgroups.size() - explicitCount);
            for (size_t i = explicitCount; i < subgroups.size(); ++i)
            {
                Entry entry;
                entry.spGroup = subgroups[i];
                IFC_RETURN(groupSort.EvaluateKeys(ctl::as_iinspectable(entry.spGroup.Get()), &entry.keys));
                entries.push_back(std::move(entry));
            }

            // Only group SortDescriptions reach here (IsGroupOrderVariable excludes a group CustomSort): cached keys
            // with a total order (CompareSortKeys), no app code, so std::stable_sort cannot see an inconsistent order.
            ASSERT(!groupSort.HasCustomSort());
            std::stable_sort(entries.begin(), entries.end(), [&groupSort](const Entry& left, const Entry& right)
            {
                int result = 0;
                VERIFYHR(groupSort.Compare(ctl::as_iinspectable(left.spGroup.Get()), left.keys.data(), 0, ctl::as_iinspectable(right.spGroup.Get()), right.keys.data(), 0, &result));
                return result < 0;
            });

            bool isChanged = false;
            for (size_t i = 0; i < entries.size(); ++i)
            {
                isChanged = isChanged || (entries[i].spGroup.Get() != subgroups[explicitCount + i].Get());
            }
            if (!isChanged)
            {
                return S_OK;
            }

            std::vector<ctl::ComPtr<CollectionViewGroupInternal>> ordered(subgroups.begin(), subgroups.begin() + explicitCount);
            for (auto& entry : entries)
            {
                ordered.push_back(std::move(entry.spGroup));
            }
            IFC_RETURN(pGroup->ReorderSubgroups(std::move(ordered)));
            *pIsReordered = true;
            return S_OK;
        }

        // A group reads its name key and, with group SortDescriptions, its sort keys once, when it is created. It
        // starts with the explicit subgroups of the next level.
        _Check_return_ HRESULT CreateGroup(_In_opt_ IInspectable* pName, _In_ CollectionViewGroupInternal* pParent, UINT level, _Outptr_ CollectionViewGroupInternal** ppGroup)
        {
            IFC_RETURN(CreateGroupCore(pName, pParent, level, false /* isExplicit */, ppGroup));
            return S_OK;
        }

        _Check_return_ HRESULT CreateGroupCore(_In_opt_ IInspectable* pName, _In_ CollectionViewGroupInternal* pParent, UINT level, bool isExplicit, _Outptr_ CollectionViewGroupInternal** ppGroup)
        {
            ctl::ComPtr<CollectionViewGroupInternal> spGroup;
            GroupNameKey key;
            bool isHashed = false;
            const auto& groupLevel = m_levels[level];

            IFC_RETURN(ctl::ComObject<CollectionViewGroupInternal>::CreateInstance(spGroup.ReleaseAndGetAddressOf()));
            IFC_RETURN(spGroup->InitializeGroup(pName, pParent, level + 1 == LevelCount()));
            if (isExplicit)
            {
                spGroup->SetIsExplicit();
            }
            IFC_RETURN(MakeNameKey(level, pName, &key, &isHashed));
            spGroup->SetNameKey(std::move(key), isHashed);
            if (groupLevel.areGroupSortKeysFixed && !groupLevel.pGroupCustomSort)
            {
                IFC_RETURN(groupLevel.groupSort.EvaluateKeys(ctl::as_iinspectable(spGroup.Get()), &spGroup->GroupSortKeys()));
            }
            IFC_RETURN(CreateExplicitSubgroups(spGroup.Get(), level + 1));
            *ppGroup = spGroup.Detach();
            return S_OK;
        }

        // The bottom-level group and position that hold the item at sourceIndex.
        bool FindLeaf(UINT sourceIndex, _Outptr_result_maybenull_ CollectionViewGroupInternal** ppGroup, _Out_ UINT* pPosition) const
        {
            *ppGroup = nullptr;
            *pPosition = 0;
            if (sourceIndex >= m_leafGroups.size() || !m_leafGroups[sourceIndex] || m_leafGroups[sourceIndex] == MultiGroupLeaf())
            {
                return false;
            }

            auto pGroup = m_leafGroups[sourceIndex];
            auto& sourceIndices = pGroup->SourceIndices();
            auto found = std::find(sourceIndices.begin(), sourceIndices.end(), sourceIndex);
            if (found == sourceIndices.end())
            {
                return false;
            }

            *ppGroup = pGroup;
            *pPosition = static_cast<UINT>(found - sourceIndices.begin());
            return true;
        }

        void SetLeafGroup(UINT sourceIndex, _In_opt_ CollectionViewGroupInternal* pGroup)
        {
            if (sourceIndex < m_leafGroups.size())
            {
                m_leafGroups[sourceIndex] = pGroup;
            }
        }

        // A source insert (delta 1) or remove (delta -1) at sourceIndex shifts the later source indices.
        void ShiftSourceIndices(UINT sourceIndex, int delta)
        {
            ShiftSourceIndicesIn(this, sourceIndex, delta);
            if (delta > 0 && sourceIndex <= m_leafGroups.size())
            {
                m_leafGroups.insert(m_leafGroups.begin() + sourceIndex, nullptr);
            }
            else if (delta < 0 && sourceIndex < m_leafGroups.size())
            {
                m_leafGroups.erase(m_leafGroups.begin() + sourceIndex);
            }
        }

    protected:
        CollectionViewGroupRoot() = default;

    private:
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
        AppReferences m_appReferences;
        wrl_wrappers::HString m_culture;
        // m_leafGroups' marker for an item in several groups (a collection-valued group name): not one group, and not
        // filtered out (null). Never dereferenced.
        static CollectionViewGroupInternal* MultiGroupLeaf()
        {
            static char s_marker;
            return reinterpret_cast<CollectionViewGroupInternal*>(&s_marker);
        }

        // The bottom-level group of each source item the view shows, by source index (null: filtered out;
        // MultiGroupLeaf: in several groups).
        std::vector<CollectionViewGroupInternal*> m_leafGroups;
        bool m_hasMultiGroupItems = false;
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

    // Compares an item (its keys) with the item at a position of a sorted output, reading the other item's
    // cached keys; only a CustomSort reads the other item itself.
    template <typename TItemAt>
    _Check_return_ HRESULT CompareWithSlot(
        const SortState& sort,
        _In_opt_ IInspectable* pItem,
        const SortKeys& keys,
        UINT sourceIndex,
        const TItemAt& itemAt,
        UINT position,
        UINT otherSlot,
        UINT otherSourceIndex,
        _Out_ int* pResult)
    {
        ctl::ComPtr<IInspectable> spOther;
        if (sort.comparer.HasCustomSort())
        {
            IFC_RETURN(itemAt(position, spOther.ReleaseAndGetAddressOf()));
        }
        IFC_RETURN(sort.comparer.Compare(pItem, keys.data(), sourceIndex, spOther.Get(), sort.keys.At(otherSlot), otherSourceIndex, pResult));
        return S_OK;
    }

    // Where an item belongs in a sorted list of size items, without the item's own slot (excludedPosition, or
    // UINT_MAX). Items isSkipped marks are not compared (WPF: LiveShapingBlock.FindPosition ignores IsSortDirty
    // items, whose place is stale). The item is in order anywhere in [*pFirst, *pLast]; *pPosition is the upper
    // bound, the place a full recompute gives it when nothing is skipped. compareAt(position, &result) compares
    // the item with the one at an actual position.
    template <typename TCompareAt, typename TSourceIndexAt, typename TIsSkipped>
    _Check_return_ HRESULT FindSortedRange(
        UINT size,
        UINT excludedPosition,
        const TCompareAt& compareAt,
        const TSourceIndexAt& sourceIndexAt,
        const TIsSkipped& isSkipped,
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

            int result = 0;
            IFC_RETURN(compareAt(actual(probe), &result));
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
    // a replaced item still in the group, or UINT_MAX. isSkipped marks (by source index) items not to compare.
    template <typename TIsSkipped>
    _Check_return_ HRESULT FindLeafPosition(
        _In_ CollectionViewGroupInternal* pGroup,
        const SortState& sort,
        _In_opt_ IInspectable* pItem,
        const SortKeys& keys,
        UINT sourceIndex,
        UINT excludedPosition,
        const TIsSkipped& isSkipped,
        _Out_ UINT* pPosition,
        _Out_opt_ UINT* pFirst = nullptr,
        _Out_opt_ UINT* pLast = nullptr)
    {
        const auto& sourceIndices = pGroup->SourceIndices();
        const auto& slots = pGroup->KeySlots();
        auto itemAt = [pGroup](UINT position, IInspectable** ppItem) { return pGroup->GetAt(position, ppItem); };
        IFC_RETURN(FindSortedRange(
            pGroup->LeafCount(),
            excludedPosition,
            [&](UINT position, int* pResult) { return CompareWithSlot(sort, pItem, keys, sourceIndex, itemAt, position, slots[position], sourceIndices[position], pResult); },
            [&sourceIndices](UINT position) { return sourceIndices[position]; },
            isSkipped,
            pPosition, pFirst, pLast));
        return S_OK;
    }

    // What an item's removal takes away, so its new place can be planned, with all the app code that needs, before
    // anything is raised: the item's bottom-level group and position, and the group that goes with it (WPF:
    // RemoveFromSubgroups removes the groups the item was the last leaf of; explicit groups stay), or null.
    struct GroupedExclusion
    {
        CollectionViewGroupInternal* pOldGroup = nullptr;
        UINT oldPosition = UINT_MAX;
        CollectionViewGroupInternal* pRemoved = nullptr;
    };

    inline CollectionViewGroupInternal* FindGroupEmptiedByRemoval(_In_ CollectionViewGroupInternal* pGroup)
    {
        CollectionViewGroupInternal* pEmptied = nullptr;
        for (auto pCurrent = pGroup; pCurrent && !pCurrent->IsRoot() && pCurrent->LeafCount() == 1; pCurrent = pCurrent->GetParentNoRef())
        {
            if (!pCurrent->IsExplicit())
            {
                pEmptied = pCurrent;
            }
        }
        return pEmptied;
    }

    inline GroupedExclusion MakeGroupedExclusion(_In_ CollectionViewGroupInternal* pOldGroup, UINT oldPosition)
    {
        GroupedExclusion exclusion;
        exclusion.pOldGroup = pOldGroup;
        exclusion.oldPosition = oldPosition;
        exclusion.pRemoved = FindGroupEmptiedByRemoval(pOldGroup);
        return exclusion;
    }

    // The first leaf under pGroup once the excluded item has gone: its bottom-level group and position.
    inline bool FindFirstLeaf(_In_ CollectionViewGroupInternal* pGroup, const GroupedExclusion& exclusion, _Outptr_ CollectionViewGroupInternal** ppBottom, _Out_ UINT* pPosition)
    {
        if (pGroup == exclusion.pRemoved || pGroup->LeafCount() == 0)
        {
            return false;
        }

        if (pGroup->IsBottomLevel())
        {
            const UINT position = (pGroup == exclusion.pOldGroup && exclusion.oldPosition == 0) ? 1 : 0;
            if (position >= pGroup->LeafCount())
            {
                return false;
            }
            *ppBottom = pGroup;
            *pPosition = position;
            return true;
        }

        for (const auto& spSubgroup : pGroup->Subgroups())
        {
            if (FindFirstLeaf(spSubgroup.Get(), exclusion, ppBottom, pPosition))
            {
                return true;
            }
        }
        return false;
    }

    // A new subgroup's index (WPF: CollectionViewGroupInternal.FindIndex): before the first subgroup whose first
    // item sorts after the new group's item. The index is the one once the excluded item has gone.
    inline _Check_return_ HRESULT FindSubgroupPosition(
        _In_ CollectionViewGroupRoot* pRoot,
        _In_ CollectionViewGroupInternal* pParent,
        UINT level,
        _In_ CollectionViewGroupInternal* pSubgroup,
        const SortState& sort,
        _In_opt_ IInspectable* pItem,
        const SortKeys& keys,
        UINT sourceIndex,
        const GroupedExclusion& exclusion,
        _Out_ UINT* pPosition)
    {
        const auto& subgroups = pParent->Subgroups();
        UINT position = pParent->ExplicitSubgroupCount();
        UINT removedBefore = 0;

        if (pRoot->HasGroupOrder(level))
        {
            IFC_RETURN(pRoot->FindOrderedSubgroupPosition(pParent, level, pSubgroup, exclusion.pRemoved, pPosition));
            return S_OK;
        }

        for (; position < subgroups.size(); ++position)
        {
            CollectionViewGroupInternal* pSeedGroup = nullptr;
            UINT seedPosition = 0;
            if (subgroups[position].Get() == exclusion.pRemoved)
            {
                removedBefore = 1;
                continue;
            }
            if (!FindFirstLeaf(subgroups[position].Get(), exclusion, &pSeedGroup, &seedPosition))
            {
                continue;
            }

            int result = 0;
            IFC_RETURN(CompareWithSlot(sort, pItem, keys, sourceIndex,
                [pSeedGroup](UINT itemPosition, IInspectable** ppSeed) { return pSeedGroup->GetAt(itemPosition, ppSeed); },
                seedPosition, pSeedGroup->KeySlots()[seedPosition], pSeedGroup->SourceIndices()[seedPosition], &result));
            if (result < 0)
            {
                break;
            }
        }

        *pPosition = position - removedBefore;
        return S_OK;
    }

    // Where an item joins the group tree, given its group names (one per level). With an exclusion, the plan is for
    // the tree once the excluded item has been removed, so the removal and the insertion can both be applied without
    // running app code between them. Runs app code (converters, comparers, NamesMatch) and changes nothing that is
    // presented.
    template <typename TIsSkipped>
    _Check_return_ HRESULT PlanGroupedInsertion(
        _In_ CollectionViewGroupRoot* pRoot,
        const SortState& sort,
        _In_opt_ IInspectable* pItem,
        const SortKeys& keys,
        const std::vector<ctl::ComPtr<IInspectable>>& names,
        UINT sourceIndex,
        const GroupedExclusion& exclusion,
        const TIsSkipped& isSkipped,
        _Out_ GroupedInsertion* pInsertion)
    {
        CollectionViewGroupInternal* pGroup = pRoot;

        pInsertion->isAccepted = true;

        for (UINT level = 0; level < pRoot->LevelCount(); ++level)
        {
            CollectionViewGroupInternal* pSubgroup = nullptr;
            IFC_RETURN(pRoot->FindSubgroup(pGroup, level, names[level].Get(), &pSubgroup));
            if (pSubgroup && pSubgroup != exclusion.pRemoved)
            {
                pGroup = pSubgroup;
                continue;
            }

            // The item starts a branch: build it, holding the item, before it is presented. Its key slot is
            // assigned when the branch is applied. Below the first new group the item may join one of the explicit
            // subgroups the new groups start with.
            CollectionViewGroupInternal* pParent = pGroup;
            for (UINT newLevel = level; newLevel < pRoot->LevelCount(); ++newLevel)
            {
                ctl::ComPtr<CollectionViewGroupInternal> spNewGroup;
                CollectionViewGroupInternal* pExplicit = nullptr;
                if (newLevel != level)
                {
                    IFC_RETURN(pRoot->FindSubgroup(pParent, newLevel, names[newLevel].Get(), &pExplicit));
                }
                if (pExplicit)
                {
                    spNewGroup = pExplicit;
                }
                else
                {
                    IFC_RETURN(pRoot->CreateGroup(names[newLevel].Get(), pParent, newLevel, spNewGroup.ReleaseAndGetAddressOf()));
                }
                IFC_RETURN(spNewGroup->InsertLeaf(0, pItem, false /* isRaised */));
                if (newLevel == level)
                {
                    pInsertion->spNewGroup = spNewGroup;
                }
                else if (!pExplicit)
                {
                    IFC_RETURN(pParent->InsertSubgroup(pParent->ExplicitSubgroupCount(), spNewGroup.Get(), false /* isRaised */));
                }
                if (newLevel + 1 == pRoot->LevelCount())
                {
                    spNewGroup->SourceIndices().push_back(sourceIndex);
                    spNewGroup->KeySlots().push_back(SortKeyStore::c_noSlot);
                }
                pParent = spNewGroup.Get();
            }

            pInsertion->pGroup = pGroup;
            IFC_RETURN(FindSubgroupPosition(pRoot, pGroup, level, pInsertion->spNewGroup.Get(), sort, pItem, keys, sourceIndex, exclusion, &pInsertion->position));
            return S_OK;
        }

        pInsertion->pGroup = pGroup;
        IFC_RETURN(FindLeafPosition(pGroup, sort, pItem, keys, sourceIndex, (pGroup == exclusion.pOldGroup) ? exclusion.oldPosition : UINT_MAX, isSkipped, &pInsertion->position));
        return S_OK;
    }

    inline _Check_return_ HRESULT ApplyGroupedInsertion(_In_ CollectionViewGroupRoot* pRoot, _Inout_ SortState& sort, const GroupedInsertion& insertion, _In_opt_ IInspectable* pItem, const SortKeys& keys, UINT sourceIndex)
    {
        auto insertLeaf = [pItem](CollectionViewGroupInternal* pGroup, UINT index) { return pGroup->InsertLeaf(index, pItem, true /* isRaised */); };

        if (insertion.spNewGroup)
        {
            auto pBottom = insertion.spNewGroup->FirstBottomGroup();
            pBottom->KeySlots().front() = sort.keys.Allocate(keys);
            pRoot->SetLeafGroup(sourceIndex, pBottom);
            IFC_RETURN(insertion.pGroup->InsertSubgroup(insertion.position, insertion.spNewGroup.Get(), true /* isRaised */));
            IFC_RETURN(ChangeAncestorLeaves(insertion.spNewGroup.Get(), 0, insertLeaf));
            return S_OK;
        }

        auto& sourceIndices = insertion.pGroup->SourceIndices();
        auto& slots = insertion.pGroup->KeySlots();
        sourceIndices.insert(sourceIndices.begin() + insertion.position, sourceIndex);
        slots.insert(slots.begin() + insertion.position, sort.keys.Allocate(keys));
        pRoot->SetLeafGroup(sourceIndex, insertion.pGroup);
        IFC_RETURN(insertLeaf(insertion.pGroup, insertion.position));
        IFC_RETURN(ChangeAncestorLeaves(insertion.pGroup, insertion.position, insertLeaf));
        return S_OK;
    }

    // An item leaves its group (WPF: RemoveFromSubgroups); the groups it was the last item of go with it.
    inline _Check_return_ HRESULT ApplyGroupedRemoval(_In_ CollectionViewGroupRoot* pRoot, _Inout_ SortState& sort, _In_ CollectionViewGroupInternal* pGroup, UINT position)
    {
        auto removeLeaf = [](CollectionViewGroupInternal* pAncestor, UINT index) { return pAncestor->RemoveLeaf(index); };
        // Explicit groups stay when they empty (WPF: EmptyGroupRemover skips them); their non-explicit ancestors go.
        CollectionViewGroupInternal* pEmptied = FindGroupEmptiedByRemoval(pGroup);

        pRoot->SetLeafGroup(pGroup->SourceIndices()[position], nullptr);
        sort.keys.Free(pGroup->KeySlots()[position]);

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
        auto& slots = pGroup->KeySlots();
        sourceIndices.erase(sourceIndices.begin() + position);
        slots.erase(slots.begin() + position);
        IFC_RETURN(pGroup->RemoveLeaf(position));
        IFC_RETURN(ChangeAncestorLeaves(pGroup, position, removeLeaf));
        return S_OK;
    }

    inline _Check_return_ HRESULT ApplyGroupedReplace(_Inout_ SortState& sort, _In_ CollectionViewGroupInternal* pGroup, UINT position, _In_opt_ IInspectable* pItem, const SortKeys& keys)
    {
        auto setLeaf = [pItem](CollectionViewGroupInternal* pAncestor, UINT index) { return pAncestor->SetLeaf(index, pItem); };
        sort.keys.Set(pGroup->KeySlots()[position], keys);
        IFC_RETURN(setLeaf(pGroup, position));
        IFC_RETURN(ChangeAncestorLeaves(pGroup, position, setLeaf));
        return S_OK;
    }
}
