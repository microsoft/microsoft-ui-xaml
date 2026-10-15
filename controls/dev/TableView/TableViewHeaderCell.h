// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableViewColumnHeaderAutomationPeer.h"
#include "TableViewCell.h"

// Keep the existing Grid layout/input behavior, but attach the header peer to the
// actual focus and hit-test target. This implementation-only type needs no WinRT API.
class TableViewHeaderCell :
    public ReferenceTracker<TableViewHeaderCell, winrt::Microsoft::UI::Xaml::Controls::GridT,
        winrt::composable, ITableViewCellAutomationPeerAccess>
{
public:
    TableViewHeaderCell(winrt::TableView const& table, winrt::TableViewColumn const& column)
        : m_table(winrt::make_weak(table)), m_column(winrt::make_weak(column))
    {
        // A Grid has no pointer overrides. Never marks the events handled, so Tapped-to-sort and
        // focus are unaffected.
        PointerEntered({ this, &TableViewHeaderCell::OnPointerEntered });
        PointerExited({ this, &TableViewHeaderCell::OnPointerExited });
        PointerPressed({ this, &TableViewHeaderCell::OnPointerPressed });
        PointerReleased({ this, &TableViewHeaderCell::OnPointerReleased });
        PointerCaptureLost({ this, &TableViewHeaderCell::OnPointerLost });
        PointerCanceled({ this, &TableViewHeaderCell::OnPointerLost });
        // Theme brushes are resolved per state, so a theme change while hovered must re-resolve them.
        ActualThemeChanged({ this, &TableViewHeaderCell::OnActualThemeChanged });
    }

    winrt::hstring GetRuntimeClassName() const
    {
        return winrt::hstring_name_of<winrt::Grid>();
    }

    winrt::AutomationPeer OnCreateAutomationPeer()
    {
        if (auto const peer = m_automationPeer.get())
        {
            return peer;
        }
        winrt::AutomationPeer const peer = winrt::make<TableViewColumnHeaderAutomationPeer>(
            get_strong().as<winrt::FrameworkElement>(), m_table.get(), m_column.get());
        m_automationPeer = winrt::make_weak(peer);
        return peer;
    }

    // Read through TableViewCell::TryGetExistingPeer; never creates a peer.
    HRESULT __stdcall GetExistingPeer(::IInspectable** result) noexcept
    {
        if (!result)
        {
            return E_POINTER;
        }
        *result = nullptr;
        try
        {
            CheckThread();
            auto peer = m_automationPeer.get();
            *result = reinterpret_cast<::IInspectable*>(winrt::detach_abi(peer));
            return S_OK;
        }
        catch (...)
        {
            return winrt::to_hresult();
        }
    }

private:
    enum class PointerVisualState
    {
        Normal,
        PointerOver,
        Pressed,
    };

    void OnPointerEntered(const winrt::IInspectable&, const winrt::PointerRoutedEventArgs&)
    {
        m_isPointerOver = true;
        UpdatePointerVisualState();
    }

    void OnPointerExited(const winrt::IInspectable&, const winrt::PointerRoutedEventArgs&)
    {
        m_isPointerOver = false;
        m_isPressed = false;
        UpdatePointerVisualState();
    }

    void OnPointerPressed(const winrt::IInspectable&, const winrt::PointerRoutedEventArgs&)
    {
        m_isPressed = true;
        UpdatePointerVisualState();
    }

    void OnPointerReleased(const winrt::IInspectable&, const winrt::PointerRoutedEventArgs&)
    {
        m_isPressed = false;
        UpdatePointerVisualState();
    }

    void OnPointerLost(const winrt::IInspectable&, const winrt::PointerRoutedEventArgs& args)
    {
        // The resize gripper captures the pointer for its drag; that capture ending bubbles here
        // while the pointer is still over the header, so it must not reset the hover state.
        if (IsInsideResizeGripper(args.OriginalSource()))
        {
            return;
        }

        m_isPointerOver = false;
        m_isPressed = false;
        UpdatePointerVisualState();
    }

    void OnActualThemeChanged(const winrt::FrameworkElement&, const winrt::IInspectable&)
    {
        UpdatePointerVisualState(true /* force */);
    }

    bool IsInsideResizeGripper(const winrt::IInspectable& source)
    {
        winrt::Panel const self = *this;
        for (auto current = source.try_as<winrt::DependencyObject>();
             current && current != self;
             current = winrt::VisualTreeHelper::GetParent(current))
        {
            if (current.try_as<winrt::ResizeGripper>())
            {
                return true;
            }
        }
        return false;
    }

    bool IsSortable() const
    {
        auto const table = m_table.get();
        auto const column = m_column.get();
        return table && column && table.CanUserSortColumns() && column.CanSort();
    }

    void UpdatePointerVisualState(bool force = false)
    {
        auto state = PointerVisualState::Normal;
        if (IsSortable())
        {
            state = m_isPressed ? PointerVisualState::Pressed
                : m_isPointerOver ? PointerVisualState::PointerOver
                : PointerVisualState::Normal;
        }

        if (state == m_visualState && !force)
        {
            return;
        }

        m_visualState = state;

        winrt::ContentPresenter content{ nullptr };
        if (Children().Size() > 0)
        {
            content = Children().GetAt(0).try_as<winrt::ContentPresenter>();
        }

        auto const indicator = TableView::FindSortIndicator(*this);

        auto const table = m_table.get();
        if (state == PointerVisualState::Normal || !table)
        {
            Background(winrt::SolidColorBrush{ winrt::Colors::Transparent() });
            ApplyStateForeground(content, indicator, nullptr);
            return;
        }

        auto const tableImpl = winrt::get_self<TableView>(table);
        const bool pressed = state == PointerVisualState::Pressed;
        // A null brush would make the cell stop taking pointer input.
        winrt::Brush background = tableImpl->LookupHeaderStateBrush(pressed
            ? L"TabularSurfaceHeaderBackgroundPressedBrush"
            : L"TabularSurfaceHeaderBackgroundPointerOverBrush");
        if (!background)
        {
            background = winrt::SolidColorBrush{ winrt::Colors::Transparent() };
        }
        Background(background);

        winrt::Brush foreground{ nullptr };
        if (tableImpl->IsHighContrast())
        {
            foreground = tableImpl->LookupHeaderStateBrush(pressed
                ? L"TabularSurfaceHeaderForegroundPressedBrush"
                : L"TabularSurfaceHeaderForegroundPointerOverBrush");
        }
        ApplyStateForeground(content, indicator, foreground);
    }

    // The chevron needs the same HC foreground as the text, or it vanishes on the highlight fill. The
    // text backplate is turned off with it, as the row and group header templates do, so it does not
    // fight the highlight fill.
    static void ApplyStateForeground(
        winrt::ContentPresenter const& content,
        winrt::SortIndicator const& indicator,
        winrt::Brush const& foreground)
    {
        if (content)
        {
            if (foreground)
            {
                content.Foreground(foreground);
                content.HighContrastAdjustment(winrt::ElementHighContrastAdjustment::None);
            }
            else
            {
                content.ClearValue(winrt::ContentPresenter::ForegroundProperty());
                content.ClearValue(winrt::UIElement::HighContrastAdjustmentProperty());
            }
        }
        if (indicator)
        {
            if (foreground)
            {
                indicator.Foreground(foreground);
                indicator.HighContrastAdjustment(winrt::ElementHighContrastAdjustment::None);
            }
            else
            {
                indicator.ClearValue(winrt::Control::ForegroundProperty());
                indicator.ClearValue(winrt::UIElement::HighContrastAdjustmentProperty());
            }
        }
    }

    winrt::weak_ref<winrt::TableView> m_table;
    winrt::weak_ref<winrt::TableViewColumn> m_column;
    winrt::weak_ref<winrt::AutomationPeer> m_automationPeer{ nullptr };
    PointerVisualState m_visualState{ PointerVisualState::Normal };
    bool m_isPointerOver{ false };
    bool m_isPressed{ false };
};
