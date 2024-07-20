// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ToggleSwitch.g.h"
#include "ToggleSwitchKeyProcess.h"

namespace DirectUI
{
    class IsEnabledChangedEventArgs;

    PARTIAL_CLASS(ToggleSwitch)
    {
        // Grant friend access to the KeyPress:ToggleSwitch class so it can access
        // the HandlesKey and Toggle methods.
        friend class KeyPress::ToggleSwitch;
    public:
        IFACEMETHOD(OnApplyTemplate)() override;

        _Check_return_ HRESULT OnHeaderChangedImpl(
            _In_ IInspectable *pOldContent,
            _In_ IInspectable *pNewContent);

        _Check_return_ HRESULT OnToggledImpl();

        _Check_return_ HRESULT OnOffContentChangedImpl(
            _In_ IInspectable *pOldContent,
            _In_ IInspectable *pNewContent);

        _Check_return_ HRESULT OnOnContentChangedImpl(
            _In_ IInspectable *pOldContent,
            _In_ IInspectable *pNewContent);

        _Check_return_ HRESULT AutomationToggleSwitchOnToggle();

        _Check_return_ HRESULT AutomationGetClickablePoint(
            _Out_ wf::Point* result);

    protected:
        ToggleSwitch();

        ~ToggleSwitch() override;

        _Check_return_ HRESULT PrepareState() override;

        _Check_return_ HRESULT
        ChangeVisualState(
            _In_ bool useTransitions) override;

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        IFACEMETHOD(OnGotFocus)(
            _In_ xaml::IRoutedEventArgs *pArgs) override;

        IFACEMETHOD(OnLostFocus)(
            _In_ xaml::IRoutedEventArgs *pArgs) override;

        IFACEMETHOD(OnPointerEntered)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnPointerExited)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnPointerCaptureLost)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnKeyDown)(
            _In_ xaml_input::IKeyRoutedEventArgs *pArgs) override;

        IFACEMETHOD(OnKeyUp)(
            _In_ xaml_input::IKeyRoutedEventArgs *pArgs) override;

        IFACEMETHOD(OnCreateAutomationPeer)(
            _Outptr_ xaml_automation_peers::IAutomationPeer **ppAutomationPeer);

        // IsEnabled property changed handler.
        _Check_return_ HRESULT OnIsEnabledChanged(
            _In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

        // Update the visual states when the Visibility property is changed.
        _Check_return_ HRESULT OnVisibilityChanged() override;

        // Gives the default values for our properties.
        _Check_return_ HRESULT GetDefaultValue2(
            _In_ const CDependencyProperty* pDP,
            _Out_ CValue* pValue) override;

    private:
        _Check_return_ HRESULT
        GetTranslations();

        _Check_return_ HRESULT
        SetTranslations();

        _Check_return_ HRESULT
        ClearTranslations();

        _Check_return_ HRESULT
        Toggle();

        _Check_return_ HRESULT
        MoveDelta(
            _In_ const DOUBLE translationDelta);

        _Check_return_ HRESULT
        MoveCompleted(
            _In_ const BOOLEAN wasMoved);

        _Check_return_ HRESULT
        DragStartedHandler(
            _In_ IInspectable *pSender,
            _In_ xaml_primitives::IDragStartedEventArgs *pArgs);

        _Check_return_ HRESULT
        DragDeltaHandler(
            _In_ IInspectable *pSender,
            _In_ xaml_primitives::IDragDeltaEventArgs *pArgs);

        _Check_return_ HRESULT
        DragCompletedHandler(
            _In_ IInspectable *pSender,
            _In_ xaml_primitives::IDragCompletedEventArgs *pArgs);

        _Check_return_ HRESULT
        SizeChangedHandler(
            _In_ IInspectable *pSender,
            _In_ xaml::ISizeChangedEventArgs *pArgs);

        _Check_return_ HRESULT
        TapHandler(
            _In_ IInspectable *pSender,
            _In_ xaml_input::ITappedRoutedEventArgs *pArgs);

        _Check_return_ HRESULT
        FocusChanged();

        // Whether the given key may cause the ToggleSwitch to toggle.
        BOOLEAN HandlesKey(
           _In_ wsy::VirtualKey key);

        _Check_return_ HRESULT UpdateHeaderPresenterVisibility();

    private:
        BOOLEAN m_isDragging;
        
        BOOLEAN m_wasDragged;

        BOOLEAN m_isPointerOver;

        // The translations for the curtain and knob template parts.

        DOUBLE m_knobTranslation;
        
        DOUBLE m_minKnobTranslation;
        
        DOUBLE m_maxKnobTranslation;

        DOUBLE m_curtainTranslation;
        
        DOUBLE m_minCurtainTranslation;
        
        DOUBLE m_maxCurtainTranslation;

        BOOLEAN m_handledKeyDown;

        // The template parts.

        TrackerPtr<xaml::IUIElement> m_tpCurtainClip;

        TrackerPtr<xaml::IFrameworkElement> m_tpKnob;
        
        TrackerPtr<xaml::IFrameworkElement> m_tpKnobBounds;
        
        TrackerPtr<xaml::IFrameworkElement> m_tpCurtainBounds;
        
        TrackerPtr<xaml_primitives::IThumb> m_tpThumb;

        TrackerPtr<xaml::IUIElement> m_tpHeaderPresenter;

        // The translate transforms from template parts.

        ctl::ComPtr<xaml_media::ITranslateTransform> m_spKnobTransform;

        ctl::ComPtr<xaml_media::ITranslateTransform> m_spCurtainTransform;

        EventRegistrationToken m_dragStarted;
        
        EventRegistrationToken m_dragDelta;
        
        EventRegistrationToken m_dragCompleted;
        
        EventRegistrationToken m_tap;
        
        EventRegistrationToken m_knobSizeChanged;
        
        EventRegistrationToken m_knobBoundsSizeChanged;

    };
}
