// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Faithful C++/WinRT port of onecoreuap\...\inkcontrols\lib\ButtonManager.cpp.
// Structure/logic preserved 1:1. WRL plumbing translated to C++/WinRT: QueryInterface -> try_as,
// get_X(&v) -> v = X(), HRESULT/THROW_IF_FAILED -> exceptions, TrackerPtr -> winrt::weak_ref,
// EventRegistrationToken -> winrt::event_token, CoreDispatcher post -> DispatcherQueue.TryEnqueue.
// Documented lift deltas: PutLocalizedContent resource lookup (no FindStringResource in lift) is a
// no-op stub; RS2 quirk (QuirkUsePreRedstone2InkToobarBehaviors) is always false so AutoPopulate uses
// the modern Stencil path; design-mode special-casing dropped.

#include "pch.h"
#include "common.h"
#include "ButtonManager.h"
#include "InkToolbar.h"
#include "InkToolbarBallpointPenButton.h"
#include "InkToolbarPencilButton.h"
#include "InkToolbarHighlighterButton.h"
#include "InkToolbarEraserButton.h"
#include "InkToolbarRulerButton.h"
#include "InkToolbarStencilButton.h"
#include "InkToolbarMenuButton.h"
#include "InkToolbarToggleButton.h"
#include "InkToolbarToolButton.h"

ButtonManager::ButtonManager(winrt::weak_ref<InkToolbar> const& inkToolbar) : m_inkToolbar(inkToolbar)
{
    // Shared routed events used to hook class handlers on each button.
    m_buttonRightTappedEvent = winrt::UIElement::RightTappedEvent();
    m_buttonDoubleTappedEvent = winrt::UIElement::DoubleTappedEvent();
    m_buttonHoldingEvent = winrt::UIElement::HoldingEvent();
    m_buttonKeyDownEvent = winrt::UIElement::KeyDownEvent();

    m_buttonRightTappedEventHandler = winrt::box_value(winrt::RightTappedEventHandler(
        [this](winrt::IInspectable const& s, winrt::RightTappedRoutedEventArgs const& e) { OnButtonRightTapped(s, e); }));
    m_buttonDoubleTappedEventHandler = winrt::box_value(winrt::DoubleTappedEventHandler(
        [this](winrt::IInspectable const& s, winrt::DoubleTappedRoutedEventArgs const& e) { OnButtonDoubleTapped(s, e); }));
    m_buttonHoldingEventHandler = winrt::box_value(winrt::HoldingEventHandler(
        [this](winrt::IInspectable const& s, winrt::HoldingRoutedEventArgs const& e) { OnButtonHolding(s, e); }));
    m_buttonKeyDownEventHandler = winrt::box_value(winrt::KeyEventHandler(
        [this](winrt::IInspectable const& s, winrt::KeyRoutedEventArgs const& e) { OnButtonKeyDown(s, e); }));

    m_buttonCheckedHandler = winrt::RoutedEventHandler(
        [this](winrt::IInspectable const& s, winrt::RoutedEventArgs const& e) { OnButtonChecked(s, e); });
    m_buttonUncheckedHandler = winrt::RoutedEventHandler(
        [this](winrt::IInspectable const& s, winrt::RoutedEventArgs const& e) { OnButtonUnchecked(s, e); });
    m_buttonIndeterminateHandler = winrt::RoutedEventHandler(
        [this](winrt::IInspectable const& s, winrt::RoutedEventArgs const& e) { OnButtonIndeterminate(s, e); });
    m_buttonClickHandler = winrt::RoutedEventHandler(
        [this](winrt::IInspectable const& s, winrt::RoutedEventArgs const& e) { OnButtonClick(s, e); });
}

ButtonManager::~ButtonManager()
{
    try
    {
        if (auto safeButton = m_ballpointPenButton.get())
        {
            RemoveHandlers(&m_ballpointPenButtonEventTokens, safeButton);
            m_ballpointPenButton = nullptr;
        }
        if (auto safeButton = m_pencilButton.get())
        {
            RemoveHandlers(&m_pencilButtonEventTokens, safeButton);
            m_pencilButton = nullptr;
        }
        if (auto safeButton = m_highlighterButton.get())
        {
            RemoveHandlers(&m_highlighterButtonEventTokens, safeButton);
            m_highlighterButton = nullptr;
        }
        // Deviation from UWP: the UWP ~ButtonManager omits the eraser here. The eraser's handlers capture
        // the ButtonManager, so we also remove them to avoid a dangling class handler during teardown.
        if (auto safeButton = m_eraserButton.get())
        {
            RemoveHandlers(&m_eraserButtonEventTokens, safeButton);
            m_eraserButton = nullptr;
        }
        if (auto safeButton = m_rulerButton.get())
        {
            RemoveHandlers(&m_rulerButtonEventTokens, safeButton);
            m_rulerButton = nullptr;
        }
        if (auto safeButton = m_stencilButton.get())
        {
            RemoveHandlers(&m_stencilButtonEventTokens, safeButton);
            m_stencilButton = nullptr;
        }

        ClearCustomButtons(m_customPenButtonsEventTokens, m_customPenButtons);
        ClearCustomButtons(m_customToolButtonsEventTokens, m_customToolButtons);
        ClearCustomButtons(m_customToggleButtonsEventTokens, m_customToggleButtons);
    }
    catch (...)
    {
    }
}

