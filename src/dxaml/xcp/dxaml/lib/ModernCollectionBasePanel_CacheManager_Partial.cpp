// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ModernCollectionBasePanel.g.h"
#include "ItemsControl.g.h"
#include "VisualTreeHelper.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//#define MCBP_DEBUG

#undef min
#undef max

ModernCollectionBasePanel::CacheManager::CacheManager(_In_ ModernCollectionBasePanel* owner)
    : m_owner(owner)    // no ref needed, cachemanager is not at all exposed to the outside world
    , m_cachedIsGrouped(FALSE)
    , m_cachedWasGrouped(FALSE)
    , m_cachedItemsCount(0)
    , m_cachedHistoryItemsCount(0)
    , m_wrHost(nullptr)
    , m_strongHost(nullptr)
    , m_strongChildren(nullptr)
    , m_metricsAreCached(FALSE)
    , m_metricsWereCached(FALSE)
    , m_isCollectionCacheValid(FALSE)
    , m_isFocusedChildValid(FALSE)
    , m_recyclingItemIndex(-1)
    , m_hidesIfEmpty(FALSE)
    , m_cachedNonEmptyGroupsCount(0)
    , m_interpretItemCollectionResetAsFullReset(FALSE)
{
}

ModernCollectionBasePanel::CacheManager::~CacheManager()
{

}

ModernCollectionBasePanel::CacheManager::GroupCache::GroupCache() noexcept
    : startItemIndex(-1)
    , endItemIndex(-1)
    , indexOfGroup(-1)
    , nonEmptyIndexOfGroup(-1)
{

}

// a bit on-orthodox. we want to return an instance of the StrongRefLifetime type here so we can store it in an auto and
// let it go off the stack at the end of usage. However, I do not want to ignore the potential HR failure that results
// from a failed call to GuaranteeCache, so I return it as an out param.
ModernCollectionBasePanel::CacheManager::StrongRefLifetime ModernCollectionBasePanel::CacheManager::CacheStrongRefs(_Out_ HRESULT* hr)
{
    *hr = GuaranteeCache();
    return StrongRefLifetime(this);
}

_Check_return_ HRESULT ModernCollectionBasePanel::CacheManager::GetItemsHost(_Out_ ctl::ComPtr<IGeneratorHost>* pspItemsHost) const
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IGeneratorHost> spItemsHost;

    if (m_strongHost)
    {
        // just take that one, we're in a guaranteed session
        spItemsHost = m_strongHost;
    }
    else
    {
        // resolve, but do not store, since we're not in a guaranteed session
        // Construct a temporary WeakRefPtr because ::As() is non-const
        IFC(ctl::WeakRefPtr(m_wrHost).As(&spItemsHost));
    }


    *pspItemsHost = std::move(spItemsHost);

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT ModernCollectionBasePanel::CacheManager::GetChildren(_Out_ ctl::ComPtr<wfc::IVector<xaml::UIElement*>>* pspChildren) const
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

    if (m_strongChildren)
    {
        // just take that one, we're in a guaranteed session
        spChildren = m_strongChildren;
    }
    else
    {
        // resolve, but do not store, since we're not in a guaranteed session
        IFC(m_owner->get_Children(&spChildren));
    }

    *pspChildren = std::move(spChildren);

Cleanup:
    RRETURN(hr);
}

