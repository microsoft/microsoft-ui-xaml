// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewColumn.h"
#include "TableViewRow.h"
#include "TableViewCellsPanel.h"
#include "TableViewAutomationPeer.h"
#include "RuntimeProfiler.h"
#include "TVDiag.h"

#include <string>
#include <cmath>

static constexpr std::wstring_view s_RowsRepeaterPartName{ L"PART_RowsRepeater"sv };
static constexpr std::wstring_view s_HeaderRowPartName{ L"PART_HeaderRow"sv };
static constexpr std::wstring_view s_HeaderHostPartName{ L"PART_HeaderHost"sv };
static constexpr std::wstring_view s_EmptyStatePresenterPartName{ L"PART_EmptyStatePresenter"sv };
static constexpr std::wstring_view s_HeaderGridLineName{ L"TableViewHeaderGridLine"sv };
// ScrollViewer template names are documented; ancestors are resolved by walking from child parts.

namespace
{
    winrt::ScrollViewer FindScrollViewerAncestor(winrt::DependencyObject const& start)
    {
        winrt::DependencyObject node = start;
        while (node)
        {
            if (auto sv = node.try_as<winrt::ScrollViewer>())
            {
                return sv;
            }
            node = winrt::VisualTreeHelper::GetParent(node);
        }
        return nullptr;
    }

    winrt::IInspectable LookupInThemeDictionaries(
        winrt::ResourceDictionary const& dict, winrt::hstring const& themeKey, winrt::IInspectable const& boxedKey)
    {
        if (!dict)
        {
            return nullptr;
        }
        if (auto themeDicts = dict.ThemeDictionaries())
        {
            const auto boxedThemeKey = winrt::box_value(themeKey);
            if (themeDicts.HasKey(boxedThemeKey))
            {
                if (auto themed = themeDicts.Lookup(boxedThemeKey).try_as<winrt::ResourceDictionary>())
                {
                    if (auto v = themed.TryLookup(boxedKey))
                    {
                        return v;
                    }
                }
            }
        }
        if (auto merged = dict.MergedDictionaries())
        {
            for (uint32_t i = merged.Size(); i-- > 0;)
            {
                if (auto v = LookupInThemeDictionaries(merged.GetAt(i), themeKey, boxedKey))
                {
                    return v;
                }
            }
        }
        return nullptr;
    }

    winrt::IInspectable LookupElementResource(winrt::FrameworkElement const& start, std::wstring_view key, bool highContrast = false)
    {
        const auto boxedKey = winrt::box_value(winrt::hstring{ key });
        // Theme-scoped resources must resolve against the element's ActualTheme.
        const auto theme = start ? start.ActualTheme() : winrt::ElementTheme::Default;
        // High Contrast is orthogonal to ActualTheme; callers pass cached HC state for hot-path brush lookups.
        const winrt::hstring themeKey{
            highContrast ? L"HighContrast" : (theme == winrt::ElementTheme::Light ? L"Light" : L"Default") };

        winrt::FrameworkElement walker = start;
        while (walker)
        {
            if (auto resources = walker.Resources())
            {
                if (auto found = LookupInThemeDictionaries(resources, themeKey, boxedKey))
                {
                    return found;
                }
                if (auto found = resources.TryLookup(boxedKey))
                {
                    return found;
                }
            }
            walker = walker.Parent().try_as<winrt::FrameworkElement>();
        }

        if (auto app = winrt::Application::Current())
        {
            if (auto resources = app.Resources())
            {
                if (auto found = LookupInThemeDictionaries(resources, themeKey, boxedKey))
                {
                    return found;
                }
                return resources.TryLookup(boxedKey);
            }
        }
        return nullptr;
    }

    constexpr winrt::Thickness s_zeroThickness{ 0, 0, 0, 0 };

    bool WantsHorizontalLines(winrt::TableViewGridLinesVisibility visibility) noexcept
    {
        return visibility == winrt::TableViewGridLinesVisibility::Horizontal ||
            visibility == winrt::TableViewGridLinesVisibility::All;
    }

    bool WantsVerticalLines(winrt::TableViewGridLinesVisibility visibility) noexcept
    {
        return visibility == winrt::TableViewGridLinesVisibility::Vertical ||
            visibility == winrt::TableViewGridLinesVisibility::All;
    }

    TableViewResourceCache& GetTableViewResourceCache(TableView* owner)
    {
        // Per-instance member (not a process-global map) so multi-UI-thread instances never share state.
        return owner->GetResourceCacheInternal();
    }

    void InvalidateTableViewResourceCache(TableView* owner)
    {
        auto& cache = owner->GetResourceCacheInternal();
        cache.density.hasRowMinHeight = false;
        cache.density.hasCellPadding = false;
        cache.density.hasHeaderCellPadding = false;
        cache.font.hasCellFontSize = false;
        cache.font.hasHeaderFontSize = false;
        cache.gridLine.hasBrush = false;
    }

    bool ShouldRefreshFrozenColumnsForScroll(TableView* owner, double horizontalOffset)
    {
        auto& cache = GetTableViewResourceCache(owner);
        if (!cache.hasLastFrozenColumnsHorizontalOffset ||
            std::abs(cache.lastFrozenColumnsHorizontalOffset - horizontalOffset) >= 0.5)
        {
            cache.hasLastFrozenColumnsHorizontalOffset = true;
            cache.lastFrozenColumnsHorizontalOffset = horizontalOffset;
            return true;
        }
        return false;
    }

    double DensityRowMinHeightFallback(winrt::TableViewDensity density)
    {
        switch (density)
        {
        case winrt::TableViewDensity::Compact: return 30.0;
        case winrt::TableViewDensity::Comfortable: return 48.0;
        default: return 40.0; // Standard
        }
    }

