// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Thumb.g.h"
#include "ToggleSwitch.g.h"
#include "ToggleSwitchAutomationPeer.g.h"
#include "ToggleSwitchTemplateSettings.g.h"
#include "KeyRoutedEventArgs.g.h"
#include "RectangleGeometry.g.h"
#include "TranslateTransform.g.h"
#include "localizedResource.h"
#include "ElementSoundPlayerService_Partial.h"

#undef min
#undef max

using namespace DirectUI;
using namespace DirectUISynonyms;

ToggleSwitch::ToggleSwitch() :
    m_isPointerOver(FALSE),
    m_isDragging(FALSE),
    m_wasDragged(FALSE),

    m_curtainTranslation(0),
    m_knobTranslation(0),

    m_maxCurtainTranslation(0),
    m_maxKnobTranslation(0),

    m_minCurtainTranslation(0),
    m_minKnobTranslation(0),

    m_handledKeyDown(FALSE)
{
}

ToggleSwitch::~ToggleSwitch()
{
    m_tpKnob.Clear();
    m_tpThumb.Clear();
    m_tpKnobBounds.Clear();
    m_tpCurtainClip.Clear();
    m_tpCurtainBounds.Clear();
}

_Check_return_ HRESULT
ToggleSwitch::ChangeVisualState(_In_ bool useTransitions)
{
    BOOLEAN isOn = FALSE;
    BOOLEAN isEnabled = FALSE;
    BOOLEAN ignored = FALSE;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;

    IFC_RETURN(ToggleSwitchGenerated::ChangeVisualState(useTransitions));

    IFC_RETURN(get_IsEnabled(&isEnabled));
    IFC_RETURN(get_FocusState(&focusState));

    if (!isEnabled)
    {
        IFC_RETURN(GoToState(useTransitions, L"Disabled", &ignored));
    }
    else if (m_isDragging)
    {
        IFC_RETURN(GoToState(useTransitions, L"Pressed", &ignored));
    }
    else if (m_isPointerOver)
    {
        IFC_RETURN(GoToState(useTransitions, L"PointerOver", &ignored));
    }
    else
    {
        IFC_RETURN(GoToState(useTransitions, L"Normal", &ignored));
    }

    if (xaml::FocusState_Unfocused != focusState && isEnabled)
    {
        if (xaml::FocusState_Pointer == focusState)
        {
            IFC_RETURN(GoToState(useTransitions, L"PointerFocused", &ignored));
        }
        else
        {
            IFC_RETURN(GoToState(useTransitions, L"Focused", &ignored));
        }
    }
    else
    {
        IFC_RETURN(GoToState(useTransitions, L"Unfocused", &ignored));
    }


    if (m_isDragging)
    {
        IFC_RETURN(GoToState(useTransitions, L"Dragging", &ignored));
    }
    else
    {
        IFC_RETURN(get_IsOn(&isOn));
        IFC_RETURN(GoToState(useTransitions, isOn ? L"On" : L"Off", &ignored));
        IFC_RETURN(GoToState(useTransitions, isOn ? L"OnContent" : L"OffContent", &ignored));
    }

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
    // HeaderStates VisualStateGroup.
    xaml_controls::ControlHeaderPlacement headerPlacement = xaml_controls::ControlHeaderPlacement_Top;
    IFC_RETURN(get_HeaderPlacement(&headerPlacement));

    switch (headerPlacement)
    {
        case DirectUI::ControlHeaderPlacement::Top:
            IFC_RETURN(GoToState(useTransitions, L"TopHeader", &ignored));
            break;

        case DirectUI::ControlHeaderPlacement::Left:
            IFC_RETURN(GoToState(useTransitions, L"LeftHeader", &ignored));
            break;
    }
#endif

    return S_OK;
}

