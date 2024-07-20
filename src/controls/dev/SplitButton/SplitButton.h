// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "SplitButton.g.h"
#include "SplitButton.properties.h"

class SplitButton :
    public ReferenceTracker<SplitButton, winrt::implementation::SplitButtonT>,
    public SplitButtonProperties
{

public:
    SplitButton();

    // IFrameworkElement
    void OnApplyTemplate();

    // IUIElement
    virtual winrt::AutomationPeer OnCreateAutomationPeer();

    // Internal
    bool IsFlyoutOpen() { return m_isFlyoutOpen; };
    void OpenFlyout();
    void CloseFlyout();
    virtual bool InternalIsChecked() { return false; }

    void UpdateVisualStates(bool useTransitions = true);

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void Invoke();

protected:
    virtual void OnClickPrimary(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);

    bool m_hasLoaded{ false };

private:
    void ExecuteCommand();
    void RegisterFlyoutEvents();
    void UnregisterEvents();

    void OnVisualPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);

    // Internal event handlers
    void OnClickSecondary(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);

    void OnFlyoutChanged();
    void OnFlyoutOpened(const winrt::IInspectable& sender, const winrt::IInspectable& args);
    void OnFlyoutClosed(const winrt::IInspectable& sender, const winrt::IInspectable& args);
    void OnFlyoutPlacementChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);

    void OnPointerEvent(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnSplitButtonKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args);
    void OnSplitButtonKeyUp(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args);

    tracker_ref<winrt::Button> m_primaryButton{ this };
    tracker_ref<winrt::Button> m_secondaryButton{ this };

    bool m_isFlyoutOpen{ false };
    winrt::PointerDeviceType m_lastPointerDeviceType{ winrt::PointerDeviceType::Mouse };
    bool m_isKeyDown{ false };

    winrt::UIElement::KeyDown_revoker m_keyDownRevoker{};
    winrt::UIElement::KeyUp_revoker m_keyUpRevoker{};

    winrt::ButtonBase::Click_revoker m_clickPrimaryRevoker{};
    PropertyChanged_revoker m_pressedPrimaryRevoker{};
    PropertyChanged_revoker m_pointerOverPrimaryRevoker{};

    winrt::UIElement::PointerEntered_revoker m_pointerEnteredPrimaryRevoker{};
    winrt::UIElement::PointerExited_revoker m_pointerExitedPrimaryRevoker{};
    winrt::UIElement::PointerPressed_revoker m_pointerPressedPrimaryRevoker{};
    winrt::UIElement::PointerReleased_revoker m_pointerReleasedPrimaryRevoker{};
    winrt::UIElement::PointerCanceled_revoker m_pointerCanceledPrimaryRevoker{};
    winrt::UIElement::PointerCaptureLost_revoker m_pointerCaptureLostPrimaryRevoker{};

    winrt::ButtonBase::Click_revoker m_clickSecondaryRevoker{};
    PropertyChanged_revoker m_pressedSecondaryRevoker{};
    PropertyChanged_revoker m_pointerOverSecondaryRevoker{};

    winrt::UIElement::PointerEntered_revoker m_pointerEnteredSecondaryRevoker{};
    winrt::UIElement::PointerExited_revoker m_pointerExitedSecondaryRevoker{};
    winrt::UIElement::PointerPressed_revoker m_pointerPressedSecondaryRevoker{};
    winrt::UIElement::PointerReleased_revoker m_pointerReleasedSecondaryRevoker{};
    winrt::UIElement::PointerCanceled_revoker m_pointerCanceledSecondaryRevoker{};
    winrt::UIElement::PointerCaptureLost_revoker m_pointerCaptureLostSecondaryRevoker{};

    winrt::FlyoutBase::Opened_revoker m_flyoutOpenedRevoker{};
    winrt::FlyoutBase::Closed_revoker m_flyoutClosedRevoker{};
    PropertyChanged_revoker m_flyoutPlacementChangedRevoker{};
};