// potentially creates grouping cache, and make strong versions of some important variables
_Check_return_ HRESULT ModernCollectionBasePanel::CacheManager::GuaranteeCache()
{
    HRESULT hr = S_OK;

    ctl::ComPtr<ICollectionView> spCollectionView;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spCollectionGroups;
    ctl::ComPtr<IGroupStyle> spGroupStyle;
    ctl::ComPtr<IItemsControl> spItemsControl;

    // these will be released during Unguarantee
    IFC(m_wrHost.As(&m_strongHost));
    IFC(m_owner->get_Children(&m_strongChildren));

    // Initialize the collection cache
    m_isCollectionCacheValid = FALSE;

    // Will force to get focus information once per virtualization
    m_isFocusedChildValid = FALSE;

    // Ensure our group size vectors are allocated
    if (!m_spGroupSizes)
    {
        IFC(ctl::make(&m_spGroupSizes));
    }
    if (!m_spNonEmptyGroupSizes)
    {
        IFC(ctl::make(&m_spNonEmptyGroupSizes));
    }

    if (!m_metricsAreCached)
    {
        // If no GroupStyle is set, then many of our decisions to perform grouped layout
        // will make no sense. Therefore, we'll just display the items as non-grouped.
        // This is also what ICG did for legacy panels
        IFC(m_owner->GetGroupStyle(&spGroupStyle));
        if (spGroupStyle)
        {
            IFC(m_strongHost->get_CollectionView(&spCollectionView));
            if (spCollectionView)
            {
                IFC(spCollectionView->get_CollectionGroups(&spCollectionGroups));
                if (spCollectionGroups)
                {
                    m_cachedIsGrouped = TRUE;
                    m_cachedItemsCount = 0;
                }
            }
        }

        m_cachedGroupInformation.clear();
        m_spGroupSizes->ClearView();
        m_spNonEmptyGroupSizes->ClearView();

        if (m_cachedIsGrouped)
        {
            UINT size = 0;
            UINT emptyGroupsSoFar = 0;
            ctl::ComPtr<wfc::IVector<IInspectable*>> spCollectionGroupsAsV;
            std::vector<UINT> groupSizes;
            std::vector<UINT> nonEmptyGroupSizes;

            IFC(spCollectionGroups.As<wfc::IVector<IInspectable*>>(&spCollectionGroupsAsV));

            IFC(spCollectionGroupsAsV->get_Size(&size));

            m_cachedGroupInformation.reserve(size);
            groupSizes.reserve(size);
            nonEmptyGroupSizes.reserve(size);

            for (UINT i = 0; i < size; ++i)
            {
                ctl::ComPtr<IInspectable> spCurrent;
                ctl::ComPtr<ICollectionViewGroup> spCurrentGroup;
                ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spCurrentGroupItems;
                ctl::ComPtr<wfc::IVector<IInspectable*>> spCurrentGroupItemsAsV;
                UINT groupSize = 0;
                ModernCollectionBasePanel::CacheManager::GroupCache cache;
                IFC(spCollectionGroupsAsV->GetAt(i, &spCurrent));
                IFC(spCurrent.As<ICollectionViewGroup>(&spCurrentGroup));
                IFC(spCurrentGroup->get_GroupItems(&spCurrentGroupItems));
                IFC(spCurrentGroupItems.As<wfc::IVector<IInspectable*>>(&spCurrentGroupItemsAsV));
                IFC(spCurrentGroupItemsAsV->get_Size(&groupSize));

                cache.startItemIndex = m_cachedItemsCount;
                cache.endItemIndex = m_cachedItemsCount + groupSize;
                cache.indexOfGroup = static_cast<INT32>(i);
                cache.nonEmptyIndexOfGroup = static_cast<INT32>(i - emptyGroupsSoFar);

                ASSERT(m_cachedGroupInformation.size() < m_cachedGroupInformation.capacity());
                m_cachedGroupInformation.push_back(cache);

                m_cachedItemsCount += groupSize;
                if (groupSize == 0)
                {
                    ++emptyGroupsSoFar;
                }
                else
                {
                    nonEmptyGroupSizes.push_back(groupSize);
                }
                groupSizes.push_back(groupSize);
            }

            m_cachedNonEmptyGroupsCount = size - emptyGroupsSoFar;
            m_spGroupSizes->SetView(std::move(groupSizes));
            m_spNonEmptyGroupSizes->SetView(std::move(nonEmptyGroupSizes));
        }
        else
        {
            UINT size = 0;
            ctl::ComPtr<wfc::IVector<IInspectable*>> spView;
            m_cachedIsGrouped = FALSE;

            IFC(m_strongHost->get_View(&spView));
            IFC(spView->get_Size(&size));
            m_cachedItemsCount = static_cast<INT32>(size);
            m_cachedNonEmptyGroupsCount = 0;
        }
        m_metricsAreCached = TRUE;
    }

    IFC(m_strongHost->SetIsGrouping(m_cachedIsGrouped));

    spItemsControl = m_strongHost.AsOrNull<IItemsControl>();
    if (spItemsControl)
    {
        IFC(spItemsControl.Cast<ItemsControl>()->GetRecyclingContext(&m_spContainerRecyclingContext));
    }
    m_recyclingItemIndex = -1;

Cleanup:
    RRETURN(hr);

}

void ModernCollectionBasePanel::CacheManager::UnguaranteeCache()
{
    // Reset focus cached information
    m_isFocusedChildValid = FALSE;
    m_spFocusedChild.Reset();

    // kill off the resolved variables
    m_strongHost.Reset();
    m_strongChildren.Reset();

    if (m_spContainerRecyclingContext)
    {
        IGNOREHR(m_spContainerRecyclingContext->StopRecycling());
    }
    m_spContainerRecyclingContext.Reset();

    IGNOREHR(ClearCollectionsCache());

    // notice how we are not invalidating the grouping cache. This needs to be
    // tied into INCC notifications
}

// this is a very impactful slow operation (or rather, it will lead to slowness).
// ***  Only call this method if you really mean it!!  ***
_Check_return_ HRESULT ModernCollectionBasePanel::CacheManager::ResetGroupCache()
{
    m_interpretItemCollectionResetAsFullReset = FALSE;
    m_cachedWasGrouped = m_cachedIsGrouped;
    m_metricsWereCached = m_metricsAreCached;
    m_cachedHistoryItemsCount = m_cachedItemsCount;
    m_cachedHistoryGroupInformation = std::move(m_cachedGroupInformation);

    m_cachedIsGrouped = FALSE;
    m_cachedItemsCount = 0;
    m_metricsAreCached = FALSE;
    RRETURN(S_OK);
}

