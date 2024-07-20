// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ModernCollectionBasePanel.g.h"
#include "ItemContainerGenerator.g.h"
#include "IGeneratorHost.g.h"
#include "CompositeTransform.g.h"
#include "SecondaryContentRelationship.g.h"
#include "ChoosingItemContainerEventArgs.g.h"
#include "ChoosingGroupHeaderContainerEventArgs.g.h"
#include "XamlTraceLogging.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

namespace DirectUI
{
    interface IContainerRecyclingContext;
    class ListViewBaseHeaderItem;

    PARTIAL_CLASS(ModernCollectionBasePanel)
        , public IModernCollectionBasePanel
    {
        // Forward declaration.
        class CollectionIterator;
    public:
        static const INT ElementType_Count = 2;

    protected:
        ModernCollectionBasePanel();
        ~ModernCollectionBasePanel() override;

        _Check_return_ HRESULT Initialize() override;

        // this base panel implementation is hidden from IDL
        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;

    public:
        IFACEMETHOD(MeasureOverride)(
            _In_ wf::Size availableSize,
            _Out_ wf::Size* pReturnValue)
            override;

        IFACEMETHOD(ArrangeOverride)(
            _In_ wf::Size arrangeSize,
            _Out_ wf::Size* pReturnValue)
            override;

        // Scrollviewer looks at its scrollbarvisibility settings and will substitute infinite
        // if those are set to visible or auto. This is an archaic design that is hard to change
        // However, certain features such as the new modern virtualizingpanels do not appreciate this.
        // They lack the extra communication that scrolldata presents to IScrollInfo implementors, so
        // in those cases we wish to go with a newer more modern approach of actually trusting layout to
        // pass in the correct values.
        BOOLEAN WantsScrollViewerToObscureAvailableSizeBasedOnScrollBarVisibility(_In_ xaml_controls::Orientation orientation) override;

        // have Itemspresenter set an origin before measure occurs so we don't have to do double invalidations
        void SetOriginFromItemsPresenter(_In_ wf::Point origin)
        {
            m_windowState.validWindowCalculation &= origin.X == m_originFromItemsPresenter.X && origin.Y == m_originFromItemsPresenter.Y;
            m_originFromItemsPresenter = origin;
        }

        // returns an array of indices of all pinned elements by type
        _Check_return_ HRESULT GetPinnedElementsIndexVector(_In_ xaml_controls::ElementType type, _Out_ std::vector<unsigned int>* pReturnValue);

    // core virtualization code
    private:
        // This method will detect the visible window, generate/recycle, measure, and pre-arrange
        _Check_return_ HRESULT RunVirtualization();

        // Determine the window to show, based on either the user scrolling, or responding to a specific request
        // such as key navigation or ScrollIntoView, and then generate the appropriate anchors for the new view
        _Check_return_ HRESULT DetermineWindowAndAnchors();

        // Measure elements in garbage section.
        _Check_return_ HRESULT MeasureElementsInGarbageSection();

        // If the layout specifies a need for aspecial group and/or item,
        // ensure these items are generated, measured, and pinned.
        _Check_return_ HRESULT MeasureSpecialElements();

        // Determines a starting element and runs Generate() both forward and backwards from that point
        _Check_return_ HRESULT RunGenerate();

        // Given a start location and a direction: measure and generate all needed elements until the window is filled in that direction
        _Check_return_ HRESULT Generate(
            _In_ CollectionIterator iterator,
            _In_ xaml_controls::LayoutReference referenceInformation,
            _In_ BOOLEAN goForward);

        // Given a start location and a direction, remove/recycle all valid containers and headers.
        // This not only trims off dangling sentinels, but also recycles all elements in that direction
        // We do this so that we don't have chunks of realized elements that weren't accounted for in the measure pass
        _Check_return_ HRESULT RemoveMeasureLeftOvers(
            _In_ CollectionIterator iterator,
            _In_ BOOLEAN goForward);

        // Arrange all elements of a given type
        _Check_return_ HRESULT ArrangeElements(
            _In_ xaml_controls::ElementType type,
            _In_ const wf::Size& finalSize,
            _In_ const wf::Rect& adjustedVisibleWindow);

        // This method works in two stages:
        // Stage 1: Determine if the new window is disconnected from either our headers or containers
        //          and figure out the closest valid elements from which to estimate
        // Stage 2: Calculate new anchor indices by estimating how many groups/items it would take for
        //          us to get from the old realized elements to the new window
        _Check_return_ HRESULT DetectAndHandleDisconnectedView();

        // Recycle containers if our recycle queue is empty and we have containers to recycle.
        _Check_return_ HRESULT EnsureRecycleContainerCandidate(_In_ INT32 index, _In_ BOOLEAN fromFront);

        // Recycle headers if our recycle queue is empty and we have headers to recycle.
        // alwaysRecycle: even if we have elements in the recyclequeue, still recycle.
        _Check_return_ HRESULT EnsureRecycleHeaderCandidate(_In_ BOOLEAN fromFront, _In_ BOOLEAN alwaysRecycle);

        // Return whether an element should be generated/placed/shown. Normally, this is always true
        // but can return false if we're hiding empty groups or collapsing groups
        bool ShouldElementBeVisible(_In_ xaml_controls::ElementType type, _In_ INT32 index) const;

        // When generating or measuring backward, this method is used when we need to set the bounds for
        // a header of a non-empty group and the first container of that group haven't been realized yet.
        _Check_return_ HRESULT EstimateFloatingHeaderLocation(_In_ ctl::ComPtr<IUIElement>& spFloatingHeader);

        // Returns whether the last element of the data source was realized.
        // If that's the case, it will also return the panel's extent.
        _Check_return_ HRESULT IsLastElementRealized(
            _Out_ BOOLEAN* pResult,
            _Out_ FLOAT* pPanelExtent);

    // Internal interface to dxaml code.
    public:
        // Scrolls the given rect into view. If another Scroll*IntoView was called
        // with asynchronous scrolling specified, the previous scroll action is forced to
        // complete synchronously with this method.
        // targetRect -         Target rectangle.
        // forceSynchronous -   if TRUE, causes the full virtualization pass (generate containers, etc)
        //                      to run and the scroll offsets to be updated before the call returns.
        //                      Otherwise, we'll invalidate measure and process the scroll in the
        //                      subsequent Measure pass.
        _Check_return_ HRESULT ScrollRectIntoView(
            _In_ wf::Rect targetRect,
            _In_ BOOLEAN forceSynchronous);

        // Scrolls the given item into view. If another Scroll*IntoView was called
        // with asynchronous scrolling specified, the previous scroll action is forced to
        // complete synchronously with this method.
        // index            -   Target index in items collection.
        // alignment        -   Desired alignment of item with viewport edges.
        // offset           -   An additional delta that get applied to the final scroll location.
        // forceSynchronous -   if TRUE, causes the full virtualization pass (generate containers, etc)
        //                      to run and the scroll offsets to be updated before the call returns.
        //                      Otherwise, we'll invalidate measure and process the scroll in the
        //                      subsequent Measure pass.
        // animate          -   if TRUE, a DirectManipulation animation is processed instead of a jump.
        _Check_return_ HRESULT ScrollItemIntoView(
            _In_ UINT index,
            _In_ xaml_controls::ScrollIntoViewAlignment alignment,
            _In_ DOUBLE offset,
            _In_ BOOLEAN forceSynchronous,
            _In_ BOOLEAN animate = FALSE);

        // Provide a focus candidate. If a scroll into view happens, the panel will
        // keep that as a focus candidate. The listview can ask for this candidate
        // when it needs to focus.
        _Check_return_ HRESULT GetFocusCandidate(
            _Out_ INT* index,
            _Out_ INT* groupIndex,
            _Out_ ctl::ComPtr<xaml::IUIElement>* pContainer);

        // Scrolls the given group header into view. If another Scroll*IntoView was called
        // with asynchronous scrolling specified, the previous scroll action is forced to
        // complete synchronously with this method.
        // index            -   Target grouped index in collection.
        // alignment        -   Desired alignment of group header with viewport edges.
        // offset           -   An additional delta that get applied to the final scroll location.
        // forceSynchronous -   if TRUE, causes the full virtualization pass (generate containers, etc)
        //                      to run and the scroll offsets to be updated before the call returns.
        //                      Otherwise, we'll invalidate measure and process the scroll in the
        //                      subsequent Measure pass.
        _Check_return_ HRESULT ScrollGroupHeaderIntoView(
            _In_ UINT index,
            _In_ xaml_controls::ScrollIntoViewAlignment alignment,
            _In_ DOUBLE offset,
            _In_ BOOLEAN forceSynchronous,
            _In_ BOOLEAN animate = FALSE,
            _In_ UINT neighboringItemIndex = 0);

        // Called when attempting to animate from the current group header (currentGroupIndex)
        // to another group header (newGroupIndex).
        // When animating down/right to a larger index, returns the last item's index in the visible
        // group prior to the target group.
        // When animating up/left to a smaller index, returns the first item's index in the target group.
        _Check_return_ HRESULT GetNeighboringItemIndex(
            _In_ UINT currentGroupIndex,
            _In_ UINT newGroupIndex,
            _Out_ UINT* pNeighboringItemIndex);

        #pragma region ICustomItemsHostPanel

        // pass through to the cache manager
        _Check_return_ HRESULT RegisterItemsHostImpl(_In_ IGeneratorHost* pHost);
        _Check_return_ HRESULT DisconnectItemsHostImpl();

        // our virtualization solution chooses to implement the mapping interfaces on the main panel,
        // since this is where the information on how to map is available.
        _Check_return_ HRESULT GetItemContainerMappingImpl(
            _Outptr_ xaml_controls::IItemContainerMapping** ppReturnValue);
        _Check_return_ HRESULT GetGroupHeaderMappingImpl(
            _Outptr_ DirectUI::IGroupHeaderMapping** ppReturnValue);

        // Recycle all headers and containers, then empty all caches and queues and invalidate Measure.
        _Check_return_ HRESULT RefreshImpl();

        // responds to a collection change on the itemssource before others look at it
        _Check_return_ HRESULT NotifyOfItemsChangingImpl(
            _In_ wfc::IVectorChangedEventArgs* e);
        // responds to a collection change on the itemssource after others look at it
        _Check_return_ HRESULT NotifyOfItemsChangedImpl(
            _In_ wfc::IVectorChangedEventArgs* e);

        // responds to a reordering. ListViewBase calls it when re-ordering happens.
        _Check_return_ HRESULT NotifyOfItemsReorderedImpl(
            _In_ UINT nCount);

        // Returns a containers iterator for CCC's incremental visualization.
        _Check_return_ HRESULT GetContainersForIncrementalVisualizationImpl(_Outptr_ DirectUI::IContainerContentChangingIterator** ppReturnValue);

        #pragma endregion

        // exposing the LayoutStrategy such that virtualizing controls that host these panels can
        // have a way to figure out the virtualization direction
        _Check_return_ HRESULT GetLayoutStrategy(_Out_ ctl::ComPtr<xaml_controls::ILayoutStrategy>* pspLayoutStrategy);

        double AdjustViewportOffsetForDeserialization(double offset);

        static bool ElementIsPositionedInGarbageSection(_In_ const ctl::ComPtr<IUIElement>& spElement);

    protected:
        // The specifics of the layout is governed by a layout strategy. Subclasses like ItemsWrapGrid
        // tell us which strategy to use via these methods.
        _Check_return_ HRESULT SetLayoutStrategyBase(_In_ const ctl::ComPtr<xaml_controls::ILayoutStrategy>& spLayoutStrategy);

        // Called when the GroupHeaderPlacement property is changed. Now we need to translate that into a
        // GroupHeaderStrategy and update the layout strategy
        _Check_return_ HRESULT SetGroupHeaderPlacement(_In_ xaml_primitives::GroupHeaderPlacement placement);

        // Only ISP supports the maintain viewport behavior.
        virtual ItemsUpdatingScrollMode GetItemsUpdatingScrollMode() const { return ItemsUpdatingScrollMode::KeepScrollOffset; }
        bool IsMaintainViewportSupportedAndEnabled() const { return GetItemsUpdatingScrollMode() != ItemsUpdatingScrollMode::KeepScrollOffset; }

        // Set the strategy's header strategy based on whether or not we're grouping.
        _Check_return_ HRESULT ReevaluateGroupHeaderStrategy();

        // called when a collection mutates.
        // Our implementation does not do anything here, just for the inheritors.
        virtual _Check_return_ HRESULT OnCollectionChangeProcessed() { return S_OK; }

        //
        // Special items overrides
        //

        virtual _Check_return_ HRESULT NeedsSpecialItem(_Out_ bool* pResult) { *pResult = false; return S_OK; }
        virtual _Check_return_ HRESULT NeedsSpecialGroup(_Out_ bool* pResult) { *pResult = false; return S_OK; }
        virtual _Check_return_ HRESULT GetSpecialItemIndex(_Out_ int* pResult) { ASSERT(false);  *pResult = -1; return E_FAIL; }
        virtual _Check_return_ HRESULT GetSpecialGroupIndex(_Out_ int* pResult) { ASSERT(false);  *pResult = -1; return E_FAIL; }
        virtual _Check_return_ HRESULT RegisterSpecialContainerSize(int itemIndex, wf::Size containerDesiredSize) { ASSERT(false); return E_FAIL; }
        virtual _Check_return_ HRESULT RegisterSpecialHeaderSize(int groupIndex, wf::Size headerDesiredSize) { ASSERT(false); return E_FAIL; }

        // IWG and ISG supports different header alignment.
        // This virtual method allows them to override their respective layout strategies.
        // In the future, other modern panels might not care about this. That's why we
        // provide a default implementation that does nothing.
        virtual _Check_return_ HRESULT SetGroupHeaderStrategy(_In_ GroupHeaderStrategy strategy) { UNREFERENCED_PARAMETER(strategy); return S_OK; }

    // core virtualization helper methods
    private:
        // Makes sure that whatever container we think is focused is still focused.
        // Otherwise, recycle it.
        _Check_return_ HRESULT VerifyStoredFocusedContainer();

        // Does the previously stored focused container still have focus?
        _Check_return_ HRESULT StoredFocusedContainerStillHasFocus(_Out_ BOOLEAN* pHasFocus);

        // If we have a stored focused container, release it back into the recycle queue.
        _Check_return_ HRESULT ReleaseStoredFocusedContainer();

        // Ensure that any pinned containers still have a reason to be pinned. If not, unpin them.
        _Check_return_ HRESULT PrunePinnedContainers();

        // move unloaded elements from unloaded queue to the visual tree.
        _Check_return_ HRESULT ReloadUnloadedElements();

    // implementation of ITreeBuilder
    public:
        _Check_return_ HRESULT IsBuildTreeSuspendedImpl(_Out_ BOOLEAN* pReturnValue);

        // perform async work. In the case of a modernpanel that is:
        // 1. increase the cache
        _Check_return_ HRESULT BuildTreeImpl(_Out_ BOOLEAN* returnValue);

        // shuts down all the work we still have to do
        // in our case that is nothing
        _Check_return_ HRESULT ShutDownDeferredWorkImpl() { RRETURN(S_OK); };

    private:
        // embodies the conditions we have for increasing the cache.
        _Check_return_ HRESULT CanIncreaseCacheLength(_Out_ BOOLEAN* pCanIncreaseCache);

    // core virtualization helper classes
    private:
        // caches to speed up access to spChildren, spIHost and so on
        class CacheManager
        {
        public:
            // nested struct describing our grouping cache
            struct GroupCache
            {
                GroupCache();
                INT32 startItemIndex;
                INT32 endItemIndex;
                INT32 indexOfGroup;
                INT32 nonEmptyIndexOfGroup;
            };

        private:
            // simple iDispose-like pattern
            class StrongRefLifetime
            {
                friend CacheManager;

            public:
                ~StrongRefLifetime() {if (m_pOwner) m_pOwner->UnguaranteeCache(); }

                // Moveability and returnability
                StrongRefLifetime(_Inout_ StrongRefLifetime&& other) : m_pOwner(other.m_pOwner) {other.m_pOwner = nullptr;}
                StrongRefLifetime& operator=(_Inout_ StrongRefLifetime&& other)
                {
                    if (this != &other) {m_pOwner = other.m_pOwner; other.m_pOwner = nullptr;}
                    return *this;
                }

            private:
                StrongRefLifetime(_In_ CacheManager* pOwner) : m_pOwner(pOwner) {}
                // Explicitly disable copying
                StrongRefLifetime(const StrongRefLifetime& other);
                StrongRefLifetime& operator=(const StrongRefLifetime& other);

                CacheManager* m_pOwner;
            };

        public:
            // the object is intended to be created just once (not per cache session)
            CacheManager(_In_ ModernCollectionBasePanel* owner);
            ~CacheManager();

            // Gets an instance of the lifetime manager for caches
            StrongRefLifetime CacheStrongRefs(_Out_ HRESULT* hr);

            // start a cache session. Some information can stay relevant after the session
            // while in Guaranteed-state, the manager will not doubt the invariants like GeneratorHost, spChildren, etc.
            _Check_return_ HRESULT GuaranteeCache();
            // the manager will clear out some information it has relied on in the past (GeneratorHost, spChildren)
            void UnguaranteeCache();

            // Reset the cached group info
            // This is very impactful, only do this if you need to
            _Check_return_ HRESULT ResetGroupCache();

            // Check if the group count in the cache is the same as in the CVS. If different, then reset the cache
            _Check_return_ HRESULT ResetGroupCacheIfGroupCountsMismatch();

            // Trying to limit the amount of api that we expose by collapsing all indices. You can only pass one actual index, the rest should be -1
            _Check_return_ HRESULT RenewCacheAfterMutation(_In_ INT itemIndexAdded, _In_ INT itemIndexRemoved, _In_ INT groupIndexAdded, _In_ INT groupIndexRemoved);

            // store the itemshost
            _Check_return_ HRESULT RegisterItemsHost(_In_ const ctl::ComPtr<IGeneratorHost>& spHost) { RRETURN(ctl::AsWeak(spHost.Get(), &m_wrHost)); }
            _Check_return_ HRESULT DisconnectItemsHost() { m_wrHost.Reset(); RRETURN(S_OK); }
            _Check_return_ BOOLEAN IsItemsHostRegistered() { return m_wrHost.AsOrNull<IGeneratorHost>().Get() != NULL; }

            // Getters for expensive things
            _Check_return_ HRESULT GetItemsHost(_Out_ ctl::ComPtr<IGeneratorHost>* pspItemsHost) const;
            _Check_return_ HRESULT GetChildren(_Out_ ctl::ComPtr<wfc::IVector<xaml::UIElement*>>* pspChildren) const;

            _Check_return_ BOOLEAN GetHidesIfEmpty() const { return m_hidesIfEmpty; }
            void PutHidesIfEmpty(_In_ BOOLEAN hidesIfEmpty) { m_hidesIfEmpty = hidesIfEmpty; }

            INT32 GetTotalItemCount() const { return m_cachedItemsCount; }
            INT32 GetTotalGroupCount() const { return m_cachedGroupInformation.size(); }
            INT32 GetNonEmptyGroupCount() const { return m_cachedNonEmptyGroupsCount; }
            _Check_return_ BOOLEAN IsGrouping() const { return m_cachedIsGrouped; }
            _Check_return_ BOOLEAN WasGrouping() const { return m_cachedWasGrouped; }

            _Check_return_ HRESULT GetGroupInformationFromItemIndex(_In_ INT32 itemIndex, _Out_opt_ INT32 *pIndexOfGroup, _Out_opt_ INT32 *pIndexInsideGroup, _Out_opt_ INT32 *pItemCountInGroup) const;
            _Check_return_ HRESULT GetGroupHistoryInformationFromItemIndex(_In_ INT32 itemIndex, _Out_opt_ INT32 *pIndexOfGroup, _Out_opt_ INT32 *pIndexInsideGroup, _Out_opt_ INT32 *pItemCountInGroup) const;
            _Check_return_ HRESULT GetGroupInformationFromGroupIndex(_In_ INT32 groupIndex, _Out_opt_ INT32* pStartItemIndex, _Out_opt_ INT32* pItemCountInGroup) const;

            // With things like "hiding empty groups" or "collapsing items in a group", it's often a helpful abstraction to be able to
            // work in terms of "layout units", rather than "data units", where traversing a range in layout units will skip over elements that are hidden
            INT32 DataIndexToLayoutIndex(_In_ xaml_controls::ElementType type, _In_ INT32 index) const;
            INT32 LayoutIndexToDataIndex(_In_ xaml_controls::ElementType type, _In_ INT32 index) const;
            INT32 GetTotalLayoutGroupCount() const;

            // register that we want to interpret the reset event in a certain way
            void RegisterInterpretResetAsFullReset() { ASSERT(m_interpretItemCollectionResetAsFullReset == FALSE); m_interpretItemCollectionResetAsFullReset = TRUE; }
            BOOLEAN ShouldInterpretResetAsFullReset() { return m_interpretItemCollectionResetAsFullReset; }

            _Check_return_ ctl::ComPtr<wfc::IVectorView<UINT>> GetLayoutGroupSizes() const;

            // Checks if the container or one of its descendants currently has the focus,
            // in which case it cannot be recycled
            // In a RunVirtualization call, we may need to recycle several items
            // and as we have to check the focus for each candidate, we could
            // spend a lot of time in FrameworkElement::HasFocus as it walks the
            // the visual tree up starting to the focused element.
            // Therefore, the first time IsFocusedChild is called in a virtualization
            // cycle, we walk the tree up and cache the result
            // Following calls will simply test the candidate against the cache
            _Check_return_ HRESULT IsFocusedChild(_In_ xaml::IUIElement *pCandidate, _Out_ BOOLEAN *pReturnValue);

            // Returns the ICollectionViewGroup reference from the grouped bound source corresponding to an index
            _Check_return_ HRESULT GetGroup(_In_ INT groupIndex, _Outptr_ xaml_data::ICollectionViewGroup **ppGroup);
            // Returns the IInspectable reference from the bound source corresponding to a index
            _Check_return_ HRESULT GetItem(_In_ INT indexInItemCollection, _Outptr_ IInspectable **ppItem);
            // Returns the index from the bound source corresponding to an IInspectable reference
            _Check_return_ HRESULT GetIndex(_In_ IInspectable *pItem, _Out_ unsigned int *pIndex, _Out_ bool *pIsGroupIndex, _Out_ bool *pItemFound);

            _Check_return_ HRESULT GetContainerRecyclingContext(_In_ UINT32 itemIndex, _Outptr_ DirectUI::IContainerRecyclingContext **ppContext);

            // Returns the index of the last header/item in the data that can be displayed in layout.
            _Check_return_ HRESULT GetLastElementInLayout(
                _Out_ int* elementIndex,
                _Out_ xaml_controls::ElementType* elementType);

        private:
            BOOLEAN m_interpretItemCollectionResetAsFullReset;
            BOOLEAN m_metricsAreCached;
            BOOLEAN m_metricsWereCached;

            std::vector<GroupCache> m_cachedGroupInformation;
            std::vector<GroupCache> m_cachedHistoryGroupInformation;
            BOOLEAN m_cachedIsGrouped;
            BOOLEAN m_cachedWasGrouped;
            INT32 m_cachedItemsCount;
            INT32 m_cachedHistoryItemsCount;
            INT32 m_cachedNonEmptyGroupsCount;

            // Quick and dirty vector-of-int to avoid plumbing GroupCache or similar across ILayoutStrategy

            ctl::ComPtr<DirectUI::ValueTypeView<UINT>> m_spGroupSizes;
            ctl::ComPtr<DirectUI::ValueTypeView<UINT>> m_spNonEmptyGroupSizes;

            // reference back to the host
            ctl::WeakRefPtr m_wrHost;

            // resolved references, only valid during guaranteed sessions
            ctl::ComPtr<IGeneratorHost> m_strongHost;
            ctl::ComPtr<wfc::IVector<xaml::UIElement*>> m_strongChildren;

            // Context for recycling: only valid during VirtualizationPhase
            ctl::ComPtr<IContainerRecyclingContext> m_spContainerRecyclingContext;
            UINT32 m_recyclingItemIndex;

            // Indicates that the visual tree has been walked in the current virtualization cycle
            // to check if a container or one of its descendants has the focus
            // (see IsFocusedChild)
            BOOLEAN m_isFocusedChildValid;
            // container which has the focus (or one of its descendants)
            ctl::ComPtr<xaml::IUIElement> m_spFocusedChild;

            // Cache to accelerate GetGroup and GetItem methods
            BOOLEAN m_isCollectionCacheValid;
            // When grouped, the vector of groups
            ctl::ComPtr<wfc::IVector<IInspectable*>> m_strongCollectionGroupsAsV;
            // Index of the cached group
            INT m_cachedGroupIndex;
            // cached group strong reference
            ctl::ComPtr<xaml_data::ICollectionViewGroup> m_strongCurrentGroup;
            // When grouped, the vector containing items of the current group, when not group, the items themselves
            ctl::ComPtr<wfc::IVector<IInspectable*>> m_strongView;

            // Caching HidesIfEmpty
            BOOLEAN m_hidesIfEmpty;

            // backtalk
            ModernCollectionBasePanel* const m_owner;

            // Initialize the strong collection cache
            _Check_return_ HRESULT InitCollectionsCache();
            // Clear the strong collection cache
            void ClearCollectionsCache();
            // Cache a group's interfaces
            _Check_return_ HRESULT CacheGroup(_In_ INT groupIndex, _In_ BOOLEAN cacheItems);
        };

        // provides data information for the layout strategies.
        class LayoutDataInfoProvider
            : public xaml_controls::ILayoutDataInfoProvider
            , public ctl::ComBase
        {
        public:
            LayoutDataInfoProvider();

            _Check_return_ HRESULT Initialize(_In_ IModernCollectionBasePanel *pPanel);
            void Uninitialize();

            // ILayoutDataInfoProvider methods.
            IFACEMETHOD(GetTotalItemCount)(_Out_ INT* pReturnValue) override;
            IFACEMETHOD(GetTotalGroupCount)(_Out_ INT* pReturnValue) override;
            IFACEMETHOD(GetGroupInformationFromItemIndex)(_In_ INT itemIndex, _Out_ INT* pIndexOfGroup, _Out_ INT* pIndexInsideGroup, _Out_ INT* pItemCountInGroup) override;
            IFACEMETHOD(GetGroupInformationFromGroupIndex)(_In_ INT groupIndex, _Out_ INT* pStartItemIndex, _Out_ INT* pItemCountInGroup) override;

        protected:
            _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppInterface) override;

        private:
            _Check_return_ HRESULT GetPanelRef(_Outptr_ IModernCollectionBasePanel **ppMCBP);

            // Raw reference to the modern collection base panel to avoid a cyclical reference.
            // MCBP unset this in its destructor.
            IModernCollectionBasePanel* _pMCBP;
        };