IFACEMETHODIMP
ToggleSwitch::OnApplyTemplate()
{
    HRESULT hr = S_OK;

    ctl::ComPtr<xaml::IDependencyObject> spCurtainIDependencyObject;
    ctl::ComPtr<xaml::IDependencyObject> spCurtainBoundsIDependencyObject;
    ctl::ComPtr<xaml::IDependencyObject> spCurtainClipIDependencyObject;
    ctl::ComPtr<xaml::IDependencyObject> spKnobIDependencyObject;
    ctl::ComPtr<xaml::IDependencyObject> spKnobBoundsIDependencyObject;
    ctl::ComPtr<xaml::IDependencyObject> spThumbIDependencyObject;
    ctl::ComPtr<xaml::IUIElement> spThumbIUIElement;
    ctl::ComPtr<xaml::IUIElement> spCurtainIUIElement;
    ctl::ComPtr<xaml::IUIElement> spKnobIUIElement;

    if (m_tpThumb)
    {
        ctl::ComPtr<xaml::IUIElement> spOldThumb;
        IFC(m_tpThumb->remove_DragStarted(m_dragStarted));
        IFC(m_tpThumb->remove_DragDelta(m_dragDelta));
        IFC(m_tpThumb->remove_DragCompleted(m_dragCompleted));

        if (SUCCEEDED(ctl::do_query_interface(spOldThumb, m_tpThumb.Get())))
        {
            IFC(spOldThumb->remove_Tapped(m_tap));
        }

    }

    if (m_tpKnob)
    {
        IFC(m_tpKnob->remove_SizeChanged(m_knobSizeChanged));
    }

    if (m_tpKnobBounds)
    {
        IFC(m_tpKnobBounds->remove_SizeChanged(m_knobBoundsSizeChanged));
    }

    m_tpCurtainBounds.Clear();
    m_tpCurtainClip.Clear();
    m_spCurtainTransform = nullptr;
    m_tpKnob.Clear();
    m_tpKnobBounds.Clear();
    m_spKnobTransform = nullptr;
    m_tpThumb.Clear();
    m_tpHeaderPresenter.Clear();

    IFC(ToggleSwitchGenerated::OnApplyTemplate());

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"SwitchCurtain")).Get(), &spCurtainIDependencyObject));
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"SwitchCurtainBounds")).Get(), &spCurtainBoundsIDependencyObject));
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"SwitchCurtainClip")).Get(), &spCurtainClipIDependencyObject));
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"SwitchKnob")).Get(), &spKnobIDependencyObject));
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"SwitchKnobBounds")).Get(), &spKnobBoundsIDependencyObject));
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"SwitchThumb")).Get(), &spThumbIDependencyObject));

    SetPtrValueWithQIOrNull(m_tpCurtainBounds, spCurtainBoundsIDependencyObject.Get());
    SetPtrValueWithQIOrNull(m_tpCurtainClip, spCurtainClipIDependencyObject.Get());
    SetPtrValueWithQIOrNull(m_tpKnob, spKnobIDependencyObject.Get());
    SetPtrValueWithQIOrNull(m_tpKnobBounds, spKnobBoundsIDependencyObject.Get());
    SetPtrValueWithQIOrNull(m_tpThumb, spThumbIDependencyObject.Get());

    spThumbIUIElement = m_tpThumb.AsOrNull<xaml::IUIElement>();

    spCurtainIUIElement = spCurtainIDependencyObject.AsOrNull<xaml::IUIElement>();
    if (spCurtainIUIElement)
    {
        ctl::ComPtr<xaml_media::ITransform> spCurtainRenderTransform;

        IFC(spCurtainIUIElement->get_RenderTransform(&spCurtainRenderTransform));
        m_spCurtainTransform = spCurtainRenderTransform.AsOrNull<ITranslateTransform>();
    }

    spKnobIUIElement = spKnobIDependencyObject.AsOrNull<xaml::IUIElement>();
    if (spKnobIUIElement)
    {
        ctl::ComPtr<xaml_media::ITransform> spKnobRenderTransform;

        IFC(spKnobIUIElement->get_RenderTransform(&spKnobRenderTransform));
        m_spKnobTransform = spKnobRenderTransform.AsOrNull<ITranslateTransform>();
    }

    if (spThumbIUIElement)
    {
        ctl::ComPtr<xaml_primitives::IDragStartedEventHandler> spIDragStartedEventHandler;
        ctl::ComPtr<xaml_primitives::IDragDeltaEventHandler> spIDragDeltaEventHandler;
        ctl::ComPtr<xaml_primitives::IDragCompletedEventHandler> spIDragCompletedEventHandler;
        ctl::ComPtr<xaml_input::ITappedEventHandler> spITappedEventHandler;

        spIDragStartedEventHandler.Attach(
            new ClassMemberEventHandler<
                    ToggleSwitch,
                    xaml_controls::IToggleSwitch,
                    xaml_primitives::IDragStartedEventHandler,
                    IInspectable,
                    xaml_primitives::IDragStartedEventArgs>(this, &ToggleSwitch::DragStartedHandler));

        spIDragDeltaEventHandler.Attach(
            new ClassMemberEventHandler<
                    ToggleSwitch,
                    xaml_controls::IToggleSwitch,
                    xaml_primitives::IDragDeltaEventHandler,
                    IInspectable,
                    xaml_primitives::IDragDeltaEventArgs>(this, &ToggleSwitch::DragDeltaHandler));

        spIDragCompletedEventHandler.Attach(
            new ClassMemberEventHandler<
                    ToggleSwitch,
                    xaml_controls::IToggleSwitch,
                    xaml_primitives::IDragCompletedEventHandler,
                    IInspectable,
                    xaml_primitives::IDragCompletedEventArgs>(this, &ToggleSwitch::DragCompletedHandler));

        spITappedEventHandler.Attach(
            new ClassMemberEventHandler<
                ToggleSwitch,
                xaml_controls::IToggleSwitch,
                xaml_input::ITappedEventHandler,
                IInspectable,
                xaml_input::ITappedRoutedEventArgs>(this, &ToggleSwitch::TapHandler));

        IFC(m_tpThumb->add_DragStarted(spIDragStartedEventHandler.Get(), &m_dragStarted));
        IFC(m_tpThumb->add_DragDelta(spIDragDeltaEventHandler.Get(), &m_dragDelta));
        IFC(m_tpThumb->add_DragCompleted(spIDragCompletedEventHandler.Get(), &m_dragCompleted));

        IFC(spThumbIUIElement->add_Tapped(spITappedEventHandler.Get(), &m_tap));
    }

    if (m_tpKnob || m_tpKnobBounds)
    {
        ctl::ComPtr<xaml::ISizeChangedEventHandler> spISizeChangedEventHandler;

        spISizeChangedEventHandler.Attach(
            new ClassMemberEventHandler<
                ToggleSwitch,
                xaml_controls::IToggleSwitch,
                xaml::ISizeChangedEventHandler,
                IInspectable,
                xaml::ISizeChangedEventArgs>(this, &ToggleSwitch::SizeChangedHandler));

        if (m_tpKnob)
        {
            IFC(m_tpKnob->add_SizeChanged(spISizeChangedEventHandler.Get(), &m_knobSizeChanged));
        }

        if (m_tpKnobBounds)
        {
            IFC(m_tpKnobBounds->add_SizeChanged(spISizeChangedEventHandler.Get(), &m_knobBoundsSizeChanged));
        }
    }

    IFC(UpdateHeaderPresenterVisibility());

    IFC(UpdateVisualState(FALSE));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ToggleSwitch::PrepareState()
{
    IFC_RETURN(ToggleSwitchGenerated::PrepareState());

    ctl::ComPtr<ToggleSwitchTemplateSettings> spTemplateSettings;
    IFC_RETURN(ctl::make<ToggleSwitchTemplateSettings>(&spTemplateSettings));
    IFC_RETURN(put_TemplateSettings(spTemplateSettings.Get()));

    return S_OK;
}

