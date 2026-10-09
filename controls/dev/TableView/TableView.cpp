// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewHeaderCell.h"
#include "TableViewColumn.h"
#include "TableViewRow.h"
#include "TableViewCellsPanel.h"
#include "TableViewToolTipHelpers.h"
#include "TableViewAutomationPeer.h"
#include "TableViewSource.h"
#include "TableViewRowTemplateSelector.h"
#include "TableViewGroupHeader.h"
#include "SortIndicator.h"
#include "RuntimeProfiler.h"
#include "TVDiag.h"

#include <string>
#include <cmath>
#include <limits>
#include <algorithm>
#include <initializer_list>
#include <exception>

static constexpr std::wstring_view s_RowsRepeaterPartName{ L"PART_RowsRepeater"sv };
static constexpr std::wstring_view s_HeaderRowPartName{ L"PART_HeaderRow"sv };
static constexpr std::wstring_view s_HeaderHostPartName{ L"PART_HeaderHost"sv };
static constexpr std::wstring_view s_EmptyStatePresenterPartName{ L"PART_EmptyStatePresenter"sv };
static constexpr std::wstring_view s_HeaderGridLineName{ L"TableViewHeaderGridLine"sv };
static constexpr std::wstring_view s_ResizeGripperWidthKey{ L"TableViewResizeGripperWidth"sv };
// Matches TableViewResizeGripperWidth in the theme dictionaries; used when that key is missing or
// unusable.
static constexpr double c_resizeGripperWidthFallback{ 8.0 };
static constexpr std::wstring_view s_SortIndicatorName{ L"TableViewSortIndicator"sv };
static constexpr std::wstring_view s_SortIndicatorSizeKey{ L"SortIndicatorSize"sv };
// Matches SortIndicatorSize in SortIndicator_themeresources.xaml; used only when that key is
// missing or unusable.
static constexpr double c_sortIndicatorSizeFallback{ 16.0 };

namespace
{
    // cppwinrt's == compares raw ABI pointers, which can differ for the same object across a QI,
    // so fall back to canonical IUnknown identity.
    bool IsSameObject(winrt::IInspectable const& left, winrt::IInspectable const& right)
    {
        if (left == right)
        {
            return true;
        }

        if (!left || !right)
        {
            return false;
        }

        return left.as<winrt::Windows::Foundation::IUnknown>() ==
               right.as<winrt::Windows::Foundation::IUnknown>();
    }

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

    double TerminalEdgeTolerance(const winrt::FrameworkElement& element) noexcept
    {
        double scale = 1.0;
        try
        {
            if (element)
            {
                if (auto root = element.XamlRoot())
                {
                    const auto rasterizationScale = root.RasterizationScale();
                    if (std::isfinite(rasterizationScale) && rasterizationScale > 0.0)
                    {
                        scale = rasterizationScale;
                    }
                }
            }
        }
        catch (...)
        {
        }

        // Half a physical pixel, widened slightly because transformed bounds are float-backed and
        // can land microscopically beyond that boundary after layout rounding.
        constexpr double layoutEpsilonPixels = 1.0 / 64.0;
        return (0.5 + layoutEpsilonPixels) / scale;
    }

    bool TryGetBoundsRelativeTo(
        const winrt::FrameworkElement& element,
        const winrt::UIElement& relativeTo,
        winrt::Rect& bounds) noexcept
    {
        try
        {
            if (!element || !relativeTo || !element.IsLoaded() ||
                element.ActualWidth() <= 0.0 || element.ActualHeight() <= 0.0)
            {
                return false;
            }

            bounds = element.TransformToVisual(relativeTo).TransformBounds(
                { 0.0f, 0.0f, static_cast<float>(element.ActualWidth()), static_cast<float>(element.ActualHeight()) });
        }
        catch (...)
        {
            return false;
        }

        return std::isfinite(bounds.X) &&
            std::isfinite(bounds.Y) &&
            std::isfinite(bounds.Width) &&
            std::isfinite(bounds.Height);
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

TableView::~TableView()
{
    IgnoreTelemetry(TableViewTelemetry::IgnoreReason::Unloaded);
    // Must run while this control still holds the selector: once anything has been recycled the
    // pools hang off the cached templates and close a cycle the reference tracker cannot walk.
    // Destructor runs off the reference tracker's teardown path (e.g. UIAffinityReleaseQueue),
    // not necessarily via a direct Release() call, so plain get() can observe the tracker handle
    // already invalidated and assert/fail-fast in chk builds. safe_get() is the documented-safe
    // accessor for tracker_ref from a destructor (see tracker_ref.h and ItemsView/ScrollView).
    if (auto const selector = m_rowTemplateSelector.safe_get())
    {
        winrt::get_self<::TableViewRowTemplateSelector>(selector)->Detach();
    }
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

    m_keyUpHandler = winrt::KeyEventHandler(
        [weakThis](winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->OnKeyUpForHeaderSort(sender, args);
            }
        });
    // Space on a focused header arms on KeyDown and sorts on an unhandled KeyUp; handledEventsToo
    // lets a handled KeyUp leave the arm intact rather than consuming it.
    AddHandler(winrt::UIElement::KeyUpEvent(), winrt::box_value(m_keyUpHandler), true /* handledEventsToo */);

    m_headerSortLostFocusRevoker = LostFocus(winrt::auto_revoke,
        [weakThis](winrt::IInspectable const&, winrt::RoutedEventArgs const&)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->m_headerSortSpaceArmedColumn = nullptr;
            }
        });
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

    // The header cell's subtree does not observe the ambient FlowDirection auto-flip, so
    // RebuildHeaders stamps the trailing-edge alignment from the control's FlowDirection. That
    // stamp is not self-updating: rebuild the headers when the direction actually flips.
    RegisterPropertyChangedCallback(
        winrt::FrameworkElement::FlowDirectionProperty(),
        [weakThis](winrt::DependencyObject const&, winrt::DependencyProperty const&)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->QueueRebuildHeaders();
                // Body cells carry the same stamp as a one-sided BorderThickness, and realized rows
                // are not rebuilt by the header pass - refresh them or the body grid lines stay on
                // the edge the previous direction chose.
                strongThis->RefreshGridLinesOnRealizedRows();
                strongThis->QueueTerminalGridLineRefresh();
            }
        });

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
    BeginInitializationTelemetry(TableViewTelemetry::Origin::Loaded);
    QueueTelemetryLayout();
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
        // Best-effort during teardown; keep the previous HC state if the read fails.
    }

    InvalidateTableViewResourceCache(this);
    if (IsLoaded())
    {
        RebuildHeaders();
        RefreshGridLinesOnRealizedRows();
        QueueTerminalGridLineRefresh();
    }
}

void TableView::OnApplyTemplate()
{
    m_headerSortSpaceArmedColumn = nullptr;
    StopTelemetryLayout();
    IgnoreTelemetry(TableViewTelemetry::IgnoreReason::Retemplated, TableViewTelemetry::Stage::Template);
    BeginInitializationTelemetry(TableViewTelemetry::Origin::Template);
    bool completed = false;
    auto telemetryFailure = wil::scope_exit([this, &completed]() noexcept
    {
        if (!completed)
        {
            FailOperationTelemetry(TableViewTelemetry::Operation::InitialLayout, 0, TableViewTelemetry::Stage::Template);
        }
    });
    __super::OnApplyTemplate();
    InvalidateTableViewResourceCache(this);

    if (m_pendingFocusLayoutToken.value)
    {
        LayoutUpdated(m_pendingFocusLayoutToken);
        m_pendingFocusLayoutToken = {};
    }
    if (m_pendingGroupFocusLayoutToken.value)
    {
        LayoutUpdated(m_pendingGroupFocusLayoutToken);
        m_pendingGroupFocusLayoutToken = {};
    }
    if (m_pendingGroupRowRefreshLayoutToken.value)
    {
        LayoutUpdated(m_pendingGroupRowRefreshLayoutToken);
        m_pendingGroupRowRefreshLayoutToken = {};
    }
    if (m_terminalGridLinesLayoutToken.value)
    {
        LayoutUpdated(m_terminalGridLinesLayoutToken);
        m_terminalGridLinesLayoutToken = {};
    }
    if (auto terminalRow = m_terminalGridLineRow.get())
    {
        winrt::get_self<TableViewRow>(terminalRow)->SetTerminalGridLineSuppression({});
    }
    if (auto terminalHeader = m_terminalGridLineGroupHeader.get())
    {
        winrt::get_self<TableViewGroupHeader>(terminalHeader)->SetTerminalBottomGridLineSuppression(false);
    }
    m_terminalGridLineRowSizeChangedRevoker.revoke();
    m_terminalGridLineRow = {};
    m_terminalGridLineGroupHeader = {};
    m_suppressTrailingGridLine = false;
    m_suppressBottomGridLine = false;
    m_terminalGridLineGeometryRetryAvailable = true;
    m_terminalGridLineColumnIndex = -1;
    m_terminalGridLineHorizontalOffset = std::numeric_limits<double>::quiet_NaN();
    m_terminalGridLineVerticalOffset = std::numeric_limits<double>::quiet_NaN();
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
        // A live popup would still host content parented into the abandoned band.
        ReleaseHeaderToolTips(oldHeaderHost);

        // Mirror the rowsRepeater Loaded cleanup for the header host.
        if (m_headerHostLoadedToken.value)
        {
            if (auto oldHeaderHostFE = oldHeaderHost.try_as<winrt::FrameworkElement>())
            {
                oldHeaderHostFE.Loaded(m_headerHostLoadedToken);
            }
            m_headerHostLoadedToken = {};
        }

        // Auto-revoke would also release these on reassignment below, but the old band must not
        // raise focus events into a control whose template has already been swapped.
        m_headerHostGettingFocusRevoker.revoke();
        m_headerHostGotFocusRevoker.revoke();
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

    // Defer ScrollViewer ancestor lookup until Loaded because template parts are not fully connected here.
    if (auto headerHost = m_headerHost.get())
    {
        // One tab stop for the header band, matching Explorer and WinUI list controls: Tab crosses
        // bands, arrows stay inside. Apply it at PART_HeaderHost, not TableViewCellsPanel; the panel
        // is shared layout, while the focus policy belongs to the two host bands.
        headerHost.TabFocusNavigation(winrt::KeyboardNavigationMode::Once);

        // Redirect band entry from the first header to the remembered column.
        m_headerHostGettingFocusRevoker = headerHost.GettingFocus(winrt::auto_revoke,
            [weakThis](winrt::IInspectable const& sender,
                winrt::Microsoft::UI::Xaml::Input::GettingFocusEventArgs const& args)
            {
                if (auto strongThis = weakThis.get())
                {
                    strongThis->OnHeaderHostGettingFocus(sender, args);
                }
            });

        // Update the shared column cursor whenever a header actually takes focus.
        m_headerHostGotFocusRevoker = headerHost.GotFocus(winrt::auto_revoke,
            [weakThis](winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
            {
                if (auto strongThis = weakThis.get())
                {
                    strongThis->OnHeaderHostGotFocus(sender, args);
                }
            });

        // Focus on an off-screen header must not scroll PART_HeaderScroller: header/body sync is
        // one-way, so the band would end up offset from the columns it labels.
        m_headerBringIntoViewRevoker = headerHost.BringIntoViewRequested(winrt::auto_revoke,
            [weakThis](winrt::IInspectable const& /*sender*/, winrt::BringIntoViewRequestedEventArgs const& args)
            {
                auto strongThis = weakThis.get();
                if (!strongThis)
                {
                    return;
                }
                strongThis->OnHeaderBringIntoViewRequested(args);
            });

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

        QueueTerminalGridLineRefresh();
    }

    if (auto repeater = m_rowsRepeater.get())
    {
        // Assigned here rather than in the template: the selector needs an owning TableView to map
        // an item to its row kind, and XAML has no way to hand it one.
        auto selector = winrt::make<::TableViewRowTemplateSelector>();
        winrt::get_self<::TableViewRowTemplateSelector>(selector)->SetOwningTableViewInternal(*this);
        m_rowTemplateSelector.set(selector);
        repeater.ItemTemplate(selector);

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

    // Drive the repeater from the active source only after its item template and lifecycle hooks
    // are wired. ItemsRepeater may react to ItemsSource immediately; doing this earlier leaves it
    // briefly sourced without the selector/ElementPrepared owner hookup TableView rows require.
    RefreshRowsPipeline();

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
                    strongThis->QueueTerminalGridLineRefresh();
                }
                catch (...)
                {
                    // Theme-switch refresh is best-effort.
                }
            });
    }
    completed = true;
    QueueTelemetryLayout();
}