    winrt::Thickness DensityCellPaddingFallback(winrt::TableViewDensity density)
    {
        switch (density)
        {
        case winrt::TableViewDensity::Compact: return winrt::ThicknessHelper::FromLengths(8, 2, 8, 2);
        case winrt::TableViewDensity::Comfortable: return winrt::ThicknessHelper::FromLengths(8, 8, 8, 8);
        default: return winrt::ThicknessHelper::FromLengths(8, 4, 8, 4); // Standard
        }
    }

    winrt::Brush CreateGridLineFallbackBrush(winrt::FrameworkElement const& start, bool highContrast)
    {
        auto color = highContrast
            ? winrt::Colors::White()
            : (start.ActualTheme() == winrt::ElementTheme::Light
                ? winrt::ColorHelper::FromArgb(0x29, 0x00, 0x00, 0x00)
                : winrt::ColorHelper::FromArgb(0x29, 0xff, 0xff, 0xff));

        if (highContrast)
        {
            color = winrt::unbox_value_or<winrt::Color>(
                LookupElementResource(start, L"SystemColorWindowTextColor", true),
                color);
        }

        return winrt::SolidColorBrush(color);
    }

}

winrt::Brush TableView::GetGridLineBrush()
{
    auto& cache = GetTableViewResourceCache(this);
    const bool highContrast = IsHighContrast();
    const auto theme = ActualTheme();
    if (cache.gridLine.hasBrush &&
        cache.gridLine.theme == theme &&
        cache.gridLine.highContrast == highContrast)
    {
        return cache.gridLine.brush;
    }

    auto brush = LookupElementResource(*this, L"TabularSurfaceGridLineBrush", highContrast).try_as<winrt::Brush>();
    if (!brush)
    {
        brush = CreateGridLineFallbackBrush(*this, highContrast);
    }

    cache.gridLine.hasBrush = true;
    cache.gridLine.theme = theme;
    cache.gridLine.highContrast = highContrast;
    cache.gridLine.brush = brush;
    return brush;
}

TableView::TableView()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_TableView);

    SetDefaultStyleKey(this);

    // Columns must be observable; OnColumnsPropertyChanged owns the VectorChanged subscription to avoid duplicate callbacks.
    auto columns = winrt::single_threaded_observable_vector<winrt::TableViewColumn>();
    Columns(columns);

    auto weakThis = get_weak();

    // Use bubbling KeyDown so focused editors can consume typing keys before row navigation.
    m_keyDownHandler = winrt::KeyEventHandler(
        [weakThis](winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->OnKeyDownForNavigation(sender, args);
            }
        });
    // Ancestor PART_BodyScroller marks nav keys Handled before they bubble here; handledEventsToo:true lets us still act on them.
    // AddHandler takes the handler as IInspectable, so the delegate must be boxed (see RoutedEventHelpers.h).
    AddHandler(winrt::UIElement::KeyDownEvent(), winrt::box_value(m_keyDownHandler), true /* handledEventsToo */);

    // Tunneling PreviewKeyDown runs before the framework's built-in focus navigation; snapshot the
    // currently focused row there so OnKeyDownForNavigation anchors on the pre-move index.
    m_previewKeyDownHandler = winrt::KeyEventHandler(
        [weakThis](winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->OnPreviewKeyDownForNavigation(sender, args);
            }
        });
    AddHandler(winrt::UIElement::PreviewKeyDownEvent(), winrt::box_value(m_previewKeyDownHandler), false /* handledEventsToo */);

    // Editing gestures. Separate from the navigation handlers above; the key sets are disjoint.
    // handledEventsToo is required because a single-line TextBox marks Enter handled and the commit
    // must still run - each key case in OnKeyDownForEditing owns its own Handled policy.
    m_editingKeyDownHandler = winrt::KeyEventHandler(
        [weakThis](winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->OnKeyDownForEditing(sender, args);
            }
        });
    AddHandler(winrt::UIElement::KeyDownEvent(), winrt::box_value(m_editingKeyDownHandler), true /* handledEventsToo */);

    // Commit when focus leaves the open editor. Uses the typed LosingFocus event so the incoming
    // focus target is known - a plain LostFocus cannot tell "moved inside the editor" from
    // "clicked another cell".
    m_editingLosingFocusRevoker = LosingFocus(winrt::auto_revoke,
        [weakThis](winrt::UIElement const& sender, winrt::Microsoft::UI::Xaml::Input::LosingFocusEventArgs const& args)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->OnLosingFocusForEditing(sender, args);
            }
        });

    // ThemeSettings needs a WindowId, so it can only be created once we are in a tree with a XamlRoot.
    // Create it (and subscribe to Changed) on Loaded; its Changed event is raised on this UI thread,
    // which is why the old AccessibilitySettings dispatcher-marshaling plumbing is gone.
    m_loadedRevoker = Loaded(winrt::auto_revoke,
        [weakThis](winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->OnTableViewLoaded(sender, args);
            }
        });

    // Null ItemsSource on unload so queued repeater work cannot run on a detached subtree.
    m_unloadedRevoker = Unloaded(winrt::auto_revoke,
        [weakThis](winrt::IInspectable const&, winrt::RoutedEventArgs const&)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->OnTableViewUnloaded();
            }
        });
}

void TableView::OnTableViewLoaded(const winrt::IInspectable& /*sender*/, const winrt::RoutedEventArgs& /*args*/)
{
    // ThemeSettings requires a WindowId, so it can only be created once we have a XamlRoot.
    if (m_themeSettings)
    {
        return; // already created for this hosting session
    }

    auto xamlRoot = XamlRoot();
    if (!xamlRoot)
    {
        return;
    }

    try
    {
        // ContentIslandEnvironment can be null during teardown / unusual hosts.
        if (auto env = xamlRoot.ContentIslandEnvironment())
        {
            m_themeSettings = winrt::Microsoft::UI::System::ThemeSettings::CreateForWindowId(env.AppWindowId());
            m_isHighContrast = m_themeSettings.HighContrast();
            // Changed is raised on this UI thread, so the handler can touch XAML directly.
            m_themeSettingsChangedRevoker = m_themeSettings.Changed(
                winrt::auto_revoke, { get_weak(), &TableView::OnThemeSettingsChanged });
        }
    }
    catch (...)
    {
        // Best-effort; IsHighContrast falls back to a one-shot AccessibilitySettings read.
    }
}

