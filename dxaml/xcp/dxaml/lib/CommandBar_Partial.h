// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines the CommandBar control which subclasses AppBar to private
//      a more curated experience.

#pragma once

#include "CommandBar.g.h"
#include "AppBarSeparator_Partial.h"
#include "IterableWrappedCollection.h"

namespace DirectUI
{
    class CommandBarElementCollection;

    PARTIAL_CLASS(CommandBar)
    {
    public:
        IFACEMETHOD(OnApplyTemplate)() override;

        IFACEMETHOD(MeasureOverride)(
            _In_ wf::Size availableSize,
            _Out_ wf::Size* returnValue) override;

        IFACEMETHOD(ArrangeOverride)(
            _In_ wf::Size arrangeSize,
            _Out_ wf::Size* returnValue) override;

        _Check_return_ HRESULT NotifyElementVectorChanging(
            _In_ CommandBarElementCollection* pElementCollection,
            _In_ wfc::CollectionChange change,
            _In_ UINT32 changeIndex
            );

        _Check_return_ HRESULT ContainsElement(_In_ DependencyObject* pElement, _Out_ bool *pContainsElement) override;

        _Check_return_ HRESULT RegisterImpl();
        _Check_return_ HRESULT UnregisterImpl();

        _Check_return_ HRESULT HasFocus(_Out_ BOOLEAN* pHasFocus) override;

        _Check_return_ HRESULT CloseSubMenus(
            _In_opt_ xaml_controls::ISubMenuOwner* pMenuToLeaveOpen = nullptr,
            bool closeOnDelay = false);

        static _Check_return_ HRESULT OnCommandExecutionStatic(
            _In_ xaml_controls::ICommandBarElement* element);

        static _Check_return_ HRESULT OnCommandBarElementVisibilityChanged(
            _In_ xaml_controls::ICommandBarElement* element
            );

        static _Check_return_ HRESULT IsCommandBarElementInOverflow(
            _In_ xaml_controls::ICommandBarElement* element,
            _Out_ BOOLEAN* isInOverflow);

        static _Check_return_ HRESULT GetPositionInSetStatic(
            _In_ xaml_controls::ICommandBarElement* element,
            _Out_ INT* positionInSet);

        static _Check_return_ HRESULT GetSizeOfSetStatic(
            _In_ xaml_controls::ICommandBarElement* element,
            _Out_ INT* sizeOfSet);

        static _Check_return_ HRESULT FindParentCommandBarForElement(
            _In_ xaml_controls::ICommandBarElement* element,
            _Outptr_ xaml_controls::ICommandBar** parentCmdBar
            );

        // IMenu implementation
        _Check_return_ HRESULT get_ParentMenuImpl(_Outptr_result_maybenull_ xaml_controls::IMenu** ppValue);
        _Check_return_ HRESULT put_ParentMenuImpl(_In_opt_ xaml_controls::IMenu* pValue);

        _Check_return_ HRESULT CloseImpl();

    protected:
        CommandBar() = default;
        ~CommandBar() override;

        _Check_return_ HRESULT PrepareState() override;

        // Handle the release of the core object
        _Check_return_ HRESULT DisconnectFrameworkPeerCore() override;

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        _Check_return_ HRESULT ChangeVisualState(bool useTransitions) override;

        IFACEMETHOD(OnKeyDown)(_In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

        _Check_return_ HRESULT ProcessTabStopOverride(
            _In_opt_ DependencyObject* pFocusedElement,
            _In_opt_ DependencyObject* pCandidateTabStopElement,
            const bool isBackward,
            const bool didCycleFocusAtRootVisualScope,
            _Outptr_ DependencyObject** ppNewTabStop,
            _Out_ BOOLEAN* pIsTabStopOverridden
            ) override;

        _Check_return_ HRESULT ProcessCandidateTabStopOverride(
            _In_opt_ DependencyObject* pFocusedElement,
            _In_ DependencyObject* pCandidateTabStopElement,
            _In_opt_ DependencyObject* pOverriddenCandidateTabStopElement,
            const bool isBackward,
            _Outptr_ DependencyObject** ppNewTabStop,
            _Out_ BOOLEAN* pIsCandidateTabStopOverridden) override;

        _Check_return_ HRESULT EnterImpl(
            _In_ bool bLive,
            _In_ bool bSkipNameRegistration,
            _In_ bool bCoercedIsEnabled,
            _In_ bool bUseLayoutRounding
            ) override;

        _Check_return_ HRESULT RestoreSavedFocusImpl(_In_opt_ DependencyObject* savedFocusedElement, xaml::FocusState savedFocusState) override;
        _Check_return_ HRESULT UpdateTemplateSettings() override;
        _Check_return_ HRESULT GetVerticalOffsetNeededToOpenInDefaultDirection(_Out_ double* neededOffset, _Out_ bool* opensWindowed) override;

        _Check_return_ HRESULT NotifyDeferredElementStateChanged(
            _In_ KnownPropertyIndex propertyIndex,
            _In_ DeferredElementStateChange state,
            _In_ UINT32 collectionIndex,
            _In_ CDependencyObject* realizedElement) override;

    private:
        enum class OverflowInitialFocusItem
        {
            None,
            FirstItem,
            LastItem
        };

        _Check_return_ HRESULT MeasureOverrideForDynamicOverflow(
            _In_ wf::Size availableSize,
            _Out_ wf::Size* returnValue
            );

        _Check_return_ HRESULT ConfigureItemsControls();

        _Check_return_ HRESULT OnLeftRightKeyDown(bool isLeftKey, _Out_ bool* wasHandled);
        _Check_return_ HRESULT OnUpDownKeyDown(bool isUpKey, bool allowFocusWrap, _Out_ bool* wasHandled);

        _Check_return_ HRESULT ShiftFocusHorizontally(bool moveToRight);
        _Check_return_ HRESULT ShiftFocusVerticallyInOverflow(bool topToBottom, bool allowFocusWrap = true);
        _Check_return_ HRESULT HandleTabKeyPressedInOverflow(bool isShiftKeyPressed, _Out_ bool* wasHandled);

        // Pass in true to focus the first element in the overflow.
        // Pass in false to focus the last element.
        _Check_return_ HRESULT SetFocusedElementInOverflow(bool focusFirstElement, _Out_ bool* wasFocusSet);

        _Check_return_ HRESULT SetCompactMode(_In_ bool isCompact);

        _Check_return_ HRESULT OnOpeningImpl(_In_ IInspectable* pArgs) override;
        _Check_return_ HRESULT OnClosingImpl(_In_ IInspectable* pArgs) override;
        _Check_return_ HRESULT OnClosedImpl(_In_ IInspectable* pArgs) override;

        _Check_return_ HRESULT OnUnloaded(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnPrimaryCommandsChanged(
            _In_ wfc::IObservableVector<xaml_controls::ICommandBarElement*>* pSender,
            _In_ wfc::IVectorChangedEventArgs* pArgs);

        _Check_return_ HRESULT OnSecondaryCommandsChanged(
            _In_ wfc::IObservableVector<xaml_controls::ICommandBarElement*>* pSender,
            _In_ wfc::IVectorChangedEventArgs* pArgs);

        _Check_return_ HRESULT OnSecondaryItemsControlLoaded(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnOverflowContentRootSizeChanged(_In_ IInspectable* pSender, _In_ xaml::ISizeChangedEventArgs* pArgs);
        _Check_return_ HRESULT OnOverflowContentKeyDown(IInspectable* pSender, xaml_input::IKeyRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnAccessKeyInvoked(_In_ IUIElement* pSender, _In_ xaml_input::IAccessKeyInvokedEventArgs* pArgs);
        _Check_return_ HRESULT OnOverflowPopupOpened(_In_ IInspectable* pSender, _In_ IInspectable* pArgs);

        _Check_return_ HRESULT SetOverflowStyleAndInputModeOnSecondaryCommand(UINT32 index, bool isItemInOverflow);
        _Check_return_ HRESULT SetOverflowStyleUsage(_In_ xaml_controls::ICommandBarElement* element, bool isItemInOverflow);
        _Check_return_ HRESULT SetOverflowStyleParams();

        _Check_return_ HRESULT PropagateDefaultLabelPosition();
        _Check_return_ HRESULT PropagateDefaultLabelPositionToElement(xaml_controls::ICommandBarElement *element);

        _Check_return_ HRESULT HasBottomLabel(BOOLEAN *hasBottomLabel);

        _Check_return_ HRESULT UpdateInputDeviceTypeUsedToOpen();
        _Check_return_ HRESULT SetInputModeOnSecondaryCommand(UINT32 index, DirectUI::InputDeviceType inputType);

        _Check_return_ HRESULT GetOverflowContentSize(_Out_ wf::Size* overfowContentSize);
        _Check_return_ HRESULT GetShouldOverflowOpenInFullWidth(_Out_ bool* shouldOpenInFullWidth);

        _Check_return_ HRESULT CalculateOverflowContentHorizontalOffset(
            wf::Size overflowContentSize,
            wf::Rect visibleBounds,
            _Out_ double* horizontalOffset
            );

        _Check_return_ HRESULT TryDismissCommandBarOverflow();

        _Check_return_ HRESULT RestoreFocusToExpandButton();

        static _Check_return_ HRESULT HasVisibleElements(_In_ CommandBarElementCollection* collection, bool* hasVisibleElements);

        _Check_return_ HRESULT FindMovablePrimaryCommands(
            _In_ double availablePrimaryCommandsWidth,
            _In_ double primaryItemsControlDesiredWidth,
            _Out_ UINT32* primaryCommandsCountInTransition);

        _Check_return_ HRESULT FindMovablePrimaryCommandsFromOrderSet(
            _In_ double availablePrimaryCommandsWidth,
            _In_ double primaryItemsControlDesiredWidth,
            _Out_ UINT32* primaryCommandsCountInTransition,
            _Inout_ bool& canProcessDynamicOverflowOrder);

        _Check_return_ HRESULT UpdatePrimaryCommandElementMinWidthInOverflow();

        _Check_return_ HRESULT TrimPrimaryCommandSeparatorInOverflow(
            _Inout_ UINT32* primaryCommandsCountInTransition);

         _Check_return_ HRESULT MoveTransitionPrimaryCommandsIntoOverflow(
            _In_ UINT32 primaryCommandsCountInTransition);

        _Check_return_ HRESULT InsertTransitionPrimaryCommandIntoOverflow(
            _In_ xaml_controls::ICommandBarElement* transitionPrimaryElement);

        _Check_return_ HRESULT SaveMovedPrimaryCommandsIntoPreviousTransitionCollection();

        _Check_return_ HRESULT FireDynamicOverflowItemsChangingEvent(_In_ bool isForceToRestore);

        _Check_return_ HRESULT IsElementInOverflow(
            _In_ xaml_controls::ICommandBarElement* element,
            _Out_ BOOLEAN* isInOverflow);

        _Check_return_ HRESULT GetPositionInSet(
            _In_ xaml_controls::ICommandBarElement* element,
            _Out_ INT* positionInSet);

        _Check_return_ HRESULT GetSizeOfSet(
            _In_ xaml_controls::ICommandBarElement* element,
            _Out_ INT* sizeOfSet);

        _Check_return_ HRESULT ResetDynamicCommands();

        _Check_return_ HRESULT IsAppBarSeparatorInDynamicPrimaryCommands(
            _In_ UINT32 index,
            _Out_ bool* isAppBarSeparator);

        _Check_return_ HRESULT FindMovableSeparatorsInBackwardDirection(
            _In_ UINT32 movingPrimaryCommandIndex,
            _Inout_ UINT32* primaryCommandsCountInTransition,
            _Inout_ double* primaryItemsControlDesiredWidth);

        _Check_return_ HRESULT FindMovableSeparatorsInForwardDirection(
            _In_ UINT32 movingPrimaryCommandIndex,
            _Inout_ UINT32* primaryCommandsCountInTransition,
            _Inout_ double* primaryItemsControlDesiredWidth);

        _Check_return_ HRESULT FindNonSeparatorInDynamicPrimaryCommands(
            _In_ bool isForward,
            _In_ INT32 indexMoving,
            _In_ bool* hasNonSeparator,
            _In_ INT32* indexNonSeparator);

        _Check_return_ HRESULT InsertPrimaryCommandToPrimaryCommandsInTransition(
            _In_ UINT32 indexMovingPrimaryCommand,
            _Inout_ UINT32* primaryCommandsCountInTransition,
            _Inout_ double* primaryItemsControlDesiredWidth);

        _Check_return_ HRESULT InsertSeparatorToPrimaryCommandsInTransition(
            _In_ UINT32 indexMovingSeparator,
            _Inout_ UINT32* primaryCommandsCountInTransition,
            _Inout_ double* primaryItemsControlDesiredWidth);

        _Check_return_ HRESULT GetRestorablePrimaryCommandsMinimumCount(
            _Out_ UINT32* restorableMinCount);

        _Check_return_ HRESULT StoreFocusedCommandBarElement();
        _Check_return_ HRESULT RestoreCommandBarElementFocus();
        void ResetCommandBarElementFocus();

        _Check_return_ HRESULT GetFocusState(
            _In_ xaml::IDependencyObject* focusedElement,
            _Out_ xaml::FocusState* focusState);

        _Check_return_ HRESULT CloseSubMenu(
            _In_opt_ ctl::ComPtr<xaml_controls::ISubMenuOwner> const& menu,
            _In_opt_ xaml_controls::ISubMenuOwner* menuToLeaveOpen,
            bool closeOnDelay);

        _Check_return_ HRESULT SetAccessKeyAutomationPropertyOnExpandButton();

        TrackerPtr<CommandBarElementCollection> m_tpPrimaryCommands;
        TrackerPtr<CommandBarElementCollection> m_tpSecondaryCommands;
        TrackerPtr<CommandBarElementCollection> m_tpDynamicPrimaryCommands;
        TrackerPtr<CommandBarElementCollection> m_tpDynamicSecondaryCommands;
        TrackerPtr<IterableWrappedObservableCollection<xaml_controls::ICommandBarElement>> m_tpWrappedPrimaryCommands;
        TrackerPtr<IterableWrappedObservableCollection<xaml_controls::ICommandBarElement>> m_tpWrappedSecondaryCommands;

        // Primary commands in the transition to move or restore the primary commands
        // to overflow or primary commands
        TrackerPtr<TrackerCollection<xaml_controls::ICommandBarElement*>> m_tpPrimaryCommandsInTransition;
        TrackerPtr<TrackerCollection<xaml_controls::ICommandBarElement*>> m_tpPrimaryCommandsInPreviousTransition;

        ctl::EventPtr<FrameworkElementUnloadedEventCallback>        m_unloadedEventHandler;
        ctl::EventPtr<CommandBarElementVectorChangedEventCallback>  m_primaryCommandsChangedEventHandler;
        ctl::EventPtr<CommandBarElementVectorChangedEventCallback>  m_secondaryCommandsChangedEventHandler;
        ctl::EventPtr<FrameworkElementLoadedEventCallback>          m_secondaryItemsControlLoadedEventHandler;
        ctl::EventPtr<FrameworkElementSizeChangedEventCallback>     m_contentRootSizeChangedEventHandler;
        ctl::EventPtr<FrameworkElementSizeChangedEventCallback>     m_overflowContentSizeChangedEventHandler;
        ctl::EventPtr<UIElementKeyDownEventCallback>                m_overflowPresenterItemsPresenterKeyDownEventHandler;

        ctl::EventPtr<UIElementAccessKeyInvokedEventCallback>       m_accessKeyInvokedEventHandler;
        ctl::EventPtr<PopupOpenedEventCallback>                     m_overflowPopupOpenedEventHandler;

        // Template parts.
        TrackerPtr<xaml::IFrameworkElement> m_tpContentControl;
        TrackerPtr<xaml::IFrameworkElement> m_tpOverflowContentRoot;
        TrackerPtr<xaml_primitives::IPopup> m_tpOverflowPopup;
        TrackerPtr<xaml_controls::IItemsPresenter> m_tpOverflowPresenterItemsPresenter;
        TrackerPtr<xaml::IFrameworkElement> m_tpWindowedPopupPadding;

        double m_overflowContentMinWidth = 0;
        double m_overflowContentTouchMinWidth = 0;
        double m_overflowContentMaxWidth = 480;

        // Restorable primary command minimum width from overflow to the primary command collection
        double m_restorablePrimaryCommandMinWidth = 0;

        bool m_skipProcessTabStopOverride = false;
        bool m_canShadowBeAnimatedByEntranceAnimation = false;
        DirectUI::InputDeviceType m_inputDeviceTypeUsedToOpen = DirectUI::InputDeviceType::Touch;

        bool                                m_hasAlreadyFiredOverflowChangingEvent = false;
        bool                                m_hasAppBarSeparatorInOverflow = false;
        BOOLEAN                             m_isDynamicOverflowEnabled = true;
        UINT                                m_SecondaryCommandStartIndex = 0;

        OverflowInitialFocusItem            m_overflowInitialFocusItem = OverflowInitialFocusItem::None;

        TrackerPtr<AppBarSeparator>         m_tpAppBarSeparatorInOverflow;

        // Whenever there is a change in the primary/secondary commands or a size change, we take note
        // of the focused command and we make sure we restore focus during the next layout pass.
        TrackerPtr<xaml_controls::ICommandBarElement> m_focusedElementPriorToCollectionOrSizeChange;
        xaml::FocusState m_focusStatePriorToCollectionOrSizeChange = {};

        double m_lastAvailableWidth = 0;
    };
}