TableView::ActionTelemetry::ActionTelemetry(
    TableView& owner, TableViewTelemetry::Operation operation, TableViewTelemetry::Stage stage, bool admitted) noexcept :
    m_owner(owner), m_stage(stage), m_lifetimeGeneration(owner.m_telemetryLifetimeGeneration),
    m_exceptions(std::uncaught_exceptions())
{
    try
    {
        if (admitted && owner.IsLoaded() && !owner.m_rowsSourceDrained)
        {
            m_state.origin = owner.m_telemetry.origin;
            if (TableViewTelemetry::UpdateConfiguration(m_state, owner.m_telemetry.configuration))
            {
                m_generation = TableViewTelemetry::BeginOperation(m_state, operation);
            }
        }
    }
    catch (...)
    {
        OutputDebugStringW(L"TableView telemetry: action attachment check failed.\n");
    }
}

TableView::ActionTelemetry::~ActionTelemetry()
{
    if (!m_generation) { return; }
    if (m_lifetimeGeneration != m_owner.m_telemetryLifetimeGeneration)
    {
        TableViewTelemetry::IgnoreOperation(m_state, m_generation, m_owner.m_telemetryCancellationReason);
        return;
    }
    auto const sourceGeneration = m_owner.m_rowMetadataGeneration;
    auto const selectionVersion = m_owner.m_selectionVersion;
    auto const editGeneration = m_owner.m_editGeneration;
    auto const configuration = m_owner.SnapshotTelemetryConfiguration(
        m_owner.m_telemetry.configuration.content, m_owner.m_telemetry.configuration.available);
    if (m_lifetimeGeneration != m_owner.m_telemetryLifetimeGeneration)
    {
        TableViewTelemetry::IgnoreOperation(m_state, m_generation, m_owner.m_telemetryCancellationReason);
        return;
    }
    if (sourceGeneration != m_owner.m_rowMetadataGeneration ||
        selectionVersion != m_owner.m_selectionVersion || editGeneration != m_owner.m_editGeneration)
    {
        TableViewTelemetry::IgnoreOperation(m_state, m_generation, TableViewTelemetry::IgnoreReason::Superseded);
        return;
    }
    if (!TableViewTelemetry::UpdateConfiguration(m_state, configuration))
    {
        TableViewTelemetry::IgnoreOperation(m_state, m_generation, TableViewTelemetry::IgnoreReason::Stale);
        return;
    }
    if (m_failed || std::uncaught_exceptions() > m_exceptions)
    {
        TableViewTelemetry::FailOperation(m_state, m_state.operation, m_generation, m_stage, m_error);
    }
    else if (m_completed)
    {
        TableViewTelemetry::CompleteOperation(m_state, m_generation, TableViewTelemetry::Result::Success);
    }
    else
    {
        TableViewTelemetry::IgnoreOperation(m_state, m_generation, m_ignoreReason);
    }
}

void TableView::ActionTelemetry::Fail(std::optional<HRESULT> error) noexcept
{
    if (!m_failed) { m_error = error; }
    m_failed = true;
}

void TableView::IgnoreTelemetry(TableViewTelemetry::IgnoreReason reason, TableViewTelemetry::Stage stage) noexcept
{
    ++m_telemetryLifetimeGeneration;
    m_telemetryCancellationReason = reason;
    TableViewTelemetry::IgnoreInitial(m_telemetry, reason, stage);
    TableViewTelemetry::IgnoreOperation(m_telemetry, m_telemetry.operationGeneration, reason);
    IgnoreScrollTelemetry(reason);
}

void TableView::BeginInitializationTelemetry(TableViewTelemetry::Origin origin) noexcept
{
    if (m_telemetry.initial != TableViewTelemetry::InitialState::NotStarted) { return; }
    try
    {
        if (origin == TableViewTelemetry::Origin::Loaded && !m_rowsRepeater.get()) { return; }
        if (XamlRoot()) { TableViewTelemetry::BeginInitial(m_telemetry, origin); }
    }
    catch (...)
    {
        OutputDebugStringW(L"TableView telemetry: unable to inspect attachment.\n");
    }
}

void TableView::StopTelemetryLayout() noexcept
{
    ++m_telemetryLayoutGeneration;
    m_telemetrySourceChangedRevoker.revoke();
    m_telemetryViewportChangedRevoker.revoke();
    m_telemetryRootChangedRevoker.revoke();
    auto watches = std::move(m_telemetryVisualWatches);
    m_telemetryVisualWatches.clear();
    for (auto const& watch : watches)
    {
        if (auto object = watch.object.get(); object && watch.token)
        {
            try { object.UnregisterPropertyChangedCallback(watch.property, *watch.token); }
            catch (...) { OutputDebugStringW(L"TableView telemetry: visual observer removal failed.\n"); }
        }
    }
    auto const token = std::exchange(m_telemetryLayoutToken, {});
    if (!token.value) { return; }
    try { LayoutUpdated(token); }
    catch (...) { OutputDebugStringW(L"TableView telemetry: layout observer removal failed.\n"); }
}

bool TableView::WatchTelemetryVisualProperty(
    winrt::DependencyObject const& object, winrt::DependencyProperty const& property)
{
    for (auto const& watch : m_telemetryVisualWatches)
    {
        if (watch.property == property && watch.object.get() == object) { return false; }
    }
    auto const dispatcher = DispatcherQueue();
    if (!dispatcher)
    {
        OutputDebugStringW(L"TableView telemetry: visual observer has no dispatcher.\n");
        return false;
    }
    auto const generation = m_telemetryLayoutGeneration;
    m_telemetryVisualWatches.push_back({ winrt::make_weak(object), property, std::nullopt });
    try
    {
        m_telemetryVisualWatches.back().token = object.RegisterPropertyChangedCallback(
            property, [weakThis = get_weak(), generation, dispatcher](auto&&, auto&&)
            {
                try
                {
                    if (!dispatcher.TryEnqueue([weakThis, generation]()
                        {
                            if (auto self = weakThis.get(); self && self->m_telemetryLayoutGeneration == generation)
                            {
                                if (self->m_telemetryMutationDepth) { self->StopTelemetryLayout(); }
                                else { self->OnTelemetryLayout(); }
                            }
                        }))
                    {
                        OutputDebugStringW(L"TableView telemetry: visual observer dispatch failed.\n");
                    }
                }
                catch (...)
                {
                    OutputDebugStringW(L"TableView telemetry: visual observer dispatch failed.\n");
                }
            });
    }
    catch (...)
    {
        m_telemetryVisualWatches.pop_back();
        throw;
    }
    return true;
}

void TableView::WatchTelemetryTransform(winrt::Transform const& transform)
{
    if (!transform) { return; }
    auto const watch = [this, &transform](std::initializer_list<winrt::DependencyProperty> properties)
    {
        for (auto const& property : properties) { WatchTelemetryVisualProperty(transform, property); }
    };
    if (transform.try_as<winrt::TranslateTransform>())
    {
        watch({ winrt::TranslateTransform::XProperty(), winrt::TranslateTransform::YProperty() });
    }
    else if (transform.try_as<winrt::ScaleTransform>())
    {
        watch({ winrt::ScaleTransform::ScaleXProperty(), winrt::ScaleTransform::ScaleYProperty(),
            winrt::ScaleTransform::CenterXProperty(), winrt::ScaleTransform::CenterYProperty() });
    }
    else if (transform.try_as<winrt::RotateTransform>())
    {
        watch({ winrt::RotateTransform::AngleProperty(), winrt::RotateTransform::CenterXProperty(),
            winrt::RotateTransform::CenterYProperty() });
    }
    else if (transform.try_as<winrt::SkewTransform>())
    {
        watch({ winrt::SkewTransform::AngleXProperty(), winrt::SkewTransform::AngleYProperty(),
            winrt::SkewTransform::CenterXProperty(), winrt::SkewTransform::CenterYProperty() });
    }
    else if (transform.try_as<winrt::MatrixTransform>())
    {
        watch({ winrt::MatrixTransform::MatrixProperty() });
    }
    else if (transform.try_as<winrt::CompositeTransform>())
    {
        watch({ winrt::CompositeTransform::CenterXProperty(), winrt::CompositeTransform::CenterYProperty(),
            winrt::CompositeTransform::ScaleXProperty(), winrt::CompositeTransform::ScaleYProperty(),
            winrt::CompositeTransform::SkewXProperty(), winrt::CompositeTransform::SkewYProperty(),
            winrt::CompositeTransform::RotationProperty(), winrt::CompositeTransform::TranslateXProperty(),
            winrt::CompositeTransform::TranslateYProperty() });
    }
    else if (auto group = transform.try_as<winrt::TransformGroup>())
    {
        if (WatchTelemetryVisualProperty(group, winrt::TransformGroup::ChildrenProperty()))
        {
            for (auto const& child : group.Children()) { WatchTelemetryTransform(child); }
        }
    }
}

void TableView::WatchTelemetrySourceChanges()
{
    if ((!TableViewTelemetry::IsOperationStarted(m_telemetry) &&
        !TableViewTelemetry::IsOperationStarted(m_scrollTelemetry)) || !m_rowsItemsSourceView) { return; }
    auto const generation = m_telemetryLayoutGeneration;
    m_telemetrySourceChangedRevoker = m_rowsItemsSourceView.CollectionChanged(winrt::auto_revoke,
        [weakThis = get_weak(), generation](auto&&, auto&&)
        {
            if (auto self = weakThis.get(); self && self->m_telemetryLayoutGeneration == generation)
            {
                self->InvalidateOperationTelemetry();
            }
        });
}

void TableView::QueueTelemetryLayout() noexcept
{
    if (m_telemetryMutationDepth || m_telemetryLayoutToken.value ||
        (!TableViewTelemetry::NeedsLayout(m_telemetry) &&
            !(m_scrollTelemetrySettled && TableViewTelemetry::IsOperationStarted(m_scrollTelemetry)))) { return; }
    try
    {
        if (!IsLoaded()) { return; }
        auto const generation = m_telemetryLayoutGeneration;
        m_telemetryLayoutToken = LayoutUpdated([weakThis = get_weak(), generation](auto&&, auto&&)
        {
            if (auto self = weakThis.get(); self && self->m_telemetryLayoutGeneration == generation)
            {
                self->OnTelemetryLayout();
            }
        });
        WatchTelemetrySourceChanges();
    }
    catch (...)
    {
        StopTelemetryLayout();
        TableViewTelemetry::IgnoreOperation(m_telemetry, m_telemetry.operationGeneration, TableViewTelemetry::IgnoreReason::Stale);
        IgnoreScrollTelemetry(TableViewTelemetry::IgnoreReason::Stale);
        OutputDebugStringW(L"TableView telemetry: layout observation failed.\n");
    }
}

void TableView::WatchTelemetryViewport()
{
    auto const dispatcher = DispatcherQueue();
    if (!dispatcher)
    {
        OutputDebugStringW(L"TableView telemetry: viewport observer has no dispatcher.\n");
        return;
    }
    auto const generation = m_telemetryLayoutGeneration;
    auto const changed = [weakThis = get_weak(), generation, dispatcher](auto&&, auto&&)
    {
        try
        {
            if (!dispatcher.TryEnqueue([weakThis, generation]()
                {
                    if (auto self = weakThis.get(); self && self->m_telemetryLayoutGeneration == generation)
                    {
                        self->OnTelemetryLayout();
                    }
                }))
            {
                OutputDebugStringW(L"TableView telemetry: viewport observer dispatch failed.\n");
            }
        }
        catch (...)
        {
            OutputDebugStringW(L"TableView telemetry: viewport observer dispatch failed.\n");
        }
    };
    m_telemetryViewportChangedRevoker = EffectiveViewportChanged(winrt::auto_revoke, changed);
    if (auto const root = XamlRoot())
    {
        m_telemetryRootChangedRevoker = root.Changed(winrt::auto_revoke, changed);
    }
}

TableViewTelemetry::Configuration TableView::SnapshotTelemetryConfiguration(TableViewTelemetry::Content content, bool contentAvailable) noexcept
{
    TableViewTelemetry::Configuration configuration{ content, 0, false, false };
    try
    {
        auto const columns = Columns();
        configuration.columnCountBucket = TableViewTelemetry::CountBucket(columns ? columns.Size() : 0);
        if (auto const source = m_activeSource.get())
        {
            auto const sourceImpl = winrt::get_self<::TableViewSource>(source);
            configuration.grouped = sourceImpl->IsGrouped();
            if (auto const count = sourceImpl->DataRowCount())
            {
                configuration.rowCountBucket = TableViewTelemetry::CountBucket(*count);
                configuration.rowCountAvailable = true;
                if (auto const view = sourceImpl->GetItemsSourceView(); view && view.Count() == 0)
                {
                    configuration.content = TableViewTelemetry::Content::Empty;
                }
            }
        }
        else if (!ItemsSource())
        {
            configuration.rowCountAvailable = true;
            configuration.content = TableViewTelemetry::Content::Empty;
        }
        configuration.available = contentAvailable;
    }
    catch (...)
    {
        OutputDebugStringW(L"TableView telemetry: configuration snapshot unavailable.\n");
    }
    return configuration;
}

