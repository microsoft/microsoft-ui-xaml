// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IGeneratorHost.g.h"
#include "Hub.g.h"
#include "DirectManipulationStateChangeHandler.h"
#include <fwd/windows.ui.core.h>

namespace DirectUI
{
    // Represents the class for Hub control.
    PARTIAL_CLASS(Hub)
        , public IGeneratorHost
    {

    private:
        // Storage for Hub.Sections.
        TrackerPtr<wfc::IVector<xaml_controls::HubSection*>> m_tpSections;

        TrackerPtr<ReadOnlyTrackerCollection<xaml_controls::HubSection*>> m_tpSectionsInView;

        ctl::EventPtr<FrameworkElementSizeChangedEventCallback> m_epHeaderSizeChangedHandler;

        TrackerPtr<xaml::IFrameworkElement> m_tpHeaderHostPart;
        TrackerPtr<xaml_controls::IPanel> m_tpPanel;
        TrackerPtr<xaml_controls::IScrollViewer> m_tpScrollViewer;

        DOUBLE m_hubHeaderHeight;

        // if Panel is not loaded when user call ScrollToSection, we need to cache the request and handle it when panel is loaded.
        TrackerPtr<xaml_controls::IHubSection> m_tpDeferredScrollToSection;

        BOOLEAN m_isPanelLoaded;

    protected:
        Hub();
        ~Hub() override;

        // Prepares object's state
        _Check_return_ HRESULT PrepareState() override;

        // Override the GetDefaultValue method to return the default values
        // for Hub dependency properties.
        _Check_return_ HRESULT GetDefaultValue2(
            _In_ const CDependencyProperty* pDP,
            _Out_ CValue* pValue) override;

        IFACEMETHOD(OnApplyTemplate)() override;

        // Handle the custom property changed event and call the OnPropertyChanged methods.
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        // Change to the correct visual state for the Hub.
        _Check_return_ HRESULT ChangeVisualState(
            // true to use transitions when updating the visual state, false
            // to snap directly to the new visual state.
            _In_ bool bUseTransitions) override;

        // Handle the release of the core object
        _Check_return_ HRESULT DisconnectFrameworkPeerCore() override;

        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);

    private:

        _Check_return_ HRESULT OnHeaderSizeChanged(
            _In_ IInspectable* pSender,
            _In_ xaml::ISizeChangedEventArgs* pArgs);

        _Check_return_ HRESULT RefreshHubSectionPlaceholderHeights();

        _Check_return_ HRESULT OnOrientationChanged();

    public:
        // Hub is the owner of CHubSectionCollection. when DOCollection changed, it will notify its owner by this virtual function.
        // so we don't need to subscribe to VectorChanged event of Hub.Sections.
        _Check_return_ HRESULT OnCollectionChanged(_In_ XUINT32 nCollectionChangeType, _In_ XUINT32 nIndex) override;

        DOUBLE GetHubHeaderHeight()
        {
            return m_hubHeaderHeight;
        }

        _Check_return_ HRESULT get_SectionsInViewImpl(
            _Outptr_ wfc::IVector<xaml_controls::HubSection*>** pValue);

        _Check_return_ HRESULT ScrollToSectionImpl(_In_ xaml_controls::IHubSection* section);

        static const xaml::FocusState FocusStateForDefaultSection;

        // Returns the FrameworkElement template part that hosts the Hub's Header.
        _Check_return_ HRESULT GetHeaderHostPart(_Outptr_result_maybenull_ xaml::IFrameworkElement** ppHeaderHost);

        // Returns the ScrollViewer template part that hosts the panel.
        _Check_return_ HRESULT GetScrollViewerPart(_Outptr_result_maybenull_ xaml_controls::IScrollViewer** ppScrollViewer);

        // Returns the Panel template part that hosts the HubSections.
        _Check_return_ HRESULT GetPanelPart(_Outptr_result_maybenull_ xaml_controls::IPanel** ppPanel);

        // if user calls ScrollToSection before Panel is loaded, cache the request and handle the request in PanelLoaded event (or when EntranceAnimation is done for Phone)
        _Check_return_ HRESULT ProcessDeferredScrollToRequest(_Out_ BOOLEAN* pProcessed);

