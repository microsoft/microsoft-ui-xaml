// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Faithful C++/WinRT port of onecoreuap\...\inkcontrols\lib\ButtonManager.h.
// ButtonManager is a plain (non-WinRT) helper owned by the InkToolbar container. Structure/logic
// preserved 1:1; WRL plumbing translated: wrl::ComPtr<IInkToolbar*Button> -> winrt projected types,
// wrl::WeakRef -> winrt::weak_ref, EventRegistrationToken -> winrt::event_token, Private::TrackerPtr
// -> winrt::weak_ref (ButtonManager is not a ReferenceTracker; buttons live in the container's tree).

#pragma once

#include "pch.h"
#include "common.h"

class InkToolbar;

// Manages buttons: those added to the InkToolbar's Children collection, and any that are
// auto-populated. Tracks buttons, provides query services over the collection, and manages pointer
// and keyboard interactions with the buttons.
class ButtonManager
{
public:
    ButtonManager(winrt::weak_ref<InkToolbar> const& inkToolbar);
    ~ButtonManager();

    static void PutLocalizedContent(winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase const& button, uint16_t resourceId);
    static bool GetL3Content(
        winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase const& button,
        winrt::UIElement& returnValue);

    bool AddButton(winrt::UIElement const& child);
    void RemoveButton(winrt::UIElement const& child);

    // Returns buttons that were added.
    std::vector<winrt::UIElement> AutoPopulate(winrt::InkToolbarInitialControls autoPopulate);

    std::vector<winrt::InkToolbarPenButton> GetSystemPenButtons() const;
    std::vector<winrt::InkToolbarCustomPenButton> GetCustomPenButtons() const;
    std::vector<winrt::InkToolbarPenButton> GetAllPenButtons() const;
    std::vector<winrt::InkToolbarCustomToolButton> GetCustomToolButtons() const;
    std::vector<winrt::InkToolbarCustomToggleButton> GetCustomToggleButtons() const;
    winrt::InkToolbarEraserButton GetEraserButton() const;
    winrt::InkToolbarRulerButton GetRulerButton() const;
    winrt::InkToolbarStencilButton GetStencilButton() const;

    winrt::InkToolbarPenButton GetFirstPenButton() const;
    winrt::InkToolbarPenButton GetLastPenButton() const;

    enum class RelativeButton
    {
        First,
        Last,
        Next,
        Previous
    };

    winrt::InkToolbarToolButton GetRelativeButton(
        RelativeButton relative,
        winrt::InkToolbarToolButton const& relativeTo) const;
    winrt::InkToolbarToolButton GetFirstToolButton() const;
    winrt::InkToolbarToolButton GetLastToolButton() const;

    winrt::InkToolbarToolButton GetToolButton(winrt::InkToolbarTool tool) const;
    winrt::InkToolbarToggleButton GetToggleButton(winrt::InkToolbarToggle toggle) const;
    winrt::InkToolbarMenuButton GetMenuButton(winrt::InkToolbarMenuKind menu) const;

    unsigned PenCount() const;
    unsigned ToolCount() const;
    unsigned MenuCount() const;
    bool IsOneOfOurs(winrt::UIElement const& child) const;

    void ForEachButton(std::function<void(winrt::UIElement const&)> function) const;
    void ForEachToolButton(std::function<void(winrt::InkToolbarToolButton const&)> function) const;
    void ForEachToggleButton(std::function<void(winrt::InkToolbarToggleButton const&)> function) const;
    void ForEachMenuButton(std::function<void(winrt::InkToolbarMenuButton const&)> function) const;