void TableView::OnThemeSettingsChanged(
    const winrt::Microsoft::UI::System::ThemeSettings& sender, const winrt::IInspectable& /*args*/)
{
    // Raised on the control's UI thread (no marshaling required). HC can toggle without a theme
    // change, so invalidate the HC-dependent resource cache and refresh realized visuals directly.
    try
    {
        m_isHighContrast = sender.HighContrast();
    }
    catch (...)
    {
    }

    InvalidateTableViewResourceCache(this);
    if (IsLoaded())
    {
        RebuildHeaders();
        RefreshGridLinesOnRealizedRows();
    }
}

void TableView::OnApplyTemplate()
{
    __super::OnApplyTemplate();
    InvalidateTableViewResourceCache(this);

    // Detach old wiring
    if (m_pendingFocusLayoutToken.value)
    {
        LayoutUpdated(m_pendingFocusLayoutToken);
        m_pendingFocusLayoutToken = {};
    }
    if (auto oldRepeater = m_rowsRepeater.get())
    {
        // Drop per-template Loaded handlers so old elements cannot keep this alive.
        if (m_rowsRepeaterLoadedToken.value)
        {
            if (auto oldRepeaterFE = oldRepeater.try_as<winrt::FrameworkElement>())
            {
                oldRepeaterFE.Loaded(m_rowsRepeaterLoadedToken);
            }
            m_rowsRepeaterLoadedToken = {};
        }

        // Release realized rows before detaching ElementClearing so rows can clear their owner.
        try { oldRepeater.ItemsSource(nullptr); } catch (...) {}

        oldRepeater.ElementPrepared(m_rowElementPreparedToken);
        oldRepeater.ElementClearing(m_rowElementClearingToken);
        oldRepeater.ElementIndexChanged(m_rowElementIndexChangedToken);
        m_rowElementPreparedToken = {};
        m_rowElementClearingToken = {};
        m_rowElementIndexChangedToken = {};
    }
    if (auto oldHeaderHost = m_headerHost.get())
    {
        // Mirror the rowsRepeater Loaded cleanup for the header host.
        if (m_headerHostLoadedToken.value)
        {
            if (auto oldHeaderHostFE = oldHeaderHost.try_as<winrt::FrameworkElement>())
            {
                oldHeaderHostFE.Loaded(m_headerHostLoadedToken);
            }
            m_headerHostLoadedToken = {};
        }
    }
    if (auto oldBodyScroller = m_bodyScroller.get())
    {
        oldBodyScroller.ViewChanged(m_bodyScrollerViewChangedToken);
        m_bodyScrollerViewChangedToken = {};
        m_bodyScrollerSizeChangedRevoker.revoke();
    }

    // Reset resolved-on-Loaded refs so re-templating re-resolves them against the new tree.
    m_headerRow.set(nullptr);
    m_headerScroller.set(nullptr);
    m_bodyScroller.set(nullptr);

    m_rowsRepeater.set(GetTemplateChild(hstring{ s_RowsRepeaterPartName }).try_as<winrt::ItemsRepeater>());
    m_headerRow.set(GetTemplateChild(hstring{ s_HeaderRowPartName }).try_as<winrt::FrameworkElement>());
    m_headerHost.set(GetTemplateChild(hstring{ s_HeaderHostPartName }).try_as<winrt::Panel>());
    m_emptyStatePresenter.set(GetTemplateChild(hstring{ s_EmptyStatePresenterPartName }).try_as<winrt::ContentControl>());
    auto weakThis = get_weak();

    // Drive the repeater from the flat ItemsSource DP once the template is alive.
    UpdateRowsItemsSource();

    // Defer ScrollViewer ancestor lookup until Loaded because template parts are not fully connected here.
    if (auto headerHost = m_headerHost.get())
    {
        if (auto headerHostFE = headerHost.try_as<winrt::FrameworkElement>())
        {
            m_headerHostLoadedToken = headerHostFE.Loaded(
                [weakThis](winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
                {
                    if (auto strongThis = weakThis.get())
                    {
                        strongThis->OnHeaderHostLoaded(sender, args);
                    }
                });
        }
    }

    if (auto repeater = m_rowsRepeater.get())
    {
        m_rowElementPreparedToken = repeater.ElementPrepared(
            [weakThis](winrt::ItemsRepeater const& sender, winrt::ItemsRepeaterElementPreparedEventArgs const& args)
            {
                if (auto strongThis = weakThis.get())
                {
                    strongThis->OnRowElementPrepared(sender, args);
                }
            });
        m_rowElementClearingToken = repeater.ElementClearing(
            [weakThis](winrt::ItemsRepeater const& sender, winrt::ItemsRepeaterElementClearingEventArgs const& args)
            {
                if (auto strongThis = weakThis.get())
                {
                    strongThis->OnRowElementClearing(sender, args);
                }
            });
        m_rowElementIndexChangedToken = repeater.ElementIndexChanged(
            [weakThis](winrt::ItemsRepeater const& sender, winrt::ItemsRepeaterElementIndexChangedEventArgs const& args)
            {
                if (auto strongThis = weakThis.get())
                {
                    strongThis->OnRowElementIndexChanged(sender, args);
                }
            });

        if (auto repeaterFE = repeater.try_as<winrt::FrameworkElement>())
        {
            m_rowsRepeaterLoadedToken = repeaterFE.Loaded(
                [weakThis](winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
                {
                    if (auto strongThis = weakThis.get())
                    {
                        strongThis->OnRowsRepeaterLoaded(sender, args);
                    }
                });
        }
    }

    // Body horizontal scrolling drives the header ScrollViewer; vertical stickiness is structural.

    RebuildHeaders();
    UpdateHeaderVisibility();

    // Theme switches require refreshing imperatively-resolved grid-line brushes.
    if (m_actualThemeChangedToken.value)
    {
        try { this->ActualThemeChanged(m_actualThemeChangedToken); } catch (...) {}
        m_actualThemeChangedToken = {};
    }

    {
        auto weakThis2 = get_weak();
        m_actualThemeChangedToken = this->ActualThemeChanged(
            [weakThis2](winrt::FrameworkElement const& /*sender*/, winrt::IInspectable const& /*args*/)
            {
                auto strongThis = weakThis2.get();
                if (!strongThis)
                {
                    return;
                }
                if (!strongThis->IsLoaded())
                {
                    return;
                }
                try
                {
                    InvalidateTableViewResourceCache(strongThis.get());
                    // Rebuild headers and realized rows so grid-line brushes re-resolve.
                    strongThis->RebuildHeaders();
                    strongThis->RefreshGridLinesOnRealizedRows();
                }
                catch (...)
                {
                    // Theme-switch refresh is best-effort.
                }
            });
    }
}

void TableView::OnHeaderHostLoaded(const winrt::IInspectable& /*sender*/, const winrt::RoutedEventArgs& /*args*/)
{
    if (m_headerScroller.get())
    {
        return; // already resolved
    }
    if (auto headerHost = m_headerHost.get())
    {
        winrt::ScrollViewer scroller{ nullptr };
        try { scroller = FindScrollViewerAncestor(headerHost); } catch (...) {}
        m_headerScroller.set(scroller);
        // Header pans can transiently desync; reverse-sync can clamp when header/body extents differ.
        UpdateHeaderVisibility();
    }
}

void TableView::OnRowsRepeaterLoaded(const winrt::IInspectable& /*sender*/, const winrt::RoutedEventArgs& /*args*/)
{
    if (m_rowsSourceDrained)
    {
        // Only re-source cached pages after Unloaded actually drained the repeater.
        m_rowsSourceDrained = false;
        UpdateRowsItemsSource();
    }

    if (m_bodyScroller.get())
    {
        return; // already resolved
    }
    if (auto repeater = m_rowsRepeater.get())
    {
        winrt::ScrollViewer scroller{ nullptr };
        try { scroller = FindScrollViewerAncestor(repeater); } catch (...) {}
        m_bodyScroller.set(scroller);
        if (auto bodyScroller = m_bodyScroller.get())
        {
            auto weakThis = get_weak();
            m_bodyScrollerViewChangedToken = bodyScroller.ViewChanged(
                [weakThis](winrt::IInspectable const& sender, winrt::ScrollViewerViewChangedEventArgs const& args)
                {
                    if (auto strongThis = weakThis.get())
                    {
                        strongThis->OnBodyScrollerViewChanged(sender, args);
                    }
                });

            // Viewport resize (ViewChanged only covers scroll/zoom) must rerun table-level measure
            // so Star widths resolve after the subtree has refreshed its measured-width caches.
            m_bodyScrollerSizeChangedRevoker = bodyScroller.SizeChanged(winrt::auto_revoke,
                [weakThis](winrt::IInspectable const& /*sender*/, winrt::SizeChangedEventArgs const& /*args*/)
                {
                    if (auto strongThis = weakThis.get())
                    {
                        strongThis->InvalidateMeasure();
                        strongThis->RefreshFrozenColumns();
                    }
                });

            // Resolve during the next table measure now that the viewport is known (initial layout).
            InvalidateMeasure();
            RefreshFrozenColumns();
        }
    }
}

void TableView::OnBodyScrollerViewChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollViewerViewChangedEventArgs& /*args*/)
{
    auto bodyScroller = m_bodyScroller.get();
    if (!bodyScroller)
    {
        return;
    }

    const double bodyHOffset = bodyScroller.HorizontalOffset();

    // Re-pin leading-frozen columns only when horizontal scroll moves.
    if (ShouldRefreshFrozenColumnsForScroll(this, bodyHOffset))
    {
        RefreshFrozenColumns();
    }

    auto headerScroller = m_headerScroller.get();
    if (!headerScroller)
    {
        return;
    }

    if (std::abs(headerScroller.HorizontalOffset() - bodyHOffset) < 0.5)
    {
        // Skip near-equal offsets to avoid ViewChanged ping-pong.
        return;
    }

    // Instant tracking keeps header and body visually glued.
    headerScroller.ChangeView(bodyHOffset, nullptr, nullptr, true);
}

winrt::AutomationPeer TableView::OnCreateAutomationPeer()
{
    return winrt::make<TableViewAutomationPeer>(*this);
}

void TableView::OnItemsSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    // Ignore same-reference ItemsSource updates to avoid a no-op row rebuild.
    if (args.OldValue() == args.NewValue())
    {
        return;
    }

    // The edited item is about to leave the control. Forced, because the data set is already gone
    // by the time a handler could react, so the close cannot be vetoed. No-focus teardown: this is
    // a dependency-property change callback, and moving focus from one re-enters the framework.
    if (IsEditing())
    {
        TerminateEditWithoutVisualRestore();
    }

    // The current cell belongs to the old data set. Left alone, CurrentItem keeps returning an item
    // that is not in the new source, BeginEdit fails with no diagnostic, and the discarded item
    // stays rooted by the tracker. WPF DataGrid likewise resets CurrentItem/CurrentCell here.
    //
    // Per-row begin-edit press state needs no reset: it is compared by item identity, so an entry
    // left over from the previous data set can never match an item from the new one.
    SetCurrentCell(nullptr, nullptr);

    // New data set: clear the grow-only Auto accumulators so widths recompute from scratch. The next
    // table measure pass pulls measured widths from the by-then re-realized rows, so the outgoing rows'
    // stale content no longer pins the columns.
    ResetColumnDesiredWidths();

    // PART_RowsRepeater is driven from the flat ItemsSource DP.
    UpdateRowsItemsSource();
}