    private:
        void SetHubHeaderHeight(DOUBLE height)
        {
            m_hubHeaderHeight = height;
        }

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
            _In_ xaml_data::ICollectionViewGroup* pItem) override;

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

        // ISemanticZoomInformation interface

        // Prepare the view for a zoom transition.
        _Check_return_ HRESULT InitializeViewChangeImpl();

        // Cleanup the view after a zoom transition.
        _Check_return_ HRESULT CompleteViewChangeImpl();

        // Forces content to scroll until the coordinate space of the
        // SemanticZoomItem is visible.
        _Check_return_ HRESULT MakeVisibleImpl(
            _In_ xaml_controls::ISemanticZoomLocation* pItem);

        // When this Hub is the active view and we're changing to
        // the other view, optionally provide the source and destination
        // items.
        _Check_return_ HRESULT StartViewChangeFromImpl(
            _In_ xaml_controls::ISemanticZoomLocation* pSource,
            _In_ xaml_controls::ISemanticZoomLocation* pDestination);

        // When this Hub is the inactive view and we're changing to
        // it, optionally provide the source and destination items.
        _Check_return_ HRESULT StartViewChangeToImpl(
            _In_ xaml_controls::ISemanticZoomLocation* pSource,
            _In_ xaml_controls::ISemanticZoomLocation* pDestination);

        // Complete the change to the other view when this Hub was
        // the active view.
        _Check_return_ HRESULT CompleteViewChangeFromImpl(
            _In_ xaml_controls::ISemanticZoomLocation* pSource,
            _In_ xaml_controls::ISemanticZoomLocation* pDestination);

        // Complete the change to make this Hub the active view.
        _Check_return_ HRESULT CompleteViewChangeToImpl(
            _In_ xaml_controls::ISemanticZoomLocation* pSource,
            _In_ xaml_controls::ISemanticZoomLocation* pDestination);
        // end ISemanticZoomInformation

        _Check_return_ HRESULT get_SectionHeadersImpl(
            _Outptr_ wfc::IObservableVector<IInspectable*>** pValue);

        _Check_return_ HRESULT RaiseSectionHeaderClick(_In_ HubSection* pSection);

        _Check_return_ HRESULT HandleNavigationKey(
            _In_ HubSection* pSection,
            _In_ wsy::VirtualKey key,
            _Out_ BOOLEAN* pWasHandled);

        // Callback for when the HubSection.Header property changes for one of the constituent HubSections.
        _Check_return_ HRESULT OnSectionHeaderChanged(_In_ xaml_controls::IHubSection* pSection);

        // Helper method transferring focus to a HubSection after a semantic zoom into the Hub.
        _Check_return_ HRESULT TransferSemanticZoomFocus(
            _In_ xaml_controls::IHubSection* pCurrentSection);

        // Checks whether a HubSection comes before m_destinationIndexForSemanticZoom.
        _Check_return_ HRESULT SectionComesBeforeDestinationForSemanticZoom(
            _In_ xaml_controls::IHubSection* pCurrentSection,
            _Out_ BOOLEAN* pComesBeforeDestinationSection);

        // Starts the timer that scrolls to the HubSection that is the destination of the semantic zoom.
        _Check_return_ HRESULT DelayScrollToSeZoDestination();

    protected:
        // Supports the IGeneratorHost interface.
        _Check_return_ HRESULT QueryInterfaceImpl(
            _In_ REFIID iid,
            _Outptr_ void** ppObject) override;

    private:
        // Helper method for keyboard navigation to move focus to an adjacent section.
        _Check_return_ HRESULT FocusNextSection(
            _In_ HubSection* pSection,
            _In_ BOOLEAN forwardDirection,
            _Out_ BOOLEAN* pWasHandled);

        _Check_return_ HRESULT CreateHubSectionRangeFromIndices(
            _In_ INT startIndex,
            _In_ INT endIndex,
            _Outptr_ wfc::IVector<xaml_controls::HubSection*>** ppVector);

        _Check_return_ HRESULT OnPanelVisibleIndicesChanged(IInspectable* pSource, IInspectable* pArgs);
        _Check_return_ HRESULT UpdateSectionsInView();

        _Check_return_ HRESULT RaiseSectionsInViewChanged(
            _In_ wfc::IVector<xaml_controls::HubSection*>* pSectionsRemoved,
            _In_ wfc::IVector<xaml_controls::HubSection*>* pSectionsAdded);

        // Quick check of visible sections in peek through area
        _Check_return_ HRESULT QuickCheckHaveVisibleWrappingSectionsInPeekThroughChanged(_Out_opt_ bool *pChanged);

        // The panel doesn't understand the wrapping game Hub plays, so it doesn't look for the "peek through"
        // sections when it tells us which items are in the visible window. We'll keep track of
        // which peekthrough sections are visible for the SectionsInView changing event
        _Check_return_ HRESULT GetVisibleWrappingSectionsInPeekThrough(
            _In_ std::map<UINT, ctl::ComPtr<xaml_controls::IHubSection>> &sectionsInViewMap);

        // Handler for the Loaded event on the m_tpPanel template part.
        // Scrolls the DefaultSection into view (if DefaultSectionIndex is set) and makes sure its header gets focus.
        _Check_return_ HRESULT OnPanelLoaded(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnPanelUnloaded(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        // Callback for m_tpSemanticZoomScrollIntoViewTimer's Tick event.
        _Check_return_ HRESULT OnSemanticZoomScrollIntoViewTimerTick();

    private:
        // Storage for the .Headers of Hub.Sections.  This collection is maintained in parallel with m_tpSections.
        // We expose it for the SeZo scenario - if Hub is the SeZo.ZoomedInView, then the SeZo.ZoomedOutView
        // is expected to often be an ItemsControl with items corresponding to the Headers of the HubSections.
        // Binding to this vector (exposed as Hub.SectionHeaders) makes that scenario easy to achieve.
        //
        // The collection is readonly because it's only meant to be bound to and read one way.
        //
        // HubSections with NULL .Header property will be omitted from this collection.
        //
        // For now, we are not supporting duplicate .Header values in this collection or in Hub's ISemanticZoomInformation.
        // When a HubSection is removed from m_tpSections, we simply remove the first element from m_tpSectionHeaders that
        // matches its .Header.
        // We may want to revisit and try to support that case in the future (i.e. having duplicate .Headers work for
        // our ISemanticZoomInformation implementation).
        TrackerPtr<ReadOnlyObservableTrackerCollection<IInspectable*>> m_tpSectionHeaders;

        // FocusState to use when focusing a HubSection header upon completing
        // a SeZo view change to this Hub.
        xaml::FocusState m_semanticZoomCompletedFocusState;

        EventRegistrationToken m_visibleIndicesUpdatedToken;

        // Loaded EventPtr for the m_tpPanel template part.
        ctl::EventPtr<FrameworkElementLoadedEventCallback> m_epPanelLoadedHandler;
        ctl::EventPtr<FrameworkElementUnloadedEventCallback> m_epPanelUnloadedHandler;

        // Index of the HubSection we are semantically zooming into.
        INT m_destinationIndexForSemanticZoom;

        // Timer used to dispatch a call to scroll the destination section of a semantic zoom operation into view.
        // This needs to be dispatched so that it takes precedence over the ScrollViewer's behavior to bring the
        // focused element into view.
        //
        // During a semantic zoom, the ScrollViewer will transfer focus and this will cause the ScrollViewer to scroll
        // the focused element into view.  Then, we will start this timer in order to dispatch a ScrollToSection() call
        // to come after the focused element is scrolled into view, so that the HubSection that is the destination section
        // of the semantic zoom ends up scrolled into view no matter where focus ends up.
        // Temporarily disabling scroll into view behavior on focus changes for ScrollViewer did not seem easily achievable,
        // so we are favoring this approach of dispatching ScrollToSection() to take precedence.
        TrackerPtr<xaml::IDispatcherTimer> m_tpSemanticZoomScrollIntoViewTimer;

        ctl::EventPtr<DispatcherTimerTickEventCallback> m_epSemanticZoomScrollIntoViewTimerTickHandler;
    };
}
