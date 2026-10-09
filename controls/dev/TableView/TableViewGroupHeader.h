// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "TableViewGroupHeader.g.h"
#include "TableViewGroupHeader.properties.h"

// Templated group-header band. See TableViewGroupHeader.idl for the part / visual-state
// contract.
//
// A ContentControl rather than a bare Control, so Content (the TableViewGroupInfo projection)
// and ContentTemplate are inherited rather than reinvented. Chrome and chevron live in the
// ControlTemplate, so an app ContentTemplate fills only the content region. ContentTemplate
// always resolves, so there is no default-vs-custom rendering fork.
//
// The whole band is the toggle target -- no inner Button -- keeping one interactive element
// and one automation story.
class TableViewGroupHeader :
    public ReferenceTracker<TableViewGroupHeader, winrt::implementation::TableViewGroupHeaderT>,
    public TableViewGroupHeaderProperties
{
public:
    TableViewGroupHeader();

    // IFrameworkElement overrides
    void OnApplyTemplate();

    // The band spans every visible column. Computed HERE, during measure, rather than pushed in
    // as an explicit Width: an assigned width is only as fresh as the moment it was assigned, and
    // during a grow the column ActualWidths are not final when column resolve runs -- the band
    // came out short and, because it then had a fixed width inside a stretch slot, centred itself
    // with a gap at both ends. Measuring is self-timing and cannot go stale.
    winrt::Size MeasureOverride(winrt::Size const& availableSize);

    // IUIElement overrides
    winrt::AutomationPeer OnCreateAutomationPeer();

    // A freshly assigned projection must pick up current expansion immediately.
    void OnContentChanged(winrt::IInspectable const& oldContent, winrt::IInspectable const& newContent);

    // Use the Control virtual signatures; event-handler overloads break the ABI shim.
    void OnPointerEntered(winrt::PointerRoutedEventArgs const& args);
    void OnPointerExited(winrt::PointerRoutedEventArgs const& args);
    void OnPointerPressed(winrt::PointerRoutedEventArgs const& args);
    void OnPointerReleased(winrt::PointerRoutedEventArgs const& args);
    void OnPointerCaptureLost(winrt::PointerRoutedEventArgs const& args);

    // Keyboard parity with the pointer toggle: a focused group header expands/collapses from the
    // keyboard, matching TreeViewItem / Expander. Enter/Space toggles; Right/Left expand/collapse
    // (mirrored under RTL). Without this the band is pointer-only -- a WCAG 2.1.1 failure on the
    // headline grouping feature.
    void OnKeyDown(winrt::KeyRoutedEventArgs const& args);

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    // Internal: raise ToggleRequested for the band gesture. Note the ExpandCollapse peer does
    // NOT come through here -- it calls TableView::SetGroupExpansion directly, because the
    // pattern is directional and this is not.
    void RequestToggle();

    // Internal: directional keyboard expand/collapse. Routes through the owner exactly like the
    // ExpandCollapse peer (idempotent, resolved through the stored owner, not an ancestor walk).
    void RequestExpansion(bool expand);

    // Internal: announced by TableView::PrepareGroupHeaderElement, which is the only place that
    // knows whether the incoming state belongs to the SAME group this container last showed.
    void RaiseExpandCollapseStateChanged(winrt::ExpandCollapseState oldState, winrt::ExpandCollapseState newState);

    // Internal: set by TableView::PrepareGroupHeaderElement. The automation peer resolves its
    // TableView through this rather than walking the visual tree -- an AT client holding a
    // provider across a scroll can call Expand() on a header that has been unparented, and an
    // ancestor walk from a detached element silently finds nothing and no-ops.
    void SetOwningTableViewInternal(winrt::TableView const& owner);
    winrt::TableView GetOwningTableView() const;
    void SetTerminalBottomGridLineSuppression(bool suppress);

    // Internal: single-subscription guard for the ToggleRequested handler.
    // PrepareGroupHeaderElement sets this once so repeated prepares are no-ops.
    bool IsToggleHooked() const { return m_toggleHooked; }
    void SetToggleHooked() { m_toggleHooked = true; }

    // Internal: called by TableView when this pooled container is prepared for or cleared from a
    // group, so a press that began on the previous group can never complete on the next one.
    void ResetPointerStateInternal();

    // Internal: the input that triggered the expansion request in flight (Keyboard for a key,
    // Pointer for a click), consumed once by TableView::RequestGroupExpansion so the focus restore
    // after the reshape uses the gesture's state rather than whatever state focus arrived with.
    void SetGestureFocusStateInternal(winrt::FocusState state) noexcept { m_gestureFocusState = state; }
    winrt::FocusState TakeGestureFocusStateInternal() noexcept
    {
        return std::exchange(m_gestureFocusState, winrt::FocusState::Unfocused);
    }

private:
    void UpdateVisualStates(bool useTransitions);
    void UpdateTerminalBottomGridLineSuppression();

    // Identity of the group this container shows now (value-based, index independent), or empty.
    winrt::hstring GetGroupIdentity();

    // Mirror the authoritative IsExpandable/IsExpanded DPs onto the bound projection.
    void SyncExpansionToContent();

    winrt::Control::IsEnabledChanged_revoker m_isEnabledChangedRevoker{};
    tracker_ref<winrt::Border> m_gridLineBorder{ this };

    bool m_isPointerOver{ false };
    bool m_isPressed{ false };
    bool m_suppressBottomGridLine{ false };
    // Group identity captured on press; the release toggles only if the container still shows it.
    winrt::hstring m_pressedGroupIdentity{};
    winrt::FocusState m_gestureFocusState{ winrt::FocusState::Unfocused };

    winrt::weak_ref<winrt::TableView> m_owningTableView{ nullptr };

    // Set to true by PrepareGroupHeaderElement the first time ToggleRequested is subscribed.
    // Guards against double-hooking when the same container is prepared many times after pool
    // recycling: one subscription per container lifetime, not per prepare.
    bool m_toggleHooked{ false };
};
