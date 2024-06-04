// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Control.g.h"
#include "JoltClasses.h"
#include "VisualStateManager.g.h"
#include "EventCallbacks.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

class CControl;

namespace DirectUI
{
    class ControlCustomDPChangedHandler;
    class IsEnabledChangedEventArgs;
    class ContextRequestedEventArgs;
    class FocusEngagedEventArgs;
    class Rectangle;

    // Represents the base class for all custom controls.
    PARTIAL_CLASS(Control)
    {
        public:
            static _Check_return_ HRESULT UpdateVisualState(
                _In_ CDependencyObject* pDO,
                _In_ bool fUseTransitions);

            static _Check_return_ HRESULT UpdateVisualState(
                _In_ Control* control,
                _In_ bool fUseTransitions);

            static _Check_return_ HRESULT UpdateEngagementState(
                _In_ CControl* pDO,
                _In_ bool engage);

            __forceinline
            static Control* GetAsControlNoRef(_In_ DependencyObject* pDO)
            {
                if (!pDO->m_bCastedAsControl)
                {
                    pDO->m_pThisAsControlNoRef = ctl::query_interface_cast_noref<Control>(pDO);
                    pDO->m_bCastedAsControl = TRUE;
                }

                return pDO->m_pThisAsControlNoRef;
            }

            _Check_return_ HRESULT GetValueFromBuiltInStyle(
                _In_ const CDependencyProperty* pDP,
                _Out_ IInspectable** ppValue,
                _Out_ bool* pbGotValue);

            virtual _Check_return_ HRESULT GetCalculatedDefaultStyleKey(
                _Outptr_result_maybenull_ const CClassInfo** ppType,
                _Outptr_result_maybenull_ IInspectable** ppBoxedKey);

            virtual _Check_return_ HRESULT OnCharacterReceivedImpl(_In_ xaml_input::ICharacterReceivedRoutedEventArgs* pArgs);

            IFACEMETHOD(get_DefaultStyleKey)(_Outptr_ IInspectable** ppValue) override;
            IFACEMETHOD(put_DefaultStyleKey)(_In_ IInspectable* pValue) override;
            IFACEMETHOD(OnApplyTemplate)() override;

            // Attempts to find the VisualStateGroups in the Control.
            // Sets ppVisualStateGroups to NULL if the VisualStates are not defined.
            _Check_return_ HRESULT GetVisualStateGroups(
                // Output buffer.
                _Outptr_result_maybenull_ wfc::IVector<xaml::VisualStateGroup*>** ppVisualStateGroups);

            // Override the GetValue method to return the values for control.
            _Check_return_ HRESULT GetValue(
                _In_ const CDependencyProperty* pDP,
                _Outptr_ IInspectable **ppValue) override;

            _Check_return_ HRESULT RemoveFocusEngagementImpl();

            _Check_return_ HRESULT SetFocusEngagementImpl();

        protected:
            // The managed Control implementation suspends state updates using
            // try/finally.  StateChangeSuspender is a helper that allows us to
            // resume state changes in the event of an IFC failure after they've
            // been suspended using RAII techniques.  You should use this via
            // the SUSPEND_STATE_CHANGES and RESUME_STATE_CHANGES macros defined
            // below because they automatically add an extra IFC(hr) after the
            // suspension block which is necessary so any failures during
            // ~StateChangeSuspender can be passed to the current HRESULT.
            struct StateChangeSuspender
            {
                private:
                    // The ButtonBase to suspend state changes on
                    ctl::ComPtr<Control> m_spControl;

                    // The HRESULT for the method that is used to notify of
                    // failures if resuming state changes fails.
                    HRESULT* m_pResult;

                public:
                    // Suspend state changes for the Control.
                    StateChangeSuspender(
                        _In_ Control* pControl,
                        _In_ HRESULT* pResult);

                    // Resume state changes for the Control.
                    ~StateChangeSuspender();
            };

            // Suspend state changes through the scope of the next block
            #define SUSPEND_STATE_CHANGES() { StateChangeSuspender(this, &hr);

            // Resume state changes
            #define RESUME_STATE_CHANGES()  } IFC(hr); // Check the HRESULT again because ~StateChangeSuspender may have changed it to E_FAIL if the UpdateVisualState call failed

            //Process keyboard accelerators
            static _Check_return_ HRESULT ProcessAcceleratorsIfApplicable(
                _Inout_ ctl::ComPtr<xaml_input::IKeyRoutedEventArgs>& spArgsAsKEA,
                _In_ Control* const pSenderAsControl);

        public:
            // Initializes a new instance of the Control class.
            Control();

            _Check_return_ HRESULT ApplyTemplateImpl(_Out_ BOOLEAN* pReturnValue);

            _Check_return_ HRESULT OnPointerEnteredImpl(_In_ xaml_input::IPointerRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnPointerPressedImpl(_In_ xaml_input::IPointerRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnPointerMovedImpl(_In_ xaml_input::IPointerRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnPointerReleasedImpl(_In_ xaml_input::IPointerRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnPointerExitedImpl(_In_ xaml_input::IPointerRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnPointerCaptureLostImpl(_In_ xaml_input::IPointerRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnPointerCanceledImpl(_In_ xaml_input::IPointerRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnPointerWheelChangedImpl(_In_ xaml_input::IPointerRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnTappedImpl(_In_ xaml_input::ITappedRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnDoubleTappedImpl(_In_ xaml_input::IDoubleTappedRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnHoldingImpl(_In_ xaml_input::IHoldingRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnRightTappedImpl(_In_ xaml_input::IRightTappedRoutedEventArgs* pArgs);
            virtual _Check_return_ HRESULT OnContextRequestedImpl(_In_ xaml_input::IContextRequestedEventArgs * pArgs);
            _Check_return_ HRESULT OnRightTappedUnhandled(_In_ xaml_input::IRightTappedRoutedEventArgs* pArgs) override;
            _Check_return_ HRESULT OnManipulationStartingImpl(_In_ xaml_input::IManipulationStartingRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnManipulationInertiaStartingImpl(_In_ xaml_input::IManipulationInertiaStartingRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnManipulationStartedImpl(_In_ xaml_input::IManipulationStartedRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnManipulationDeltaImpl(_In_ xaml_input::IManipulationDeltaRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnManipulationCompletedImpl(_In_ xaml_input::IManipulationCompletedRoutedEventArgs* pArgs);
            IFACEMETHOD(OnInheritedPropertyChanged)(_In_ IInspectable* pArgs);

            _Check_return_ HRESULT OnKeyUpImpl(_In_ xaml_input::IKeyRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnKeyDownImpl(_In_ xaml_input::IKeyRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnPreviewKeyDownImpl(_In_ xaml_input::IKeyRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnPreviewKeyUpImpl(_In_ xaml_input::IKeyRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnGotFocusImpl(_In_ xaml::IRoutedEventArgs* pArgs);
            _Check_return_ HRESULT OnLostFocusImpl(_In_ xaml::IRoutedEventArgs* pArgs);

            // Drag target interface
            _Check_return_ HRESULT OnDragEnterImpl(_In_ xaml::IDragEventArgs* pArgs);
            _Check_return_ HRESULT OnDragOverImpl(_In_ xaml::IDragEventArgs* pArgs);
            _Check_return_ HRESULT OnDragLeaveImpl(_In_ xaml::IDragEventArgs* pArgs);
            _Check_return_ HRESULT OnDropImpl(_In_ xaml::IDragEventArgs* pArgs);

            _Check_return_ HRESULT OnFocusEngaged(_In_ FocusEngagedEventArgs* pArgs);

            _Check_return_ HRESULT GetTemplateChildImpl(_In_ HSTRING childName, _Outptr_ xaml::IDependencyObject** returnValue);

            _Check_return_ HRESULT get_ImplementationRoot(_Outptr_ xaml::IFrameworkElement** ppValue);

            static _Check_return_ HRESULT FireEvent(_In_ KnownEventIndex nDelegate, _In_ DependencyObject* pSender, _In_opt_ IInspectable* pArgs);

            // Get the currently pressed keyboard modifiers.
            static _Check_return_ HRESULT GetKeyboardModifiers(
                _Out_ wsy::VirtualKeyModifiers* pModifierKeys);

            IFACEMETHOD(add_IsEnabledChanged)(
                    _In_ xaml::IDependencyPropertyChangedEventHandler* pValue,
                    _Out_ EventRegistrationToken* ptToken) override;

            IFACEMETHOD(remove_IsEnabledChanged)(
                    _In_ EventRegistrationToken tToken) override;

            // Show the context menu if the ContextFlyout property has been set for the control
            _Check_return_ HRESULT ShowContextFlyout(
                _In_ xaml_input::IContextRequestedEventArgs * pArgs,
                _In_ DirectUI::Control * pContextFlyoutControl);

        protected:

            virtual _Check_return_ HRESULT OnVisibilityChanged()
            {
                RRETURN(S_OK);
            }

            // IsEnabled property changed handler.
            virtual _Check_return_ HRESULT OnIsEnabledChanged(
                _In_ DirectUI::IsEnabledChangedEventArgs* pArgs);

            // Update the current visual state of the control.
            _Check_return_ HRESULT UpdateVisualState(
                // true to use transitions when updating the visual state, false
                // to snap directly to the new visual state.
                _In_ bool bUseTransitions = true);

            // Change to the correct visual state for the control.
            virtual _Check_return_ HRESULT ChangeVisualState(
                // true to use transitions when updating the visual state, false
                // to snap directly to the new visual state.
                _In_ bool bUseTransitions);

            _Check_return_ HRESULT UpdateEngagementState(_In_ bool engage);

            // Go to a specific visual state.
            _Check_return_ HRESULT GoToState(
                // true to use transitions when updating the visual state, false
                // to snap directly to the new visual state.
                _In_ bool bUseTransitions,
                // The name of the state to transition to.
                _In_z_ const wchar_t* pszState,
                _Out_ BOOLEAN* pbReturnValue);

            _Check_return_ HRESULT LayoutRoundRectangleStrokeThickness(
                _In_ Rectangle *pRectangle);

            // Override OnPointerCaptured() to handle ToolTips.  For mouse/keyboard ToolTips,
            // we dismiss them.  For touch ToolTips, we set flag m_cancelAutomaticTouchToolTipOnCaptureLost
            // to TRUE to remember that we must close the ToolTip when pointer capture is lost.
            _Check_return_ HRESULT OnPointerCaptured() override;

            _Check_return_ HRESULT OnPropertyChanged2(
                _In_ const PropertyChangedParams& args) override;

            static bool IsValueRequired(_In_ xaml_controls::IInputValidationControl* control);

            static _Check_return_ HRESULT InvokeValidationCommand(_In_ xaml_controls::IInputValidationControl2* control, _In_ HSTRING value);
            static _Check_return_ HRESULT InvokeValidationCommand(_In_ xaml_controls::IInputValidationControl2* control, _In_ IInspectable* value);
        private:
            // Register all of the event handlers required for ButtonBase.
            _Check_return_ HRESULT RegisterEventHandlers();

            _Check_return_ HRESULT GetIsEnabledChangedEventSourceNoRef(
                _Outptr_ CIsEnabledChangedEventSource** ppEventSource);

        public:

            // Check if the given UIElement overlaps with the viewport
            _Check_return_ HRESULT DoesElementOverlapViewport(
                _In_ IUIElement* pCandidate,
                _Out_ bool& overlapsViewport);

        public:
            // Retrieves a reference to a child template object given its name.
            template<class TInterface, class TRuntime>
            _Check_return_ HRESULT GetTemplatePart(
                _In_reads_(cName) WCHAR* pName,
                _In_ size_t cName,
                _Outptr_ TRuntime** ppReference)
            {
                HRESULT hr = S_OK;
                wrl_wrappers::HString strName;
                ctl::ComPtr<xaml::IDependencyObject> spElement;
                ctl::ComPtr<TInterface> spElementAsInterface;

                IFCPTR(pName);
                IFCPTR(ppReference);

                // Perform the lookup and cast the result
                IFC(strName.Set(pName, cName));
                IFC(GetTemplateChild(strName.Get(), &spElement));
                spElementAsInterface = spElement.AsOrNull<TInterface>();
                IFC(spElementAsInterface.CopyTo(ppReference));

            Cleanup:
                RRETURN(hr);
            }

        protected:
            template<typename TElementType>
            _Check_return_ HRESULT ConditionallyGetTemplatePartAndUpdateVisibility(
                _In_ const xstring_ptr_view& strName,
                _In_ bool visible,
                TrackerPtr<TElementType>& element)
            {
                if (!element && (visible || !DXamlCore::GetCurrent()->GetHandle()->GetDeferredElementIfExists(strName, GetHandle(), Jupiter::NameScoping::NameScopeType::TemplateNameScope)))
                {
                    // If element should be visible or is not deferred, then fetch it.
                    xruntime_string_ptr strNamePromoted;
                    ctl::ComPtr<xaml::IDependencyObject> spElement;
                    IFC_RETURN(strName.Promote(&strNamePromoted));
                    IFC_RETURN(GetTemplateChild(strNamePromoted.GetHSTRING(), &spElement));
                    SetPtrValueWithQIOrNull(element, spElement.Get());
                }

                // If element was found then set its Visibility - this is behavior consistent with pre-Threshold releases.
                if (element)
                {
                    ctl::ComPtr<IUIElement> spElementAsUIE = element.template AsOrNull<IUIElement>();

                    if (spElementAsUIE)
                    {
                        IFC_RETURN(spElementAsUIE->put_Visibility((visible) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
                    }
                }

                return S_OK;
            }

        private:
            // True if visual state changes are suspended; false otherwise.
            BOOLEAN m_fSuspendStateChanges : 1;

            BOOLEAN m_fHasCustomDefaultStyleKey : 1;

            // When CapturePointer() is called on a UIElement while a ToolTip is open, then the ToolTip owner
            // no longer receives the pointer events it needs to dismiss the ToolTip (if it is a different UIElement
            // than the one calling CapturePointer() now).  Therefore, since this UIElement is capturing pointer events,
            // it needs to be responsible for dismissing any open automatic ToolTip.
            //
            // We don't always want to synchronously dismiss the ToolTip in CapturePointer() because it breaks the case
            // where the touch ToolTip is owned by an element in the ControlTemplate of the control that is calling
            // CapturePointer().  In this case, we want to keep the touch ToolTip open as long as the pointer is down
            // on the target element.  Therefore, we set this flag to true in CapturePointer() and clear this flag
            // and close the ToolTip when pointer capture is lost.  For mouse/keyboard automatic ToolTips, it is safe
            // to synchronously close them in CapturePointer, so that's what we do.
            BOOLEAN m_cancelAutomaticTouchToolTipOnCaptureLost : 1;            
    };
}
