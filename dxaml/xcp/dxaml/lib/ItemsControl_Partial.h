// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IGeneratorHost.g.h"
#include "ItemsControl.g.h"

namespace DirectUI
{
    // forward declarations
    class ItemCollection;
    class ItemContainerGenerator;

    // Represents a ItemsControl control.
    //
    PARTIAL_CLASS(ItemsControl)
        , public IGeneratorHost
    {

        friend class ListViewBaseItem;
        friend class ItemCollection;

        // special value to workaround the async nature of the loaded event.
        // layoutticks are 16 bits wide, so 32_max will never be reached.
        static const INT32 loadCounterToken = INT32_MAX;

    public:

        _Check_return_ HRESULT get_ItemsHost(_Outptr_ xaml_controls::IPanel** pValue) override;

        _Check_return_ HRESULT get_IsItemsHostInvalid(_Out_ BOOLEAN* pValue) override;

        // Returns the ItemsPresenter, if found, that is the templated parent of the ItemsHost.
        _Check_return_ HRESULT get_ItemsPresenter(_Outptr_opt_ xaml_controls::IItemsPresenter** ppItemsPresenter);

        // ItemsControlGenerated overrides
        IFACEMETHOD(get_Items)(
            _Outptr_ wfc::IObservableVector<IInspectable*>** pValue) override;

        // Properties.
        _Check_return_ HRESULT get_ItemContainerGeneratorImpl(
            _Outptr_ xaml_controls::IItemContainerGenerator** pValue);

        _Check_return_ HRESULT get_GroupStyleImpl(
            _Outptr_ wfc::IObservableVector<xaml_controls::GroupStyle*>** pValue);

        // ItemsControlGenerated methods implementation
        _Check_return_ HRESULT IsItemItsOwnContainerOverrideImpl(
            _In_ IInspectable* item,
            _Out_ BOOLEAN* returnValue);

        _Check_return_ HRESULT GetContainerForItemOverrideImpl(
            _Outptr_ xaml::IDependencyObject** returnValue);

        // Cleans up the given container previously set up in PrepareContainerForItemOverride.
        // Will remove ItemContainerStyle if the container is not internally generated, and had this style previously set.
        _Check_return_ HRESULT ClearContainerForItemOverrideImpl(
            _In_ xaml::IDependencyObject* element,
            _In_ IInspectable* item);

        _Check_return_ HRESULT PrepareContainerForItemOverrideImpl(
            _In_ xaml::IDependencyObject* element,
            _In_ IInspectable* item);

        // workaround for not having IVectorChangedEventArgs in IDL
        virtual _Check_return_ HRESULT OnItemsChangedImpl(_In_ IInspectable* e);

        _Check_return_ HRESULT OnItemContainerStyleChangedImpl(
            _In_ xaml::IStyle* oldItemContainerStyle,
            _In_ xaml::IStyle* newItemContainerStyle);

        _Check_return_ HRESULT OnItemContainerStyleSelectorChangedImpl(
            _In_ xaml_controls::IStyleSelector* oldItemContainerStyleSelector,
            _In_ xaml_controls::IStyleSelector* newItemContainerStyleSelector);

        _Check_return_ HRESULT OnItemTemplateChangedImpl(
            _In_ xaml::IDataTemplate* oldItemTemplate,
            _In_ xaml::IDataTemplate* newItemTemplate);

        _Check_return_ HRESULT OnItemTemplateSelectorChangedImpl(
            _In_ xaml_controls::IDataTemplateSelector* oldItemTemplateSelector,
            _In_ xaml_controls::IDataTemplateSelector* newItemTemplateSelector);

        _Check_return_ HRESULT OnGroupStyleSelectorChangedImpl(
            _In_ xaml_controls::IGroupStyleSelector* oldGroupStyleSelector,
            _In_ xaml_controls::IGroupStyleSelector* newGroupStyleSelector);

        _Check_return_ HRESULT ItemFromContainerImpl(
            _In_ xaml::IDependencyObject* container,
            _Outptr_ IInspectable** returnValue);

        _Check_return_ HRESULT ContainerFromItemImpl(
            _In_opt_ IInspectable* item,
            _Outptr_ xaml::IDependencyObject** returnValue);

        _Check_return_ HRESULT IndexFromContainerImpl(
            _In_ xaml::IDependencyObject* container,
            _Out_ INT* returnValue);

        _Check_return_ HRESULT ContainerFromIndexImpl(
            _In_ INT index,
            _Outptr_ xaml::IDependencyObject** returnValue);

        _Check_return_ HRESULT GroupHeaderContainerFromItemContainerImpl(
            _In_ xaml::IDependencyObject* pItemContainer,
            _Outptr_result_maybenull_ xaml::IDependencyObject** ppReturnValue);

        _Check_return_ HRESULT GroupFromHeaderImpl(_In_ xaml::IDependencyObject* header, _Outptr_ IInspectable** returnValue);
        _Check_return_ HRESULT HeaderFromGroupImpl(_In_ IInspectable* group, _Outptr_ xaml::IDependencyObject** returnValue);
        _Check_return_ HRESULT IndexFromHeaderImpl(_In_ xaml::IDependencyObject* header, _In_ BOOLEAN excludeHiddenEmptyGroups, _Out_ INT* returnValue);
        _Check_return_ HRESULT HeaderFromIndexImpl(_In_ INT index, _Outptr_ xaml::IDependencyObject** returnValue);


        // Calls ChangeVisualState on all child SelectorItems (including items inside a GroupItem),
        // with optimizations for the virtualization provided by IOrientedVirtualizingPanel.
        virtual _Check_return_ HRESULT ChangeSelectorItemsVisualState(
            _In_ bool bUseTransitions);

        // This code gets called from Automation Provider e.g. ItemAutomationPeer and must not be called from any
        // internal APIs in the control itself. It basically returns the Container for the DataItem in case it exist.
        virtual _Check_return_ HRESULT UIA_GetContainerForDataItemOverride(
            _In_opt_ IInspectable* pItem,
            _In_ INT itemIndex,
            _Outptr_ xaml::IUIElement** ppContainer);

        static _Check_return_ HRESULT GetItemsOwner(
            _In_ xaml::IDependencyObject* element,
            _Outptr_result_maybenull_ xaml_controls::IItemsControl** returnValue);

        static _Check_return_ HRESULT GetItemsOwner(
            _In_ xaml::IDependencyObject* element,
            _In_ bool ignopreGrouping,
            _Outptr_ xaml_controls::IItemsControl** returnValue);

        static _Check_return_ HRESULT ItemsControlFromItemContainer(
            _In_ xaml::IDependencyObject* container,
            _Outptr_result_maybenull_ xaml_controls::IItemsControl** returnValue);

    //Callbacks
    public:
        static _Check_return_ HRESULT SetItemCollectionStatic(
            _In_ CItemCollection* pNativeItemCollection,
            _In_ CItemsControl* pNativeItemsControl);

        static _Check_return_ HRESULT ClearVisualChildren(
            _In_ CItemsControl* pNativeItemsControl,
            _In_ bool bHostIsReplaced);

        static _Check_return_ HRESULT DisplayMemberPathChanged(
            _In_ CItemsControl* pNativeItemsControl);

        static _Check_return_ HRESULT RecreateVisualChildren(
            _In_ CItemsControl* pNativeItemsControl);

        static _Check_return_ HRESULT NotifyAllItemsAdded(
            _In_ CItemsControl* pNativeItemsControl);

    public:

        #pragma region IGeneratorHost interface

        _Check_return_ IFACEMETHOD(get_View)(
            _Outptr_ wfc::IVector<IInspectable*>** ppView) override;

        _Check_return_ IFACEMETHOD(get_CollectionView)(
                _Outptr_ xaml_data::ICollectionView** ppCollectionView) override;

        _Check_return_ IFACEMETHOD(IsItemItsOwnContainer)(
            _In_ IInspectable* pItem,
            _Out_ BOOLEAN* pIsOwnContainer) override;

        _Check_return_ IFACEMETHOD(GetContainerForItem)(
            _In_ IInspectable* pItem,
            _In_opt_ xaml::IDependencyObject* pRecycledContainer,
            _Outptr_ xaml::IDependencyObject** ppContainer) override;

        _Check_return_ IFACEMETHOD(PrepareItemContainer)(
            _In_ xaml::IDependencyObject* pContainer,
            _In_ IInspectable* pItem) override;

        _Check_return_ IFACEMETHOD(ClearContainerForItem)(
            _In_ xaml::IDependencyObject* pContainer,
            _In_ IInspectable* pItem) override;

        _Check_return_ IFACEMETHOD(IsHostForItemContainer)(
            _In_ xaml::IDependencyObject* pContainer,
            _Out_ BOOLEAN* pIsHost) override;

        _Check_return_ IFACEMETHOD(GetGroupStyle)(
            _In_opt_ xaml_data::ICollectionViewGroup* pGroup,
            _In_ UINT level,
            _Out_ xaml_controls::IGroupStyle** ppGroupStyle) override;

        _Check_return_ IFACEMETHOD(SetIsGrouping)(_In_ BOOLEAN isGrouping) override;

        // we don't expose this publicly, there is an override for our own controls
        // to mirror the public api
        _Check_return_ IFACEMETHOD(GetHeaderForGroup)(
            _In_ IInspectable* pGroup,
            _Outptr_ xaml::IDependencyObject** ppContainer) override;

        _Check_return_ IFACEMETHOD(PrepareGroupContainer)(
            _In_ xaml::IDependencyObject* pContainer,
            _In_ xaml_data::ICollectionViewGroup* pGroup) override;

        _Check_return_ IFACEMETHOD(ClearGroupContainerForGroup)(
            _In_ xaml::IDependencyObject* pContainer,
            _In_opt_ xaml_data::ICollectionViewGroup* pItem) override;

        _Check_return_ IFACEMETHOD(SetupContainerContentChangingAfterPrepare)(
            _In_ xaml::IDependencyObject* pContainer,
            _In_ IInspectable* pItem,
            _In_ INT itemIndex,
            _In_ wf::Size measureSize) override { RRETURN(S_OK); }

        _Check_return_ IFACEMETHOD(RegisterWorkFromArgs)(
            _In_ xaml_controls::IContainerContentChangingEventArgs* pArgs) override { RRETURN(S_OK); }

        _Check_return_ IFACEMETHOD(RegisterWorkForContainer)(
            _In_ xaml::IUIElement* pContainer) override {  RRETURN(S_OK); }

        _Check_return_ IFACEMETHOD(CanRecycleContainer)(
            _In_ xaml::IDependencyObject* pContainer,
            _Out_ BOOLEAN* pCanRecycleContainer) override;

        // During lookups of duplicate or null values, there might be a container that
        // the host can provide the ICG.
        _Check_return_ IFACEMETHOD(SuggestContainerForContainerFromItemLookup)(
            _Outptr_ xaml::IDependencyObject** ppContainer) override;

        _Check_return_ IFACEMETHOD(ShouldRaiseChoosingItemContainer)(
            _Out_ BOOLEAN* pShouldRaiseChoosingItemContainer) override { *pShouldRaiseChoosingItemContainer = false; RRETURN(S_OK); }

        _Check_return_ IFACEMETHOD(RaiseChoosingItemContainer)(
            _In_ xaml_controls::IChoosingItemContainerEventArgs* pArgs) override { RRETURN(S_OK); }

        _Check_return_ IFACEMETHOD(ShouldRaiseChoosingGroupHeaderContainer)(
            _Out_ BOOLEAN* pShouldRaiseChoosingGroupHeaderContainer) override { *pShouldRaiseChoosingGroupHeaderContainer = false; return S_OK; }

        _Check_return_ IFACEMETHOD(RaiseChoosingGroupHeaderContainer)(
            _In_ xaml_controls::IChoosingGroupHeaderContainerEventArgs* pArgs) override { RRETURN(S_OK); }

        _Check_return_ IFACEMETHOD(RaiseContainerContentChangingOnRecycle)(
            _In_ xaml::IUIElement* pContainer,
            _In_ IInspectable* pItem) override { RRETURN(S_OK); }

        _Check_return_ IFACEMETHOD(VirtualizationFinished)() override { return S_OK; }

        _Check_return_ IFACEMETHOD(OverrideContainerArrangeBounds)(
            _In_ INT index,
            _In_ wf::Rect suggestedBounds,
            _Out_ wf::Rect* newBounds) override
        {
            *newBounds = suggestedBounds;
            return S_OK;
        }

        #pragma endregion

        _Check_return_ HRESULT put_IsGrouping(_In_ BOOLEAN value) override;

        _Check_return_ HRESULT GetItemOrContainerFromContainer(
            _In_ IDependencyObject* pContainer,
            _Outptr_ IInspectable** pItem);

        // ITransitionContextProvider interface

        // Get currentTransitionContext
        // This method returns information regarding recent activities on ListView/GridView Control,
        // Activities are whether the items get added/removed/reordered or content changed.
        // This method returns the above information as AnimationContext which is used by PVL
        // to identify which animation to run
        virtual _Check_return_ HRESULT GetCurrentTransitionContext(
            _In_ INT layoutTickId,
            _Out_ ThemeTransitionContext* returnValue);

        virtual _Check_return_ HRESULT GetDropOffsetToRoot(_Out_ wf::Point* returnValue);

        // Called by a Panel to force IsVirtualization
        _Check_return_ HRESULT SetVirtualizationStateByPanel();

        _Check_return_ HRESULT GetRecyclingContext(_Outptr_ DirectUI::IContainerRecyclingContext ** ppReturnValue);

        // publicly exposes itemshost to the world
        _Check_return_ HRESULT get_ItemsPanelRootImpl(_Outptr_ xaml_controls::IPanel** pValue);

        // IContainerRecyclingContext implementation (mostly redirected to DataTemplateSelectorRecyclingContext class)
        _Check_return_ HRESULT get_SelectedContainerImpl(_Outptr_ xaml::IUIElement** pValue);
        _Check_return_ HRESULT put_SelectedContainerImpl(_In_opt_ xaml::IUIElement* value);
        _Check_return_ HRESULT ConfigureSelectedContainerImpl(_In_ xaml::IUIElement* container);
        _Check_return_ HRESULT PrepareForItemRecyclingImpl(_In_opt_ IInspectable* item);
        _Check_return_ HRESULT IsCompatibleImpl(_In_ xaml::IUIElement* candidate, _Out_ BOOLEAN* returnValue);
        _Check_return_ HRESULT StopRecyclingImpl();

    protected:
        IFACEMETHOD(OnDisconnectVisualChildren)() override;

        virtual _Check_return_ HRESULT NotifyOfSourceChanged(
            _In_ wfc::IObservableVector<IInspectable*>* pSender,
            _In_ wfc::IVectorChangedEventArgs* e);

        virtual _Check_return_ HRESULT OnItemsChanged(
            _In_ wfc::IVectorChangedEventArgs* e);

        virtual _Check_return_ HRESULT GetHeaderForGroupOverrideImpl(
            _Outptr_ xaml::IDependencyObject** returnValue);

    private:
        _Check_return_ HRESULT OnGeneratorItemsChanged(
            _In_ IInspectable* pSender,
            _In_ xaml_primitives::IItemsChangedEventArgs* e);

        _Check_return_ HRESULT OnItemContainerStyleChanged(
            _In_ IInspectable* pOldValue,
            _In_ IInspectable* pNewValue);

        _Check_return_ HRESULT OnItemContainerStyleSelectorChanged(
            _In_ IInspectable* pOldValue,
            _In_ IInspectable* pNewValue);

        _Check_return_ HRESULT OnItemTemplateChanged(
            _In_ IInspectable* pOldValue,
            _In_ IInspectable* pNewValue);

        _Check_return_ HRESULT OnItemTemplateSelectorChanged(
            _In_ IInspectable* pOldValue,
            _In_ IInspectable* pNewValue);

        _Check_return_ HRESULT OnGroupStyleSelectorChanged(
            _In_ IInspectable* pOldValue,
            _In_ IInspectable* pNewValue);

        _Check_return_ HRESULT OnItemsPanelChanged(
            _In_ IInspectable* pOldValue,
            _In_ IInspectable* pNewValue);

        _Check_return_ HRESULT ClearContainers(
            _In_ BOOLEAN bHostIsReplaced);

        _Check_return_ HRESULT AddContainers();

        _Check_return_ HRESULT AddContainerForPosition(
            _In_ xaml_primitives::GeneratorPosition position);

        _Check_return_ HRESULT RemoveContainerForPosition(
            _In_ xaml_primitives::GeneratorPosition position);

        _Check_return_ HRESULT AddVisualChild(
            _In_ UINT nContainerIndex,
            _In_ xaml::IDependencyObject* pContainer,
            _In_ BOOLEAN bNeedPrepareContainer);

        _Check_return_ HRESULT OnGroupStyleChanged(
            _In_ wfc::IObservableVector<xaml_controls::GroupStyle*>* pSender,
            _In_ wfc::IVectorChangedEventArgs* e);

        // Used by ThemeTransitionContext to save previous TransitionContext
        ThemeTransitionContext m_themeTransitionContext;

        // Compute the TransitionContext
        _Check_return_ HRESULT GetCurrentTransitionContext(
            _Out_ ThemeTransitionContext* returnValue);

        static _Check_return_ HRESULT ItemsControlFromItemContainer(
            _In_ xaml::IDependencyObject* container,
            _In_ bool ignopreGrouping,
            _Outptr_ xaml_controls::IItemsControl** returnValue);

        _Check_return_ HRESULT SetItemCollection(_In_ DirectUI::ItemCollection* pItemCollection);

        // Create and/or swap set an ICG implementation depending on currently loaded panel
        _Check_return_ HRESULT InitializeItemContainerGenerator();

        _Check_return_ HRESULT RefreshContainers();

    protected:
        // constructor/destructor
        ItemsControl();
        ~ItemsControl() override;

        // Prepares object's state.
        _Check_return_ HRESULT PrepareState() override;

        // Supports the IGeneratorHost interface.
        _Check_return_ HRESULT QueryInterfaceImpl(
            _In_ REFIID iid,
            _Outptr_ void** ppObject) override;

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        // Applies ItemContainerStyle to the given container and item, if appropriate.
        // If the Style is applied and the container is not internally-generated, the Style
        // will be cleared on ClearContainerForItemOverride.
        virtual _Check_return_ HRESULT ApplyItemContainerStyle(
            _In_ xaml::IDependencyObject* pContainer,
            _In_ IInspectable* pItem);

        _Check_return_ HRESULT GetItemCount(
            _Out_ UINT& itemCount);

        //  When the ItemsSourceProperty is set or cleared, we update the internal _itemsSourceAsList,
        //  and notify the core ItemsControl.
        virtual _Check_return_ HRESULT OnItemsSourceChanged(
            _In_ IInspectable* pNewValue);

        // Helper to find out GroupItemIndex from GroupItem
        _Check_return_ HRESULT GetGroupItemIndex(
            _In_ IInspectable* groupItem,
            _Out_ UINT* pGroupItemIndex,
            _Out_ BOOLEAN* pFound);

        // Get our internal Mapping interfaces
        _Check_return_ HRESULT GetItemContainerMapping(_Outptr_ xaml_controls::IItemContainerMapping** ppReturnValue);
        _Check_return_ HRESULT GetGroupHeaderMapping(_Outptr_ DirectUI::IGroupHeaderMapping** ppReturnValue);

        // Any context related operation must be guarded with this call.
        // It will reset all context counters if we get new tick.
        // If the tick haven't changed then we keep increasing operation counters
        _Check_return_ HRESULT UpdateTickContextCounters();

        // callback from ReorderItems operation. We use it to update reorder counts for transitions context.
        _Check_return_ HRESULT OnItemsReordered(_In_ UINT nCount);

        // guaranteed to be called when we call prepare (even if base is not)
        virtual _Check_return_ HRESULT PreProcessContentPreparation(
            _In_ xaml::IDependencyObject* pContainer,
            _In_opt_ IInspectable* pItem) { RRETURN(S_OK); }

        // indicates that we should defer setting the content
        virtual _Check_return_ HRESULT IsPrepareContainerForItemDeferred(
            _In_ xaml::IDependencyObject* element,
            _Out_ BOOLEAN* pDefer)
        { *pDefer = FALSE; RRETURN(S_OK); }

        _Check_return_ HRESULT NotifyDeferredElementStateChanged(
            _In_ KnownPropertyIndex propertyIndex,
            _In_ DeferredElementStateChange state,
            _In_ UINT32 collectionIndex,
            _In_ CDependencyObject* realizedElement) override;

        virtual _Check_return_ HRESULT OnItemsHostAvailable();

        void TraceVirtualizationEnabledByModernPanel();

    private:
        // This helper class implements the selection of a recycling candidate based on its SelectedDataTemplate
        // As of today, ItemsControl simply delegates the corresponding calls to its unique
        // DataTemplateSelectorRecyclingContext member
        // (We are using a separate class as the lifecycle of private fields is the preparation of a single
        //  item and hence different from the ItemsControl itself. Anyway, as item's preparations are run
        //  on a single thread, we do not need to create separate instances and prefer to avoid the associated
        //  performance cost)
        // A recycling cycle is started with PrepareForItemRecycling and terminated with StopRecycling
        // (or the preparation of another item)
        class DataTemplateSelectorRecyclingContext
        {
        public:
            DataTemplateSelectorRecyclingContext(_In_ ItemsControl* pOwner);
            ~DataTemplateSelectorRecyclingContext();

            void SetDataTemplateSelector(_In_opt_ xaml_controls::IDataTemplateSelector* const pSelector);

            // Methods from IContainerRecyclingContext
            // Properties.
            _Check_return_ HRESULT get_SelectedContainer(_Outptr_ xaml::IUIElement** value);
            _Check_return_ HRESULT put_SelectedContainer(_In_ xaml::IUIElement* value);

            // Events.

            // Methods.
            _Check_return_ HRESULT PrepareForItemRecycling(_In_opt_ IInspectable* item);
            _Check_return_ HRESULT IsCompatible(_In_ xaml::IUIElement* candidate, _Out_ BOOLEAN* pReturnValue);
            _Check_return_ HRESULT StopRecycling();
            _Check_return_ HRESULT ConfigureSelectedContainer(_In_ xaml::IUIElement* container);
        private:
            // Lifetime of DataTemplateSelectorRecyclingContext is identical to its owner
            ItemsControl* m_pOwner;
            // The TrackerPtr references are set by the owner which is a ItemsControl and therefore will be walked through it
            TrackerPtr<xaml_controls::IDataTemplateSelector> m_tpSelector;
            TrackerPtr<xaml::IUIElement> m_tpSelectedContainer;
            TrackerPtr<xaml::IDataTemplate> m_tpSelectedTemplate;
            TrackerPtr<IInspectable> m_tpDataItem;
        };
        DataTemplateSelectorRecyclingContext m_dataSelectorRecyclingContext;

        ctl::EventPtr<VectorChangedEventCallback> m_epItemCollectionVectorChangedHandler;
        EventRegistrationToken m_ItemsChangedToken;
        EventRegistrationToken m_GroupStyleChangedToken;

        TrackerPtr<DirectUI::ItemCollection> m_tpItems;
        TrackerPtr<xaml_controls::IItemContainerGenerator> m_tpGenerator;
        TrackerPtr<xaml_controls::IItemContainerMapping> m_tpMapping;
        TrackerPtr<DirectUI::IGroupHeaderMapping> m_tpGroupMapping;
        TrackerPtr<wfc::IObservableVector<xaml_controls::GroupStyle*>> m_tpGroupStyle;
        TrackerPtr<xaml::IDataTemplate> m_tpDisplayMemberTemplate;
        TrackerPtr<xaml_controls::IPanel> m_tpItemsHost;

        // Used by ThemeTransitionContext to compute Added elements count
        UINT32 m_elementCountAddedThisLayoutTick;

        // Used by ThemeTransitionContext to compute Removed elements count
        UINT32 m_elementCountRemovedThisLayoutTick;

        // Used by ThemeTransitionContext to save previous TickCounter
        INT32 m_previousTickCounterId;

        // Used by ThemeTransitionContext to save Loaded Tick
        INT32 m_loadedTick;

        // Used by ThemeTransitionContext to find whether context is changed or not
        BOOLEAN m_resetItemsThisLayoutTick;

        // Used by ThemeTransitionContext to compute Reordered elements count
        INT32 m_elementCountReorderedThisLayoutTick;

        // Did ItemsPanel set the IsVirtualizing property?
        BOOLEAN m_isVirtualizingPropertySetByPanel;

        // Are we in grouping mode?
        BOOLEAN m_isGrouping;
    };
}