bool TableView::TryGetTelemetryConfiguration(TableViewTelemetry::Configuration& configuration)
{
    if (!IsLoaded() || ActualWidth() <= 0 || ActualHeight() <= 0 || m_rowsSourceDrained) { return false; }
    auto const hasVisiblePath = [this](winrt::DependencyObject node)
    {
        for (; node; node = winrt::VisualTreeHelper::GetParent(node))
        {
            if (auto element = node.try_as<winrt::UIElement>())
            {
                if (element.Visibility() != winrt::Visibility::Visible) { return false; }
                if (element.Opacity() <= 0)
                {
                    WatchTelemetryVisualProperty(element, winrt::UIElement::OpacityProperty());
                    return false;
                }
            }
        }
        return true;
    };
    auto const repeater = m_rowsRepeater.get();
    auto const scroller = m_bodyScroller.get();
    if (!repeater || !scroller || !scroller.IsLoaded() || scroller.ViewportWidth() <= 0 ||
        scroller.ViewportHeight() <= 0 || !hasVisiblePath(scroller))
    {
        return false;
    }
    auto const intersect = [](winrt::Rect const& left, winrt::Rect const& right)
    {
        auto const x = std::max(left.X, right.X);
        auto const y = std::max(left.Y, right.Y);
        return winrt::Rect{
            x, y,
            std::max(0.0f, std::min(left.X + left.Width, right.X + right.Width) - x),
            std::max(0.0f, std::min(left.Y + left.Height, right.Y + right.Height) - y) };
    };
    auto const root = XamlRoot();
    if (!root) { return false; }
    auto const geometry = scroller.as<winrt::Microsoft::UI::Xaml::Controls::IContentControlPrivate>();
    const auto scale = geometry.GetRasterizationScale();
    if (!std::isfinite(scale) || scale <= 0) { return false; }
    auto hostVisible = geometry.GetGlobalBounds();
    // This existing framework contract applies real layout/ancestor clips in root-physical
    // coordinates. Normalize exactly as GetGlobalBoundsLogical does; do not infer clips from bounds.
    hostVisible = { hostVisible.X / scale, hostVisible.Y / scale,
        hostVisible.Width / scale, hostVisible.Height / scale };
    auto const rootSize = root.Size();
    hostVisible = intersect(hostVisible, { 0, 0, rootSize.Width, rootSize.Height });
    auto const bodyToRoot = scroller.TransformToVisual(nullptr);
    auto const inViewport = [this, &scroller, &intersect, &hostVisible, &bodyToRoot](winrt::FrameworkElement const& element)
    {
        auto visible = intersect(
            element.TransformToVisual(scroller).TransformBounds(
                { 0, 0, static_cast<float>(element.ActualWidth()), static_cast<float>(element.ActualHeight()) }),
            { 0, 0, static_cast<float>(scroller.ViewportWidth()), static_cast<float>(scroller.ViewportHeight()) });
        for (winrt::DependencyObject node = element; node && visible.Width > 0 && visible.Height > 0;
             node = winrt::VisualTreeHelper::GetParent(node))
        {
            if (auto visual = node.try_as<winrt::UIElement>(); visual)
            {
                if (auto clip = visual.Clip())
                {
                    visible = intersect(visible, visual.TransformToVisual(scroller).TransformBounds(clip.Bounds()));
                    if (visible.Width <= 0 || visible.Height <= 0)
                    {
                        WatchTelemetryVisualProperty(visual, winrt::UIElement::ClipProperty());
                        WatchTelemetryVisualProperty(clip, winrt::RectangleGeometry::RectProperty());
                        WatchTelemetryVisualProperty(clip, winrt::Geometry::TransformProperty());
                        WatchTelemetryTransform(clip.Transform());
                    }
                }
            }
        }
        visible = intersect(bodyToRoot.TransformBounds(visible), hostVisible);
        if (visible.Width <= 0 || visible.Height <= 0)
        {
            for (winrt::DependencyObject node = element; node; node = winrt::VisualTreeHelper::GetParent(node))
            {
                if (auto visual = node.try_as<winrt::UIElement>())
                {
                    WatchTelemetryVisualProperty(visual, winrt::UIElement::RenderTransformProperty());
                    WatchTelemetryVisualProperty(visual, winrt::UIElement::RenderTransformOriginProperty());
                    WatchTelemetryTransform(visual.RenderTransform());
                }
            }
            return false;
        }
        return visible.Width > 0 && visible.Height > 0;
    };
    if (!inViewport(scroller)) { return false; }
    if (GetItemsSourceCount() != 0)
    {
        bool found = false;
        bool hasGroupHeader = false;
        auto const count = winrt::VisualTreeHelper::GetChildrenCount(repeater);
        for (int32_t index = 0; index < count && !found; ++index)
        {
            auto const child = winrt::VisualTreeHelper::GetChild(repeater, index).try_as<winrt::FrameworkElement>();
            if (!child || !child.IsLoaded() || repeater.GetElementIndex(child) < 0 ||
                child.ActualWidth() <= 0 || child.ActualHeight() <= 0 ||
                !hasVisiblePath(child) || !inViewport(child)) { continue; }
            if (child.try_as<winrt::TableViewGroupHeader>())
            {
                hasGroupHeader = true;
            }
            else if (auto row = child.try_as<winrt::TableViewRow>())
            {
                auto const rowImpl = winrt::get_self<TableViewRow>(row);
                auto const cells = rowImpl->GetCellsHostPanelInternal();
                if (rowImpl->GetOwningTableView() != *this || !cells) { continue; }
                for (auto const& cell : cells.Children())
                {
                    if (auto element = cell.try_as<winrt::FrameworkElement>();
                        element && element.Visibility() == winrt::Visibility::Visible &&
                        element.ActualWidth() > 0 && element.ActualHeight() > 0 &&
                        hasVisiblePath(element) && inViewport(element))
                    {
                        configuration.content = TableViewTelemetry::Content::Rows;
                        found = true;
                        break;
                    }
                }
            }
        }
        if (!found)
        {
            if (!hasGroupHeader) { return false; }
            configuration.content = TableViewTelemetry::Content::GroupHeaders;
        }
    }
    configuration = SnapshotTelemetryConfiguration(configuration.content, true);
    return true;
}

void TableView::OnTelemetryLayout()
{
    StopTelemetryLayout();
    if (m_telemetryMutationDepth || m_pendingGroupRowRefreshLayoutToken.value ||
        (!TableViewTelemetry::NeedsLayout(m_telemetry) &&
        !(m_scrollTelemetrySettled && TableViewTelemetry::IsOperationStarted(m_scrollTelemetry)))) { return; }
    auto const generation = m_telemetry.operationGeneration;
    auto const lifetime = m_telemetryLifetimeGeneration;
    auto const sourceGeneration = m_rowMetadataGeneration;
    auto const scrollGeneration = m_scrollTelemetry.operationGeneration;
    auto const scrollVersion = m_scrollTelemetryVersion;
    try
    {
        TableViewTelemetry::Configuration configuration;
        if (!TryGetTelemetryConfiguration(configuration))
        {
            WatchTelemetrySourceChanges();
            WatchTelemetryViewport();
            return;
        }
        StopTelemetryLayout();
        if (m_telemetryMutationDepth || m_pendingGroupRowRefreshLayoutToken.value ||
            lifetime != m_telemetryLifetimeGeneration ||
            sourceGeneration != m_rowMetadataGeneration || generation != m_telemetry.operationGeneration)
        {
            QueueTelemetryLayout();
            return;
        }
        if (TableViewTelemetry::NeedsLayout(m_telemetry))
        {
            if (TableViewTelemetry::UpdateConfiguration(m_telemetry, configuration))
            {
                TableViewTelemetry::CompleteInitial(m_telemetry, TableViewTelemetry::Result::Success);
                TableViewTelemetry::CompleteOperation(m_telemetry, generation, TableViewTelemetry::Result::Success);
                TableViewTelemetry::ReportUsage(m_telemetry, configuration);
            }
            else
            {
                TableViewTelemetry::IgnoreInitial(m_telemetry, TableViewTelemetry::IgnoreReason::Stale);
                TableViewTelemetry::IgnoreOperation(m_telemetry, generation, TableViewTelemetry::IgnoreReason::Stale);
            }
        }
        if (m_scrollTelemetrySettled && scrollVersion == m_scrollTelemetryVersion &&
            TableViewTelemetry::IsOperationStarted(m_scrollTelemetry))
        {
            if (TableViewTelemetry::UpdateConfiguration(m_scrollTelemetry, configuration))
            {
                TableViewTelemetry::CompleteOperation(m_scrollTelemetry, scrollGeneration, TableViewTelemetry::Result::Success);
                m_scrollTelemetrySettled = false;
            }
            else
            {
                IgnoreScrollTelemetry(TableViewTelemetry::IgnoreReason::Stale);
            }
        }
        if (!m_scrollTelemetryViewport.known)
        {
            if (auto const scroller = m_bodyScroller.get()) { ObserveScrollTelemetry(scroller, false, false); }
        }
    }
    catch (...)
    {
        StopTelemetryLayout();
        TableViewTelemetry::IgnoreOperation(m_telemetry, generation, TableViewTelemetry::IgnoreReason::Stale);
        TableViewTelemetry::IgnoreOperation(m_scrollTelemetry, scrollGeneration, TableViewTelemetry::IgnoreReason::Stale);
        OutputDebugStringW(L"TableView telemetry: usable layout could not be inspected.\n");
    }
}

uint64_t TableView::BeginOperationTelemetry(TableViewTelemetry::Operation operation, bool admitted) noexcept
{
    ++m_telemetryMutationDepth;
    StopTelemetryLayout();
    if (!admitted) { return 0; }
    IgnoreScrollTelemetry(TableViewTelemetry::IgnoreReason::Superseded);
    try
    {
        if (IsLoaded()) { return TableViewTelemetry::BeginOperation(m_telemetry, operation); }
    }
    catch (...) { OutputDebugStringW(L"TableView telemetry: operation attachment check failed.\n"); }
    return 0;
}

void TableView::EndOperationTelemetry() noexcept
{
    --m_telemetryMutationDepth;
    QueueTelemetryLayout();
}

void TableView::InvalidateOperationTelemetry() noexcept
{
    IgnoreScrollTelemetry(TableViewTelemetry::IgnoreReason::Superseded);
    if (!TableViewTelemetry::IsOperationStarted(m_telemetry)) { return; }
    // Control-owned sort writes notify inside their own scope; a Sorted handler can reshape
    // after that scope ends but before the enclosing telemetry operation unwinds.
    if (m_telemetryMutationDepth &&
        (m_telemetry.operation != TableViewTelemetry::Operation::Sort ||
            m_isApplyingControlInitiatedSort || m_telemetrySourceMutationDepth))
    {
        return;
    }
    StopTelemetryLayout();
    TableViewTelemetry::IgnoreOperation(m_telemetry, m_telemetry.operationGeneration, TableViewTelemetry::IgnoreReason::Superseded);
    QueueTelemetryLayout();
}

void TableView::FailOperationTelemetry(
    TableViewTelemetry::Operation operation, uint64_t generation, TableViewTelemetry::Stage stage, std::optional<HRESULT> error) noexcept
{
    if (m_telemetry.initial == TableViewTelemetry::InitialState::NotStarted) { return; }
    if (generation != m_telemetry.operationGeneration &&
        !(operation == TableViewTelemetry::Operation::InitialLayout && generation == 0)) { return; }
    auto const currentGeneration = m_telemetry.operationGeneration;
    auto const configuration = SnapshotTelemetryConfiguration(m_telemetry.configuration.content, m_telemetry.configuration.available);
    if (currentGeneration == m_telemetry.operationGeneration &&
        TableViewTelemetry::UpdateConfiguration(m_telemetry, configuration))
    {
        TableViewTelemetry::FailOperation(m_telemetry, operation, generation, stage, error);
    }
}

void TableView::IgnoreScrollTelemetry(TableViewTelemetry::IgnoreReason reason) noexcept
{
    TableViewTelemetry::IgnoreOperation(m_scrollTelemetry, m_scrollTelemetry.operationGeneration, reason);
    m_scrollTelemetrySettled = false;
    m_scrollTelemetryViewport.known = false;
    m_headerScrollTelemetryOffset.reset();
    ++m_scrollTelemetryVersion;
}

