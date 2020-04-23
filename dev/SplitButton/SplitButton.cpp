// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SplitButton.h"
#include "SplitButtonAutomationPeer.h"
#include "SplitButtonTestHelper.h"
#include "SplitButtonEventArgs.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

SplitButton::SplitButton()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_SplitButton);

    SetDefaultStyleKey(this);

    m_keyDownRevoker = KeyDown(winrt::auto_revoke, { this, &SplitButton::OnSplitButtonKeyDown });
    m_keyUpRevoker = KeyUp(winrt::auto_revoke, { this, &SplitButton::OnSplitButtonKeyUp });
}

void SplitButton::OnApplyTemplate()
{
    UnregisterEvents();

    winrt::IControlProtected controlProtected{ *this };
    m_primaryButton.set(GetTemplateChildT<winrt::Button>(L"PrimaryButton", controlProtected));
    m_secondaryButton.set(GetTemplateChildT<winrt::Button>(L"SecondaryButton", controlProtected));

    if (auto primaryButton = m_primaryButton.get())
    {
        m_clickPrimaryRevoker = primaryButton.Click(winrt::auto_revoke, { this, &SplitButton::OnClickPrimary });

        m_pressedPrimaryRevoker = RegisterPropertyChanged(primaryButton, winrt::ButtonBase::IsPressedProperty(), { this, &SplitButton::OnVisualPropertyChanged });
        m_pointerOverPrimaryRevoker = RegisterPropertyChanged(primaryButton, winrt::ButtonBase::IsPointerOverProperty(), { this, &SplitButton::OnVisualPropertyChanged });

        // Register for pointer events so we can keep track of the last used pointer type
        m_pointerEnteredPrimaryRevoker = primaryButton.PointerEntered(winrt::auto_revoke, { this, &SplitButton::OnPointerEvent });
        m_pointerExitedPrimaryRevoker = primaryButton.PointerExited(winrt::auto_revoke, { this, &SplitButton::OnPointerEvent });
        m_pointerPressedPrimaryRevoker = primaryButton.PointerPressed(winrt::auto_revoke, { this, &SplitButton::OnPointerEvent });
        m_pointerReleasedPrimaryRevoker = primaryButton.PointerReleased(winrt::auto_revoke, { this, &SplitButton::OnPointerEvent });
        m_pointerCanceledPrimaryRevoker = primaryButton.PointerCanceled(winrt::auto_revoke, { this, &SplitButton::OnPointerEvent });
        m_pointerCaptureLostPrimaryRevoker = primaryButton.PointerCaptureLost(winrt::auto_revoke, { this, &SplitButton::OnPointerEvent });
    }

    if (auto secondaryButton = m_secondaryButton.get())
    {
        // Do localization for the secondary button
        auto secondaryName = ResourceAccessor::GetLocalizedStringResource(SR_SplitButtonSecondaryButtonName);
        winrt::AutomationProperties::SetName(secondaryButton, secondaryName);

        m_clickSecondaryRevoker = secondaryButton.Click(winrt::auto_revoke, { this, &SplitButton::OnClickSecondary });

        m_pressedSecondaryRevoker = RegisterPropertyChanged(secondaryButton, winrt::ButtonBase::IsPressedProperty(), { this, &SplitButton::OnVisualPropertyChanged });
        m_pointerOverSecondaryRevoker = RegisterPropertyChanged(secondaryButton, winrt::ButtonBase::IsPointerOverProperty(), { this, &SplitButton::OnVisualPropertyChanged });

        // Register for pointer events so we can keep track of the last used pointer type
        m_pointerEnteredSecondaryRevoker = secondaryButton.PointerEntered(winrt::auto_revoke, { this, &SplitButton::OnPointerEvent });
        m_pointerExitedSecondaryRevoker = secondaryButton.PointerExited(winrt::auto_revoke, { this, &SplitButton::OnPointerEvent });
        m_pointerPressedSecondaryRevoker = secondaryButton.PointerPressed(winrt::auto_revoke, { this, &SplitButton::OnPointerEvent });
        m_pointerReleasedSecondaryRevoker = secondaryButton.PointerReleased(winrt::auto_revoke, { this, &SplitButton::OnPointerEvent });
        m_pointerCanceledSecondaryRevoker = secondaryButton.PointerCanceled(winrt::auto_revoke, { this, &SplitButton::OnPointerEvent });
        m_pointerCaptureLostSecondaryRevoker = secondaryButton.PointerCaptureLost(winrt::auto_revoke, { this, &SplitButton::OnPointerEvent });
    }

    // Register events on flyout
    RegisterFlyoutEvents();

    UpdateVisualStates();

    m_hasLoaded = true;
}

void SplitButton::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_FlyoutProperty)
    {
        OnFlyoutChanged();
    }
}