_Check_return_ HRESULT ModernCollectionBasePanel::CacheManager::ResetGroupCacheIfGroupCountsMismatch()
{
    if (m_metricsAreCached && m_cachedIsGrouped)
    {
        ctl::ComPtr<IGeneratorHost> strongHost;
        IFC_RETURN(m_wrHost.As(&strongHost));
        if (strongHost)
        {
            ctl::ComPtr<ICollectionView> collectionView;
            IFC_RETURN(strongHost->get_CollectionView(&collectionView));
            if (collectionView)
            {
                ctl::ComPtr<wfc::IObservableVector<IInspectable*>> collectionGroups;
                IFC_RETURN(collectionView->get_CollectionGroups(&collectionGroups));
                if (collectionGroups)
                {
                    ctl::ComPtr<wfc::IVector<IInspectable*>> collectionGroupsAsV;
                    UINT size = 0;

                    IFC_RETURN(collectionGroups.As<wfc::IVector<IInspectable*>>(&collectionGroupsAsV));
                    IFC_RETURN(collectionGroupsAsV->get_Size(&size));

                    if (size != GetTotalGroupCount())
                    {
                        IFC_RETURN(ResetGroupCache());
                    }
                }
            }
        }
    }

    return S_OK;
}


// we expect this method to be only called for one event at a time
// Instead of going hardcore and redoing the whole cache, just take the change and update the cache
_Check_return_ HRESULT ModernCollectionBasePanel::CacheManager::RenewCacheAfterMutation(_In_ INT itemIndexAdded, _In_ INT itemIndexRemoved, _In_ INT groupIndexAdded, _In_ INT groupIndexRemoved)
{
    HRESULT hr = S_OK;
    m_cachedWasGrouped = m_cachedIsGrouped;
    m_metricsWereCached = m_metricsAreCached;
    m_cachedHistoryItemsCount = m_cachedItemsCount;
    // this should actually be pretty fast since those arrays are of the same size and std should be bitblasting this
    m_cachedHistoryGroupInformation = m_cachedGroupInformation;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spCollectionGroups;
    ctl::ComPtr<ICollectionView> spCollectionView;


    // make sure that only one index is not -1
    ASSERT(std::min(0, itemIndexAdded) + std::min(0, itemIndexRemoved) + std::min(0, groupIndexAdded) + std::min(0, groupIndexRemoved) == -3);

    // even though I asked you to give me a itemIndexAdded and Removed, really we only care about the index that was changed (so we
    // know that group is of interest)
    INT itemIndexOfInterest = std::max(itemIndexAdded, itemIndexRemoved);

#ifdef MCBP_DEBUG
    WCHAR szTrace[256];
    IFC_RETURN(swprintf_s(szTrace, 256, L"RenewCache: itemAdded %d, itemRemoved %d, groupAdded %d, groupRemoved %d", itemIndexAdded, itemIndexRemoved, groupIndexAdded, groupIndexRemoved) >= 0);
    Trace(szTrace);
#endif

    // instead of blindly iterating over all groups, lets update using the old cache

    // only need to do anything if we were already cached, otherwise the ensure will just build up everything
    if (m_metricsAreCached)
    {
        // if a group was added, we need to insert an empty cache node in the m_cachedGroupInformation list
        if (groupIndexAdded > -1)
        {
            ModernCollectionBasePanel::CacheManager::GroupCache cache;
            cache.indexOfGroup = groupIndexAdded;
            m_cachedGroupInformation.insert(m_cachedGroupInformation.begin() + groupIndexAdded, cache);
        }

        // if a group was removed, we need to remove that cache. From there, the iteration will just work as normal
        if (groupIndexRemoved > -1)
        {
            m_cachedGroupInformation.erase(m_cachedGroupInformation.begin() + groupIndexRemoved);
        }

        auto strongCache = CacheStrongRefs(&hr); // Releases when it goes out of scope at the end of method
        IFC_RETURN(hr);

        if (m_cachedIsGrouped)
        {
            bool CacheWasNotConsistent = false;

            UINT size = 0;
            UINT emptyGroupsSoFar = 0;
            ctl::ComPtr<wfc::IVector<IInspectable*>> spCollectionGroupsAsV;
            std::vector<UINT> groupSizes;
            std::vector<UINT> nonEmptyGroupSizes;
            IFC_RETURN(m_strongHost->get_CollectionView(&spCollectionView));


            // no if conditions checking the collectionview are needed. We got an ItemMutation,
            // which cannot have changed grouping
            IFC_RETURN(spCollectionView->get_CollectionGroups(&spCollectionGroups));

            IFC_RETURN(spCollectionGroups.As<wfc::IVector<IInspectable*>>(&spCollectionGroupsAsV));
            IFC_RETURN(spCollectionGroupsAsV->get_Size(&size));
            // we will actually re-do these, but not m_cachedGroupInformation, as you might notice
            m_spGroupSizes->ClearView();
            m_spNonEmptyGroupSizes->ClearView();

            // When an item is inserted, we will first have gotten the group insert, so the sizes can not have changed
            // In some cases, we might have missed the group insert/removal event.
            CacheWasNotConsistent = m_cachedGroupInformation.size() != size;

            // going to use this as a counter
            m_cachedItemsCount = 0;

            // we are going to iterate over all the groups and use the cached information until we find the group
            // that might have the added item. If the index comes at the boundary between two groups, use the actual
            // collectionviewgroup api's to get the truth!
            // Once we've seen our mutation, we need to keep updating the cache after it

            for (UINT i = 0; i < size; ++i)
            {
                ctl::ComPtr<IInspectable> spCurrent;
                ctl::ComPtr<ICollectionViewGroup> spCurrentGroup;
                ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spCurrentGroupItems;
                ctl::ComPtr<wfc::IVector<IInspectable*>> spCurrentGroupItemsAsV;

                // if the cache sizes were not consistent, we want to be able to deal with that.
                // basically we are going into 'refresh' mode. We do need to make sure we have the
                // correct caches available, so lets fix that up here.
                if (CacheWasNotConsistent && (m_cachedGroupInformation.size() == 0 || m_cachedGroupInformation.size() <= i))
                {
#ifdef MCBP_DEBUG
                    WCHAR szTrace[256];
                    IFC_RETURN(swprintf_s(szTrace, 256, L"RenewCache: adding group cache on the fly") >= 0);
                    Trace(szTrace);
#endif
                    // create a cache on the fly
                    ModernCollectionBasePanel::CacheManager::GroupCache tempcache;
                    tempcache.indexOfGroup = i;
                    m_cachedGroupInformation.push_back(tempcache);
                }

                ModernCollectionBasePanel::CacheManager::GroupCache& cache = m_cachedGroupInformation[i];
                UINT groupSize = cache.endItemIndex - cache.startItemIndex;

                // index can be either an item that was added or removed. As we are walking through the caches,
                // the basic strategy here is to actually do a (slow) api call for the group that _might_ be changed.
                // INSERT scenario:  item was added between beginning and end or item was appended to the end of the group
                // DELETE scenario:  item was removed from between and end
                //
                // empty groups: notice that there can be a few empty groups after each other and the index could be
                // part of any of those empty groups, so keep going until we find it (we _have_ to make the api calls)
                //
                // a group can be either removed or added or changed. As we are walking through the caches,
                // if a group is added or changed, we should just get the size from api. Since we have already removed
                // a cache from a group that was removed, the iteration will just work as normal for removed groups


                // notice endItemIndex is exclusive, so group 0 which has 10 items will have a start of 0, end of 10 instead of 9

                if ((cache.startItemIndex <= itemIndexOfInterest && itemIndexOfInterest <= cache.endItemIndex) ||
                    i == groupIndexAdded || CacheWasNotConsistent)
                {
                    UINT newGroupSize = 0;
                    // this is the place where we might have inserted an item and it now belongs to
                    // this group. Notice that it could also have been inserted at the beginning of the next group
                    // In both cases, we want to get the actual numbers (groupsize is what we care about)
                    IFC_RETURN(spCollectionGroupsAsV->GetAt(i, &spCurrent));
                    IFC_RETURN(spCurrent.As<ICollectionViewGroup>(&spCurrentGroup));
                    IFC_RETURN(spCurrentGroup->get_GroupItems(&spCurrentGroupItems));
                    IFC_RETURN(spCurrentGroupItems.As<wfc::IVector<IInspectable*>>(&spCurrentGroupItemsAsV));
                    IFC_RETURN(spCurrentGroupItemsAsV->get_Size(&newGroupSize));

                    groupSize = newGroupSize;
                }

                cache.startItemIndex = m_cachedItemsCount;
                cache.endItemIndex = m_cachedItemsCount + groupSize;
                cache.indexOfGroup = static_cast<INT32>(i);
                cache.nonEmptyIndexOfGroup = static_cast<INT32>(i - emptyGroupsSoFar);

                m_cachedItemsCount += groupSize;
                if (groupSize == 0)
                {
                    ++emptyGroupsSoFar;
                }
                else
                {
                    nonEmptyGroupSizes.push_back(groupSize);
                }
                groupSizes.push_back(groupSize);
            }

            m_cachedNonEmptyGroupsCount = size - emptyGroupsSoFar;
            m_spGroupSizes->SetView(std::move(groupSizes));
            m_spNonEmptyGroupSizes->SetView(std::move(nonEmptyGroupSizes));
        }
        else
        {
            // We have found that some collections do not raise the appropriate amount of events,
            // and thus a lookup is needed here always. This issue is working around a RI blocking issue on Facebook
            UINT size = 0;
            ctl::ComPtr<wfc::IVector<IInspectable*>> spView;

            IFC_RETURN(m_strongHost->get_View(&spView));
            IFC_RETURN(spView->get_Size(&size));
            m_cachedItemsCount = static_cast<INT32>(size);
        }
    }

    return S_OK;
}

