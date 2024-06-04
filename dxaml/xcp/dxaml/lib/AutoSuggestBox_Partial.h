// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//      The AutoSuggestBox control is a standard textbox that provides contextual
//      suggestions in a temporary drop-down list while users type in characters.
//
//      Suggestions change as the user types more characters.

#pragma once

#include "AutoSuggestBox.g.h"
#include "XamlTraceLogging.h"
#include "ReversedVector.h"
#include <fwd/windows.ui.viewmanagement.h>
#include "DisplayOrientationHelper.h"

namespace DirectUI
{
    class PropertyPathListener;

    PARTIAL_CLASS(AutoSuggestBox)
    {

    public:
        AutoSuggestBox();
        ~AutoSuggestBox() override;

        IFACEMETHOD(put_IsSuggestionListOpen)(_In_ BOOLEAN value);

        IFACEMETHOD(OnApplyTemplate)() override;

        static _Check_return_ HRESULT OnInkingFunctionButtonClicked(_In_ CDependencyObject* pAutoSuggestBox);

        UINT GetTextChangedEventCounter() { return m_textChangedCounter; }

        _Check_return_ HRESULT ProgrammaticSubmitQuery();

    protected:
        _Check_return_ HRESULT PrepareState() override;

        // Focus & SIP Occlude:
        IFACEMETHOD(OnLostFocus)(
              _In_ xaml::IRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnGotFocus)(
              _In_ xaml::IRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnKeyDown)(
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

        _Check_return_ HRESULT IsTextBoxFocusedElement(
            _Out_ BOOLEAN* pFocused);

        _Check_return_ HRESULT OnSipShowing(
            _In_ wuv::IInputPane* pSender,
            _In_ wuv::IInputPaneVisibilityEventArgs* pArgs);

        _Check_return_ HRESULT OnSipHiding(
            _In_ wuv::IInputPane* pSender,
            _In_ wuv::IInputPaneVisibilityEventArgs* pArgs);

        IFACEMETHOD(OnItemsChanged)(
            _In_ IInspectable* e) override;

        _Check_return_ HRESULT TryGetSuggestionValue(
            _In_ IInspectable* object,
            _In_opt_ PropertyPathListener* pathListener,
            _Out_ HSTRING* value);

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        // IUIElementOverrides methods
        IFACEMETHOD(OnCreateAutomationPeer)(
            _Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;

        _Check_return_ HRESULT GetPlainText(_Out_ HSTRING* strPlainText) override;

    private:
        // internal event handlers:
        _Check_return_ HRESULT OnUnloaded(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnTextBoxTextChanged(
            _In_ IInspectable* pSender,
            _In_ xaml_controls::ITextChangedEventArgs* pArgs);

        _Check_return_ HRESULT OnTextBoxCandidateWindowBoundsChanged(
            _In_ xaml_controls::ITextBox* pSender,
            _In_ xaml_controls::ICandidateWindowBoundsChangedEventArgs* pArgs);

        _Check_return_ HRESULT OnSizeChanged(
            _In_ IInspectable* pSender,
            _In_ xaml::ISizeChangedEventArgs *pArgs);

        _Check_return_ HRESULT OnPopupOpened(
            _In_ IInspectable* pSender,
            _In_ IInspectable* pArgs);

        _Check_return_ HRESULT OnSuggestionsContainerLoaded(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnSuggestionListKeyDown(
            _In_ IInspectable* pSender,
            _In_ xaml::Input::IKeyRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnTextBoxLoaded(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnTextBoxUnloaded(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnTextBoxQueryButtonClick(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT SetTextBoxQueryButtonIcon();

        _Check_return_ HRESULT ClearTextBoxQueryButtonIcon();

        _Check_return_ HRESULT SubmitQuery(_In_ IInspectable* pChosenSuggestion);

        _Check_return_ HRESULT UpdateText(_In_ HSTRING value);

        _Check_return_ HRESULT UpdateTextBoxText(
            _In_ HSTRING value,
            _In_ xaml_controls::AutoSuggestionBoxTextChangeReason reason);

        // private typedefs:
        typedef wf::ITypedEventHandler<wuv::InputPane*, wuv::InputPaneVisibilityEventArgs*> InputPaneVisibilityEventHandler;

        enum class ControlledPeer
        {
            None,
            SuggestionsList
        };

        _Check_return_ HRESULT SetCurrentControlledPeer(_In_ ControlledPeer peer);

        _Check_return_ HRESULT HookToRootScrollViewer();

        _Check_return_ HRESULT ChangeVisualState();

        _Check_return_ HRESULT AlignmentHelper(
            _In_ wf::Rect sipOverlay);

        // try to scroll textbox up or down to maximize suggestion area
        _Check_return_ HRESULT MaximizeSuggestionArea(
            _In_ DOUBLE topY,
            _In_ DOUBLE bottomY,
            _In_ DOUBLE sipOverlayY,
            _In_ wf::Rect layoutBounds);

        _Check_return_ HRESULT MaximizeSuggestionAreaWithoutInputPane();

        _Check_return_ HRESULT Scroll(_Inout_ DOUBLE& totalOffset);

        _Check_return_ HRESULT PushScrollAction(
            _In_ xaml_controls::IScrollViewer* pScrollViewer,
            _Inout_ DOUBLE& targetOffset);

        // apply scroll actions to move this control to desired position
        // or revert back to original position
        _Check_return_ HRESULT ApplyScrollActions(_In_ BOOLEAN hasNewScrollActions);

        _Check_return_ HRESULT UpdateSuggestionListSize();

        _Check_return_ HRESULT UpdateSuggestionListVisibility();

        _Check_return_ HRESULT UpdateSuggestionListPosition();

        // by design, we always delay the TextChanged event by an interval (150ms)
        // and only raise the event for the last input if user types fast.
        // here we use a timer to delay the event.
        _Check_return_ HRESULT OnTextChangedEventTimerTick(_In_ IInspectable *pSender, _In_ IInspectable *pArgs);

        _Check_return_ HRESULT OnSuggestionSelectionChanged(_In_ IInspectable* pSender, _In_ xaml_controls::ISelectionChangedEventArgs* pArgs);

        _Check_return_ HRESULT OnListViewItemClick(_In_ IInspectable* pSender, _In_ xaml_controls::IItemClickEventArgs* pArgs);

        _Check_return_ HRESULT OnListViewContainerContentChanging(_In_ xaml_controls::IListViewBase* pSender, _In_ xaml_controls::IContainerContentChangingEventArgs* pArgs);

        _Check_return_ HRESULT TransformPoint(
            _In_ xaml::IUIElement* pTargetElement,
            _Inout_ wf::Point* pPoint);

        _Check_return_ HRESULT OnSipHidingInternal();

        _Check_return_ HRESULT OnSipShowingInternal(
            _In_ wuv::IInputPaneVisibilityEventArgs* pArgs);

        _Check_return_ HRESULT GetCandidateWindowPopupAdjustment(
            _In_ bool ignoreSuggestionListPosition,
            _Out_opt_ double *pXOffset,
            _Out_opt_ double *pYOffset);

        _Check_return_ HRESULT ResetIgnoreTextChanges()
        {
            m_ignoreTextChanges = false;
            return S_OK;
        }

        _Check_return_ HRESULT UpdateSuggestionListItemsSource();

        _Check_return_ HRESULT ScrollLastItemIntoView();

        // In WP8.1, we implemented the mode where the suggestion list is above the textbox
        // by veritically mirroring the Suggestion ListView.  Now that ASB needs to support
        // keyboard and mouse in win10, we now reverse the vector we set on the suggestion
        // ListView's ItemsSource instead.  We keep the old path for compat.  If the ASB's
        // template contains the ScaleTransform that flips the ListView (m_tpListItemOrderTransformPart)
        // we'll go ahead and use it.  Else, we'll do it the new way (reverse the vector).
        bool IsSuggestionListVerticallyMirrored() const
        {
            return m_suggestionListPosition == SuggestionListPosition::Above
                && m_tpListItemOrderTransformPart;
        }

        // This helper function is used to identify the ASBs which have new ASB implementation
        // where we reverse the vector set on the suggestion ListView's ItemSource.
        bool  IsSuggestionListVectorReversed() const
        {
            return m_suggestionListPosition == SuggestionListPosition::Above
                && !m_tpListItemOrderTransformPart;
        }

        // This helper function is used to identify whether we should move down (look at an item of higher
        // index) or move up (look at an item of lower index) in the list.
        bool ShouldMoveIndexForwardForKey(wsy::VirtualKey key, wsy::VirtualKeyModifiers modifiers) const
        {
            return (key == wsy::VirtualKey_Down && !IsSuggestionListVerticallyMirrored()) ||
                (key == wsy::VirtualKey_Up   && IsSuggestionListVerticallyMirrored()) ||
                (key == wsy::VirtualKey_Tab  && !IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Shift) && !IsSuggestionListVerticallyMirrored()) ||
                (key == wsy::VirtualKey_Tab  && IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Shift) && IsSuggestionListVerticallyMirrored());
        }


        static _Check_return_ HRESULT ProcessDeferredUpdateStatic(ctl::WeakRefPtr wpThis);
        _Check_return_ HRESULT ProcessDeferredUpdate();

        _Check_return_ HRESULT ReevaluateIsOverlayVisible();

        _Check_return_ HRESULT SetupOverlayState();
        _Check_return_ HRESULT TeardownOverlayState();

        _Check_return_ HRESULT CreateLTEs();
        _Check_return_ HRESULT PositionLTEs();
        _Check_return_ HRESULT DestroyLTEs();

        bool ShouldUseParentedLTE();
        _Check_return_ HRESULT GetAdjustedLayoutBounds (_Out_ wf::Rect &layoutBounds) const;
        _Check_return_ HRESULT GetActualTextBoxSize(_Out_ double& actualWidth, _Out_ double& actualHeight) const;

        _Check_return_ HRESULT GetDisplayOrientation(_Out_ double& orientation);

    private:
        // static constatants specific to this control:
        static const INT64 s_textChangedEventTimerDuration;

        static const unsigned int s_minSuggestionListHeight;

        // template parts
        TrackerPtr<xaml_controls::ITextBox> m_tpTextBoxPart;
        TrackerPtr<xaml::IUIElement> m_requiredHeaderPresenterPart;
        TrackerPtr<xaml_primitives::IButtonBase> m_tpTextBoxQueryButtonPart;

        TrackerPtr<xaml_primitives::ISelector> m_tpSuggestionsPart;

        TrackerPtr<xaml_primitives::IPopup> m_tpPopupPart;

        TrackerPtr<xaml::IFrameworkElement> m_tpSuggestionsContainerPart;

        // when we need to show suggestion list above the text box, we apply a transform to move the suggestion list.
        TrackerPtr<xaml_media::ITranslateTransform> m_tpUpwardTransformPart;

        // when we need to show suggestion list above the text box, we invert the order of the items in the list.
        TrackerPtr<xaml_media::ITransform> m_tpListItemOrderTransformPart;

        TrackerPtr<xaml_controls::IGrid> m_tpLayoutRootPart;

        TrackerPtr<xaml::IDispatcherTimer> m_tpTextChangedEventTimer;

        TrackerPtr<xaml_controls::IAutoSuggestBoxTextChangedEventArgs> m_tpTextChangedEventArgs;

        xaml_controls::AutoSuggestionBoxTextChangeReason m_textChangeReason{ xaml_controls::AutoSuggestionBoxTextChangeReason_UserInput};

        TrackerPtr<wuv::IInputPane> m_tpInputPane;

        TrackerPtr<wuv::IInputPaneVisibilityEventArgs> m_tpSipArgs;

        // counter++ when Text is changed. TextChangedEvent will use this counter
        // to determine if the current value of the TextBox is unchanged from
        // the point when TextChangedEvent was raised.
        UINT m_textChangedCounter{ 0 };

        // events:
        EventRegistrationToken m_sipEvents[2] {};

        ctl::EventPtr<FrameworkElementUnloadedEventCallback> m_epUnloadedEventHandler;
        ctl::EventPtr<FrameworkElementSizeChangedEventCallback> m_epSizeChangedEventHandler;

        ctl::EventPtr<TextBoxTextChangedEventCallback> m_epTextBoxTextChangedEventHandler;
        ctl::EventPtr<DispatcherTimerTickEventCallback> m_epTextChangedEventTimerTickEventHandler;
        ctl::EventPtr<SelectionChangedEventCallback> m_epSuggestionSelectionChangedEventHandler;
        ctl::EventPtr<UIElementKeyDownEventCallback> m_suggestionListKeyDownEventHandler;
        ctl::EventPtr<ListViewBaseItemClickEventCallback> m_epListViewItemClickEventHandler;
        ctl::EventPtr<ListViewBaseContainerContentChangingEventCallback> m_epListViewContainerContentChangingEventHandler;
        ctl::EventPtr<PopupOpenedEventCallback> m_epPopupOpenedEventHandler;
        ctl::EventPtr<FrameworkElementLoadedEventCallback> m_epSuggestionsContainerLoadedEventHandler;

        ctl::EventPtr<FrameworkElementLoadedEventCallback> m_epTextBoxLoadedEventHandler;
        ctl::EventPtr<FrameworkElementUnloadedEventCallback> m_epTextBoxUnloadedEventHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epQueryButtonClickEventHandler;

        ctl::EventPtr<TextBoxCandidateWindowBoundsChangedEventCallback> m_epTextBoxCandidateWindowBoundsChangedEventHandler;
        ctl::EventPtr<FrameworkElementLayoutUpdatedEventCallback> m_layoutUpdatedEventHandler;

        wrl::ComPtr<ReversedVector> m_spReversedVector;


    private:
        struct ScrollAction
        {
            Microsoft::WRL::WeakRef wkScrollViewer;
            DOUBLE target;
            DOUBLE initial;

            ScrollAction()
                : target(0.), initial(0.)
            {
            }
        };

        static const WCHAR c_TextBoxName[];
        static const WCHAR c_TextBoxQueryButtonName[];
        static const WCHAR c_SuggestionsPopupName[];
        static const WCHAR c_SuggestionsListName[];
        static const WCHAR c_SuggestionsContainerName[];
        static const WCHAR c_UpwardTransformName[];
        static const WCHAR c_TextBoxScrollViewerName[];
        static const WCHAR c_VisualStateLandscape[];
        static const WCHAR c_VisualStatePortrait[];
        static const WCHAR c_ListItemOrderTransformName[];
        static const WCHAR c_LayoutRootName[];
        static const WCHAR c_RequiredHeaderName[];

        // save the scroll actions when we adjust the position (typically when this control gets focus)
        // restore them when SIP is gone (not in LoseFocus because user may change the focus to another
        // textbox, in that case the SIP is still on).
        std::vector<ScrollAction> m_scrollActions;

        // RootScrollViewer acts an important role when we adjust the position.
        // when SIP is on, the RootScrollViewer's viewport will be changed so the content inside
        // RootScrollViewer can be scrollable.
        // here there is a known issue that when RootScrollViewer's viewport is being restored and the
        // content becomes non-scrollable, but the vertical offset is not reset back to 0.
        // AutoSuggestBox needs correct offset to determine how much space we can scroll it up or down.
        // the workaround is when SIP is hiding, we manually reset the vertical offset to 0.
        Microsoft::WRL::WeakRef m_wkRootScrollViewer;

        enum class SuggestionListPosition
        {
            Above,
            Below
        };

        SuggestionListPosition m_suggestionListPosition{ SuggestionListPosition::Below };

        DirectUI::InputDeviceType m_inputDeviceTypeUsed{ DirectUI::InputDeviceType::None };

        // when SIP is showing, we'll get two SIP_Showing events
        // when SIP is hiding, we get one SIP_Showing following by a SIP_Hiding event
        // to avoid redundant adjustment, use this flag to mark the SIP status.
        bool m_isSipVisible{ false };

        bool m_ignoreSelectionChanges{ false };

        bool m_ignoreTextChanges{ false };

        wrl_wrappers::HString m_userTypedText;

        DOUBLE m_availableSuggestionHeight{ 0. };

        bool m_hasFocus{ false };

        bool m_keepFocus{ false };

        bool m_suppressSuggestionListVisibility{ false };

        bool m_deferringUpdate{ false };
        ctl::ComPtr<PropertyPathListener> m_spPropertyPathListener;

        static bool m_sSipIsOpen;

#if DBG
        bool m_handlingCollectionChange{ false };
#endif
        XamlDisplay::Orientation m_displayOrientation{ XamlDisplay::Orientation::None };

        wf::Rect m_candidateWindowBoundsRect{};

        bool m_isOverlayVisible{ false };

        TrackerPtr<xaml::IUIElement>           m_overlayLayoutTransition;
        TrackerPtr<xaml::IUIElement>           m_layoutTransition;

        TrackerPtr<xaml::IUIElement>           m_parentElementForLTEs;
        TrackerPtr<xaml::IFrameworkElement>    m_overlayElement;
    };

}