void TableView::OnHeadersVisibilityPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (args.OldValue() == args.NewValue())
    {
        return;
    }

    UpdateHeaderVisibility();
}

void TableView::OnGridLinesVisibilityPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (args.OldValue() == args.NewValue())
    {
        return;
    }

    ApplyGridLinesToHeader();
    RefreshGridLinesOnRealizedRows();
}

void TableView::OnRowBackgroundPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (args.OldValue() == args.NewValue())
    {
        return;
    }

    // Re-tint realized rows so opt-in banding refreshes.
    RefreshRowBackgroundsOnRealizedRows();
}

void TableView::OnAlternatingRowBackgroundPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (args.OldValue() == args.NewValue())
    {
        return;
    }

    RefreshRowBackgroundsOnRealizedRows();
}

void TableView::ApplyGridLinesToHeader()
{
    const auto visibility = GridLinesVisibility();

    if (auto headerFE = m_headerRow.get())
    {
        if (auto headerBorder = headerFE.try_as<winrt::Border>())
        {
            if (WantsHorizontalLines(visibility))
            {
                headerBorder.ClearValue(winrt::Border::BorderThicknessProperty());
            }
            else
            {
                headerBorder.BorderThickness(s_zeroThickness);
            }
        }
    }

    auto host = m_headerHost.get();
    if (!host)
    {
        return;
    }

    const bool wantVertical = WantsVerticalLines(visibility);
    const auto headerGridLineName = winrt::hstring{ s_HeaderGridLineName };
    const auto headerCells = host.Children();
    const uint32_t headerCellCount = headerCells.Size();
    for (uint32_t i = 0; i < headerCellCount; ++i)
    {
        if (auto headerCell = headerCells.GetAt(i).try_as<winrt::Panel>())
        {
            const auto children = headerCell.Children();
            const uint32_t childCount = children.Size();
            for (uint32_t childIndex = 0; childIndex < childCount; ++childIndex)
            {
                if (auto border = children.GetAt(childIndex).try_as<winrt::Border>())
                {
                    if (border.Name() == headerGridLineName)
                    {
                        border.Visibility(wantVertical ? winrt::Visibility::Visible : winrt::Visibility::Collapsed);
                    }
                }
            }
        }
    }
}