_Check_return_
HRESULT
ModernCollectionBasePanel::CacheManager::GetGroupInformationFromItemIndex(
    _In_ INT32 itemIndex,
    _Out_opt_ INT32 *pIndexOfGroup,
    _Out_opt_ INT32 *pIndexInsideGroup,
    _Out_opt_ INT32 *pItemCountInGroup) const
{
    ASSERT(m_metricsAreCached);
    ASSERT(m_cachedIsGrouped && 0 <= itemIndex && itemIndex < m_cachedItemsCount);

    INT32 indexOfGroup = -1;
    INT32 indexInsideGroup = -1;
    INT32 itemCountInGroup = -1;

    if (m_cachedIsGrouped && 0 <= itemIndex && itemIndex < m_cachedItemsCount)
    {
        // very perf sensitive. notice the binary search
        auto foundGroup = std::upper_bound(begin(m_cachedGroupInformation), end(m_cachedGroupInformation), itemIndex,
            [](INT32 workingIndex, const GroupCache& current) -> bool
        {
            return (workingIndex < current.endItemIndex);
        });

        if (foundGroup != m_cachedGroupInformation.end())
        {
            itemCountInGroup = foundGroup->endItemIndex - foundGroup->startItemIndex;
            indexInsideGroup = itemIndex - foundGroup->startItemIndex;
            indexOfGroup = foundGroup->indexOfGroup;
        }
    }

    if (pIndexOfGroup)
    {
        *pIndexOfGroup = indexOfGroup;
    }
    if (pIndexInsideGroup)
    {
        *pIndexInsideGroup = indexInsideGroup;
    }
    if (pItemCountInGroup)
    {
        *pItemCountInGroup = itemCountInGroup;
    }

    RRETURN(S_OK);
}