    // Button gesture / routed-event handlers (public so they can be wired as delegates).
    void OnButtonRightTapped(winrt::IInspectable const& sender, winrt::RightTappedRoutedEventArgs const& args);
    void OnButtonDoubleTapped(winrt::IInspectable const& sender, winrt::DoubleTappedRoutedEventArgs const& args);
    void OnButtonHolding(winrt::IInspectable const& sender, winrt::HoldingRoutedEventArgs const& args);
    void OnButtonKeyDown(winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args);
    void OnButtonChecked(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnButtonUnchecked(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnButtonIndeterminate(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnButtonClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);

private:
    winrt::weak_ref<InkToolbar> m_inkToolbar;

    // Shared routed events (from UIElement statics) used to add/remove class handlers.
    winrt::RoutedEvent m_buttonRightTappedEvent{ nullptr };
    winrt::RoutedEvent m_buttonDoubleTappedEvent{ nullptr };
    winrt::RoutedEvent m_buttonHoldingEvent{ nullptr };
    winrt::RoutedEvent m_buttonKeyDownEvent{ nullptr };

    // Handler delegates (stored so we can AddHandler/RemoveHandler with the same instance).
    winrt::IInspectable m_buttonRightTappedEventHandler{ nullptr };
    winrt::IInspectable m_buttonDoubleTappedEventHandler{ nullptr };
    winrt::IInspectable m_buttonHoldingEventHandler{ nullptr };
    winrt::IInspectable m_buttonKeyDownEventHandler{ nullptr };
    winrt::RoutedEventHandler m_buttonCheckedHandler{ nullptr };
    winrt::RoutedEventHandler m_buttonUncheckedHandler{ nullptr };
    winrt::RoutedEventHandler m_buttonIndeterminateHandler{ nullptr };
    winrt::RoutedEventHandler m_buttonClickHandler{ nullptr };

    struct ManagedButtonEventTokens
    {
        winrt::event_token CheckedRegistrationToken{};
        winrt::event_token UncheckedRegistrationToken{};
        winrt::event_token IndeterminateRegistrationToken{};
        winrt::event_token ClickRegistrationToken{};
    };

    ManagedButtonEventTokens m_ballpointPenButtonEventTokens;
    winrt::weak_ref<winrt::UIElement> m_ballpointPenButton;

    ManagedButtonEventTokens m_pencilButtonEventTokens;
    winrt::weak_ref<winrt::UIElement> m_pencilButton;

    ManagedButtonEventTokens m_highlighterButtonEventTokens;
    winrt::weak_ref<winrt::UIElement> m_highlighterButton;

    ManagedButtonEventTokens m_eraserButtonEventTokens;
    winrt::weak_ref<winrt::UIElement> m_eraserButton;

    ManagedButtonEventTokens m_rulerButtonEventTokens;
    winrt::weak_ref<winrt::UIElement> m_rulerButton;

    ManagedButtonEventTokens m_stencilButtonEventTokens;
    winrt::weak_ref<winrt::UIElement> m_stencilButton;

    std::vector<ManagedButtonEventTokens> m_customPenButtonsEventTokens;
    std::vector<winrt::weak_ref<winrt::UIElement>> m_customPenButtons;

    std::vector<ManagedButtonEventTokens> m_customToolButtonsEventTokens;
    std::vector<winrt::weak_ref<winrt::UIElement>> m_customToolButtons;

    std::vector<ManagedButtonEventTokens> m_customToggleButtonsEventTokens;
    std::vector<winrt::weak_ref<winrt::UIElement>> m_customToggleButtons;

    winrt::InkToolbar ReferenceInkToolbar() const;

    void AutoPopulateSystemPens(std::vector<winrt::UIElement>& added);
    void AutoPopulateEraser(std::vector<winrt::UIElement>& added);
    void AutoPopulateRuler(std::vector<winrt::UIElement>& added);
    void AutoPopulateStencil(std::vector<winrt::UIElement>& added);

    void AddHandlers(winrt::UIElement const& button, ManagedButtonEventTokens* managed);
    void RemoveHandlers(ManagedButtonEventTokens* button, winrt::UIElement const& safeButton);

    void ClearCustomButtons(
        std::vector<ManagedButtonEventTokens>& eventTokens,
        std::vector<winrt::weak_ref<winrt::UIElement>>& trackerVector);

    void ButtonAction(winrt::IInspectable const& sender, bool isNormalActivation);
    void DispatchButtonCheckStateChanged(winrt::IInspectable const& sender, bool check);
};