void TableView::ForEachRealizedRow(std::function<void(winrt::TableViewRow const&)> const& fn)
{
    if (auto repeater = m_rowsRepeater.get())
    {
        const auto childCount = winrt::VisualTreeHelper::GetChildrenCount(repeater);
        for (int32_t i = 0; i < childCount; ++i)
        {
            if (auto row = winrt::VisualTreeHelper::GetChild(repeater, i).try_as<winrt::TableViewRow>())
            {
                fn(row);
            }
        }
    }
}

void TableView::RefreshGridLinesOnRealizedRows()
{
    ForEachRealizedRow([](winrt::TableViewRow const& row)
    {
        winrt::get_self<TableViewRow>(row)->RefreshGridLines();
    });
}

void TableView::RefreshRowBackgroundsOnRealizedRows()
{
    ForEachRealizedRow([](winrt::TableViewRow const& row)
    {
        winrt::get_self<TableViewRow>(row)->RefreshRowBackground();
    });
}

void TableView::UpdateRowsItemsSource()
{
    // Push the flat ItemsSource DP into the repeater when one exists.
    winrt::IInspectable rowsSource = ItemsSource();

    if (auto repeater = m_rowsRepeater.get())
    {
        repeater.ItemsSource(rowsSource);

        UpdateEmptyStateCollectionChangedSubscription();
        UpdateEmptyState();

        // Re-point selection at the new source. SelectionModel::Source clears unconditionally, so a
        // swap always drops the selection; then drain anything requested before a source existed.
        ResolveSelectionAfterSourceChange();
    }
    // else: OnApplyTemplate hasn't run yet; the repeater will be sourced from there.
}

void TableView::OnEmptyTemplatePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    // Skip same-value sets (no re-subscription / re-evaluation), matching the other DP callbacks.
    if (args.OldValue() == args.NewValue())
    {
        return;
    }

    UpdateEmptyStateCollectionChangedSubscription();
    UpdateEmptyState();
}

void TableView::UpdateEmptyStateCollectionChangedSubscription()
{
    // Rewire count-change tracking; auto_revoke drops the prior source subscription.
    m_emptyStateCollectionChangedRevoker = {};
    if (EmptyTemplate() != nullptr)
    {
        // Count changes matter only when an empty template can be displayed.
        if (auto repeater = m_rowsRepeater.get())
        {
            if (auto view = repeater.ItemsSourceView())
            {
                m_emptyStateCollectionChangedRevoker = view.CollectionChanged(
                    winrt::auto_revoke, { this, &TableView::OnEmptyStateItemsSourceCollectionChanged });
            }
        }
    }
}

void TableView::OnEmptyStateItemsSourceCollectionChanged(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/)
{
    UpdateEmptyState();
}

void TableView::UpdateEmptyState()
{
    auto presenter = m_emptyStatePresenter.get();
    if (!presenter)
    {
        // Template hasn't applied, or this template carries no empty-state part.
        return;
    }

    auto const emptyTemplate = EmptyTemplate();
    auto repeater = m_rowsRepeater.get();

    if (!emptyTemplate)
    {
        // Default opt-out keeps rows visible and never shows the empty surface.
        presenter.Visibility(winrt::Visibility::Collapsed);
        presenter.ContentTemplate(nullptr);
        if (repeater) { repeater.Visibility(winrt::Visibility::Visible); }
        return;
    }

    bool isEmpty = true;
    if (repeater)
    {
        if (auto view = repeater.ItemsSourceView())
        {
            isEmpty = view.Count() == 0;
        }
    }

    if (isEmpty)
    {
        if (presenter.ContentTemplate() != emptyTemplate)
        {
            presenter.ContentTemplate(emptyTemplate);
        }
        // Ensure the ContentControl inflates the template without a data item.
        if (!presenter.Content())
        {
            presenter.Content(box_value(winrt::hstring{}));
        }
        presenter.Visibility(winrt::Visibility::Visible);
        if (repeater) { repeater.Visibility(winrt::Visibility::Collapsed); }
    }
    else
    {
        presenter.Visibility(winrt::Visibility::Collapsed);
        if (repeater) { repeater.Visibility(winrt::Visibility::Visible); }
    }
}