_Check_return_
HRESULT
ModernCollectionBasePanel::CacheManager::GetGroupHistoryInformationFromItemIndex(
    _In_ INT32 itemIndex,
    _Out_opt_ INT32 *pIndexOfGroup,
    _Out_opt_ INT32 *pIndexInsideGroup,
    _Out_opt_ INT32 *pItemCountInGroup) const
{
    ASSERT(m_metricsWereCached);
    ASSERT(m_cachedWasGrouped && 0 <= itemIndex && itemIndex < m_cachedHistoryItemsCount);

    INT32 indexOfGroup = -1;
    INT32 indexInsideGroup = -1;
    INT32 itemCountInGroup = -1;

    if (m_cachedWasGrouped && 0 <= itemIndex && itemIndex < m_cachedHistoryItemsCount)
    {
        // very perf sensitive. notice the binary search
        auto foundGroup = std::upper_bound(begin(m_cachedHistoryGroupInformation), end(m_cachedHistoryGroupInformation), itemIndex,
            [](INT32 workingIndex, const GroupCache& current) -> bool
        {
            return (workingIndex < current.endItemIndex);
        });

        if (foundGroup != m_cachedHistoryGroupInformation.end())
        {
            itemCountInGroup = foundGroup->endItemIndex - foundGroup->startItemIndex;
            indexInsideGroup = itemIndex - foundGroup->startItemIndex;
            indexOfGroup = foundGroup->indexOfGroup;
        }
    }

    if (pIndexOfGroup)
    {
        *pIndexOfGroup = indexOfGroup;
    }
    if (pIndexInsideGroup)
    {
        *pIndexInsideGroup = indexInsideGroup;
    }
    if (pItemCountInGroup)
    {
        *pItemCountInGroup = itemCountInGroup;
    }

    RRETURN(S_OK);
}

_Check_return_ HRESULT ModernCollectionBasePanel::CacheManager::GetGroupInformationFromGroupIndex(_In_ INT32 groupIndex, _Out_opt_ INT32* pStartItemIndex, _Out_opt_ INT32* pItemCountInGroup) const
{
    ASSERT(m_metricsAreCached);
    ASSERT(m_cachedIsGrouped && 0 <= groupIndex && groupIndex < GetTotalGroupCount());

    INT32 startItemIndex = -1;
    INT32 itemCountInGroup = -1;

    if (0 <= groupIndex && groupIndex < GetTotalGroupCount())
    {
        const GroupCache& cache = m_cachedGroupInformation[groupIndex];
        startItemIndex = cache.startItemIndex;
        itemCountInGroup = cache.endItemIndex - startItemIndex;
    }

    if (pStartItemIndex)
    {
        *pStartItemIndex = startItemIndex;
    }
    if (pItemCountInGroup)
    {
        *pItemCountInGroup = itemCountInGroup;
    }

    RRETURN(S_OK);
}

// With things like "hiding empty groups" or "collapsing items in a group", it's often a helpful abstraction to be able to
// work in terms of "layout units", rather than "data units", where traversing a range in layout units will skip over elements that are hidden
INT32 ModernCollectionBasePanel::CacheManager::DataIndexToLayoutIndex(_In_ xaml_controls::ElementType type, _In_ INT32 index) const
{
    ASSERT(m_metricsAreCached);
    INT32 result = index;
    switch(type)
    {
    case xaml_controls::ElementType_GroupHeader:
        {
            // If empty groups are hidden, figure out how many empty groups precede this one
            //ASSERT(0 <= index && index < GetTotalGroupCount());
            if (m_hidesIfEmpty && 0 <= index && index < GetTotalGroupCount())
            {
                // Get the empty group count first, because this is far faster than querying the groupstyle
                result = m_cachedGroupInformation[index].nonEmptyIndexOfGroup;

                result = std::min(result, GetTotalLayoutGroupCount() -1);
                ASSERT(result >= 0, L"You should not be calling this when we have nothing to display");
            }

        }
        break;

    case xaml_controls::ElementType_ItemContainer:
        // Nothing to do for items... for now...
        break;

    default:
        ASSERT(0);
        break;
    }
    return result;
}