// If the button content is plain text, put the localized string as the content. If the button is
// composite, put the string to the button name textblock.
// LIFT DELTA: per-button localized resource strings are not exposed to the lift (no FindStringResource
// equivalent); this is the same localization gap tracked for ColorNames/tooltips. Structure preserved;
// resource lookup yields empty -> no-op. File a bug to restore localized button content.
void ButtonManager::PutLocalizedContent(
    winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase const& button,
    uint16_t resourceId)
{
    UNREFERENCED_PARAMETER(button);
    UNREFERENCED_PARAMETER(resourceId);
    // No localized resource provider in the lift; nothing to set.
}

// The button has an L3 if there's a flyout with content. Return that content if it exists.
bool ButtonManager::GetL3Content(
    winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase const& button,
    winrt::UIElement& returnValue)
{
    auto flyoutBase = winrt::FlyoutBase::GetAttachedFlyout(button);
    if (!flyoutBase)
    {
        return false;
    }

    if (auto flyout = flyoutBase.try_as<winrt::Flyout>())
    {
        if (auto flyoutContent = flyout.Content())
        {
            returnValue = flyoutContent;
            return true;
        }
    }

    return false;
}

// The InkToolbar owns the ButtonManager (unique_ptr) for its lifetime, so strengthening never fails.
// get_weak() yields a weak_ref of the implementation type, so resolve to the impl and project it here.
winrt::InkToolbar ButtonManager::ReferenceInkToolbar() const
{
    auto inkToolbar = m_inkToolbar.get();
    if (!inkToolbar)
    {
        throw winrt::hresult_error(E_UNEXPECTED);
    }
    return inkToolbar.as<winrt::InkToolbar>();
}

bool ButtonManager::AddButton(winrt::UIElement const& child)
{
    if (auto toolButton = child.try_as<winrt::InkToolbarToolButton>())
    {
        switch (toolButton.ToolKind())
        {
        case winrt::InkToolbarTool::BallpointPen:
            if (m_ballpointPenButton.get()) { return false; }
            m_ballpointPenButton = winrt::make_weak(child);
            AddHandlers(child, &m_ballpointPenButtonEventTokens);
            return true;

        case winrt::InkToolbarTool::CustomPen:
        {
            for (auto const& weak : m_customPenButtons)
            {
                if (weak.get() == child) { return false; }
            }
            m_customPenButtonsEventTokens.push_back(ManagedButtonEventTokens());
            m_customPenButtons.push_back(winrt::make_weak(child));
            AddHandlers(child, &m_customPenButtonsEventTokens.back());
            return true;
        }

        case winrt::InkToolbarTool::CustomTool:
        {
            for (auto const& weak : m_customToolButtons)
            {
                if (weak.get() == child) { return false; }
            }
            m_customToolButtonsEventTokens.push_back(ManagedButtonEventTokens());
            m_customToolButtons.push_back(winrt::make_weak(child));
            AddHandlers(child, &m_customToolButtonsEventTokens.back());
            return true;
        }

        case winrt::InkToolbarTool::Eraser:
            if (m_eraserButton.get()) { return false; }
            m_eraserButton = winrt::make_weak(child);
            AddHandlers(child, &m_eraserButtonEventTokens);
            return true;

        case winrt::InkToolbarTool::Highlighter:
            if (m_highlighterButton.get()) { return false; }
            m_highlighterButton = winrt::make_weak(child);
            AddHandlers(child, &m_highlighterButtonEventTokens);
            return true;

        case winrt::InkToolbarTool::Pencil:
            if (m_pencilButton.get()) { return false; }
            m_pencilButton = winrt::make_weak(child);
            AddHandlers(child, &m_pencilButtonEventTokens);
            return true;

        default:
            throw winrt::hresult_error(E_UNEXPECTED, L"No code for this kind of tool button");
        }
    }

    if (auto toggleButton = child.try_as<winrt::InkToolbarToggleButton>())
    {
        switch (toggleButton.ToggleKind())
        {
        case winrt::InkToolbarToggle::Custom:
        {
            for (auto const& weak : m_customToggleButtons)
            {
                if (weak.get() == child) { return false; }
            }
            m_customToggleButtonsEventTokens.push_back(ManagedButtonEventTokens());
            m_customToggleButtons.push_back(winrt::make_weak(child));
            AddHandlers(child, &m_customToggleButtonsEventTokens.back());
            return true;
        }

        case winrt::InkToolbarToggle::Ruler:
            if (m_rulerButton.get()) { return false; }
            m_rulerButton = winrt::make_weak(child);
            AddHandlers(child, &m_rulerButtonEventTokens);
            return true;

        default:
            throw winrt::hresult_error(E_UNEXPECTED, L"No code for this kind of toggle button");
        }
    }

    if (auto menuButton = child.try_as<winrt::InkToolbarMenuButton>())
    {
        switch (menuButton.MenuKind())
        {
        case winrt::InkToolbarMenuKind::Stencil:
            if (m_stencilButton.get()) { return false; }
            m_stencilButton = winrt::make_weak(child);
            AddHandlers(child, &m_stencilButtonEventTokens);
            return true;

        default:
            throw winrt::hresult_error(E_UNEXPECTED, L"No code for this kind of menu button");
        }
    }

    return false;
}