void TableView::ObserveScrollTelemetry(winrt::ScrollViewer const& scroller, bool intermediate, bool admit) noexcept
{
    try
    {
        ScrollTelemetryViewport const viewport{
            scroller.HorizontalOffset(), scroller.VerticalOffset(), scroller.ExtentWidth(), scroller.ExtentHeight(),
            scroller.ViewportWidth(), scroller.ViewportHeight(), scroller.ZoomFactor(), true };
        auto const& previous = m_scrollTelemetryViewport;
        auto const areClose = [](double left, double right) { return std::abs(left - right) < 0.5; };
        const bool horizontalChanged = !areClose(previous.horizontalOffset, viewport.horizontalOffset);
        const bool verticalChanged = !areClose(previous.verticalOffset, viewport.verticalOffset);
        const bool changed = horizontalChanged || verticalChanged || previous.zoomFactor != viewport.zoomFactor;
        const bool sameGeometry = areClose(previous.extentWidth, viewport.extentWidth) &&
            areClose(previous.extentHeight, viewport.extentHeight) &&
            areClose(previous.width, viewport.width) && areClose(previous.height, viewport.height);
        const bool headerSync = m_headerScrollTelemetryOffset && !verticalChanged &&
            areClose(*m_headerScrollTelemetryOffset, viewport.horizontalOffset);
        if (!admit || !previous.known || !sameGeometry || headerSync || !IsLoaded() || m_rowsSourceDrained)
        {
            IgnoreScrollTelemetry(TableViewTelemetry::IgnoreReason::Superseded);
            m_scrollTelemetryViewport = viewport;
            return;
        }
        m_scrollTelemetryViewport = viewport;
        if (changed)
        {
            m_headerScrollTelemetryOffset.reset();
            ++m_scrollTelemetryVersion;
            if (!TableViewTelemetry::IsOperationStarted(m_scrollTelemetry))
            {
                m_scrollTelemetry.origin = m_telemetry.origin;
                if (!TableViewTelemetry::UpdateConfiguration(m_scrollTelemetry,
                    SnapshotTelemetryConfiguration(m_telemetry.configuration.content, m_telemetry.configuration.available))) { return; }
                TableViewTelemetry::BeginOperation(m_scrollTelemetry, TableViewTelemetry::Operation::Scroll);
            }
        }
        if (TableViewTelemetry::IsOperationStarted(m_scrollTelemetry))
        {
            m_scrollTelemetrySettled = !intermediate;
        }
    }
    catch (...)
    {
        IgnoreScrollTelemetry(TableViewTelemetry::IgnoreReason::Stale);
        OutputDebugStringW(L"TableView telemetry: scroll viewport observation failed.\n");
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
        RefreshRowsPipeline();
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
            ObserveScrollTelemetry(bodyScroller, false, false);
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
                        strongThis->QueueTerminalGridLineRefresh();
                    }
                });

            // Resolve during the next table measure now that the viewport is known (initial layout).
            InvalidateMeasure();
            RefreshFrozenColumns();
            QueueTerminalGridLineRefresh();
        }
    }
}

void TableView::OnBodyScrollerViewChanged(
    const winrt::IInspectable& sender,
    const winrt::ScrollViewerViewChangedEventArgs& args)
{
    auto bodyScroller = m_bodyScroller.get();
    if (!bodyScroller || sender != bodyScroller)
    {
        return;
    }

    ObserveScrollTelemetry(bodyScroller, args.IsIntermediate());
    auto const generation = m_scrollTelemetry.operationGeneration;
    try
    {
        const double bodyHOffset = bodyScroller.HorizontalOffset();

        // Re-pin leading-frozen columns only when horizontal scroll moves.
        if (ShouldRefreshFrozenColumnsForScroll(this, bodyHOffset))
        {
            RefreshFrozenColumns();
        }

        const double bodyVOffset = bodyScroller.VerticalOffset();
        const bool horizontalMoved =
            !std::isfinite(m_terminalGridLineHorizontalOffset) ||
            std::abs(m_terminalGridLineHorizontalOffset - bodyHOffset) >= 0.25;
        const bool verticalMoved =
            !std::isfinite(m_terminalGridLineVerticalOffset) ||
            std::abs(m_terminalGridLineVerticalOffset - bodyVOffset) >= 0.25;

        if (horizontalMoved || verticalMoved)
        {
            m_terminalGridLineHorizontalOffset = bodyHOffset;
            m_terminalGridLineVerticalOffset = bodyVOffset;
            m_terminalGridLineGeometryRetryAvailable = true;
            RefreshTerminalGridLines();
        }

        if (auto headerScroller = m_headerScroller.get();
            headerScroller && std::abs(headerScroller.HorizontalOffset() - bodyHOffset) >= 0.5)
        {
            // Instant tracking, without near-equal ViewChanged ping-pong.
            headerScroller.ChangeView(bodyHOffset, nullptr, nullptr, true);
        }
    }
    catch (...)
    {
        if (TableViewTelemetry::UpdateConfiguration(m_scrollTelemetry,
            SnapshotTelemetryConfiguration(m_telemetry.configuration.content, m_telemetry.configuration.available)))
        {
            TableViewTelemetry::FailOperation(m_scrollTelemetry, TableViewTelemetry::Operation::Scroll,
                generation, TableViewTelemetry::Stage::Scroll, winrt::to_hresult());
        }
        throw;
    }
    if (m_scrollTelemetrySettled) { OnTelemetryLayout(); }
    QueueTelemetryLayout();
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

    auto const telemetryGeneration = BeginOperationTelemetry(TableViewTelemetry::Operation::ReplaceSource);
    bool completed = false;
    auto telemetryCompletion = wil::scope_exit([this, telemetryGeneration, &completed]() noexcept
    {
        if (!completed)
        {
            FailOperationTelemetry(TableViewTelemetry::Operation::ReplaceSource, telemetryGeneration, TableViewTelemetry::Stage::Source);
        }
        EndOperationTelemetry();
    });

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

    // The column set changed; reset the shared cursor so first header entry does not skip an
    // unvisited column.
    ResetColumnCursorInternal();

    // New data set: clear the grow-only Auto accumulators so widths recompute from scratch. The next
    // table measure pass pulls measured widths from the by-then re-realized rows, so the outgoing rows'
    // stale content no longer pins the columns.
    ResetColumnDesiredWidths();

    // A different collection invalidates any projection we synthesized over the old one, so the
    // shaping state that lived in it goes with it. AdoptItemsSource mints a fresh projection here
    // (the active source is still available to it as the previous source to detach), and
    // RefreshRowsPipeline then pushes the new view into the repeater. This is the only path that
    // reassigns the active source - every other re-entry keeps it and only refreshes the pipeline.

    // PART_RowsRepeater is driven from the flat ItemsSource DP. The sort belonged to the discarded
    // projection, so the reported sort state and the chevron go with it - otherwise they describe
    // an order the new rows are not in.
    ResetSortStateForNewItemsSource();
    AdoptItemsSource();
    RefreshRowsPipeline();
    completed = true;
}

void TableView::OnHeadersVisibilityPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (args.OldValue() == args.NewValue())
    {
        return;
    }

    UpdateHeaderVisibility();
    QueueTerminalGridLineRefresh();
}

void TableView::OnGridLinesVisibilityPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (args.OldValue() == args.NewValue())
    {
        return;
    }

    ApplyGridLinesToHeader();
    RefreshGridLinesOnRealizedRows();
    QueueTerminalGridLineRefresh();
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
    uint32_t lastVisibleHeaderCell = headerCellCount;
    for (uint32_t i = headerCellCount; i > 0; --i)
    {
        if (auto headerCell = headerCells.GetAt(i - 1).try_as<winrt::Panel>())
        {
            const auto column = headerCell.Tag().try_as<winrt::TableViewColumn>();
            if (headerCell.Visibility() == winrt::Visibility::Visible &&
                column &&
                column.ActualWidth() > 0.0)
            {
                lastVisibleHeaderCell = i - 1;
                break;
            }
        }
    }

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
                        border.Visibility(
                            wantVertical && !(m_suppressTrailingGridLine && i == lastVisibleHeaderCell)
                                ? winrt::Visibility::Visible
                                : winrt::Visibility::Collapsed);
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
    const auto terminalRow = m_terminalGridLineRow.get();
    ForEachRealizedRow([&](winrt::TableViewRow const& row)
    {
        const bool suppressBottom = terminalRow && IsSameObject(row, terminalRow) && m_suppressBottomGridLine;
        winrt::get_self<TableViewRow>(row)->SetTerminalGridLineSuppression({
            .suppressTrailing = m_suppressTrailingGridLine,
            .suppressBottom = suppressBottom });
    });
}

void TableView::QueueTerminalGridLineRefresh(bool isGeometryRetry)
{
    if (!isGeometryRetry)
    {
        m_terminalGridLineGeometryRetryAvailable = true;
    }

    if (m_terminalGridLinesLayoutToken.value)
    {
        return;
    }

    if (isGeometryRetry)
    {
        if (!m_terminalGridLineGeometryRetryAvailable)
        {
            return;
        }
        m_terminalGridLineGeometryRetryAvailable = false;
    }

    auto weakThis = get_weak();
    m_terminalGridLinesLayoutToken = LayoutUpdated(
        [weakThis](winrt::IInspectable const&, winrt::IInspectable const&)
        {
            if (auto strongThis = weakThis.get())
            {
                if (strongThis->m_terminalGridLinesLayoutToken.value)
                {
                    strongThis->LayoutUpdated(strongThis->m_terminalGridLinesLayoutToken);
                    strongThis->m_terminalGridLinesLayoutToken = {};
                }

                strongThis->RefreshTerminalGridLines();
            }
        });
}

std::optional<bool> TableView::ShouldSuppressTrailingGridLine()
{
    if (!WantsVerticalLines(GridLinesVisibility()))
    {
        return false;
    }

    const auto border = BorderThickness();
    // Both edges are read in the panel's logical coordinate space, which XAML mirrors wholesale
    // under RTL, so the logical trailing edge meets BorderThickness.Right in either direction.
    const double outerThickness = (std::max)(0.0, border.Right);
    if (outerThickness <= 0.0 || ActualWidth() <= 0.0)
    {
        return false;
    }

    winrt::FrameworkElement candidate{ nullptr };
    ForEachRealizedRow([&](winrt::TableViewRow const& row)
    {
        if (!candidate)
        {
            candidate = winrt::get_self<TableViewRow>(row)->GetLastVisibleCellInternal();
        }
    });

    if (!candidate && ShouldShowColumnHeaders())
    {
        if (auto host = m_headerHost.get())
        {
            const auto cells = host.Children();
            for (uint32_t i = cells.Size(); i > 0; --i)
            {
                if (auto cell = cells.GetAt(i - 1).try_as<winrt::FrameworkElement>();
                    cell &&
                    cell.Visibility() == winrt::Visibility::Visible &&
                    cell.ActualWidth() > 0.0)
                {
                    candidate = cell;
                    break;
                }
            }
        }
    }

    if (!candidate)
    {
        return false;
    }

    winrt::Rect bounds{};
    if (!TryGetBoundsRelativeTo(candidate, *this, bounds))
    {
        return std::nullopt;
    }

    // TransformToVisual reports the panel's logical coordinate space.
    const double candidateEdge = bounds.X + bounds.Width;
    const double outerEdge = ActualWidth() - outerThickness;
    return std::abs(candidateEdge - outerEdge) <= TerminalEdgeTolerance(*this);
}

std::optional<bool> TableView::ShouldSuppressBottomGridLine(
    const winrt::FrameworkElement& element,
    bool hasBottomGridLine)
{
    if (!element || !hasBottomGridLine)
    {
        return false;
    }

    const double bottomThickness = (std::max)(0.0, BorderThickness().Bottom);
    if (bottomThickness <= 0.0 || ActualHeight() <= 0.0)
    {
        return false;
    }

    winrt::Rect bounds{};
    if (!TryGetBoundsRelativeTo(element, *this, bounds))
    {
        return std::nullopt;
    }

    const double innerBottom = ActualHeight() - bottomThickness;
    return std::abs(bounds.Y + bounds.Height - innerBottom) <= TerminalEdgeTolerance(*this);
}