// Gives the default values for our properties.
_Check_return_ HRESULT
ToggleSwitch::GetDefaultValue2(
_In_ const CDependencyProperty* pDP,
_Out_ CValue* pValue)
{
    DXamlCore* pCore = DXamlCore::GetCurrent();

    switch (pDP->GetIndex())
    {
    case KnownPropertyIndex::ToggleSwitch_OnContent:
    {
        wrl_wrappers::HString strOnHString;
        IFC_RETURN(pCore->GetLocalizedResourceString(TEXT_TOGGLESWITCH_ON, strOnHString.GetAddressOf()));

        ctl::ComPtr<IInspectable> spOnIInspectable;
        IFC_RETURN(IValueBoxer::BoxValue(&spOnIInspectable, strOnHString.Get()));

        pValue->SetIInspectableAddRef(spOnIInspectable.Get());
        break;
    }
    case KnownPropertyIndex::ToggleSwitch_OffContent:
    {
        wrl_wrappers::HString strOffHString;
        IFC_RETURN(pCore->GetLocalizedResourceString(TEXT_TOGGLESWITCH_OFF, strOffHString.GetAddressOf()));

        ctl::ComPtr<IInspectable> spOffIInspectable;
        IFC_RETURN(IValueBoxer::BoxValue(&spOffIInspectable, strOffHString.Get()));

        pValue->SetIInspectableAddRef(spOffIInspectable.Get());
        break;
    }
    default:
        IFC_RETURN(ToggleSwitchGenerated::GetDefaultValue2(pDP, pValue));
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT
ToggleSwitch::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    BOOLEAN hasAutomationListener = FALSE;

    IFC_RETURN(ToggleSwitchGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::ToggleSwitch_IsOn:
            IFC_RETURN(OnToggledProtected());

            IFC_RETURN(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_PropertyChanged, &hasAutomationListener));

            if (hasAutomationListener)
            {
                ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;

                IFC_RETURN(GetOrCreateAutomationPeer(&spAutomationPeer));
                if (spAutomationPeer)
                {
                    ctl::ComPtr<xaml_automation_peers::IToggleSwitchAutomationPeer>
                    spToggleSwitchAutomationPeer(spAutomationPeer.AsOrNull<xaml_automation_peers::IToggleSwitchAutomationPeer>());

                    if (spToggleSwitchAutomationPeer)
                    {
                        ctl::ComPtr<IInspectable> spOldValue, spNewValue;
                        IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pOldValue, args.m_pDP->GetPropertyType(), &spOldValue));
                        IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spNewValue));

                        IFC_RETURN(spToggleSwitchAutomationPeer.Cast<ToggleSwitchAutomationPeer>()->RaiseToggleStatePropertyChangedEvent(spOldValue.Get(), spNewValue.Get()));
                    }
                }
            }
            break;

        case KnownPropertyIndex::ToggleSwitch_Header:
            {
                ctl::ComPtr<IInspectable> spOldValue, spNewValue;
                IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pOldValue, args.m_pDP->GetPropertyType(), &spOldValue));
                IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spNewValue));
                IFC_RETURN(UpdateHeaderPresenterVisibility());
                IFC_RETURN(OnHeaderChangedProtected(spOldValue.Get(), spNewValue.Get()));
            }
            break;

        case KnownPropertyIndex::ToggleSwitch_HeaderTemplate:
            IFC_RETURN(UpdateHeaderPresenterVisibility());
            break;

        case KnownPropertyIndex::ToggleSwitch_OffContent:
            {
                ctl::ComPtr<IInspectable> spOldValue, spNewValue;
                IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pOldValue, args.m_pDP->GetPropertyType(), &spOldValue));
                IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spNewValue));
                IFC_RETURN(OnOffContentChangedProtected(spOldValue.Get(), spNewValue.Get()));
            }
            break;

        case KnownPropertyIndex::ToggleSwitch_OnContent:
            {
                ctl::ComPtr<IInspectable> spOldValue, spNewValue;
                IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pOldValue, args.m_pDP->GetPropertyType(), &spOldValue));
                IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spNewValue));
                IFC_RETURN(OnOnContentChangedProtected(spOldValue.Get(), spNewValue.Get()));
            }
            break;

        case KnownPropertyIndex::UIElement_Visibility:
            IFC_RETURN(OnVisibilityChanged());
            break;

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
        case KnownPropertyIndex::ToggleSwitch_HeaderPlacement:
            IFC_RETURN(UpdateVisualState());
            break;