void ButtonManager::RemoveButton(winrt::UIElement const& child)
{
    if (auto toolButton = child.try_as<winrt::InkToolbarToolButton>())
    {
        switch (toolButton.ToolKind())
        {
        case winrt::InkToolbarTool::BallpointPen:
            if (auto safeButton = m_ballpointPenButton.get(); safeButton == child)
            {
                RemoveHandlers(&m_ballpointPenButtonEventTokens, safeButton);
                m_ballpointPenButton = nullptr;
            }
            return;

        case winrt::InkToolbarTool::CustomPen:
            for (size_t index = 0; index < m_customPenButtons.size(); ++index)
            {
                if (auto safeButton = m_customPenButtons[index].get(); safeButton && safeButton == child)
                {
                    RemoveHandlers(&m_customPenButtonsEventTokens[index], safeButton);
                    m_customPenButtons.erase(m_customPenButtons.begin() + index);
                    m_customPenButtonsEventTokens.erase(m_customPenButtonsEventTokens.cbegin() + index);
                    return;
                }
            }
            return;

        case winrt::InkToolbarTool::CustomTool:
            for (size_t index = 0; index < m_customToolButtons.size(); ++index)
            {
                if (auto safeButton = m_customToolButtons[index].get(); safeButton && safeButton == child)
                {
                    RemoveHandlers(&m_customToolButtonsEventTokens[index], safeButton);
                    m_customToolButtons.erase(m_customToolButtons.begin() + index);
                    m_customToolButtonsEventTokens.erase(m_customToolButtonsEventTokens.cbegin() + index);
                    return;
                }
            }
            return;

        case winrt::InkToolbarTool::Eraser:
            if (auto safeButton = m_eraserButton.get(); safeButton == child)
            {
                RemoveHandlers(&m_eraserButtonEventTokens, safeButton);
                m_eraserButton = nullptr;
            }
            return;

        case winrt::InkToolbarTool::Highlighter:
            if (auto safeButton = m_highlighterButton.get(); safeButton == child)
            {
                RemoveHandlers(&m_highlighterButtonEventTokens, safeButton);
                m_highlighterButton = nullptr;
            }
            return;

        case winrt::InkToolbarTool::Pencil:
            if (auto safeButton = m_pencilButton.get(); safeButton == child)
            {
                RemoveHandlers(&m_pencilButtonEventTokens, safeButton);
                m_pencilButton = nullptr;
            }
            return;

        default:
            throw winrt::hresult_error(E_UNEXPECTED, L"No code for this kind of tool button");
        }
    }

    if (auto toggleButton = child.try_as<winrt::InkToolbarToggleButton>())
    {
        switch (toggleButton.ToggleKind())
        {
        case winrt::InkToolbarToggle::Custom:
            for (size_t index = 0; index < m_customToggleButtons.size(); ++index)
            {
                if (auto safeButton = m_customToggleButtons[index].get(); safeButton && safeButton == child)
                {
                    RemoveHandlers(&m_customToggleButtonsEventTokens[index], safeButton);
                    m_customToggleButtons.erase(m_customToggleButtons.begin() + index);
                    m_customToggleButtonsEventTokens.erase(m_customToggleButtonsEventTokens.cbegin() + index);
                    return;
                }
            }
            return;

        case winrt::InkToolbarToggle::Ruler:
            if (auto safeButton = m_rulerButton.get(); safeButton == child)
            {
                RemoveHandlers(&m_rulerButtonEventTokens, safeButton);
                m_rulerButton = nullptr;
            }
            return;

        default:
            throw winrt::hresult_error(E_UNEXPECTED, L"No code for this kind of toggle button");
        }
    }

    if (auto menuButton = child.try_as<winrt::InkToolbarMenuButton>())
    {
        switch (menuButton.MenuKind())
        {
        case winrt::InkToolbarMenuKind::Stencil:
            if (auto safeButton = m_stencilButton.get(); safeButton == child)
            {
                RemoveHandlers(&m_stencilButtonEventTokens, safeButton);
                m_stencilButton = nullptr;
            }
            return;

        default:
            throw winrt::hresult_error(E_UNEXPECTED, L"No code for this kind of menu button");
        }
    }
}