INT32 ModernCollectionBasePanel::CacheManager::LayoutIndexToDataIndex(_In_ xaml_controls::ElementType type, _In_ INT32 index) const
{
    ASSERT(m_metricsAreCached);
    INT32 result = index;
    switch(type)
    {
    case xaml_controls::ElementType_GroupHeader:
        // Find the last occurrence of "index" in the m_groupIndexIgnoringEmpties array
        // This corresponds to the nonempty group that occupies this slot in the layout units
        // Reasoning: If the next group falls in a different layout slot, it's because the target group is visible
        {
            if (m_hidesIfEmpty && GetTotalGroupCount() != GetNonEmptyGroupCount())
            {
                auto position = std::upper_bound(begin(m_cachedGroupInformation), end(m_cachedGroupInformation), index,
                    [](INT32 workingIndex, const GroupCache& arg)
                {
                    return workingIndex < arg.nonEmptyIndexOfGroup;
                });
                result = static_cast<INT32>(position - begin(m_cachedGroupInformation) -1);
            }
        }
        break;

    case xaml_controls::ElementType_ItemContainer:
        // Nothing to do for items... for now...
        break;

    default:
        ASSERT(0);
        break;
    }
    return result;
}

INT32 ModernCollectionBasePanel::CacheManager::GetTotalLayoutGroupCount() const
{
    return m_hidesIfEmpty ? GetNonEmptyGroupCount() : GetTotalGroupCount();
}

// Checks if the container or one of its descendants currently has the focus,
// in which case it cannot be recycled
// In a RunVirtualization call, we may need to recycle several items
// and as we have to check the focus for each candidate, we could
// spend a lot of time in FrameworkElement::HasFocus as it walks the
// the visual tree up starting to the focused element.
// Therefore, the first time IsFocusedChild is called in a virtualization
// cycle, we walk the tree up and cache the result
// Following calls will simply test the candidate against the cache
_Check_return_ HRESULT ModernCollectionBasePanel::CacheManager::IsFocusedChild(_In_ xaml::IUIElement *pCandidate, _Out_ BOOLEAN *pReturnValue)
{
    HRESULT hr = S_OK;

    *pReturnValue = FALSE;

    if (!m_isFocusedChildValid)
    {
        ctl::ComPtr<DependencyObject> spFocused;
        ctl::ComPtr<xaml::IDependencyObject> spCurrentAsDO;
        ctl::ComPtr<xaml::IDependencyObject> spParentAsDO;
        ctl::ComPtr<xaml::IDependencyObject> spHostAsDO;

        IFCEXPECT(m_strongChildren);
        // We walk the tree up to find a ContentControl which is an ancestor of the focused element
        // We stop either if we encounter the host or the root

        spHostAsDO = m_owner;

        IFC(spHostAsDO.Cast<DependencyObject>()->GetFocusedElement(&spFocused));
        if (!spFocused)
        {
            // No focused element
            m_isFocusedChildValid = TRUE;
        }
        else
        {
            // we walk the tree up until we find a child of the host
            IFC(spFocused.As(&spCurrentAsDO));

            while (!m_isFocusedChildValid)
            {
                IFC(VisualTreeHelper::GetParentStatic(spCurrentAsDO.Get(), &spParentAsDO));
                if (!spParentAsDO)
                {
                    // The focused control does not belong to the host
                    // We release the pointers and the Focus test will always remain "NULL"
                    m_isFocusedChildValid = TRUE;
                } else if (spParentAsDO == spHostAsDO) {
                    // Found it !
                    IFC(spCurrentAsDO.As(&m_spFocusedChild));
                    m_isFocusedChildValid = TRUE;
                } else {
                    // Parent is neither null nor host, continue up
                    spCurrentAsDO = spParentAsDO;
                }
            }
        }
    }

    *pReturnValue = (pCandidate == m_spFocusedChild.Get());

Cleanup:
    RRETURN(hr);
}

_Check_return_ ctl::ComPtr<wfc::IVectorView<UINT>> ModernCollectionBasePanel::CacheManager::GetLayoutGroupSizes() const
{
    return m_hidesIfEmpty ? m_spNonEmptyGroupSizes : m_spGroupSizes;
}