winrt::AutomationPeer SplitButton::OnCreateAutomationPeer()
{
    return winrt::make<SplitButtonAutomationPeer>(*this);
}

void SplitButton::OnFlyoutChanged()
{
    RegisterFlyoutEvents();

    UpdateVisualStates();
}

void SplitButton::RegisterFlyoutEvents()
{
    m_flyoutOpenedRevoker.revoke();
    m_flyoutClosedRevoker.revoke();
    m_flyoutPlacementChangedRevoker.revoke();

    if (Flyout())
    {
        m_flyoutOpenedRevoker = Flyout().Opened(winrt::auto_revoke, { this, &SplitButton::OnFlyoutOpened });

        m_flyoutClosedRevoker = Flyout().Closed(winrt::auto_revoke, { this, &SplitButton::OnFlyoutClosed });

        m_flyoutPlacementChangedRevoker = RegisterPropertyChanged(Flyout(), winrt::FlyoutBase::PlacementProperty(), { this, &SplitButton::OnFlyoutPlacementChanged });
    }
}

void SplitButton::OnVisualPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    UpdateVisualStates();
}

void SplitButton::UpdateVisualStates(bool useTransitions)
{
    // place the secondary button
    if (m_lastPointerDeviceType == winrt::PointerDeviceType::Touch || m_isKeyDown)
    {
        winrt::VisualStateManager::GoToState(*this, L"SecondaryButtonSpan", useTransitions);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"SecondaryButtonRight", useTransitions);
    }

    // change visual state
    auto primaryButton = m_primaryButton.get();
    auto secondaryButton = m_secondaryButton.get();
    if (primaryButton && secondaryButton)
    {
        if (m_isFlyoutOpen)
        {
            winrt::VisualStateManager::GoToState(*this, L"FlyoutOpen", useTransitions);
        }
        // SplitButton and ToggleSplitButton share a template -- this section is driving the checked states for ToggleSplitButton.
        else if (InternalIsChecked())
        {
            if (m_lastPointerDeviceType == winrt::PointerDeviceType::Touch || m_isKeyDown)
            {
                if (primaryButton.IsPressed() || secondaryButton.IsPressed() || m_isKeyDown)
                {
                    winrt::VisualStateManager::GoToState(*this, L"CheckedTouchPressed", useTransitions);
                }
                else
                {
                    winrt::VisualStateManager::GoToState(*this, L"Checked", useTransitions);
                }
            }
            else if (primaryButton.IsPressed())
            {
                winrt::VisualStateManager::GoToState(*this, L"CheckedPrimaryPressed", useTransitions);
            }
            else if (primaryButton.IsPointerOver())
            {
                winrt::VisualStateManager::GoToState(*this, L"CheckedPrimaryPointerOver", useTransitions);
            }
            else if (secondaryButton.IsPressed())
            {
                winrt::VisualStateManager::GoToState(*this, L"CheckedSecondaryPressed", useTransitions);
            }
            else if (secondaryButton.IsPointerOver())
            {
                winrt::VisualStateManager::GoToState(*this, L"CheckedSecondaryPointerOver", useTransitions);
            }
            else
            {
                winrt::VisualStateManager::GoToState(*this, L"Checked", useTransitions);
            }
        }
        else
        {
            if (m_lastPointerDeviceType == winrt::PointerDeviceType::Touch || m_isKeyDown)
            {
                if (primaryButton.IsPressed() || secondaryButton.IsPressed() || m_isKeyDown)
                {
                    winrt::VisualStateManager::GoToState(*this, L"TouchPressed", useTransitions);
                }
                else
                {
                    winrt::VisualStateManager::GoToState(*this, L"Normal", useTransitions);
                }
            }
            else if (primaryButton.IsPressed())
            {
                winrt::VisualStateManager::GoToState(*this, L"PrimaryPressed", useTransitions);
            }
            else if (primaryButton.IsPointerOver())
            {
                winrt::VisualStateManager::GoToState(*this, L"PrimaryPointerOver", useTransitions);
            }
            else if (secondaryButton.IsPressed())
            {
                winrt::VisualStateManager::GoToState(*this, L"SecondaryPressed", useTransitions);
            }
            else if (secondaryButton.IsPointerOver())
            {
                winrt::VisualStateManager::GoToState(*this, L"SecondaryPointerOver", useTransitions);
            }
            else
            {
                winrt::VisualStateManager::GoToState(*this, L"Normal", useTransitions);
            }
        }
    }
}

void SplitButton::OpenFlyout()
{
    if (auto flyout = Flyout())
    {
        if (SharedHelpers::IsFlyoutShowOptionsAvailable())
        {
            winrt::FlyoutShowOptions options{};
            options.Placement(winrt::FlyoutPlacementMode::BottomEdgeAlignedLeft);
            flyout.ShowAt(*this, options);
        }
        else
        {
            flyout.ShowAt(*this);
        }
    }
}

