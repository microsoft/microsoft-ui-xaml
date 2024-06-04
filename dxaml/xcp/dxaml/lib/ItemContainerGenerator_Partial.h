// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemContainerGenerator.g.h"
#include "UIElement_Partial.h"

namespace DirectUI
{
    // forward declarations
    struct IGeneratorHost;

    class DeferredUnlinkActionPayload :
        public ctl::WeakReferenceSource
    {
    public:
        virtual _Check_return_ HRESULT Execute(_In_ UIElement* pContainer) = 0;

    protected:
        BEGIN_INTERFACE_MAP(DeferredUnlinkActionPayload, ComBase)
            INTERFACE_ENTRY(DeferredUnlinkActionPayload, IInspectable)
        END_INTERFACE_MAP(DeferredUnlinkActionPayload, ComBase)
    };

    /// <summary>
    /// An ItemContainerGenerator is responsible for generating the UI on behalf of
    /// its host (e.g. ItemsControl).  It maintains the association between the items in
    /// the control's data view and the corresponding
    /// UIElements.  The control's item-host can ask the ItemContainerGenerator for
    /// a Generator, which does the actual generation of UI.
    /// </summary>
    PARTIAL_CLASS(ItemContainerGenerator),
        public xaml_controls::IItemContainerMapping    // Internally implement this interface. Not public API because of method name collisions in the IDL
    {
        friend class OrientedVirtualizingPanel;
        friend class VirtualizingPanel;
        friend class WrapGrid;
        friend class CarouselPanel;
        friend class VirtualizingStackPanel;
        friend class ItemsControl;
        friend class Selector;
        friend class GroupItem;
        friend class Generator;
        friend class ComboBox;
        friend class ListViewBase;
        class ItemBlock;
        class UnrealizedItemBlock;
        class RealizedItemBlock;

    public:
        // Return the ItemContainerGenerator appropriate for use by the given panel
        _Check_return_ HRESULT GetItemContainerGeneratorForPanelImpl(
            _In_opt_ xaml_controls::IPanel* panel,
            _Outptr_ xaml_controls::IItemContainerGenerator** returnValue);

        // Begin generating at the given position and direction
        // This method must be called before calling GenerateNext.  It returns an
        // IDisposable object that tracks the lifetime of the generation loop.
        // This method sets the generator's status to GeneratingContent;  when
        // the IDisposable is disposed, the status changes to ContentReady or
        // Error, as appropriate.
        _Check_return_ HRESULT StartAtImpl(
            _In_ xaml_primitives::GeneratorPosition position,
            _In_ xaml_primitives::GeneratorDirection direction,
            _In_ BOOLEAN allowStartAtRealizedItem);

        // Return the container element used to display the next item.
        // When the next item has not been realized, this method returns a container
        // and sets isNewlyRealized to true.  When the next item has been realized,
        // this method returns the existing container and sets isNewlyRealized to
        // false.
        // This method must be called after a previous call to StartAt.
        _Check_return_ HRESULT GenerateNextImpl(
            _Out_ BOOLEAN* isNewlyRealized,
            _Outptr_ xaml::IDependencyObject** returnValue);

        // Dispose the generator
        _Check_return_ HRESULT StopImpl();

        // Prepare the given element to act as the container for the
        // corresponding item.  This includes applying the container style,
        // forwarding information from the host control (ItemTemplate, etc.),
        // and other small adjustments.
        // This method must be called after the element has been added to the
        // visual tree, so that resource references and inherited properties
        // work correctly.
        _Check_return_ HRESULT PrepareItemContainerImpl(
            _In_ xaml::IDependencyObject* container);

        // Remove generated elements.
        using ItemContainerGeneratorGenerated::Remove;
        _Check_return_ HRESULT RemoveImpl(
            _In_ xaml_primitives::GeneratorPosition position,
            _In_ INT count);

        // Remove all generated elements.
        _Check_return_ HRESULT RemoveAllImpl();

        // Primitives::IRecyclingItemContainerGenerator
        _Check_return_ HRESULT RecycleImpl(
            _In_ xaml_primitives::GeneratorPosition position,
            _In_ INT count);

        // IItemContainerGenerator

        // Map an index into the items collection to a GeneratorPosition.
        _Check_return_ HRESULT GeneratorPositionFromIndexImpl(
            _In_ INT itemIndex,
            _Out_ xaml_primitives::GeneratorPosition* returnValue);

        // Map a GeneratorPosition to an index into the items collection.
        _Check_return_ HRESULT IndexFromGeneratorPositionImpl(
            _In_ xaml_primitives::GeneratorPosition position,
            _Out_ INT* returnValue);

        // Return the item corresponding to the given UI element.
        // If the element was not generated as a container for this generator's
        // host, the method returns DependencyProperty.UnsetValue.
        _Check_return_ HRESULT ItemFromContainerImpl(
            _In_ xaml::IDependencyObject* container,
            _Outptr_ IInspectable** returnValue);

        // Return the UI element corresponding to the given item.
        // Returns null if the item does not belong to the item collection,
        // or if no UI has been generated for it.
        _Check_return_ HRESULT ContainerFromItemImpl(
            _In_opt_ IInspectable* item,
            _Outptr_ xaml::IDependencyObject** returnValue);

        // Given a generated UI element, return the index of the corresponding item
        // within the ItemCollection.
        _Check_return_ HRESULT IndexFromContainerImpl(
            _In_ xaml::IDependencyObject* container,
            _Out_ INT* returnValue);

        // Return the UI element corresponding to the item at the given index
        // within the ItemCollection.
        _Check_return_ HRESULT ContainerFromIndexImpl(
            _In_ INT index,
            _Outptr_ xaml::IDependencyObject** ppReturnValue);

        static _Check_return_ HRESULT ExecuteDeferredUnlinkAction(_In_ CUIElement* nativeTarget);

        // Return the UI element corresponding to the item at the given index
        // within the current level.
        _Check_return_ HRESULT ContainerFromGroupIndex(
            _In_ INT index,
            _Outptr_ xaml::IDependencyObject** returnValue);

        // IReferenceTrackerInternal
        void OnReferenceTrackerWalk(INT walkType) final;

        // Detach host. An ICG is created by default for an ItemsControl, but when using a
        // ModernCollectionPanel, the ICG is detached at the ItemsControl end. This method
        // also detaches the ICG's WeakRef to the IC, to ensure it doesn't try to access it.
        _Check_return_ HRESULT DetachHost() {m_wrHost.Reset(); RRETURN(S_OK);}

        // Return true if the requested item index is in the range of the available item count.
        bool IsIndexInRange(_In_ INT itemIndex);

    protected:
        _Check_return_ HRESULT GetItemsChangedEventSourceNoRef(
            _Outptr_ ItemsChangedEventSourceType** ppEventSource) override;

        ItemContainerGenerator();
        ~ItemContainerGenerator() override;

        // Protected implementation of IItemContainerMapping
        HRESULT QueryInterfaceImpl(
            _In_ REFIID iid,
            _Outptr_ void** ppObject) override;
        IFACEMETHOD(ItemFromContainer)(
            _In_ xaml::IDependencyObject* container,
            _Outptr_ IInspectable** returnValue) override;
        IFACEMETHOD(ContainerFromItem)(
            _In_opt_ IInspectable* item,
            _Outptr_ xaml::IDependencyObject** returnValue) override;
        IFACEMETHOD(IndexFromContainer)(
            _In_ xaml::IDependencyObject* container,
            _Out_ INT* returnValue) override;
        IFACEMETHOD(ContainerFromIndex)(
            _In_ INT index,
            _Outptr_ xaml::IDependencyObject** returnValue) override;

    private:
        static _Check_return_ HRESULT CreateGenerator(
            _In_ IGeneratorHost* pHost,
            _Outptr_ ItemContainerGenerator** ppGenerator);

        static _Check_return_ HRESULT CreateGenerator(
            _In_ ItemContainerGenerator* pParent,
            _In_ GroupItem* pGroupItem,
            _In_ wfc::IVector<xaml::DependencyObject*>* pRecyclableContainers,
            _Outptr_ ItemContainerGenerator** ppGenerator);

        static _Check_return_ HRESULT CreateGenerator(
            _In_opt_ ItemContainerGenerator* pParent,
            _In_ IGeneratorHost* pHost,
            _In_opt_ xaml_controls::IGroupItem* pPeer,
            _In_ wfc::IVector<xaml::DependencyObject*>* pRecyclableContainers,
            _In_ UINT nLevel,
            _Outptr_ ItemContainerGenerator** ppGenerator);

        // Remove generated elements.
        _Check_return_ HRESULT Remove(
            _In_ xaml_primitives::GeneratorPosition position,
            _In_ INT count,
            _In_ BOOLEAN isRecycling);

        _Check_return_ HRESULT ClearBlocks(_In_ BOOLEAN bRecycle);

        //     Performs a linear search.

        //     There's no avoiding a linear search, which leads to O(n^2) performance
        //     if someone calls ContainerFromItem or IndexFromContainer for every item.
        //     To mitigate this, we start each search at _startIndexForUIFromItem, and
        //     heuristically set this in various places to where we expect the next
        //     call to occur.

        //     For example, after a successful search, we set it to the resulting
        //     index, hoping that the next call will query either the same item or
        //     the one after it.  And after inserting a new item, we expect a query
        //     about the new item.  Etc.

        //     Saving this as an index instead of a (block, offset) pair, makes it
        //     more robust during insertions/deletions.  If the index ends up being
        //     wrong, the worst that happens is a full search (as opposed to following
        //     a reference to a block that's no longer in use).

        //     To re-use the search code for two methods, please read the description
        //     of the parameters.

        // <param name="container">
        //     If non-null, then searches for the container.
        //     Otherwise, updated with container for the item.
        // </param>
        // <param name="item">
        //     If non-null, then searches for the item.
        //     Otherwise, updated with the item that the container represents.
        // </param>
        // <param name="itemIndex">
        //     If a container or item is found, then updated with its index.
        //     Otherwise, set to -1.
        // </param>
        _Check_return_ HRESULT DoLinearSearch(
            _Inout_ xaml::IDependencyObject** ppContainer,
            _In_opt_ IInspectable* pItem,
            _Out_opt_ INT* pItemIndex,
            _Out_opt_ bool* found = nullptr);

        _Check_return_ HRESULT GetCount(
            _Out_ INT& count);

        _Check_return_ HRESULT GetCount(
            _In_ ItemBlock* pStopBlock,
            _Out_ INT& count);

        static _Check_return_ HRESULT GetItemsCount(
            _In_ IInspectable* pItem,
            _Out_ UINT& nItemCount);

        _Check_return_ HRESULT GetRealizedItemBlockCount(
            _In_ RealizedItemBlock* pRealizedBlock,
            _In_ INT end,
            _Out_ INT& count);

        _Check_return_ HRESULT Refresh()
        {
            RRETURN(OnRefresh());
        }

         // The read only view of items
        _Check_return_ HRESULT get_View(
            _Outptr_ wfc::IVector<IInspectable*>** pValue);

        // cached state of the factory's item map (updated by factory)
        // used to speed up calls to Generate
        struct GeneratorState
        {
            ItemBlock* Block;    // some block in the map (most recently used)
            INT        Offset;    // offset with the block
            INT        Count;     // cumulative item count of blocks before the cached one
            INT        ItemIndex; // index of current item
        };

        _Check_return_ HRESULT MoveToPosition(
            _In_ xaml_primitives::GeneratorPosition position,
            _In_ xaml_primitives::GeneratorDirection direction,
            _In_ BOOLEAN allowStartAtRealizedItem,
            _In_ UINT nItemsCount,
            _Out_ GeneratorState& state);

        _Check_return_ HRESULT Realize(
            _In_ UnrealizedItemBlock* pBlock,
            _In_ INT offset,
            _In_ IInspectable* pItem,
            _In_ xaml::IDependencyObject* pContainer);

        void RemoveAndCoalesceBlocksIfNeeded(
            _In_ ItemBlock* pBlock);

        void MoveItems(
            _In_ ItemBlock* pBlock,
            _In_ INT offset,
            _In_ INT count,
            _In_ ItemBlock* pNewBlock,
            _In_ INT newOffset,
            _In_ INT deltaCount);

        // create a group item for the given group
        _Check_return_ HRESULT ContainerForGroup(
            _In_ xaml_data::ICollectionViewGroup* pGroup,
            _Outptr_ xaml::IDependencyObject** ppContainer);

        // create a group item for the given group using recycled group item if possible
        _Check_return_ HRESULT ContainerForGroup(
            _In_ xaml_data::ICollectionViewGroup* pGroup,
            _In_opt_ xaml::IDependencyObject* pRecycledContainer,
            _Outptr_ xaml::IDependencyObject** ppContainer);

        // prepare the grouping information.  Called from RemoveAll.
        _Check_return_ HRESULT PrepareGrouping();

        // should the given group be hidden?
        _Check_return_ HRESULT ShouldHide(
            _In_ xaml_data::ICollectionViewGroup* pGroup,
            _Out_ BOOLEAN& returnValue);

        _Check_return_ HRESULT OnGroupStylePropertyChanged(
            _In_ IInspectable* pSender,
            _In_ xaml_data::IPropertyChangedEventArgs* e);

        _Check_return_ HRESULT GetBlockAndPosition(
            _In_ UINT itemIndex,
            _Out_ xaml_primitives::GeneratorPosition& position,
            _Out_ ItemBlock*& pItemBlock,
            _Out_ INT& offsetFromBlockStart);

        _Check_return_ HRESULT LinkContainerToItem(
            _In_ xaml::IDependencyObject* pContainer,
            _In_ IInspectable* pItem);

        _Check_return_ HRESULT UnlinkContainerFromItem(
            _In_ xaml::IDependencyObject* pContainer,
            _In_ BOOLEAN isRecycling,
            _In_ BOOLEAN isFinishingDeferredAction = FALSE);

        _Check_return_ HRESULT OnItemCollectionChanged(
            _In_ wfc::IObservableVector<IInspectable*>* pSender,
            _In_ wfc::IVectorChangedEventArgs* e);

        _Check_return_ HRESULT OnItemAdded(_In_ UINT nIndex);

        _Check_return_ HRESULT OnItemRemoved(_In_ UINT nIndex);

        _Check_return_ HRESULT OnItemReplaced(_In_ UINT nIndex);

        _Check_return_ HRESULT HideGroupItemIfEmpty();

        _Check_return_ HRESULT OnRefresh();

        _Check_return_ HRESULT RaiseItemsChanged(
            _In_ wfc::CollectionChange action,
            _In_ xaml_primitives::GeneratorPosition position,
            _In_ UINT nItemsCount,
            _In_ UINT nContainersCount);

        _Check_return_ HRESULT ClearRecyclingQueue()
        {
            if (m_tpRecyclableContainers)
            {
                RRETURN(m_tpRecyclableContainers->Clear());
            }
            RRETURN(S_OK);
        }

        // ItemsControl informs the generator when collection is going to fire Collection changed event.
        _Check_return_ HRESULT NotifyOfSourceChanged(
            _In_ wfc::IObservableVector<IInspectable*>* pSender,
            _In_ wfc::IVectorChangedEventArgs* e);

    private:
        // weak reference to IGeneratorHost
        TrackerPtr<wfc::IObservableVector<IInspectable*>> m_tpObservableHostItems;
        ctl::WeakRefPtr m_wrHost;
        TrackerPtr<wfc::IVector<IInspectable*>> m_tpView;
        TrackerPtr<wfc::IObservableVector<IInspectable*>> m_tpObservableItemsSource;

        TrackerPtr<xaml_controls::IGroupStyle> m_tpGroupStyle;
        BOOLEAN m_bIsGrouping;

        // TODO: This ComPtr will not cause implicit pegging that could lead to a a reference tracker
        // leak since EventSource derives from ComBase only (not WeakReferenceSourceNoThreadID) and has no reference tracking functionality.
        // This will get cleaned up when we move event sources in general to be managed using std::shared_ptr<> - see TFS 993969.
        ctl::ComPtr<ItemsChangedEventSourceType> m_spEventSource;

        EventRegistrationToken m_ObservableHostItemsChangedToken;
        EventRegistrationToken m_ObservableItemsSourceChangedToken;
        EventRegistrationToken m_GroupStyleChangedToken;

        xaml_primitives::GeneratorPosition m_PositionFromNotify;

        ItemBlock* m_pItemMap; // Abstract Node for Head block

        INT m_iStartIndexForUIFromItem;
        // weak reference to IGroupItem
        ctl::WeakRefPtr m_wrPeer;
        UINT m_nLevel;
        ctl::WeakRefPtr m_wrParent;
        TrackerPtr<wfc::IVector<xaml::DependencyObject *>> m_tpRecyclableContainers;
        TrackerPtr<wfc::IVector<xaml::DependencyObject *>> m_tpSharedRecyclableContainers;

    private:
        //------------------------------------------------------
        //
        //  Private Nested Class -  ItemContainerGenerator.Generator
        //
        //------------------------------------------------------

        //     Generator is the object that generates UI on behalf of an ItemsControl,
        //     working under the supervision of an ItemContainerGenerator.
        //     Note that the generated UI can be different from
        class Generator
        {
            friend class ItemContainerGenerator;

            //------------------------------------------------------
            //
            //  Constructors
            //
            //------------------------------------------------------
            Generator(
                _In_ ItemContainerGenerator* pFactory,
                _In_ UINT nItemsCount,
                _In_ xaml_primitives::GeneratorPosition position,
                _In_ xaml_primitives::GeneratorDirection direction,
                _In_ BOOLEAN allowStartAtRealizedItem);

            _Check_return_ HRESULT GenerateNext(
                _Out_ BOOLEAN* isNewlyRealized,
                _Outptr_ xaml::IDependencyObject** returnValue);

            // The map data structure has changed, so the state must change accordingly.
            // This is called in various different ways.
            //  A. Items were moved within the data structure, typically because
            //  items were realized or un-realized.  In this case, the args are:
            //      block - the block from where the items were moved
            //      offset - the offset within the block of the first item moved
            //      count - how many items moved
            //      newBlock - the block to which the items were moved
            //      newOffset - the offset within the new block of the first item moved
            //      deltaCount - the difference between the cumulative item counts
            //                  of newBlock and block
            //  B. An item was added or removed from the data structure.  In this
            //  case the args are:
            //      block - null  (to distinguish case B from case A)
            //      offset - the index of the changed item, w.r.t. the entire item list
            //      count - +1 for insertion, -1 for deletion
            //      others - unused
            //  C. Refresh: all items are returned to a single unrealized block.
            //  In this case, the args are:
            //      block - null
            //      offset - -1 (to distinguish case C from case B)
            //      newBlock = the single unrealized block
            //      others - unused
            void OnMapChanged(
                _In_opt_ ItemBlock* pBlock,
                _In_ INT offset,
                _In_ INT count,
                _In_opt_ ItemBlock* pNewBlock,
                _In_ INT newOffset,
                _In_ INT deltaCount);

            //------------------------------------------------------
            //
            //  Private Fields
            //
            //------------------------------------------------------

            ItemContainerGenerator* m_pFactory;
            xaml_primitives::GeneratorDirection m_direction;
            BOOLEAN m_bDone{};
            GeneratorState m_cachedState;

        }* m_pGenerator;

        //------------------------------------------------------
        //
        //  Private Nested Classes
        //
        //------------------------------------------------------

        class DeferredGeneratorUnlinkActionPayload :
            public DeferredUnlinkActionPayload
        {
        public:
            DeferredGeneratorUnlinkActionPayload() :
                m_isRecycling(FALSE)
            {}

            void InitializePayload(_In_ ItemContainerGenerator* pGenerator, _In_ BOOLEAN isRecycling );

            _Check_return_ HRESULT Execute(_In_ UIElement* pContainer) override;

        private:
            TrackerPtr<ItemContainerGenerator> m_tpGenerator;
            BOOLEAN m_isRecycling;
        };

        // The ItemContainerGenerator uses the following data structure to maintain
        // the correspondence between items and their containers.  It's a doubly-linked
        // list of ItemBlocks, with a sentinel node serving as the header.
        // Each node maintains two counts:  the number of items it holds, and
        // the number of containers.
        //
        // There are two kinds of blocks - one holding only "realized" items (i.e.
        // items that have been generated into containers) and one holding only
        // unrealized items.  The container count of a realized block is the same
        // as its item count (one container per item);  the container count of an
        // unrealized block is zero.
        //
        // Unrealized blocks can hold any number of items.  We only need to know
        // the count.  Realized blocks have a fixed-sized array (BlockSize) so
        // they hold up to that many items and their corresponding containers.  When
        // a realized block fills up, it inserts a new (empty) realized block into
        // the list and carries on.
        //
        // This data structure was chosen with virtualization in mind.  The typical
        // state is a long block of unrealized items (the ones that have scrolled
        // off the top), followed by a moderate number (<50?) of realized items
        // (the ones in view), followed by another long block of unrealized items
        // (the ones that have not yet scrolled into view).  So the list will contain
        // an unrealized block, followed by 3 or 4 realized blocks, followed by
        // another unrealized block.  Fewer than 10 blocks altogether, so linear
        // searching won't cost that much.  Thus we don't need a more sophisticated
        // data structure.  (If profiling reveals that we do, we can always replace
        // this one.  It's totally private to the ItemContainerGenerator and its
        // Generators.)

        // represents a block of items
        class ItemBlock
        {
            friend class Generator;
            friend class ItemContainerGenerator;

        public:
            static const INT32 BlockSize = 16;

        protected:
            ItemBlock()
            {
                m_pNextBlock = NULL;
                m_pPrevBlock = NULL;
                m_nItemCount = 0;
            }

            virtual ~ItemBlock()
            {
                m_pNextBlock = NULL;
                m_pPrevBlock = NULL;
                m_nItemCount = 0;
            }

            virtual INT32 get_ContainerCount()
            {
                return INT32_MAX;
            }

            virtual _Check_return_ HRESULT get_ContainerAt(
                _In_ UINT index,
                _Outptr_ xaml::IDependencyObject** ppContainer)
            {
                HRESULT hr = S_OK;
                IFCPTR(ppContainer);
                *ppContainer = NULL;

            Cleanup:
                RRETURN(hr);
            }

            virtual _Check_return_ HRESULT get_ItemAt(
                _In_ UINT index,
                _Outptr_ IInspectable** ppItem)
            {
                HRESULT hr = S_OK;
                IFCPTR(ppItem);
                *ppItem = NULL;

            Cleanup:
                RRETURN(hr);
            }

            virtual UnrealizedItemBlock* AsUnrealizedBlock()
            {
                return NULL;
            }

            virtual RealizedItemBlock* AsRealizedBlock()
            {
                return NULL;
            }

            virtual BOOLEAN IsMoveAllowed(_In_ BOOLEAN allowMovePastRealizedItem)
            {
                return TRUE;
            }

        private:
            void InsertAfter(_In_ ItemBlock* prev)
            {
                m_pNextBlock = prev->m_pNextBlock;
                m_pPrevBlock = prev;

                m_pPrevBlock->m_pNextBlock = this;
                m_pNextBlock->m_pPrevBlock = this;
            }

            void InsertBefore(_In_ ItemBlock* next)
            {
                InsertAfter(next->m_pPrevBlock);
            }

            void Remove()
            {
                m_pPrevBlock->m_pNextBlock = m_pNextBlock;
                m_pNextBlock->m_pPrevBlock = m_pPrevBlock;
                delete this;
            }

            void StepForward(GeneratorState& state, BOOLEAN allowMovePastRealizedItem)
            {
                INT32 offset = 1;
                MoveToForward(state, offset, allowMovePastRealizedItem);
            }

            void StepBackward(GeneratorState& state, BOOLEAN allowMovePastRealizedItem)
            {
                INT32 offset = -1;
                MoveToBackward(state, offset, allowMovePastRealizedItem);
            }

            // offset positive
            void MoveToForward(GeneratorState& state, INT32& offset, BOOLEAN allowMovePastRealizedItem)
            {
                if (IsMoveAllowed(allowMovePastRealizedItem))
                {
                    if (state.Offset + offset < m_nItemCount)
                    {
                        state.ItemIndex += offset;
                        state.Offset += offset;
                        offset = 0;
                    }
                    else
                    {
                        INT32 index = m_nItemCount - state.Offset;
                        index = index > 1 ? index : 1;
                        state.ItemIndex += index;
                        offset -= index;
                        state.Count += m_nItemCount;

                        state.Block = m_pNextBlock;
                        state.Offset = 0;
                    }
                }
            }

            // offset negative
            void MoveToBackward(GeneratorState& state, INT32& offset, BOOLEAN allowMovePastRealizedItem)
            {
                if (IsMoveAllowed(allowMovePastRealizedItem))
                {
                    if (state.Offset + offset >= 0)
                    {
                        state.ItemIndex += offset;
                        state.Offset += offset;
                        offset = 0;
                    }
                    else
                    {
                        INT32 index = m_nItemCount + state.Offset;
                        index = index > 1 ? index : 1;
                        state.ItemIndex -= index;
                        offset += index;
                        state.Count -= m_nItemCount;

                        state.Block = m_pPrevBlock;
                        state.Offset = m_pPrevBlock->m_nItemCount - 1;
                    }
                }
            }

        protected:
            INT32 m_nItemCount;
            ItemBlock* m_pPrevBlock;
            ItemBlock* m_pNextBlock;
        };

        // represents a block of unrealized (un-generated) items
        class UnrealizedItemBlock : public ItemBlock
        {
            friend class Generator;

        protected:
            INT32 get_ContainerCount() override
            {
                return 0;
            }

            UnrealizedItemBlock* AsUnrealizedBlock() override
            {
                return this;
            }
        };

        // an entry in the table maintained by RealizedItemBlock
        struct BlockEntry
        {
            TrackerPtr<IInspectable> _item;
            TrackerPtr<xaml::IDependencyObject> _container;
        };

        // represents a block of realized (generated) items
        class RealizedItemBlock final : public ItemBlock
        {
            friend class Generator;
            friend class ItemContainerGenerator;

        protected:
            RealizedItemBlock()
            {
                // _entry initializes itself
            }

            ~RealizedItemBlock() override
            {
                for (UINT index = 0; index < BlockSize; ++index)
                {
                    _entry[index]._item.Clear();
                    _entry[index]._container.Clear();
                }
            }

            INT32 get_ContainerCount() override
            {
                return m_nItemCount;
            }

            RealizedItemBlock* AsRealizedBlock() override
            {
                return this;
            }

            _Check_return_ HRESULT get_ContainerAt(
                _In_ UINT index,
                _Outptr_ xaml::IDependencyObject** ppContainer) override
            {
                //TODO TrackerPtr CopyTo
                HRESULT hr = S_OK;
                IFCPTR(ppContainer);
                *ppContainer = _entry[index]._container.Get();
                AddRefInterface(*ppContainer);

            Cleanup:
                RRETURN(hr);
            }


            // This is a special version of get_ContainerAtNoRef that doesn't do an AddRef.
            // During destruction, an AddRef might not be possible, because the controlling unknown
            // may be gone.
            _Check_return_ HRESULT get_ContainerAtNoRef(
                _In_ UINT index,
                _Outptr_ xaml::IDependencyObject** ppContainer)
            {
                HRESULT hr = S_OK;
                IFCPTR(ppContainer);
                *ppContainer = _entry[index]._container.GetAsDO();

            Cleanup:
                RRETURN(hr);
            }

            _Check_return_ HRESULT get_ItemAt(
                _In_ UINT index,
                _Outptr_ IInspectable** ppItem) override
            {
                HRESULT hr = S_OK;
                IFCPTR(ppItem);
                *ppItem = _entry[index]._item.Get();
                AddRefInterface(*ppItem);

            Cleanup:
                RRETURN(hr);
            }

            BOOLEAN IsMoveAllowed(_In_ BOOLEAN allowMovePastRealizedItem) override
            {
                return allowMovePastRealizedItem;
            }

        private:
            void CopyEntries(RealizedItemBlock* src, INT32 offset, INT32 count, INT32 newOffset)
            {
                // choose which direction to copy so as not to clobber existing
                // entries (in case the source and destination blocks are the same)
                if (offset < newOffset)
                {
                    // copy right-to-left
                    for (INT i = count - 1; i >= 0; --i)
                    {
                        _entry[newOffset + i]._container.Clear();
                        _entry[newOffset + i]._item.Clear();

                        IGNOREHR(_entry[newOffset + i]._container.Set(src->_entry[offset + i]._container.Get()));
                        IGNOREHR(_entry[newOffset + i]._item.Set(src->_entry[offset + i]._item.Get()));

                        src->_entry[offset + i]._container.Clear();
                        src->_entry[offset + i]._item.Clear();
                    }
                }
                else
                {
                    // copy left-to-right
                    for (INT i = 0; i < count; ++i)
                    {
                        _entry[newOffset + i]._container.Clear();
                        _entry[newOffset + i]._item.Clear();

                        IGNOREHR(_entry[newOffset + i]._container.Set(src->_entry[offset + i]._container.Get()));
                        IGNOREHR(_entry[newOffset + i]._item.Set(src->_entry[offset + i]._item.Get()));

                        src->_entry[offset + i]._container.Clear();
                        src->_entry[offset + i]._item.Clear();
                    }
                }
            }

            void ClearEntries(_In_ INT offset, _In_ INT count)
            {
                for (INT i = 0; i < count; ++i)
                {
                    _entry[offset + i]._item.Clear();
                    _entry[offset + i]._container.Clear();
                }
            }

            void RealizeItem(
                _In_ INT32 index,
                _In_ IInspectable* pItem,
                _In_ xaml::IDependencyObject* pContainer)
            {
                IGNOREHR(_entry[index]._item.Set(pItem));
                IGNOREHR(_entry[index]._container.Set(pContainer));
            }

            _Check_return_ HRESULT OffsetOfItem(
                _In_ IInspectable* item,
                _Out_ INT& nOffset)
            {
                HRESULT hr = S_OK;
                nOffset = -1;

                for (INT32 i = 0; i < m_nItemCount; ++i)
                {
                    bool areEqual = false;
                    IFC(PropertyValue::AreEqual(_entry[i]._item.Get(), item, &areEqual));
                    if (areEqual)
                    {
                        nOffset = i;
                        goto Cleanup;
                    }
                }

            Cleanup:
                RRETURN(hr);
            }

            BlockEntry _entry[BlockSize];
        };
    };
}