void TableView::RefreshTerminalGridLines()
{
    int32_t terminalColumnIndex = -1;
    if (auto columns = Columns())
    {
        for (uint32_t i = columns.Size(); i > 0; --i)
        {
            const auto column = columns.GetAt(i - 1);
            if (column &&
                column.Visibility() == winrt::Visibility::Visible &&
                column.ActualWidth() > 0.0)
            {
                terminalColumnIndex = static_cast<int32_t>(i - 1);
                break;
            }
        }
    }
    const bool terminalColumnChanged = terminalColumnIndex != m_terminalGridLineColumnIndex;
    m_terminalGridLineColumnIndex = terminalColumnIndex;

    const auto trailingResult = ShouldSuppressTrailingGridLine();
    const bool suppressTrailing = trailingResult.value_or(m_suppressTrailingGridLine);
    const bool trailingChanged = suppressTrailing != m_suppressTrailingGridLine;
    m_suppressTrailingGridLine = suppressTrailing;

    winrt::FrameworkElement terminalElement{ nullptr };
    winrt::TableViewRow terminalRow{ nullptr };
    winrt::TableViewGroupHeader terminalGroupHeader{ nullptr };
    bool terminalGeometryUnavailable = false;
    if (auto repeater = m_rowsRepeater.get())
    {
        // Select by geometry rather than by item index: once content overflows and scrolls, the
        // container meeting the inner bottom edge is not the last item.
        const double innerBottom = ActualHeight() - (std::max)(0.0, BorderThickness().Bottom);
        double closestDistance = std::numeric_limits<double>::infinity();
        const int32_t childCount = winrt::VisualTreeHelper::GetChildrenCount(repeater);
        for (int32_t i = 0; i < childCount; ++i)
        {
            const auto child =
                winrt::VisualTreeHelper::GetChild(repeater, i).try_as<winrt::FrameworkElement>();
            if (!child)
            {
                continue;
            }

            winrt::Rect bounds{};
            if (!TryGetBoundsRelativeTo(child, *this, bounds))
            {
                terminalGeometryUnavailable = true;
                continue;
            }

            if (const double distance = std::abs(bounds.Y + bounds.Height - innerBottom);
                distance < closestDistance)
            {
                closestDistance = distance;
                terminalElement = child;
            }
        }

        terminalRow = terminalElement.try_as<winrt::TableViewRow>();
        terminalGroupHeader = terminalElement.try_as<winrt::TableViewGroupHeader>();
    }

    const auto previousTerminalRow = m_terminalGridLineRow.get();
    const auto previousTerminalGroupHeader = m_terminalGridLineGroupHeader.get();
    const bool terminalChanged =
        !IsSameObject(previousTerminalRow, terminalRow) ||
        !IsSameObject(previousTerminalGroupHeader, terminalGroupHeader);
    if (terminalChanged)
    {
        if (previousTerminalRow)
        {
            winrt::get_self<TableViewRow>(previousTerminalRow)->SetTerminalGridLineSuppression({
                .suppressTrailing = m_suppressTrailingGridLine,
                .suppressBottom = false });
        }
        if (previousTerminalGroupHeader)
        {
            winrt::get_self<TableViewGroupHeader>(previousTerminalGroupHeader)
                ->SetTerminalBottomGridLineSuppression(false);
        }

        m_terminalGridLineRowSizeChangedRevoker.revoke();
        m_terminalGridLineRow = terminalRow ? winrt::make_weak(terminalRow) : nullptr;
        m_terminalGridLineGroupHeader =
            terminalGroupHeader ? winrt::make_weak(terminalGroupHeader) : nullptr;

        if (terminalElement)
        {
            auto weakThis = get_weak();
            m_terminalGridLineRowSizeChangedRevoker = terminalElement.SizeChanged(
                winrt::auto_revoke,
                [weakThis](winrt::IInspectable const&, winrt::SizeChangedEventArgs const&)
                {
                    if (auto strongThis = weakThis.get())
                    {
                        strongThis->QueueTerminalGridLineRefresh();
                    }
                });
        }
    }

    const bool hasBottomGridLine =
        terminalRow
            ? WantsHorizontalLines(GridLinesVisibility())
            : terminalGroupHeader && terminalGroupHeader.BorderThickness().Bottom > 0.0;
    auto bottomResult = terminalElement
        ? ShouldSuppressBottomGridLine(terminalElement, hasBottomGridLine)
        : std::optional<bool>{ false };
    // A container that failed its transform could be the real edge container, so a negative
    // result is only trustworthy once every container was measurable.
    if (terminalGeometryUnavailable && !bottomResult.value_or(false))
    {
        bottomResult = std::nullopt;
    }
    const bool suppressBottom = bottomResult.value_or(m_suppressBottomGridLine);
    m_suppressBottomGridLine = suppressBottom;

    if (trailingChanged || terminalColumnChanged)
    {
        ApplyGridLinesToHeader();
        RefreshGridLinesOnRealizedRows();
    }

    // Push unconditionally rather than only on a detected change. A container can be recycled or
    // re-prepared while this state is applied, so its own copy can disagree with the table's; a
    // change-gated push would leave that disagreement permanent. This is also the only thing that
    // re-derives the overlay from the container's current BorderThickness.
    if (auto repeater = m_rowsRepeater.get())
    {
        const int32_t childCount = winrt::VisualTreeHelper::GetChildrenCount(repeater);
        for (int32_t i = 0; i < childCount; ++i)
        {
            if (auto header = winrt::VisualTreeHelper::GetChild(repeater, i)
                    .try_as<winrt::TableViewGroupHeader>();
                header && !IsSameObject(header, terminalGroupHeader))
            {
                winrt::get_self<TableViewGroupHeader>(header)
                    ->SetTerminalBottomGridLineSuppression(false);
            }
        }
    }
    if (terminalRow)
    {
        winrt::get_self<TableViewRow>(terminalRow)->SetTerminalGridLineSuppression({
            .suppressTrailing = m_suppressTrailingGridLine,
            .suppressBottom = m_suppressBottomGridLine });
    }
    if (terminalGroupHeader)
    {
        winrt::get_self<TableViewGroupHeader>(terminalGroupHeader)
            ->SetTerminalBottomGridLineSuppression(m_suppressBottomGridLine);
    }

    if (!trailingResult.has_value() || !bottomResult.has_value())
    {
        QueueTerminalGridLineRefresh(true);
    }
    else
    {
        m_terminalGridLineGeometryRetryAvailable = true;
    }
}

void TableView::RefreshRowBackgroundsOnRealizedRows()
{
    ForEachRealizedRow([](winrt::TableViewRow const& row)
    {
        winrt::get_self<TableViewRow>(row)->RefreshRowBackground();
    });
}

void TableView::QueueGroupExpansionRowRefresh()
{
    if (m_pendingGroupRowRefreshLayoutToken.value)
    {
        return;
    }

    auto weakThis = get_weak();
    m_pendingGroupRowRefreshLayoutToken = LayoutUpdated(
        [weakThis](winrt::IInspectable const&, winrt::IInspectable const&)
        {
            auto strongThis = weakThis.get();
            if (!strongThis)
            {
                return;
            }

            if (strongThis->m_pendingGroupRowRefreshLayoutToken.value)
            {
                strongThis->LayoutUpdated(strongThis->m_pendingGroupRowRefreshLayoutToken);
                strongThis->m_pendingGroupRowRefreshLayoutToken = {};
            }

            // This coalesced phase repairs the current realized rows, not the projection that
            // first queued it. Bind failure ownership before any row factory can re-enter.
            const auto telemetryGeneration = strongThis->m_telemetry.operationGeneration;
            const auto scrollGeneration = strongThis->m_scrollTelemetry.operationGeneration;
            const auto lifetime = strongThis->m_telemetryLifetimeGeneration;
            strongThis->BeginOperationTelemetry(TableViewTelemetry::Operation::Layout, false);
            auto telemetryCompletion = wil::scope_exit([&strongThis]() noexcept
            {
                strongThis->EndOperationTelemetry();
            });
            try
            {
                strongThis->RefreshRealizedRowsAfterGroupExpansion();
            }
            catch (...)
            {
                const auto error = winrt::to_hresult();
                if (lifetime == strongThis->m_telemetryLifetimeGeneration)
                {
                    strongThis->FailOperationTelemetry(TableViewTelemetry::Operation::Layout,
                        telemetryGeneration, TableViewTelemetry::Stage::Layout, error);
                    if (lifetime == strongThis->m_telemetryLifetimeGeneration &&
                        scrollGeneration == strongThis->m_scrollTelemetry.operationGeneration &&
                        TableViewTelemetry::IsOperationStarted(strongThis->m_scrollTelemetry) &&
                        TableViewTelemetry::UpdateConfiguration(strongThis->m_scrollTelemetry,
                            strongThis->SnapshotTelemetryConfiguration(strongThis->m_telemetry.configuration.content,
                                strongThis->m_telemetry.configuration.available)))
                    {
                        TableViewTelemetry::FailOperation(strongThis->m_scrollTelemetry, TableViewTelemetry::Operation::Scroll,
                            scrollGeneration, TableViewTelemetry::Stage::Scroll, error);
                    }
                }
                // Best-effort repair after a deferred grouped reshape.
            }
        });
}

void TableView::RefreshRealizedRowsAfterGroupExpansion()
{
    ForEachRealizedRow([this](winrt::TableViewRow const& row)
    {
        auto* const rowImpl = winrt::get_self<TableViewRow>(row);
        rowImpl->EnsureOwningTableViewInternal(*this);
        RefreshRowSelectionState(row);
    });

    InvalidateMeasure();
}

void TableView::AdoptItemsSource()
{
    auto const itemsSource = ItemsSource();
    auto tableViewSource = itemsSource.try_as<winrt::TableViewSource>();

    // Normalize the source. When the app hands us a plain collection, project it through a
    // TableViewSource of our own so the control has exactly one row pipeline rather than a shaped
    // path and a raw one. This mirrors ItemsControl, which always routes ItemsSource through a
    // collection view, so the grid above it never has to ask what kind of source it was given.
    //
    // This runs only when ItemsSource actually changes, so there is never a prior projection to
    // reuse here - a different collection invalidated it, and the re-entries that must keep the
    // active projection (OnApplyTemplate, repeater Loaded, a shaping verb) go through
    // RefreshRowsPipeline and never reach this method.
    if (!tableViewSource && itemsSource)
    {
        // Deliberately unguarded. The shaping engine accepts exactly the collection interfaces
        // ItemsSourceView does, so a source it refuses is one XAML cannot project either, and
        // ItemsRepeater raises that as an error rather than degrading. Let it surface.
        tableViewSource = winrt::TableViewSource::From(itemsSource);
    }

    // Detach the previous source before adopting the new one. Swapping ItemsSource between two
    // TableViewSources leaves the old one alive and still subscribed to the app's collection, so
    // without this it keeps a back-pointer to this control and a later rebuild of that discarded
    // source would drive a TableView it no longer belongs to.
    if (auto const previouslyOwned = m_activeSource.get(); previouslyOwned && previouslyOwned != tableViewSource)
    {
        winrt::get_self<::TableViewSource>(previouslyOwned)->SetOwningTableView(nullptr);
    }

    m_activeSource.set(tableViewSource);

    if (tableViewSource)
    {
        auto* const sourceImpl = winrt::get_self<::TableViewSource>(tableViewSource);
        sourceImpl->SetOwningTableView(*this);

        // The source raises this rather than calling back into TableView by name, so the shaping
        // stack stays below the control. Weak, because the control owns the source through
        // m_activeSource and a strong capture would be a cycle.
        auto weakThis = get_weak();
        sourceImpl->SetProjectionChangedHandler([weakThis]()
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->OnTableViewSourceProjectionChanged();
            }
        });
        sourceImpl->SetShapingChangedHandler([weakThis](bool reorderOnly)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->OnTableViewSourceShapingChanged(reorderOnly);
            }
        });
        sourceImpl->SetShapingOperationHandlers(
            [weakThis](TableViewTelemetry::Operation operation, bool admitted) noexcept -> uint64_t
            {
                if (auto self = weakThis.get())
                {
                    ++self->m_telemetrySourceMutationDepth;
                    if (operation == TableViewTelemetry::Operation::Sort && self->m_isApplyingControlInitiatedSort)
                    {
                        ++self->m_telemetryMutationDepth;
                        return admitted && self->m_controlSortTelemetryGeneration &&
                            self->m_controlSortTelemetryGeneration == self->m_telemetry.operationGeneration &&
                            self->m_telemetry.operation == operation &&
                            TableViewTelemetry::IsOperationStarted(self->m_telemetry) ?
                            self->m_controlSortTelemetryGeneration : 0;
                    }
                    return self->BeginOperationTelemetry(operation, admitted);
                }
                return 0;
            },
            [weakThis](TableViewTelemetry::Operation operation, uint64_t generation,
                TableViewTelemetry::Result result, std::optional<HRESULT> error) noexcept
            {
                if (auto self = weakThis.get())
                {
                    if (generation && result == TableViewTelemetry::Result::Failure)
                    {
                        auto const stage = operation == TableViewTelemetry::Operation::Filter ? TableViewTelemetry::Stage::Filter :
                            operation == TableViewTelemetry::Operation::Sort ? TableViewTelemetry::Stage::Sort : TableViewTelemetry::Stage::Grouping;
                        self->FailOperationTelemetry(operation, generation, stage, error);
                    }
                    else if (generation && result == TableViewTelemetry::Result::Cancelled &&
                        !self->m_isApplyingControlInitiatedSort)
                    {
                        TableViewTelemetry::IgnoreOperation(self->m_telemetry, generation, TableViewTelemetry::IgnoreReason::Stale);
                    }
                    --self->m_telemetrySourceMutationDepth;
                    self->EndOperationTelemetry();
                }
            });
    }
}