std::vector<winrt::UIElement> ButtonManager::AutoPopulate(winrt::InkToolbarInitialControls autoPopulate)
{
    std::vector<winrt::UIElement> added;

    if (autoPopulate == winrt::InkToolbarInitialControls::All
        || autoPopulate == winrt::InkToolbarInitialControls::PensOnly)
    {
        AutoPopulateSystemPens(added);
    }

    if (autoPopulate == winrt::InkToolbarInitialControls::All
        || autoPopulate == winrt::InkToolbarInitialControls::AllExceptPens)
    {
        AutoPopulateEraser(added);
        // LIFT DELTA: RS2 quirk (QuirkUsePreRedstone2InkToobarBehaviors) is always false in the lift,
        // so we always take the modern stencil path (never the legacy ruler-only path).
        AutoPopulateStencil(added);
    }

    return added;
}

void ButtonManager::AutoPopulateSystemPens(std::vector<winrt::UIElement>& added)
{
    if (!m_ballpointPenButton.get())
    {
        auto button = winrt::make<InkToolbarBallpointPenButton>().as<winrt::UIElement>();
        AddButton(button);
        added.push_back(button);
    }

    if (!m_pencilButton.get())
    {
        auto button = winrt::make<InkToolbarPencilButton>().as<winrt::UIElement>();
        AddButton(button);
        added.push_back(button);
    }

    if (!m_highlighterButton.get())
    {
        auto button = winrt::make<InkToolbarHighlighterButton>().as<winrt::UIElement>();
        AddButton(button);
        added.push_back(button);
    }
}

void ButtonManager::AutoPopulateEraser(std::vector<winrt::UIElement>& added)
{
    if (!m_eraserButton.get())
    {
        auto button = winrt::make<InkToolbarEraserButton>().as<winrt::UIElement>();
        AddButton(button);
        added.push_back(button);
    }
}

void ButtonManager::AutoPopulateRuler(std::vector<winrt::UIElement>& added)
{
    if (!m_rulerButton.get())
    {
        auto button = winrt::make<InkToolbarRulerButton>().as<winrt::UIElement>();
        AddButton(button);
        added.push_back(button);
    }
}

void ButtonManager::AutoPopulateStencil(std::vector<winrt::UIElement>& added)
{
    if (!m_stencilButton.get())
    {
        auto button = winrt::make<InkToolbarStencilButton>().as<winrt::UIElement>();
        AddButton(button);
        added.push_back(button);
    }
}

// Hook up handlers for pointer manipulation and other important events. We watch Click (not Tapped)
// because a button can become selected without a Tapped event.
void ButtonManager::AddHandlers(winrt::UIElement const& button, ManagedButtonEventTokens* managed)
{
    button.AddHandler(m_buttonRightTappedEvent, m_buttonRightTappedEventHandler, true);
    button.AddHandler(m_buttonDoubleTappedEvent, m_buttonDoubleTappedEventHandler, true);
    button.AddHandler(m_buttonHoldingEvent, m_buttonHoldingEventHandler, true);
    button.AddHandler(m_buttonKeyDownEvent, m_buttonKeyDownEventHandler, true);

    auto buttonAsToggleButton = button.as<winrt::Microsoft::UI::Xaml::Controls::Primitives::ToggleButton>();
    auto buttonAsButtonBase = button.as<winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase>();

    managed->CheckedRegistrationToken = buttonAsToggleButton.Checked(m_buttonCheckedHandler);
    managed->UncheckedRegistrationToken = buttonAsToggleButton.Unchecked(m_buttonUncheckedHandler);
    managed->IndeterminateRegistrationToken = buttonAsToggleButton.Indeterminate(m_buttonIndeterminateHandler);
    managed->ClickRegistrationToken = buttonAsButtonBase.Click(m_buttonClickHandler);
}