// Initialize the strong collection cache
_Check_return_ HRESULT ModernCollectionBasePanel::CacheManager::InitCollectionsCache()
{
    HRESULT hr = S_OK;

    if (m_cachedIsGrouped)
    {
        ctl::ComPtr<xaml_data::ICollectionView> spCollectionView;
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spCollectionGroups;
        IFC(m_strongHost->get_CollectionView(&spCollectionView));
        IFC(spCollectionView->get_CollectionGroups(&spCollectionGroups));
        IFC(spCollectionGroups.As<wfc::IVector<IInspectable*>>(&m_strongCollectionGroupsAsV));
        // Current Index is Invalid
        m_cachedGroupIndex = -2;
    }
    else
    {
        IFC(m_strongHost->get_View(&m_strongView));
    }
    m_isCollectionCacheValid = TRUE;

Cleanup:
    RRETURN(hr);

}

// Clear the strong collection cache
void ModernCollectionBasePanel::CacheManager::ClearCollectionsCache()
{
    m_strongCollectionGroupsAsV.Reset();
    m_strongCurrentGroup.Reset();
    m_strongView.Reset();
    m_isCollectionCacheValid = FALSE;
}

// Cache a group's interfaces
_Check_return_ HRESULT ModernCollectionBasePanel::CacheManager::CacheGroup(_In_ INT groupIndex, _In_ BOOLEAN cacheItems)
{
    if (!m_isCollectionCacheValid)
    {
        IFC_RETURN(InitCollectionsCache());
    }

    if (m_cachedIsGrouped)
    {
        if (m_cachedGroupIndex != groupIndex)
        {
            ctl::ComPtr<IInspectable> spCurrentGroupAsII;
            m_strongView.Reset();
            auto hr = m_strongCollectionGroupsAsV->GetAt(groupIndex, &spCurrentGroupAsII);
            FAIL_FAST_ASSERT(hr == S_OK);
            IFC_RETURN(spCurrentGroupAsII.As<ICollectionViewGroup>(&m_strongCurrentGroup));
            m_cachedGroupIndex = groupIndex;
        }
        // We need to get the items either if this is a new group or if items were not asked when we moved to this group
        if (cacheItems && (!m_strongView))
        {
            ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spCurrentGroupItems;
            IFC_RETURN(m_strongCurrentGroup->get_GroupItems(&spCurrentGroupItems));
            IFC_RETURN(spCurrentGroupItems.As<wfc::IVector<IInspectable*>>(&m_strongView));
        }
    }

    return S_OK;
}

// Returns the ICollectionViewGroup reference from the grouped bound source corresponding to an index
_Check_return_ HRESULT ModernCollectionBasePanel::CacheManager::GetGroup(_In_ INT groupIndex, _Outptr_ xaml_data::ICollectionViewGroup **ppGroup)
{
    HRESULT hr = S_OK;

    ASSERT(0 <= groupIndex && groupIndex < GetTotalGroupCount());

    IFC(CacheGroup(groupIndex, FALSE /* cacheItems */));
    IFC(m_strongCurrentGroup.CopyTo(ppGroup));

Cleanup:
    RRETURN(hr);
}