void TableView::RefreshRowsPipeline()
{
    // Recompute the cached row view from whatever the active source currently projects. A shaping
    // verb can swap the projection underneath us, so this is refreshed on every re-entry, not just
    // on a source change.
    winrt::IInspectable rowsSource{ nullptr };

    // Held so the generation bump below can tell a genuine provider swap from a re-entry that
    // merely re-reads the same one.
    auto const previousRowMetadata = m_tableViewSourceRowMetadata;
    m_tableViewSourceRowMetadata = nullptr;

    if (auto const activeSource = m_activeSource.get())
    {
        auto* const sourceImpl = winrt::get_self<::TableViewSource>(activeSource);
        // Straight through: for a TableViewSource the row view IS the projection's view.
        m_rowsItemsSourceView = sourceImpl->GetItemsSourceView();
        m_tableViewSourceRowMetadata = sourceImpl->GetRowMetadata();
        rowsSource = m_rowsItemsSourceView ? m_rowsItemsSourceView.as<winrt::IInspectable>() : nullptr;
    }
    else
    {
        // Null ItemsSource: nothing to project, so the repeater empties out below.
        m_rowsItemsSourceView = nullptr;
    }

    // Bump only when the provider that produced previously handed-out row identities has actually
    // been replaced, so a request captured against the old one can tell it is stale. Identities are
    // value-based strings, so without the bump the same string could name an unrelated group in a
    // new projection. Bumping unconditionally is equally wrong in the other direction: this method
    // also runs on re-entries that keep the very same projection (OnApplyTemplate, a Loaded repump
    // after an unload drain, an applied sort), and a bump there silently discards a queued group
    // toggle that is still perfectly valid.
    if (m_tableViewSourceRowMetadata != previousRowMetadata)
    {
        ++m_rowMetadataGeneration;
    }

    if (auto repeater = m_rowsRepeater.get())
    {
        // ItemsRepeater has no identity short-circuit: re-assigning the same source tears down
        // every container and resets scroll. Guard so a theme-change or Loaded repump does not
        // blow away realized rows.
        if (!IsSameObject(repeater.ItemsSource(), rowsSource))
        {
            repeater.ItemsSource(rowsSource);
        }

        UpdateItemsSourceCollectionChangedSubscription();
        UpdateEmptyState();

        // Re-point selection at the new source. SelectionModel::Source clears unconditionally, so a
        // swap always drops the selection; then drain anything requested before a source existed.
        ResolveSelectionAfterSourceChange();
    }
    // else: OnApplyTemplate hasn't run yet; the repeater will be sourced from there.
}

void TableView::OnTableViewSourceProjectionChanged()
{
    InvalidateOperationTelemetry();
    // A shaping verb swapped the projected shape after we bound, so the cached view and row
    // metadata describe the previous projection. Re-read them and re-drive the rows.
    if (IsEditing())
    {
        // The edited item may not exist in the new projection. Forced, for the same reason as an
        // ItemsSource swap: the shape is already gone by the time a handler could veto it.
        TerminateEditWithoutVisualRestore();
    }

    RefreshRowsPipeline();
    QueueGroupExpansionRowRefresh();
}

void TableView::OnTableViewSourceShapingChanged(bool reorderOnly)
{
    InvalidateOperationTelemetry();
    if (!reorderOnly)
    {
        QueueGroupExpansionRowRefresh();
    }
    // The app may have declared or cleared a sort straight on the source, which the control has no
    // other way to learn about. Reconcile before anything else so the chevrons never outlive the
    // axis they describe.
    if (!m_isApplyingControlInitiatedSort)
    {
        QueueReconcileSortStateWithSource();
    }

    // A programmatic shaping verb rewrites the projection with no input event behind it, so
    // nothing else tells a UIA client that the rows it cached are stale. A pure re-order keeps the
    // same children in a new order; anything else can add or remove them.
    if (!winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::StructureChanged))
    {
        return;
    }

    auto peer = winrt::FrameworkElementAutomationPeer::FromElement(*this);
    if (!peer)
    {
        return;
    }

    if (auto const tableViewPeer = peer.try_as<winrt::TableViewAutomationPeer>())
    {
        auto* const impl = winrt::get_self<TableViewAutomationPeer>(tableViewPeer);
        if (reorderOnly)
        {
            impl->RaiseStructureChangedForSortChange();
        }
        else
        {
            impl->RaiseStructureChangedForVirtualizationReset();
        }
    }
}

void TableView::OnEmptyTemplatePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    // Skip same-value sets (no re-subscription / re-evaluation), matching the other DP callbacks.
    if (args.OldValue() == args.NewValue())
    {
        return;
    }

    UpdateItemsSourceCollectionChangedSubscription();
    UpdateEmptyState();
}

void TableView::UpdateItemsSourceCollectionChangedSubscription()
{
    // Count changes also move the terminal row separator, so keep this subscription even when no
    // EmptyTemplate is configured.
    m_itemsSourceCollectionChangedRevoker = {};
    if (auto repeater = m_rowsRepeater.get())
    {
        if (auto view = repeater.ItemsSourceView())
        {
            m_itemsSourceCollectionChangedRevoker = view.CollectionChanged(
                winrt::auto_revoke, { this, &TableView::OnItemsSourceCollectionChanged });
        }
    }
}

void TableView::OnItemsSourceCollectionChanged(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/)
{
    UpdateEmptyState();
    QueueTerminalGridLineRefresh();
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
        rowImpl->EnsureOwningTableViewInternal(*this);
        rowImpl->SetTerminalGridLineSuppression({
            .suppressTrailing = m_suppressTrailingGridLine,
            .suppressBottom = false });
        rowImpl->RefreshRowBackground();
        RefreshRowSelectionState(row);
        InvalidateMeasure();
        QueueTerminalGridLineRefresh();
    }
    else if (auto header = args.Element().try_as<winrt::TableViewGroupHeader>())
    {
        PrepareGroupHeaderElement(header, args.Index());
        InvalidateMeasure();
        QueueTerminalGridLineRefresh();
    }
    QueueTelemetryLayout();
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

        auto const rowImpl = winrt::get_self<TableViewRow>(row);
        // Release app-supplied tooltip content rather than pinning it in the recycle pool.
        rowImpl->ReleaseCellToolTips();
        rowImpl->SetTerminalGridLineSuppression({});
        if (auto terminalRow = m_terminalGridLineRow.get(); terminalRow && IsSameObject(row, terminalRow))
        {
            m_terminalGridLineRowSizeChangedRevoker.revoke();
            m_terminalGridLineRow = {};
            m_suppressBottomGridLine = false;
        }
        rowImpl->SetOwningTableViewInternal(nullptr);
        // Grouped reshapes can rebind a pooled row through DataContext without a fresh
        // ElementPrepared/ElementIndexChanged callback. Keep the weak owner available so that path
        // can rebuild cells and publish a live row peer; item-identity tracking still rejects stale
        // peers after the rebind.
        rowImpl->EnsureOwningTableViewInternal(*this);
        InvalidateMeasure();
        QueueTerminalGridLineRefresh();
    }
    else if (auto header = args.Element().try_as<winrt::TableViewGroupHeader>())
    {
        winrt::get_self<TableViewGroupHeader>(header)->SetTerminalBottomGridLineSuppression(false);
        if (auto terminalHeader = m_terminalGridLineGroupHeader.get();
            terminalHeader && IsSameObject(header, terminalHeader))
        {
            m_terminalGridLineRowSizeChangedRevoker.revoke();
            m_terminalGridLineGroupHeader = {};
            m_suppressBottomGridLine = false;
        }
        ClearGroupHeaderElement(header);
        InvalidateMeasure();
        QueueTerminalGridLineRefresh();
    }
    QueueTelemetryLayout();
}

void TableView::OnRowElementIndexChanged(
    const winrt::ItemsRepeater& /*sender*/,
    const winrt::ItemsRepeaterElementIndexChangedEventArgs& args)
{
    auto const row = args.Element().try_as<winrt::TableViewRow>();
    if (!row)
    {
        // A realized header keeps its element but moves to a new index, and its expansion state is
        // read from that index's metadata, so it has to be re-prepared.
        if (auto const header = args.Element().try_as<winrt::TableViewGroupHeader>())
        {
            PrepareGroupHeaderElement(header, args.NewIndex());
            QueueTerminalGridLineRefresh();
        }
        return;
    }

    auto rowImpl = winrt::get_self<TableViewRow>(row);
    // Realized rows can be preserved through grouped projection reshapes without a fresh
    // ElementPrepared callback, so keep the owner/cells invariant true on index changes too.
    rowImpl->SetOwningTableViewInternal(*this);
    rowImpl->SetTerminalGridLineSuppression({
        .suppressTrailing = m_suppressTrailingGridLine,
        .suppressBottom = false });
    QueueTerminalGridLineRefresh();

    // Realized rows keep their element but get a new index, so banding parity must refresh.
    if (RowBackground() != nullptr || AlternatingRowBackground() != nullptr)
    {
        rowImpl->RefreshRowBackground();
    }

    // ...and so must selected chrome. The element keeps its item here (only its index moved), so
    // this normally re-derives the same answer - it is the cheap guarantee that a row whose index
    // shifted under an insert cannot end up disagreeing with the model.
    RefreshRowSelectionState(row);
}

// One header-text extraction per column, shared by the header cell's automation name and the
// gripper's OwnerName.
static winrt::hstring GetColumnHeaderText(const winrt::TableViewColumn& column)
{
    if (auto const header = column.Header())
    {
        if (auto const stringable = header.try_as<winrt::IStringable>())
        {
            return stringable.ToString();
        }
        if (auto const propValue = header.try_as<winrt::IPropertyValue>();
            propValue && propValue.Type() == winrt::PropertyType::String)
        {
            return propValue.GetString();
        }
    }

    return {};
}

// GetColumnHeaderText renders any IStringable for automation purposes, but only a genuine string
// may be swapped for a TextBlock -- a UIElement or a type with an implicit DataTemplate must keep
// the ContentPresenter's content model.
static bool IsPlainStringHeader(const winrt::TableViewColumn& column)
{
    const auto propValue = column.Header().try_as<winrt::IPropertyValue>();
    return propValue && propValue.Type() == winrt::PropertyType::String;
}

void TableView::ReleaseHeaderToolTips(const winrt::Panel& host)
{
    if (!host)
    {
        return;
    }

    // Snapshot: Closed runs app code that can re-enter RebuildHeaders and clear this collection.
    auto const children = host.Children();
    std::vector<winrt::FrameworkElement> cells;
    cells.reserve(children.Size());
    for (uint32_t i = 0; i < children.Size(); ++i)
    {
        if (auto const headerCell = children.GetAt(i).try_as<winrt::FrameworkElement>())
        {
            cells.push_back(headerCell);
        }
    }

    for (auto const& headerCell : cells)
    {
        TableViewDetails::ClearOwnedToolTip(headerCell);
    }
}