void ButtonManager::RemoveHandlers(ManagedButtonEventTokens* button, winrt::UIElement const& safeButton)
{
    if (safeButton)
    {
        safeButton.RemoveHandler(m_buttonRightTappedEvent, m_buttonRightTappedEventHandler);
        safeButton.RemoveHandler(m_buttonDoubleTappedEvent, m_buttonDoubleTappedEventHandler);
        safeButton.RemoveHandler(m_buttonHoldingEvent, m_buttonHoldingEventHandler);
        safeButton.RemoveHandler(m_buttonKeyDownEvent, m_buttonKeyDownEventHandler);

        auto buttonAsToggleButton = safeButton.as<winrt::Microsoft::UI::Xaml::Controls::Primitives::ToggleButton>();
        buttonAsToggleButton.Checked(button->CheckedRegistrationToken);
        buttonAsToggleButton.Unchecked(button->UncheckedRegistrationToken);
        buttonAsToggleButton.Indeterminate(button->IndeterminateRegistrationToken);

        auto buttonAsButtonBase = safeButton.as<winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase>();
        buttonAsButtonBase.Click(button->ClickRegistrationToken);
    }
}

void ButtonManager::ClearCustomButtons(
    std::vector<ManagedButtonEventTokens>& eventTokens,
    std::vector<winrt::weak_ref<winrt::UIElement>>& trackerVector)
{
    for (size_t i = 0; i < eventTokens.size() && i < trackerVector.size(); ++i)
    {
        if (auto safeButton = trackerVector[i].get())
        {
            RemoveHandlers(&eventTokens[i], safeButton);
        }
    }
    trackerVector.clear();
    eventTokens.clear();
}

unsigned ButtonManager::PenCount() const
{
    return (m_ballpointPenButton.get() ? 1 : 0)
        + (m_pencilButton.get() ? 1 : 0)
        + (m_highlighterButton.get() ? 1 : 0)
        + static_cast<unsigned>(m_customPenButtons.size());
}

unsigned ButtonManager::ToolCount() const
{
    return PenCount()
        + (m_eraserButton.get() ? 1 : 0)
        + static_cast<unsigned>(m_customToolButtons.size());
}

unsigned ButtonManager::MenuCount() const
{
    return (m_stencilButton.get() ? 1 : 0);
}

std::vector<winrt::InkToolbarPenButton> ButtonManager::GetSystemPenButtons() const
{
    std::vector<winrt::InkToolbarPenButton> buttons;

    if (auto b = m_ballpointPenButton.get()) { buttons.push_back(b.as<winrt::InkToolbarPenButton>()); }
    if (auto b = m_pencilButton.get()) { buttons.push_back(b.as<winrt::InkToolbarPenButton>()); }
    if (auto b = m_highlighterButton.get()) { buttons.push_back(b.as<winrt::InkToolbarPenButton>()); }

    return buttons;
}

std::vector<winrt::InkToolbarCustomPenButton> ButtonManager::GetCustomPenButtons() const
{
    std::vector<winrt::InkToolbarCustomPenButton> buttons;
    for (auto const& weak : m_customPenButtons)
    {
        if (auto b = weak.get()) { buttons.push_back(b.as<winrt::InkToolbarCustomPenButton>()); }
    }
    return buttons;
}

std::vector<winrt::InkToolbarPenButton> ButtonManager::GetAllPenButtons() const
{
    std::vector<winrt::InkToolbarPenButton> buttons = GetSystemPenButtons();
    for (auto const& custom : GetCustomPenButtons())
    {
        buttons.push_back(custom.as<winrt::InkToolbarPenButton>());
    }
    return buttons;
}

winrt::InkToolbarPenButton ButtonManager::GetFirstPenButton() const
{
    if (auto b = m_ballpointPenButton.get()) { return b.as<winrt::InkToolbarPenButton>(); }
    if (auto b = m_pencilButton.get()) { return b.as<winrt::InkToolbarPenButton>(); }
    if (auto b = m_highlighterButton.get()) { return b.as<winrt::InkToolbarPenButton>(); }
    if (!m_customPenButtons.empty())
    {
        if (auto b = m_customPenButtons.front().get()) { return b.as<winrt::InkToolbarPenButton>(); }
    }
    return nullptr;
}

