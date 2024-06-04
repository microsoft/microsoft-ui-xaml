// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CInputScope.h"
#include "PasswordBox.g.h"
#include "TextBox.g.h"
#include "PasswordBoxAutomationPeer.g.h"
#include "ButtonBase.g.h"
#include "ToggleButton.g.h"
#include "PointerRoutedEventArgs.g.h"
#include "DoubleTappedRoutedEventArgs.g.h"
#include "TappedRoutedEventArgs.g.h"
#include "RightTappedRoutedEventArgs.g.h"
#include "HoldingRoutedEventArgs.g.h"
#include "ManipulationStartedRoutedEventArgs.g.h"
#include "ManipulationCompletedRoutedEventArgs.g.h"
#include "IsEnabledChangedEventArgs.g.h"
#include "CharacterReceivedRoutedEventArgs.g.h"
#include <XamlTraceLogging.h>
#include "AutomationProperties.h"
#include "TextBoxPlaceholderTextHelper.h"
#include "TextControlHelper.h"
#include "localizedResource.h"
#include "ContextRequestedEventArgs.g.h"


using namespace DirectUI;
using namespace DirectUISynonyms;

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance.
//
//---------------------------------------------------------------------------
PasswordBox::PasswordBox()
    : m_isInitializing(TRUE)
    , m_revealButtonPressed(false)
    , m_revealButtonCheckBox(false)
{
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Releases resources held by an instance.
//
//---------------------------------------------------------------------------
PasswordBox::~PasswordBox()
{
    VERIFYHR(ReleaseTemplateParts());
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP PasswordBox::OnPointerEntered(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(PasswordBoxGenerated::OnPointerEntered(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<PointerRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerEntered));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP PasswordBox::OnPointerExited(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(PasswordBoxGenerated::OnPointerExited(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<PointerRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerExited));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP PasswordBox::OnPointerPressed(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(PasswordBoxGenerated::OnPointerPressed(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<PointerRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerPressed));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP PasswordBox::OnPointerMoved(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(PasswordBoxGenerated::OnPointerMoved(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<PointerRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerMoved));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP PasswordBox::OnPointerReleased(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(PasswordBoxGenerated::OnPointerReleased(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<PointerRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerReleased));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP PasswordBox::OnPointerCaptureLost(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(PasswordBoxGenerated::OnPointerCaptureLost(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<PointerRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerCaptureLost));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP PasswordBox::OnPointerCanceled(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(PasswordBoxGenerated::OnPointerCanceled(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<PointerRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerCanceled));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP PasswordBox::OnDoubleTapped(_In_ xaml_input::IDoubleTappedRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(PasswordBoxGenerated::OnDoubleTapped(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<DoubleTappedRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_DoubleTapped));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP PasswordBox::OnTapped(_In_ xaml_input::ITappedRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(PasswordBoxGenerated::OnTapped(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<TappedRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_Tapped));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP PasswordBox::OnRightTapped(_In_ xaml_input::IRightTappedRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(PasswordBoxGenerated::OnRightTapped(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<RightTappedRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_RightTapped));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP PasswordBox::OnHolding(_In_ xaml_input::IHoldingRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(PasswordBoxGenerated::OnHolding(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<HoldingRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_Holding));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP PasswordBox::OnManipulationStarted(_In_ xaml_input::IManipulationStartedRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(PasswordBoxGenerated::OnManipulationStarted(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<ManipulationStartedRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_ManipulationStarted));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP PasswordBox::OnManipulationCompleted(_In_ xaml_input::IManipulationCompletedRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(PasswordBoxGenerated::OnManipulationCompleted(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<ManipulationCompletedRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_ManipulationCompleted));
    }

    return S_OK;
}

_Check_return_ HRESULT PasswordBox::OnContextRequestedImpl(
    _In_ xaml_input::IContextRequestedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(ValidateEvent(static_cast<ContextRequestedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(ShowContextFlyout(pArgs, this));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP PasswordBox::OnKeyUp(_In_ IKeyRoutedEventArgs* pArgs)
{
    IFC_RETURN(PasswordBoxGenerated::OnKeyUp(pArgs));
    IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_KeyUp));
    wsy::VirtualKey key = wsy::VirtualKey_None;
    IFC_RETURN(pArgs->get_Key(&key));

    if (key == wsy::VirtualKey_F8)
    {
        IFC_RETURN(static_cast<CPasswordBox*>(GetHandle())->RevealPassword(FALSE));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP PasswordBox::OnKeyDown(_In_ IKeyRoutedEventArgs* pArgs)
{

    IFC_RETURN(PasswordBoxGenerated::OnKeyDown(pArgs));
    IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_KeyDown));

    wsy::VirtualKey key = wsy::VirtualKey_None;
    IFC_RETURN(pArgs->get_Key(&key));

    if (key == wsy::VirtualKey_F8)
    {
        wuc::CorePhysicalKeyStatus keyStatus;
        IFC_RETURN(pArgs->get_KeyStatus(&keyStatus));
        if (keyStatus.IsMenuKeyDown)
        {
            IFC_RETURN(static_cast<CPasswordBox*>(GetHandle())->RevealPassword(TRUE));
        }
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP PasswordBox::OnGotFocus(_In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(PasswordBoxGenerated::OnGotFocus(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_GotFocus));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP PasswordBox::OnLostFocus(_In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(PasswordBoxGenerated::OnLostFocus(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_LostFocus));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a PasswordBoxAutomationPeer to represent the PasswordBox.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP PasswordBox::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IPasswordBoxAutomationPeer> spPasswordBoxAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IPasswordBoxAutomationPeerFactory> spPasswordBoxAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::PasswordBoxAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spPasswordBoxAPFactory));

    IFC(spPasswordBoxAPFactory.Cast<PasswordBoxAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spPasswordBoxAutomationPeer));
    IFC(spPasswordBoxAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a plain text string to provide a default AutomationProperties.Name
//        in the absence of an explicitly defined one
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT PasswordBox::GetPlainText(_Out_ HSTRING* strPlainText)
{
    ctl::ComPtr<IInspectable> spHeader;
    XUINT32 pLength = 0;
    *strPlainText = nullptr;

    IFC_RETURN(get_Header(&spHeader));

    if (spHeader != nullptr)
    {
        IFC_RETURN(FrameworkElement::GetStringFromObject(spHeader.Get(), strPlainText));
        pLength = ::WindowsGetStringLen(*strPlainText);
    }

    if (pLength == 0)
    {
        IFC_RETURN(get_PlaceholderText(strPlainText));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT PasswordBox::OnCharacterReceivedImpl(_In_ xaml_input::ICharacterReceivedRoutedEventArgs* pArgs)
{
    IFC_RETURN(TextBox::RaiseNative(this, ctl::as_iinspectable(pArgs), KnownEventIndex::UIElement_CharacterReceived));
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, raises event in the core layer
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT PasswordBox::OnInheritedPropertyChanged(_In_ IInspectable* pArgs)
{
    HRESULT hr = S_OK;
    IFC(PasswordBoxGenerated::OnInheritedPropertyChanged(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::Control_InheritedPropertyChanged));
Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IsEnabled property changed override, raises event in the core layer
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT PasswordBox::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(PasswordBoxGenerated::OnIsEnabledChanged(pArgs));
    IFC(TextBox::RaiseNative(this, ctl::iinspectable_cast(pArgs), KnownEventIndex::Control_IsEnabledChanged));
Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Callback from core, adds an IsPressed property change listener on
//      the template's RevealButton (if present).
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT PasswordBox::OnApplyTemplateHandler(_In_ CPasswordBox *pNativePasswordBox)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spPeer;
    ctl::ComPtr<IDependencyObject> spObject;
    ctl::ComPtr<IButtonBase> spButton;
    ctl::ComPtr<IDPChangedEventSource> spDPChangedEventSource;
    ctl::ComPtr<IUIElement> spPlaceholderTextPresenter;
    PasswordBox* pPasswordBoxNoRef = NULL;
    HSTRING pPasswordBoxText = NULL;

    //
    // Get the framework peer.
    //

    IFC(DXamlCore::GetCurrent()->GetPeer(pNativePasswordBox, &spPeer));
    pPasswordBoxNoRef = spPeer.Cast<PasswordBox>();

    //
    // Cleanup any state from a previous template.
    //

    IFC(pPasswordBoxNoRef->ReleaseTemplateParts());

    //
    // Find the RevealButton in the current template.
    //

    IFC(pPasswordBoxNoRef->GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"RevealButton")).Get(), &spObject));
    IFC(pPasswordBoxNoRef->GetTemplatePart<IUIElement>(STR_LEN_PAIR(L"PlaceholderTextContentPresenter"), spPlaceholderTextPresenter.ReleaseAndGetAddressOf()));
    IFC(spObject.As(&spButton));

    pPasswordBoxNoRef->SetRevealButton(spButton.Get());
    pPasswordBoxNoRef->SetPlaceholderTextPresenter(spPlaceholderTextPresenter.Get());

    if (spButton) // NULL when removed from default template.
    {
        ctl::WeakRefPtr wrWeakThis;
        IFC(ctl::AsWeak(pPasswordBoxNoRef, &wrWeakThis));

        static_cast<CFrameworkElement*>(spButton.Cast<ButtonBase>()->GetHandle())->SetCursor(MouseCursorArrow);

        bool isRevealToggleButton = !!ctl::is<IToggleButton>(spButton);
        bool isRevealCheckBox = !!ctl::is<ICheckBox>(spButton);
        if (isRevealCheckBox)
        {
            pPasswordBoxNoRef->SetRevealButtoneIsCheckBox();
        }

        // hook up Checked/UnChecked event handler if reveal button is a toggle button
        if (isRevealToggleButton)
        {
            if (!isRevealCheckBox)
            {
                // Update reveal button's AutomationProperties.Name to "Show Password" by default
                wrl_wrappers::HString automationName;
                IFC(AutomationProperties::GetNameStatic(spButton.Cast<ButtonBase>(), automationName.ReleaseAndGetAddressOf()));
                if (automationName.Get() == nullptr)
                {
                    IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_PASSWORDBOX_REVEAL, automationName.ReleaseAndGetAddressOf()));
                    IFC(DirectUI::AutomationProperties::SetNameStatic(spButton.Cast<ButtonBase>(), automationName.Get()));
                }
            }

            ctl::ComPtr<IToggleButton> spToggleButton;

            IFC(spButton.As(&spToggleButton));
            IFC(spToggleButton->put_IsChecked(FALSE));

            //
            // Attach the Checked event handler.
            //
            hr = pPasswordBoxNoRef->m_epRevealButtonCheckedHandler.AttachEventHandler(spToggleButton.Cast<ToggleButton>(),
                [wrWeakThis](IInspectable* pSender, xaml::IRoutedEventArgs* pArgs) mutable
            {
                HRESULT hr = S_OK;
                ctl::ComPtr<IPasswordBox> spThis;

                spThis = wrWeakThis.AsOrNull<IPasswordBox>();
                if (spThis)
                {
                    // Reveal password on Phone Blue or triggered through UIA toggle provider
                    if (spThis.Cast<PasswordBox>()->IsRevealButtonCheckbox() || !spThis.Cast<PasswordBox>()->IsRevealButtonPressed())
                    {
                        IFC(spThis.Cast<PasswordBox>()->OnRevealButtonChecked());
                    }
                    else
                    {
                        // reset the reveal button pressed flag
                        IFC(spThis.Cast<PasswordBox>()->SetRevealButtonPressed(false));
                    }
                }
            Cleanup:
                RRETURN(hr);
            });

            IFC(hr);

            //
            // Attach the Unchecked event handler.
            //
            hr = pPasswordBoxNoRef->m_epRevealButtonUncheckedHandler.AttachEventHandler(spToggleButton.Cast<ToggleButton>(),
                [wrWeakThis](IInspectable* pSender, xaml::IRoutedEventArgs* pArgs) mutable
            {
                HRESULT hr = S_OK;
                ctl::ComPtr<IPasswordBox> spThis;

                spThis = wrWeakThis.AsOrNull<IPasswordBox>();
                if (spThis)
                {
                    // Hide password on Phone Blue or triggered through UIA toggle provider
                    if (spThis.Cast<PasswordBox>()->IsRevealButtonCheckbox() || !spThis.Cast<PasswordBox>()->IsRevealButtonPressed())
                    {
                        IFC(spThis.Cast<PasswordBox>()->OnRevealButtonUnchecked());
                    }
                    else
                    {
                        // reset the reveal button pressed flag
                        IFC(spThis.Cast<PasswordBox>()->SetRevealButtonPressed(false));
                    }
                }
            Cleanup:
                RRETURN(hr);
            });

            IFC(hr);
        }

        if (!isRevealCheckBox)
        {
            ctl::ComPtr<RevealButtonPropertyChangedHandler> spRevealButtonPropertyChangedHandler;

            //
            // Attach an IsPressed change listener.
            //
            spRevealButtonPropertyChangedHandler.Attach(new RevealButtonPropertyChangedHandler(pPasswordBoxNoRef));
            pPasswordBoxNoRef->SetRevealButtonPropertyChangedHandler(spRevealButtonPropertyChangedHandler.Get());

            IFC(spButton.Cast<ButtonBase>()->GetDPChangedEventSource(&spDPChangedEventSource));
            IFC(spDPChangedEventSource->AddHandler(spRevealButtonPropertyChangedHandler.Get()));

            //
            // Attach the SizeChanged event handler.
            //
            hr = pPasswordBoxNoRef->m_epSizeChangedHandler.AttachEventHandler(spButton.Cast<ButtonBase>(),
                [wrWeakThis](IInspectable* pSender, xaml::ISizeChangedEventArgs* pArgs) mutable
            {
                HRESULT hr = S_OK;
                ctl::ComPtr<IPasswordBox> spThis;

                spThis = wrWeakThis.AsOrNull<IPasswordBox>();
                if (spThis)
                {
                    IFC(spThis.Cast<PasswordBox>()->OnRevealButtonSizeChanged(pArgs));
                }
            Cleanup:
                RRETURN(hr);
            });

            IFC(hr);

            hr = pPasswordBoxNoRef->m_epButtonPointerReleasedHandler.AttachEventHandler(spButton.Cast<ButtonBase>(),
                [wrWeakThis](IInspectable* pSender, xaml_input::IPointerRoutedEventArgs* pArgs) mutable
            {
                return (pArgs->put_Handled(TRUE));
            });

            IFC(hr);
        }
        else
        {
            // Blue phone
            ctl::ComPtr<IToggleButton> spToggleButton;
            BOOLEAN fIsPropertyLocal = FALSE;

            IFC(spButton.As(&spToggleButton));
            IFC(spToggleButton.Cast<ToggleButton>()->IsPropertyLocal(
                MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ContentControl_Content),
                &fIsPropertyLocal));

            // if content hasn't been set on the templated part then use localized string
            if (!fIsPropertyLocal)
            {
                // Get factory for PropertyValue to wrap string for consumption as ToggleButton.Content
                ctl::ComPtr<wf::IPropertyValueStatics> spPropertyValueFactory;

                ctl::ComPtr<wf::IPropertyValue> spContentAsPV;
                wrl_wrappers::HString strShowPasswordText;

                IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(), spPropertyValueFactory.ReleaseAndGetAddressOf()));

                IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(TEXT_PASSWORDBOX_SHOW_PASSWORD, strShowPasswordText.ReleaseAndGetAddressOf()));
                IFC(spPropertyValueFactory->CreateString(strShowPasswordText.Get(), &spContentAsPV));
                IFC(spToggleButton.Cast<ToggleButton>()->put_Content(ctl::as_iinspectable(spContentAsPV.Get())));
            }

            pNativePasswordBox->m_fAlwaysShowRevealButton = true;
        }
    }

    IFC(pPasswordBoxNoRef->UpdateHeaderPresenterVisibility());

    IFC(pPasswordBoxNoRef->get_Password(&pPasswordBoxText));
    IFC(pPasswordBoxNoRef->UpdatePlaceholderTextPresenterVisibility(pPasswordBoxText == NULL));

Cleanup:
    if (pPasswordBoxNoRef)
    {
        pPasswordBoxNoRef->m_isInitializing = FALSE;
    }
    DELETE_STRING(pPasswordBoxText);

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Callback from core, toggles the Visibility of Placeholder Text
//      whenever text is changed.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT PasswordBox::ShowPlaceholderTextHandler(_In_ CDependencyObject* pNativePasswordBox, _In_ bool isEnabled)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spPeer;
    PasswordBox* pPasswordBoxNoRef = NULL;

    //
    // Get the framework peer.
    //

    IFC(DXamlCore::GetCurrent()->GetPeer(pNativePasswordBox, &spPeer));
    pPasswordBoxNoRef = spPeer.Cast<PasswordBox>();

    IFC(pPasswordBoxNoRef->UpdatePlaceholderTextPresenterVisibility(isEnabled));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Releases resources associated with the current template.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT PasswordBox::ReleaseTemplateParts()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDPChangedEventSource> spDPChangedEventSource;

    if (auto peg = m_tpRevealButton.TryMakeAutoPeg())
    {
        if (m_tpRevealButtonPropertyChangedHandler)
        {
            IFC(m_tpRevealButton.Cast<ButtonBase>()->TryGetDPChangedEventSource(&spDPChangedEventSource));

            if (spDPChangedEventSource)
            {
                // In the unlikely event that we failed to add the handler we could fail here.
                IGNOREHR(spDPChangedEventSource->RemoveHandler(m_tpRevealButtonPropertyChangedHandler.Get()));
            }
        }

        IFC(DetachHandler(m_epSizeChangedHandler, m_tpRevealButton));
        IFC(DetachHandler(m_epRevealButtonCheckedHandler, m_tpRevealButton));
        IFC(DetachHandler(m_epRevealButtonUncheckedHandler, m_tpRevealButton));
        IFC(DetachHandler(m_epButtonPointerReleasedHandler, m_tpRevealButton));
    }

Cleanup:
    m_tpRevealButtonPropertyChangedHandler.Clear();
    m_tpRevealButton.Clear();
    m_tpHeaderPresenter.Clear();
    m_tpPlaceholderTextPresenter.Clear();
    m_requiredHeaderPresenter.Clear();
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the ContentElement's size is changed.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT PasswordBox::OnRevealButtonSizeChanged(
    _In_ xaml::ISizeChangedEventArgs* pArgs
)
{
    HRESULT hr = S_OK;

    // Height of the RevealButton is changing when the height of the TextBox is changing.
    // In order to maintain square button, set the width whenever height is changed.
    if (m_tpRevealButton)
    {
        wf::Size newRevealButtonSize;
        DOUBLE revealButtonWidth;

        IFC(pArgs->get_NewSize(&newRevealButtonSize));
        IFC(m_tpRevealButton.Cast<ButtonBase>()->get_Width(&revealButtonWidth));

        if (newRevealButtonSize.Height != revealButtonWidth)
        {
            IFC(m_tpRevealButton.Cast<ButtonBase>()->put_Width(newRevealButtonSize.Height));
        }
    }

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the Reveal button is checked
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT PasswordBox::OnRevealButtonChecked()
{
    HRESULT hr = S_OK;

    if (IsRevealButtonCheckbox())
    {
        BOOLEAN returnValue = FALSE; // unused
        IFC(Focus(xaml::FocusState_Pointer, &returnValue));
        IFC(static_cast<CPasswordBox*>(GetHandle())->RevealPassword(TRUE));
    }
    else
    {
        if (!static_cast<CPasswordBox*>(GetHandle())->IsPasswordRevealed())
        {
            IFC(static_cast<CPasswordBox*>(GetHandle())->RevealPassword(TRUE, false /*syncToggleButtonState*/));
        }
    }


Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the Reveal button is unchecked
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT PasswordBox::OnRevealButtonUnchecked()
{
    HRESULT hr = S_OK;

    if (IsRevealButtonCheckbox())
    {
        BOOLEAN returnValue = FALSE; // unused
        IFC(Focus(xaml::FocusState_Pointer, &returnValue));
        IFC(static_cast<CPasswordBox*>(GetHandle())->RevealPassword(FALSE));
    }
    else
    {
        if (static_cast<CPasswordBox*>(GetHandle())->IsPasswordRevealed())
        {
            IFC(static_cast<CPasswordBox*>(GetHandle())->RevealPassword(FALSE, false /*syncToggleButtonState*/));
        }
    }

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance.
//
//---------------------------------------------------------------------------
PasswordBox::RevealButtonPropertyChangedHandler::RevealButtonPropertyChangedHandler(
    _In_ PasswordBox *pPasswordBox
) :
    m_pPasswordBoxNoRef(pPasswordBox)
{
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever a property changes on the RevealButton.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP PasswordBox::RevealButtonPropertyChangedHandler::Invoke(
    _In_ xaml::IDependencyObject *pSender,
    _In_ const CDependencyProperty* pDP
)
{
    HRESULT hr = S_OK;

    // If the IsPressed property changed, let the core CPasswordBox know.
    if (KnownPropertyIndex::ButtonBase_IsPressed == pDP->GetIndex())
    {
        ctl::ComPtr<ButtonBase> spRevealButton;
        BOOLEAN bIsRevealed = false;

        IFC(ctl::do_query_interface(spRevealButton, pSender));
        IFC(spRevealButton->get_IsPressed(&bIsRevealed));
        // We don't want to sync the toggle button checked/unchecked state here since it will mess up with the RevealButtonPressed flag when Checked/UnChecked
        // handler if called back. Instead, we will reset button to unchecked state when user finished pressing, in the call to SetRevealButtonPressed(false)
        IFC(static_cast<CPasswordBox*>(m_pPasswordBoxNoRef->GetHandle())->RevealPassword(!!bIsRevealed, false /*syncToggleButtonState*/));
        if (bIsRevealed)
        {
            m_pPasswordBoxNoRef->SetRevealButtonPressed(true);
        }

        //Log "OnPasswordRevealedChanged" Event
        TraceLoggingWrite(g_hTraceProvider,
            "PasswordBoxShowPassword",
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY),
            TraceLoggingValue(bIsRevealed, "OnPasswordRevealedChanged"));
    }

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the visibility of the Header property. If Header and Header
//      Template are not set, it should collapse the property.
//
//---------------------------------------------------------------------------
_Check_return_
HRESULT
PasswordBox::UpdateHeaderPresenterVisibility()
{
    ctl::ComPtr<xaml::IDataTemplate> spHeaderTemplate;
    ctl::ComPtr<IInspectable> spHeader;

    IFC_RETURN(get_HeaderTemplate(&spHeaderTemplate));
    IFC_RETURN(get_Header(&spHeader));

    IFC_RETURN(ConditionallyGetTemplatePartAndUpdateVisibility(
        XSTRING_PTR_EPHEMERAL(L"HeaderContentPresenter"),
        (spHeader || spHeaderTemplate),
        m_tpHeaderPresenter));

    IFC_RETURN(ConditionallyGetTemplatePartAndUpdateVisibility(
        XSTRING_PTR_EPHEMERAL(L"RequiredHeaderPresenter"),
        (spHeader || spHeaderTemplate) && IsValueRequired(this),
        m_requiredHeaderPresenter));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Updates PlaceholderText visibility whenever text is updated
//
//---------------------------------------------------------------------------
_Check_return_
HRESULT
PasswordBox::UpdatePlaceholderTextPresenterVisibility(_In_ bool isEnabled)
{
    if (m_tpPlaceholderTextPresenter.Get())
    {
        IFC_RETURN(TextBoxPlaceholderTextHelper::UpdatePlaceholderTextPresenterVisibility(
            this, m_tpPlaceholderTextPresenter.Get(), isEnabled));
    }
    return S_OK;
}

_Check_return_ HRESULT PasswordBox::get_InputScopeImpl(_Out_ xaml_input::IInputScope** ppValue)
{
    return(GetValueByKnownIndex(KnownPropertyIndex::PasswordBox_InputScope, ppValue));
}

_Check_return_ HRESULT PasswordBox::put_InputScopeImpl(_In_ xaml_input::IInputScope* pValue)
{
    ctl::ComPtr<wfc::IVector<xaml_input::InputScopeName*>> spInputScopeNames;
    ctl::ComPtr<xaml_input::IInputScopeName> spInputScopeName;

    IFC_RETURN(pValue->get_Names(&spInputScopeNames));
    UINT cInputScopes = 0;
    IFC_RETURN(spInputScopeNames->get_Size(&cInputScopes));
    if (cInputScopes > 0)
    {
        IFC_RETURN(spInputScopeNames->GetAt(0, &spInputScopeName));
        xaml_input::InputScopeNameValue nameValue;
        IFC_RETURN(spInputScopeName->get_NameValue(&nameValue));
        if ((nameValue != xaml_input::InputScopeNameValue::InputScopeNameValue_NumericPin)
            && (nameValue != xaml_input::InputScopeNameValue::InputScopeNameValue_Password))
        {
            // inputscope for password box has to be Password or NumericPin
            IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_INVALID_PASSWORDBOX_INPUTSCOPE_VALUE));
        }
    }

    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::PasswordBox_InputScope, pValue));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Handles the custom property changed event and calls OnPropertyChanged2
//      Methods.
//
//---------------------------------------------------------------------------
_Check_return_
HRESULT
PasswordBox::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(PasswordBoxGenerated::OnPropertyChanged2(args));

    const KnownPropertyIndex index = args.m_pDP->GetIndex();
    bool setAcceleratorKey = false;
    switch (index)
    {
        case KnownPropertyIndex::PasswordBox_IsPasswordRevealButtonEnabled:
        {
            BOOLEAN isPasswordRevealButtonEnabled = FALSE;
            IFC_RETURN(PasswordBoxGenerated::get_IsPasswordRevealButtonEnabled(&isPasswordRevealButtonEnabled));
            setAcceleratorKey = !!isPasswordRevealButtonEnabled;
            break;
        }
        case KnownPropertyIndex::PasswordBox_PasswordRevealMode:
        {
            xaml_controls::PasswordRevealMode revealMode = xaml_controls::PasswordRevealMode::PasswordRevealMode_Hidden;
            IFC_RETURN(PasswordBoxGenerated::get_PasswordRevealMode(&revealMode));
            setAcceleratorKey = revealMode != xaml_controls::PasswordRevealMode::PasswordRevealMode_Hidden;
            break;
        }
        case KnownPropertyIndex::PasswordBox_Header:
        case KnownPropertyIndex::PasswordBox_HeaderTemplate:
        {
            // We will ignore property changes during initialization and take care of them when we have a template.
            if (!m_isInitializing)
            {
                IFC_RETURN(UpdateHeaderPresenterVisibility());
                static_cast<CPasswordBox*>(GetHandle())->InvalidateView();
                break;
            }
        }
        case KnownPropertyIndex::PasswordBox_PlaceholderText:
        {
            //UpdatePlaceholder visibility here
            IFC_RETURN(UpdatePlaceholderTextPresenterVisibility(
                TextBoxPlaceholderTextHelper::ShouldMakePlaceholderTextVisible(
                    m_tpPlaceholderTextPresenter.Get(), this)));
            break;
        }
        case KnownPropertyIndex::PasswordBox_Password:
        {
            if (!m_isInitializing)
            {
                wrl_wrappers::HString password;
                IFC_RETURN(get_Password(password.GetAddressOf()));
                IFC_RETURN(InvokeValidationCommand(this, password.Get()));
            }
            break;
        }
    }

    // Update AutomationProperties::AcceleratorKey property for PasswordBox
    // The 'ALT F8' shortcut is only enabled when PasswordRevealMode is not 'Hidden' or RevealButton is not disabled.
    // (IsRevealButtonEnabled property has been superceded by PasswordRevealMode, so these are mutually exclusive)
    if (index == KnownPropertyIndex::PasswordBox_IsPasswordRevealButtonEnabled
        || index == KnownPropertyIndex::PasswordBox_PasswordRevealMode)
    {
        wrl_wrappers::HString currentAcceleratorKey;
        IFC_RETURN(AutomationProperties::GetAcceleratorKeyStatic(this, currentAcceleratorKey.ReleaseAndGetAddressOf()));

        wrl_wrappers::HString automationAcceleratorKey;
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_PASSWORDBOX_REVEAL_ACCELERATORKEY, automationAcceleratorKey.ReleaseAndGetAddressOf()));

        //Do not overwrite an existing accelerator key
        if (!currentAcceleratorKey.Get() && setAcceleratorKey)
        {
            IFC_RETURN(DirectUI::AutomationProperties::SetAcceleratorKeyStatic(this, automationAcceleratorKey.Get()));
        }
        else if (!setAcceleratorKey && currentAcceleratorKey == automationAcceleratorKey)
        {
            //If the AcceleratorKey is currently set to a password reveal, remove it.
            IFC_RETURN(DirectUI::AutomationProperties::SetAcceleratorKeyStatic(this, nullptr));
        }
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Checks whether event belongs to RichEdit area
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT PasswordBox::ValidateEvent(_In_ xaml::IRoutedEventArgs* pEventArgs, _Out_ bool* pIsValidEvent)
{
    *pIsValidEvent = true;

    ctl::ComPtr<IInspectable> spOriginalSource;
    ctl::ComPtr<IFrameworkElement> spFrameworkElement;

    IFC_RETURN(pEventArgs->get_OriginalSource(&spOriginalSource));
    spFrameworkElement = spOriginalSource.AsOrNull<IFrameworkElement>();

    if (spFrameworkElement)
    {
        BOOLEAN bSourceIsRevealButton = FALSE;
        if (m_tpRevealButton)
        {
            IFC_RETURN(m_tpRevealButton.Cast<ButtonBase>()->IsAncestorOf(static_cast<FrameworkElement *>(spFrameworkElement.Get()), &bSourceIsRevealButton));
        }

        // Disallow event on reveal button
        *pIsValidEvent = !bSourceIsRevealButton;
    }

    return S_OK;
}

_Check_return_ HRESULT PasswordBox::SetRevealButtonPressed(bool userPressed)
{
    m_revealButtonPressed = userPressed;

    // Resetting reveal button pressed flag will also reset the toggle button to unchecked state
    if (!userPressed)
    {
        IFC_RETURN(m_tpRevealButton.Cast<ToggleButton>()->put_IsChecked(FALSE));
    }

    return S_OK;
}

_Check_return_ HRESULT PasswordBox::PasteFromClipboardImpl()
{
    IFC_RETURN(static_cast<CPasswordBox*>(GetHandle())->Paste());
    return S_OK;
}

// static
_Check_return_ HRESULT PasswordBox::OnPasswordChangingHandler(_In_ CPasswordBox* const nativePasswordBox, _In_ bool passwordChanged)
{
    ctl::ComPtr<DependencyObject> peer;
    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(nativePasswordBox, &peer));

    if (!peer)
    {
        // There is no need to fire the PasswordBoxChanging event if there is no peer, since no one is listening
        return S_FALSE;
    }

    ctl::ComPtr<IPasswordBox> passwordBox;
    IFC_RETURN(peer.As(&passwordBox));
    IFCPTR_RETURN(passwordBox);

    ctl::ComPtr<PasswordBox> peerAsPasswordBox = passwordBox.Cast<PasswordBox>();

    if (!peerAsPasswordBox->m_passwordChangingEventArgs)
    {
        IFC_RETURN(ctl::make(&(peerAsPasswordBox->m_passwordChangingEventArgs)));
    }

    peerAsPasswordBox->m_passwordChangingEventArgs->put_IsContentChanging(passwordChanged);

    PasswordBox::PasswordChangingEventSourceType* eventSource = nullptr;
    IFC_RETURN(passwordBox.Cast<PasswordBox>()->GetPasswordChangingEventSourceNoRef(&eventSource));
    IFC_RETURN(eventSource->Raise(passwordBox.Get(), peerAsPasswordBox->m_passwordChangingEventArgs.Get()));

    return S_OK;
}

_Check_return_ HRESULT PasswordBox::OnContextMenuOpeningHandler(_In_ CDependencyObject* const pNativePasswordBox, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop, _Out_ bool& handled)
{
    return (TextControlHelper::OnContextMenuOpeningHandler<IPasswordBox, PasswordBox>(pNativePasswordBox, cursorLeft, cursorTop, handled));
}

_Check_return_ HRESULT PasswordBox::QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject *pNativePasswordBox)
{
    return TextControlHelper::QueueUpdateSelectionFlyoutVisibility<PasswordBox>(pNativePasswordBox);
}

_Check_return_ HRESULT PasswordBox::UpdateSelectionFlyoutVisibility()
{
    return TextControlHelper::UpdateSelectionFlyoutVisibility<CPasswordBox>(this);
}
