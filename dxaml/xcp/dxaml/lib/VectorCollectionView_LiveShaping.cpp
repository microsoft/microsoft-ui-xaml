// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      CVS shaping engine: live sorting, filtering and grouping (WPF: ICollectionViewLiveShaping,
//      LiveShapingList). See VectorCollectionView_Shaping.cpp.

#include "precomp.h"
#include "VectorCollectionView_ShapingInternal.h"

using namespace DirectUI;

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
        // A plain property name (no '.', indexer or attached property): on an INotifyPropertyChanged item it is
        // followed by the item's one PropertyChanged handler (LivePropertyObserver::ConnectToPropertyChanged).
        bool isPropertyName = false;
    };

    bool IsPropertyName(_In_ HSTRING path)
    {
        UINT32 length = 0;
        LPCWSTR buffer = WindowsGetStringRawBuffer(path, &length);
        for (UINT32 i = 0; i < length; ++i)
        {
            const WCHAR c = buffer[i];
            if (!((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9') || c == L'_'))
            {
                return false;
            }
        }
        return length > 0;
    }

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
        added.isPropertyName = IsPropertyName(path);
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
        if (m_epPropertyChanged && !m_source.Empty())
        {
            auto spSource = m_source.Begin()->GetSafeReference();
            if (spSource)
            {
                IGNOREHR(m_epPropertyChanged.DetachEventHandler(spSource.Get()));
            }
        }
        m_source.Clear();
        m_pView = nullptr;
    }

    // Follows every plain property name among the paths on an INotifyPropertyChanged item with one handler, as the
    // listener's step would follow each of them (INPCListenerBase: the names compare ordinally, a null or empty name
    // is every property). One handler and one tracked reference per item, instead of a listener with its steps,
    // property access and handler per item and path (P-4 in the design note).
    _Check_return_ HRESULT ConnectToPropertyChanged(_In_ const std::vector<LivePath>* pPaths, _In_ IInspectable* pSource, _In_ xaml_data::INotifyPropertyChanged* pNotifier)
    {
        m_pPaths = pPaths;
        m_source.Append(pSource);
        IFC_RETURN(m_epPropertyChanged.AttachEventHandler(pNotifier,
            [this](IInspectable*, xaml_data::IPropertyChangedEventArgs* pArgs)
            {
                return OnSourcePropertyChanged(pArgs);
            }));
        return S_OK;
    }

    // The listener holds the item (an app object), so the view walks it (VectorCollectionView::OnReferenceTrackerWalk).
    void ReferenceTrackerWalk(EReferenceTrackerWalkType walkType)
    {
        m_listener.ReferenceTrackerWalk(walkType);
        m_source.ReferenceTrackerWalk(walkType);
    }

    LiveShapingItem* Item() const
    {
        return m_pItem;
    }

    UINT Flags() const
    {
        return m_flags;
    }

    // The flags of the live properties the change being reported is about.
    UINT ChangedFlags() const
    {
        return m_changedFlags;
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
        IFC_RETURN(ReportChange(m_flags));
        return S_OK;
    }

private:
    _Check_return_ HRESULT ReportChange(UINT flags)
    {
        if (m_pView && flags)
        {
            IFC_RETURN(m_pView->CheckThread());
            m_changedFlags = flags;
            IFC_RETURN(m_pView->OnLivePropertyChanged(this));
        }
        return S_OK;
    }

    _Check_return_ HRESULT OnSourcePropertyChanged(_In_ xaml_data::IPropertyChangedEventArgs* pArgs)
    {
        wrl_wrappers::HString name;
        UINT flags = 0;

        if (!m_pView || !m_pPaths)
        {
            return S_OK;
        }

        IFC_RETURN(pArgs->get_PropertyName(name.GetAddressOf()));
        if (!name.Get())
        {
            flags = m_flags;
        }
        else
        {
            UINT32 length = 0;
            LPCWSTR buffer = name.GetRawBuffer(&length);
            for (const auto& path : *m_pPaths)
            {
                UINT32 pathLength = 0;
                LPCWSTR pathBuffer = path.isPropertyName ? WindowsGetStringRawBuffer(path.path.Get(), &pathLength) : nullptr;
                if (pathBuffer && pathLength == length && wcsncmp(buffer, pathBuffer, length) == 0)
                {
                    flags |= path.flags;
                }
            }
        }

        IFC_RETURN(ReportChange(flags & m_flags));
        return S_OK;
    }

    VectorCollectionView* m_pView;
    LiveShapingItem* m_pItem;
    UINT m_flags;
    UINT m_changedFlags = 0;
    // At most one; a TrackerPtrVector so the view can walk it.
    TrackerPtrVector<PropertyPathListener> m_listener;
    // ConnectToPropertyChanged: the item (at most one, walked as the listener is) and its PropertyChanged handler.
    TrackerPtrVector<IInspectable> m_source;
    ctl::EventPtr<PropertyChangedEventCallback> m_epPropertyChanged;
    const std::vector<LivePath>* m_pPaths = nullptr;
};

// One per source item, in source order (WPF: LiveShapingItem). Dirty flags mark a live property that changed.
struct VectorCollectionView::LiveShapingItem
{
    std::vector<std::unique_ptr<LivePropertyObserver>> observers;
    UINT dirtyFlags = 0;
    // The source item's identity, kept alive by the observers' listeners (none: the item is not compared).
    IUnknown* pIdentity = nullptr;
};

struct VectorCollectionView::LiveShapingState
{
    std::vector<LivePath> paths;
    std::vector<std::unique_ptr<LiveShapingItem>> items;
    // Items whose dirty flags include sorting.
    UINT sortDirtyCount = 0;
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
    return m_spLiveShaping && m_isLiveSorting && m_spLiveShaping->sortDirtyCount > 0;
}

// Whether the item at sourceIndex (after a source change at index the live items do not follow yet) has a
// sort key that changed since it was placed. Such an item is out of place, so a binary search skips it
// (WPF: LiveShapingBlock.FindPosition ignores IsSortDirty items).
bool VectorCollectionView::IsSortDirtyAfterSourceChange(wfc::CollectionChange change, UINT index, UINT sourceIndex) const
{
    if (!HasSortDirtyLiveItems())
    {
        return false;
    }

    UINT itemIndex = sourceIndex;
    if (change == wfc::CollectionChange_ItemInserted)
    {
        if (sourceIndex == index)
        {
            return false;
        }
        itemIndex = (sourceIndex > index) ? sourceIndex - 1 : sourceIndex;
    }

    const auto& items = m_spLiveShaping->items;
    return itemIndex < items.size() && (items[itemIndex]->dirtyFlags & LiveShapingFlag_Sorting) != 0;
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
    const auto walkTypeAsEnum = static_cast<EReferenceTrackerWalkType>(walkType);

    if (m_spFilterEvents)
    {
        CollectionViewSource::WalkFilterEvents(m_spFilterEvents.Get(), walkTypeAsEnum);
    }

    if (m_spSortState)
    {
        m_spSortState->references.ReferenceTrackerWalk(walkTypeAsEnum);
    }

    if (m_spGroupRoot)
    {
        static_cast<CollectionViewGroupRoot*>(static_cast<CollectionViewGroupInternal*>(m_spGroupRoot.Get()))->WalkAppReferences(walkTypeAsEnum);
    }

    if (m_spLiveShaping)
    {
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

    if (!IsLiveShaping() || !m_tpInternalList || !m_tpSourceList)
    {
        ClearLiveShapingItems();
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
                ctl::ComPtr<xaml_data::IGroupDescription> spDescription;
                wrl_wrappers::HString path;
                IFC_RETURN(m_tpGroupDescriptions->GetAt(i, spDescription.ReleaseAndGetAddressOf()));
                auto spProperty = ctl::query_interface_cast<PropertyGroupDescription>(spDescription.Get());
                if (spProperty)
                {
                    IFC_RETURN(spProperty->get_PropertyName(path.GetAddressOf()));
                    IFC_RETURN(AddLivePath(paths, path.Get(), LiveShapingFlag_Grouping));
                }
            }
        }
    }

    IFC_RETURN(m_tpSourceList->get_Size(&size));

    auto canReuse = [&](_Out_ bool* pIsReused) -> HRESULT
    {
        *pIsReused = false;
        auto spState = m_spLiveShaping;
        if (!spState || spState->items.size() != size || spState->paths.size() != paths.size())
        {
            return S_OK;
        }

        for (size_t i = 0; i < paths.size(); ++i)
        {
            INT32 result = 1;
            IFC_RETURN(WindowsCompareStringOrdinal(spState->paths[i].path.Get(), paths[i].path.Get(), &result));
            if (result != 0 || spState->paths[i].flags != paths[i].flags)
            {
                return S_OK;
            }
        }

        for (UINT i = 0; i < size; ++i)
        {
            ctl::ComPtr<IInspectable> spSourceItem;
            ctl::ComPtr<IUnknown> spIdentity;
            const auto& spItem = spState->items[i];
            IFC_RETURN(m_tpSourceList->GetAt(i, spSourceItem.ReleaseAndGetAddressOf()));
            if (spSourceItem)
            {
                IFC_RETURN(spSourceItem.As(&spIdentity));
            }
            if (spItem->observers.empty() || spItem->pIdentity != spIdentity.Get())
            {
                return S_OK;
            }
        }

        *pIsReused = (m_spLiveShaping == spState);
        return S_OK;
    };

    // The same paths over the same items: the observers stay, and the refresh this follows placed every item.
    bool isReused = false;
    IFC_RETURN(canReuse(&isReused));
    if (isReused)
    {
        for (auto& spItem : m_spLiveShaping->items)
        {
            spItem->dirtyFlags = 0;
        }
        m_spLiveShaping->sortDirtyCount = 0;
        return S_OK;
    }

    ClearLiveShapingItems();
    if (paths.empty())
    {
        return S_OK;
    }

    auto spState = std::make_shared<LiveShapingState>();
    spState->paths = std::move(paths);
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
    if (spSourceItem)
    {
        ctl::ComPtr<IUnknown> spIdentity;
        IFC_RETURN(spSourceItem.As(&spIdentity));
        spItem->pIdentity = spIdentity.Get();
    }

    // The listener would follow these items' plain property names through their PropertyChanged too; a
    // DependencyObject (property changes), a map (MapChanged) and a collection view (its current item) are followed
    // by a listener per path.
    ctl::ComPtr<xaml_data::INotifyPropertyChanged> spNotifier;
    if (spSourceItem
        && !ctl::is<xaml::IDependencyObject>(spSourceItem.Get())
        && !ctl::is<wfc::IMap<HSTRING, IInspectable*>>(spSourceItem.Get())
        && !ctl::is<xaml_data::ICollectionView>(spSourceItem.Get()))
    {
        spNotifier = ctl::query_interface_cast<xaml_data::INotifyPropertyChanged>(spSourceItem.Get());
    }

    UINT notifiedFlags = 0;
    for (const auto& livePath : state.paths)
    {
        if (spNotifier && livePath.isPropertyName)
        {
            notifiedFlags |= livePath.flags;
            continue;
        }

        auto spObserver = std::make_unique<LivePropertyObserver>(this, spItem.get(), livePath.flags);
        // A path the item does not have is observed as a binding to it would be: it never changes.
        if (SUCCEEDED(spObserver->Connect(livePath.path.Get(), spSourceItem.Get())))
        {
            spItem->observers.push_back(std::move(spObserver));
        }
    }

    if (notifiedFlags != 0)
    {
        auto spObserver = std::make_unique<LivePropertyObserver>(this, spItem.get(), notifiedFlags);
        if (SUCCEEDED(spObserver->ConnectToPropertyChanged(&state.paths, spSourceItem.Get(), spNotifier.Get())))
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

// The reference tracker walk reads the sort state, the group tree and the Filter events, so a destroyed view lets go of them under the
// lock the walk runs under, before its members are destroyed.
void VectorCollectionView::ReleaseShapingState()
{
    std::shared_ptr<ShapingSortState> spSortState;
    ctl::ComPtr<xaml_data::ICollectionViewGroup> spGroupRoot;
    ctl::ComPtr<IFilterEventSource> spFilterEvents;
    if (!m_spSortState && !m_spGroupRoot && !m_spFilterEvents)
    {
        return;
    }

    {
        AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());
        spSortState = std::move(m_spSortState);
        spGroupRoot = std::move(m_spGroupRoot);
        spFilterEvents = std::move(m_spFilterEvents);
    }
}
void VectorCollectionView::ClearLiveShapingItems()
{
    std::shared_ptr<LiveShapingState> spState;
    if (!m_spLiveShaping)
    {
        return;
    }

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
            if (spRemoved->dirtyFlags & LiveShapingFlag_Sorting)
            {
                --spState->sortDirtyCount;
            }
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

    auto pItem = pObserver->Item();
    if (!(pItem->dirtyFlags & LiveShapingFlag_Sorting) && (pObserver->ChangedFlags() & LiveShapingFlag_Sorting))
    {
        ++spState->sortDirtyCount;
    }
    pItem->dirtyFlags |= pObserver->ChangedFlags();

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

namespace
{
    // Estimated costs of a live restore (amd64fre, CollectionViewPerfBenchmarks; see the design note). An insert or
    // remove shifts the presented list at ~14 ns a row and plans and raises the change in ~30 us; a flat re-sort on
    // the cached keys announced as a Reset costs ~1.4 us a row, and a grouped recompute ~13 us a row. Moves keep the
    // realized rows, focus, scroll position and UIA tree, so they are preferred until they cost twice a Reset, or
    // while they fit a budget of a few frames.
    constexpr DOUBLE c_nsPerShiftedRow = 14;
    constexpr DOUBLE c_nsPerChange = 30000;
    constexpr DOUBLE c_nsPerResortedRow = 1400;
    constexpr DOUBLE c_nsPerRecomputedRow = 13000;
    constexpr DOUBLE c_changeBudgetNs = 50000000;

    // changeCount single changes (a move is two), each shifting rowsShifted rows, against a Reset that costs resetNs.
    bool AreChangesPreferred(UINT64 changeCount, UINT rowsShifted, DOUBLE resetNs)
    {
        const DOUBLE changesNs = static_cast<DOUBLE>(changeCount) * (rowsShifted * c_nsPerShiftedRow + c_nsPerChange);
        return changesNs <= std::max(2 * resetNs, c_changeBudgetNs);
    }

    // Marks a longest increasing subsequence of values: the items a reorder leaves in place.
    std::vector<bool> MarkLongestIncreasingRun(const std::vector<UINT>& values)
    {
        std::vector<UINT> tails;
        std::vector<UINT> previous(values.size(), UINT_MAX);
        for (UINT i = 0; i < values.size(); ++i)
        {
            auto tail = std::lower_bound(tails.begin(), tails.end(), values[i], [&values](UINT index, UINT value) { return values[index] < value; });
            if (tail != tails.begin())
            {
                previous[i] = *(tail - 1);
            }
            if (tail == tails.end())
            {
                tails.push_back(i);
            }
            else
            {
                *tail = i;
            }
        }

        std::vector<bool> isInRun(values.size(), false);
        for (UINT i = tails.empty() ? UINT_MAX : tails.back(); i != UINT_MAX; i = previous[i])
        {
            isInRun[i] = true;
        }
        return isInRun;
    }
}

_Check_return_ HRESULT VectorCollectionView::OnLiveShapingRestoreCallback()
{
    if (m_spLiveShaping)
    {
        m_spLiveShaping->isRestoreQueued = false;
    }

    // Nothing the app called waits for this restore, so a failure of its code is reported; the view stays consistent.
    ContainedFailureReport report;
    const HRESULT hr = RestoreLiveShaping();
    if (IsUncontainedFailure(hr))
    {
        IFC_RETURN(hr);
    }
    if (FAILED(hr))
    {
        report.Report(hr);
    }
    return S_OK;
}

// WPF: ListCollectionView.RestoreLiveShaping. Each changed item is moved to where it now belongs, filtered in
// or out, or moved to its new group, with an ItemRemoved and an ItemInserted (WinRT vectors have no Move); an
// item still in order raises nothing. Like WPF's FindPosition, placing one item ignores the changed items not
// yet placed, whose positions are stale. The app code an item needs runs before its changes are raised, so a
// failure leaves no removal without its insertion, and the view then recomputes.
//
// When moving each changed item would cost more than a Reset, a flat view is re-sorted once on the cached keys and
// raises the fewest moves that reach the new order (the items outside its longest run still in order), or one Reset
// if those still cost more; a grouped view is recomputed. WPF re-sorts the whole list with a Reset once 80% of it
// changed.
_Check_return_ HRESULT VectorCollectionView::RestoreLiveShaping()
{
    auto spState = m_spLiveShaping;
    auto spSort = m_spSortState;
    ShapingFilterEvents filterEvents;
    std::vector<bool> isSortDirty;
    const UINT sourceChangeCount = m_sourceListChangeCount;
    UINT dirtyCount = 0;
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
        dirtyCount += (spItem->dirtyFlags != 0) ? 1 : 0;
    }

    if (dirtyCount == 0)
    {
        return S_OK;
    }

    // A source change that could not be followed is pending: recompute, which observes the items afresh.
    if (!m_areSourceIndicesValid || !spSort || spState->items.size() != m_sourceListSize)
    {
        IFC_RETURN(RefreshOrDefer());
        return S_OK;
    }

    const bool isSorting = m_isLiveSorting && !spSort->comparer.IsEmpty();
    const UINT itemCount = static_cast<UINT>(spState->items.size());
    const UINT presentedCount = static_cast<UINT>(m_sourceIndices.size());
    bool isResorted = false;
    if (m_isGrouping)
    {
        // A move shifts the leaves of its top-level group.
        UINT topLevelGroupCount = 0;
        IFC_RETURN(m_tpInternalList->get_Size(&topLevelGroupCount));
        if (!AreChangesPreferred(2ull * dirtyCount, itemCount / std::max(topLevelGroupCount, 1u), itemCount * c_nsPerRecomputedRow))
        {
            IFC_RETURN(RefreshOrDefer());
            return S_OK;
        }
    }
    else
    {
        isResorted = !AreChangesPreferred(2ull * dirtyCount, presentedCount, presentedCount * c_nsPerResortedRow);
    }

    // Currency follows the current item through its moves (VectorCollectionView_Currency.cpp).
    CurrencyScope currency;
    IFC_RETURN(BeginLiveCurrency(&currency));
    IFC_RETURN(GetFilterEvents(&filterEvents));

    const bool hasFilter = !!filterEvents.spEvents || !!m_tpFilter;

    // A flat view where many items changed: the changed items' keys (and filter) are read again, the others' are
    // cached, the output is re-sorted once, and the fewest moves that reach it are raised, or a Reset.
    auto resortFlat = [&]() -> HRESULT
    {
        struct Entry
        {
            ctl::ComPtr<IInspectable> item;
            UINT sourceIndex = 0;
            UINT slot = 0;
            // UINT_MAX: the filter accepts the item now.
            UINT oldPosition = UINT_MAX;
        };

        auto pOutput = static_cast<InternalList*>(m_tpInternalList.Get());
        auto& slots = spSort->flatSlots;
        const bool isFiltering = m_isLiveFiltering && hasFilter;
        std::vector<Entry> entries;
        std::vector<UINT> leavingSourceIndices;
        std::vector<bool> isInView(itemCount, false);
        HRESULT hr = S_OK;

        entries.reserve(m_sourceIndices.size());
        {
            BusyScope busy(m_shapeBusyDepth);
            hr = [&]() -> HRESULT
            {
                for (UINT position = 0; position < m_sourceIndices.size(); ++position)
                {
                    Entry entry;
                    entry.sourceIndex = m_sourceIndices[position];
                    entry.slot = slots[position];
                    entry.oldPosition = position;
                    IFC_RETURN(pOutput->GetAt(position, entry.item.ReleaseAndGetAddressOf()));
                    isInView[entry.sourceIndex] = true;

                    const UINT flags = spState->items[entry.sourceIndex]->dirtyFlags;
                    bool isAccepted = true;
                    if ((flags & LiveShapingFlag_Filtering) && isFiltering)
                    {
                        IFC_RETURN(PassesFilter(entry.item.Get(), filterEvents, &isAccepted));
                    }
                    if (!isAccepted)
                    {
                        leavingSourceIndices.push_back(entry.sourceIndex);
                        continue;
                    }
                    if (isSorting && (flags & LiveShapingFlag_Sorting))
                    {
                        SortKeys keys;
                        IFC_RETURN(spSort->comparer.EvaluateKeys(entry.item.Get(), &keys));
                        spSort->keys.Set(entry.slot, keys);
                    }
                    entries.push_back(std::move(entry));
                }

                // Items the filter rejected before and may accept now.
                for (UINT sourceIndex = 0; isFiltering && sourceIndex < itemCount; ++sourceIndex)
                {
                    if (isInView[sourceIndex] || !(spState->items[sourceIndex]->dirtyFlags & LiveShapingFlag_Filtering))
                    {
                        continue;
                    }

                    Entry entry;
                    bool isAccepted = false;
                    entry.sourceIndex = sourceIndex;
                    IFC_RETURN(m_tpSourceList->GetAt(sourceIndex, entry.item.ReleaseAndGetAddressOf()));
                    IFC_RETURN(PassesFilter(entry.item.Get(), filterEvents, &isAccepted));
                    if (isAccepted)
                    {
                        SortKeys keys;
                        IFC_RETURN(spSort->comparer.EvaluateKeys(entry.item.Get(), &keys));
                        entry.slot = spSort->keys.Allocate(std::move(keys));
                        entries.push_back(std::move(entry));
                    }
                }

                HRESULT hrCompare = S_OK;
                const auto& sort = *spSort;
                auto isBefore = [&sort, &hrCompare](const Entry& left, const Entry& right)
                {
                    int result = 0;
                    if (SUCCEEDED(hrCompare))
                    {
                        hrCompare = sort.comparer.Compare(left.item.Get(), sort.keys.At(left.slot), left.sourceIndex, right.item.Get(), sort.keys.At(right.slot), right.sourceIndex, &result);
                    }
                    return SUCCEEDED(hrCompare) && result < 0;
                };

                if (isSorting)
                {
                    std::stable_sort(entries.begin(), entries.end(), isBefore);
                }
                else
                {
                    // Without live sorting the items stay in order, and each item the filter now accepts is placed
                    // among them by a binary search, as a small tick places it. The kept items need not be in order
                    // under the comparer's current values (a CustomSort sees their edits), so a position is only where
                    // the search lands; joiners that land together are ordered by the comparer. One merge builds the
                    // output.
                    auto joining = std::find_if(entries.begin(), entries.end(), [](const Entry& entry) { return entry.oldPosition == UINT_MAX; });
                    const size_t keptCount = static_cast<size_t>(joining - entries.begin());
                    std::vector<std::pair<size_t, size_t>> joinerSlots;
                    joinerSlots.reserve(entries.size() - keptCount);
                    for (auto it = joining; it != entries.end() && SUCCEEDED(hrCompare); ++it)
                    {
                        const auto landingAt = std::upper_bound(entries.begin(), joining, *it, isBefore) - entries.begin();
                        joinerSlots.emplace_back(static_cast<size_t>(landingAt), static_cast<size_t>(it - entries.begin()));
                    }
                    std::stable_sort(joinerSlots.begin(), joinerSlots.end(), [&](const auto& left, const auto& right)
                    {
                        return left.first != right.first ? left.first < right.first : isBefore(entries[left.second], entries[right.second]);
                    });

                    std::vector<Entry> merged;
                    merged.reserve(entries.size());
                    size_t nextKept = 0;
                    for (const auto& [landing, joinerIndex] : joinerSlots)
                    {
                        while (nextKept < landing)
                        {
                            merged.push_back(std::move(entries[nextKept++]));
                        }
                        merged.push_back(std::move(entries[joinerIndex]));
                    }
                    while (nextKept < keptCount)
                    {
                        merged.push_back(std::move(entries[nextKept++]));
                    }
                    entries = std::move(merged);
                }
                IFC_RETURN(hrCompare);
                return S_OK;
            }();
        }
        IFC_RETURN(hr);

        if (sourceChangeCount != m_sourceListChangeCount || m_spLiveShaping != spState)
        {
            isApplied = false;
            return S_OK;
        }

        for (auto& spItem : spState->items)
        {
            spItem->dirtyFlags = 0;
        }
        spState->sortDirtyCount = 0;

        // The fewest changes that reach the new order: the items outside the longest run still in order move.
        std::vector<UINT> oldPositions;
        oldPositions.reserve(entries.size());
        for (const auto& entry : entries)
        {
            if (entry.oldPosition != UINT_MAX)
            {
                oldPositions.push_back(entry.oldPosition);
            }
        }
        const auto isStaying = MarkLongestIncreasingRun(oldPositions);
        UINT64 changeCount = leavingSourceIndices.size();
        for (size_t i = 0, run = 0; i < entries.size(); ++i)
        {
            if (entries[i].oldPosition == UINT_MAX)
            {
                ++changeCount;
            }
            else if (!isStaying[run++])
            {
                changeCount += 2;
            }
        }

        auto positionOf = [this](UINT sourceIndex) { return static_cast<UINT>(std::find(m_sourceIndices.begin(), m_sourceIndices.end(), sourceIndex) - m_sourceIndices.begin()); };

        if (AreChangesPreferred(changeCount, presentedCount, presentedCount * c_nsPerResortedRow))
        {
            BusyScope busy(m_shapeBusyDepth);
            for (const UINT sourceIndex : leavingSourceIndices)
            {
                const UINT position = positionOf(sourceIndex);
                m_sourceIndices.erase(m_sourceIndices.begin() + position);
                spSort->keys.Free(slots[position]);
                slots.erase(slots.begin() + position);
                IFC_RETURN(pOutput->InternalRemoveAt(position));
            }

            // In the new order, each item that moves or joins goes after the one before it, already in place.
            for (size_t i = 0, run = 0; i < entries.size(); ++i)
            {
                const auto& entry = entries[i];
                if (entry.oldPosition != UINT_MAX)
                {
                    if (isStaying[run++])
                    {
                        continue;
                    }
                    const UINT position = positionOf(entry.sourceIndex);
                    m_sourceIndices.erase(m_sourceIndices.begin() + position);
                    slots.erase(slots.begin() + position);
                    IFC_RETURN(pOutput->InternalRemoveAt(position));
                }

                const UINT newPosition = (i == 0) ? 0 : positionOf(entries[i - 1].sourceIndex) + 1;
                m_sourceIndices.insert(m_sourceIndices.begin() + newPosition, entry.sourceIndex);
                slots.insert(slots.begin() + newPosition, entry.slot);
                IFC_RETURN(pOutput->InternalInsertAt(newPosition, entry.item.Get()));
            }
            return S_OK;
        }

        for (const UINT sourceIndex : leavingSourceIndices)
        {
            spSort->keys.Free(slots[positionOf(sourceIndex)]);
        }

        std::vector<UINT> sourceIndices;
        std::vector<UINT> newSlots;
        std::vector<ctl::ComPtr<IInspectable>> items;
        sourceIndices.reserve(entries.size());
        newSlots.reserve(entries.size());
        items.reserve(entries.size());
        for (auto& entry : entries)
        {
            sourceIndices.push_back(entry.sourceIndex);
            newSlots.push_back(entry.slot);
            items.push_back(std::move(entry.item));
        }

        m_sourceIndices = std::move(sourceIndices);
        slots = std::move(newSlots);
        {
            BusyScope busy(m_shapeBusyDepth);
            IFC_RETURN(pOutput->ReplaceAll(items));
        }
        return S_OK;
    };

    isSortDirty.resize(spState->items.size());
    for (size_t i = 0; i < spState->items.size(); ++i)
    {
        isSortDirty[i] = isSorting && (spState->items[i]->dirtyFlags & LiveShapingFlag_Sorting) != 0;
    }

    auto isSkipped = [&isSortDirty](UINT sourceIndex) { return sourceIndex < isSortDirty.size() && isSortDirty[sourceIndex]; };

    // A flat view: the item's place in the InternalList.
    auto restoreFlat = [&](UINT sourceIndex, UINT flags, _In_opt_ IInspectable* pItem) -> HRESULT
    {
        auto pOutput = static_cast<InternalList*>(m_tpInternalList.Get());
        auto& slots = spSort->flatSlots;
        auto found = std::find(m_sourceIndices.begin(), m_sourceIndices.end(), sourceIndex);
        const bool isInView = found != m_sourceIndices.end();
        const UINT oldPosition = isInView ? static_cast<UINT>(found - m_sourceIndices.begin()) : UINT_MAX;
        const bool isFiltering = (flags & LiveShapingFlag_Filtering) && m_isLiveFiltering && hasFilter;
        const bool isResorting = (flags & LiveShapingFlag_Sorting) && isSorting;
        bool isAccepted = isInView;
        bool isMoving = false;
        UINT newPosition = 0;
        SortKeys keys;
        HRESULT hr = S_OK;

        {
            BusyScope busy(m_shapeBusyDepth);
            hr = [&]() -> HRESULT
            {
                if (isFiltering)
                {
                    IFC_RETURN(PassesFilter(pItem, filterEvents, &isAccepted));
                }

                if (!isAccepted || (isInView && !isResorting))
                {
                    return S_OK;
                }

                UINT first = 0;
                UINT last = 0;
                IFC_RETURN(spSort->comparer.EvaluateKeys(pItem, &keys));
                IFC_RETURN(FindSortedRange(
                    static_cast<UINT>(m_sourceIndices.size()),
                    oldPosition,
                    [&](UINT position, int* pResult)
                    {
                        return CompareWithSlot(*spSort, pItem, keys, sourceIndex,
                            [pOutput](UINT outputPosition, IInspectable** ppOther) { return pOutput->GetAt(outputPosition, ppOther); },
                            position, slots[position], m_sourceIndices[position], pResult);
                    },
                    [this](UINT position) { return m_sourceIndices[position]; },
                    isSkipped,
                    &newPosition, &first, &last));
                isMoving = isInView && (oldPosition < first || oldPosition > last);
                return S_OK;
            }();
        }
        IFC_RETURN(hr);

        if (sourceChangeCount != m_sourceListChangeCount)
        {
            isApplied = false;
            return S_OK;
        }

        const bool isRemoved = isInView && (!isAccepted || isMoving);
        const bool isInserted = isAccepted && (!isInView || isMoving);
        if (!isRemoved && !isInserted)
        {
            // Still in order: its new keys are the ones later placements compare with.
            if (isInView && isResorting)
            {
                spSort->keys.Set(slots[oldPosition], keys);
            }
            return S_OK;
        }

        BusyScope busy(m_shapeBusyDepth);
        if (isRemoved)
        {
            m_sourceIndices.erase(m_sourceIndices.begin() + oldPosition);
            spSort->keys.Free(slots[oldPosition]);
            slots.erase(slots.begin() + oldPosition);
            IFC_RETURN(pOutput->InternalRemoveAt(oldPosition));
        }

        if (isInserted)
        {
            m_sourceIndices.insert(m_sourceIndices.begin() + newPosition, sourceIndex);
            slots.insert(slots.begin() + newPosition, spSort->keys.Allocate(keys));
            IFC_RETURN(pOutput->InternalInsertAt(newPosition, pItem));
        }
        return S_OK;
    };

    // A grouped view (WPF: CollectionViewGroupRoot.RestoreGrouping and MoveItemWithinGroups). An item that changes
    // group is planned for the tree its removal leaves, before the removal is raised.
    auto restoreGrouped = [&](UINT sourceIndex, UINT flags, _In_opt_ IInspectable* pItem) -> HRESULT
    {
        auto pRoot = static_cast<CollectionViewGroupRoot*>(static_cast<CollectionViewGroupInternal*>(m_spGroupRoot.Get()));
        CollectionViewGroupInternal* pOldGroup = nullptr;
        UINT oldPosition = 0;
        const bool isInView = pRoot && pRoot->FindLeaf(sourceIndex, &pOldGroup, &oldPosition);
        const bool isFiltering = (flags & LiveShapingFlag_Filtering) && m_isLiveFiltering && hasFilter;
        const bool isResorting = (flags & LiveShapingFlag_Sorting) && isSorting;
        const bool isRegrouping = (flags & LiveShapingFlag_Grouping) && m_isLiveGrouping;
        SortKeys keys;
        GroupedInsertion insertion;
        bool isAccepted = isInView;
        bool isMoving = false;
        HRESULT hr = S_OK;

        if (!pRoot)
        {
            return S_OK;
        }

        // An item in several groups is placed by a recompute.
        if (pRoot->HasMultiGroupItems())
        {
            isApplied = false;
            return S_OK;
        }

        bool hasCollectionName = false;
        {
            BusyScope busy(m_shapeBusyDepth);
            hr = [&]() -> HRESULT
            {
                if (isFiltering)
                {
                    IFC_RETURN(PassesFilter(pItem, filterEvents, &isAccepted));
                }

                if (!isAccepted || (isInView && !isResorting && !isRegrouping))
                {
                    return S_OK;
                }

                std::vector<ctl::ComPtr<IInspectable>> names;
                IFC_RETURN(spSort->comparer.EvaluateKeys(pItem, &keys));
                if (!isInView)
                {
                    IFC_RETURN(pRoot->GetGroupNames(pItem, &names, &hasCollectionName));
                    if (hasCollectionName)
                    {
                        return S_OK;
                    }
                    IFC_RETURN(PlanGroupedInsertion(pRoot, *spSort, pItem, keys, names, sourceIndex, GroupedExclusion(), isSkipped, &insertion));
                    return S_OK;
                }

                bool isInGroup = true;
                if (isRegrouping)
                {
                    IFC_RETURN(pRoot->GetGroupNames(pItem, &names, &hasCollectionName));
                    if (hasCollectionName)
                    {
                        return S_OK;
                    }
                    IFC_RETURN(pRoot->IsGroupPath(pOldGroup, names, &isInGroup));
                }

                if (!isInGroup)
                {
                    isMoving = true;
                    IFC_RETURN(PlanGroupedInsertion(pRoot, *spSort, pItem, keys, names, sourceIndex, MakeGroupedExclusion(pOldGroup, oldPosition), isSkipped, &insertion));
                    return S_OK;
                }

                if (isResorting)
                {
                    UINT position = 0;
                    UINT first = 0;
                    UINT last = 0;
                    IFC_RETURN(FindLeafPosition(pOldGroup, *spSort, pItem, keys, sourceIndex, oldPosition, isSkipped, &position, &first, &last));
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
        }
        IFC_RETURN(hr);

        if (sourceChangeCount != m_sourceListChangeCount || hasCollectionName)
        {
            isApplied = false;
            return S_OK;
        }

        const bool isRemoved = isInView && (!isAccepted || isMoving);
        const bool isInserted = isAccepted && (!isInView || isMoving);
        if (!isRemoved && !isInserted)
        {
            if (isInView && isResorting)
            {
                spSort->keys.Set(pOldGroup->KeySlots()[oldPosition], keys);
            }
            return S_OK;
        }

        BusyScope busy(m_shapeBusyDepth);
        if (isRemoved)
        {
            IFC_RETURN(ApplyGroupedRemoval(pRoot, *spSort, pOldGroup, oldPosition));
        }

        if (isInserted)
        {
            IFC_RETURN(ApplyGroupedInsertion(pRoot, *spSort, insertion, pItem, keys, sourceIndex));
        }
        return S_OK;
    };

    RaiseFailureScope raiseFailure;
    HRESULT hr = S_OK;
    if (isResorted)
    {
        hr = resortFlat();
    }

    for (UINT i = 0; !isResorted && SUCCEEDED(hr) && isApplied && i < isSortDirty.size(); ++i)
    {
        // A handler of a change raised here may have refreshed the view, which observes the items afresh.
        if (m_spLiveShaping != spState || spState->items.size() != isSortDirty.size())
        {
            break;
        }

        const UINT flags = std::exchange(spState->items[i]->dirtyFlags, 0u);
        if (flags & LiveShapingFlag_Sorting)
        {
            --spState->sortDirtyCount;
        }
        if (flags != 0)
        {
            ctl::ComPtr<IInspectable> spItem;
            hr = m_tpSourceList->GetAt(i, spItem.ReleaseAndGetAddressOf());
            if (SUCCEEDED(hr))
            {
                hr = m_isGrouping ? restoreGrouped(i, flags, spItem.Get()) : restoreFlat(i, flags, spItem.Get());
            }
        }
        isSortDirty[i] = false;
        isApplied = isApplied && (sourceChangeCount == m_sourceListChangeCount);
    }

    isApplied = isApplied && (sourceChangeCount == m_sourceListChangeCount);
    if (SUCCEEDED(hr) && isApplied)
    {
        hr = CheckGroupOrderAfterChange(&isApplied);
    }
    if (FAILED(hr) || !isApplied)
    {
        // The changes raised so far are complete; the rest is recomputed.
        ContainedFailureReport::Capture(hr);
        m_areSourceIndicesValid = false;
        KeepFirstFailure(&hr, RefreshOrDefer());
        KeepFirstFailure(&hr, EndLiveCurrency(&currency));
        KeepFirstFailure(&hr, raiseFailure.Result());
        return hr;
    }

    IFC_RETURN(EndLiveCurrency(&currency));

    IFC_RETURN(RunPendingRecompute());
    IFC_RETURN(raiseFailure.Result());
    return S_OK;
}