winrt::InkToolbarPenButton ButtonManager::GetLastPenButton() const
{
    if (!m_customPenButtons.empty())
    {
        if (auto b = m_customPenButtons.back().get()) { return b.as<winrt::InkToolbarPenButton>(); }
    }
    if (auto b = m_highlighterButton.get()) { return b.as<winrt::InkToolbarPenButton>(); }
    if (auto b = m_pencilButton.get()) { return b.as<winrt::InkToolbarPenButton>(); }
    if (auto b = m_ballpointPenButton.get()) { return b.as<winrt::InkToolbarPenButton>(); }
    return nullptr;
}

// Returns the relative button, or nullptr if no such button is found.
winrt::InkToolbarToolButton ButtonManager::GetRelativeButton(
    RelativeButton relative,
    winrt::InkToolbarToolButton const& relativeTo) const
{
    std::vector<winrt::InkToolbarToolButton> buttons;
    ForEachToolButton([&buttons](winrt::InkToolbarToolButton const& button)
    {
        // Only consider visible buttons.
        if (button.as<winrt::UIElement>().Visibility() == winrt::Visibility::Visible)
        {
            buttons.push_back(button);
        }
    });

    if (buttons.empty())
    {
        return nullptr;
    }

    switch (relative)
    {
    case RelativeButton::First:
        return buttons.front();
    case RelativeButton::Last:
        return buttons.back();
    default:
        break;
    }

    unsigned buttonIndex = 0;
    bool found = false;
    for (unsigned i = 0; i < buttons.size(); ++i)
    {
        if (buttons[i] == relativeTo)
        {
            buttonIndex = i;
            found = true;
            break;
        }
    }

    if (!found)
    {
        return nullptr;
    }

    switch (relative)
    {
    case RelativeButton::Previous:
        if (buttonIndex == 0) { return nullptr; }   // Reached the first button, do not wrap around.
        return buttons[buttonIndex - 1];

    case RelativeButton::Next:
        if (buttonIndex == (buttons.size() - 1)) { return nullptr; }   // Reached the last button, no wrap.
        return buttons[buttonIndex + 1];

    default:
        throw winrt::hresult_invalid_argument(L"ButtonManager: Got unexpected RelativeButton");
    }
}

winrt::InkToolbarToolButton ButtonManager::GetFirstToolButton() const
{
    return GetRelativeButton(RelativeButton::First, nullptr);
}

winrt::InkToolbarToolButton ButtonManager::GetLastToolButton() const
{
    return GetRelativeButton(RelativeButton::Last, nullptr);
}

std::vector<winrt::InkToolbarCustomToolButton> ButtonManager::GetCustomToolButtons() const
{
    std::vector<winrt::InkToolbarCustomToolButton> buttons;
    for (auto const& weak : m_customToolButtons)
    {
        if (auto b = weak.get()) { buttons.push_back(b.as<winrt::InkToolbarCustomToolButton>()); }
    }
    return buttons;
}

std::vector<winrt::InkToolbarCustomToggleButton> ButtonManager::GetCustomToggleButtons() const
{
    std::vector<winrt::InkToolbarCustomToggleButton> buttons;
    for (auto const& weak : m_customToggleButtons)
    {
        if (auto b = weak.get()) { buttons.push_back(b.as<winrt::InkToolbarCustomToggleButton>()); }
    }
    return buttons;
}

winrt::InkToolbarEraserButton ButtonManager::GetEraserButton() const
{
    if (auto b = m_eraserButton.get()) { return b.as<winrt::InkToolbarEraserButton>(); }
    return nullptr;
}

winrt::InkToolbarRulerButton ButtonManager::GetRulerButton() const
{
    if (auto b = m_rulerButton.get()) { return b.as<winrt::InkToolbarRulerButton>(); }
    return nullptr;
}

winrt::InkToolbarStencilButton ButtonManager::GetStencilButton() const
{
    if (auto b = m_stencilButton.get()) { return b.as<winrt::InkToolbarStencilButton>(); }
    return nullptr;
}

winrt::InkToolbarToolButton ButtonManager::GetToolButton(winrt::InkToolbarTool tool) const
{
    switch (tool)
    {
    case winrt::InkToolbarTool::BallpointPen:
        if (auto b = m_ballpointPenButton.get()) { return b.as<winrt::InkToolbarToolButton>(); }
        break;
    case winrt::InkToolbarTool::Highlighter:
        if (auto b = m_highlighterButton.get()) { return b.as<winrt::InkToolbarToolButton>(); }
        break;
    case winrt::InkToolbarTool::Pencil:
        if (auto b = m_pencilButton.get()) { return b.as<winrt::InkToolbarToolButton>(); }
        break;
    case winrt::InkToolbarTool::Eraser:
        if (auto b = m_eraserButton.get()) { return b.as<winrt::InkToolbarToolButton>(); }
        break;
    default:
        break;
    }
    return nullptr;
}