        // Iterates over containers for CCC's incremental visualization.
        class ContainerContentChangingIterator
            : public IContainerContentChangingIterator
            , public ctl::ComBase
        {
        public:
            ContainerContentChangingIterator();

            _Check_return_ HRESULT Initialize(_In_ ModernCollectionBasePanel *pPanel);

            IFACEMETHOD(get_Size)(_Out_ UINT* pValue) override;
            IFACEMETHOD(get_Current)(_Outptr_result_maybenull_ xaml::IDependencyObject** ppValue) override;
            IFACEMETHOD(MoveNext)(_Out_ BOOLEAN* pReturnValue) override;
            IFACEMETHOD(Reset)() override;

        protected:
            _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppInterface) override;

        private:
            // Raw reference because the panel keeps this alive.
            ModernCollectionBasePanel* m_pMCBP;

            xaml_controls::PanelScrollingDirection m_direction;
            INT m_visibleStartInVector;
            INT m_visibleEndInVector;
            INT m_cacheEndInVector;
            INT m_currentIndex;
        };

        // manages containers in the children collection

        // Some notes on terminology.
        // Child index      - Index within Panel.Children.
        // Valid index      - Specific to type (header or container). With this in mind,
        //                    it's an index within the list of that particular type. I.e.,
        //                    container valid index 0 is the first realized container, and
        //                    header valid index 0 is the first realized header.
        // Item index       - Index in the ItemsSource.

        // A quick diagram showing layout in Children relative to our internal fields,
        // arranged in increasing index.

        // Index                    Significance
        // 0                        First realized header.
        // ...                      Bunch of realized headers.
        // m_validHeadersCount      First realized _CONTAINER_
        // ...                      Bunch of realized containers.
        // m_validHeadersCount+\
        //  m_validContainerCount   Start of garbage.
        // Pinned containers are interspersed inside the Children collection (potentially in the garbage section).

        class ContainerManager
        {
            // pinned items: containers we want to track the index of
        public:
            class PinnedElementInfo
            {
            public:
                PinnedElementInfo() = default;
                explicit PinnedElementInfo(_Inout_ ctl::WeakRefPtr&& wrElement, _In_ INT index);
                explicit PinnedElementInfo(_In_ const ctl::WeakRefPtr& wrElement, _In_ INT index);

                INT GetIndex() const { ASSERT(m_index > -1 || !m_wrElement); return m_index; }
                const ctl::ComPtr<xaml::IUIElement> GetElement() const { return ctl::WeakRefPtr(m_wrElement).AsOrNull<IUIElement>(); }
                void UpdateIndex(_In_ INT index) { m_index = index; }
                void UpdateElement(_In_ ctl::WeakRefPtr&& wrElement) { m_wrElement = std::move(wrElement); }

            private:
                INT m_index = -1;
                ctl::WeakRefPtr m_wrElement;
            };

        public:
            ContainerManager(_In_ ModernCollectionBasePanel* owner);
            ~ContainerManager();

            // Clear Pinning and Focus
            void ClearPinAndFocus();
            // Clear all realized headers and containers (pinned or not).
            _Check_return_ HRESULT Refresh();
            // Re do indices, like a less destructive refresh
            _Check_return_ HRESULT Reset();

            // Element management and translation.

            // Given an the index of a valid element (i.e., 0 is always the first container in the valid range), return the element.
            _Check_return_ HRESULT GetHeaderAtValidIndex(_In_ INT32 indexInValidHeaders, _Out_ ctl::ComPtr<xaml::IUIElement>* pspHeader) const
            { RRETURN(GetElementAtValidIndex(xaml_controls::ElementType_GroupHeader, indexInValidHeaders, pspHeader)); }
            _Check_return_ HRESULT GetContainerAtValidIndex(_In_ INT32 indexInValidContainers, _Out_ ctl::ComPtr<xaml::IUIElement>* pspContainer) const
            { RRETURN(GetElementAtValidIndex(xaml_controls::ElementType_ItemContainer, indexInValidContainers, pspContainer)); }
            _Check_return_ HRESULT GetElementAtValidIndex(_In_ xaml_controls::ElementType type, _In_ INT32 indexInValidElements, _Out_ ctl::ComPtr<xaml::IUIElement>* pspElement) const;

            // Given a data index, return the element - could be null if unrealized
            _Check_return_ HRESULT GetHeaderAtGroupIndex(_In_ INT32 groupIndex, _Out_ ctl::ComPtr<xaml::IUIElement>* pspHeader) const
            { RRETURN(GetElementAtDataIndex(xaml_controls::ElementType_GroupHeader, groupIndex, pspHeader)); }
            _Check_return_ HRESULT GetContainerAtItemIndex(_In_ INT32 itemIndex, _Out_ ctl::ComPtr<xaml::IUIElement>* pspContainer) const
            { RRETURN(GetElementAtDataIndex(xaml_controls::ElementType_ItemContainer, itemIndex, pspContainer)); }
            _Check_return_ HRESULT GetElementAtDataIndex(_In_ xaml_controls::ElementType type, _In_ INT32 dataIndex, _Out_ ctl::ComPtr<xaml::IUIElement>* pspElement) const;

            // Gets the number of realized elements.
            INT32 GetValidHeaderCount() const { return GetValidElementCount(xaml_controls::ElementType_GroupHeader); }
            INT32 GetValidContainerCount() const { return GetValidElementCount(xaml_controls::ElementType_ItemContainer); }
            INT32 GetValidElementCount(_In_ xaml_controls::ElementType type) const { return static_cast<INT32>(m_validElements[type].size()); }

            // Translate a valid element index to an index in Items(Source).
            INT32 GetGroupIndexFromValidIndex(_In_ INT32 indexInValidHeaders) const { return GetDataIndexFromValidIndex(xaml_controls::ElementType_GroupHeader, indexInValidHeaders); }
            INT32 GetItemIndexFromValidIndex(_In_ INT32 indexInValidContainers) const { return GetDataIndexFromValidIndex(xaml_controls::ElementType_ItemContainer, indexInValidContainers); }
            INT32 GetDataIndexFromValidIndex(_In_ xaml_controls::ElementType type, _In_ INT32 indexInValidElements) const
            { return indexInValidElements + m_indicesOfFirstValidElement[type]; }

            // Translate an index in data Items(Source) to a "valid" element index.
            // Note that the index may not be really valid in the classic sense,
            // as the element may not actually be realized. Check it with
            // IsValidElementIndexWithinBounds.
            INT32 GetValidHeaderIndexFromGroupIndex(_In_ INT32 groupIndex) const
            { return GetValidElementIndexFromDataIndex(xaml_controls::ElementType_GroupHeader, groupIndex); }
            INT32 GetValidContainerIndexFromItemIndex(_In_ INT32 indexInItemCollection) const
            { return GetValidElementIndexFromDataIndex(xaml_controls::ElementType_ItemContainer, indexInItemCollection); }
            INT32 GetValidElementIndexFromDataIndex(_In_ xaml_controls::ElementType type, _In_ INT32 dataIndexInCollection) const
            {
                INT32 indexOfFirstValidElement = m_indicesOfFirstValidElement[type];
                ASSERT (indexOfFirstValidElement >= -1);
                return indexOfFirstValidElement == -1 ? indexOfFirstValidElement : dataIndexInCollection - indexOfFirstValidElement;
            }

            // Do we actually have a realized element with this valid element index?
            bool IsValidContainerIndexWithinBounds(_In_ INT32 indexInValidContainers) const { return IsValidElementIndexWithinBounds(xaml_controls::ElementType_ItemContainer, indexInValidContainers); }
            bool IsValidHeaderIndexWithinBounds(_In_ INT32 indexInValidHeaders) const { return IsValidElementIndexWithinBounds(xaml_controls::ElementType_GroupHeader, indexInValidHeaders); }
            bool IsValidElementIndexWithinBounds(_In_ xaml_controls::ElementType type, _In_ INT32 indexInValidElements) const
            { return indexInValidElements >= 0 && indexInValidElements < GetValidElementCount(type); }

            bool IsItemIndexWithinValidRange(_In_ INT32 itemIndex) const
            { return IsValidContainerIndexWithinBounds(GetValidContainerIndexFromItemIndex(itemIndex)); }
            bool IsGroupIndexWithinValidRange(_In_ INT32 groupIndex) const
            { return IsValidHeaderIndexWithinBounds(GetValidHeaderIndexFromGroupIndex(groupIndex)); }
            bool IsDataIndexWithinValidRange(_In_ xaml_controls::ElementType type, _In_ INT32 dataIndex) const
            { return IsValidElementIndexWithinBounds(type, GetValidElementIndexFromDataIndex(type, dataIndex)); }

            // Returns True if there is a realized item in the valid collection for the provided index.
            bool IsItemConnected(_In_ UINT index) const;

            // Returns True if the group header for the provided index is connected.
            bool IsGroupHeaderConnected(_In_ UINT groupIndex, _In_ UINT neighboringItemIndex) const;

            // Returns True if there is a realized group header for the provided index.
            bool IsGroupHeaderRealized(_In_ UINT groupIndex) const;

            // Inform the ContainerManager that the data index of the first realized element has changed
            // to the given value.
            void UpdateGroupIndexForFirstValidHeader(_In_ INT32 groupIndexOfFirstValidHeader) { UpdateDataIndexForFirstValidElement(xaml_controls::ElementType_GroupHeader, groupIndexOfFirstValidHeader); }
            void UpdateItemIndexForFirstValidContainer(_In_ INT32 itemIndexOfFirstValidContainer) { UpdateDataIndexForFirstValidElement(xaml_controls::ElementType_ItemContainer, itemIndexOfFirstValidContainer); }
            void UpdateDataIndexForFirstValidElement(_In_ xaml_controls::ElementType type, _In_ INT32 dataIndexOfFirstValidElement);

            // Given the element and its index in the Items(Source), add the element (or move, if it's a pinned item/group)
            // to the proper place in the Children collection. Also updates the element's virtualization info to reflect
            // that it is realized.
            _Check_return_ HRESULT PlaceInValidHeaders(_In_ INT32 groupIndex, _In_ const ctl::ComPtr<IUIElement>& spHeader)
            { RRETURN(PlaceInValidElements(xaml_controls::ElementType_GroupHeader, groupIndex, spHeader)); }
            _Check_return_ HRESULT PlaceInValidContainers(_In_ INT32 itemIndex, _In_ const ctl::ComPtr<IUIElement>& spContainer)
            { RRETURN(PlaceInValidElements(xaml_controls::ElementType_ItemContainer, itemIndex, spContainer)); }
            _Check_return_ HRESULT PlaceInValidElements(_In_ xaml_controls::ElementType type, _In_ INT32 dataIndex, _In_ const ctl::ComPtr<IUIElement>& spElement);

            // Insert a placeholder sentinel into the valid range
            _Check_return_ HRESULT InsertSentinelContainer(_In_ INT32 itemIndex) { RRETURN(InsertSentinelElement(xaml_controls::ElementType_ItemContainer, itemIndex)); }
            _Check_return_ HRESULT InsertSentinelHeader(_In_ INT32 groupIndex) { RRETURN(InsertSentinelElement(xaml_controls::ElementType_GroupHeader, groupIndex)); }
            _Check_return_ HRESULT InsertSentinelElement(_In_ xaml_controls::ElementType type, _In_ INT32 groupIndex);

            // Removes an element from the valid range. It is now considered to be junked and we are probably calling this because we are recycling.
            // In some cases we are removing a item/group from the itemcollection and we do not want to update the dataindexFromFirstValidElement (itemBeingRemoved).
            // Generated elements get moved to the junk sections; ungenerated elements get removed from the children.
            // If specified, gateIndex represents the limit of ungenerated elements removal: it is used to ensure that the element being realized
            // does not get disjoint from the range of elements after this cleanup phase
            _Check_return_ HRESULT RemoveFromValidHeaders(_In_ INT32 indexInValidHeaders, _In_ BOOLEAN isForGroupRemoval, _In_ INT32 gateIndex = -1)
            { RRETURN(RemoveFromValidElements(xaml_controls::ElementType_GroupHeader, indexInValidHeaders, isForGroupRemoval, gateIndex)); }
            _Check_return_ HRESULT RemoveFromValidContainers(_In_ INT32 indexInValidContainers, _In_ BOOLEAN isForItemRemoval, _In_ INT32 gateIndex = -1)
            { RRETURN(RemoveFromValidElements(xaml_controls::ElementType_ItemContainer, indexInValidContainers, isForItemRemoval, gateIndex)); }
            _Check_return_ HRESULT RemoveFromValidElements(_In_ xaml_controls::ElementType type, _In_ INT32 indexInValidElements, _In_ BOOLEAN isForDataRemoval, _In_ INT32 gateIndex);

            // Mark all valid elements as junked
            _Check_return_ HRESULT RemoveAllValidHeaders()
            { RRETURN(RemoveAllValidElements(xaml_controls::ElementType_GroupHeader)); }
            _Check_return_ HRESULT RemoveAllValidContainers()
            { RRETURN(RemoveAllValidElements(xaml_controls::ElementType_ItemContainer)); }
            _Check_return_ HRESULT RemoveAllValidElements(_In_ xaml_controls::ElementType type);

            // place in garbage
            _Check_return_ HRESULT PlaceInGarbageRegionIfNotInChildren(_In_ const ctl::ComPtr<IUIElement>& spElement);

            // IICM and IGHM implementation
            _Check_return_ HRESULT IICM_ContainerFromItem(_In_opt_ IInspectable* item, _Outptr_ xaml::IDependencyObject** ppReturnValue)
            { RRETURN(Interface_ElementFromDataItem(xaml_controls::ElementType_ItemContainer, item, ppReturnValue)); }
            _Check_return_ HRESULT IGHM_HeaderFromGroup(_In_ IInspectable* group, _Outptr_ xaml::IDependencyObject** ppReturnValue)
            { RRETURN(Interface_ElementFromDataItem(xaml_controls::ElementType_GroupHeader, group, ppReturnValue)); }
            _Check_return_ HRESULT Interface_ElementFromDataItem(_In_ xaml_controls::ElementType type, _In_opt_ IInspectable* dataItem, _Outptr_ xaml::IDependencyObject** returnValue);

            _Check_return_ HRESULT IICM_IndexFromContainer(_In_ xaml::IDependencyObject* container, _Out_ INT* pReturnValue)
            { RRETURN(Interface_IndexFromElement(xaml_controls::ElementType_ItemContainer, container, FALSE /*excludeHiddenEmptyGroups*/, pReturnValue)); }
            _Check_return_ HRESULT IGHM_IndexFromHeader(_In_ xaml::IDependencyObject* header, _In_ BOOLEAN excludeHiddenEmptyGroups, _Out_ INT* pReturnValue)
            { RRETURN(Interface_IndexFromElement(xaml_controls::ElementType_GroupHeader, header, excludeHiddenEmptyGroups, pReturnValue)); }
            _Check_return_ HRESULT Interface_IndexFromElement(_In_ xaml_controls::ElementType type, _In_ xaml::IDependencyObject* element, _In_ BOOLEAN excludeHiddenEmptyGroups, _Out_ INT* pReturnValue);

            _Check_return_ HRESULT IICM_ContainerFromIndex(_In_ INT index, _Outptr_ xaml::IDependencyObject** ppReturnValue)
            { RRETURN(Interface_ElementFromDataIndex(xaml_controls::ElementType_ItemContainer, index, ppReturnValue)); }
            _Check_return_ HRESULT IGHM_HeaderFromIndex(_In_ INT index, _Outptr_ xaml::IDependencyObject** ppReturnValue)
            { RRETURN(Interface_ElementFromDataIndex(xaml_controls::ElementType_GroupHeader, index, ppReturnValue)); }
            _Check_return_ HRESULT Interface_ElementFromDataIndex(_In_ xaml_controls::ElementType type, _In_ INT index, _Outptr_ xaml::IDependencyObject** ppReturnValue);

            _Check_return_ HRESULT IICM_ItemFromContainer(_In_ xaml::IDependencyObject* container, _Outptr_ IInspectable** ppReturnValue)
            { RRETURN(Interface_DataItemFromElement(xaml_controls::ElementType_ItemContainer, container, ppReturnValue)); }
            _Check_return_ HRESULT IGHM_GroupFromHeader(_In_ xaml::IDependencyObject* header, _Outptr_ IInspectable** ppReturnValue)
            { RRETURN(Interface_DataItemFromElement(xaml_controls::ElementType_GroupHeader, header, ppReturnValue)); }
            _Check_return_ HRESULT Interface_DataItemFromElement(_In_ xaml_controls::ElementType type, _In_ xaml::IDependencyObject* element, _Outptr_ IInspectable** ppReturnValue);

            _Check_return_ HRESULT IICM_GroupHeaderContainerFromItemContainerImpl(_In_ xaml::IDependencyObject* pItemContainer, _Outptr_result_maybenull_ xaml::IDependencyObject** ppReturnValue)
            { RRETURN(Interface_GroupHeaderContainerFromItemContainerImpl(pItemContainer, ppReturnValue)); }
            _Check_return_ HRESULT Interface_GroupHeaderContainerFromItemContainerImpl(_In_ xaml::IDependencyObject* pItemContainer, _Outptr_result_maybenull_ xaml::IDependencyObject** ppReturnValue);

            // Pinned Containers & Headers
            // These are containers or headers that are not necessarily part of the valid range,
            // but must be kept around for various reasons.

            // Return a vector of indices of all pinned elements
            _Check_return_ HRESULT GetContainerPinnedElementsIndexVector(_Out_ std::vector<unsigned int>* pReturnValue)
            { RRETURN(GetPinnedElementsIndexVector(xaml_controls::ElementType_ItemContainer, pReturnValue)); }
            _Check_return_ HRESULT GetHeaderHeaderPinnedElementsIndexVector(_Out_ std::vector<unsigned int>* pReturnValue)
            { RRETURN(GetPinnedElementsIndexVector(xaml_controls::ElementType_GroupHeader, pReturnValue)); }
            _Check_return_ HRESULT GetPinnedElementsIndexVector(_In_ xaml_controls::ElementType type, _Out_ std::vector<unsigned int>* pReturnValue);

            // Given an element index in the Items(Source), grab the actual pinned element (if it is actually pinned).
            _Check_return_ HRESULT GetHeaderFromPinnedHeaders(_In_ INT index, _Out_ ctl::ComPtr<xaml::IUIElement>* pspHeader) const
            { RRETURN(GetElementFromPinnedElements(xaml_controls::ElementType_GroupHeader, index, pspHeader)); }
            _Check_return_ HRESULT GetContainerFromPinnedContainers(_In_ INT index, _Out_ ctl::ComPtr<xaml::IUIElement>* pspContainer) const
            { RRETURN(GetElementFromPinnedElements(xaml_controls::ElementType_ItemContainer, index, pspContainer)); }
            _Check_return_ HRESULT GetElementFromPinnedElements(_In_ xaml_controls::ElementType type, _In_ INT index, _Out_ ctl::ComPtr<xaml::IUIElement>* pspElement) const;

            // Given a data index in the Items(Source) and the element, add it to the pinned elements.
            _Check_return_ HRESULT RegisterPinnedHeader(_In_ INT index, _In_ const ctl::ComPtr<xaml::IUIElement>& spHeader)
            { RRETURN(RegisterPinnedElement(xaml_controls::ElementType_GroupHeader, index, spHeader)); }
            _Check_return_ HRESULT RegisterPinnedContainer(_In_ INT index, _In_ const ctl::ComPtr<xaml::IUIElement>& spContainer)
            { RRETURN(RegisterPinnedElement(xaml_controls::ElementType_ItemContainer, index, spContainer)); }
            _Check_return_ HRESULT RegisterPinnedElement(_In_ xaml_controls::ElementType type, _In_ INT index, _In_ const ctl::ComPtr<xaml::IUIElement>& spElement);

            // Given a data index in the Items(Source), remove the corresponding element from the pinned elements (if present).
            _Check_return_ HRESULT UnregisterPinnedHeader(_In_ INT index)
            { RRETURN(UnregisterPinnedElement(xaml_controls::ElementType_GroupHeader, index)); }
            _Check_return_ HRESULT UnregisterPinnedContainer(_In_ INT index)
            { RRETURN(UnregisterPinnedElement(xaml_controls::ElementType_ItemContainer, index)); }
            _Check_return_ HRESULT UnregisterPinnedElement(_In_ xaml_controls::ElementType type, _In_ INT index);

            // Keep only the pinned containers that are approved by the given filter function.
            // The function takes the index and container as arguments, and should return TRUE for pinned containers to keep.
            _Check_return_ HRESULT FilterPinnedContainers(_In_ std::function<HRESULT (INT, xaml::IUIElement*, BOOLEAN*)> filterFunction);

            // Our API here has a wart around the focused container. The focused container acts as if it is pinned, but when that focused element is cleared,
            // we don't preserve its pinned state. This only really matters for special containers, so this method is here to force the container to be really pinned
            // regardless of whether or not it's the focused element.
            // TODO: This method will likely become superfluous if we move focus code into the GeneratorHost.
            // See ModernCollectionBasePanel::EnsureRecycleContainerCandidate for more details.
            _Check_return_ HRESULT EnsureContainerPinned(_In_ INT index, _In_ const ctl::ComPtr<xaml::IUIElement>& spContainer);

            // Is the given container pinned?
            // Is the given header pinned?
            bool GetIsHeaderPinned(_In_ const ctl::ComPtr<xaml::IUIElement>& spHeader) const
            { RRETURN(GetIsElementPinned(xaml_controls::ElementType_GroupHeader, spHeader)); }
            bool GetIsContainerPinned(_In_ const ctl::ComPtr<xaml::IUIElement>& spContainer) const
            { RRETURN(GetIsElementPinned(xaml_controls::ElementType_ItemContainer, spContainer)); }
            bool GetIsElementPinned(_In_ xaml_controls::ElementType type, _In_ const ctl::ComPtr<xaml::IUIElement>& spElement, _Out_opt_ INT* pPinnedIndex = nullptr) const;

            bool GetIsIndexPinned(_In_ xaml_controls::ElementType type, _In_ INT index) const;

            // Temporary holding for elements being prepared
            void HoldForPrepare(_In_ ctl::ComPtr<IUIElement> spElement, _In_ xaml_controls::ElementType type, _In_ INT index);
            void ResetAfterPrepare();

            // focus
            // store a container/index pair. Can only occur if there is no previously focused container (will fail if there is)
            _Check_return_ HRESULT SetFocusedContainer(_In_ INT itemIndex, _In_ const ctl::ComPtr<xaml::IUIElement>& spContainer);
            _Check_return_ HRESULT GetFocusedContainer(_Out_ INT* pItemIndex, _Out_ ctl::ComPtr<xaml::IUIElement>* pspContainer) const;
            _Check_return_ HRESULT ResetFocusedContainer(_In_ BOOLEAN removeNonGeneratedContainer);
            bool HasFocusedContainer() { return m_focusedContainer.GetIndex() > -1; };

            _Check_return_ HRESULT SetFocusedHeaderContainer(_In_ INT groupIndex, _In_ const ctl::ComPtr<xaml::IUIElement>& spContainer);
            _Check_return_ HRESULT GetFocusedHeaderContainer(_Out_ INT* pGroupIndex, _Out_ ctl::ComPtr<xaml::IUIElement>* pspContainer) const;
            _Check_return_ HRESULT ResetFocusedHeaderContainer(_In_ BOOLEAN removeNonGeneratedContainer);
            bool HasFocusedHeader() { return m_focusedHeaderContainer.GetIndex() > -1; };

            BOOLEAN GetIsIndexFocused(_In_ xaml_controls::ElementType type, _In_ INT index) const
            { return type == xaml_controls::ElementType_ItemContainer && m_focusedContainer.GetIndex() == index; }

            // Scroll into view and last focus.
            // Was the last scroll into view for item or header
            bool m_wasLastScrollIntoViewForHeader;
            bool m_isScrollIntoViewPending;
            bool m_isScrollIntoViewInProgress;

            // Collection change notifications

            // Called when a data item is added to Items(Source). We need to know about this so we can update various internal indexes.
            _Check_return_ HRESULT OnGroupAdded(_In_ INT index)
            { RRETURN(OnDataItemAdded(xaml_controls::ElementType_GroupHeader, index)); }
            _Check_return_ HRESULT OnItemAdded(_In_ INT index)
            { RRETURN(OnDataItemAdded(xaml_controls::ElementType_ItemContainer, index)); }
            _Check_return_ HRESULT OnDataItemAdded(_In_ xaml_controls::ElementType type, _In_ INT index);

            // Called when a data item is removed from Items(Source). We need to know about this so we can update various internal indexes.
            _Check_return_ HRESULT OnGroupRemoved(_In_ INT index)
            { RRETURN(OnDataItemRemoved(xaml_controls::ElementType_GroupHeader, index)); }
            _Check_return_ HRESULT OnItemRemoved(_In_ INT index)
            { RRETURN(OnDataItemRemoved(xaml_controls::ElementType_ItemContainer, index)); }
            _Check_return_ HRESULT OnDataItemRemoved(_In_ xaml_controls::ElementType type, _In_ INT index);

            // Called when a data item is replaced from Items(Source).
            _Check_return_ HRESULT OnDataItemReplaced(_In_ xaml_controls::ElementType type, _In_ INT index, _In_ xaml::IUIElement* pNewElement);

            // Return the start and size of various sections of the Children.
            INT32 StartOfVisualSection(xaml_controls::ElementType type) const { return type == xaml_controls::ElementType_ItemContainer ? StartOfContainerVisualSection() : StartOfHeaderVisualSection(); }
            INT32 SizeOfVisualSection(xaml_controls::ElementType type) const { return m_elementIndicesForChildren[type].size(); }
            INT32 EndOfVisualSection(xaml_controls::ElementType type) const { return StartOfVisualSection(type) + SizeOfVisualSection(type); }

            INT32 StartOfHeaderVisualSection() const { return 0; }
            INT32 StartOfContainerVisualSection() const { return EndOfHeaderVisualSection(); } // Containers sit right after the headers

            INT32 SizeOfHeaderVisualSection() const { return SizeOfVisualSection(xaml_controls::ElementType_GroupHeader); }
            INT32 SizeOfContainerVisualSection() const { return SizeOfVisualSection(xaml_controls::ElementType_ItemContainer); }
            INT32 EndOfHeaderVisualSection() const { return EndOfVisualSection(xaml_controls::ElementType_GroupHeader); }
            INT32 EndOfContainerVisualSection() const { return EndOfVisualSection(xaml_controls::ElementType_ItemContainer); }

            // Returns the index of the first element in Children that is in the garbage section.
            INT32 StartOfGarbageSection() const { return SizeOfHeaderVisualSection() + SizeOfContainerVisualSection(); }

            // Get a visual child
            // NOTE: This method is much slower than getting elements from the valid ranges, because this is going to access the collection of visual children.
            _Check_return_ HRESULT GetVisualChild(_In_ INT visualChildIndex, _Out_ ctl::ComPtr<xaml::IUIElement>* pspElement);

            // Trims sentinels from the ends of the collection
            void TrimEdgeSentinels();

            // Returns whether a group is being reset.
            __inline bool IsGroupBeingReset() const
            {
                return m_indexOfGroupBeingReset != -1;
            }

            // Returns the index of the group being reset.
            __inline INT32 GetIndexOfGroupBeingReset() const
            {
                ASSERT(IsGroupBeingReset());
                return m_indexOfGroupBeingReset;
            }

            // Sets the index of the group being reset.
            __inline void SetIndexOfGroupBeingReset(_In_ INT32 index)
            {
                ASSERT(index >= 0);
                m_indexOfGroupBeingReset = index;
            }

            // Invalidates the index of group being reset so that IsGroupBeingReset
            // returns false.
            __inline void InvalidateIndexOfGroupBeingReset()
            {
                m_indexOfGroupBeingReset = -1;
            }

        private:
            // Helper to remove a realized element from the visual children
            _Check_return_ HRESULT RemoveFromVisualChildren(
                _In_ xaml_controls::ElementType type,
                _In_ INT32 index,
                _In_ const ctl::ComPtr<IUIElement>& spElement,
                _In_ BOOLEAN isForElementRemoval);

            // Helper to add a realized element to the visual children
            _Check_return_ HRESULT AddToVisualChildren(
                _In_ xaml_controls::ElementType type,
                _In_ INT32 index,
                _In_ const ctl::ComPtr<IUIElement>& spElement);

            // Perform cleanup once element is unpinned and outside of valid window.
            _Check_return_ HRESULT ElementUnpinned(
                _In_ xaml_controls::ElementType type,
                _In_ INT32 index,
                _In_ const ctl::ComPtr<IUIElement>& spElement,
                _In_ BOOLEAN removeNonGeneratedContainer);

            // Helper to trim one type of sentinel
            void TrimEdgeSentinels(_In_ xaml_controls::ElementType type);


        private:
            // The valid range of containers and headers we use to maintain easy index-to-element lookups
            // NOTE: This collection may include sentinels and other hidden elements which won't participate in layout
            //       and won't ever be added to the visible children.
            std::array<std::deque<ctl::WeakRefPtr>, ElementType_Count> m_validElements;

            // perf optimization: last found indices
            std::array<INT32, ElementType_Count> m_lastFoundIndexElements;

            // Maps each child element to their index in the item collection
            // Should always be the same size as the corresponding region in the children collection
            // We keep this map around to easily and quickly map elements to the chidlren collection
            // because the valid range might have "holes" in it due to sentinels and hidden elements
            typedef std::deque<INT32> IndexMap;
            std::array<IndexMap, ElementType_Count> m_elementIndicesForChildren;

            // Index of first item/group represented by the beginning of the valid range
            std::array<INT32, ElementType_Count> m_indicesOfFirstValidElement;

            // storage for focused container, being focused counts as being pinned, even though we might not
            // have been put into the pinnedcontainers vector
            PinnedElementInfo m_focusedContainer;
            PinnedElementInfo m_focusedHeaderContainer;

            // backtalk
            ModernCollectionBasePanel* const m_owner;

            // pinned containers and headers
            std::array<std::vector<PinnedElementInfo>, ElementType_Count> m_pinnedElements;

            // Temporary holding area for elements that are being prepared. PrepareContainerForItemOverride and its ilk
            // often call back into the mapping methods, so we need a place to hold the preparing container for the
            // lookups to succeed. I'm not using pinning/unpinning because they need to do cleanup that could interfere
            // with what we need.
            struct PreparationHoldingArea
            {
                ctl::ComPtr<IUIElement> m_element;
                INT32 m_index;
                xaml_controls::ElementType m_type;

                PreparationHoldingArea() { Reset(); }
                void Reset() { m_element.Reset(); m_index = -1; m_type = xaml_controls::ElementType_ItemContainer; }
                bool Valid() const { return m_index > -1; }
            };
            PreparationHoldingArea m_preparationHoldingArea;

            // We reset the group ourselves by deleting its items one by one.
            // We store its index because the cache manager doesn't have any information on the group.
            INT32 m_indexOfGroupBeingReset;

        public:
            // when a group is added, the order of events is:
            // 1. group gets added (notifyGroupChange)
            // 2. items get inserted in descending order
            // This is a very strange way of filling up groups and items, but it is what we have to work with.
            // This ties us into GroupedDataCollection's implementation
            // So, when a group is added, we create all the containers that are needed, put their indices in this vector
            // to remember and then _expect_ to get called with inserts. If we get an insert of one of the indices listed
            // in this vector, we will know how to treat it.
            // This vector should only exist temporarily while processing through a group and can be asserted to be null otherwise
            std::vector<INT> m_unlinkedFillers;
        };

        class DeferredModernPanelUnlinkActionPayload :
            public DeferredUnlinkActionPayload
        {
        public:
            void InitializePayload(_In_ ModernCollectionBasePanel* const pOwner);

            _Check_return_ HRESULT Execute(_In_ UIElement* pContainer) override;

        private:
            TrackerPtr<ModernCollectionBasePanel> m_tpOwner;
        };

        // Iterates over a grouped or flat collection.
        // Preconditions: the collection must not be empty and must not change while iterating.
        class CollectionIterator
        {
        public:
            struct ElementInfo
            {
                // No need to initialize the fields as it would be redundant
                // because CollectionInterator makes use of all of them.

                // Item and group information.
                INT itemIndex;
                INT groupIndex;
                INT startItemIndex;
                INT itemCountInGroup;
                xaml_controls::ElementType type;

                // Is this a header?
                BOOLEAN isHeader() const { return type == xaml_controls::ElementType_GroupHeader; }

                INT IndexInGroup() const { return itemIndex - startItemIndex; }
            };

            // Initializes the iterator at the beginning.
            CollectionIterator(_In_ const CacheManager& cache);

            // Initializes the iterator at a given location in the collection.
            CollectionIterator(_In_ const CacheManager& cache, _In_ INT index, _In_ xaml_controls::ElementType type);

            // Used to initializes or re-initializes the iterator.
            void Init(_In_ INT index, _In_ xaml_controls::ElementType type);

            // Try moving to the next element. Retruns true if it succeeds.
            BOOLEAN MoveNext();

            // Try moving to the previous element. Retruns true if it succeeds.
            BOOLEAN MovePrevious();

            // Returns the current element the iterator is pointing at.
            const ElementInfo GetCurrent() const
            {
                return m_current;
            }

        private:
            // Uses the group index to update group information.
            void UpdateCurrentGroupInformation()
            {
                VERIFYHR(m_cache.GetGroupInformationFromGroupIndex(
                            m_current.groupIndex,
                            &m_current.startItemIndex,
                            &m_current.itemCountInGroup));

                m_current.itemIndex = m_current.startItemIndex;
            }

            ElementInfo m_current;
            const CacheManager& m_cache;
        };

    // Actual implementation of ILayoutDataInfoProvider.
    private:
        _Check_return_ HRESULT GetTotalItemCountImpl(_Out_ INT* pReturnValue);
        _Check_return_ HRESULT GetGroupInformationFromItemIndexImpl(_In_ INT itemIndex, _Out_ INT* pIndexOfGroup, _Out_ INT* pIndexInsideGroup, _Out_ INT* pItemCountInGroup);
        _Check_return_ HRESULT GetGroupInformationFromGroupIndexImpl(_In_ INT groupIndex, _Out_ INT* pStartItemIndex, _Out_ INT* pItemCountInGroup);

    public:
        // Implementation of ILayoutDataInfoProvider, that was also required in ListViewBaseHeaderItemAutomationPeer for SizeOfCount.
        _Check_return_ HRESULT GetTotalGroupCountImpl(_Out_ INT* pReturnValue);

        // Implementation of IICG2
        // All of these deal with real Items(Source) indexes.

        // Creates or recycles a container for the given index.
        _Check_return_ HRESULT GenerateContainerAtIndexImpl(
            _In_ INT index,
            _Outptr_ xaml::IUIElement** ppReturnValue) noexcept;

        _Check_return_ HRESULT GetRootOfItemTemplateAsContainer(const ctl::ComPtr<IGeneratorHost>& spHost, const ctl::ComPtr<IInspectable>& item, bool queryForInspecting, _Out_ ctl::ComPtr<xaml::IUIElement>& container);

        // Creates or recycles a header for the given index.
        _Check_return_ HRESULT GenerateHeaderAtGroupIndexImpl(_In_ INT index, _Outptr_ xaml::IUIElement** ppReturnValue);

        // Creates or recycles an element for the given type and index
        _Check_return_ HRESULT GenerateElementAtDataIndex(_In_ xaml_controls::ElementType type, _In_ INT index, _Outptr_ xaml::IUIElement** ppReturnValue)
        {
            HRESULT hr = S_OK;
            switch (type)
            {
            case xaml_controls::ElementType_GroupHeader:
                IFC(GenerateHeaderAtGroupIndexImpl(index, ppReturnValue));
                break;

            case xaml_controls::ElementType_ItemContainer:
                IFC(GenerateContainerAtIndexImpl(index, ppReturnValue));
                break;

            default:
                ASSERT(false, "Invalid enum value");
                IFC(E_UNEXPECTED);
            }

        Cleanup:
            RRETURN(hr);
        }

        // Is our container recycle queue empty?
        _Check_return_ HRESULT GetContainerRecycleQueueEmptyImpl(_Out_ BOOLEAN* pReturnValue);

        // Call UnlinkContainerFromItem and ClearContainerForItem on the given container. If the item isn't
        // it's own container, return the container to the recycle queue.
        // For pinned items, this is a no-op.
        _Check_return_ HRESULT RecycleContainerImpl(_In_ xaml::IUIElement* container);

        // Call UnlinkContainerFromItem and ClearContainerForItem on the given container. If the item isn't
        // it's own container, return the container to the recycle queue.
        // For pinned items, this is a no-op.
        _Check_return_ HRESULT TryRecycleContainerImpl(_In_ xaml::IUIElement* container, _Out_ BOOLEAN* returnValue);

        // Calls RecycleContainer on all realized containers.
        _Check_return_ HRESULT RecycleAllContainersImpl();

        // Is our header recycle queue empty?
        _Check_return_ HRESULT GetHeaderRecycleQueueEmptyImpl(_Out_ BOOLEAN* pReturnValue);

        // Call UnlinkHeaderFromGroup and ClearContainerForGroup on the given header and return it to
        // the recycle queue (except for pinned headers, for which this is a no-op).
        _Check_return_ HRESULT RecycleHeaderImpl(_In_ xaml::IUIElement* header);

        // Calls RecycleHeaderImpl on all realized headers.
        _Check_return_ HRESULT RecycleAllHeadersImpl();

        // Gets a candidate compatible with the item at the given index
        _Check_return_ HRESULT FindRecyclingCandidateImpl(_In_ INT index, _Out_ BOOLEAN* hasMatchingCandidate);

        // Get the queue length
        _Check_return_ HRESULT GetQueueLengthImpl(_Out_ UINT* returnValue);

    // ICG related

        // Remove a container from queue. In most cases, it should be located at the end of it but we handle all cases
        _Check_return_ HRESULT RemoveFromQueue(_In_ xaml::IUIElement* pContainer, _In_ bool expectContainerToBePresent = true);

        // Remove a header from the header recycle queue.
        _Check_return_ HRESULT RemoveFromHeaderRecycleQueue(_In_ xaml::IUIElement* pContainer, _Out_ BOOLEAN* wasInQueue);

        // Associates the item to the container.
        _Check_return_ HRESULT LinkContainerToItem(_In_ const ctl::ComPtr<xaml::IUIElement>& spContainer, _In_ const ctl::ComPtr<IInspectable>& spItem);

        // Clears the item from the container.
        _Check_return_ HRESULT UnlinkContainerFromItem(_In_ const ctl::ComPtr<xaml::IUIElement>& spContainer, _Out_ ctl::ComPtr<IInspectable>* pspItem);

        // Associates the group to the header.
        _Check_return_ HRESULT LinkHeaderToGroup(_In_ const ctl::ComPtr<xaml::IUIElement>& spHeader, _In_ const ctl::ComPtr<IInspectable>& spItem);

        // Clears the group from the header.
        _Check_return_ HRESULT UnlinkHeaderFromGroup(_In_ const ctl::ComPtr<xaml::IUIElement>& spHeader, _Out_ ctl::ComPtr<IInspectable>* pspItem);

        // Helper methods.

        // Prepare the container for recycling by unlinking and clearing, then recycle the container.
        // (recycling means different things depending on if the container is generated or not).
        // Generated containers are left in the tree and reused for another item, while
        // nongenerated containers are removed from the tree entirely (and will be re-added when the
        // virtualization window is appropriate for them).
        // Linked containers are containers which have had LinkContainerToItem and PrepareContainerForItem
        // called on them.
        _Check_return_ HRESULT RecycleLinkedContainer(_In_ xaml::IUIElement* container);

    public:
        // Callback for finishing unlinking from DeferredModernPanelUnlinkActionPayload when layout transition is complete.
        // Transition completion routine should use unloaded queue to avoid adding elements to the visual tree after the Layout completes.
        _Check_return_ HRESULT FinishUnlinkingContainer(_In_ UIElement* pContainer, _In_ BOOLEAN useUnloadingQueue);

    public: // todo: should be moved into ICG2 interface

        // responds to a group change. Hooked up internally
        _Check_return_ HRESULT NotifyOfGroupsChanged(
            _In_ wfc::IObservableVector<IInspectable*>* pSender,
            _In_ wfc::IVectorChangedEventArgs* e);

        // Returns the first visible element's type, offset relative to the top and UIElement.
        _Check_return_ HRESULT GetFirstVisibleElementForSerialization(
            _Out_ xaml_controls::ElementType* pType,
            _Out_ DOUBLE* pOffset,
            _Outptr_result_maybenull_ xaml::IUIElement** ppUIElement);

        // Is the panel in a state where we cannot do a layout pass on it?
        bool IsLockedForLayout() const
        {
            // 1. we are doing a collection change or
            // 2. we are in 'group filling' mode: when a group is added, we have to be prepared
            // to catch the following item inserts in OnItemAdded. We should _never_ be in this mode
            // during regular measure
            return m_inCollectionChange ||
                !m_containerManager.m_unlinkedFillers.empty();
        }

        bool CanScrollIntoView() const
        {
            return !IsLockedForLayout() &&
                   // If layout is in progress (i.e. during measure) and we have an
                   // active window command, we can't scroll into view.
                   // That's because we can't flush the current command during layout.
                   // (e.g. calling UpdateLayout during layout is a no-op).
                   !(m_layoutInProgress && (m_windowState.m_command || m_containerManager.m_isScrollIntoViewInProgress));
        }

    private:
        // Item added in the collection
        _Check_return_ HRESULT OnItemAdded(_In_ INT nIndex);

        // Item removed in the collection
        _Check_return_ HRESULT OnItemRemoved(INT nIndex, bool itemRemovedForGroupReset = false);

        // Group added to the collection.
        _Check_return_ HRESULT OnGroupAdded(_In_ INT nIndex);

        // Group removed in the collection
        _Check_return_ HRESULT OnGroupRemoved(_In_ INT nIndex);

        // Group reset in the collection
        _Check_return_ HRESULT OnGroupReset(_In_ INT nIndex);

        // Data element replaced
        _Check_return_ HRESULT OnDataElementReplaced(_In_ xaml_controls::ElementType type, _In_ INT index);

        // Collection refreshed
        _Check_return_ HRESULT OnCollectionReset();

        // Delegate the preparation of an element to the items host.
        _Check_return_ HRESULT PrepareContainerViaItemsHost(
            _In_ const INT indexInItemCollection,
            _In_ wf::Size measureSize,
            _In_ const ctl::ComPtr<xaml::IUIElement>& spContainer);

        _Check_return_ HRESULT PrepareHeaderViaItemsHost(
            _In_ const INT indexInGroupCollection,
            _In_ wf::Size measureSize,
            _In_ const ctl::ComPtr<xaml::IUIElement>& spHeader);

        _Check_return_ HRESULT PrepareElementViaItemsHost(
            _In_ xaml_controls::ElementType type,
            _In_ const INT indexInDataCollection,
            _In_ wf::Size measureSize,
            _In_ const ctl::ComPtr<xaml::IUIElement>& spElement)
        {
            HRESULT hr = S_OK;
            switch (type)
            {
            case xaml_controls::ElementType_GroupHeader:
                IFC(PrepareHeaderViaItemsHost(indexInDataCollection, measureSize, spElement));
                break;

            case xaml_controls::ElementType_ItemContainer:
                IFC(PrepareContainerViaItemsHost(indexInDataCollection, measureSize, spElement));
                break;

            default:
                ASSERT(false, "Invalid enum value");
                IFC(E_UNEXPECTED);
            }

        Cleanup:
            RRETURN(hr);
        }

        // When groups are inserted, we get subsequent item insertions, but they arrive in reverse order!
        // This could be ugly if we don't correct for it.
        // For instance, if we are looking at group 0, items 0-10, and a new group comes in with 10 items,
        // we'll see a group header come in, but then we'll see an insertion at index 10, which will look to us
        // like we're inserting something right before the old index 10. Things will get messed up.
        _Check_return_ HRESULT CreateUnlinkedFillersForGroup(_In_ INT nIndex);

        // A collection change is in progress
        bool m_inCollectionChange : 1;
        // layout is in progress
        bool m_layoutInProgress : 1;
        bool m_refreshPendingLayout : 1;

    // Interactions
    public:
        // IKeyboardNavigationPanel implementation.
        IFACEMETHOD(SupportsKeyNavigationAction)(
                _In_ xaml_controls::KeyNavigationAction action,
                _Out_ BOOLEAN* pSupportsAction)
                override;

        IFACEMETHOD(GetTargetIndexFromNavigationAction)(
                _In_ UINT sourceIndex,
                _In_ xaml_controls::ElementType sourceType,
                _In_ xaml_controls::KeyNavigationAction action,
                _In_ BOOLEAN allowWrap,
                _In_ UINT itemIndexHintForHeaderNavigation,
                _Out_ UINT* computedTargetIndex,
                _Out_ xaml_controls::ElementType* computedTargetElementType,
                _Out_ BOOLEAN* actionValidForSourceIndex)
                override;

        // IKeyboardHeaderNavigationPanel implementation.
        IFACEMETHOD(GetTargetHeaderIndexFromNavigationAction)(
                _In_ UINT sourceIndex,
                _In_ xaml_controls::KeyNavigationAction action,
                _Out_ UINT* pComputedTargetIndex,
                _Out_ BOOLEAN* pActionHandled)
                override;

        // Sentinel values for indices
        static const INT s_cNavigationIndexFirst;
        static const INT s_cNavigationIndexLast;
        static const INT s_cNavigationIndexNone;

    public:
        // IChildTransitionContextProvider implementation
        IFACEMETHOD(GetChildTransitionContext)(
                _In_ xaml::IUIElement* element,
                _In_ INT layoutTickId,
                _Out_ ThemeTransitionContext* returnValue) override;

        // determines if mutations are going fast
        IFACEMETHOD(IsCollectionMutatingFast)(
                _Out_ BOOLEAN* returnValue)
                override;

        IFACEMETHOD(GetChildTransitionBounds)(
                _In_ xaml::IUIElement* element,
                _Out_ wf::Rect* returnValue) override;

        // IScrollSnapPointsInfo implementation
        _Check_return_ HRESULT get_AreHorizontalSnapPointsRegularImpl(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT get_AreVerticalSnapPointsRegularImpl(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT GetIrregularSnapPointsImpl(_In_ xaml_controls::Orientation orientation, _In_ xaml_primitives::SnapPointsAlignment alignment, _Outptr_ wfc::IVectorView<FLOAT>** returnValue);
        _Check_return_ HRESULT GetRegularSnapPointsImpl(_In_ xaml_controls::Orientation orientation, _In_ xaml_primitives::SnapPointsAlignment alignment, _Out_ FLOAT* pOffset, _Out_ FLOAT* pSpacing);

    private:
        _Check_return_ HRESULT UpdateSnapPoints();

    private:
        class TransitionContextManager
        {
        public:
            TransitionContextManager(_In_ ModernCollectionBasePanel* owner);

            void RegisterForIncrementalLoading()
            {
                // if we already load then ignore incremental loading.
                if (!m_isLoadingStarted)
                {
                    m_shouldIncrementallyLoad = TRUE;
                }
            }

            _Check_return_ HRESULT NotifyOfItemsChanging(
                _In_ wfc::CollectionChange action);

            _Check_return_ HRESULT NotifyOfItemsChanged(
                _In_ wfc::CollectionChange action);

            _Check_return_ HRESULT NotifyOfGroupsChanged(
                _In_ wfc::CollectionChange action);

            _Check_return_ HRESULT NotifyOfItemsReordered(
                _In_ UINT nCount);

            _Check_return_ HRESULT GetTransitionContext(
                _In_ INT32 layoutTickId,
                _In_ BOOLEAN isWrapping,
                _Out_ ThemeTransitionContext* returnValue);

            _Check_return_ HRESULT IsCollectionMutatingFast(_Out_ BOOLEAN* pFast);

        private:
            // Any context related operation must be guarded with this call.
            // It will reset all context counters if we get new tick.
            // If the tick haven't changed then we keep increasing operation counters
            _Check_return_ HRESULT UpdateContextCounters(bool isUpdateDueToMutation);

            // backtalk
            ModernCollectionBasePanel* const m_owner;

            // special value to workaround the async nature of the loaded event.
            // layoutticks are 16 bits wide, so 32_max will never be reached.
            static const INT32 loadCounterToken = INT32_MAX;

            // Used by ThemeTransitionContext to compute Added elements count
            UINT32 m_elementCountAddedThisLayoutTick;

            // Used by ThemeTransitionContext to compute Removed elements count
            UINT32 m_elementCountRemovedThisLayoutTick;

            // Keep current counter
            INT32 m_currentTickCounterId;

            // Used by ThemeTransitionContext to save time when we do a mutation
            DWORD m_currentMutationTime;
            DWORD m_previousMutationTime;

            // Used by ThemeTransitionContext for loading period
            BOOLEAN m_isLoading;

            // Used by ThemeTransitionContext for incremental loading Entrance period
            BOOLEAN m_shouldIncrementallyLoad;

            // Used by ThemeTransitionContext to save the fact that we start loading
            BOOLEAN m_isLoadingStarted;

            // Used by ThemeTransitionContext to save time when we start loading
            DWORD m_startIncrementallyLoadingTime;

            // Used by ThemeTransitionContext to find whether context is changed or not
            BOOLEAN m_resetItemsThisLayoutTick;

            // Used by ThemeTransitionContext to compute Reordered elements count
            INT32 m_elementCountReorderedThisLayoutTick;

            // How long we should show entrance animation.
            static const DWORD incrementalLoadingDuration;

            // The threshold within which we mark collection changes as going fast
            static const DWORD fastmutationThreshold;
        };

    public:
        // calculation of panning direction
        _Check_return_ HRESULT get_PanningDirectionBaseImpl(_Out_ xaml_controls::PanelScrollingDirection* pValue);
        _Check_return_ HRESULT put_PanningDirectionBaseImpl(_In_ xaml_controls::PanelScrollingDirection value);

    protected:
        // window state related
        _Check_return_ HRESULT ProcessOrientationChange();

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

    private:
        // Spatial lookup helper methods

        // Given an index, calculate the index of the item one page from it.
        // WARNING: This method will potentially realize containers to figure
        // things out. Call with caution.
        _Check_return_ HRESULT GetTargetIndexFromNavigationActionByPage(
                _In_ UINT sourceIndex,
                _In_ BOOLEAN isPageDown,
                _In_ BOOLEAN allowWrap,
                _Out_ UINT* pComputedTargetIndex);

        // Calculates the next group header index for a given selection index and item navigation action.
        _Check_return_ HRESULT GetTargetHeaderIndexFromNavigationActionByItem(
                _In_ UINT sourceIndex,
                _In_ xaml_controls::KeyNavigationAction action,
                _Out_ UINT* pComputedTargetIndex);

        // Calculates the prior visible group header index from the provided index.
        _Check_return_ HRESULT GetPreviousHeaderIndex(
            _In_ UINT sourceIndex,
            _Out_ UINT* pComputedPreviousIndex);

        // Calculates the last visible index, from either direction.
        // This is usually the last fully visible index, but if
        // there is no fully visible index, this returns the last
        // partly visible index.
        // fromLowerIndex - direction the search starts from. I.E., with a horizontal orientation,
        // fromLowerIndex = TRUE means the rightmost visible container.
        _Check_return_ HRESULT GetLastVisibleContainer(
                _In_ BOOLEAN fromLowerIndex,
                _In_ BOOLEAN searchForItems,
                _Out_ UINT* pLastVisibleContainerIndex,
                _Out_opt_ BOOLEAN* pFound);

        // Scroll by one visible viewport length,
        // forcing virtualization to run.
        // toIncreasingIndex - Governs the direction of scroll.
        _Check_return_ HRESULT SynchronousScrollByPage(
                _In_ BOOLEAN toIncreasingIndex);

        // Finds an arbitrary realized item from within the group under the given point.
        // Non-grouped scenarios are treated as one large group.
        _Check_return_ HRESULT FindValidContainerInGroupForPoint(
            _In_ wf::Point point,
            _Out_ BOOLEAN* pGroupWasEmpty,
            _Out_ INT32* pGroupIndex,
            _Out_ INT32* preferenceItemIndex,
            _Out_ INT32* preferenceItemIndexInGroup,
            _Out_ wf::Rect* pReferenceItemBounds);

    // Implementation of IItemLookupPanel
    public:
        // Get the closest element information to the point.
        IFACEMETHOD(GetClosestElementInfo)(
            _In_ wf::Point position,
            _Out_ xaml_primitives::ElementInfo* returnValue)
            override;

        // Get the index where an item should be inserted if it were dropped at
        // the given position. This will be used by live reordering.
        IFACEMETHOD(GetInsertionIndex)(
            _In_ wf::Point position,
            _Out_ INT* returnValue)
            override;

        // Gets a series of BOOLEAN values indicating whether a given index is
        // positioned on the leftmost, topmost, rightmost, or bottommost
        // edges of the layout.  This can be useful for both determining whether
        // to tilt items at the edges of rows or columns as well as providing
        // data for portal animations.
        IFACEMETHOD(IsLayoutBoundary)(
            _In_ INT index,
            _Out_ BOOLEAN* isLeftBoundary,
            _Out_ BOOLEAN* isTopBoundary,
            _Out_ BOOLEAN* isRightBoundary,
            _Out_ BOOLEAN* isBottomBoundary)
            override;

        // Gets the bounds of the items.
        IFACEMETHOD(GetItemsBounds)(
            _Out_ wf::Rect* returnValue)
            override;

        // Gets the cache length.
        _Check_return_ HRESULT get_CacheLengthBase(_Out_ DOUBLE* pValue);

        // Sets the cache length.
        _Check_return_ HRESULT put_CacheLengthBase(_In_ DOUBLE value);

    // Implementation of IInsertionPanel
    public:
        // Get the indexes where an item should be inserted if it were dropped at
        // the given position
        _Check_return_ HRESULT GetInsertionIndexesImpl(
            _In_ wf::Point position,
            _Out_ INT* pFirst,
            _Out_ INT* pSecond);

    // Implementation of IPaginatedPanel
    public:
        IFACEMETHOD(GetLastItemIndexInViewport)(
            _In_ DirectUI::IScrollInfo* scrollInfo,
            _Out_ INT* returnValue) override;

        IFACEMETHOD(GetItemsPerPage)(
            _In_ DirectUI::IScrollInfo* scrollInfo,
            _Out_ DOUBLE* returnValue) override;

        _Check_return_ HRESULT GetGroupInformationFromGroupIndex(_In_ INT32 groupIndex, _Out_opt_ INT32* pStartItemIndex, _Out_opt_ INT32* pItemCountInGroup) const
            { RRETURN(m_cacheManager.GetGroupInformationFromGroupIndex(groupIndex, pStartItemIndex, pItemCountInGroup)); }
        _Check_return_ HRESULT GetGroupInformationFromItemIndex(_In_ INT32 itemIndex, _Out_opt_ INT32 *pIndexOfGroup, _Out_opt_ INT32 *pIndexInsideGroup, _Out_opt_ INT32 *pItemCountInGroup) const
            { RRETURN(m_cacheManager.GetGroupInformationFromItemIndex(itemIndex, pIndexOfGroup, pIndexInsideGroup, pItemCountInGroup)); }

    protected:
        // Virtual helper method to get the ItemsPerPage that can be overridden by derived classes.
        virtual _Check_return_ HRESULT GetItemsPerPageImpl(_In_ wf::Rect window, _Out_ DOUBLE* pItemsPerPage);

    // Implementation of IICM and IGHM
    public:
        _Check_return_ HRESULT ItemFromContainerImpl(_In_ xaml::IDependencyObject* container, _Outptr_ IInspectable** ppReturnValue);
        _Check_return_ HRESULT ContainerFromItemImpl(_In_opt_ IInspectable* item, _Outptr_ xaml::IDependencyObject** ppReturnValue);
        _Check_return_ HRESULT IndexFromContainerImpl(_In_ xaml::IDependencyObject* container, _Out_ INT* pReturnValue);
        _Check_return_ HRESULT ContainerFromIndexImpl(_In_ INT index, _Outptr_ xaml::IDependencyObject** ppReturnValue);
        _Check_return_ HRESULT GroupHeaderContainerFromItemContainerImpl(_In_ xaml::IDependencyObject* pItemContainer, _Outptr_result_maybenull_ xaml::IDependencyObject** ppReturnValue);

        _Check_return_ HRESULT GroupFromHeaderImpl(_In_ xaml::IDependencyObject* header, _Outptr_ IInspectable** returnValue);
        _Check_return_ HRESULT HeaderFromGroupImpl(_In_ IInspectable* group, _Outptr_ xaml::IDependencyObject** returnValue);
        _Check_return_ HRESULT IndexFromHeaderImpl(_In_ xaml::IDependencyObject* header, _In_ BOOLEAN excludeHiddenEmptyGroups, _Out_ INT* returnValue);
        _Check_return_ HRESULT HeaderFromIndexImpl(_In_ INT index, _Outptr_ xaml::IDependencyObject** returnValue);

    private:
        TransitionContextManager m_transitionContextManager;
        CacheManager m_cacheManager;
        ContainerManager m_containerManager;
        ctl::ComPtr<xaml_controls::ILayoutStrategy> m_spLayoutStrategy;
        TrackerPtr<wfc::IObservableVector<IInspectable*>> m_tpObservableItemsSource;
        EventRegistrationToken m_observableItemsSourceChangedToken;

        // Window management concepts
        class WindowState
        {
        public:
            // Default constructor
            WindowState();

            // Gets the currently known visible window. This is either the window "coerced" from a scroll request
            // like ScrollIntoView, if a request exists, or just the window calculated from the ScrollViewer otherwise.
            const wf::Rect& GetVisibleWindow() const;

            // Gets the realization window which was calculated from the current visible window
            const wf::Rect& GetRealizationWindow() const { return m_currentRealizationWindow; }
            void SetRealizationWindow(_In_ const wf::Rect& arg) { m_currentRealizationWindow = arg; }

            // Set the last known visible window as determined from the scroll state
            // Returns a BOOLEAN if this new scroll state differs from our current visible window
            void SetScrollStateVisibleWindow(
                _In_ const wf::Rect& newWindowFromScrollState,
                _Out_ BOOLEAN* pShouldInvalidate);

            void SetScrollStateVisibleWindow(
                _In_ const wf::Rect& newWindowFromScrollState) { BOOLEAN ignored; SetScrollStateVisibleWindow(newWindowFromScrollState, &ignored); }

            // Overrides/coerces the locally stored visible window for performing layout
            // If this window differs from the last window calculated from the ScrollViewer, we'll
            // send a new scroll offset to the ScrollViewer at the end of our layout.
            void SetCoercedVisibleWindow(
                _In_ const wf::Rect& newCoercedWindow) { m_visibleWindowCoerced = newCoercedWindow; }

            // Adjust the visible window if this was a mouse large increase or decrease click.
            // Note that we need to do this adjustment after the arrange, now that we have the
            // accurate visible window to calculate how much of the sticky header is occluding
            // the visible window.
            HRESULT CorrectVisibleWindowForMouseLargeClick(_In_ ModernCollectionBasePanel* panel);

            BOOLEAN HasCoercedWindow() const;
            void ClearCoercedWindow();
            const wf::Rect& GetCoercedVisibleWindow() const { return m_visibleWindowCoerced; }

            // Returns the current adjusment resulting for the possible CoercedVisibleWindow
            // Note : protected by apiset, never called on desktop, called only for Vertical Orientation on Phone
            DOUBLE GetCurrentAdjustmentY();

            // we can calculate our window on the fly, but in the case we were limited in some dimension by measure, we should
            // know that always
            wf::Size lastAvailableSize;

            // Flag indicating whether the window is valid (ie, we were able to get correct width/height)
            bool validWindowCalculation;

            // Flag indicating we are done building up our cache
            bool cachePotentialReached;

            // Apply a correction to our window states
            void ApplyAdjustment(_In_ const wf::Point& correction);

            // The last processed scroll offset
            wf::Point m_lastScrollOffset;

            // The last processed zoom factor
            FLOAT m_lastZoomFactor;

            // The last scroll view change was caused by clicking on
            // the scroll bar's LargeIncrease or LargeDecrease
            // buttons. We need special handling for this specific case when
            // sticky headers are used to adjust the visible window since part
            // of the visible window is occluded by the sticky header
            bool m_lastScrollWasMouseLargeClick;

            // was the last scroll forward or backward
            bool m_lastScrollWasForward;

            // Window commands to override default window detection, e.g., ScrollIntoView
            // If pNewVisibleWindow is set to the empty rectangle, no view override
            // will be done.
            std::function<HRESULT (wf::Rect* /* pNewVisibleWindow */)> m_command;

            // updated in arrange, used to determine panning direction
            INT32 currentTickNumber;
            // updated in arrange, if currenttick is different
            INT32 firstVisibleItemIndexOfPreviousTick;
            // updated in arrange
            xaml_controls::PanelScrollingDirection m_panningDirectionBase;

            // The current amount of cache, in pixels, that will be applied on each side of the visible window
            // when calculating the realization window.
            // currentCacheBufferPerSide increases asynchronously at a lower priority than preparing the containers
            // that are in the visible window, and caps at (m_cacheLength * window length in virtualization direction) / 2.
            DOUBLE currentCacheBufferPerSide;

            // This flag is set to true when scrolling so that if we notice that we (over/under)estimated the offset of our elements
            // during the virtualization pass, we shift both elements and the window to correct for the estimation error.
            // We don't shift the window if we are correcting due to a size or collection change.
            BOOLEAN allowWindowShiftOnEstimationErrorCorrection;

            // We did something that might cause us to estimate incorrectly. Set this
            // so that the next measure corrects for estimation errors.
            // To scope the change, we are only re-estimating based on disconnected scrollviews. We considered basing the
            // correction purely on the delta in average container size - but that could case corrections to kick off very
            // often - possibly making thumb drag experience worse.
            bool m_estimationCorrectionPending;

        private:
            // Current window for us to fill with containers, including buffers
            wf::Rect m_currentRealizationWindow;

            // The ScrollViewer's viewport, projected onto our panel
            // Distinct from realization window for multiple reasons. For instance, realized buffered containers
            // can be immediately recycled, but visible containers need to perform a content transition. Also, page up/down
            // should scroll by the size of the visual set, not the realized set of containers.
            wf::Rect m_visibleWindowFromScrollState;
            wf::Rect m_visibleWindowCoerced;
        };

        // Viewport behavior settings.
        struct ViewportBehaviorInfo
        {
            // Default constructor
            ViewportBehaviorInfo()
            {
                Reset();
            }

            // Cancels tracking.
            void Reset()
            {
                wf::Rect emptyRect = {};

                isTracking = FALSE;
                isTrackingExtentEnd = FALSE;
                originalExtent = currentExtent = 0.0f;
                elementBounds = emptyRect;
                type = xaml_controls::ElementType_ItemContainer;
                index = -1;
                elementOffset = 0.0f;
                initialViewportEdge = 0.0f;
                isOffsetRelativeToSecondEdge = FALSE;
                trackedElementShift = 0.0f;
                viewportOffsetDelta = 0.0f;
            }

            // True when we are tracking a container or a header.
            BOOLEAN isTracking;

            // True when KeepLastItemInView is set and we are at the very end
            // of the list when we start tracking. We need to make sure that after
            // collection changes (e.g. some new items might get added at the end),
            // measure changes or size changes, we are still at the very end of the list.
            BOOLEAN isTrackingExtentEnd;
            // Stores the panel's extent when we started tracking the end of it (isTrackingExtentEnd == true).
            float originalExtent;
            // Stores the most up to date extent while we are tracking the end of it (isTrackingExtentEnd == true).
            float currentExtent;

            // Bounds of the tracked element.
            // They get updated when there are changes before it (e.g. element inserted before).
            wf::Rect elementBounds;
            // Is the tracked element a header or a container?
            xaml_controls::ElementType type;
            // The absolute index of the tracked element in the source collection.
            INT32 index;
            // Offset of the viewport's top/left or bottom/right when the tracking started.
            FLOAT initialViewportEdge;
            // Offset between the viewport and the tracked element.
            FLOAT elementOffset;
            // Is the offset calculated based on the first or second edge?
            BOOLEAN isOffsetRelativeToSecondEdge;
            // During the Generate phase (which take place during the measure pass),
            // we shift all the elements by the amount the tracked element moved.
            // We need to store that delta in case we need to elect a new tracked
            // element during Generate. This happens only when we track a group header
            // whose group was cleared and HidesIfEmpty equals true.
            FLOAT trackedElementShift;

            // If we start tracking outside layout (ie due to a collection change), we run the
            // risk that, by the time we get to arrange, the viewport offset we used when we started
            // tracking is stale (out of date) if the collection change happened while the viewport is
            // moving. viewportOffsetDelta is the amount of pixels we need to compensate for the viewport
            // drift.
            float viewportOffsetDelta;
        };

        // When called the first time, this method starts tracking the first
        // visible element. Otherwise, it does nothing.
        _Check_return_ HRESULT BeginTrackingFirstVisibleElement();

        // Starts tracking the first visible element if it's not already the case.
        // If we already tracking something, it updates the tracked element so
        // we can deal with its removal or replacement.
        _Check_return_ HRESULT BeginTrackingFirstVisibleElement(
            _In_ bool isCollectionChange,
            _In_ bool isGroupChange,
            _In_ INT index,
            _In_ wfc::CollectionChange action = wfc::CollectionChange_Reset);

        // Wrapper that calls BeginTrackingFirstVisibleElement when a collection change happens.
        _Check_return_ HRESULT BeginTrackingOnCollectionChange(
            _In_ bool isGroupChange,
            _In_ INT index,
            _In_ wfc::CollectionChange action);

        // Wrapper that calls BeginTrackingFirstVisibleElement when an element changes size.
        _Check_return_ HRESULT BeginTrackingOnMeasureChange(
            _In_ bool isGroupChange,
            _In_ INT index);

        // Wrapper that calls BeginTrackingFirstVisibleElement when we detect a viewport size change.
        _Check_return_ HRESULT BeginTrackingOnViewportSizeChange(
            const wf::Rect& previousWindow,
            const wf::Rect& currentWindow);

        // Wrapper that calls BeginTrackingFirstVisibleElement but also changes the location of that element and the window
        _Check_return_ HRESULT BeginTrackingOnOrientationChange(_Out_ wf::Rect* pComputedWindow);

        _Check_return_ HRESULT BeginTrackingOnRefresh(_Out_ wf::Rect* newVisibleWindow);

        // Called during MeasureExistingElements to get the realized window accounting for the viewport shift.
        // This make sure ShouldContinueFillingUpSpace behaves correctly.
        wf::Rect GetRealizationWindowAfterViewportShift();

        // Called during the Generate pass to shift all the elements, including the
        // tracked element, to the new realization window.
        _Check_return_ HRESULT ApplyTrackedElementShift();

        // Returns whether a change happened before the tracked element.
        _Check_return_ HRESULT IsChangeBeforeTrackedElement(
            _In_ bool isCollectionChange,
            _In_ bool isGroupChange,
            _In_ INT index,
            _In_ wfc::CollectionChange action,
            _Out_ bool* pIsChangeBeforeTrackedElement,
            _Out_ bool* pIsChangeGoingToHideTrackedGroupHeader);

        // If ItemsUpdatingScrollMode is set to KeepLastElementInView and the viewport is at the very end
        // of the list, we need to shift the viewport to the last element which might have been recently inserted.
        _Check_return_ HRESULT TrackLastElement();

        // Shifts the viewport to the new location of the first visible element.
        _Check_return_ HRESULT EndTrackingFirstVisibleElement();

        // Adjust layout transitions to account for viewport shift.
        _Check_return_ HRESULT AdjustLayoutTransitions(float viewportShift);
        _Check_return_ HRESULT AdjustLayoutTransitionsForElement(
            _In_ const ctl::ComPtr<IUIElement>& element,
            _In_ float viewportShift,
            _In_ xaml_controls::Orientation layoutOrientation);

        // Returns the first visible element's type, index and UIElement.
        _Check_return_ HRESULT GetFirstVisibleElement(
            _Out_ xaml_controls::ElementType* type,
            _Out_ INT* index,
            _Outptr_result_maybenull_ xaml::IUIElement** ppUIElement);

        // Returns the first visible element's type, index and UIElement
        // in a given window.
        _Check_return_ HRESULT GetFirstElementInWindow(
            _In_ const wf::Rect& window,
            _Out_ xaml_controls::ElementType* elementType,
            _Out_ INT* elementIndex,
            _Outptr_result_maybenull_ xaml::IUIElement** ppUIElement,
            _Out_opt_ wf::Rect* pAdjustedWindowForStickyHeaders);

        // Sets the offset between the tracked element and the visible window.
        void SetTrackedElementOffsetRelativeToViewport();

        // Returns the tracked UIElement.
        _Check_return_ HRESULT GetTrackedElement(_Out_ ctl::ComPtr<IUIElement>* pspTrackedElement);

        // When the tracked element gets removed, we need to elect a new one.
        _Check_return_ HRESULT ElectNewTrackedElement();

        // Returns where should the viewport move to 'follow' the tracked element.
        // The edge is relative to whether we are tracking the first element (top/left)
        // or the last element (bottom/right).
        FLOAT GetNewViewportEdge();

        _Check_return_ HRESULT IsBottomOrRightAligned(
            _In_ xaml::IFrameworkElement* element,
            _In_ xaml_controls::Orientation orientation,
            _Out_ bool* result);

        void UpdateViewportOffsetDelta(
            _In_ const wf::Rect& oldVisibleWindow,
            _In_ const wf::Rect& newVisibleWindow);

        // Determine if a point is inside the window, or is before or after it in the virtualizing direction.
        RelativePosition GetReferenceDirectionFromWindow(
            _In_ wf::Rect referenceRect,
            _In_ wf::Rect window) const;

        // Calculates the visible window by analyzing our layout and our parent ScrollViewer's state.
        _Check_return_ HRESULT CalculateVisibleWindowFromScrollPoint(
            _In_ const wf::Point& newScrollPoint,
            _In_ FLOAT zoomFactor,
            _Out_ wf::Rect* pVisibleWindow);

        // Calculates the visible window from a set of scroll data
        _Check_return_ HRESULT CalculateVisibleWindowFromScrollData(
            _In_ const wf::Rect& scrollViewport,
            _In_ FLOAT zoomFactor,
            _Out_ wf::Rect* pVisibleWindow);

        // Before we get any ScrollViewChanging or ScrollViewChanged events, we need to proactively get a window
        // (instead of waiting for an event to tell us of a window).
        // This method gets a window from the ScrollViewer's offset, if possible. If a ScrollViewer isn't
        // available, we go with a default window.
        _Check_return_ HRESULT PrimeVisibleWindow(bool allowTrackingOnViewportSizeChange = false);

        // Using the current visible window, calculate the realization window (taking into
        // account buffer space, etc).
        //
        // allowCache should be true during normal measure passes and false otherwise when initially setting the realization window.
        // The allowCache parameter allows us to incrementally build a cache by inflating the realization window during
        // measure passes.  When true, we will increase the realization window by CacheBufferPerSideInflationPixelDelta.
        // When false, we will reset the cache buffers to 0.
        _Check_return_ HRESULT SetRealizationWindowFromVisibleWindow(_In_ BOOLEAN allowCache);

        // Recycles alls containers (and headers if grouping) and removes them from the cache manager.
        _Check_return_ HRESULT RecycleAllContainersAndHeaders();

        // Given a window to fill with containers and headers, estimate
        // the location of the anchor container and header (if any),
        // call into ICG2 to get the container and header, set both of
        // their bounds, and add to valid containers and headers.
        _Check_return_ HRESULT GenerateAnchorsForWindow(
            _In_ const wf::Rect& windowToFill,
            _In_opt_ const xaml_controls::EstimationReference* headerReference,
            _In_opt_ const xaml_controls::EstimationReference* containerReference);

        // Given a group index and its reference, estimate its position,
        // call into ICG2 to get a header for the group, place the header
        // in the valid headers, and sets the header's bounds to the estimated
        // position.
        _Check_return_ HRESULT GenerateAnchorForGroup(
            _In_ const INT32 groupIndex,
            _In_opt_ const xaml_controls::EstimationReference* headerReference,
            _In_opt_ const xaml_controls::EstimationReference* containerReference);

        // Given an item index and its references, estimate its position,
        // call into ICG2 to get a container, place the container
        // in the valid containers, and sets the container's bounds to the estimated
        // position.
        _Check_return_ HRESULT GenerateAnchorForItem(
            _In_ const INT32 itemIndex,
            _In_opt_ const xaml_controls::EstimationReference* headerReference,
            _In_opt_ const xaml_controls::EstimationReference* containerReference);

        // window state

        // Triggers a synchronous layout if a command exists.
        _Check_return_ HRESULT FlushWindowStateCommand();

        // Estimates are inevitably wrong, so... when they are wrong, we need to adjust our containers' placement
        // on the panel so users can scroll exactly to the first/last elements, no more, no less. So, when we start
        // getting close to the edge, we need to see if we are wrong and perform the shift.
        _Check_return_ HRESULT CorrectForEstimationErrors(bool adjustWindow = true);

        // This method actually performs the heavy lifting of ScrollItemIntoView.
        _Check_return_ HRESULT DoScrollItemIntoView(
            _In_ INT32 itemIndex,
            _In_ xaml_controls::ScrollIntoViewAlignment alignment,
            _In_ DOUBLE offset,
            _In_ BOOLEAN animate,
            _Out_opt_ wf::Rect* pComputedWindow);

        // This method actually performs the heavy lifting of ScrollGroupHeaderIntoView.
        _Check_return_ HRESULT DoScrollGroupHeaderIntoView(
            _In_ INT32 groupIndex,
            _In_ xaml_controls::ScrollIntoViewAlignment alignment,
            _In_ DOUBLE offset,
            _In_ BOOLEAN animate,
            _Out_opt_ wf::Rect* pComputedWindow);

        // Get or generate the container for the given index and set that
        // container as the focused container.
        _Check_return_ HRESULT GenerateAnchorAndSetFocus(
            _In_ INT32 index,
            _In_ bool isHeader);

        // This method actually performs the heavy lifting of doing an orientation switch.
        _Check_return_ HRESULT DoProcessOrientation(_Out_ wf::Rect* pComputedWindow);

        // Estimate the extent of our panel
        // Since the last realized header and container represent the extent realized so far, we can
        // get more accurate results by estimating the remaining unrealized extent and tacking it on to the end of the realized extent
        _Check_return_ HRESULT EstimatePanelExtent(_Out_ wf::Size* pExtent);

        // Scrolls our ScrollViewer to the given area (synchronous).
        _Check_return_ HRESULT SetScrollViewerOffsetsTo(_In_ const wf::Rect& targetRect, _In_ bool animate, _Out_ bool* pIssued);

        // Helper method to place a window around an element's rect
        _Check_return_ HRESULT PlaceWindowAroundElementRect(
            _In_ const wf::Rect& sourceWindowRect,
            _In_ const wf::Rect& elementRectToAlignTo,
            _In_ xaml_controls::ScrollIntoViewAlignment alignment,
            _In_ DOUBLE offset,
            _Out_ wf::Rect* pResult);

        // builds up a referencestruct about an anchor that can be used to base generation on
        _Check_return_ HRESULT GetGenerationAnchorInformation(
            _Out_ xaml_controls::LayoutReference* pReferenceInformation,
            _Out_ CollectionIterator* pIterator);

        // Walk up the parent chain and find our ScrollViewer.
        // Attach its OnScrollViewChanging and ScrollViewChanged to our OnScrollViewChange method.
        // Also attach to our ItemsPresenter.
        _Check_return_ HRESULT AttachPanelComponents();

        // Clear our understanding of our parent ScrollViewer and ItemsPresenter.
        _Check_return_ HRESULT DetachPanelComponents();

        // Called when our ScrollViewer's view changes. Update our current visible window.
        _Check_return_ HRESULT OnScrollViewChanging(
            _In_ IInspectable* pSender,
            _In_ xaml_controls::IScrollViewerViewChangingEventArgs* pArgs);

        // Called when our ScrollViewer's has changed. Update our current visible window.
        _Check_return_ HRESULT OnScrollViewChanged(
            _In_ IInspectable* pSender,
            _In_ xaml_controls::IScrollViewerViewChangedEventArgs* pArgs);

        // Called when our ScrollViewer's size has changed. Re-evaluate our current visible window.
        // Cannot use measure available size because we might have changed in the virtualizing direction.
        _Check_return_ HRESULT OnScrollViewerSizeChanged(
            _In_ IInspectable *pSender,
            _In_ xaml::ISizeChangedEventArgs *pArgs);

    // Event handlers.
    private:
        _Check_return_ HRESULT OnPanelUnloaded(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnGroupStyleChanged(
            _In_ xaml::IDependencyObject* pSender,
            _In_ const CDependencyProperty* pDP);


    // GroupStyle helpers
    public:
        _Check_return_ HRESULT GetGroupStyle(_Outptr_ xaml_controls::IGroupStyle** ppGroupStyle)
        { RRETURN(m_tpGroupStyle.CopyTo(ppGroupStyle)); }

    private:
        // Get a new GroupStyle from the items host
        _Check_return_ HRESULT UpdateGroupStyle();

    // Miscellaneous helper methods.
    private:
        static UIElement::VirtualizationInformation* GetVirtualizationInformationFromElement(_In_ const ctl::ComPtr<IUIElement>& spElement)
        {
            return spElement.Cast<UIElement>()->GetVirtualizationInformation();
        }

        static BOOLEAN ElementHasVirtualizationInformation(_In_ const ctl::ComPtr<IUIElement>& spElement)
        {
            return spElement.Cast<UIElement>()->GetVirtualizationInformation() != nullptr;
        }

        static wf::Rect GetBoundsFromElement(_In_ const ctl::ComPtr<IUIElement>& spElement)
        {
            return spElement.Cast<UIElement>()->GetVirtualizationInformation()->GetBounds();
        }

        static void SetElementEmptySizeInGarbageSection(_In_ const ctl::ComPtr<IUIElement>& spElement);
        
        static void SetElementSizeInGarbageSection(_In_ const ctl::ComPtr<IUIElement>& spElement, _In_ wf::Size size);

        static void SetBoundsForElement(_In_ const ctl::ComPtr<IUIElement>& spElement, _In_ wf::Rect bounds);

        static void SetElementIsRealized(_In_ const ctl::ComPtr<IUIElement>& spElement, bool isRealized)
        {
            spElement.Cast<UIElement>()->GetVirtualizationInformation()->SetIsRealized(isRealized);
        }

        static ctl::ComPtr<IInspectable> GetItemFromElement(_In_ const ctl::ComPtr<IUIElement>& spElement)
        {
            RRETURN(spElement.Cast<UIElement>()->GetVirtualizationInformation()->GetItem());
        }

        static _Check_return_ HRESULT SetItemForElement(_In_ const ctl::ComPtr<IUIElement>& spElement, _In_ const ctl::ComPtr<IInspectable>& spItem)
        {
            RRETURN(spElement.Cast<UIElement>()->GetVirtualizationInformation()->SetItem(spItem.Get()));
        }

        static void SetElementIsGenerated(_In_ const ctl::ComPtr<IUIElement>& spElement, _In_ BOOLEAN isGenerated)
        {
            spElement.Cast<UIElement>()->GetVirtualizationInformation()->SetIsGenerated(isGenerated);
        }

        static BOOLEAN GetElementIsGenerated(_In_ const ctl::ComPtr<IUIElement>& spElement)
        {
            return spElement.Cast<UIElement>()->GetVirtualizationInformation()->GetIsGenerated();
        }

        static void SetElementIsHeader(_In_ const ctl::ComPtr<IUIElement>& spElement, _In_ BOOLEAN isHeader)
        {
            spElement.Cast<UIElement>()->GetVirtualizationInformation()->SetIsHeader(isHeader);
        }

        static BOOLEAN GetElementIsHeader(_In_ const ctl::ComPtr<IUIElement>& spElement)
        {
            return spElement.Cast<UIElement>()->GetVirtualizationInformation()->GetIsHeader();
        }

        static BOOLEAN GetElementIsSentinel(_In_ const ctl::ComPtr<IUIElement>& spElement)
        {
            return !spElement;
        }

        static void SetElementWantsToSkipContainerPreparation(_In_ const ctl::ComPtr<IUIElement>& spElement, _In_ bool shouldSkipContainerPreparation)
        {
            spElement.Cast<UIElement>()->GetVirtualizationInformation()->SetWantsToSkipContainerPreparation(shouldSkipContainerPreparation);
        }

        static void SetElementIsPrepared(_In_ const ctl::ComPtr<IUIElement>& spElement, _In_ bool isPrepared)
        {
            spElement.Cast<UIElement>()->GetVirtualizationInformation()->SetIsPrepared(isPrepared);
        }

        static bool GetElementIsPrepared(_In_ const ctl::ComPtr<IUIElement>& spElement)
        {
            return spElement.Cast<UIElement>()->GetVirtualizationInformation()->GetIsPrepared();
        }

        static void SetIsContainerFromTemplateRoot(_In_ const ctl::ComPtr<IUIElement>& spElement, bool isFromRoot)
        {
            spElement.Cast<UIElement>()->GetVirtualizationInformation()->SetIsContainerFromTemplateRoot(isFromRoot);
        }

        static bool GetIsContainerFromTemplateRoot(_In_ const ctl::ComPtr<IUIElement>& spElement)
        {
            return spElement.Cast<UIElement>()->GetVirtualizationInformation()->GetIsContainerFromTemplateRoot();
        }

        _Check_return_ HRESULT RegisterSpecialElementSize(_In_ xaml_controls::ElementType type, _In_ INT index, _In_ wf::Size desiredSize)
        {
            HRESULT hr = S_OK;

            switch (type)
            {
            case xaml_controls::ElementType_GroupHeader:
                IFC(RegisterSpecialHeaderSize(index, desiredSize));
                break;

            case xaml_controls::ElementType_ItemContainer:
                IFC(RegisterSpecialContainerSize(index, desiredSize));
                break;

            default:
                ASSERT(false, "Invalid enum value");
                IFC(E_UNEXPECTED);
                break;
            }
        Cleanup:
            RRETURN(hr);
        }

        static wf::Point GetOffScreenPosition() { wf::Point position = { -15000, -15000 }; return position; }

        BOOLEAN IsItemsHostRegistered();

        // Struct component helper methods
        float wf::Point::* PointFromPointInVirtualizingDirection () const;
        float wf::Size::* PointFromSizeInVirtualizingDirection() const;
        float wf::Size::* SizeInVirtualizingDirection() const;
        float wf::Size::* SizeInNonVirtualizingDirection() const;
        float wf::Rect::* PointFromRectInVirtualizingDirection() const;
        float wf::Rect::* PointFromRectInNonVirtualizingDirection () const;
        float wf::Rect::* SizeFromRectInVirtualizingDirection () const;

        // Output data desired by ETW tracing
        _Check_return_ HRESULT TraceVirtualizationData();

        // Just returns E_FAIL inside an IFC_RETURN to help us
        // capture the current callstack in the error context
        _Check_return_ HRESULT ThrowEFAIL()
        {
            IFC_RETURN(E_FAIL);
            return S_OK;
        }

        void TraceGuardFailure(_In_ PCWSTR failureReason)
        {
            ASSERT(false);
            TraceLoggingWrite(
                g_hTraceProvider,
                "ModernPanelGuard",
                TraceLoggingWideString(failureReason, "FailureReason"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY));

            // We cannot crash the app but we can store the callstack
            // in the stowed exceptions as a breadcrumb to help us debug
            // if this guard failure ultimately led to a crash
            IGNOREHR(ThrowEFAIL());
        }

        // Creates a default layout reference.
        xaml_controls::LayoutReference CreateDefaultLayoutReference() const;

        // Creates a default (and invalid) estimation reference.
        xaml_controls::EstimationReference CreateDefaultEstimationReference(
            _In_ xaml_controls::ElementType elementType) const;

        // Sticky Headers
        // Configure or update the sticky behavior of a header
        _Check_return_ HRESULT ConfigureStickyHeader(
            _In_ const ctl::ComPtr<IUIElement>& spHeader,
            _In_ DirectUI::ScrollViewer* pScrollViewer,
            _In_ DirectUI::UIElement* pLTEParent,
            _In_ DOUBLE groupBottomY,
            _In_ const wf::Rect& desiredBounds,
            _In_ bool manuallyUpdatePosition,
            _Out_ bool* pUpdated);

        _Check_return_ HRESULT ConfigureStickyHeader(
            _In_ const ctl::ComPtr<IUIElement>& spHeader,
            _In_ INT groupIndex,
            _In_ const wf::Rect& desiredBounds);

        // Update all sticky header behaviors
        _Check_return_ HRESULT UpdateStickyHeaders(_In_ const wf::Size& finalSize);

        // Update the clipping region for items under sticky headers
        _Check_return_ HRESULT UpdateItemClippingForStickyHeaders(
            _In_ const wf::Size& finalSize,
            _In_ bool manuallyUpdatePosition,
            _In_ bool updateClip) noexcept;

        // Remove sticky header transformation
        void RemoveStickyHeader(_In_ ctl::ComPtr<IUIElement>& spHeader);

        // Remove all sticky headers transformations
        _Check_return_ HRESULT RemoveStickyHeaders();

        // Computes the vertical position of a sticky header
        static DOUBLE GetStickyHeaderPositionY(
            _In_ DOUBLE groupTopY,
            _In_ DOUBLE groupInnerHeight,
            _In_ DOUBLE visibleTopY);

        // compute the extent of the real header (as in not sticky) that is outside the visible window.
        _Check_return_ HRESULT GetRealHeaderExtentOutsideVisibleWindow(_Out_ FLOAT *headerExtentOutsideVisibleWindow);

        _Check_return_ HRESULT CoerceWindowToExtent(_Inout_ wf::Rect& visibleWindow);
public:

        // Computes the offsets resulting from Sticky Headers
        _Check_return_ HRESULT CoerceStickyHeaderOffsets(
            _In_ ListViewBaseHeaderItem *pHeaderItem,
            _In_ INT cOffsets,
            _Inout_updates_(cOffsets) DOUBLE *pOffsets);

        _Check_return_ HRESULT NotifyLayoutTransitionStart(
            _In_ ListViewBaseHeaderItem* pHeaderItem);

        _Check_return_ HRESULT NotifyLayoutTransitionEnd(
            _In_ ListViewBaseHeaderItem* pHeaderItem);

        // Provide the padding to the viewport in vertical direction - this occurs only when
        // there is a sticky header.
        _Check_return_ HRESULT GetVerticalViewportPadding(_Out_ FLOAT *verticalViewportPadding);

    public:
        // Gets the default cache length
        static DOUBLE GetDefaultCacheLength();

    // state
    protected:
        // Resets the current cache buffer to 0 and invalidates measure.
        _Check_return_ HRESULT ResetCacheBuffers();

        // Stores the first visible element.
        // We need this method on orientation change to remember the first visible element in the visible window.
        // Then, when Generate() next runs (asynchronously) with the new orientation set and new realization window,
        // we can center the visible window around this element that we cached.
        // When switching orientations, we thus preserve the first element in the visible window.
        // Note that we do not preserve offset into the first visible element.
        _Check_return_ HRESULT CacheFirstVisibleElementBeforeOrientationChange();

        // Return the viewport size of our ScrollViewer.
        _Check_return_ HRESULT GetViewportSize(_Out_ wf::Size* pSize);

        const ContainerManager& GetContainerManager() { return m_containerManager; }

    private:
        // Basic fundamental state
        ctl::EventPtr<FrameworkElementUnloadedEventCallback> m_epUnloadedHandler;

        // IICG2
        IItemContainerGenerator2* m_icg2;   // just a pre-QI'd version of ourselves, to maintain the abstract facade we put up.

        // Queues of containers ready to link to items.
        std::vector<ctl::WeakRefPtr> m_recycleQueue;

        // List of elements which are being removed from the panel and are the target of an unload theme transition.
        TrackerPtr<TrackerCollection<xaml::UIElement*>> m_unloadingElements;

        // Queues of elements which were unloaded after transitions completed.
        TrackerPtr<TrackerCollection<xaml::UIElement*>> m_tpUnloadedElements;

        // Queues of headers ready to link to groups.
        std::vector<ctl::WeakRefPtr> m_recycleHeaderQueue;

        // Window management state
        WindowState m_windowState;

        // Our ScrollViewer.
        ctl::WeakRefPtr m_wrScrollViewer;

        // Our ItemsPresenter (if any).
        ctl::WeakRefPtr m_wrItemsPresenter;

        // Viewport behavior management
        ViewportBehaviorInfo m_viewportBehavior;

        // Event registrations on the ScrollViewer.
        ctl::EventPtr<ViewChangingEventCallback> m_epScrollViewerViewChangingHandler;
        ctl::EventPtr<ViewChangedEventCallback> m_epScrollViewerViewChangedHandler;
        ctl::EventPtr<FrameworkElementSizeChangedEventCallback> m_epScrollViewerSizeChangedHandler;

        // Note, need to invalidate sizes anytime we disconnect/anchor, and any time we change the collection
        // When we perform estimate corrections, however, we can just apply the same correction to the stored extent
        // we need to keep this for later anchoring when disconnected
        wf::Size m_estimatedSize;
        INT32 m_elementCountAtLastMeasure;

        // The GroupHeaderStrategy is stored on the layout strategy, but we need to store the
        // GroupHeaderPlacement here, so we can reevaluate the strategy if our orientation changes
        xaml_primitives::GroupHeaderPlacement m_groupHeaderPlacement;

        // Manage GroupStyle state
        TrackerPtr<xaml_controls::IGroupStyle> m_tpGroupStyle;
        ctl::EventPtr<DependencyPropertyChangedCallback> m_epGroupStyleChangedHandler;

        // Pixel delta by which to inflate the cache buffer on each side.  Rather than fill the entire
        // cache buffer all at once, we chunk the work to make the UI thread more responsive.  We inflate
        // the cache buffer from 0 to a max value determined by the CacheLength property.
        static const DOUBLE CacheBufferPerSideInflationPixelDelta;

        // only increase pixel delta if the time since last tick is small
        static const INT PerformCacheInflationWhenTimeAvailable;

        // Backing storage for ItemsWrapGrid.CacheLength and ItemsStackPanel.CacheLength properties.
        // Represents the amount of cache buffers in terms of viewports.
        DOUBLE m_cacheLength;

        // ItemsPresenter ensures that we have a nice origin before measure is called
        wf::Point m_originFromItemsPresenter;

        // Caches the first element in the visible window.
        TrackerPtr<xaml::IUIElement> m_tpFirstVisibleElementBeforeOrientationChange;

        // Caches the type of the first element in the visible window.
        xaml_controls::ElementType m_typeOfFirstVisibleElementBeforeOrientationChange;

        // Caches the index of the first element in the visible window.
        INT32 m_indexOfFirstVisibleElementBeforeOrientationChange;

        // SnapPoint state
        class SnapPointState
        {
        public:
            SnapPointState();

            BOOLEAN AreSnapPointsRegular(_In_ xaml_controls::Orientation orientation) const
            {
                return m_orientation == orientation && m_isRegular;
            }

            _Check_return_ xaml_controls::Orientation
                GetOrientation() const
            {
                return m_orientation;
            }

            _Check_return_ HRESULT SetIrregularSnapPoints(
                _In_ xaml_controls::Orientation orientation,
                _Inout_ std::vector<FLOAT>&& keys,
                _Out_ BOOLEAN* pChanged);

            _Check_return_ HRESULT GetIrregularSnapPoints(
                _In_ xaml_controls::Orientation orientation,
                _In_ xaml_primitives::SnapPointsAlignment alignment,
                _Outptr_ wfc::IVectorView<FLOAT>** returnValue) const;

            _Check_return_ HRESULT SetRegularSnapPoints(
                _In_ xaml_controls::Orientation orientation,
                _In_ FLOAT spacing,
                _In_ FLOAT nearOffset,
                _In_ FLOAT farOffset,
                _Out_ BOOLEAN* pChanged);

            _Check_return_ HRESULT GetRegularSnapPoints(
                _In_ xaml_controls::Orientation orientation,
                _In_ xaml_primitives::SnapPointsAlignment alignment,
                _Out_ FLOAT* pOffset,
                _Out_ FLOAT* pSpacing) const;

        private:
            // Core state
            BOOLEAN m_snapPointsSet;
            BOOLEAN m_isRegular;
            xaml_controls::Orientation m_orientation;

            // Values
            std::vector<FLOAT> m_irregularSnapPointKeys;
            FLOAT m_regularSpacing;
            FLOAT m_nearOffset;
            FLOAT m_farOffset;
        } m_snapPointState;

        typedef std::vector<wf::Rect> RectVector;
        std::array<RectVector, ElementType_Count> m_arrangeRects;

        BOOLEAN m_bLastItemRealized;
        TrackerPtr<xaml::Internal::ISecondaryContentRelationshipStatics> m_tpSecondaryContentRelationshipStatics;
        BOOLEAN m_bUseStickyHeaders;
        DOUBLE m_currentHeaderHeight;
        // Represents how much space sticky headers are taking away from the visible window (as of the last arrange).
        FLOAT m_lastVisibleWindowClippingHeight;
        DOUBLE m_cachedPannableExtent;

        struct StickyHeaderInfo {
            DOUBLE m_offset;
            DOUBLE m_size;
            StickyHeaderInfo(DOUBLE offset, DOUBLE size) : m_offset(offset), m_size(size) {}
            StickyHeaderInfo() : m_offset(0), m_size(0) {}
            bool operator==(const StickyHeaderInfo& rhs) const { return m_offset == rhs.m_offset && m_size == rhs.m_size; }
        };
        std::vector<StickyHeaderInfo> m_currentStickyHeaderLocations;
        std::vector<StickyHeaderInfo> m_newStickyHeaderLocations;

        TrackerPtr<DirectUI::CompositeTransform> m_tpItemClippingTransform;
        TrackerPtr<DirectUI::SecondaryContentRelationship> m_tpItemClippingRelationship;

        // Debug state.
#ifdef DBG
        // We can temporarily enter an invalid state during the processing of item collection changes-
        // if we enter RunVirtualization somehow during this processing (probably via user code),
        // we'll have problems matching up indexes.
        // We may need to fix this mismatching issue in the future, but for now we have this field
        // to catch when it happens.
        BOOLEAN m_debugIsInTemporaryInvalidState;
#endif

        // Default value for the CacheLength property.  CacheLength defines the maximum amount of cache buffer.
        static const DOUBLE WindowsDefaultCacheLength;
        static const DOUBLE PhoneDefaultCacheLength;

        // DataTemplateSelector related constants
        // When no container in the queue currently has the DataTemplate we are looking for
        // if the number of containers in the queue is less than QueueLengthBeforeFallback,
        // we create a new container
        // if it is greater, then we select a container even if it means changing its template
        static const UINT QueueLengthBeforeFallback;
        // When we are looking in valid containers outside the virtualization window which
        // have the DataTemplate we are looking for, we will have to recycle all containers
        // between the extremity and the container (if any) with the right template
        // and this can have an immediate cost greater than switching a template
        // ValidContainersBeforeFallback is the maximum number of containers we will examine
        // before creating a new one instead of trying to recycle
        static const UINT ValidContainersBeforeFallback;

        // Elements are arranged one after the other and they may overlay over a fine line.
        // For example, in a vertical list, if item 1 is arranged at location (0, 0) width a size
        // of (100, 50), then item 2 is arranged at location (0, 50). They overlap over line y = 50.
        // This impacts the calculation of the first and last visible elements, and we need this
        // small delta to remove the edge overlay.
        static const FLOAT EdgeOverlayDisambiguationDelta;

        // Counts for tracking container request/generation for telemetry purposes
        UINT m_containerRequestedCount;
        UINT m_containerCreatedCount;

        // Provides information about the data source for the layout strategies.
        ctl::ComPtr<xaml_controls::ILayoutDataInfoProvider> m_spLayoutDataInfoProvider;

        // Iterator used for CCC's incremental visualization.
        ctl::ComPtr<ContainerContentChangingIterator> m_spContainerContentChangingIterator;

        // In order to avoid the cost of repeated and unnecessary allocations,
        // we'll allocate exactly one instance of ChoosingItemContainerEventArgs
        // and will then just reuse it every single time ChoosingItemContainer is raised.
        TrackerPtr<ChoosingItemContainerEventArgs> m_trChoosingItemContainerEventArgs;

        // In order to avoid the cost of repeated and unnecessary allocations,
        // we'll allocate exactly one instance of ChoosingGroupHeaderContainerEventArgs
        // and will then just reuse it every single time ChoosingGroupHeaderContainer is raised.
        TrackerPtr<ChoosingGroupHeaderContainerEventArgs> m_trChoosingGroupHeaderContainerEventArgs;
    };
}