void TableView::RebuildHeaders()
{
    m_headerSortSpaceArmedColumn = nullptr;
    auto host = m_headerHost.get();
    if (!host)
    {
        return;
    }

    // Clearing the host under a live popup tears its child down reentrantly.
    ReleaseHeaderToolTips(host);

    host.Children().Clear();

    // Cache theme-resource padding once per header rebuild; values are stable for the pass.
    winrt::Thickness cachedHeaderCellPadding = GetDensityHeaderCellPadding();
    // Always cache the header grid-line brush at rebuild time; visibility toggles do not reassign it later.
    const bool wantVerticalHeaderLines = WantsVerticalLines(GridLinesVisibility());
    const auto cachedHeaderGridLineBrush = GetGridLineBrush();
    // Header cells share the density row min-height so the header band matches the body rows.
    const double cachedRowMinHeight = GetDensityRowMinHeight();
    const double cachedHeaderFontSize = GetHeaderFontSize();
    const winrt::Brush cachedHeaderCellFill = winrt::SolidColorBrush{ winrt::Colors::Transparent() };
    // unbox_value_or, not unbox_value: the key is app-overridable and a non-double would throw out
    // of RebuildHeaders; a non-positive or non-finite override would flow straight into Width().
    double cachedResizeGripperWidth = winrt::unbox_value_or<double>(
        LookupElementResource(*this, s_ResizeGripperWidthKey), c_resizeGripperWidthFallback);
    if (!std::isfinite(cachedResizeGripperWidth) || cachedResizeGripperWidth <= 0.0)
    {
        cachedResizeGripperWidth = c_resizeGripperWidthFallback;
    }
    double cachedSortIndicatorWidth = winrt::unbox_value_or<double>(
        LookupElementResource(*this, s_SortIndicatorSizeKey), c_sortIndicatorSizeFallback);
    if (!std::isfinite(cachedSortIndicatorWidth) || cachedSortIndicatorWidth <= 0.0)
    {
        cachedSortIndicatorWidth = c_sortIndicatorSizeFallback;
    }
    const bool canUserSortColumns = CanUserSortColumns();

    // Logical-end (trailing) edge alignment must mirror under RTL. The header cell's subtree does
    // not observe the ambient FlowDirection auto-flip, so read the control's FlowDirection and swap
    // explicitly. This is a build-time stamp, kept current because a runtime FlowDirection flip
    // rebuilds the headers.
    const bool isRightToLeft = FlowDirection() == winrt::FlowDirection::RightToLeft;
    const auto logicalEndAlignment = isRightToLeft ? winrt::HorizontalAlignment::Left : winrt::HorizontalAlignment::Right;

    if (auto columns = Columns())
    {
        for (auto const& column : columns)
        {
            if (!column || winrt::get_self<TableViewColumn>(column)->GetOwningTableView() != *this)
            {
                continue;
            }

            // Header cell root: TableViewHeaderCell is the focus/hit-test target so its automation
            // peer attaches to the right element. Content and chevron get separate columns so the
            // chevron reserves width instead of overlaying text.
            auto const headerCell = winrt::make<TableViewHeaderCell>(*this, column).as<winrt::Grid>();
            const int contentColumnIndex = isRightToLeft ? 1 : 0;
            const int indicatorColumnIndex = isRightToLeft ? 0 : 1;
            {
                winrt::ColumnDefinition starColumn;
                starColumn.Width(winrt::GridLengthHelper::FromValueAndType(1, winrt::GridUnitType::Star));
                winrt::ColumnDefinition autoColumn;
                autoColumn.Width(winrt::GridLengthHelper::FromValueAndType(0, winrt::GridUnitType::Auto));
                if (isRightToLeft)
                {
                    headerCell.ColumnDefinitions().Append(autoColumn);
                    headerCell.ColumnDefinitions().Append(starColumn);
                }
                else
                {
                    headerCell.ColumnDefinitions().Append(starColumn);
                    headerCell.ColumnDefinitions().Append(autoColumn);
                }
            }
            headerCell.Visibility(column.Visibility());
            // Header cells are the named keyboard/UIA targets; the host is one Tab stop, and arrows
            // must still reach every visible header. Every visible header is a tab stop by product
            // decision, including non-actionable ones: ARIA permits skipping them, but skipping makes
            // the band's keyboard model depend on per-column capability, which is harder to explain
            // than one uniform rule.
            const bool headerIsResizable = CanUserResizeColumns() && column.CanResize();
            const bool headerIsSortable = canUserSortColumns && column.CanSort();
            headerCell.IsTabStop(true);
            headerCell.UseSystemFocusVisuals(true);
            const winrt::hstring headerText = GetColumnHeaderText(column);
            if (!headerText.empty())
            {
                winrt::AutomationProperties::SetName(headerCell, headerText);
            }
            winrt::AutomationProperties::SetAccessibilityView(headerCell, winrt::AccessibilityView::Content);
            headerCell.MinHeight(cachedRowMinHeight);
            // Without a fill the padding takes no pointer input, killing the tooltip and
            // click-to-sort there.
            headerCell.Background(cachedHeaderCellFill);


            winrt::ContentPresenter content;
            content.Content(column.Header());
            // Header peer already names this subtree; leaving it in Content view double-announces.
            winrt::AutomationProperties::SetAccessibilityView(content, winrt::AccessibilityView::Raw);
            if (auto headerTemplateSelector = column.HeaderTemplateSelector())
            {
                content.Content(column.Header());
                content.ContentTemplateSelector(headerTemplateSelector);
            }
            else if (auto headerTemplate = column.HeaderTemplate())
            {
                content.Content(column.Header());
                content.ContentTemplate(headerTemplate);
            }
            else if (!headerText.empty() && IsPlainStringHeader(column))
            {
                // A ContentPresenter renders a bare string through an implicit TextBlock carrying no
                // TextTrimming, so a too-wide header hard-clips mid-glyph while its cells ellipsize.
                winrt::TextBlock headerBlock;
                headerBlock.Text(headerText);
                headerBlock.TextTrimming(winrt::TextTrimming::CharacterEllipsis);
                headerBlock.VerticalAlignment(winrt::VerticalAlignment::Center);
                // The header cell's peer already announces this text.
                winrt::AutomationProperties::SetAccessibilityView(headerBlock, winrt::AccessibilityView::Raw);
                content.Content(headerBlock);
            }
            // Consume TableViewHeaderCellPadding from theme resources (cached once per rebuild).
            // Sortable headers reserve the chevron's themed width so text trims before the overlay.
            auto contentPadding = cachedHeaderCellPadding;
            if (headerIsSortable)
            {
                if (isRightToLeft)
                {
                    contentPadding.Left += cachedSortIndicatorWidth;
                }
                else
                {
                    contentPadding.Right += cachedSortIndicatorWidth;
                }
            }
            content.Padding(contentPadding);
            content.HorizontalAlignment(winrt::HorizontalAlignment::Stretch);
            content.VerticalAlignment(winrt::VerticalAlignment::Center);
            // Column-header text: theme font size, SemiBold to stand out from cells (templates override).
            content.FontSize(cachedHeaderFontSize);
            content.FontWeight(winrt::FontWeights::SemiBold());
            winrt::Grid::SetColumn(content, contentColumnIndex);
            headerCell.Children().Append(content);

            // Resolve from TableView so header grid lines track theme.
            {
                winrt::Border headerGridLine;
                headerGridLine.Name(winrt::hstring{ s_HeaderGridLineName });
                headerGridLine.Width(1);
                headerGridLine.HorizontalAlignment(logicalEndAlignment);
                headerGridLine.IsHitTestVisible(false);
                winrt::AutomationProperties::SetAccessibilityView(headerGridLine, winrt::AccessibilityView::Raw);
                headerGridLine.Visibility(wantVerticalHeaderLines ? winrt::Visibility::Visible : winrt::Visibility::Collapsed);
                headerGridLine.Background(cachedHeaderGridLineBrush);
                winrt::Grid::SetColumnSpan(headerGridLine, 2);
                headerCell.Children().Append(headerGridLine);
            }

            headerCell.Tag(column);

            TableViewDetails::ApplyHeaderToolTip(headerCell, column.HeaderToolTip());

            if (headerIsSortable)
            {
                // Hosted in its own panel so the chevron sits on the logical trailing edge without
                // competing with the header content's Stretch alignment.
                winrt::StackPanel indicatorHost;
                indicatorHost.Orientation(winrt::Orientation::Horizontal);
                indicatorHost.HorizontalAlignment(logicalEndAlignment);
                indicatorHost.VerticalAlignment(winrt::VerticalAlignment::Center);
                // The chevron is decoration on top of a clickable header: letting it take the hit
                // would create a dead spot in the middle of the click target.
                indicatorHost.IsHitTestVisible(false);
                // SortIndicator has a fixed themed Width and only fades via Opacity, so an always-
                // visible host would cost that width on every sortable column.
                indicatorHost.Visibility(column.SortDirection() == winrt::SortDirection::None
                    ? winrt::Visibility::Collapsed : winrt::Visibility::Visible);
                AppendSortIndicatorVisual(indicatorHost, column);
                winrt::Grid::SetColumn(indicatorHost, indicatorColumnIndex);
                headerCell.Children().Append(indicatorHost);

                // Weak: the handler is owned by a visual the control also owns, so a strong
                // capture would keep the TableView alive through its own header.
                auto weakThis = get_weak();
                headerCell.Tapped([weakThis, column](auto const&, winrt::TappedRoutedEventArgs const& args)
                {
                    if (auto strongThis = weakThis.get())
                    {
                        if (strongThis->ToggleSortDirection(column))
                        {
                            args.Handled(true);
                        }
                    }
                });
            }

            // Last, so it wins the hit test on the edge it shares with the grid line and the sort
            // affordance. The gripper marks the press handled, which also keeps a resize drag from
            // reaching the header's Tapped handler and sorting the column.
            if (headerIsResizable)
            {
                AppendResizeGripperVisual(headerCell, column, cachedResizeGripperWidth, headerText, logicalEndAlignment);
            }

            host.Children().Append(headerCell);
        }
    }

    // Pin (or refresh) leading-frozen header cells at the current scroll offset.
    RefreshFrozenColumns();
    ApplyGridLinesToHeader();
}

// Builds the chevron for one header. The indicator is a display-only primitive: it owns no sort
// policy, so the control sets Direction and nothing else.
void TableView::AppendSortIndicatorVisual(const winrt::Panel& host, const winrt::TableViewColumn& column)
{
    if (!host || !column)
    {
        return;
    }

    winrt::SortIndicator indicator;
    indicator.Name(winrt::hstring{ s_SortIndicatorName });
    indicator.VerticalAlignment(winrt::VerticalAlignment::Center);
    indicator.Direction(ToSortIndicatorDirection(column.SortDirection()));
    // The header cell already reports the sort state through its automation peer's help text;
    // surfacing the chevron separately would make AT announce the same thing twice.
    winrt::AutomationProperties::SetAccessibilityView(indicator, winrt::AccessibilityView::Raw);
    host.Children().Append(indicator);
}

winrt::SortIndicatorDirection TableView::ToSortIndicatorDirection(winrt::SortDirection direction)
{
    // Two distinct WinRT enums with matching numeric values; map explicitly rather than casting so
    // a future divergence is a compile error rather than a wrong glyph.
    switch (direction)
    {
    case winrt::SortDirection::Ascending:
        return winrt::SortIndicatorDirection::Ascending;
    case winrt::SortDirection::Descending:
        return winrt::SortIndicatorDirection::Descending;
    case winrt::SortDirection::None:
    default:
        return winrt::SortIndicatorDirection::None;
    }
}

winrt::SortIndicator TableView::FindSortIndicator(const winrt::Panel& root)
{
    if (!root)
    {
        return nullptr;
    }

    // The chevron lives one level down inside its own host StackPanel today, but recurse so a
    // future template tweak that nests it deeper does not silently break the refresh.
    for (auto const& child : root.Children())
    {
        if (auto const indicator = child.try_as<winrt::SortIndicator>())
        {
            return indicator;
        }
        if (auto const childPanel = child.try_as<winrt::Panel>())
        {
            if (auto const found = FindSortIndicator(childPanel))
            {
                return found;
            }
        }
    }

    return nullptr;
}

void TableView::RefreshSortIndicators()
{
    auto host = m_headerHost.get();
    if (!host)
    {
        return;
    }

    for (auto const& child : host.Children())
    {
        auto const headerCell = child.try_as<winrt::Panel>();
        if (!headerCell)
        {
            continue;
        }

        auto const column = headerCell.Tag().try_as<winrt::TableViewColumn>();
        if (!column)
        {
            continue;
        }

        // A child walk by type rather than FindName: the indicator is code-created into a nested
        // host panel, so its Name was never registered in a namescope and FindName returns null --
        // which left a programmatic sort (no header rebuild) with a stale chevron.
        if (auto const indicator = FindSortIndicator(headerCell))
        {
            const auto direction = column.SortDirection();
            indicator.Direction(ToSortIndicatorDirection(direction));
            // Keep the reserved column in step with the chevron.
            if (auto const indicatorHost = indicator.Parent().try_as<winrt::UIElement>())
            {
                indicatorHost.Visibility(direction == winrt::SortDirection::None
                    ? winrt::Visibility::Collapsed : winrt::Visibility::Visible);
            }
        }
    }
}

void TableView::OnCanUserSortColumnsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    // The gate turning off must also drop any sort it was responsible for; leaving the rows in a
    // sorted order with no affordance to change it would strand the user.
    if (!CanUserSortColumns())
    {
        ClearSortInternal(false);
    }

    // The chevron and the click handler are stamped at header-build time.
    QueueRebuildHeaders();
}

void TableView::OnColumnCanSortChanged(const winrt::TableViewColumn& column){
    // A column that just opted out must not keep an active sort applied to it.
    if (column && !column.CanSort() && column.SortDirection() != winrt::SortDirection::None)
    {
        SortByColumnInternal(column, winrt::SortDirection::None, false);
    }

    // The chevron and the click handler are stamped at header-build time.
    QueueRebuildHeaders();
}

