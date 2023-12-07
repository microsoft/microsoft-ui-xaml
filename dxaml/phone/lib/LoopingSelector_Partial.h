// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { namespace Primitives
{

    class LoopingSelector :
        public LoopingSelectorGenerated
    {

    public:
        LoopingSelector();

        _Check_return_ HRESULT OnPropertyChanged(
            _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs);

        // UIA functions
        _Check_return_ HRESULT AutomationGetIsScrollable(_Out_ BOOLEAN* pIsScrollable);
        _Check_return_ HRESULT AutomationGetScrollPercent(_Out_ DOUBLE* pScrollPercent);
        _Check_return_ HRESULT AutomationGetScrollViewSize(_Out_ DOUBLE* pScrollPercent);
        _Check_return_ HRESULT AutomationSetScrollPercent(_In_ DOUBLE scrollPercent);
        _Check_return_ HRESULT AutomationTryGetSelectionUIAPeer(_Outptr_result_maybenull_ xaml::Automation::Peers::IAutomationPeer** ppPeer);
        _Check_return_ HRESULT AutomationScroll(_In_ xaml::Automation::ScrollAmount scrollAmount);
        _Check_return_ HRESULT AutomationFillCollectionWithRealizedItems(_In_ wfc::IVector<IInspectable*> *pVector);
        _Check_return_ HRESULT AutomationGetContainerUIAPeerForItem(
            _In_ IInspectable* pItem,
            _Outptr_result_maybenull_ xaml_automation_peers::ILoopingSelectorItemAutomationPeer** ppPeer);
        _Check_return_ HRESULT AutomationTryScrollItemIntoView(_In_ IInspectable* pItem);
        _Check_return_ HRESULT AutomationClearPeerMap();
        _Check_return_ HRESULT AutomationRealizeItemForAP(_In_ UINT32 itemIdxToRealize);

        // Used by LoopingSelectorItem to ScrollIntoView/Select
        _Check_return_ HRESULT AutomationScrollToVisualIdx(_In_ INT visualIdx, bool ignoreScrollingState = false);
        _Check_return_ HRESULT AutomationGetSelectedItem(_Outptr_result_maybenull_ LoopingSelectorItem** ppItemNoRef);

        _Check_return_ HRESULT VisualIndexToItemIndex(_In_ UINT32 visualIndex, _Out_ UINT32* itemIndex);

    protected:

        // IFrameworkElementOverrides
        _Check_return_ HRESULT MeasureOverrideImpl(wf::Size availableSize, _Out_ wf::Size* returnValue) override;
        _Check_return_ HRESULT ArrangeOverrideImpl(wf::Size finalSize, _Out_opt_ wf::Size* returnValue) override;
        _Check_return_ HRESULT OnApplyTemplateImpl() override;

        // IUIElementOverrides
        _Check_return_ HRESULT OnCreateAutomationPeerImpl(
            _Outptr_ xaml::Automation::Peers::IAutomationPeer **returnValue) override;

        // IControlOverrides
        _Check_return_ HRESULT OnKeyDownImpl(_In_ xaml_input::IKeyRoutedEventArgs* pEventArgs) override;

    private:
        enum ListEnd
        {
            Head,
            Tail
        };

        enum ItemState
        {
            ManipulationInProgress,
            Expanded,
            LostFocus
        };

        ~LoopingSelector() {}

        _Check_return_ HRESULT InitializeImpl() override;

        // Template-part related state
        EventRegistrationToken _viewChangedToken;
        EventRegistrationToken _viewChangingToken;
        EventRegistrationToken _pressedToken;
        EventRegistrationToken _focusLost;
        EventRegistrationToken _focusGot;
        EventRegistrationToken _pointerEnteredToken;
        EventRegistrationToken _pointerExitedToken;
        EventRegistrationToken _upButtonClickedToken;
        EventRegistrationToken _downButtonClickedToken;

        static const WCHAR c_scrollViewerTemplatePart[];
        static const WCHAR c_upButtonTemplatePart[];
        static const WCHAR c_downButtonTemplatePart[];
        static const INT c_automationLargeIncrement = 5;
        static const INT c_automationSmallIncrement = 1;
        static const DOUBLE c_targetScreenWidth;

        Private::TrackerPtr<xaml_primitives::ILoopingSelectorPanel> _tpPanel;
        Private::TrackerPtr<xaml_controls::IScrollViewer> _tpScrollViewer;
        Private::TrackerPtr<xaml_controls::IScrollViewerPrivate> _tpScrollViewerPrivate;
        Private::TrackerPtr<xaml_controls::Primitives::IButtonBase> _tpUpButton;
        Private::TrackerPtr<xaml_controls::Primitives::IButtonBase> _tpDownButton;

        // We keep a weak ref to the AP when it is created in order to be able
        // to update the map of items to data automation peers it maintains.
        wrl::WeakRef _wrAP;

        // We subscribe to the routed Got/LostFocus events. Focus moving between
        // subelements generates both a Lost and Got event. This boolean
        // tracks whether we actually have focus. When a manipulation begins
        // it does not always trigger a focus event (framework bug?). If the
        // control does not have focus it programatically forces focus.
        BOOLEAN _hasFocus;

        // Indicates whether the panel is sized. Reset
        // when the Items or ShouldLoop property is changed, or the control is
        // resized.
        BOOLEAN _isSized;

        // Indicates whether there is a pending setup operation. This operation
        // sets up realized bounds, realization starting idx, and the scroll
        // position.
        BOOLEAN _isSetupPending;

        // Before the first layout pass the ScrollViewer isn't fully
        // initialized and calls to ScrollToOffsetWithOptionalAnimation
        // produce no effect. We prevent initialization of the ScrollViewer's
        // scroll position and the realization of items until after it has
        // been fully initialized.
        BOOLEAN _isScrollViewerInitialized;

        // Normalization calls SetScrollPosition to jump the viewport back
        // to the center of the scrollable region. This causes an extra ViewChanged
        // event to occur. Because Normalization doesn't affect the itme Balance and
        // a balance had to of just occurred we skip the next balance as an optimization.
        BOOLEAN _skipNextBalance;

        // When using UI automation, we want to be able to scroll without selecting.
        // We'll use this to temporarily disable updating the selected item
        // while scrolling using UI automation.
        bool _skipSelectionChangeUntilFinalViewChanged;

        // When the ScrollViewer first initializes it invalidates its layout when
        // first setting up the ScrollContentPresenter. We skip the next arrange
        // as an optimization.
        BOOLEAN _skipNextArrange;

        // Item state keeps track of the visual state of all the items. This
        // allows us to properly transition between expanded and normal modes.
        ItemState _itemState;

        // Where the viewport currently sits. Kept as object state because
        // ScrollViewer's extents will become inaccurate if a SetScrollPosition
        // has been called but an invalidate scroll info pass (internal to ScrollViewer)
        // hasn't occurred. These values are kept accurate in this situation.
        DOUBLE _unpaddedExtentTop;
        DOUBLE _unpaddedExtentBottom;

        // These values are updated as we add and remove items and serve
        // as the record of where our UI ELements are laid out.
        DOUBLE _realizedTop;
        DOUBLE _realizedBottom;

        // The top and bottom index don't loop. They are a purely
        // visual index, which is modded with the logical item count
        // to obtain a logical index.
        INT32 _realizedTopIdx;
        INT32 _realizedBottomIdx;

        // We store the visual index to ensure in cases where
        // multiple logical items are displayed on the screen
        // that more than one is not in the visual 'Selected'
        // state.
        INT32 _realizedMidpointIdx;

        // We cache this values so we're not continuously
        // looking them up.
        UINT32 _itemCount;
        DOUBLE _scaledItemHeight;
        DOUBLE _itemHeight;
        DOUBLE _itemWidth;

        // If the ItemWidth property is not explicitly set, we fall back to a computed value equal to the width of the LoopingSelector.
        // This allows the LoopingSelectorItems to stretch to the available space if there is no explict width given.
        // Ordinarily, it is not necessary to set the Width of a UIElement if the desired behavior is for it to stretch to the available
        // space, but due to the fact that the LoopingSelectorPanel is a Canvas we always need to specify a width for the LoopingSelectorItems.
        DOUBLE _itemWidthFallback;

        // The panel size when calculated in EnsureSetup.
        DOUBLE _panelSize;

        // This isn't just _panelSize/2, it is snap point aligned.
        DOUBLE _panelMidpointScrollPosition;

        // Used to guard against Balencing and other operations.
        // When SetScrollPosition is called it will synchronously flush
        // pending ViewChanging events.
        BOOLEAN _isWithinScrollChange;

        // Used to guard against Balencing and other operations when we are in
        // ArrangeOverride.
        BOOLEAN _isWithinArrangeOverride;

        // Prevents reaction to setting the index property in response to a
        // manipulation.
        BOOLEAN _disablePropertyChange;

        // Used to detect orientation changes.
        FLOAT _lastArrangeSizeHeight;

        // When moving from a small viewport to a large viewport during an orientation change,
        // the viewport vertical offset might get coerced and change. If that's the case,
        // we defer the scroll operation to the next layout pass because it's too late by then.
        DOUBLE _delayScrollPositionY;

        // Track LSIs. All LSIs are in the Canvas's children, but recycled
        // LSIs are kept far offscreen.
        std::vector<xaml_primitives::ILoopingSelectorItem*> _recycledItems;
        std::vector<xaml_primitives::ILoopingSelectorItem*> _realizedItems;

        // LoopingSelectorAutomationPeer asks for items to be realized, but they're not brought into the canvas.
        // This map, indexed with the modded visual index, keeps track of those
        std::map<int, wrl::ComPtr<ILoopingSelectorItem>> _realizedItemsForAP;

        // Automation cached values for property changed events.
        Private::TrackerPtr<IInspectable> _tpPreviousScrollPosition;

        wrl::EventSource<xaml_controls::ISelectionChangedEventHandler> _selectionChangedEventSource;

        // Cached instance of the ICanvasStatics interface for performance
        // sensative operations.
        wrl::ComPtr<xaml_controls::ICanvasStatics> _spCanvasStatics;

        _Check_return_ HRESULT OnViewChanged(_In_ IInspectable* pSender, _In_ xaml_controls::IScrollViewerViewChangedEventArgs* pEventArgs);
        _Check_return_ HRESULT OnViewChanging(_In_ IInspectable* pSender, _In_ xaml_controls::IScrollViewerViewChangingEventArgs* pEventArgs);
        _Check_return_ HRESULT OnPressed(_In_ IInspectable* pSender, _In_ xaml::Input::IPointerRoutedEventArgs* pEventArgs);
        _Check_return_ HRESULT OnGotFocus(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pEventArgs);
        _Check_return_ HRESULT OnLostFocus(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pEventArgs);
        _Check_return_ HRESULT OnItemTapped(_In_ IInspectable* pSender, _In_ xaml_input::ITappedRoutedEventArgs* pEventArgs);
        _Check_return_ HRESULT OnPointerEntered(_In_ IInspectable* pSender, _In_ xaml_input::IPointerRoutedEventArgs* pEventArgs);
        _Check_return_ HRESULT OnPointerExited(_In_ IInspectable* pSender, _In_ xaml_input::IPointerRoutedEventArgs* pEventArgs);
        _Check_return_ HRESULT OnUpButtonClicked(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pEventArgs);
        _Check_return_ HRESULT OnDownButtonClicked(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pEventArgs);

        _Check_return_ HRESULT RaiseOnSelectionChanged(_In_ IInspectable* pOldItem, _In_ IInspectable* pNewItem);
        _Check_return_ HRESULT IsTemplateAndItemsAttached(_Out_ BOOLEAN* shouldBalance);
        _Check_return_ HRESULT IsSetupForAutomation(_Out_ BOOLEAN* isSetup);

        _Check_return_ HRESULT GoToState(_In_ HSTRING strState, _In_ BOOLEAN useTransitions);

        // Balance is the entry point for virtualization maintenance, calling
        // Normalize, EnsureSetup, and UpdateSelectedItem when needed.
        _Check_return_ HRESULT Balance(_In_ BOOLEAN isOnSnapPoint);

        // SetSelectedIndex handled the programatic changing of the
        // index.
        _Check_return_ HRESULT SetSelectedIndex(_In_ INT32 oldIdx, _In_ INT32 newIdx);

        // Change the selected item
        _Check_return_ HRESULT SelectNextItem();
        _Check_return_ HRESULT SelectPreviousItem();

        _Check_return_ HRESULT HandlePageDownKeyPress();
        _Check_return_ HRESULT HandlePageUpKeyPress();
        _Check_return_ HRESULT HandleEndKeyPress();
        _Check_return_ HRESULT HandleHomeKeyPress();

        _Check_return_ HRESULT UpdateSelectedItem(bool ignoreScrollingState = false);
        _Check_return_ HRESULT Normalize();
        _Check_return_ HRESULT EnsureSetup();

        // Helper automation functions
        _Check_return_ HRESULT AutomationRaiseStructureChanged();
        _Check_return_ HRESULT AutomationRaiseExpandCollapse();
        _Check_return_ HRESULT AutomationRaiseSelectionChanged();

        // Helper functions
        _Check_return_ HRESULT ClearAllItems();
        _Check_return_ HRESULT HasFocus(_Out_ BOOLEAN* pHasFocus);
        _Check_return_ HRESULT IsAscendantOfTarget(_In_ xaml::IDependencyObject* pChild, _Out_ BOOLEAN* pIsChildOfTarget);
        _Check_return_ HRESULT SizePanel();
        _Check_return_ HRESULT SetScrollPosition(_In_ DOUBLE offset, _In_ BOOLEAN useAnimation);
        _Check_return_ HRESULT GetPanelChildren(_Outptr_ wfc::IVector<xaml::UIElement*>** ppChildren, _Out_ UINT32* count);
        _Check_return_ HRESULT MeasureExtent(_Out_ DOUBLE* extentTop, _Out_ DOUBLE* extentBottom);
        _Check_return_ HRESULT Trim(_In_ ListEnd end);
        _Check_return_ HRESULT Add(_In_ ListEnd end);
        _Check_return_ HRESULT TransitionItemsState(_In_ ItemState state);
        _Check_return_ HRESULT UpdateVisualSelectedItem(_In_ UINT32 oldIdx, _In_ UINT32 newIdx);
        _Check_return_ HRESULT RealizeItem(_In_ UINT32 itemIdxToRealize, _Outptr_ ILoopingSelectorItem** ppItem);
        _Check_return_ HRESULT RecycleItem(_In_ ILoopingSelectorItem* pItem);
        _Check_return_ HRESULT SetPosition(_In_ xaml::IUIElement* pItem, _In_ DOUBLE offset);
        _Check_return_ HRESULT ShiftChildren(_In_ DOUBLE delta);
        _Check_return_ HRESULT SetupSnapPoints(_In_ DOUBLE offset, _In_ DOUBLE size);
        _Check_return_ HRESULT GetMaximumAddIndexPosition(_Out_ INT32* headIdx, _Out_ INT32* tailIdx);

        _Check_return_ HRESULT ExpandIfNecessary();

        _Check_return_ HRESULT RetreiveItemFromAPRealizedItems(_In_ UINT32 moddeItemdIdx, _Outptr_result_maybenull_ ILoopingSelectorItem** ppItem);

        // Mods negative numbers in a way that makes sense
        // for generating an index position from an infinite
        // sequence.
        UINT32 PositiveMod(INT32 x, INT32 n)
        {
            return (x%n + n) % n;
        }

        _Check_return_ HRESULT RequestInteractionSound(xaml::ElementSoundKind soundKind);
    };

    ActivatableClassWithFactory(LoopingSelector, LoopingSelectorFactory);

}

} } } } XAML_ABI_NAMESPACE_END