winrt::InkToolbarToggleButton ButtonManager::GetToggleButton(winrt::InkToolbarToggle toggle) const
{
    switch (toggle)
    {
    case winrt::InkToolbarToggle::Ruler:
        if (auto b = m_rulerButton.get()) { return b.as<winrt::InkToolbarToggleButton>(); }
        break;
    default:
        break;
    }
    return nullptr;
}

winrt::InkToolbarMenuButton ButtonManager::GetMenuButton(winrt::InkToolbarMenuKind menu) const
{
    switch (menu)
    {
    case winrt::InkToolbarMenuKind::Stencil:
        if (auto b = m_stencilButton.get()) { return b.as<winrt::InkToolbarMenuButton>(); }
        break;
    default:
        break;
    }
    return nullptr;
}

bool ButtonManager::IsOneOfOurs(winrt::UIElement const& child) const
{
    bool found = false;
    ForEachButton([&](winrt::UIElement const& button)
    {
        if (button == child) { found = true; }
    });
    return found;
}

void ButtonManager::OnButtonRightTapped(winrt::IInspectable const& sender, winrt::RightTappedRoutedEventArgs const& args)
{
    // False == not 'normal' activation (not left-click).
    ButtonAction(sender, false);
    args.Handled(true);
}

void ButtonManager::OnButtonDoubleTapped(winrt::IInspectable const& sender, winrt::DoubleTappedRoutedEventArgs const& args)
{
    // Do not handle double tap because we also receive two click events. This messes up toggle buttons.
    UNREFERENCED_PARAMETER(sender);
    UNREFERENCED_PARAMETER(args);
}

void ButtonManager::OnButtonHolding(winrt::IInspectable const& sender, winrt::HoldingRoutedEventArgs const& args)
{
    // False == not 'normal' activation (not left-click).
    ButtonAction(sender, false);
    args.Handled(true);
}

void ButtonManager::OnButtonKeyDown(winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args)
{
    auto child = sender.as<winrt::UIElement>();
    auto key = args.Key();

    // Pass keystrokes other than Enter up to InkToolbar because ToggleButton doesn't respond to Enter.
    // Space triggers Toggle -> OnClick -> ButtonAction, so it's covered there; we intentionally ignore it here.
    if (key != winrt::Windows::System::VirtualKey::Enter)
    {
        winrt::get_self<InkToolbar>(ReferenceInkToolbar())->OnUnhandledButtonKeyPress(child, args);
        return;
    }

    // For InkToolbarToggleButtons, the Enter key is ignored by CheckBox, so we toggle manually here.
    bool shouldToggle = false;
    bool isChecked = false;
    winrt::Microsoft::UI::Xaml::Controls::Primitives::ToggleButton senderAsToggle{ nullptr };

    auto senderAsInkToolbarToggleButton = sender.try_as<winrt::InkToolbarToggleButton>();
    winrt::InkToolbarMenuButton senderAsInkToolbarMenuButton{ nullptr };

    if (senderAsInkToolbarToggleButton)
    {
        shouldToggle = true;
        isChecked = InkToolbar::IsButtonChecked(senderAsInkToolbarToggleButton);
        senderAsToggle = sender.as<winrt::Microsoft::UI::Xaml::Controls::Primitives::ToggleButton>();
    }
    else
    {
        // Toggle actions for InkToolbarMenuButtons are managed in ExecuteMenuAction. Don't do anything here.
        senderAsInkToolbarMenuButton = sender.try_as<winrt::InkToolbarMenuButton>();
        if (senderAsInkToolbarMenuButton)
        {
            shouldToggle = false;
        }
    }

    if (shouldToggle && senderAsToggle)
    {
        senderAsToggle.IsChecked(winrt::box_value(!isChecked).as<winrt::Windows::Foundation::IReference<bool>>());
    }

    // Pressing Enter on an InkToolbarMenuButton (a ToggleButton) triggers KeyDown AND Click; don't call
    // ButtonAction here for menu buttons as it will be called again on Click.
    if (!senderAsInkToolbarMenuButton)
    {
        // Enter/return always behave like left-click; pass true for "normal activation".
        ButtonAction(sender, true);
    }

    args.Handled(true);
}

