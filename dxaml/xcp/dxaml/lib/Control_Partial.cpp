// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Control.g.h"
#include "ToolTipService.g.h"
#include "Rectangle.g.h"
#include "IsEnabledChangedEventArgs.g.h"
#include "CharacterReceivedRoutedEventArgs.g.h"
#include "MenuFlyout.g.h"
#include "MenuFlyoutItemBaseCollection.g.h"
#include "MenuFlyoutPresenter.g.h"
#include "FocusEngagedEventArgs.g.h"
#include "FocusDisengagedEventArgs.g.h"
#include "FxCallbacks.h"
#include "Slider.g.h"
#include "ScrollViewer.g.h"
#include "VisualTreeHelper.h"
#include <FocusMgr.h>
#include <VisualTree.h>
#include "KeyboardAcceleratorUtility.h"
#include "KeyRoutedEventArgs.g.h"
#include "ContextRequestedEventArgs.g.h"
#include "StaticStore.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Initializes a new instance of the Control class.
Control::Control()
    : m_fSuspendStateChanges(FALSE)
    , m_fHasCustomDefaultStyleKey(FALSE)
    , m_cancelAutomaticTouchToolTipOnCaptureLost(FALSE)
{
}

_Check_return_ HRESULT
Control::get_ImplementationRoot(_Outptr_ xaml::IFrameworkElement** ppValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spElement;

    IFC(GetImplementationRoot(&spElement));
    IFC(spElement.CopyTo(ppValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP Control::get_DefaultStyleKey(_Outptr_ IInspectable** ppValue)
{
    HRESULT hr = S_OK;
    wxaml_interop::TypeName typeName = {};

    if (m_fHasCustomDefaultStyleKey)
    {
        IFC(ControlGenerated::get_DefaultStyleKey(ppValue));
    }
    else
    {
        IFC(MetadataAPI::GetTypeNameByClassInfo(MetadataAPI::GetClassInfoByIndex(GetTypeIndex()), &typeName));
        IFC(DirectUI::PropertyValue::CreateReference<wxaml_interop::TypeName>(typeName, ppValue));
    }

Cleanup:
    DELETE_STRING(typeName.Name);
    RRETURN(hr);
}

IFACEMETHODIMP Control::put_DefaultStyleKey(_In_ IInspectable* pValue)
{
    m_fHasCustomDefaultStyleKey = TRUE;
    RRETURN(ControlGenerated::put_DefaultStyleKey(pValue));
}

_Check_return_ HRESULT
Control::GetCalculatedDefaultStyleKey(
    _Outptr_result_maybenull_ const CClassInfo** ppType,
    _Outptr_result_maybenull_ IInspectable** ppBoxedKey)
{
    HRESULT hr = S_OK;

    *ppType = NULL;
    *ppBoxedKey = NULL;

    if (m_fHasCustomDefaultStyleKey)
    {
        IFC(get_DefaultStyleKey(ppBoxedKey));
    }
    else
    {
        *ppType = MetadataAPI::GetClassInfoByIndex(GetTypeIndex());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Control::GetTemplateChildImpl(
    _In_ HSTRING childName,
    _Outptr_ xaml::IDependencyObject** returnValue)
{
    *returnValue = nullptr;

    if (auto child = static_cast<CControl*>(GetHandle())->GetTemplateChild(XSTRING_PTR_EPHEMERAL_FROM_HSTRING(childName)))
    {
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(child.get(), returnValue));
    }

    return S_OK;
}

_Check_return_ HRESULT
Control::UpdateVisualState(
    _In_ CDependencyObject* pDO,
    _In_ bool fUseTransitions)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spPeer;
    ctl::ComPtr<IControl> spControl;

    IFC(DXamlCore::GetCurrent()->TryGetPeer(pDO, &spPeer));
    if (spPeer)
    {
        spControl = spPeer.AsOrNull<IControl>();
        if (spControl)
        {
            IFC(spControl.Cast<Control>()->UpdateVisualState(fUseTransitions));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Control::UpdateVisualState(
    _In_ Control* control,
    _In_ bool fUseTransitions)
{
    IFC_RETURN(control->UpdateVisualState(fUseTransitions));
    return S_OK;
}

_Check_return_ HRESULT
Control::UpdateEngagementState(
    _In_ CControl* pDO,
    _In_ bool engage)
{
    ctl::ComPtr<DependencyObject> spPeer;
    ctl::ComPtr<IControl> spControl;

    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(pDO, &spPeer));
    if (spPeer)
    {
        spControl = spPeer.AsOrNull<IControl>();
        if (spControl)
        {
            IFC_RETURN(spControl.Cast<Control>()->UpdateEngagementState(engage));
        }
    }

    return S_OK;
}


_Check_return_ HRESULT
Control::GetValueFromBuiltInStyle(
    _In_ const CDependencyProperty* pDP,
    _Out_ IInspectable** ppValue,
    _Out_ bool* pbGotValue)
{
    HRESULT hr = S_OK;
    CValue boxedValue;

    // Effective value of core properties is updated by native code,
    // so this should be called only by managed properties.
    ASSERT(pDP->IsSparse());

    // Style is implemented in native code. If property exists in the
    // objects' built-in style, get it's value.
    IFC(CoreImports::GetManagedPropertyValueFromStyle(
        TRUE, // bUseBuiltInStyle
        GetHandle(),
        pDP,
        &boxedValue,
        pbGotValue));

    if (*pbGotValue)
    {
        IFC(CValueBoxer::UnboxObjectValue(&boxedValue, pDP->GetPropertyType(), __uuidof(IInspectable), reinterpret_cast<void**>(ppValue)));
    }
    else
    {
        *ppValue = NULL;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Control::FireEvent(_In_ KnownEventIndex nDelegate, _In_ DependencyObject* pSender, _In_opt_ IInspectable* pArgs)
{
    HRESULT hr = S_OK;

    Control* const pSenderAsControl = Control::GetAsControlNoRef(pSender);
    if (pSenderAsControl)
    {
        switch (nDelegate)
        {
            case KnownEventIndex::UIElement_ContextRequested:
                {
                    ctl::ComPtr<IContextRequestedEventArgs> spContextRequestedArgs =
                        ctl::query_interface_cast<IContextRequestedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnContextRequestedImpl(spContextRequestedArgs.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_PointerEntered:
                {
                    ctl::ComPtr<IPointerRoutedEventArgs> spArgsAsPEA =
                        ctl::query_interface_cast<IPointerRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnPointerEnteredProtected(spArgsAsPEA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_PointerPressed:
                {
                    ctl::ComPtr<IPointerRoutedEventArgs> spArgsAsPEA =
                        ctl::query_interface_cast<IPointerRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnPointerPressedProtected(spArgsAsPEA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_PointerMoved:
                {
                    ctl::ComPtr<IPointerRoutedEventArgs> spArgsAsPEA =
                        ctl::query_interface_cast<IPointerRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnPointerMovedProtected(spArgsAsPEA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_PointerReleased:
                {
                    ctl::ComPtr<IPointerRoutedEventArgs> spArgsAsPEA =
                        ctl::query_interface_cast<IPointerRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnPointerReleasedProtected(spArgsAsPEA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_PointerExited:
                {
                    ctl::ComPtr<IPointerRoutedEventArgs> spArgsAsPEA =
                        ctl::query_interface_cast<IPointerRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnPointerExitedProtected(spArgsAsPEA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_PointerCaptureLost:
                {
                    ctl::ComPtr<IPointerRoutedEventArgs> spArgsAsPEA =
                        ctl::query_interface_cast<IPointerRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnPointerCaptureLostProtected(spArgsAsPEA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_PointerCanceled:
                {
                    ctl::ComPtr<IPointerRoutedEventArgs> spArgsAsPEA =
                        ctl::query_interface_cast<IPointerRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnPointerCanceledProtected(spArgsAsPEA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_PointerWheelChanged:
                {
                    ctl::ComPtr<IPointerRoutedEventArgs> spArgsAsPEA =
                        ctl::query_interface_cast<IPointerRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnPointerWheelChangedProtected(spArgsAsPEA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_Tapped:
                {
                    ctl::ComPtr<ITappedRoutedEventArgs> spArgsAsTappedEA =
                        ctl::query_interface_cast<ITappedRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnTappedProtected(spArgsAsTappedEA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_DoubleTapped:
                {
                    ctl::ComPtr<IDoubleTappedRoutedEventArgs> spArgsAsDoubleTappedEA =
                        ctl::query_interface_cast<IDoubleTappedRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnDoubleTappedProtected(spArgsAsDoubleTappedEA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_Holding:
                {
                    ctl::ComPtr<IHoldingRoutedEventArgs> spArgsAsHoldingEA =
                        ctl::query_interface_cast<IHoldingRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnHoldingProtected(spArgsAsHoldingEA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_RightTapped:
                {
                    ctl::ComPtr<IRightTappedRoutedEventArgs> spArgsAsRightTappedEA =
                        ctl::query_interface_cast<IRightTappedRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnRightTappedProtected(spArgsAsRightTappedEA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_RightTappedUnhandled:
                {
                    ctl::ComPtr<IRightTappedRoutedEventArgs> spArgsAsRightTappedEA =
                        ctl::query_interface_cast<IRightTappedRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnRightTappedUnhandled(spArgsAsRightTappedEA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_ManipulationStarting:
                {
                    ctl::ComPtr<IManipulationStartingRoutedEventArgs> spArgsAsManipulationStartingEA =
                        ctl::query_interface_cast<IManipulationStartingRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnManipulationStartingProtected(spArgsAsManipulationStartingEA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_ManipulationInertiaStarting:
                {
                    ctl::ComPtr<IManipulationInertiaStartingRoutedEventArgs> spArgsAsManipulationInertiaStartingEA =
                        ctl::query_interface_cast<IManipulationInertiaStartingRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnManipulationInertiaStartingProtected(spArgsAsManipulationInertiaStartingEA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_ManipulationStarted:
                {
                    ctl::ComPtr<IManipulationStartedRoutedEventArgs> spArgsAsMSEA =
                        ctl::query_interface_cast<IManipulationStartedRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnManipulationStartedProtected(spArgsAsMSEA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_ManipulationDelta:
                {
                    ctl::ComPtr<IManipulationDeltaRoutedEventArgs> spArgsAsMDEA =
                        ctl::query_interface_cast<IManipulationDeltaRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnManipulationDeltaProtected(spArgsAsMDEA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_ManipulationCompleted:
                {
                    ctl::ComPtr<IManipulationCompletedRoutedEventArgs> spArgsAsMCEA =
                        ctl::query_interface_cast<IManipulationCompletedRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnManipulationCompletedProtected(spArgsAsMCEA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_KeyUp:
                {
                    ctl::ComPtr<IKeyRoutedEventArgs> spArgsAsKEA =
                        ctl::query_interface_cast<IKeyRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnKeyUpProtected(spArgsAsKEA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_KeyDown:
                {
                    ctl::ComPtr<IKeyRoutedEventArgs> spArgsAsKEA = ctl::query_interface_cast<IKeyRoutedEventArgs>(pArgs);
                    IFC(ProcessAcceleratorsIfApplicable(spArgsAsKEA, pSenderAsControl));
                    BOOLEAN handled = FALSE;
                    IFC(spArgsAsKEA->get_Handled(&handled));
                    if (!handled)
                    {
                        IFC(pSenderAsControl->OnKeyDownProtected(spArgsAsKEA.Get()));
                    }
                    break;
                }
            case KnownEventIndex::UIElement_PreviewKeyDown:
            {
                ctl::ComPtr<IKeyRoutedEventArgs> spArgsAsKEA =
                    ctl::query_interface_cast<IKeyRoutedEventArgs>(pArgs);
                IFC(pSenderAsControl->OnPreviewKeyDownProtected(spArgsAsKEA.Get()));
                break;
            }
            case KnownEventIndex::UIElement_PreviewKeyUp:
            {
                ctl::ComPtr<IKeyRoutedEventArgs> spArgsAsKEA =
                    ctl::query_interface_cast<IKeyRoutedEventArgs>(pArgs);
                IFC(pSenderAsControl->OnPreviewKeyUpProtected(spArgsAsKEA.Get()));
                break;
            }
            case KnownEventIndex::UIElement_GotFocus:
                {
                    ctl::ComPtr<IRoutedEventArgs> spArgsAsREA =
                        ctl::query_interface_cast<IRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnGotFocusProtected(spArgsAsREA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_LostFocus:
                {
                    ctl::ComPtr<IRoutedEventArgs> spArgsAsREA =
                        ctl::query_interface_cast<IRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnLostFocusProtected(spArgsAsREA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_CharacterReceived:
                {
                    ctl::ComPtr<CharacterReceivedRoutedEventArgs> spArgsAsREA =
                        ctl::query_interface_cast<CharacterReceivedRoutedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnCharacterReceivedProtected(spArgsAsREA.Get()));
                    break;
                }

            case KnownEventIndex::UIElement_DragEnter:
                {
                    ctl::ComPtr<IDragEventArgs> spArgsAsDEA =
                        ctl::query_interface_cast<IDragEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnDragEnterProtected(spArgsAsDEA.Get()));
                    // NOTE: If we want to add visual feedback to the drag icon,
                    // here would be the place to set visual states on the drag icon.
                    break;
                }
            case KnownEventIndex::UIElement_DragLeave:
                {
                    ctl::ComPtr<IDragEventArgs> spArgsAsDEA =
                        ctl::query_interface_cast<IDragEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnDragLeaveProtected(spArgsAsDEA.Get()));
                    break;
                }
            case KnownEventIndex::UIElement_DragOver:
                {
                    ctl::ComPtr<IDragEventArgs> spArgsAsDEA =
                        ctl::query_interface_cast<IDragEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnDragOverProtected(spArgsAsDEA.Get()));
                    break;
                }
            case KnownEventIndex::UIElement_Drop:
                {
                    ctl::ComPtr<IDragEventArgs> spArgsAsDEA =
                        ctl::query_interface_cast<IDragEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnDropProtected(spArgsAsDEA.Get()));
                    break;
                }
                break;
            case KnownEventIndex::UIElement_BringIntoViewRequested:
                {
                    ctl::ComPtr<IBringIntoViewRequestedEventArgs> spArgsAsBIVEA =
                        ctl::query_interface_cast<IBringIntoViewRequestedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnBringIntoViewRequestedProtected(spArgsAsBIVEA.Get()));
                    break;
                }
                break;
            case KnownEventIndex::Control_IsEnabledChanged:
                {
                    ctl::ComPtr<IsEnabledChangedEventArgs> spTypedArgs =
                        ctl::query_interface_cast<IsEnabledChangedEventArgs>(pArgs);
                    IFC(pSenderAsControl->OnIsEnabledChanged(spTypedArgs.Get()));
                    break;
                }

            case KnownEventIndex::Control_InheritedPropertyChanged:
                {
                    IFC(pSenderAsControl->OnInheritedPropertyChanged(pArgs));
                    break;
                }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Control::ProcessAcceleratorsIfApplicable(
    _Inout_ ctl::ComPtr<IKeyRoutedEventArgs>& spArgsAsKEA,
    _In_ Control* const pSenderAsControl)
{
    BOOLEAN handled = FALSE;
    BOOLEAN handledShouldNotImpedeTextInput = FALSE;
    wsy::VirtualKey originalKey;
    wsy::VirtualKeyModifiers keyModifiers;

    IFCFAILFAST(spArgsAsKEA.Cast<KeyRoutedEventArgs>()->get_OriginalKey(&originalKey));
    IFC_RETURN(GetKeyboardModifiers(&keyModifiers));

    if (KeyboardAcceleratorUtility::IsKeyValidForAccelerators(originalKey, KeyboardAcceleratorUtility::MapVirtualKeyModifiersToIntegersModifiers(keyModifiers)))
    {
        IFC_RETURN(KeyboardAcceleratorUtility::ProcessKeyboardAccelerators(
            originalKey,
            keyModifiers,
            VisualTree::GetContentRootForElement(pSenderAsControl->GetHandle())->GetAllLiveKeyboardAccelerators(),
            pSenderAsControl->GetHandle(),
            &handled,
            &handledShouldNotImpedeTextInput,
            nullptr,
            false));

        if (handled)
        {
            IFCFAILFAST(spArgsAsKEA->put_Handled(TRUE));
        }
        if (handledShouldNotImpedeTextInput)
        {
            IFCFAILFAST(spArgsAsKEA.Cast<KeyRoutedEventArgs>()->put_HandledShouldNotImpedeTextInput(TRUE));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT Control::OnPointerEnteredImpl(_In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    return hr;
}

_Check_return_ HRESULT Control::OnPointerPressedImpl(_In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    return hr;
}

_Check_return_ HRESULT Control::OnPointerMovedImpl(_In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    return hr;
}

_Check_return_ HRESULT Control::OnPointerReleasedImpl(_In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    return hr;
}

_Check_return_ HRESULT Control::OnPointerExitedImpl(_In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    return hr;
}

// Override OnPointerCaptured() to handle ToolTips.  For mouse/keyboard ToolTips,
// we dismiss them.  For touch ToolTips, we set flag m_cancelAutomaticTouchToolTipOnCaptureLost
// to TRUE to remember that we must close the ToolTip when pointer capture is lost.
_Check_return_ HRESULT Control::OnPointerCaptured()
{
    HRESULT hr = S_OK;

    IFC(ControlGenerated::OnPointerCaptured());

    switch (ToolTipService::s_lastEnterInputMode)
    {
    case AutomaticToolTipInputMode::Mouse:
    case AutomaticToolTipInputMode::Keyboard:
        IFC(ToolTipService::CancelAutomaticToolTip());
        break;
    case AutomaticToolTipInputMode::Touch:
        m_cancelAutomaticTouchToolTipOnCaptureLost = TRUE;
        break;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Control::OnPointerCaptureLostImpl(_In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);

    if (m_cancelAutomaticTouchToolTipOnCaptureLost)
    {
        IFC(ToolTipService::CancelAutomaticToolTip());
    }

Cleanup:
    m_cancelAutomaticTouchToolTipOnCaptureLost = FALSE;
    return hr;
}

_Check_return_ HRESULT Control::OnPointerCanceledImpl(_In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    return hr;
}

_Check_return_ HRESULT Control::OnPointerWheelChangedImpl(_In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    return hr;
}

_Check_return_ HRESULT Control::OnTappedImpl(_In_ ITappedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    return hr;
}

_Check_return_ HRESULT Control::OnDoubleTappedImpl(_In_ IDoubleTappedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    return hr;
}

_Check_return_ HRESULT Control::OnHoldingImpl(_In_ IHoldingRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    return hr;
}

_Check_return_ HRESULT Control::OnRightTappedImpl(_In_ IRightTappedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    return hr;
}

_Check_return_ HRESULT Control::OnRightTappedUnhandled(_In_ IRightTappedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    return hr;
}

_Check_return_ HRESULT Control::OnManipulationStartingImpl(_In_ IManipulationStartingRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    return hr;
}

_Check_return_ HRESULT Control::OnManipulationInertiaStartingImpl(_In_ IManipulationInertiaStartingRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    return hr;
}

_Check_return_ HRESULT Control::OnManipulationStartedImpl(_In_ IManipulationStartedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Control::OnManipulationDeltaImpl(_In_ IManipulationDeltaRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Control::OnManipulationCompletedImpl(_In_ IManipulationCompletedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Control::ApplyTemplateImpl(_Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IControlTemplate> spTemplate;
    wxaml_interop::TypeName targetType = {};
    const CClassInfo* pTargetType = nullptr;
    bool bIsInstanceOf = false;

    IFCPTR(pReturnValue);

    IFC(get_Template(&spTemplate));

    if (spTemplate)
    {
        IFC(spTemplate->get_TargetType(&targetType));
        IFC(MetadataAPI::GetClassInfoByTypeName(targetType, &pTargetType));
        if (pTargetType)
        {
            // If there's a target type, verify we are an instance of it.
            IFC(MetadataAPI::IsInstanceOfType(ctl::iinspectable_cast(this), pTargetType, &bIsInstanceOf));
            if (!bIsInstanceOf)
            {
                IFC(ErrorHelper::OriginateErrorUsingResourceID(E_FAIL,ERROR_TEMPLATE_TARGETTYPE_WRONG));
            }
        }
    }

    hr = InvokeApplyTemplate(pReturnValue);
    if (FAILED(hr))
    {
        // Translate to XamlParseFailed error. The CLR knows how to translate this to
        // a XamlParseException.
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_XAML, E_XAMLPARSEFAILED);
    }
    IFC(hr);

Cleanup:
    DELETE_STRING(targetType.Name);
    RRETURN(hr);
}

_Check_return_ HRESULT Control::OnKeyUpImpl(_In_ IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Control::OnKeyDownImpl(_In_ IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Control::OnPreviewKeyDownImpl(_In_ IKeyRoutedEventArgs* pArgs)
{
    IFCPTR_RETURN(pArgs);

    // An optimisation to avoid marking this method
    // as virtual, when we only have one override:
    if (GetHandle()->OfTypeByIndex<KnownTypeIndex::Slider>())
    {
        ctl::ComPtr<ISlider> spThisAsSlider = ctl::query_interface_cast<ISlider>(this);
        if (spThisAsSlider)
        {
            IFC_RETURN(spThisAsSlider.Cast<Slider>()->OnPreviewKeyDownImpl(pArgs));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT Control::OnPreviewKeyUpImpl(_In_ IKeyRoutedEventArgs* pArgs)
{
    IFCPTR_RETURN(pArgs);

    // An optimisation to avoid marking this method
    // as virtual, when we only have one override:
    if (GetHandle()->OfTypeByIndex<KnownTypeIndex::Slider>())
    {
        ctl::ComPtr<ISlider> spThisAsSlider = ctl::query_interface_cast<ISlider>(this);
        if (spThisAsSlider)
        {
            IFC_RETURN(spThisAsSlider.Cast<Slider>()->OnPreviewKeyUpImpl(pArgs));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT Control::OnGotFocusImpl(_In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Control::OnLostFocusImpl(_In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Control::OnCharacterReceivedImpl(_In_ xaml_input::ICharacterReceivedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    RRETURN(hr);
}

// Called when the IsEnabled property changes.
_Check_return_ HRESULT Control::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* pArgs)
{
    RRETURN(S_OK);
}



// Inherited property changed handler.
_Check_return_ HRESULT Control::OnInheritedPropertyChanged(
    _In_ IInspectable* pArgs)
{
    RRETURN(S_OK);
}

// Update the current visual state of the control.
_Check_return_ HRESULT Control::UpdateVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;

    if (!m_fSuspendStateChanges)
    {
        IFC(ChangeVisualState(bUseTransitions));
    }

Cleanup:
    RRETURN(hr);
}

//Raise FocusEngaged and FocusDisengaged events and run
//default engagement visuals if necessary
_Check_return_ HRESULT Control::UpdateEngagementState(_In_ bool engage)
{
    if (engage)
    {
        FocusEngagedEventSourceType* pFocusEngagedEventSource = nullptr;
        ctl::ComPtr<FocusEngagedEventArgs> spRoutedEventArgs;

        IFC_RETURN(ctl::make<FocusEngagedEventArgs>(&spRoutedEventArgs));
        IFC_RETURN(spRoutedEventArgs->put_OriginalSource(ctl::as_iinspectable(this)));
        IFC_RETURN(spRoutedEventArgs->put_Handled(FALSE));

        IFC_RETURN(GetFocusEngagedEventSourceNoRef(&pFocusEngagedEventSource));

        IFC_RETURN(pFocusEngagedEventSource->Raise(this, spRoutedEventArgs.Get()));
        IFC_RETURN(OnFocusEngaged(spRoutedEventArgs.Get()));
    }
    else
    {
        FocusDisengagedEventSourceType* pEventSource = nullptr;
        ctl::ComPtr<FocusDisengagedEventArgs> spRoutedEventArgs;

        IFC_RETURN(ctl::make<FocusDisengagedEventArgs>(&spRoutedEventArgs));
        IFC_RETURN(spRoutedEventArgs->put_OriginalSource(ctl::as_iinspectable(this)));

        IFC_RETURN(GetFocusDisengagedEventSourceNoRef(&pEventSource));
        IFC_RETURN(pEventSource->Raise(this, spRoutedEventArgs.Get()));
    }
    return S_OK;
}

_Check_return_ HRESULT Control::DoesElementOverlapViewport(
    _In_ IUIElement* pCandidate,
    _Out_ bool& overlapsViewport)
{
    double horizontalOffset = 0;
    double verticalOffset = 0;
    double width;
    double height;
    const wf::Point zeroPoint{ 0,0 };
    ctl::ComPtr<IGeneralTransform> spTransform;
    ctl::ComPtr<IUIElement> spCandidateAsUI(pCandidate);
    ctl::ComPtr<IFrameworkElement> spCandidateAsFE;
    wf::Point offsetRelativeToElement;
    double candidateWidth;
    double candidateHeight;

    overlapsViewport = false;

    {
        ctl::ComPtr<IScrollViewer> spThisAsScrollViewer = ctl::query_interface_cast<IScrollViewer>(this);
        if (spThisAsScrollViewer)
        {
            IFC_RETURN(spThisAsScrollViewer.Cast<ScrollViewer>()->get_ViewportWidth(&width));
            IFC_RETURN(spThisAsScrollViewer.Cast<ScrollViewer>()->get_ViewportHeight(&height));
            IFC_RETURN(spThisAsScrollViewer.Cast<ScrollViewer>()->get_HorizontalOffset(&horizontalOffset));
            IFC_RETURN(spThisAsScrollViewer.Cast<ScrollViewer>()->get_VerticalOffset(&verticalOffset));
        }
        else
        {
            IFC_RETURN(get_ActualHeight(&height));
            IFC_RETURN(get_ActualWidth(&width));
        }
    }

    // check if the candidate is within the viewport
    IFC_RETURN(spCandidateAsUI->TransformToVisual(this, &spTransform));
    IFC_RETURN(spTransform->TransformPoint(zeroPoint, &offsetRelativeToElement));

    IFC_RETURN(spCandidateAsUI.As(&spCandidateAsFE));
    IFC_RETURN(spCandidateAsFE->get_ActualWidth(&candidateWidth));
    IFC_RETURN(spCandidateAsFE->get_ActualHeight(&candidateHeight));

    // candidate rectangle top left and bottom right points
    wf::Point candidateTopLeft{ static_cast<float>(offsetRelativeToElement.X + horizontalOffset),
        static_cast<float>(offsetRelativeToElement.Y + verticalOffset) };
    wf::Point candidateBottomRight{ static_cast<float>(candidateTopLeft.X + candidateWidth),
        static_cast<float>(candidateTopLeft.Y + candidateHeight) };

    // viewport top left and bottom right points
    wf::Point viewportTopLeft{ static_cast<float>(horizontalOffset), static_cast<float>(verticalOffset) };
    wf::Point viewportBottomRight{ static_cast<float>(viewportTopLeft.X + width),
        static_cast<float>(viewportTopLeft.Y + height) };

    // does the candidate overlaps the viewport
    overlapsViewport = !(candidateTopLeft.X > viewportBottomRight.X ||
        viewportTopLeft.X > candidateBottomRight.X ||
        candidateTopLeft.Y > viewportBottomRight.Y ||
        viewportTopLeft.Y > candidateBottomRight.Y);

    return S_OK;
}

_Check_return_ HRESULT Control::OnFocusEngaged(_In_ FocusEngagedEventArgs* pArgs)
{
    BOOLEAN isHandled = false;
    IFC_RETURN(pArgs->get_Handled(&isHandled));

    if (isHandled) { return S_OK; }

    ctl::ComPtr<xaml_input::IFocusManagerStaticsPrivate> spFocusManager;

    IFC_RETURN(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Input_FocusManager).Get(),
        &spFocusManager));

    if (!spFocusManager) { return S_OK; }

    ctl::ComPtr<IInspectable> spCandidate;
    ctl::ComPtr<Control> spControl(this);
    ctl::ComPtr<IInspectable> spControlAsI;

    bool focusCandidateOverlapsViewport = false;

    // Use autofocus api with a hint rectangle to find a candidate within the control,
    // If there is no candidate, fallback to first focusible item

    // create a hint rect above the control
    XRECTF_RB bounds = {};
    IFC_RETURN(GetHandle()->GetGlobalBounds(&bounds, true /*ignoreClipping*/));

    // Rect above the ScrollViewer that is 2 physical pixels high and spans the viewport width
    wf::Rect hintRect = { bounds.left, bounds.top - 3, bounds.right - bounds.left, 2 };

    // FindNextFocusWithSearchRootIgnoreEngagementWithHintRect expect rects in Dips.
    const auto scale = RootScale::GetRasterizationScaleForElement(GetHandle());
    DXamlCore::GetCurrent()->PhysicalPixelsToDips(scale, &hintRect, &hintRect);

    // let the exclusion rect be the same as the hint rect
    // that way, we look for an item that is completely inside the new region that is below/right of the hintRect
    // in other words, we will not get items that are intersecting with the hint rect
    const wf::Rect exclusionRect = hintRect;

    IFC_RETURN(spControl.As(&spControlAsI));

    IFC_RETURN(spFocusManager->FindNextFocusWithSearchRootIgnoreEngagementWithHintRect(
        xaml_input::FocusNavigationDirection::FocusNavigationDirection_Down /* focusNavigationDirection */,
        spControlAsI.Get(), /* searchStart */
        hintRect, /* hintRect */
        exclusionRect, /* exclusionRect */
        spCandidate.GetAddressOf()));

    if (spCandidate)
    {
        ctl::ComPtr<IUIElement> spCandidateAsUIElement;

        IFC_RETURN(UIElement::GetUIElementFocusCandidate(spCandidate.Get(), &spCandidateAsUIElement));
        IFC_RETURN(DoesElementOverlapViewport(spCandidateAsUIElement.Get(), focusCandidateOverlapsViewport));

        BOOLEAN isTabStop = FALSE;
        IFC_RETURN(get_IsTabStop(&isTabStop));

        // if the control is not a tab stop, forward focus.
        // if it is a tab stop, forward focus only if the candidate is in the viewport
        if (!isTabStop || focusCandidateOverlapsViewport)
        {
            ctl::ComPtr<DependencyObject> spCandidateAsDO;
            BOOLEAN focusUpdated = FALSE;

            IFC_RETURN(spCandidate.As(&spCandidateAsDO));
            IFC_RETURN(SetFocusedElement(spCandidateAsDO.Get(),
                xaml::FocusState_Programmatic,
                true /* animateIfBringIntoView */,
                &focusUpdated));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT Control::OnDragEnterImpl(_In_ xaml::IDragEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    return hr;
}

_Check_return_ HRESULT Control::OnDragLeaveImpl(_In_ xaml::IDragEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    return hr;
}

_Check_return_ HRESULT Control::OnDragOverImpl(_In_ xaml::IDragEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    return hr;
}

_Check_return_ HRESULT Control::OnDropImpl(_In_ xaml::IDragEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFCPTR(pArgs);
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
Control::OnApplyTemplate()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spFocusVisualWhiteDO;
    ctl::ComPtr<xaml::IDependencyObject> spFocusVisualBlackDO;

    IFC(ControlGenerated::OnApplyTemplate());

    // Default control templates style focus rects as alternate black and white rectangles with stroke dash arrays,
    // to produce the effect of alternating black and white stroke dashes. Stroke dash array values and offsets are multipliers on stroke thickness,
    // which defaults to 1 which is rendered on subpixel boundaries at plateau > 1.
    // Stroke thickness needs to be rounded using plateau-aware rounding utility to render at pixel boundaries.
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"FocusVisualWhite")).Get(), &spFocusVisualWhiteDO));
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"FocusVisualBlack")).Get(), &spFocusVisualBlackDO));
    if (spFocusVisualWhiteDO &&
        spFocusVisualBlackDO &&
        ctl::is<IRectangle>(spFocusVisualWhiteDO) &&
        ctl::is<IRectangle>(spFocusVisualBlackDO))
    {
        IFC(LayoutRoundRectangleStrokeThickness(spFocusVisualWhiteDO.Cast<Rectangle>()));
        IFC(LayoutRoundRectangleStrokeThickness(spFocusVisualBlackDO.Cast<Rectangle>()));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Control::LayoutRoundRectangleStrokeThickness(
    _In_ DirectUI::Rectangle *pRectangle
    )
{
    HRESULT hr = S_OK;
    BOOLEAN roundStrokeThickness = FALSE;
    DOUBLE strokeThickness = 0.0f;
    FLOAT strokeThicknessFloat = 0.0f;

    IFC(pRectangle->get_UseLayoutRounding(&roundStrokeThickness));
    if (roundStrokeThickness)
    {
        IFC(pRectangle->get_StrokeThickness(&strokeThickness));
        strokeThicknessFloat = static_cast<FLOAT>(strokeThickness);
        IFC(LayoutRound(strokeThicknessFloat, &strokeThicknessFloat));
        IFC(pRectangle->put_StrokeThickness(strokeThicknessFloat));
    }

Cleanup:
    RRETURN(hr);
}

// Change to the correct visual state for the control.
_Check_return_ HRESULT Control::ChangeVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions)
{
    return S_OK;
}

// Go to a specific visual state.
_Check_return_ HRESULT Control::GoToState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions,
    // The name of the state to transition to.
    _In_z_ const wchar_t* pszState,
    _Out_ BOOLEAN* pbReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pszState);
    IFCPTR(pbReturnValue);

    IFC(VisualStateManager::GoToState(this, wrl_wrappers::HStringReference(pszState).Get(), bUseTransitions, pbReturnValue));

Cleanup:
    RRETURN(hr);
}

// Attempts to find the VisualStateGroups in the Control.
// Sets ppVisualStateGroups to NULL if the VisualStates are not defined.
_Check_return_ HRESULT Control::GetVisualStateGroups(
    _Outptr_result_maybenull_ wfc::IVector<xaml::VisualStateGroup*>** ppVisualStateGroups)
{
    HRESULT hr = S_OK;

    INT childCount = 0;

    IFCPTR(ppVisualStateGroups);

    *ppVisualStateGroups = nullptr;

    IFC(VisualTreeHelper::GetChildrenCountStatic(this, &childCount));
    if (childCount > 0)
    {
        ctl::ComPtr<IDependencyObject> spChild;
        ctl::ComPtr<IFrameworkElement> spChildAsFE;

        // VisualStates are on the first child.
        IFC(VisualTreeHelper::GetChildStatic(this, 0, &spChild));
        spChildAsFE = spChild.AsOrNull<IFrameworkElement>();

        if (spChildAsFE)
        {
            ctl::ComPtr<wfc::IVector<xaml::VisualStateGroup*>> spChildVisualStateGroups;
            ctl::ComPtr<VisualStateManagerFactory> spFactory;
            IFC(ctl::make<VisualStateManagerFactory>(&spFactory));
            IFC(spFactory->GetVisualStateGroups(spChildAsFE.Get(), &spChildVisualStateGroups));

            if (spChildVisualStateGroups)
            {
                IFC(spChildVisualStateGroups.MoveTo(ppVisualStateGroups));
            }
        }
    }


Cleanup:
    RRETURN(hr);
}

// Get the currently pressed keyboard modifiers.
_Check_return_ HRESULT Control::GetKeyboardModifiers(
    _Out_ wsy::VirtualKeyModifiers* pModifierKeys)
{
    HRESULT hr = S_OK;

    IFCPTR(pModifierKeys);

    IFC(CoreImports::Input_GetKeyboardModifiers(pModifierKeys));

Cleanup:
    RRETURN(hr);
}

// Suspend state changes for the Control.
Control::StateChangeSuspender::StateChangeSuspender(
    _In_ Control* pControl,
    _In_ HRESULT* pResult)
    : m_spControl(pControl)
    , m_pResult(pResult)
{
    if (m_spControl)
    {
        m_spControl->m_fSuspendStateChanges = TRUE;
    }
}

// Resume state changes for the Control.
Control::StateChangeSuspender::~StateChangeSuspender()
{
    HRESULT hr = S_OK;

    // Suspend state changes and update the visual states
    if (m_spControl)
    {
        m_spControl->m_fSuspendStateChanges = FALSE;
        IFC(m_spControl->UpdateVisualState());
    }

Cleanup:
    // If all of the calls in the scope of the StateChangeSuspender succeeded
    // but the UpdateVisualState call failed, then we'll pass along the failing
    // UpdateVisualState result.
    if (SUCCEEDED(*m_pResult) && FAILED(hr))
    {
        *m_pResult = hr;
    }
}

_Check_return_
HRESULT
Control::GetIsEnabledChangedEventSourceNoRef(
    _Outptr_ CIsEnabledChangedEventSource** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::Control_IsEnabledChanged, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<CIsEnabledChangedEventSource>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::Control_IsEnabledChanged, this);
        IFC(StoreEventSource(KnownEventIndex::Control_IsEnabledChanged, *ppEventSource));

        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
Control::add_IsEnabledChanged(
    _In_ IDependencyPropertyChangedEventHandler* pValue,
    _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    CIsEnabledChangedEventSource* pEventSourceNoRef = nullptr;

    IFC(GetIsEnabledChangedEventSourceNoRef(&pEventSourceNoRef));
    IFC(pEventSourceNoRef->AddHandler(pValue));

    ptToken->value = reinterpret_cast<INT64>(pValue);

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
Control::remove_IsEnabledChanged(
    _In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    CIsEnabledChangedEventSource* pEventSourceNoRef = nullptr;
    IDependencyPropertyChangedEventHandler* pValue = reinterpret_cast<IDependencyPropertyChangedEventHandler*>(tToken.value);

    IFC(GetIsEnabledChangedEventSourceNoRef(&pEventSourceNoRef));
    IFC(pEventSourceNoRef->RemoveHandler(pValue));

    tToken.value = 0;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Control::GetValue(
    _In_ const CDependencyProperty* pDP,
    _Outptr_ IInspectable **ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pDP);
    IFCPTR(ppValue);

    if (!m_fHasCustomDefaultStyleKey && pDP->GetIndex() == KnownPropertyIndex::Control_DefaultStyleKey)
    {
        IFC(get_DefaultStyleKey(ppValue));
    }
    else
    {
        IFC(ControlGenerated::GetValue(pDP, ppValue));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Control::OnPropertyChanged2(
    _In_ const PropertyChangedParams& args)
{
    IFC_RETURN(ControlGenerated::OnPropertyChanged2(args));

    if (args.m_pDP->GetIndex() == KnownPropertyIndex::UIElement_Visibility)
    {
        IFC_RETURN(OnVisibilityChanged());
    }
    else if (args.m_pDP->GetIndex() == KnownPropertyIndex::Control_IsFocusEngagementEnabled)
    {
        //Will disengage only if the Control is engaged
        IFC_RETURN(RemoveFocusEngagementImpl());
    }
    else if (args.m_pDP->GetIndex() == KnownPropertyIndex::Control_IsFocusEngaged)
    {
        IFC_RETURN(SetFocusEngagementImpl());
    }

    return S_OK;
}

_Check_return_ HRESULT Control::RemoveFocusEngagementImpl()
{
    IFC_RETURN(static_cast<CControl*>(GetHandle())->RemoveFocusEngagement());
    return S_OK;
}

_Check_return_ HRESULT Control::OnContextRequestedImpl(_In_ xaml_input::IContextRequestedEventArgs * pArgs)
{
    return ShowContextFlyout(pArgs, this);
}

_Check_return_ HRESULT Control::ShowContextFlyout(
    _In_ xaml_input::IContextRequestedEventArgs * pArgs,
    _In_ DirectUI::Control * pContextFlyoutControl)
{
    ctl::ComPtr<xaml_input::IContextRequestedEventArgs> args(pArgs);

    // Unlike GetHandle(), GetCorePeer() adds a reference to its return value,
    // so we need to make sure we clean it up.
    xref_ptr<CEventArgs> coreArgs;
    coreArgs.attach(args.Cast<ContextRequestedEventArgs>()->GetCorePeer());

    IFC_RETURN(CUIElement::OnContextRequestedCore(GetHandle(), pContextFlyoutControl->GetHandle(), coreArgs.get()));
    return S_OK;
}

_Check_return_ HRESULT Control::SetFocusEngagementImpl()
{
    IFC_RETURN(static_cast<CControl*>(GetHandle())->SetFocusEngagement());
    return S_OK;
}

bool Control::IsValueRequired(_In_ xaml_controls::IInputValidationControl* control)
{
    boolean isRequired = FALSE;

    wrl::ComPtr<xaml_controls::IInputValidationContext> context;
    IFCFAILFAST(control->get_ValidationContext(&context));
    if (context != nullptr)
    {
        IFCFAILFAST(context->get_IsInputRequired(&isRequired));
    }

    return !!isRequired;
}

_Check_return_ HRESULT Control::InvokeValidationCommand(_In_ xaml_controls::IInputValidationControl2* control, _In_ HSTRING value)
{
    wrl::ComPtr<IInspectable> valueInsp;
    IFC_RETURN(StaticStore::GetPropertyValueStaticsNoRef()->CreateString(value, &valueInsp));
    IFC_RETURN(InvokeValidationCommand(control, valueInsp.Get()));
    return S_OK;
}

_Check_return_ HRESULT Control::InvokeValidationCommand(_In_ xaml_controls::IInputValidationControl2* control2, _In_ IInspectable* value)
{
    wrl::ComPtr<xaml_controls::IInputValidationCommand> validationCommand;
    IFC_RETURN(control2->get_ValidationCommand(&validationCommand));

    if (validationCommand)
    {
        ctl::ComPtr<xaml_controls::IInputValidationControl> control;
        IFC_RETURN(control2->QueryInterface<xaml_controls::IInputValidationControl>(&control));

        // Custom validationcommands can override the default behavior. Query the command
        // to see if we should validate
        boolean canValidate = FALSE;
        IFC_RETURN(validationCommand->CanValidate(control.Get(), &canValidate));

        if (canValidate)
        {
            IFC_RETURN(validationCommand->Validate(control.Get()));
        }
    }

    return S_OK;
}