#endif
    }

    return S_OK;
}

_Check_return_ HRESULT
ToggleSwitch::GetTranslations()
{
    HRESULT hr = S_OK;

    if (m_spKnobTransform)
    {
        IFC(m_spKnobTransform->get_X(&m_knobTranslation));
    }

    if (m_spCurtainTransform)
    {
        IFC(m_spCurtainTransform->get_X(&m_curtainTranslation));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ToggleSwitch::SetTranslations()
{
    HRESULT hr = S_OK;
    DOUBLE translation = 0;
    ToggleSwitchTemplateSettings* pToggleSwitchTemplateSettingsNoRef = NULL;
    ctl::ComPtr<IToggleSwitchTemplateSettings> spIToggleSwitchTemplateSettings;

    IFC(get_TemplateSettings(&spIToggleSwitchTemplateSettings));

    if (spIToggleSwitchTemplateSettings)
    {
        pToggleSwitchTemplateSettingsNoRef = spIToggleSwitchTemplateSettings.Cast<ToggleSwitchTemplateSettings>();
    }

    if (m_spKnobTransform)
    {
        translation = std::min(m_knobTranslation, m_maxKnobTranslation);
        translation = std::max(translation, m_minKnobTranslation);

        IFC(m_spKnobTransform->put_X(translation));

        if (pToggleSwitchTemplateSettingsNoRef)
        {
            IFC(pToggleSwitchTemplateSettingsNoRef->put_KnobCurrentToOffOffset(translation - m_minKnobTranslation));
            IFC(pToggleSwitchTemplateSettingsNoRef->put_KnobCurrentToOnOffset(translation - m_maxKnobTranslation));
        }
    }

    if (m_spCurtainTransform)
    {
        translation = std::min(m_curtainTranslation, m_maxCurtainTranslation);
        translation = std::max(translation, m_minCurtainTranslation);

        IFC(m_spCurtainTransform->put_X(translation));

        if (pToggleSwitchTemplateSettingsNoRef)
        {
            IFC(pToggleSwitchTemplateSettingsNoRef->put_CurtainCurrentToOffOffset(translation - m_minCurtainTranslation));
            IFC(pToggleSwitchTemplateSettingsNoRef->put_CurtainCurrentToOnOffset(translation - m_maxCurtainTranslation));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ToggleSwitch::ClearTranslations()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spKnobIDependencyObject;
    ctl::ComPtr<xaml::IDependencyObject> spCurtainIDependencyObject;

    const CDependencyProperty* pDependencyPropertyInfo = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::TranslateTransform_X);

    if (m_spKnobTransform)
    {
        IFC(m_spKnobTransform.As(&spKnobIDependencyObject));
        IFC(spKnobIDependencyObject.Cast<DependencyObject>()->ClearValue(pDependencyPropertyInfo));
    }

    if (m_spCurtainTransform)
    {
        IFC(m_spCurtainTransform.As(&spCurtainIDependencyObject));
        IFC(spCurtainIDependencyObject.Cast<DependencyObject>()->ClearValue(pDependencyPropertyInfo));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ToggleSwitch::Toggle()
{
    BOOLEAN isOn = FALSE;

    IFC_RETURN(get_IsOn(&isOn));
    IFC_RETURN(put_IsOn(!isOn));

    // Request a play invoke sound
    IFC_RETURN(DirectUI::ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_Invoke, this));

    return S_OK;
}

_Check_return_ HRESULT
ToggleSwitch::AutomationToggleSwitchOnToggle()
{
    HRESULT hr = S_OK;

    IFC(Toggle());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ToggleSwitch::AutomationGetClickablePoint(
    _Out_ wf::Point* result)
{
    auto clickableElement =
        m_tpThumb ?
        static_cast<UIElement*>(m_tpThumb.Cast<Thumb>()) :
        static_cast<UIElement*>(this);

    XPOINTF point;
    IFC_RETURN(static_cast<CUIElement*>(clickableElement->GetHandle())->GetClickablePointRasterizedClient(&point));

    *result = { point.x, point.y };

    return S_OK;
}

IFACEMETHODIMP
ToggleSwitch::OnCreateAutomationPeer(
    _Outptr_ xaml_automation_peers::IAutomationPeer **ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IToggleSwitchAutomationPeer> spToggleSwitchAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IToggleSwitchAutomationPeerFactory> spToggleSwitchAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::ToggleSwitchAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spToggleSwitchAPFactory));

    IFC(spToggleSwitchAPFactory.Cast<ToggleSwitchAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spToggleSwitchAutomationPeer));

    IFC(spToggleSwitchAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ToggleSwitch::MoveDelta(_In_ const DOUBLE translationDelta)
{
    HRESULT hr = S_OK;

    m_curtainTranslation += translationDelta;
    m_knobTranslation += translationDelta;

    IFC(SetTranslations());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ToggleSwitch::MoveCompleted(_In_ const BOOLEAN wasMoved)
{
    HRESULT hr = S_OK;
    BOOLEAN wasToggled = FALSE;

    if (wasMoved)
    {
        DOUBLE halfOfTranslationRange = (m_maxKnobTranslation - m_minKnobTranslation) / 2;
        BOOLEAN isOn = FALSE;

        IFC(get_IsOn(&isOn));
        wasToggled = isOn ? m_knobTranslation <= halfOfTranslationRange : m_knobTranslation >= halfOfTranslationRange;
    }

    IFC(ClearTranslations());

    if (wasToggled)
    {
        IFC(Toggle());
    }
    else
    {
        IFC(UpdateVisualState(TRUE));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ToggleSwitch::OnToggledImpl()
{
    HRESULT hr = S_OK;
    ToggledEventSourceType* pToggledEventSource = nullptr;
    ctl::ComPtr<RoutedEventArgs> spRoutedEventArgs;

    IFC(ctl::make<RoutedEventArgs>(&spRoutedEventArgs));
    IFC(spRoutedEventArgs->put_OriginalSource(ctl::as_iinspectable(this)));

    IFC(GetToggledEventSourceNoRef(&pToggledEventSource));
    IFC(pToggledEventSource->Raise(ctl::as_iinspectable(this), spRoutedEventArgs.Get()));

    if (!m_isDragging)
    {
        IFC(UpdateVisualState(TRUE));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ToggleSwitch::OnHeaderChangedImpl(
    _In_ IInspectable* pOldContent,
    _In_ IInspectable* pNewContent)
{
    RRETURN(S_OK);
}

_Check_return_ HRESULT ToggleSwitch::OnOffContentChangedImpl(
    _In_ IInspectable* pOldContent,
    _In_ IInspectable* pNewContent)
{
    RRETURN(S_OK);
}

_Check_return_ HRESULT ToggleSwitch::OnOnContentChangedImpl(
    _In_ IInspectable* pOldContent,
    _In_ IInspectable* pNewContent)
{
     RRETURN(S_OK);
}

IFACEMETHODIMP
ToggleSwitch::OnGotFocus(_In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(ToggleSwitchGenerated::OnGotFocus(pArgs));

    IFC(FocusChanged());

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
ToggleSwitch::OnLostFocus(_In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(ToggleSwitchGenerated::OnLostFocus(pArgs));

    IFC(FocusChanged());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ToggleSwitch::FocusChanged()
{
    HRESULT hr = S_OK;

    IFC(UpdateVisualState(TRUE));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
ToggleSwitch::OnPointerEntered(_In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(ToggleSwitchGenerated::OnPointerEntered(pArgs));

    m_isPointerOver = TRUE;
    IFC(UpdateVisualState(TRUE));

Cleanup:
    RRETURN(hr);
}


IFACEMETHODIMP
ToggleSwitch::OnPointerExited(_In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(ToggleSwitchGenerated::OnPointerExited(pArgs));

    m_isPointerOver = FALSE;
    IFC(UpdateVisualState(TRUE));

Cleanup:
    RRETURN(hr);
}

// We need to add this event handler because in the "Vertical Pan" case, the pointer would most likely
// move out of the control but OnPointerExit() would not be invoked so the control might remain in PointerOver
// visual state. This handler rectifies this issue by clearing the PointerOver state.
IFACEMETHODIMP
ToggleSwitch::OnPointerCaptureLost(_In_ IPointerRoutedEventArgs* pArgs)
{
    IFC_RETURN(ToggleSwitchGenerated::OnPointerCaptureLost(pArgs));

    // We are checking to make sure dragging has finished before resetting the PointerOver state,
    // because in the "Vertical Pan" case, we get a call to DragCompletedHandler() before
    // OnPointerCaptureLost() unlike the case of "Tap"/"Horizontal Drag" where Thumb::OnPointerReleased()
    // invokes ReleasePointerCapture() so OnPointerCaptureLost() is called before DragCompletedHandler().
    if (!m_isDragging)
    {
        m_isPointerOver = FALSE;
    }
    IFC_RETURN(UpdateVisualState());

    return S_OK;
}

// We don't perform any action in OnKeyDown because we wait for the key to be
// released before performing a Toggle().  However, if we don't handle OnKeyDown,
// it bubbles up to the parent ScrollViewer and may cause scrolling, which is
// undesirable.  Therefore, we check to see if we will handle OnKeyUp for this key
// press, and if so, we set Handled=TRUE for OnKeyDown to stop bubbling this event.
IFACEMETHODIMP
ToggleSwitch::OnKeyDown(_In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;
    wsy::VirtualKey key = wsy::VirtualKey_None;

    IFCPTR(pArgs);
    IFC(ToggleSwitchGenerated::OnKeyDown(pArgs));

    IFC(pArgs->get_Handled(&isHandled));

    if (isHandled || m_isDragging)
    {
        goto Cleanup;
    }

    IFC(static_cast<KeyRoutedEventArgs*>(pArgs)->get_OriginalKey(&key));
    IFC(KeyPress::ToggleSwitch::KeyDown<ToggleSwitch>(key, this, &isHandled));

    IFC(pArgs->put_Handled(isHandled));
    m_handledKeyDown = isHandled;

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
ToggleSwitch::OnKeyUp(_In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;
    wsy::VirtualKey key = wsy::VirtualKey_None;

    IFCPTR(pArgs);
    IFC(ToggleSwitchGenerated::OnKeyUp(pArgs));

    IFC(static_cast<KeyRoutedEventArgs*>(pArgs)->get_OriginalKey(&key));
    IFC(pArgs->get_Handled(&isHandled));

    IFC(KeyPress::ToggleSwitch::KeyUp<ToggleSwitch>(key, this, &isHandled));
    IFC(pArgs->put_Handled(isHandled));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ToggleSwitch::DragStartedHandler(
    _In_ IInspectable* pSender,
    _In_ xaml_primitives::IDragStartedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isFocused = FALSE;

    m_isDragging = TRUE;
    m_wasDragged = FALSE;

    IFC(Focus(xaml::FocusState_Pointer, &isFocused));
    IFC(GetTranslations());
    IFC(UpdateVisualState(TRUE));
    IFC(SetTranslations());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ToggleSwitch::DragDeltaHandler(
    _In_ IInspectable* pSender,
    _In_ xaml_primitives::IDragDeltaEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    DOUBLE translation = 0;

    IFC(pArgs->get_HorizontalChange(&translation));

    // Allow Y movement, which prevents scrolling.
    if (translation != 0)
    {
        m_wasDragged = TRUE;
        IFC(MoveDelta(translation));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ToggleSwitch::DragCompletedHandler(
    _In_ IInspectable* pSender,
    _In_ xaml_primitives::IDragCompletedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isCanceled = FALSE;

    IFCPTR(pArgs);

    IFC(pArgs->get_Canceled(&isCanceled));

    if (isCanceled)
    {
        goto Cleanup;
    }

    m_isDragging = FALSE;

    IFC(MoveCompleted(m_wasDragged));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ToggleSwitch::TapHandler(
    _In_ IInspectable* pSender,
    _In_ xaml_input::ITappedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;

    IFCPTR(pArgs);

    IFC(pArgs->get_Handled(&isHandled));

    if (isHandled || m_isDragging)
    {
        goto Cleanup;
    }

    IFC(Toggle());

    IFC(pArgs->put_Handled(TRUE));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ToggleSwitch::SizeChangedHandler(
    _In_ IInspectable* pSender,
    _In_ xaml::ISizeChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    DOUBLE knobWidth = 0;
    DOUBLE knobBoundsWidth = 0;
    DOUBLE knobTranslation = 0;
    DOUBLE curtainBoundsWidth = 0;
    DOUBLE curtainTranslation = 0;
    DOUBLE curtainBoundsHeight = 0;
    wf::Rect clipRect = {};
    xaml::Thickness knobMarginThickness = {};
    BOOLEAN isOn = FALSE;

    ctl::ComPtr<IToggleSwitchTemplateSettings> spTemplateSettings;

    // Set the clip.
    if (m_tpCurtainBounds)
    {
        ctl::ComPtr<RectangleGeometry> spClipRectangleGeometry;

        IFC(ctl::make<RectangleGeometry>(&spClipRectangleGeometry));

        IFC(m_tpCurtainBounds->get_ActualWidth(&curtainBoundsWidth));

        if (m_tpCurtainClip)
        {
            IFC(m_tpCurtainBounds->get_ActualHeight(&curtainBoundsHeight));

            clipRect.X = clipRect.Y = 0;
            clipRect.Width = static_cast<FLOAT>(curtainBoundsWidth);
            clipRect.Height = static_cast<FLOAT>(curtainBoundsHeight);

            IFC(spClipRectangleGeometry->put_Rect(clipRect));
            IFC(m_tpCurtainClip->put_Clip(spClipRectangleGeometry.Get()));
        }
    }

    IFC(get_IsOn(&isOn));

    // Compute the knob translation bounds.
    if (m_tpKnob && m_tpKnobBounds && m_spKnobTransform)
    {
        IFC(m_spKnobTransform->get_X(&knobTranslation));
        IFC(m_tpKnobBounds->get_ActualWidth(&knobBoundsWidth));
        IFC(m_tpKnob->get_ActualWidth(&knobWidth));
        IFC(m_tpKnob->get_Margin(&knobMarginThickness));

        if (isOn)
        {
            m_maxKnobTranslation = knobTranslation;
            m_minKnobTranslation = m_maxKnobTranslation - knobBoundsWidth + knobWidth;
        }
        else
        {
            m_minKnobTranslation = knobTranslation;
            m_maxKnobTranslation = m_minKnobTranslation + knobBoundsWidth - knobWidth;
        }

        // Enable the negative margin effects used with the phone version.
        if (knobMarginThickness.Left < 0)
        {
            m_maxKnobTranslation -= knobMarginThickness.Left;
        }

        if (knobMarginThickness.Right < 0)
        {
            m_maxKnobTranslation -= knobMarginThickness.Right;
        }
    }

    // Compute the curtain translation bounds.
    if (m_tpCurtainBounds && m_spCurtainTransform)
    {
        IFC(m_spCurtainTransform->get_X(&curtainTranslation));

        if (isOn)
        {
            m_maxCurtainTranslation = curtainTranslation;
            m_minCurtainTranslation = m_maxCurtainTranslation - curtainBoundsWidth;
        }
        else
        {
            m_minCurtainTranslation = curtainTranslation;
            m_maxCurtainTranslation = m_minCurtainTranslation + curtainBoundsWidth;
        }
    }

    // flow these values into interested parties
    IFC(get_TemplateSettings(&spTemplateSettings));
    if (spTemplateSettings)
    {
        ToggleSwitchTemplateSettings* pTemplateSettingsConcreteNoRef = spTemplateSettings.Cast<ToggleSwitchTemplateSettings>();
        IFC(pTemplateSettingsConcreteNoRef->put_KnobOffToOnOffset(m_minKnobTranslation - m_maxKnobTranslation));
        IFC(pTemplateSettingsConcreteNoRef->put_KnobOnToOffOffset(m_maxKnobTranslation - m_minKnobTranslation));

        IFC(pTemplateSettingsConcreteNoRef->put_CurtainOffToOnOffset(m_minCurtainTranslation - m_maxCurtainTranslation));
        IFC(pTemplateSettingsConcreteNoRef->put_CurtainOnToOffOffset(m_maxCurtainTranslation - m_minCurtainTranslation));
    }


Cleanup:
    RRETURN(hr);
}

// Whether the given key may cause the ToggleSwitch to toggle.
BOOLEAN
ToggleSwitch::HandlesKey(
    _In_ wsy::VirtualKey key)
{
    return wsy::VirtualKey_Space == key ||
        wsy::VirtualKey_GamepadA == key;
}

// Called when the IsEnabled property changes.
_Check_return_ HRESULT ToggleSwitch::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsEnabled = FALSE;

    IFC(ToggleSwitchGenerated::OnIsEnabledChanged(pArgs));

    IFC(get_IsEnabled(&bIsEnabled));
    if (!bIsEnabled)
    {
        m_isDragging = FALSE;
        m_isPointerOver = FALSE;
    }
    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Update the visual states when the Visibility property is changed.
_Check_return_ HRESULT
ToggleSwitch::OnVisibilityChanged()
{
    HRESULT hr = S_OK;
    xaml::Visibility visibility = xaml::Visibility_Collapsed;

    IFC(get_Visibility(&visibility));
    if (xaml::Visibility_Visible != visibility)
    {
        m_isDragging = FALSE;
        m_isPointerOver = FALSE;
    }

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Updates the visibility of the Header ContentPresenter
_Check_return_ HRESULT ToggleSwitch::UpdateHeaderPresenterVisibility()
{
    ctl::ComPtr<xaml::IDataTemplate> spHeaderTemplate;
    ctl::ComPtr<IInspectable> spHeader;

    IFC_RETURN(get_HeaderTemplate(&spHeaderTemplate));
    IFC_RETURN(get_Header(&spHeader));

    IFC_RETURN(ConditionallyGetTemplatePartAndUpdateVisibility(
        XSTRING_PTR_EPHEMERAL(L"HeaderContentPresenter"),
        (spHeader || spHeaderTemplate),
        m_tpHeaderPresenter));

    return S_OK;
}