double TableView::GetHeaderMeasuredWidthForColumn(const winrt::TableViewColumn& column) const{
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
                    TableViewTelemetry::ReportError(strongThis->m_telemetry,
                        TableViewTelemetry::Operation::HeaderRefresh, TableViewTelemetry::Stage::HeaderRefresh, true);
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
    m_headerSortSpaceArmedColumn = nullptr;
    StopTelemetryLayout();
    IgnoreTelemetry(TableViewTelemetry::IgnoreReason::Unloaded);
    if (m_pendingFocusLayoutToken.value)
    {
        LayoutUpdated(m_pendingFocusLayoutToken);
        m_pendingFocusLayoutToken = {};
    }

    if (m_pendingGroupFocusLayoutToken.value)
    {
        LayoutUpdated(m_pendingGroupFocusLayoutToken);
        m_pendingGroupFocusLayoutToken = {};
    }
    if (m_pendingGroupRowRefreshLayoutToken.value)
    {
        LayoutUpdated(m_pendingGroupRowRefreshLayoutToken);
        m_pendingGroupRowRefreshLayoutToken = {};
    }
    if (m_terminalGridLinesLayoutToken.value)
    {
        LayoutUpdated(m_terminalGridLinesLayoutToken);
        m_terminalGridLinesLayoutToken = {};
    }
    if (auto terminalRow = m_terminalGridLineRow.get())
    {
        winrt::get_self<TableViewRow>(terminalRow)->SetTerminalGridLineSuppression({});
    }
    if (auto terminalHeader = m_terminalGridLineGroupHeader.get())
    {
        winrt::get_self<TableViewGroupHeader>(terminalHeader)->SetTerminalBottomGridLineSuppression(false);
    }
    m_terminalGridLineRowSizeChangedRevoker.revoke();
    m_terminalGridLineRow = {};
    m_terminalGridLineGroupHeader = {};
    m_suppressTrailingGridLine = false;
    m_suppressBottomGridLine = false;
    m_terminalGridLineGeometryRetryAvailable = true;
    m_terminalGridLineColumnIndex = -1;
    m_terminalGridLineHorizontalOffset = std::numeric_limits<double>::quiet_NaN();
    m_terminalGridLineVerticalOffset = std::numeric_limits<double>::quiet_NaN();
    m_pendingGroupFocusIdentity.clear();
    m_pendingGroupFocusState = winrt::FocusState::Unfocused;

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
    //
    // Order matters. The auto_revoke revoker holds only a weak_ref to ThemeSettings and its revoke()
    // is noexcept: if remove_Changed throws (which it does at app shutdown, where this Unloaded runs
    // from DispatcherQueueController::ShutdownQueue and ThemeSettings' underlying window feature is
    // already detaching, surfacing RPC_E_WRONG_THREAD), the exception escapes the noexcept boundary
    // and terminates the process -- a try/catch here can never intercept it. So we release our strong
    // reference FIRST. If it was the last one the object dies, the revoker's weak_ref goes stale and
    // revoke() becomes a no-op (no ABI call, no throw); if the framework still holds the object it is
    // alive and remove_Changed succeeds normally. Either way remove_Changed is never called against a
    // half-torn-down feature.
    m_themeSettings = nullptr;
    m_themeSettingsChangedRevoker.revoke();
}

void TableView::OnHeaderBringIntoViewRequested(const winrt::BringIntoViewRequestedEventArgs& args)
{
    auto const headerHost = m_headerHost.get();
    auto const target = args.TargetElement();
    auto const bodyScroller = m_bodyScroller.get();
    if (!headerHost || !target || !bodyScroller)
    {
        return;
    }

    const double viewport = bodyScroller.ViewportWidth();
    if (viewport <= 0.0)
    {
        return;
    }

    auto targetRect = args.TargetRect();
    if (targetRect.Width <= 0.0 && targetRect.Height <= 0.0)
    {
        // Focus-driven BringIntoView passes an empty rect; using it as-is makes the right-edge
        // branch below under-scroll by the element's width.
        if (auto const targetFe = target.try_as<winrt::FrameworkElement>())
        {
            targetRect = winrt::Rect{ 0.0f, 0.0f,
                static_cast<float>(targetFe.ActualWidth()), static_cast<float>(targetFe.ActualHeight()) };
        }
    }

    winrt::Rect bounds{};
    try
    {
        bounds = target.TransformToVisual(headerHost).TransformBounds(targetRect);
    }
    catch (...)
    {
        return; // Not connected yet; a stale rect would scroll to the wrong place.
    }

    const double current = bodyScroller.HorizontalOffset();
    double offset = current;
    if (bounds.X < current)
    {
        offset = bounds.X;
    }
    else if (bounds.X + bounds.Width > current + viewport)
    {
        offset = bounds.X + bounds.Width - viewport;
    }

    // Clamped before the comparison: a negative offset would otherwise mark the event handled
    // while the clamped scroll went nowhere.
    offset = std::max(0.0, offset);

    if (std::abs(offset - current) >= 0.5)
    {
        // Handled only when we actually redirect. Marking it unconditionally also silenced
        // ancestor scrollers, so a TableView below the fold never scrolled into view on header
        // focus. The header scroller cannot scroll itself anyway (HorizontalScrollMode=Disabled).
        args.Handled(true);
        m_headerScrollTelemetryOffset = std::min(offset, bodyScroller.ScrollableWidth());
        if (!bodyScroller.ChangeView(offset, nullptr, nullptr, true /* disableAnimation */))
        {
            m_headerScrollTelemetryOffset.reset();
        }
    }
}

void TableView::AppendResizeGripperVisual(
    const winrt::Grid& headerCell,
    const winrt::TableViewColumn& column,
    double gripperWidth,
    const winrt::hstring& headerText,
    winrt::HorizontalAlignment logicalEndAlignment)
{
    auto weakThis = get_weak();
    winrt::ResizeGripper gripperVisual;
    gripperVisual.DragOrientation(winrt::Orientation::Horizontal);
    // Pointer affordance only here: the header cell owns keyboard focus, and one tab stop per
    // column would sit between the user and the data.
    gripperVisual.IsTabStop(false);
    winrt::AutomationProperties::SetAccessibilityView(gripperVisual, winrt::AccessibilityView::Raw);
    // Same explicit logical-end alignment the grid line and the sort affordance use: the header
    // cell's subtree does not observe the ambient FlowDirection auto-flip, so the gripper has to be
    // told which edge is trailing or it lands opposite the grid line under RTL.
    gripperVisual.HorizontalAlignment(logicalEndAlignment);
    gripperVisual.Width(gripperWidth);
    if (!headerText.empty())
    {
        gripperVisual.OwnerName(headerText);
    }

    // Deltas must be measured against the TableView: its frame does not move while a column
    // resizes, and unlike the XamlRoot it is below any scale an app applies above the control.
    gripperVisual.ManipulationContainer(*this);

    // shared_ptr so the handler closures keep it alive, and so Escape can reach the live drag.
    auto state = std::make_shared<ColumnResizeDragState>();
    auto weakColumn = winrt::make_weak(column);

    // The column owns the width: capture it when the drag starts, then apply the reported offset
    // against that anchor and clamp. Nothing is written until the user actually drags.
    gripperVisual.DragStarted(
        [weakColumn, weakThis, state](winrt::IInspectable const& sender, winrt::IInspectable const&)
    {
        auto const strongThis = weakThis.get();

        // Before the guard below: a stale didWrite would revert to the previous drag's start width.
        state->didWrite = false;
        state->didDelta = false;
        state->frozen.clear();

        // One resize at a time. Manipulation arbitrates per element, so a second contact on a
        // DIFFERENT gripper would otherwise run a concurrent drag that Escape could not reach.
        if (strongThis)
        {
            if (auto const active = strongThis->m_activeColumnResizeDrag)
            {
                if (auto const activeGripper = active->gripper.get(); activeGripper && activeGripper.IsDragging())
                {
                    if (auto const self = sender.try_as<winrt::ResizeGripper>())
                    {
                        self.EndDrag(true /* canceled */);
                    }
                    return;
                }
            }
        }

        if (auto const col = weakColumn.get())
        {
            state->startValue = col.ActualWidth();
            state->startWidth = col.ReadLocalValue(winrt::TableViewColumn::WidthProperty());
            if (strongThis)
            {
                state->bounds = strongThis->ResizeBoundsForColumn(col);
            }
        }

        // Published so Escape can find the gesture in flight; the gripper owns everything else
        // about it.
        if (strongThis)
        {
            state->gripper = winrt::make_weak(sender.try_as<winrt::ResizeGripper>());
            strongThis->m_activeColumnResizeDrag = state;
        }
    });

    gripperVisual.DragDelta(
        [weakColumn, weakThis, state](winrt::ResizeGripper const&, winrt::ResizeGripperDragDeltaEventArgs const& vargs)
    {
        auto const col = weakColumn.get();
        if (!col)
        {
            return;
        }

        state->didDelta = true;
        // std::max mirrors TableViewColumn::UpdateActualWidth, so a column whose MaxWidth is below
        // its MinWidth cannot make Width and ActualWidth disagree.
        double lo = (std::isfinite(col.MinWidth()) && col.MinWidth() >= 0.0) ? col.MinWidth() : 0.0;
        double hi = (std::isfinite(col.MaxWidth()) && col.MaxWidth() >= 0.0)
            ? std::max(lo, col.MaxWidth())
            : std::numeric_limits<double>::infinity();

        // The upper bound never falls below the width the drag started from, so a table that
        // already overflows can still shrink.
        lo = std::max(lo, state->bounds.Min);
        hi = std::max(lo, std::min(hi, std::max(state->startValue, state->bounds.Max)));

        const double next = std::clamp(state->startValue + vargs.TotalDelta(), lo, hi);

        // Pinned at a bound the pointer keeps moving but the width does not. Writing anyway would
        // re-run measure, and on a Star column it would also latch the width to pixels.
        auto const current = col.Width();
        const double currentValue =
            current.GridUnitType == winrt::GridUnitType::Pixel ? current.Value : col.ActualWidth();
        if (std::abs(currentValue - next) < 0.0001)
        {
            return;
        }

        auto const columnImpl = winrt::get_self<TableViewColumn>(col);
        if (!state->didWrite)
        {
            if (auto const strongThis = weakThis.get())
            {
                strongThis->FreezeColumnsBeforeResize(col, state->frozen);
            }
        }
        auto const resizeScope = columnImpl->BeginUserResizeScope();
        col.Width(winrt::GridLengthHelper::FromPixels(next));
        state->didWrite = true;
    });

    // Canceled means the gesture was torn down rather than released (Escape, palm rejection, the
    // header rebuilt mid-drag): revert. A clean release announces the new width instead.
    auto weakHeaderCell = winrt::make_weak(headerCell);
    gripperVisual.DragCompleted(
        [weakColumn, weakThis, weakHeaderCell, state](winrt::ResizeGripper const&, winrt::ResizeGripperDragCompletedEventArgs const& cargs)
    {
        auto const strongThis = weakThis.get();

        // Cleared first: reverting the width below can rebuild the headers, and a stale pointer
        // here would let Escape end a gesture that no longer exists.
        if (strongThis && strongThis->m_activeColumnResizeDrag == state)
        {
            strongThis->m_activeColumnResizeDrag.reset();
        }
        state->gripper = nullptr;

        auto const col = weakColumn.get();

        if (cargs.Canceled())
        {
            // Only when a write actually happened, so a press that never moved cannot pin an
            // Auto/Star column.
            // Non-empty only when a freeze actually ran, so this needs no didWrite gate: a write
            // that threw after freezing would otherwise strand the predecessors as pixels.
            for (auto const& entry : state->frozen)
            {
                if (auto const frozenCol = entry.column.get())
                {
                    TableView::RestoreColumnWidth(frozenCol, entry.width);
                }
            }

            if (state->didWrite && col)
            {
                TableView::RestoreColumnWidth(col, state->startWidth);
            }
            return;
        }

        // Attributed to the header cell: it is the focusable element that represents the column,
        // and the one assistive technology is already on during a keyboard resize.
        if (strongThis && col && state->didDelta)
        {
            strongThis->AnnounceColumnWidth(weakHeaderCell.get(), col);
        }
    });
    winrt::Grid::SetColumnSpan(gripperVisual, 2);
    headerCell.Children().Append(gripperVisual);
}

// Escape is the only host-driven cancel left: the gripper ends its own gesture on release, on a
// canceled contact and on unload. Idempotent - EndDrag no-ops when no drag is in flight.
void TableView::CancelColumnResizeDrag()
{
    auto const state = m_activeColumnResizeDrag;
    if (!state)
    {
        return;
    }

    if (auto const gripper = state->gripper.get(); gripper && gripper.IsDragging())
    {
        // The DragCompleted handler clears m_activeColumnResizeDrag.
        try { gripper.EndDrag(true /* canceled */); }
        catch (...) { /* best-effort: EndDrag consumer handlers must not strand state. */ }
    }
    else
    {
        m_activeColumnResizeDrag.reset();
    }
}

// The header cell is tagged with its column; the gripper is one of its children.
// The gripper is one of the header cell's children; the caller already has the cell.
winrt::ResizeGripper TableView::FindResizeGripperInCell(const winrt::FrameworkElement& headerCell) const
{
    auto const cell = headerCell.try_as<winrt::Panel>();
    if (!cell)
    {
        return nullptr;
    }

    auto const cellChildren = cell.Children();
    const uint32_t cellCount = cellChildren.Size();
    for (uint32_t j = 0; j < cellCount; ++j)
    {
        if (auto const gripper = cellChildren.GetAt(j).try_as<winrt::ResizeGripper>())
        {
            return gripper;
        }
    }

    return nullptr;
}

void TableView::OnCanUserResizeColumnsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (args.OldValue() == args.NewValue())
    {
        return;
    }

    RebuildHeaders();
}