void SplitButton::CloseFlyout()
{
    if (auto flyout = Flyout())
    {
        flyout.Hide();
    }
}

void SplitButton::OnFlyoutOpened(const winrt::IInspectable& sender, const winrt::IInspectable& args)
{
    m_isFlyoutOpen = true;
    UpdateVisualStates();
    SharedHelpers::RaiseAutomationPropertyChangedEvent(*this, winrt::ExpandCollapseState::Collapsed, winrt::ExpandCollapseState::Expanded);
}

void SplitButton::OnFlyoutClosed(const winrt::IInspectable& sender, const winrt::IInspectable& args)
{
    m_isFlyoutOpen = false;
    UpdateVisualStates();
    SharedHelpers::RaiseAutomationPropertyChangedEvent(*this, winrt::ExpandCollapseState::Expanded, winrt::ExpandCollapseState::Collapsed);
}

void SplitButton::OnFlyoutPlacementChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    UpdateVisualStates();
}

void SplitButton::OnClickPrimary(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    auto eventArgs = winrt::make_self<SplitButtonClickEventArgs>();
    m_clickEventSource(*this, *eventArgs);

    if (winrt::AutomationPeer peer = winrt::FrameworkElementAutomationPeer::FromElement(*this))
    {
        peer.RaiseAutomationEvent(winrt::AutomationEvents::InvokePatternOnInvoked);
    }
}

void SplitButton::OnClickSecondary(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    OpenFlyout();
}

void SplitButton::OnPointerEvent(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    winrt::PointerDeviceType pointerDeviceType = args.Pointer().PointerDeviceType();

    // Allows the test app to simulate how the control responds to touch input.
    if (SplitButtonTestHelper::SimulateTouch())
    {
        pointerDeviceType = winrt::PointerDeviceType::Touch;
    }

    if (m_lastPointerDeviceType != pointerDeviceType)
    {
        m_lastPointerDeviceType = pointerDeviceType;
        UpdateVisualStates();
    }
}

void SplitButton::OnSplitButtonKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args)
{
    const winrt::VirtualKey key = args.Key();
    if (key == winrt::VirtualKey::Space || key == winrt::VirtualKey::Enter || key == winrt::VirtualKey::GamepadA)
    {
        m_isKeyDown = true;
        UpdateVisualStates();
    }
}

void SplitButton::OnSplitButtonKeyUp(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args)
{
    const winrt::VirtualKey key = args.Key();
    if (key == winrt::VirtualKey::Space || key == winrt::VirtualKey::Enter || key == winrt::VirtualKey::GamepadA)
    {
        m_isKeyDown = false;
        UpdateVisualStates();

        // Consider this a click on the primary button
        if (IsEnabled())
        {
            OnClickPrimary(nullptr, nullptr);
            args.Handled(true);
        }
    }
    else if (key == winrt::VirtualKey::Down)
    {
        winrt::CoreVirtualKeyStates menuState = winrt::CoreWindow::GetForCurrentThread().GetKeyState(winrt::VirtualKey::Menu);
        const bool menuKeyDown = (menuState & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;

        if (IsEnabled() && menuKeyDown)
        {
            // Open the menu on alt-down
            OpenFlyout();
            args.Handled(true);
        }
    }
    else if (key == winrt::VirtualKey::F4 && IsEnabled())
    {
        // Open the menu on F4
        OpenFlyout();
        args.Handled(true);
    }
}

void SplitButton::UnregisterEvents()
{
    // This explicitly unregisters all events related to the two buttons in OnApplyTemplate
    // in case the new template doesn't have all the expected elements.

    m_clickPrimaryRevoker.revoke();
    m_pressedPrimaryRevoker.revoke();
    m_pointerOverPrimaryRevoker.revoke();

    m_pointerEnteredPrimaryRevoker.revoke();
    m_pointerExitedPrimaryRevoker.revoke();
    m_pointerPressedPrimaryRevoker.revoke();
    m_pointerReleasedPrimaryRevoker.revoke();
    m_pointerCanceledPrimaryRevoker.revoke();
    m_pointerCaptureLostPrimaryRevoker.revoke();

    m_clickSecondaryRevoker.revoke();
    m_pressedSecondaryRevoker.revoke();
    m_pointerOverSecondaryRevoker.revoke();

    m_pointerEnteredSecondaryRevoker.revoke();
    m_pointerExitedSecondaryRevoker.revoke();
    m_pointerPressedSecondaryRevoker.revoke();
    m_pointerReleasedSecondaryRevoker.revoke();
    m_pointerCanceledSecondaryRevoker.revoke();
    m_pointerCaptureLostSecondaryRevoker.revoke();
}