static std::wstring_view DensitySuffix(winrt::TableViewDensity density)
{
    switch (density)
    {
    case winrt::TableViewDensity::Compact: return L"Compact"sv;
    case winrt::TableViewDensity::Comfortable: return L"Comfortable"sv;
    default: return L""sv;
    }
}

bool TableView::IsHighContrast()
{
    // ActualTheme cannot report HC. Prefer the cached ThemeSettings value (kept fresh by Changed);
    // before Loaded (no WindowId yet) fall back to a one-shot AccessibilitySettings read.
    if (m_themeSettings)
    {
        return m_isHighContrast;
    }
    try
    {
        return winrt::Windows::UI::ViewManagement::AccessibilitySettings{}.HighContrast();
    }
    catch (...)
    {
        return false;
    }
}

double TableView::GetDensityRowMinHeight()
{
    auto& cache = GetTableViewResourceCache(this);
    if (cache.density.hasRowMinHeight)
    {
        return cache.density.rowMinHeight;
    }

    std::wstring key{ L"TableViewRowMinHeight" };
    key += DensitySuffix(Density());
    const auto fallback = DensityRowMinHeightFallback(Density());
    if (auto raw = LookupElementResource(*this, key))
    {
        cache.density.rowMinHeight = winrt::unbox_value_or<double>(raw, fallback);
        cache.density.hasRowMinHeight = true;
        return cache.density.rowMinHeight;
    }
    // Resource-miss fallback mirrors Fluent density defaults so Standard stays taller than Compact.
    cache.density.rowMinHeight = fallback;
    cache.density.hasRowMinHeight = true;
    return cache.density.rowMinHeight;
}

winrt::Thickness TableView::GetDensityCellPadding()
{
    auto& cache = GetTableViewResourceCache(this);
    if (cache.density.hasCellPadding)
    {
        return cache.density.cellPadding;
    }

    std::wstring key{ L"TableViewCellPadding" };
    key += DensitySuffix(Density());
    const auto fallback = DensityCellPaddingFallback(Density());
    if (auto raw = LookupElementResource(*this, key))
    {
        cache.density.cellPadding = winrt::unbox_value_or<winrt::Thickness>(raw, fallback);
        cache.density.hasCellPadding = true;
        return cache.density.cellPadding;
    }
    // Resource-miss fallback mirrors density padding presets; Standard keeps the legacy padding.
    cache.density.cellPadding = fallback;
    cache.density.hasCellPadding = true;
    return cache.density.cellPadding;
}

winrt::Thickness TableView::GetDensityHeaderCellPadding()
{
    auto& cache = GetTableViewResourceCache(this);
    if (cache.density.hasHeaderCellPadding)
    {
        return cache.density.headerCellPadding;
    }

    std::wstring key{ L"TableViewHeaderCellPadding" };
    key += DensitySuffix(Density());
    const auto fallback = DensityCellPaddingFallback(Density());
    if (auto raw = LookupElementResource(*this, key))
    {
        cache.density.headerCellPadding = winrt::unbox_value_or<winrt::Thickness>(raw, fallback);
        cache.density.hasHeaderCellPadding = true;
        return cache.density.headerCellPadding;
    }
    // Resource-miss fallback mirrors cell-padding density presets.
    cache.density.headerCellPadding = fallback;
    cache.density.hasHeaderCellPadding = true;
    return cache.density.headerCellPadding;
}

double TableView::GetCellFontSize()
{
    auto& cache = GetTableViewResourceCache(this);
    if (cache.font.hasCellFontSize)
    {
        return cache.font.cellFontSize;
    }

    constexpr double fallback = 14.0;
    if (auto raw = LookupElementResource(*this, L"TableViewCellFontSize"))
    {
        cache.font.cellFontSize = winrt::unbox_value_or<double>(raw, fallback);
        cache.font.hasCellFontSize = true;
        return cache.font.cellFontSize;
    }
    cache.font.cellFontSize = fallback;
    cache.font.hasCellFontSize = true;
    return cache.font.cellFontSize;
}

double TableView::GetHeaderFontSize()
{
    auto& cache = GetTableViewResourceCache(this);
    if (cache.font.hasHeaderFontSize)
    {
        return cache.font.headerFontSize;
    }

    constexpr double fallback = 14.0;
    if (auto raw = LookupElementResource(*this, L"TableViewHeaderFontSize"))
    {
        cache.font.headerFontSize = winrt::unbox_value_or<double>(raw, fallback);
        cache.font.hasHeaderFontSize = true;
        return cache.font.headerFontSize;
    }
    cache.font.headerFontSize = fallback;
    cache.font.hasHeaderFontSize = true;
    return cache.font.headerFontSize;
}

void TableView::OnDensityPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (args.OldValue() == args.NewValue())
    {
        return;
    }

    InvalidateTableViewResourceCache(this);

    // Density changes require rebuilding headers and refreshing realized rows.
    RebuildHeaders();
    ForEachRealizedRow([](winrt::TableViewRow const& row)
    {
        winrt::get_self<TableViewRow>(row)->RefreshDensity();
    });

    // Density changes only vertical padding and row height (horizontal cell padding is identical
    // across all presets), so it does not alter Auto content *width*. Re-measure for the new row
    // metrics and re-pin frozen columns (clips depend on row height); the grow-only Auto accumulator
    // is deliberately left intact since density is not a data-set change.
    InvalidateMeasure();
    RefreshFrozenColumns();
}