void ButtonManager::OnButtonChecked(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(args);
    DispatchButtonCheckStateChanged(sender, true);
}

void ButtonManager::OnButtonUnchecked(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(args);
    DispatchButtonCheckStateChanged(sender, false);
}

void ButtonManager::OnButtonIndeterminate(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(args);
    // We don't support this state; if a button is in it, an app manipulated it. Treat as unchecked.
    auto child = sender.as<winrt::UIElement>();
    winrt::get_self<InkToolbar>(ReferenceInkToolbar())->OnCheckStateChanged(child, false);
}

void ButtonManager::OnButtonClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(args);
    // True == 'normal' activation (left-click).
    ButtonAction(sender, true);
}

void ButtonManager::ButtonAction(winrt::IInspectable const& sender, bool isNormalActivation)
{
    if (auto toolButton = sender.try_as<winrt::InkToolbarToolButton>())
    {
        winrt::get_self<InkToolbar>(ReferenceInkToolbar())->ExecuteToolAction(toolButton, isNormalActivation);
        return;
    }

    if (auto toggleButton = sender.try_as<winrt::InkToolbarToggleButton>())
    {
        winrt::get_self<InkToolbar>(ReferenceInkToolbar())->ExecuteToggleAction(toggleButton);
        return;
    }

    if (auto menuButton = sender.try_as<winrt::InkToolbarMenuButton>())
    {
        winrt::get_self<InkToolbar>(ReferenceInkToolbar())->ExecuteMenuAction(menuButton, isNormalActivation);
        return;
    }
}

// The checked event is delivered *before* the pointer event that caused it, which would make the pointer
// event appear to land on an already-selected button (opening its L3). So we post to the dispatcher to
// run *after* pending pointer events.
void ButtonManager::DispatchButtonCheckStateChanged(winrt::IInspectable const& sender, bool check)
{
    auto child = sender.as<winrt::UIElement>();
    auto toolbar = ReferenceInkToolbar();

    if (auto dispatcherQueue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread())
    {
        dispatcherQueue.TryEnqueue([toolbar, child, check]()
        {
            winrt::get_self<InkToolbar>(toolbar)->OnCheckStateChanged(child, check);
        });
    }
}

void ButtonManager::ForEachButton(std::function<void(winrt::UIElement const&)> function) const
{
    if (auto b = m_ballpointPenButton.get()) { function(b); }
    if (auto b = m_pencilButton.get()) { function(b); }
    if (auto b = m_highlighterButton.get()) { function(b); }
    if (auto b = m_eraserButton.get()) { function(b); }
    if (auto b = m_rulerButton.get()) { function(b); }
    if (auto b = m_stencilButton.get()) { function(b); }

    for (auto const& weak : m_customPenButtons) { if (auto b = weak.get()) { function(b); } }
    for (auto const& weak : m_customToolButtons) { if (auto b = weak.get()) { function(b); } }
    for (auto const& weak : m_customToggleButtons) { if (auto b = weak.get()) { function(b); } }
}

void ButtonManager::ForEachToolButton(std::function<void(winrt::InkToolbarToolButton const&)> function) const
{
    if (auto b = m_ballpointPenButton.get()) { function(b.as<winrt::InkToolbarToolButton>()); }
    if (auto b = m_pencilButton.get()) { function(b.as<winrt::InkToolbarToolButton>()); }
    if (auto b = m_highlighterButton.get()) { function(b.as<winrt::InkToolbarToolButton>()); }
    if (auto b = m_eraserButton.get()) { function(b.as<winrt::InkToolbarToolButton>()); }
    for (auto const& weak : m_customPenButtons) { if (auto b = weak.get()) { function(b.as<winrt::InkToolbarToolButton>()); } }
    for (auto const& weak : m_customToolButtons) { if (auto b = weak.get()) { function(b.as<winrt::InkToolbarToolButton>()); } }
}

void ButtonManager::ForEachToggleButton(std::function<void(winrt::InkToolbarToggleButton const&)> function) const
{
    if (auto b = m_rulerButton.get()) { function(b.as<winrt::InkToolbarToggleButton>()); }
    for (auto const& weak : m_customToggleButtons) { if (auto b = weak.get()) { function(b.as<winrt::InkToolbarToggleButton>()); } }
}

void ButtonManager::ForEachMenuButton(std::function<void(winrt::InkToolbarMenuButton const&)> function) const
{
    if (auto b = m_stencilButton.get()) { function(b.as<winrt::InkToolbarMenuButton>()); }
}
