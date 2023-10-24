// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ComboBox.g.h"
#include "Clock.h"

class CComboBoxTextSubmittedEventArgs;

namespace DirectUI
{
    class IsEnabledChangedEventArgs;

    // Represents a ComboBox control.
    //
    PARTIAL_CLASS(ComboBox)
    {
    public:

        // Apply a template to the ComboBox.
        IFACEMETHOD(OnApplyTemplate)() override;

        // Provides the behavior for the Arrange pass of layout.  Classes
        // can override this method to define their own Arrange pass
        // behavior.
        IFACEMETHOD(ArrangeOverride)(
            // The computed size that is used to arrange the content.
            _In_ wf::Size finalSize,
            // The size of the control.
            _Out_ wf::Size* pReturnValue);

        BOOLEAN GetIsPopupPannable()
        {
            return m_bIsPopupPannable;
        }

        BOOLEAN GetShouldCarousel()
        {
            return m_bShouldCarousel;
        }

        _Check_return_ IFACEMETHOD(IsHostForItemContainer)(
            _In_ xaml::IDependencyObject* pContainer,
            _Out_ BOOLEAN* pIsHost) override;

        // Change to the correct visual state for the ComboBox.
        _Check_return_ HRESULT ChangeVisualState(
            // true to use transitions when updating the visual state, false
            // to snap directly to the new visual state.
            _In_ bool bUseTransitions) override;

        // Handles the case where right tapped is raised by unhandled.
        _Check_return_ HRESULT OnRightTappedUnhandled(_In_ xaml_input::IRightTappedRoutedEventArgs* pArgs) override;

        _Check_return_ HRESULT GetLightDismissElement(_Outptr_ xaml::IUIElement** ppUIElement);

        _Check_return_ HRESULT GetEditableTextPart(_Outptr_ xaml::IUIElement** ppUIElement);

        _Check_return_ HRESULT IsInlineMode(_Out_ BOOLEAN* isInlineMode);

        DirectUI::InputDeviceType GetInputDeviceTypeUsedToOpen();

        // This code gets called from Automation Provider e.g. ItemAutomationPeer and must not be called from any
        // internal APIs in the control itself. It basically returns the Container for the DataItem in case it exist.
        // When container doesn't exist and Item is selected Item faceplate ContentPresenter is returned.
        _Check_return_ HRESULT UIA_GetContainerForDataItemOverride(
            _In_opt_ IInspectable* pItem,
            _In_ INT itemIndex,
            _Outptr_ xaml::IUIElement** ppContainer) override;

        _Check_return_ HRESULT GetContentPresenterPart(_Outptr_ xaml_controls::IContentPresenter** ppContentPresenterPart);

        // Used to handle back button presses on phone
        _Check_return_ HRESULT OnBackButtonPressedImpl(_Out_ BOOLEAN* returnValue);

        _Check_return_ HRESULT OnDropDownClosedImpl(
            _In_ IInspectable* pArgs);

        _Check_return_ HRESULT OnDropDownOpenedImpl(
            _In_ IInspectable* pArgs);

        const bool IsSearchResultIndexSet()
        {
            return m_searchResultIndexSet;
        }

        const int GetSearchResultIndex()
        {
            return m_searchResultIndex;
        }

    protected:
        ComboBox();
        ~ComboBox() override;

        // Prepares object's state
        _Check_return_ HRESULT Initialize() override;

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        // Update the visual states when the Visibility property is changed.
        _Check_return_ HRESULT OnVisibilityChanged() override;

        IFACEMETHOD(OnKeyDown)(
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnKeyUp)(
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnPointerWheelChanged)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnPointerPressed)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnPointerReleased)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnPointerEntered)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnPointerMoved)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnPointerExited)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnPointerCaptureLost)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // Determines if the specified item is (or is eligible to be) its own container.
        IFACEMETHOD(IsItemItsOwnContainerOverride)(
            _In_ IInspectable* item,
            _Out_ BOOLEAN* returnValue) override;

        // Creates or identifies the element that is used to display the given item.
        IFACEMETHOD(GetContainerForItemOverride)(
            _Outptr_ xaml::IDependencyObject** returnValue) override;

        // Called whenever control gets focus.
        IFACEMETHOD(OnGotFocus)(
            _In_ xaml::IRoutedEventArgs* pArgs) override;

        // Called whenever control gets focus.
        IFACEMETHOD(OnLostFocus)(
            _In_ xaml::IRoutedEventArgs* pArgs) override;

        /// Called to detect whether we can scroll to the View or not.
        /// Returns true when base is true and drop down list is opened
        _Check_return_ HRESULT CanScrollIntoView(
            _Out_ BOOLEAN& canScroll) override;

        IFACEMETHOD(ClearContainerForItemOverride)(
            _In_ xaml::IDependencyObject* element,
            _In_ IInspectable* item) override;

        IFACEMETHOD(PrepareContainerForItemOverride)(
            _In_ xaml::IDependencyObject* element,
            _In_ IInspectable* item) override;

        _Check_return_ HRESULT OnSelectionChanged(
            _In_ INT oldSelectedIndex,
            _In_ INT newSelectedIndex,
            _In_ IInspectable* pOldSelectedItem,
            _In_ IInspectable* pNewSelectedItem,
            _In_ BOOLEAN animateIfBringIntoView = FALSE,
            _In_ xaml_input::FocusNavigationDirection focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None) override;

        // Called when the IsEnabled property changes.
        _Check_return_ HRESULT OnIsEnabledChanged(_In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

        _Check_return_ HRESULT HasFocus(_Out_ BOOLEAN *pbHasFocus) override;

        _Check_return_ HRESULT EnsureTextBoxIsEnabled(bool moveFocusToTextBox);

        _Check_return_ HRESULT ProcessTabStopOverride(
            _In_opt_ DependencyObject* pFocusedElement,
            _In_opt_ DependencyObject* pCandidateTabStopElement,
            const bool isBackward,
            const bool didCycleFocusAtRootVisualScope,
            _Outptr_ DependencyObject** ppNewTabStop,
            _Out_ BOOLEAN* pIsTabStopOverridden) override;

        bool EditableTextHasFocus();

        _Check_return_ HRESULT PrepareState() override;

        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;

        _Check_return_ HRESULT OnItemsChanged(
            _In_ wfc::IVectorChangedEventArgs* e) override;

    private:

        _Check_return_ HRESULT GetNonPannablePopupLayout(
            _In_ INT centerItemIndex,
            _In_ UINT itemCount,
            _In_ DOUBLE cbY,
            _In_ DOUBLE cbHeight,
            _In_ xaml::Thickness cbPopupContentMargin,
            _In_ wf::Size rootWindowSize,
            _Out_ DOUBLE& popupY,
            _Out_ DOUBLE& popupMaxHeight,
            _Out_ DOUBLE& offset);

        _Check_return_ HRESULT GetEditableComboBoxPopupLayout(
            _In_ uint32_t itemCount,
            _In_ double cbY,
            _In_ double cbHeight,
            _In_ xaml::Thickness cbPopupContentMargin,
            _In_ wf::Size rootWindowSize,
            _Out_ double& popupY,
            _Out_ double& popupMaxHeight);

        _Check_return_ HRESULT GetPannablePopupLayout(
            _In_ INT centerItemIndex,
            _In_ UINT itemCount,
            _In_ DOUBLE cbY,
            _In_ DOUBLE cbHeight,
            _In_ DOUBLE childHeight,
            _In_ wf::Size rootWindowSize,
            _Out_ DOUBLE& popupY,
            _Inout_ DOUBLE& popupMaxHeight,
            _Out_ DOUBLE& offset);

        _Check_return_ HRESULT GetItemLayoutHeight(
            _In_ INT index,
            _In_ wf::Size availableSize,
            _Out_ DOUBLE& itemHeight);

        _Check_return_ HRESULT UpdateIsPopupPannable(
            _In_ UINT itemCount,
            _In_ DOUBLE maxAllowedPopupHeight,
            _In_ wf::Size availableSize,
            _Inout_ DOUBLE& childHeight);

        _Check_return_ HRESULT IsLeftButtonPressed(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs,
            _Out_ BOOLEAN* pIsLeftButtonPressed,
            _Out_opt_ mui::PointerDeviceType* pPointerDeviceType);

        _Check_return_ HRESULT OnUnloaded(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);

        static bool ShouldIgnoreKeyCode(WCHAR keyCode);

        // Called when a character-received message bubbles up to the ComboBox.
        _Check_return_ HRESULT OnCharacterReceived(_In_ IUIElement* pSender, _In_ xaml_input::ICharacterReceivedRoutedEventArgs* pArgs);
        // Called by the popup when it gets a character received.
        _Check_return_  HRESULT OnPopupCharacterReceived(_In_ IUIElement* pSender, _In_ xaml_input::ICharacterReceivedRoutedEventArgs* pEventArgs);
        // The worker method that does TypeAhead work and overall search.
        _Check_return_ HRESULT ProcessSearch(_In_ WCHAR keyCode);
        // Searches for an item inside the ItemSource.
        _Check_return_ HRESULT SearchItemSourceIndex(_In_ WCHAR keyCode, _In_ bool startSearchFromCurrentIndex, _In_ bool searchExactMatch, _Outptr_ int& foundIndex);

        _Check_return_ HRESULT CommitRevertEditableSearch(_In_ bool restoreValue);

        void ResetSearch();
        void SetSearchResultIndex(_In_ int index);

        // Called whenever the we receive a key-down message.
        _Check_return_ HRESULT OnKeyDownPrivate(
            _In_opt_ IInspectable* pSender,
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs);

        // Called whenever the we receive a key-up message.
        _Check_return_ HRESULT OnKeyUpPrivate(
            _In_opt_ IInspectable* pSender,
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs);

        // Called whenever the Popup gets focus.
        _Check_return_ HRESULT OnElementPopupChildGotFocus(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        // Called whenever the Popup loses focus.
        _Check_return_ HRESULT OnElementPopupChildLostFocus(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        // Called whenever the pointer enters the Popup.
        _Check_return_ HRESULT OnElementPopupChildPointerEntered(
            _In_ IInspectable* pSender,
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        // Called whenever the pointer leaves the Popup.
        _Check_return_ HRESULT OnElementPopupChildPointerExited(
            _In_ IInspectable* pSender,
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        // Called whenever the pointer enters the DropDown button.
        _Check_return_ HRESULT OnDropDownOverlayPointerEntered(
            _In_ IInspectable* pSender,
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        // Called whenever the pointer leaves the DropDown button.
        _Check_return_ HRESULT OnDropDownOverlayPointerExited(
            _In_ IInspectable* pSender,
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        // Called whenever the Popup size changes.
        _Check_return_ HRESULT OnElementPopupChildSizeChanged(
            _In_ IInspectable* pSender,
            _In_ xaml::ISizeChangedEventArgs* pArgs);

        _Check_return_ HRESULT OnElementPopupChildLoaded(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        // Called whenever the size of this ComboBox changes
        _Check_return_ HRESULT OnSizeChanged(
            _In_ IInspectable* pSender,
            _In_ xaml::ISizeChangedEventArgs* pArgs);

        // Called whenever the pointer pressed happens on the Popup.
        _Check_return_ HRESULT OnElementOutsidePopupPointerPressed(
            _In_ IInspectable* pSender,
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnPopupClosed(
                _In_opt_ IInspectable*,
                _In_opt_ IInspectable*);

        _Check_return_ HRESULT OnTextBoxPointerPressedPrivate(
            _In_ IInspectable* pSender,
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnTextBoxTapped(
            _In_ IInspectable* pSender,
            _In_ xaml_input::ITappedRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnTextBoxPreviewKeyDown(
            _In_opt_ IInspectable* pSender,
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnTextBoxTextChanged(
            _In_ IInspectable* pSender,
            _In_ xaml_controls::ITextChangedEventArgs* pArgs);

        _Check_return_ HRESULT OnTextBoxCandidateWindowBoundsChanged(
            _In_ xaml_controls::ITextBox* /*pSender*/,
            _In_ xaml_controls::ICandidateWindowBoundsChangedEventArgs* pArgs);

        _Check_return_ HRESULT OnTextBoxSizeChanged(
            _In_ IInspectable* pSender,
            _In_ xaml::ISizeChangedEventArgs* pArgs);

        _Check_return_ HRESULT UpdateHeaderPresenterVisibility();

        // Handler for the dropdown state changed event
        _Check_return_ HRESULT OnIsDropDownOpenChanged();

        // Handler for max drop down height changed event
        _Check_return_ HRESULT OnMaxDropDownHeightChanged();

        _Check_return_ HRESULT SetContentPresenter(
            _In_ INT index,
            _In_opt_ bool forceSelectionBoxToNull = false) noexcept;

        _Check_return_ HRESULT UpdateSelectionBoxItemProperties(_In_ INT32 index);

        _Check_return_ HRESULT ArrangePopup(
            _In_ bool bCenterSelectedItem) noexcept;

        _Check_return_ HRESULT HandleRotateTransformOnParentForPopupPlacement(
            _In_ xaml_media::Matrix transformToRootMatrix,
            _In_ xaml::FlowDirection flowDirection,
            _Inout_ DOUBLE& comboBoxOffsetX,
            _Inout_ DOUBLE& comboBoxOffsetY,
            _Inout_ wf::Size& rootWindowSize);

        _Check_return_ HRESULT InvertMatrix(
            _In_ xaml_media::Matrix* pMatrix);

        _Check_return_ HRESULT FocusChanged(
            _In_ BOOLEAN hasFocus);

        _Check_return_ HRESULT UpdateSelectionBoxHighlighted();

        _Check_return_ HRESULT SetupElementPopupChild();

        _Check_return_ HRESULT SetupElementOutsidePopupChild();

        _Check_return_ HRESULT SetupEditableMode();

        _Check_return_ HRESULT DisableEditableMode();

        _Check_return_ HRESULT SetupOtherEventHandlers();

        _Check_return_ HRESULT ReleaseMembers();

        _Check_return_ HRESULT SetupElementPopup();

        _Check_return_ HRESULT SetupElementPopupChildCanvas();

        _Check_return_ HRESULT PopupKeyDown(
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs);

        _Check_return_ HRESULT MainKeyDown(
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs);

        // Called when the IsSelectionActive property has changed.
        _Check_return_ HRESULT OnIsSelectionActiveChanged();

        _Check_return_ HRESULT SetupVisualStateEventHandlerForDropDownClosedState();

        _Check_return_ HRESULT OnClosedStateStoryboardCompleted(
            _In_ IInspectable* pSender,
            _In_ IInspectable* pArgs);

        _Check_return_ HRESULT SetVerticalOffsetOnCarouselPanel(
            _In_ DOUBLE offset);

        _Check_return_ HRESULT ResetCarouselPanelState();

        _Check_return_ HRESULT GetRealizedFirstChildFromCarouselPanel(
            _Outptr_ xaml::IUIElement**  ppRealizedFirstChild);

        _Check_return_ HRESULT GetMeasureDeltaForVisualsBetweenPopupAndCarouselPanelFromCarouselPanel(
            _Out_ DOUBLE& delta);

        _Check_return_ HRESULT SetClosingAnimationDirection();

        _Check_return_ HRESULT GetLayoutPosition(
            _In_ xaml::IUIElement* pUIElement,
            _Out_ wf::Point& layoutPosition);

        _Check_return_ HRESULT TransformComboBoxToVisual(
            _Outptr_ xaml_media::IGeneralTransform** ppTransform);

        _Check_return_ HRESULT SetIsPopupPannable();

        // Clear flags relating to the visual state.  Called when IsEnabled is set to FALSE
        // or when Visibility is set to Hidden or Collapsed.
        _Check_return_ HRESULT ClearStateFlags();

        _Check_return_ HRESULT ClearStateFlagsOnItems();

        // Contains the logic to be employed if we decide to handle pointer released.
        _Check_return_ HRESULT PerformPointerUpAction(_In_ bool isDropDownOverlay);

        _Check_return_ HRESULT IsEventSourceTarget(
            _In_ xaml::IRoutedEventArgs* pArgs,
            _Out_ BOOLEAN* pIsEventSourceChildOfTarget);

        // The "target" area of a ComboBox excludes the header. Pointer and focus events will only
        // have effect if they originate from an element within the target area. This is a
        // helper function to make that determination.
        _Check_return_ HRESULT IsChildOfTarget(
            _In_opt_ xaml::IDependencyObject* pChild,
            _In_ BOOLEAN doSearchLogicalParents,
            _In_ BOOLEAN doCacheResult,
            _Out_ BOOLEAN* pIsChildOfTarget);

        _Check_return_ HRESULT OnOpen();

        _Check_return_ HRESULT OnClose();

        _Check_return_ HRESULT FinishClosingDropDown();

        _Check_return_ HRESULT RaiseDropDownOpenChangedEvents(_In_ BOOLEAN isDropDownOpen);

        _Check_return_ HRESULT RaiseAutomationPeerExpandCollapse(_In_ BOOLEAN isDropDownOpen);

        _Check_return_ HRESULT OverrideSelectedIndexForVisualStates(_In_ int selectedIndexOverride);

        _Check_return_ HRESULT ClearSelectedIndexOverrideForVisualStates();

        bool IsDropDownOverlay(
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT EnsurePropertyPathListener();

        // ********************************************************************
        // Bitfield group 1 (8 bits)
        // ********************************************************************
        bool m_bIsPopupPannable                     : 1;
        bool m_bShouldCarousel                      : 1;
        bool m_bShouldCenterSelectedItem            : 1;
        bool m_handledGamepadOrRemoteKeyDown        : 1;
        bool m_ignoreCancelKeyDowns                 : 1;
        bool m_isEditModeConfigured                 : 1;
        bool m_isInSearchingMode                    : 1;
        bool m_openedUp                             : 1;

        // ********************************************************************
        // Bitfield group 2 (8 bits)
        // ********************************************************************

        // On pointer released we perform some actions depending on control. We decide to whether to perform them
        // depending on some parameters including but not limited to whether released is followed by a pressed, which
        // mouse button is pressed, what type of pointer is it etc. This BOOLEAN keeps our decision.
        bool m_shouldPerformActions                 : 1;
        bool m_IsPointerOverMain                    : 1;
        bool m_IsPointerOverPopup                   : 1;
        bool m_bIsPressed                           : 1;
        bool m_preparingContentPresentersElement    : 1;
        bool m_isDropDownClosing                    : 1;
        bool m_bPopupHasBeenArrangedOnce            : 1;
        // Used to determine when to open the popup based on touch, we open the popup when TextBox gains
        // focus due to a pointer event.
        bool m_openPopupOnTouch                     : 1;

        // ********************************************************************
        // Bitfield group 3 (8 bits)
        // ********************************************************************

        // Determines if any search has started and has not been committed or reverted.
        bool m_searchResultIndexSet                 : 1;
        // Touch input model for Editable ComboBox establishes that second tap should select all text,
        // we use this variable to determine when to select all text on touch.
        bool m_selectAllOnTouch                     : 1;
        // Setting Editable Mode configures several event listeners, we use this variable to prevent configuring Editable mode twice.
        // Editable ComboBox is designed to set the focus on TextBox when ComboBox is focused, there are some cases when we don't want
        // this behavior eg(Shift+Tab).
        bool m_shouldMoveFocusToTextBox             : 1;
        bool m_isExpanded                           : 1;
        bool m_isOverlayVisible                     : 1;
        bool m_restoreIndexSet                      : 1;
        bool m_isClosingDueToCancel                 : 1;
        bool m_doKeepInView                         : 1;

        // ********************************************************************
        // Bitfield group 4 (8 bits)
        // ********************************************************************

        bool m_IsPointerOverDropDownOverlay         : 1;
        // bool m_unused2                           : 1;
        // bool m_unused3                           : 1;
        // bool m_unused4                           : 1;
        // bool m_unused5                           : 1;
        // bool m_unused6                           : 1;
        // bool m_unused7                           : 1;
        // bool m_unused8                           : 1;

        DirectUI::InputDeviceType m_inputDeviceTypeUsedToOpen;
        DirectUI::InputDeviceType m_previousInputDeviceTypeUsedToOpen;

        TrackerPtr<xaml::IFrameworkElement> m_tpElementPopupChild;
        TrackerPtr<xaml::IFrameworkElement> m_tpElementPopupContent;
        xaml::Thickness popupContentMargin;
        EventRegistrationToken m_pElementPopupChildKeyDownToken;
        EventRegistrationToken m_pElementPopupChildKeyUpToken;
        EventRegistrationToken m_pElementPopupChildGotFocusToken;
        EventRegistrationToken m_pElementPopupChildLostFocusToken;
        EventRegistrationToken m_pElementPopupChildPointerEnteredToken;
        EventRegistrationToken m_pElementPopupChildPointerExitedToken;
        EventRegistrationToken m_pElementPopupChildISizeChangedToken;
        EventRegistrationToken m_pElementOutSidePopupPointerPressedToken;
        EventRegistrationToken m_closedStateStoryboardCompletedToken;
        EventRegistrationToken m_pElementPopupChildCharacterReceivedToken;
        EventRegistrationToken m_pElementPopupChildLoadedToken;

        ctl::ComPtr<xaml_input::IPointerEventHandler> m_spEditableTextPointerPressedEventHandler;
        ctl::ComPtr<xaml_input::ITappedEventHandler> m_spEditableTextTappedEventHandler;
        ctl::EventPtr<UIElementKeyDownEventCallback> m_spEditableTextKeyDownHandler;
        ctl::EventPtr<UIElementPointerPressedEventCallback> m_spEditableTextPointerPressedHandler;
        ctl::EventPtr<UIElementPreviewKeyDownEventCallback> m_spEditableTextPreviewKeyDownHandler;
        ctl::EventPtr<TextBoxTextChangedEventCallback> m_spEditableTextTextChangedHandler;
        ctl::EventPtr<TextBoxCandidateWindowBoundsChangedEventCallback> m_spEditableTextCandidateWindowBoundsChangedEventHandler;
        ctl::EventPtr<FrameworkElementSizeChangedEventCallback> m_spEditableTextSizeChangedHandler;
        ctl::EventPtr<UIElementPointerEnteredEventCallback> m_spDropDownOverlayPointerEnteredHandler;
        ctl::EventPtr<UIElementPointerExitedEventCallback> m_spDropDownOverlayPointerExitedHandler;

        TrackerPtr<IInspectable> m_tpEmptyContent;
        TrackerPtr<xaml_controls::ITextBlock> m_tpEditableContentPresenterTextBlock;
        TrackerPtr<xaml_controls::IComboBoxItem> m_tpSwappedOutComboBoxItem;
        TrackerPtr<xaml_controls::ICanvas> m_tpElementPopupChildCanvas;
        TrackerPtr<xaml_controls::ICanvas> m_tpElementOutsidePopup;
        TrackerPtr<xaml_animation::IStoryboard> m_tpClosedStoryboard;
        INT m_MaxNumberOfItemsThatCanBeShown = 9;
        INT m_MaxNumberOfItemsThatCanBeShownOnOneSide = 4;
        TrackerPtr<xaml::IDependencyObject> m_tpGeneratedContainerForContentPresenter;
        INT m_iLastGeneratedItemIndexforFaceplate;
        ctl::ComPtr<IInspectable> m_customValueRef;
        wf::Rect m_candidateWindowBoundsRect;

        ctl::EventPtr<FrameworkElementUnloadedEventCallback> m_epUnloadedHandler;
        ctl::EventPtr<PopupClosedEventCallback> m_epPopupClosedHandler;

        // TypeAhead methods and members
        wrl_wrappers::HString m_searchString;
        Jupiter::HighResolutionClock::time_point m_timeSinceLastCharacterReceived;
        ctl::ComPtr<PropertyPathListener> m_spPropertyPathListener;
        // Keeps track of the item's index that matched the last search.
        int m_searchResultIndex = -1;

        HRESULT AppendCharToSearchString(_In_ WCHAR ch, _Out_ BOOLEAN* createdNewString); // Returns true if created new string, false if appended
        void ResetSearchString();
        bool HasSearchStringTimedOut();
        bool IsInSearchingMode();
        bool IsTextSearchEnabled();
        bool IsEditable();

        _Check_return_ HRESULT TryGetStringValue(
            _In_ IInspectable* object,
            _In_opt_ PropertyPathListener* pathListener,
            _Out_ HSTRING* value);

        _Check_return_ HRESULT RaiseTextSubmittedEvent(
            _In_ wrl_wrappers::HString& text,
            _Out_ BOOLEAN* isHandled);

        _Check_return_ HRESULT CreateEditableContentPresenterTextBlock();

        _Check_return_ HRESULT UpdateEditableContentPresenterTextBlock(_In_ IInspectable* item);
        _Check_return_ HRESULT UpdateEditableContentPresenterTextBlock(const wrl_wrappers::HString& text);

        _Check_return_ HRESULT UpdateEditableTextBox(_In_ IInspectable* item, _In_ bool selectText, _In_ bool selectAll);
        _Check_return_ HRESULT UpdateEditableTextBox(_In_ wrl_wrappers::HString* str, _In_ bool selectText, _In_ bool selectAll);

        static bool StartsWithIgnoreLinguisticSemantics(const wrl_wrappers::HString& strSource, const wrl_wrappers::HString& strPrefix);
        static bool AreStringsEqual(const wrl_wrappers::HString& strSource, const wrl_wrappers::HString& strPrefix);
        static bool IsSearchStringValid(const wrl_wrappers::HString& str);
        static _Check_return_ HRESULT FindParentComboBoxFromDO(_In_ CDependencyObject* pSender, _Out_ ComboBox** parentComboBox);

        bool ShouldMoveFocusToTextBox() const
        {
            return m_shouldMoveFocusToTextBox;
        }

private: // members to support small form factor mode

        _Check_return_ HRESULT OnOpenSmallFormFactor();
        _Check_return_ HRESULT OnCloseSmallFormFactor();

        _Check_return_ HRESULT UpdateTemplateSettings();
        _Check_return_ HRESULT UpdateSelectedItemVisualState();

        _Check_return_ HRESULT EnsurePresenterReadyForFullMode();
        _Check_return_ HRESULT EnsurePresenterReadyForInlineMode();

        _Check_return_ HRESULT OnFlyoutButtonClick(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnGetListPickerSelectionAsyncCompleted(
            _In_ wf::IAsyncOperation<INT32>* pAsyncState,
            _In_ wf::AsyncStatus status);

        _Check_return_ HRESULT OnItemsPresenterHostParentSizeChanged(
            _In_ IInspectable* pSender,
            _In_ xaml::ISizeChangedEventArgs* pArgs);

        _Check_return_ HRESULT OnItemsPresenterSizeChanged(
            _In_ IInspectable* pSender,
            _In_ xaml::ISizeChangedEventArgs* pArgs);

        // ----------------------------------------------------------------------------------------
        // In inline mode, there is an items presenter that shows up to 5 items, and an
        // items presenter host that provides a viewport onto the presenter (applies a clip
        // so that only part of the presenter is visible. The presenter slides up and down
        // and the host's clip is resized so that only the selected item is visible when the drop-
        // down is closed, and all of the items are visible when the drop-down is open.
        // ----------------------------------------------------------------------------------------
        _Check_return_ HRESULT PrepareLayoutForInlineDropDownClosed();
        _Check_return_ HRESULT PrepareLayoutForInlineDropDownOpened();

        // ----------------------------------------------------------------------------------------
        // This is used to slide the items presenter up or down and resize the presenter host
        // so that the selected element is displayed when something is changed programatically
        // while in the Normal expansion state.
        // ----------------------------------------------------------------------------------------
        _Check_return_ HRESULT ForceApplyInlineLayoutUpdate();

        // ----------------------------------------------------------------------------------------
        // For Windows Phone, ComboBox is capable of operating in two modes, "inline" and "full."
        // When the ComboBox contains more than five items, full mode is used. In full mode, the
        // ComboBox is a button that launches a fullscreen single-selection ListPickerFlyout.
        // If the ComboBox contains less than or equal to five itmes, inline mode is used. In
        // inline mode, when tapped, the ComboBox expands to show all of its items, pushing all
        // content below it on the page down.
        // This method checks that the provided template contains all of the parts required
        // to support the inline and full modes of operation.
        // ----------------------------------------------------------------------------------------
        inline BOOLEAN IsSmallFormFactor() const
        {
            return m_tpItemsPresenterHostPart &&
                m_tpItemsPresenterHostParent &&
                m_tpItemsPresenterPart &&
                m_tpItemsPresenterTranslateTransformPart &&
                m_tpFlyoutButtonPart;
        }

        inline BOOLEAN IsInline() const
        {
            return IsSmallFormFactor() && m_itemCount <= s_itemCountThreashold;
        }

        inline BOOLEAN IsFullMode() const
        {
            return IsSmallFormFactor() && m_itemCount > s_itemCountThreashold;
        }

        _Check_return_ HRESULT GetVisibleBoundsInternal(_Out_ wf::Rect* pVisibleBounds);

        _Check_return_ HRESULT ReevaluateIsOverlayVisible();
        _Check_return_ HRESULT UpdateTargetForOverlayAnimations();
        _Check_return_ HRESULT PlayOverlayOpeningAnimation();
        _Check_return_ HRESULT PlayOverlayClosingAnimation();

        static const UINT s_itemCountThreashold;

        ctl::EventPtr<ButtonBaseClickEventCallback> m_epFlyoutButtonClickHandler;
        ctl::EventPtr<FrameworkElementSizeChangedEventCallback> m_epHostParentSizeChangedHandler;
        ctl::EventPtr<FrameworkElementSizeChangedEventCallback> m_epItemsPresenterSizeChangedHandler;
        TrackerPtr<IFrameworkElement> m_tpItemsPresenterHostParent;
        TrackerPtr<IAsyncInfo> m_tpAsyncSelectionInfo;
        UINT32 m_itemCount;
        INT32 m_itemsPresenterIndex;

        TrackerPtr<xaml_controls::IContentPresenter> m_tpHeaderContentPresenterPart;
        TrackerPtr<xaml_controls::IContentPresenter> m_requiredHeaderContentPresenterPart;
        TrackerPtr<xaml_animation::IStoryboard> m_overlayOpeningStoryboard;
        TrackerPtr<xaml_animation::IStoryboard> m_overlayClosingStoryboard;

        int m_indexToRestoreOnCancel{ -1 };
        int m_indexForcedToUnselectedVisual = -1;
        int m_indexForcedToSelectedVisual = -1;
    };
}