// A column becoming read-only must close an edit open ON THAT COLUMN, for the same reason the
// control-level IsReadOnly does: otherwise the user keeps an editor, and a committable value, on a
// cell that now reports it cannot be edited. Uses the no-focus teardown because this arrives from a
// dependency-property change callback, where moving focus re-enters focus/layout processing.
void TableView::OnColumnIsReadOnlyChanged(winrt::TableViewColumn const& column)
{
    if (!column || !IsEditing())
    {
        return;
    }

    if (m_currentEditColumn.get() == column)
    {
        TerminateEditWithoutVisualRestore();
    }
}

void TableView::OnIsReadOnlyPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    // Turning the control read-only must close an open cell, or the user keeps an editor - and a
    // committable value - on a control that now reports it cannot be edited. Forced, because
    // read-only is a control-level statement a handler must not veto. Uses the no-focus teardown:
    // this runs from a DP change callback, where moving focus re-enters focus/layout processing.
    if (IsReadOnly())
    {
        if (IsEditing())
        {
            TerminateEditWithoutVisualRestore();
        }
    }
}

bool TableView::ShouldShowColumnHeaders()
{
    return (static_cast<uint32_t>(HeadersVisibility()) & static_cast<uint32_t>(winrt::TableViewHeadersVisibility::Column)) != 0;
}

void TableView::UpdateHeaderVisibility()
{
    const bool showColumnHeaders = ShouldShowColumnHeaders();
    const auto columnVisibility = showColumnHeaders ?
        winrt::Visibility::Visible : winrt::Visibility::Collapsed;

    if (auto headerHost = m_headerHost.get())
    {
        headerHost.Visibility(columnVisibility);
    }
    if (auto headerScroller = m_headerScroller.get())
    {
        headerScroller.Visibility(columnVisibility);
    }
    if (auto headerRow = m_headerRow.get())
    {
        headerRow.Visibility(columnVisibility);
    }
}

int32_t TableView::GetItemsSourceCount() const
{
    if (auto repeater = m_rowsRepeater.get())
    {
        if (auto sourceView = repeater.ItemsSourceView())
        {
            return sourceView.Count();
        }
    }
    return 0;
}

void TableView::OnRowElementPrepared(
    const winrt::ItemsRepeater& /*sender*/,
    const winrt::ItemsRepeaterElementPreparedEventArgs& args)
{
    if (auto row = args.Element().try_as<winrt::TableViewRow>())
    {
        auto rowImpl = winrt::get_self<TableViewRow>(row);
        rowImpl->SetOwningTableViewInternal(*this);
        rowImpl->RefreshGridLines();
        rowImpl->RefreshRowBackground();
        RefreshRowSelectionState(row);
        InvalidateMeasure();
    }
}

void TableView::OnRowElementClearing(
    const winrt::ItemsRepeater& /*sender*/,
    const winrt::ItemsRepeaterElementClearingEventArgs& args)
{
    if (auto row = args.Element().try_as<winrt::TableViewRow>())
    {
        // A recycled row is about to be re-bound to a different item; an edit left open would keep
        // an editor over the new item, and the next commit would write into the wrong data object.
        // ElementClearing runs inside the repeater's measure pass, so this must not restore the
        // display visual.
        if (row == m_currentEditRow.get())
        {
            if (!TerminateEditWithoutVisualRestore(true /* insideLayoutPass */))
            {
                // The teardown was refused - the only way that happens here is an edit still in the
                // Beginning window, where app code (a BeginningEdit handler, ITableViewEditableItem
                // .BeginEdit) mutated the collection and recycled the row underneath us. The row
                // must not go back to the pool holding a live editor, so abandon it directly and
                // make BeginEdit unwind instead of promoting to Editing.
                winrt::get_self<TableViewRow>(row)->AbandonCellEdit();
                m_abandonPendingBeginEdit = true;
                ++m_editGeneration;
                m_currentEditRow.set(nullptr);
                m_currentEditItem.set(nullptr);
                m_currentEditColumn.set(nullptr);
                m_editUneditedValue.set(nullptr);

                TVDiag::LogRetailF(
                    L"[TableView] A row was recycled while an edit was still opening; the edit was abandoned.");
            }
        }

        winrt::get_self<TableViewRow>(row)->SetOwningTableViewInternal(nullptr);
        InvalidateMeasure();
    }
}

void TableView::OnRowElementIndexChanged(
    const winrt::ItemsRepeater& /*sender*/,
    const winrt::ItemsRepeaterElementIndexChangedEventArgs& args)
{
    auto const row = args.Element().try_as<winrt::TableViewRow>();
    if (!row)
    {
        return;
    }

    // Realized rows keep their element but get a new index, so banding parity must refresh.
    if (RowBackground() != nullptr || AlternatingRowBackground() != nullptr)
    {
        winrt::get_self<TableViewRow>(row)->RefreshRowBackground();
    }

    // ...and so must selected chrome. The element keeps its item here (only its index moved), so
    // this normally re-derives the same answer - it is the cheap guarantee that a row whose index
    // shifted under an insert cannot end up disagreeing with the model.
    RefreshRowSelectionState(row);
}