// Returns the IInspectable reference from the bound source corresponding to a index
_Check_return_ HRESULT ModernCollectionBasePanel::CacheManager::GetItem(_In_ INT indexInItemCollection, _Outptr_ IInspectable **ppItem)
{
    HRESULT hr = S_OK;
    INT32 correctedItemIndex = indexInItemCollection;

    ASSERT(0 <= indexInItemCollection && indexInItemCollection < m_cachedItemsCount);

    if (m_cachedIsGrouped)
    {
        INT groupIndex = 0;
        IFC(GetGroupInformationFromItemIndex(indexInItemCollection, &groupIndex, &correctedItemIndex, nullptr));
        IFC(CacheGroup(groupIndex, TRUE /* cacheItems */));
    }
    else
    {
        if (!m_isCollectionCacheValid)
        {
            IFC(InitCollectionsCache());
        }
    }
    IFCEXPECT(m_strongView);
    IFC(m_strongView->GetAt(correctedItemIndex, ppItem));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::CacheManager::GetIndex(
    _In_ IInspectable *pItem,
    _Out_ unsigned int *pIndex,
    _Out_ bool *pIsGroupIndex,
    _Out_ bool *pItemFound)
{
    HRESULT hr = S_OK;

    *pIndex = 0;
    *pIsGroupIndex = false;
    *pItemFound = false;

    if (!m_isCollectionCacheValid)
    {
        IFC(InitCollectionsCache());
    }

    if (m_cachedIsGrouped)
    {
        ctl::ComPtr<ICollectionViewGroup> spItemAsCollectionViewGroup = ctl::ComPtr<IInspectable>(pItem).AsOrNull<ICollectionViewGroup>();

        if (spItemAsCollectionViewGroup)
        {
            ctl::ComPtr<IInspectable> spGroupContext;

            spItemAsCollectionViewGroup->get_Group(&spGroupContext);

            // If this item is a CollectionViewGroup, then it represents a group header.
            // We'll just return the group index in this case.
            ctl::ComPtr<wfc::IIterable<IInspectable*>> spIterable;
            ctl::ComPtr<wfc::IIterator<IInspectable*>> spIter;

            int groupIndex = 0;
            BOOLEAN areItemsRemaining = FALSE;

            IFC(m_strongCollectionGroupsAsV.As(&spIterable));
            IFC(spIterable->First(&spIter));
            IFC(spIter->get_HasCurrent(&areItemsRemaining));

            while (areItemsRemaining)
            {
                ctl::ComPtr<IInspectable> spCurrentGroupAsII;
                ctl::ComPtr<ICollectionViewGroup> spCurrentGroup;
                ctl::ComPtr<IInspectable> spCurrentGroupContext;

                IFC(spIter->get_Current(&spCurrentGroupAsII));
                IFC(spCurrentGroupAsII.As<ICollectionViewGroup>(&spCurrentGroup));
                IFC(spCurrentGroup->get_Group(&spCurrentGroupContext));

                if (spGroupContext.Get() == spCurrentGroupContext.Get())
                {
                    *pIndex = groupIndex;
                    *pIsGroupIndex = true;
                    *pItemFound = true;

                    break;
                }
                else
                {
                    groupIndex++;
                }

                IFC(spIter->MoveNext(&areItemsRemaining));
            }
        }
        else
        {
            // If this item is *not* a CollectionViewGroup, then we'll find which group it's located in,
            // and then return its index in that group plus that group's start index,
            // since that'll get us the index to scroll to, which is what we want.
            ctl::ComPtr<wfc::IIterable<IInspectable*>> spIterable;
            ctl::ComPtr<wfc::IIterator<IInspectable*>> spIter;

            int groupIndex = 0;
            BOOLEAN areItemsRemaining = FALSE;

            IFC(m_strongCollectionGroupsAsV.As(&spIterable));
            IFC(spIterable->First(&spIter));
            IFC(spIter->get_HasCurrent(&areItemsRemaining));

            while (areItemsRemaining)
            {
                ctl::ComPtr<IInspectable> spCurrentGroupAsII;
                ctl::ComPtr<ICollectionViewGroup> spCurrentGroup;
                ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spCurrentGroupItems;
                ctl::ComPtr<wfc::IVector<IInspectable*>> spCurrentGroupItemsAsV;

                BOOLEAN itemFound = FALSE;
                unsigned int index = 0;

                IFC(spIter->get_Current(&spCurrentGroupAsII));
                IFC(spCurrentGroupAsII.As<ICollectionViewGroup>(&spCurrentGroup));
                IFC(spCurrentGroup->get_GroupItems(&spCurrentGroupItems));

                IFC(spCurrentGroupItems.As(&spCurrentGroupItemsAsV));
                IFC(spCurrentGroupItemsAsV->IndexOf(pItem, &index, &itemFound));

                if (itemFound)
                {
                    int indexCorrection = 0;

                    IFC(GetGroupInformationFromGroupIndex(groupIndex, &indexCorrection, NULL));

                    *pIndex = index + indexCorrection;
                    *pItemFound = true;

                    break;
                }
                else
                {
                    groupIndex++;
                }

                IFC(spIter->MoveNext(&areItemsRemaining));
            }
        }
    }
    else
    {
        BOOLEAN itemFound = FALSE;

        IFCEXPECT(m_strongView);
        IFC(m_strongView->IndexOf(pItem, pIndex, &itemFound));

        *pItemFound = !!itemFound;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::CacheManager::GetContainerRecyclingContext(_In_ UINT32 itemIndex, _Outptr_ DirectUI::IContainerRecyclingContext **ppContext)
{
    HRESULT hr = S_OK;
    if (m_spContainerRecyclingContext && (itemIndex != m_recyclingItemIndex))
    {
        ctl::ComPtr<IInspectable> spItem;
        IFC(GetItem(itemIndex, &spItem));
        IFC(m_spContainerRecyclingContext->PrepareForItemRecycling(spItem.Get()));
        m_recyclingItemIndex = itemIndex;
    }
    IFC(m_spContainerRecyclingContext.CopyTo(ppContext));
Cleanup:
    RRETURN(hr);
}

// Returns the index of the last header/item in the data that can be displayed in layout.
_Check_return_ HRESULT
ModernCollectionBasePanel::CacheManager::GetLastElementInLayout(
    _Out_ int* elementIndex,
    _Out_ xaml_controls::ElementType* elementType)
{
    *elementIndex = -1;
    *elementType = xaml_controls::ElementType::ElementType_ItemContainer;

    if (IsGrouping())
    {
        const int targetGroupIndex = LayoutIndexToDataIndex(
            xaml_controls::ElementType::ElementType_GroupHeader,
            GetTotalLayoutGroupCount() - 1);

        int firstItemInAnchorGroup;
        int itemCountInAnchorGroup;
        IFC_RETURN(GetGroupInformationFromGroupIndex(
            targetGroupIndex,
            &firstItemInAnchorGroup,
            &itemCountInAnchorGroup));

        if (itemCountInAnchorGroup == 0)
        {
            *elementIndex = targetGroupIndex;
            *elementType = xaml_controls::ElementType::ElementType_GroupHeader;
        }
        else
        {
            *elementIndex = firstItemInAnchorGroup + itemCountInAnchorGroup - 1;
        }
    }
    else
    {
        *elementIndex = GetTotalItemCount() - 1;
    }
    return S_OK;
}