void TableView::RebuildHeaders()
{
    auto host = m_headerHost.get();
    if (!host)
    {
        return;
    }

    host.Children().Clear();

    // Cache theme-resource padding once per header rebuild; values are stable for the pass.
    winrt::Thickness cachedHeaderCellPadding = GetDensityHeaderCellPadding();
    // Always cache the header grid-line brush at rebuild time; visibility toggles do not reassign it later.
    const bool wantVerticalHeaderLines = WantsVerticalLines(GridLinesVisibility());
    const auto cachedHeaderGridLineBrush = GetGridLineBrush();
    // Header cells share the density row min-height so the header band matches the body rows.
    const double cachedRowMinHeight = GetDensityRowMinHeight();
    const double cachedHeaderFontSize = GetHeaderFontSize();

    if (auto columns = Columns())
    {
        for (auto const& column : columns)
        {
            // Skip entries this TableView rejected so a half-owned column cannot render here while
            // its callbacks still route to another owner.
            if (!column || winrt::get_self<TableViewColumn>(column)->GetOwningTableView() != *this)
            {
                continue;
            }

            // Header cell root.
            winrt::Grid headerCell;
            headerCell.Visibility(column.Visibility());
            // Match the body row min-height so the header band and rows render at the same height.
            headerCell.MinHeight(cachedRowMinHeight);

            // No Width binding: TableViewCellsPanel arranges header cells at the column's ActualWidth;
            // an explicit Width would defeat the panel's unconstrained Auto measured-width measurement.

            winrt::ContentPresenter content;
            content.Content(column.Header());
            if (auto headerTemplateSelector = column.HeaderTemplateSelector())
            {
                content.ContentTemplateSelector(headerTemplateSelector);
            }
            else if (auto headerTemplate = column.HeaderTemplate())
            {
                content.ContentTemplate(headerTemplate);
            }
            // Consume TableViewHeaderCellPadding from theme resources (cached once per rebuild).
            content.Padding(cachedHeaderCellPadding);
            content.HorizontalAlignment(winrt::HorizontalAlignment::Stretch);
            content.VerticalAlignment(winrt::VerticalAlignment::Center);
            // Column-header text: theme font size, SemiBold to stand out from cells (templates override).
            content.FontSize(cachedHeaderFontSize);
            content.FontWeight(winrt::FontWeights::SemiBold());
            headerCell.Children().Append(content);

            // Resolve from TableView so header grid lines track theme.
            {
                winrt::Border headerGridLine;
                headerGridLine.Name(winrt::hstring{ s_HeaderGridLineName });
                headerGridLine.Width(1);
                headerGridLine.HorizontalAlignment(winrt::HorizontalAlignment::Right);
                headerGridLine.IsHitTestVisible(false);
                headerGridLine.Visibility(wantVerticalHeaderLines ? winrt::Visibility::Visible : winrt::Visibility::Collapsed);
                headerGridLine.Background(cachedHeaderGridLineBrush);
                headerCell.Children().Append(headerGridLine);
            }

            // Tag header cells so frozen-column refresh can map them back to columns.
            headerCell.Tag(column);

            host.Children().Append(headerCell);
        }
    }

    // Pin (or refresh) leading-frozen header cells at the current scroll offset.
    RefreshFrozenColumns();
    ApplyGridLinesToHeader();
}

double TableView::GetHeaderMeasuredWidthForColumn(const winrt::TableViewColumn& column) const
{
    // Own the header host's concrete panel type here so the layout engine (TableView_Layout.cpp)
    // pulls the header's measured width through this seam and never casts to TableViewCellsPanel.
    if (auto headerHost = m_headerHost.get())
    {
        if (auto cellsPanel = headerHost.try_as<winrt::TableViewCellsPanel>())
        {
            return winrt::get_self<TableViewCellsPanel>(cellsPanel)->MeasuredWidthForColumn(column);
        }
    }

    return 0.0;
}

void TableView::QueueRebuildHeaders()
{
    // Before the template applies there is no header host; OnApplyTemplate builds headers once, so a
    // rebuild queued now would be a wasted no-op (RebuildHeaders early-returns on a null host anyway).
    if (!m_headerHost.get())
    {
        return;
    }

    // A rebuild is already scheduled for this tick -- collapse the burst into one.
    if (m_rebuildHeadersQueued)
    {
        return;
    }

    auto dispatcher = DispatcherQueue();
    if (!dispatcher)
    {
        // No dispatcher (teardown) -- rebuild synchronously so headers are not left stale.
        RebuildHeaders();
        return;
    }

    m_rebuildHeadersQueued = true;
    auto weakThis = get_weak();
    if (!dispatcher.TryEnqueue([weakThis]()
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->m_rebuildHeadersQueued = false;
                try
                {
                    strongThis->RebuildHeaders();
                    // Header sizes may have changed; re-resolve column widths against the new headers.
                    strongThis->InvalidateMeasure();
                }
                catch (...)
                {
                    // Coalesced header rebuild is best-effort; never fail-fast the dispatcher.
                }
            }
        }))
    {
        // Enqueue failed -- fall back to a synchronous rebuild so headers are not left stale.
        m_rebuildHeadersQueued = false;
        RebuildHeaders();
    }
}

void TableView::OnTableViewUnloaded()
{
    if (m_pendingFocusLayoutToken.value)
    {
        LayoutUpdated(m_pendingFocusLayoutToken);
        m_pendingFocusLayoutToken = {};
    }

    // Null ItemsSource on unload to release repeater cache work before it ticks on a detached subtree.
    // OnRowsRepeaterLoaded re-sources cached pages when they return.
    if (auto repeater = m_rowsRepeater.get())
    {
        // Re-sourcing on load hands SelectionModel a new view, and setting Source always clears.
        // Hold the selected item so the reload re-selects it instead of dropping it.
        StashSelectionForReload();

        try { repeater.ItemsSource(nullptr); }
        catch (...) {}
        m_rowsSourceDrained = true; // Remember that Loaded must restore the source.
    }

    // Detach ViewChanged so deferred scroll callbacks do not run after unload.
    if (auto bodyScroller = m_bodyScroller.get())
    {
        if (m_bodyScrollerViewChangedToken.value)
        {
            try { bodyScroller.ViewChanged(m_bodyScrollerViewChangedToken); }
            catch (...) {}
            m_bodyScrollerViewChangedToken = {};
        }
    }
    m_bodyScrollerSizeChangedRevoker.revoke();
    m_bodyScroller.set(nullptr);
    m_headerScroller.set(nullptr);

    // Drop the ThemeSettings subscription and instance so a subsequent Loaded re-creates it against
    // the (possibly different) window's WindowId. IsHighContrast falls back to AccessibilitySettings
    // while detached.
    m_themeSettingsChangedRevoker.revoke();
    m_themeSettings = nullptr;
}
