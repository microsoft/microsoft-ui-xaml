// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewColumnHeader.h"
#include "TableViewColumn.h"
#include "TableViewRow.h"
#include "TableViewCellsPanel.h"
#include "TableViewToolTipHelpers.h"
#include "TableViewAutomationPeer.h"
#include "TableViewColumnHeaderAutomationPeer.h"
#include "TableViewSource.h"
#include "TableViewRowTemplateSelector.h"
#include "TableViewGroupHeader.h"
#include "SortIndicator.h"
#include "RuntimeProfiler.h"
#include "TVDiag.h"
#include "TabularControlsResources.h"

#include <string>
#include <cmath>
#include <limits>
#include <algorithm>

// Not in CppWinRTIncludes.h: Microsoft.UI.h's WindowId conflicts with Microsoft.UI.Content.h's.
#include <winrt\Microsoft.UI.Interop.h>

static constexpr std::wstring_view s_RowsRepeaterPartName{ L"PART_RowsRepeater"sv };
static constexpr std::wstring_view s_HeaderRowPartName{ L"PART_HeaderRow"sv };
static constexpr std::wstring_view s_HeaderHostPartName{ L"PART_HeaderHost"sv };
static constexpr std::wstring_view s_EmptyStatePresenterPartName{ L"PART_EmptyStatePresenter"sv };
static constexpr std::wstring_view s_ResizeGripperWidthKey{ L"TableViewResizeGripperWidth"sv };
// Matches TableViewResizeGripperWidth in the theme dictionaries; used when that key is missing or
// unusable.
static constexpr double c_resizeGripperWidthFallback{ 8.0 };

// Fallbacks mirroring the resources shipped in TableView.xaml.
static constexpr double c_defaultRowIndentSize{ 16.0 };
static constexpr double c_defaultRowExpanderSize{ 24.0 };
static constexpr double c_defaultGroupExpanderSize{ 24.0 };
static constexpr double c_defaultGroupExpanderSpacing{ 8.0 };
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

    winrt::ScrollViewer FindScrollViewerAncestor(winrt::DependencyObject const& start, winrt::DependencyObject const& boundary)
    {
        winrt::DependencyObject node = start;
        while (node && !IsSameObject(node, boundary))
        {
            if (auto sv = node.try_as<winrt::ScrollViewer>())
            {
                return sv;
            }
            node = winrt::VisualTreeHelper::GetParent(node);
        }
        return nullptr;
    }

    // Misses are the common case here; HasKey does not originate an error on a miss the way
    // Lookup (and therefore TryLookup) does.
    winrt::IInspectable QuietLookup(winrt::IMap<winrt::IInspectable, winrt::IInspectable> const& map, winrt::IInspectable const& boxedKey)
    {
        if (map && map.HasKey(boxedKey))
        {
            return map.Lookup(boxedKey);
        }
        return nullptr;
    }

    // Reference-typed resources keep their identity across lookups; value-typed ones (x:Double,
    // Thickness, Color, ...) come back as a fresh box every time, so they compare by value.
    bool IsSameResource(winrt::IInspectable const& left, winrt::IInspectable const& right)
    {
        if (!left || !right)
        {
            return false;
        }
        if (IsSameObject(left, right))
        {
            return true;
        }

        const auto leftValue = left.try_as<winrt::IPropertyValue>();
        const auto rightValue = right.try_as<winrt::IPropertyValue>();
        if (!leftValue || !rightValue || leftValue.Type() != rightValue.Type())
        {
            return false;
        }

        switch (leftValue.Type())
        {
        case winrt::PropertyType::Boolean: return leftValue.GetBoolean() == rightValue.GetBoolean();
        case winrt::PropertyType::Double: return leftValue.GetDouble() == rightValue.GetDouble();
        case winrt::PropertyType::Single: return leftValue.GetSingle() == rightValue.GetSingle();
        case winrt::PropertyType::Int32: return leftValue.GetInt32() == rightValue.GetInt32();
        case winrt::PropertyType::UInt32: return leftValue.GetUInt32() == rightValue.GetUInt32();
        case winrt::PropertyType::Int64: return leftValue.GetInt64() == rightValue.GetInt64();
        case winrt::PropertyType::String: return leftValue.GetString() == rightValue.GetString();
        default: break;
        }

        if (auto const l = left.try_as<winrt::IReference<winrt::Thickness>>())
        {
            auto const r = right.try_as<winrt::IReference<winrt::Thickness>>();
            return r && l.Value() == r.Value();
        }
        if (auto const l = left.try_as<winrt::IReference<winrt::Color>>())
        {
            auto const r = right.try_as<winrt::IReference<winrt::Color>>();
            return r && l.Value() == r.Value();
        }
        if (auto const l = left.try_as<winrt::IReference<winrt::CornerRadius>>())
        {
            auto const r = right.try_as<winrt::IReference<winrt::CornerRadius>>();
            return r && l.Value() == r.Value();
        }
        if (auto const l = left.try_as<winrt::IReference<winrt::Windows::UI::Text::FontWeight>>())
        {
            auto const r = right.try_as<winrt::IReference<winrt::Windows::UI::Text::FontWeight>>();
            return r && l.Value().Weight == r.Value().Weight;
        }
        return false;
    }

    // Every key EnsureActiveThemeDictionary can select.
    constexpr std::wstring_view s_highContrastWhiteKey{ L"HighContrastWhite"sv };
    constexpr std::wstring_view s_highContrastBlackKey{ L"HighContrastBlack"sv };
    constexpr std::wstring_view s_highContrastCustomKey{ L"HighContrastCustom"sv };

    // SystemThemingInterop::GetSystemHighContrastTheme: the variant comes from the system window
    // colours (including Xbox's video-safe pair). Empty when High Contrast is off.
    std::wstring_view SystemHighContrastVariantKey()
    {
        HIGHCONTRASTW highContrast{};
        highContrast.cbSize = sizeof(highContrast);
        if (!::SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(highContrast), &highContrast, 0) ||
            !(highContrast.dwFlags & HCF_HIGHCONTRASTON))
        {
            return {};
        }

        const auto foreground = static_cast<unsigned int>(::GetSysColor(COLOR_WINDOWTEXT));
        const auto background = static_cast<unsigned int>(::GetSysColor(COLOR_WINDOW));
        if ((background == 0x00FFFFFF && foreground == 0x0) || (background == 0x00EBEBEB && foreground == 0x00101010))
        {
            return s_highContrastWhiteKey;
        }
        if ((background == 0x0 && foreground == 0x00FFFFFF) || (background == 0x00101010 && foreground == 0x00EBEBEB))
        {
            return s_highContrastBlackKey;
        }
        return s_highContrastCustomKey;
    }

    // EnsureActiveThemeDictionary's selection order: the HC variant (a subtree-requested Light/Dark
    // maps to HighContrastWhite/Black, otherwise the system variant), then "HighContrast", then the
    // base theme, then "Default".
    std::vector<std::wstring_view> ThemeDictionaryKeys(
        bool highContrast,
        std::wstring_view highContrastVariant,
        bool isLight)
    {
        std::vector<std::wstring_view> keys;
        if (highContrast)
        {
            if (!highContrastVariant.empty())
            {
                keys.push_back(highContrastVariant);
            }
            keys.push_back(L"HighContrast"sv);
        }
        keys.push_back(isLight ? L"Light"sv : L"Dark"sv);
        keys.push_back(L"Default"sv);
        return keys;
    }

    struct ElementResourceQuery
    {
        winrt::IInspectable boxedKey;
        // Candidate theme-dictionary keys for the ELEMENT's theme, in selection order.
        std::vector<std::wstring_view> themeKeys;
        // The same for the APP-wide theme, which is what a code-issued Lookup selects with.
        std::vector<std::wstring_view> appThemeKeys;
        // Global theme resources for this key, which Lookup appends to every dictionary's scope.
        // Resolved lazily through an empty dictionary, which has no scope of its own.
        winrt::IInspectable globalValue{ nullptr };
        bool globalResolved{ false };

        winrt::IInspectable const& GlobalValue()
        {
            if (!globalResolved)
            {
                globalResolved = true;
                globalValue = QuietLookup(winrt::ResourceDictionary{}, boxedKey);
            }
            return globalValue;
        }
    };

    winrt::ResourceDictionary SelectThemeDictionary(winrt::ResourceDictionary const& dict, std::vector<std::wstring_view> const& themeKeys)
    {
        if (auto const themeDicts = dict.ThemeDictionaries())
        {
            for (auto const themeKey : themeKeys)
            {
                if (auto const themed = QuietLookup(themeDicts, winrt::box_value(winrt::hstring{ themeKey })).try_as<winrt::ResourceDictionary>())
                {
                    return themed;
                }
            }
        }
        return nullptr;
    }

    // Lookup/HasKey are full-scope (own, merged, APP-theme dictionary, global), and enumerating a
    // dictionary forces CResourceDictionary::EnsureAll (realizes every deferred resource - the whole
    // WinUI theme). So "own entry" is decided by comparing the hit with the one value that full-scope
    // Lookup would have returned had there been no own entry: the nearest merged scope (last-to-first),
    // then the app-theme dictionary, then global. Comparing with ONE candidate rather than any scope
    // keeps an own entry that merely equals an unselected merged or theme value.
    //
    // Approximations, both deliberate:
    // - A merged hit equal to global is skipped as the global fallback. GetKeyNoRefImpl actually lets
    //   the last merged dictionary answer with global first, but Tabular keys are global (the dll's
    //   theme resources), so mirroring that sends nearly every lookup down through the app's merged
    //   dictionaries (XamlControlsResources included) - prohibitively expensive.
    // - An own value-typed entry equal to the global default is indistinguishable from the fallback
    //   (value types come back as fresh boxes) and is treated as not own; only an ancestor override of
    //   the same key can then differ from what the framework returns.
    bool IsOwnEntry(winrt::ResourceDictionary const& dict, winrt::IInspectable const& found, ElementResourceQuery& query)
    {
        // GetKeyNoRefImpl searches the LAST merged dictionary with global scope, so when the key
        // exists globally that first search always answers and the loop stops; only a key with no
        // global value reaches earlier merged dictionaries, and there a full-scope Lookup is exact.
        winrt::IInspectable fallback{ nullptr };
        auto const& global = query.GlobalValue();
        if (auto const merged = dict.MergedDictionaries())
        {
            for (uint32_t i = merged.Size(); i-- > 0 && !fallback;)
            {
                // Every Lookup ends at global; only a hit that differs from it is the merged scope's.
                auto value = QuietLookup(merged.GetAt(i), query.boxedKey);
                if (value && !IsSameResource(value, global))
                {
                    fallback = std::move(value);
                }
            }
        }
        if (!fallback)
        {
            if (auto const themed = SelectThemeDictionary(dict, query.appThemeKeys))
            {
                auto value = QuietLookup(themed, query.boxedKey);
                if (value && !IsSameResource(value, global))
                {
                    fallback = std::move(value);
                }
            }
        }
        if (!fallback)
        {
            fallback = query.GlobalValue();
        }

        return !IsSameResource(found, fallback);
    }

    // Framework (GetKeyNoRefImpl) order: own entries, MergedDictionaries last-to-first, then the
    // active theme dictionary - chosen from the ELEMENT's theme, which a code-issued Lookup cannot see.
    winrt::IInspectable LookupInDictionaryScope(winrt::ResourceDictionary const& dict, ElementResourceQuery& query)
    {
        if (!dict)
        {
            return nullptr;
        }

        if (auto const found = QuietLookup(dict, query.boxedKey))
        {
            if (IsOwnEntry(dict, found, query))
            {
                return found;
            }
        }

        if (auto const merged = dict.MergedDictionaries())
        {
            for (uint32_t i = merged.Size(); i-- > 0;)
            {
                if (auto found = LookupInDictionaryScope(merged.GetAt(i), query))
                {
                    return found;
                }
            }
        }

        if (auto const themed = SelectThemeDictionary(dict, query.themeKeys))
        {
            return LookupInDictionaryScope(themed, query);
        }

        return nullptr;
    }

    // Resolves a key the way {ThemeResource} would from `start`: ancestors nearest first, then
    // Application.Resources, then global theme resources, with theme dictionaries selected as
    // EnsureActiveThemeDictionary does (see ThemeDictionaryKeys).
    winrt::IInspectable LookupElementResource(winrt::FrameworkElement const& start, std::wstring_view key, bool highContrast = false)
    {
        ElementResourceQuery query;
        query.boxedKey = winrt::box_value(winrt::hstring{ key });

        const auto systemHighContrastVariant = SystemHighContrastVariantKey();

        // Theme-scoped resources must resolve against the element's ActualTheme.
        // High Contrast is orthogonal to ActualTheme; callers pass cached HC state for hot-path brush lookups.
        const auto theme = start ? start.ActualTheme() : winrt::ElementTheme::Default;
        std::wstring_view elementHighContrastVariant = systemHighContrastVariant;
        if (highContrast)
        {
            // A subtree that requested a theme selects the matching HC variant, not the system's.
            for (auto requester = start; requester; requester = requester.Parent().try_as<winrt::FrameworkElement>())
            {
                if (requester.RequestedTheme() != winrt::ElementTheme::Default)
                {
                    elementHighContrastVariant = theme == winrt::ElementTheme::Light ? s_highContrastWhiteKey : s_highContrastBlackKey;
                    break;
                }
            }
        }
        query.themeKeys = ThemeDictionaryKeys(highContrast, elementHighContrastVariant, theme == winrt::ElementTheme::Light);

        // What a code-issued Lookup selects: no subtree theme, so the app-wide base and system HC.
        auto appIsLight = theme == winrt::ElementTheme::Light;
        if (auto const app = winrt::Application::Current())
        {
            appIsLight = app.RequestedTheme() == winrt::ApplicationTheme::Light;
        }
        query.appThemeKeys = ThemeDictionaryKeys(!systemHighContrastVariant.empty(), systemHighContrastVariant, appIsLight);

        winrt::FrameworkElement walker = start;
        while (walker)
        {
            if (auto found = LookupInDictionaryScope(walker.Resources(), query))
            {
                return found;
            }
            walker = walker.Parent().try_as<winrt::FrameworkElement>();
        }

        if (auto app = winrt::Application::Current())
        {
            if (auto found = LookupInDictionaryScope(app.Resources(), query))
            {
                return found;
            }
        }

        return query.GlobalValue();
    }

    bool WantsHorizontalLines(winrt::TableViewGridLinesVisibility visibility) noexcept
    {
        return visibility == winrt::TableViewGridLinesVisibility::Horizontal ||
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
        cache.hierarchy.hasRowIndentSize = false;
        cache.hierarchy.hasRowExpanderSize = false;
        cache.hierarchy.hasGroupHeaderContentOffset = false;
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

    // The stock style always supplies TabularSurfaceGridLineBrush; on a miss draw only in High Contrast.
    winrt::Brush CreateGridLineFallbackBrush(winrt::FrameworkElement const& start, bool highContrast)
    {
        if (!highContrast)
        {
            return nullptr;
        }

        return winrt::SolidColorBrush(winrt::unbox_value_or<winrt::Color>(
            LookupElementResource(start, L"SystemColorWindowTextColor", true),
            winrt::Colors::White()));
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
    IgnoreTip(TableViewTip::IgnoreReason::Unloaded);
    // Unloaded can run after the window is destroyed, or not at all; TableView_Instance goes out at most once
    // either way.
    TableViewTelemetry::EndSession(m_telemetrySession);
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

    // Unloaded may not have run; release ThemeSettings and drop the Destroying subscription.
    ReleaseThemeSettings();
}

TableView::TableView()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_TableView);

    SetDefaultStyleKey(this);

    // Construction is only a setup heuristic; equivalent resources or custom templates are valid.
    static thread_local bool s_reportedMissingResources{ false };
    if (!TabularControlsResources::HasBeenCreatedOnCurrentThread() && !s_reportedMissingResources)
    {
        s_reportedMissingResources = TVDiag::LogDebugger(
            L"TableView setup: if using the default template, merge "
            L"<TabularControlsResources xmlns=\"using:Microsoft.UI.Xaml.Controls.Tabular\" /> into "
            L"App.xaml > Application.Resources.MergedDictionaries after XamlControlsResources, before overrides. "
            L"Equivalent resources or a complete custom template are also supported.");
    }

    // Columns must be observable; OnColumnsPropertyChanged owns the VectorChanged subscription to avoid duplicate callbacks.
    auto columns = winrt::single_threaded_observable_vector<winrt::TableViewColumn>();
    Columns(columns);

    auto weakThis = get_weak();

    // IsEditing is read-only to the app; a write that is not the control's own is reverted.
    RegisterPropertyChangedCallback(s_IsEditingProperty,
        [weakThis](winrt::DependencyObject const&, winrt::DependencyProperty const&)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->OnIsEditingPropertyChanged();
            }
        });

    // Use bubbling KeyDown so focused editors can consume typing keys before row navigation.
    m_keyDownHandler = winrt::KeyEventHandler(
        [weakThis](winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args)
        {
            if (auto strongThis = weakThis.get())
            {
                if (std::exchange(strongThis->m_navKeyHandledEarly, false))
                {
                    return;
                }
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

    // handledEventsToo: rows and headers mark their presses handled, and a press anywhere in the
    // table is the user taking focus over from a pending re-shape restore.
    AddHandler(
        winrt::UIElement::PointerPressedEvent(),
        winrt::box_value(winrt::PointerEventHandler(
            [weakThis](winrt::IInspectable const&, winrt::PointerRoutedEventArgs const&)
            {
                if (auto strongThis = weakThis.get())
                {
                    strongThis->CancelReshapeFocusRestoreForUserInput();
                }
            })),
        true /* handledEventsToo */);

    // Headers need no direction stamp (see RebuildHeaders); the rebuild keeps them in step with the body.
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
    TableViewTelemetry::BeginSession(m_telemetrySession);
    TableViewTelemetry::ReportFirstLoad();
    ReclaimItemsSource();
    // The first of the session's two samples; a session that ends without Unloaded carries this one alone.
    SampleInstanceTelemetry();

    // A StartBringItemIntoView made before Loaded; posted so the rows have their first layout.
    if (m_pendingBringIntoViewIndex >= 0)
    {
        ArmPendingBringIntoView();
        PostPendingBringIntoViewAttempt();
    }

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

            // Destroying is raised on the window's thread; a cross-thread host keeps the Unloaded-path release.
            auto const appWindowId = env.AppWindowId();
            auto const hwnd = winrt::Microsoft::UI::GetWindowFromWindowId(appWindowId);
            if (::GetWindowThreadProcessId(hwnd, nullptr) != ::GetCurrentThreadId())
            {
                return;
            }
            if (auto appWindow = winrt::Microsoft::UI::Windowing::AppWindow::GetFromWindowId(appWindowId))
            {
                m_appWindowDestroyingToken = appWindow.Destroying(
                    [weakThis = get_weak()](auto const&, auto const&)
                    {
                        if (auto strongThis = weakThis.get())
                        {
                            strongThis->ReleaseThemeSettings();
                        }
                    });
                m_appWindow = std::move(appWindow);
            }
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
        // Lead-cell padding and indent are imperative; re-derive from the new metrics.
        RefreshRealizedRowHierarchyState();
    }
}

void TableView::OnApplyTemplate()
try
{
    m_headerSortSpaceArmedColumn = nullptr;
    IgnoreTip(TableViewTip::IgnoreReason::Retemplated);
    RevokeTipInitializationLayout();
    TableViewTip::ResetInitialization(m_tip);
    m_templateApplied = true;
    m_templateIncomplete = false;
    // The template is expanded before this runs, so with a source attached Initialize's inputs are ready.
    if (m_activeSource.get()) { StartTipInitialization(); }
    __super::OnApplyTemplate();
    InvalidateTableViewResourceCache(this);
    CancelHeaderFocusRestore();

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
        // The abandoned band still parents its tooltips and UIElement Headers.
        ReleaseHeaderCells(oldHeaderHost);

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
    m_headerContextRequestedRevoker.revoke();
    m_templateRootContextRequestedRevoker.revoke();

    // Below the TableView so TableView.ContextFlyout (and any ancestor's) stays closed while a cell
    // is editing; the framework shows an element's ContextFlyout before its own handlers run.
    m_templateRootContextRequestedRevoker = TableViewDetails::HookTemplateRootContextRequested(*this,
        [weakThis = get_weak()](auto&&, winrt::ContextRequestedEventArgs const& args)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->SuppressContextRequestWhileEditing(args);
            }
        });
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

    m_bodyBringIntoViewRevoker.revoke();
    m_rowsRepeater.set(GetTemplateChild(hstring{ s_RowsRepeaterPartName }).try_as<winrt::ItemsRepeater>());
    m_headerRow.set(GetTemplateChild(hstring{ s_HeaderRowPartName }).try_as<winrt::FrameworkElement>());
    m_headerHost.set(GetTemplateChild(hstring{ s_HeaderHostPartName }).try_as<winrt::Panel>());
    m_emptyStatePresenter.set(GetTemplateChild(hstring{ s_EmptyStatePresenterPartName }).try_as<winrt::ContentControl>());
    // Every part is looked up as optional, so nothing else reports a template without one. TableView's own template
    // has all three, so a missing one is the app's template.
    if (!m_rowsRepeater.get() || !m_headerRow.get() || !m_headerHost.get())
    {
        m_templateIncomplete = true;
        TVDiag::LogRetailF(L"[TableView] The control template lacks PART_RowsRepeater, PART_HeaderRow or PART_HeaderHost.");
        StartTipInitialization();
        TableViewTip::IgnoreInitialization(m_tip, TableViewTip::IgnoreReason::AppTemplateIncomplete);
    }
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

        m_headerContextRequestedRevoker = headerHost.ContextRequested(winrt::auto_revoke,
            [weakThis](winrt::IInspectable const&, winrt::ContextRequestedEventArgs const& args)
            {
                if (auto strongThis = weakThis.get())
                {
                    strongThis->OnHeaderContextRequested(args);
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

        m_bodyBringIntoViewRevoker = repeater.BringIntoViewRequested(winrt::auto_revoke,
            [weakThis](winrt::IInspectable const& /*sender*/, winrt::BringIntoViewRequestedEventArgs const& args)
            {
                if (auto strongThis = weakThis.get())
                {
                    strongThis->OnBodyBringIntoViewRequested(args);
                }
            });

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

    // Track the body cursor as focus moves: re-shape notifications arrive after ItemsRepeater has
    // recycled the focused row, so the pre-reshape position is only observable here.
    m_bodyFocusTrackerGotFocusRevoker = GotFocus(
        winrt::auto_revoke, { this, &TableView::OnTableViewGotFocusForBodyTracking });
    m_bodyFocusTrackerLosingFocusRevoker = LosingFocus(
        winrt::auto_revoke, { this, &TableView::OnTableViewLosingFocusForBodyTracking });

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
                    // Re-apply indent and lead-cell reservation against the new theme's metrics.
                    strongThis->RefreshRealizedRowHierarchyState();
                }
                catch (...)
                {
                    // Theme-switch refresh is best-effort.
                }
            });
    }
    CompleteTipInitializationIfEmpty();
}
catch (...)
{
    // The control, row and cell templates are expanded outside this method; an exception from app code is the
    // app's, and any other throw fails Initialize unless it has its verdict.
    FailTipInitialization(TableViewTip::Stage::Template, TableViewTip::CurrentFault());
    throw;
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
        try { scroller = FindScrollViewerAncestor(headerHost, *this); } catch (...) {}
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
        try { scroller = FindScrollViewerAncestor(repeater, *this); } catch (...) {}
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

bool TableView::HasRowsViewport()
{
    // The scroller is resolved when the rows repeater loads, which a repeater in a collapsed body never does.
    auto scroller = m_bodyScroller.get();
    auto const repeater = m_rowsRepeater.get();
    if (!scroller && repeater) { scroller = FindScrollViewerAncestor(repeater, *this); }
    return scroller ? scroller.ViewportWidth() > 0 && scroller.ViewportHeight() > 0 :
        repeater && repeater.ActualWidth() > 0 && repeater.ActualHeight() > 0;
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

    // A keyboard move's ChangeView has applied; its pending focus may re-measure positions now.
    m_requestedBodyVerticalOffset.reset();

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

    // A pending StartBringItemIntoView index names a row of the data set being replaced.
    CancelPendingBringItemIntoView();

    // Until Initialize has its verdict, attaching a source is part of initialization rather than a tracked swap.
    auto const initializing = !m_tip.initializationDone;
    auto const tipGeneration = BeginTipOperation(TableViewTip::Operation::ReplaceSource, !initializing);
    bool completed = false;
    std::optional<TableViewTip::Fault> failure;
    auto tipCompletion = wil::scope_exit([this, tipGeneration, initializing, &completed, &failure]() noexcept
    {
        if (initializing)
        {
            if (completed) { NoteTipInitializationSource(); }
            else { FailTipInitialization(TableViewTip::Stage::Source, failure.value_or(TableViewTip::Fault{})); }
        }
        else if (!completed)
        {
            FailTipOperation(tipGeneration, TableViewTip::Stage::Source, failure);
        }
        else { CompleteTipOperation(tipGeneration); }
        EndTipOperation();
    });

    try
    {
        // The edited item is about to leave the control. Forced, because the data set is already gone
        // by the time a handler could react, so the close cannot be vetoed. No-focus teardown: this is
        // a dependency-property change callback, and moving focus from one re-enters the framework.
        if (IsEditInProgress())
        {
            TerminateEditWithoutVisualRestore(false /* insideLayoutPass */, true /* sourceReplaced */);
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
    }
    catch (...)
    {
        // App code's exceptions (the new collection, its contract, a callout) carry their kind.
        failure = TableViewTip::CurrentFault();
        throw;
    }
    completed = true;
}

void TableView::OnHeadersVisibilityPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (args.OldValue() == args.NewValue())
    {
        return;
    }

    UpdateHeaderVisibility();
    InvalidateMeasure();
    QueueTerminalGridLineRefresh();

    // UIA: every realized cell's name and header items follow whether column headers are shown.
    const auto columnBit = static_cast<uint32_t>(winrt::TableViewHeadersVisibility::Column);
    const bool showedColumnHeaders =
        (static_cast<uint32_t>(winrt::unbox_value<winrt::TableViewHeadersVisibility>(args.OldValue())) & columnBit) != 0;
    if (showedColumnHeaders != ShouldShowColumnHeaders() &&
        winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::PropertyChanged))
    {
        ForEachRealizedRow([](winrt::TableViewRow const& row)
        {
            winrt::get_self<TableViewRow>(row)->RaiseCellAutomationNamesChangedInternal();
        });
    }
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

// Header lines are always drawn; only the last separator yields to the outer border.
void TableView::ApplyGridLinesToHeader()
{
    auto host = m_headerHost.get();
    if (!host)
    {
        return;
    }

    const auto headerCells = host.Children();
    const uint32_t headerCellCount = headerCells.Size();
    uint32_t lastVisibleHeaderCell = headerCellCount;
    for (uint32_t i = headerCellCount; i > 0; --i)
    {
        if (auto headerCell = headerCells.GetAt(i - 1).try_as<winrt::FrameworkElement>())
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
        if (auto header = headerCells.GetAt(i).try_as<winrt::TableViewColumnHeader>())
        {
            if (auto const gridLine = winrt::get_self<TableViewColumnHeader>(header)->GetGridLineInternal())
            {
                gridLine.Visibility(
                    !(m_suppressTrailingGridLine && i == lastVisibleHeaderCell)
                        ? winrt::Visibility::Visible
                        : winrt::Visibility::Collapsed);
            }
        }
    }
}

void TableView::OnColumnHeaderTemplateApplied(winrt::TableViewColumnHeader const& /*header*/)
{
    if (!m_rebuildingHeaders)
    {
        ApplyGridLinesToHeader();
        RefreshResizeGripperAvailability();
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

            try
            {
                strongThis->RefreshRealizedRowsAfterGroupExpansion();
            }
            catch (...)
            {
                // Best-effort repair after a deferred grouped reshape.
            }
        });
}

void TableView::RefreshRealizedRowsAfterGroupExpansion()
{
    // A reshape can rebind a pooled header in place without ElementPrepared/ElementIndexChanged,
    // leaving a stale IsExpanded (read by the ExpandCollapse peer and chevron states).
    RefreshRealizedGroupHeadersAfterExpansion();

    ForEachRealizedRow([this](winrt::TableViewRow const& row)
    {
        auto* const rowImpl = winrt::get_self<TableViewRow>(row);
        rowImpl->EnsureOwningTableViewInternal(*this);
        RefreshRowSelectionState(row);
    });

    InvalidateMeasure();
}

void TableView::AdoptItemsSource(bool reclaimActiveSource)
{
    auto const itemsSource = ItemsSource();
    // Reclaiming keeps the projection this table already minted, and with it the shaping state.
    winrt::TableViewSource tableViewSource = reclaimActiveSource ? m_activeSource.get() : winrt::TableViewSource{ nullptr };
    if (!tableViewSource)
    {
        tableViewSource = itemsSource.try_as<winrt::TableViewSource>();
        m_telemetrySourceKinds = tableViewSource ? TableViewTelemetry::SourceKind::AppSupplied : 0u;
    }

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
        m_telemetrySourceKinds = TableViewTelemetry::SourceKind::Projected;
    }

    StopAwaitingSource();
    if (tableViewSource && !winrt::get_self<::TableViewSource>(tableViewSource)->SetOwningTableView(*this))
    {
        TVDiag::LogRetailF(L"[TableView] ItemsSource is a TableViewSource already bound to another TableView; ignored.");
        // Retry when the owner lets go; OnTableViewLoaded retries too.
        auto weakThis = get_weak();
        winrt::get_self<::TableViewSource>(tableViewSource)->AddOwnerReleasedHandler(*this, [weakThis]()
        {
            if (auto strongThis = weakThis.get(); strongThis && strongThis->IsLoaded())
            {
                strongThis->ReclaimItemsSource();
            }
        });
        m_awaitedSource = winrt::make_weak(tableViewSource);
        tableViewSource = nullptr;
    }

    // Publish the new source before releasing the previous one: the release synchronously runs
    // other tables' waiters, and an app handler reached from there can assign this table's
    // ItemsSource again. That nested adoption must see coherent state and must win.
    auto const previouslyOwned = m_activeSource.get();
    m_activeSource.set(tableViewSource);
    m_activeSourceReleased = false;

    // A remembered body position names a row of the old source.
    ForgetBodyFocusPosition();
    m_reshapeFocusValid = false;
    m_reshapeFocusRow = -1;
    CancelReshapeFocusRestore();

    if (tableViewSource)
    {
        auto* const sourceImpl = winrt::get_self<::TableViewSource>(tableViewSource);

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
        AttachSourceTip(tableViewSource);
        // Live reshape must not end the user's edit; DrainCoalescedEditReshape resumes it.
        sourceImpl->SetLiveShapingHold([weakThis]()
        {
            auto strongThis = weakThis.get();
            return strongThis && strongThis->IsEditInProgress();
        });
        // CustomSortComparer ranks are frozen at header sort; re-place the edited row.
        sourceImpl->SetLiveItemChangedHook([weakThis](winrt::IInspectable const& item, bool reshapePending)
        {
            auto strongThis = weakThis.get();
            return strongThis && strongThis->RepositionCustomSortItem(item, reshapePending);
        });
        sourceImpl->SetRowsReleasedHook([weakThis](winrt::IInspectable const& item, std::vector<winrt::IInspectable> const& liveRows)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->ReleaseCustomSortRows(item, liveRows);
            }
        });
        sourceImpl->SetLiveItemReshapeHandler([weakThis]()
        {
            if (auto strongThis = weakThis.get())
            {
                TableViewTelemetry::MarkUsed(strongThis->m_telemetrySession, strongThis->m_telemetrySession.generation,
                    TableViewTelemetry::Feature::LiveShaping);
            }
        });

        if (IsLoaded())
        {
            // The source may have been sorted while another table owned it.
            m_rerankCustomSortOnReconcile = reclaimActiveSource;
            QueueReconcileSortStateWithSource();
        }
        else
        {
            // No Unloaded will come to release it; OnTableViewLoaded reclaims it.
            m_activeSourceReleased = true;
            sourceImpl->ReleaseOwningTableView(*this);
        }
    }

    // Detach the previous source last. Swapping ItemsSource between two TableViewSources leaves
    // the old one alive and still subscribed to the app's collection, so without this it keeps a
    // back-pointer to this control and a later rebuild of that discarded source would drive a
    // TableView it no longer belongs to. A nested adoption may have made it active again.
    if (previouslyOwned && previouslyOwned != tableViewSource && previouslyOwned != m_activeSource.get())
    {
        winrt::get_self<::TableViewSource>(previouslyOwned)->ReleaseOwningTableView(*this);
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
        // Subscribe BEFORE the repeater adopts the view: handlers run in subscription order, so we
        // see a Reset while the focused row is still realized.
        UpdateReshapeFocusDetectorSubscription(m_rowsItemsSourceView);

        // ItemsRepeater has no identity short-circuit: re-assigning the same source tears down
        // every container and resets scroll. Guard so a theme-change or Loaded repump does not
        // blow away realized rows.
        if (!IsSameObject(repeater.ItemsSource(), rowsSource))
        {
            repeater.ItemsSource(rowsSource);
        }

        UpdateItemsSourceCollectionChangedSubscription();
        UpdateEmptyState();
        UpdateRowHierarchyResetSubscription();

        // Re-point selection at the new source. SelectionModel::Source clears unconditionally, so a
        // swap always drops the selection; then drain anything requested before a source existed.
        ResolveSelectionAfterSourceChange();
    }
    // else: OnApplyTemplate hasn't run yet; the repeater will be sourced from there.
}

void TableView::OnTableViewSourceProjectionChanged()
{
    SupersedeTipOperation();
    CancelPendingBringItemIntoView();

    // The repeater's focus rescue during recycling must not be mistaken for the user's position.
    SuppressBodyFocusTrackingForThisTurn();

    // Capture first: RefreshRowsPipeline tears down realized rows synchronously, and
    // TerminateEditWithoutVisualRestore can move focus before that.
    CaptureBodyFocusForReshape();

    // A shaping verb swapped the projected shape after we bound, so the cached view and row
    // metadata describe the previous projection. Re-read them and re-drive the rows.
    if (IsEditInProgress())
    {
        // The edited item may not exist in the new projection. Forced, for the same reason as an
        // ItemsSource swap: the shape is already gone by the time a handler could veto it.
        TerminateEditWithoutVisualRestore();
    }

    RefreshRowsPipeline();
    QueueGroupExpansionRowRefresh();

    // In-place projection rewrites (e.g. declaring a hierarchy over the same view) re-prepare no
    // rows, so restamp them. Deferred until the repeater reconciles.
    QueueRefreshRealizedRowHierarchyState();
}

void TableView::UpdateRowHierarchyResetSubscription()
{
    winrt::ItemsSourceView view{ nullptr };
    if (auto const repeater = m_rowsRepeater.get())
    {
        view = repeater.ItemsSourceView();
    }

    if (m_rowHierarchyResetRevoker && SameInspectableIdentity(view, m_rowHierarchyResetView))
    {
        return;
    }

    m_rowHierarchyResetRevoker = {};
    m_rowHierarchyResetView = nullptr;

    if (view)
    {
        m_rowHierarchyResetRevoker = view.CollectionChanged(
            winrt::auto_revoke, { this, &TableView::OnRowsSourceResetForHierarchy });
        m_rowHierarchyResetView = view;
    }
}

void TableView::OnRowsSourceResetForHierarchy(
    const winrt::IInspectable& /*sender*/,
    const winrt::NotifyCollectionChangedEventArgs& args)
{
    // Reset only: splices restamp via ElementIndexChanged or the toggling verb. A Reset may
    // republish metadata for the same rows (e.g. a collapsed node losing its last child).
    if (!args || args.Action() != winrt::NotifyCollectionChangedAction::Reset)
    {
        return;
    }

    QueueRefreshRealizedRowHierarchyState();
}

void TableView::QueueRefreshRealizedRowHierarchyState()
{
    // Deferred and coalesced: raised mid-rebuild, before the repeater reconciles indices.
    if (m_rowHierarchyRefreshQueued)
    {
        return;
    }
    auto weakThis = get_weak();
    auto const queue = winrt::DispatcherQueue::GetForCurrentThread();
    // Unlatch if the post fails (queue shutting down) so later Resets can still queue a restamp.
    m_rowHierarchyRefreshQueued = queue && queue.TryEnqueue([weakThis]()
    {
        if (auto strongThis = weakThis.get())
        {
            strongThis->m_rowHierarchyRefreshQueued = false;
            strongThis->RefreshRealizedRowHierarchyState();
        }
    });
}

void TableView::OnTableViewSourceShapingChanged(bool reorderOnly)
{
    SupersedeTipOperation();
    if (!reorderOnly)
    {
        QueueGroupExpansionRowRefresh();
    }

    // The new shape is in place by now (re-order and rebuild have both run).
    RestoreBodyFocusAfterReshape();

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
    SupersedeTipOperation();
    UpdateEmptyState();
    QueueTerminalGridLineRefresh();
}

void TableView::UpdateReshapeFocusDetectorSubscription(const winrt::ItemsSourceView& view)
{
    if (m_reshapeFocusDetectorRevoker && SameInspectableIdentity(view, m_reshapeFocusDetectorView))
    {
        // Re-subscribing would move us behind the repeater.
        return;
    }

    m_reshapeFocusDetectorRevoker = {};
    m_reshapeFocusDetectorView = nullptr;

    if (view)
    {
        m_reshapeFocusDetectorRevoker = view.CollectionChanged(
            winrt::auto_revoke, { this, &TableView::OnRowsSourceResetForFocus });
        m_reshapeFocusDetectorView = view;
    }
}

void TableView::OnRowsSourceResetForFocus(
    const winrt::IInspectable& /*sender*/,
    const winrt::NotifyCollectionChangedEventArgs& args)
{
    // Only a Reset recycles every realized row; on Add/Remove focus rides along with its element.
    if (args.Action() != winrt::NotifyCollectionChangedAction::Reset)
    {
        return;
    }

    SuppressBodyFocusTrackingForThisTurn();

    CaptureBodyFocusForReshape();

    // Close the edit before the repeater recycles the editor's container for another record.
    if (IsEditInProgress())
    {
        TerminateEditWithoutVisualRestore();
    }
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

// The system focus visual takes its corners from the focused element's CornerRadius, but a rounded
// root also clips its children and hit-testing, so round only while the focus rect shows. Programmatic
// focus is coerced to Keyboard or Pointer by the last input device, so Keyboard is exactly that case.
void TableView::AttachFocusCornerRadius(winrt::FrameworkElement const& target)
{
    auto const radiusProperty = target.try_as<winrt::Control>()
        ? winrt::Control::CornerRadiusProperty()
        : winrt::Grid::CornerRadiusProperty();
    target.RegisterPropertyChangedCallback(winrt::UIElement::FocusStateProperty(),
        [weakThis = get_weak(), radiusProperty](winrt::DependencyObject const& sender, winrt::DependencyProperty const&)
        {
            auto const element = sender.as<winrt::UIElement>();
            auto const self = weakThis.get();
            if (self && element.FocusState() == winrt::FocusState::Keyboard)
            {
                // The TableView style sets CornerRadius from ControlCornerRadius, so the value stays in XAML.
                sender.SetValue(radiusProperty, winrt::box_value(self->CornerRadius()));
            }
            else
            {
                sender.ClearValue(radiusProperty);
            }
        });
}

double TableView::GetRowIndentSize()
{
    // Cached: hit per realized row on every pass, and resource lookup walks the whole tree. Cleared
    // by density/theme/HC invalidation; other runtime swaps need rows re-prepared.
    auto& cache = GetTableViewResourceCache(this);
    if (cache.hierarchy.hasRowIndentSize)
    {
        return cache.hierarchy.rowIndentSize;
    }

    double resolved = c_defaultRowIndentSize;
    if (auto raw = LookupElementResource(*this, L"TableViewRowIndentSize"))
    {
        const double value = winrt::unbox_value_or<double>(raw, c_defaultRowIndentSize);
        // Negative or non-finite indent would pull content under the chevron; fall back.
        if (std::isfinite(value) && value >= 0.0)
        {
            resolved = value;
        }
    }

    cache.hierarchy.rowIndentSize = resolved;
    cache.hierarchy.hasRowIndentSize = true;
    return resolved;
}

double TableView::GetRowExpanderSize()
{
    auto& cache = GetTableViewResourceCache(this);
    if (cache.hierarchy.hasRowExpanderSize)
    {
        return cache.hierarchy.rowExpanderSize;
    }

    double resolved = c_defaultRowExpanderSize;
    if (auto raw = LookupElementResource(*this, L"TableViewRowExpanderSize"))
    {
        const double value = winrt::unbox_value_or<double>(raw, c_defaultRowExpanderSize);
        if (std::isfinite(value) && value >= 0.0)
        {
            resolved = value;
        }
    }

    cache.hierarchy.rowExpanderSize = resolved;
    cache.hierarchy.hasRowExpanderSize = true;
    return resolved;
}

// Grouped root-row indent: the group header's expander gutter plus TableViewGroupExpanderSpacing.
// The header's Padding is not included, so a root chevron sits Padding.Left (8px by default) left of
// the group key text.
double TableView::GetGroupHeaderContentOffset()
{
    auto& cache = GetTableViewResourceCache(this);
    if (!cache.hierarchy.hasGroupHeaderContentOffset)
    {
        auto const lookup = [this](std::wstring_view key, double fallback)
        {
            if (auto raw = LookupElementResource(*this, key))
            {
                const double value = winrt::unbox_value_or<double>(raw, fallback);
                if (std::isfinite(value) && value >= 0.0)
                {
                    return value;
                }
            }
            return fallback;
        };

        cache.hierarchy.groupHeaderContentOffset =
            lookup(L"TableViewGroupExpanderSize", c_defaultGroupExpanderSize) +
            lookup(L"TableViewGroupExpanderSpacing", c_defaultGroupExpanderSpacing);
        cache.hierarchy.hasGroupHeaderContentOffset = true;
    }

    return cache.hierarchy.groupHeaderContentOffset;
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
    if (!column || !IsEditInProgress())
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
        if (IsEditInProgress())
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
    auto const layoutScope = BeginLayoutCallbackScope();
    TableViewTelemetry::NoteContent(m_telemetrySession, TableViewTelemetry::ContentShown::Rows);
    if (auto row = args.Element().try_as<winrt::TableViewRow>())
    {
        auto rowImpl = winrt::get_self<TableViewRow>(row);
        // Stage the suppression state first so the cell rebuild's chrome refresh paints it; refresh
        // grid lines and banding here only if the rebuild did not (each grid-line pass walks every cell).
        rowImpl->SetTerminalGridLineSuppression({
            .suppressTrailing = m_suppressTrailingGridLine,
            .suppressBottom = false }, false /* refresh */);
        if (!rowImpl->EnsureOwningTableViewInternal(*this))
        {
            rowImpl->RefreshGridLines();
            rowImpl->RefreshRowBackground();
        }
        rowImpl->RefreshCellToolTips();
        RefreshRowHierarchyState(row, args.Index());
        RefreshRowSelectionState(row);
        InvalidateMeasureForRowRealizationChange();
        QueueTerminalGridLineRefresh();
    }
    else if (auto header = args.Element().try_as<winrt::TableViewGroupHeader>())
    {
        // A pooled container must not carry a press that began on its previous group.
        winrt::get_self<TableViewGroupHeader>(header)->ResetPointerStateInternal();
        PrepareGroupHeaderElement(header, args.Index());
        InvalidateMeasure();
        QueueTerminalGridLineRefresh();
    }
    if (m_tip.initializationPending && !m_tipInitializationLayoutToken.value) { ArmTipInitializationLayout(); }
}

void TableView::OnRowElementClearing(
    const winrt::ItemsRepeater& /*sender*/,
    const winrt::ItemsRepeaterElementClearingEventArgs& args)
{
    auto const layoutScope = BeginLayoutCallbackScope();
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
                // Beginning window, where app code (a BeginningEdit handler or the editing binding)
                // mutated the collection and recycled the row underneath us. The row
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
        // No longer the drilled row; the cursor level and column survive for re-entry.
        if (auto const drilled = m_cellLevelRow.get(); drilled && IsSameObject(row, drilled))
        {
            m_cellLevelRow = nullptr;
        }
        // Release app-supplied tooltip content rather than pinning it in the recycle pool.
        rowImpl->ReleaseCellToolTips();
        rowImpl->SetTerminalGridLineSuppression({}, false /* refresh */);
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
        if (!rowImpl->EnsureOwningTableViewInternal(*this))
        {
            rowImpl->RefreshGridLines();
        }
        InvalidateMeasureForRowRealizationChange();
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
}

void TableView::OnRowElementIndexChanged(
    const winrt::ItemsRepeater& /*sender*/,
    const winrt::ItemsRepeaterElementIndexChangedEventArgs& args)
{
    auto const layoutScope = BeginLayoutCallbackScope();
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

    // ...and so must hierarchy state: a moved row may be at a new depth.
    RefreshRowHierarchyState(row, args.NewIndex());

    // ...and so must selected chrome. The element keeps its item here (only its index moved), so
    // this normally re-derives the same answer - it is the cheap guarantee that a row whose index
    // shifted under an insert cannot end up disagreeing with the model.
    RefreshRowSelectionState(row);
}

// Pushes this index's hierarchy metadata onto the row (Level 0 for flat/grouped sources).
void TableView::RefreshRowHierarchyState(winrt::TableViewRow const& row, int32_t index)
{
    if (!row)
    {
        return;
    }

    TableViewRowInfo rowInfo{};
    // Use only data-row metadata: a recycled row can be re-prepared at an index that just became
    // a group header (same guard as PrepareGroupHeaderElement).
    const bool hasRowInfo =
        TryGetTableViewSourceRowInfo(index, rowInfo) && rowInfo.Kind == TableViewRowKind::Data;

    auto const rowImpl = winrt::get_self<TableViewRow>(row);
    if (!hasRowInfo)
    {
        rowImpl->SetHierarchyStateInternal(0, false, false);
        return;
    }

    // Ask the source whether it is a tree: a leaf root (Level 1, not expandable) looks like a
    // grouped data row but must still reserve chevron width.
    const bool isHierarchicalRow =
        m_tableViewSourceRowMetadata && m_tableViewSourceRowMetadata->IsHierarchicalSource();
    rowImpl->SetHierarchyStateInternal(
        isHierarchicalRow ? (std::max)(1, rowInfo.Level) : 0,
        rowInfo.IsExpandable,
        rowInfo.IsExpanded);
}

using TableViewDetails::GetColumnHeaderText;

// Includes BasedOn styles.
static bool StyleSetsProperty(const winrt::Style& style, const winrt::DependencyProperty& property)
{
    for (auto current = style; current; current = current.BasedOn())
    {
        for (auto const& setterBase : current.Setters())
        {
            if (auto const setter = setterBase.try_as<winrt::Setter>(); setter && setter.Property() == property)
            {
                return true;
            }
        }
    }
    return false;
}

// GetColumnHeaderText renders an untemplated IStringable for automation purposes, but only a genuine string
// may be swapped for a TextBlock -- a UIElement or a type with an implicit DataTemplate must keep
// the ContentPresenter's content model.
static bool IsPlainStringHeader(const winrt::TableViewColumn& column)
{
    const auto propValue = column.Header().try_as<winrt::IPropertyValue>();
    return propValue && propValue.Type() == winrt::PropertyType::String;
}

// Applying a Style whose TargetType the header is not, at any level of its BasedOn chain, would throw
// out of RebuildHeaders.
winrt::Style TableView::CompatibleHeaderStyle(const winrt::Style& style)
{
    if (!style)
    {
        return nullptr;
    }

    // Depth in TableViewColumnHeader's base chain, or -1 outside it. A BasedOn Style may only target
    // the same type as the Style it bases, or a base type. A Style without a TargetType hangs when
    // applied, so it is outside the chain too.
    auto const targetDepth = [](const winrt::Style& level) -> int
    {
        // TargetType returns a TypeName by value; the view must not outlive its Name.
        const winrt::hstring targetName = level.TargetType().Name;
        const std::wstring_view targetType = targetName;
        if (targetType == winrt::name_of<winrt::TableViewColumnHeader>()) return 5;
        if (targetType == winrt::name_of<winrt::ContentControl>()) return 4;
        if (targetType == winrt::name_of<winrt::Control>()) return 3;
        if (targetType == winrt::name_of<winrt::FrameworkElement>()) return 2;
        if (targetType == winrt::name_of<winrt::UIElement>()) return 1;
        if (targetType == winrt::name_of<winrt::DependencyObject>()) return 0;
        return -1;
    };

    bool compatible = true;
    int derivedDepth = INT_MAX;
    winrt::Style level = style;
    // Bounded: a BasedOn cycle would otherwise never end.
    for (int guard = 0; level && compatible && guard < 64; ++guard, level = level.BasedOn())
    {
        const int depth = targetDepth(level);
        compatible = depth >= 0 && depth <= derivedDepth;
        derivedDepth = depth;
    }
    // A chain still going when the guard runs out loops, and applying it would throw.
    if (level)
    {
        compatible = false;
    }
    if (compatible)
    {
        return style;
    }

    // Once per Style: a rebuild runs on every Density, sort or column change.
    auto const known = std::find_if(m_loggedIncompatibleHeaderStyles.begin(), m_loggedIncompatibleHeaderStyles.end(),
        [&style](auto const& weakStyle) { return weakStyle.get() == style; });
    if (known == m_loggedIncompatibleHeaderStyles.end())
    {
        m_loggedIncompatibleHeaderStyles.erase(std::remove_if(m_loggedIncompatibleHeaderStyles.begin(), m_loggedIncompatibleHeaderStyles.end(),
            [](auto const& weakStyle) { return !weakStyle.get(); }), m_loggedIncompatibleHeaderStyles.end());
        m_loggedIncompatibleHeaderStyles.push_back(winrt::make_weak(style));
        TVDiag::LogRetailF(L"[TableView] Ignored a column header Style whose TargetType or BasedOn chain is not TableViewColumnHeader or a base type.");
    }
    return nullptr;
}

// A header cell keeps its column's UIElement Header and tooltip content parented until released, and
// no other header can host them meanwhile. Every path that drops a header cell must release it first.
void TableView::ReleaseHeaderCells(const winrt::Panel& host, HeaderRelease scope, const winrt::TableViewColumn& column)
{
    if (!host)
    {
        return;
    }

    // Snapshot: Closed and the Content change run app code that can re-enter RebuildHeaders and
    // clear this collection.
    auto const children = host.Children();
    std::vector<winrt::FrameworkElement> cells;
    cells.reserve(children.Size());
    for (uint32_t i = 0; i < children.Size(); ++i)
    {
        if (auto const headerCell = children.GetAt(i).try_as<winrt::FrameworkElement>())
        {
            if (scope != HeaderRelease::All)
            {
                auto const cellColumn = headerCell.Tag().try_as<winrt::TableViewColumn>();
                const bool released = scope == HeaderRelease::Column
                    ? cellColumn && cellColumn == column
                    : cellColumn && winrt::get_self<TableViewColumn>(cellColumn)->GetOwningTableView() != *this;
                if (!released)
                {
                    continue;
                }
            }
            cells.push_back(headerCell);
        }
    }
    if (cells.empty())
    {
        return;
    }

    // Only a rebuild captures and restores header focus itself. Elsewhere the cell stays until the
    // queued rebuild, so park focus on it rather than lose it with the content. Focus leaves a removed
    // column's cell for the nearest remaining header, and stays parked on it only when none can take it.
    if (scope != HeaderRelease::All)
    {
        winrt::FocusState focusState = winrt::FocusState::Unfocused;
        if (auto const focusedColumn = GetFocusedHeaderColumn(host, focusState))
        {
            auto const focusedCell = std::find_if(cells.begin(), cells.end(),
                [&focusedColumn](auto const& cell) { return cell.Tag().try_as<winrt::TableViewColumn>() == focusedColumn; });
            if (focusedCell != cells.end())
            {
                if (scope == HeaderRelease::Column)
                {
                    focusedCell->Focus(focusState);
                }
                else
                {
                    MoveHeaderFocusOffRemovedColumn(host, *focusedCell, focusedColumn, focusState);
                }
            }
        }
    }

    for (auto const& headerCell : cells)
    {
        TableViewDetails::ClearOwnedToolTip(headerCell);
    }
    for (auto const& headerCell : cells)
    {
        if (auto const header = headerCell.try_as<winrt::ContentControl>())
        {
            header.ClearValue(winrt::ContentControl::ContentProperty());
        }
    }
}

void TableView::MoveHeaderFocusOffRemovedColumn(
    const winrt::Panel& host,
    const winrt::FrameworkElement& removedCell,
    const winrt::TableViewColumn& removedColumn,
    winrt::FocusState focusState)
{
    // Snapshot: Focus raises app focus handlers that can re-enter RebuildHeaders and clear Children.
    std::vector<winrt::UIElement> cells;
    {
        auto const children = host.Children();
        const uint32_t size = children.Size();
        cells.reserve(size);
        for (uint32_t i = 0; i < size; ++i)
        {
            cells.push_back(children.GetAt(i));
        }
    }
    auto const removedIt = std::find(cells.begin(), cells.end(), removedCell);
    if (removedIt == cells.end())
    {
        return;
    }
    const size_t removedIndex = static_cast<size_t>(removedIt - cells.begin());

    auto const isHosted = [&host](const winrt::UIElement& cell)
    {
        return winrt::VisualTreeHelper::GetParent(cell) == host;
    };
    auto const tryFocus = [this, &cells, &isHosted, focusState](size_t index) -> winrt::TableViewColumn
    {
        auto const cell = cells[index].try_as<winrt::Control>();
        winrt::TableViewColumn column{ nullptr };
        if (cell)
        {
            column = cell.Tag().try_as<winrt::TableViewColumn>();
        }
        if (column
            && winrt::get_self<TableViewColumn>(column)->GetOwningTableView() == *this
            && cell.Visibility() == winrt::Visibility::Visible
            && isHosted(cell)
            && cell.Focus(focusState))
        {
            return column;
        }
        return nullptr;
    };

    winrt::TableViewColumn fallbackColumn{ nullptr };
    const size_t count = cells.size();
    for (size_t distance = 1; !fallbackColumn && distance < count; ++distance)
    {
        if (removedIndex + distance < count)
        {
            fallbackColumn = tryFocus(removedIndex + distance);
        }
        if (!fallbackColumn && removedIndex >= distance)
        {
            fallbackColumn = tryFocus(removedIndex - distance);
        }
    }

    // With no header to take it, focus parks on the removed cell until the queued rebuild, which
    // returns it to the column if it came back and otherwise moves it to the first header or the body.
    m_headerFocusRemovedColumn = winrt::make_weak(removedColumn);
    m_headerFocusFallbackColumn = nullptr;
    if (fallbackColumn)
    {
        m_headerFocusFallbackColumn = winrt::make_weak(fallbackColumn);
    }
    m_headerFocusRemovedState = focusState;
    if (!fallbackColumn)
    {
        if (auto const parkCell = removedCell.try_as<winrt::Control>(); parkCell && isHosted(parkCell))
        {
            parkCell.Focus(winrt::FocusState::Programmatic);
        }
    }
}

// The body element focus enters on from outside: the current row or cell, else the viewport edge.
bool TableView::FocusBodyAfterHeaderRemoval(winrt::FocusState focusState)
{
    winrt::UIElement target{ nullptr };
    if (auto const edgeRow = FindViewportEdgeRowInternal(false))
    {
        target = ResolveFocusEntryTarget(edgeRow, nullptr, winrt::FocusNavigationDirection::None).Element;
    }
    if (!target)
    {
        target = FindViewportEdgeElementInternal(false, true /* includeGroupHeaders */);
    }
    return target && target.Focus(focusState);
}

void TableView::RebuildHeaders()
{
    // App code run while the old headers are released (focus, tooltip and content changes) can ask
    // for a rebuild. A nested one would build cells the outer one then drops without releasing.
    if (m_inRebuildHeaders)
    {
        m_rebuildHeadersRequested = true;
        return;
    }

    m_rebuildHeadersRequested = false;
    {
        m_inRebuildHeaders = true;
        auto const inRebuildGuard = wil::scope_exit([this]() noexcept { m_inRebuildHeaders = false; });
        RebuildHeadersCore();
    }
    if (std::exchange(m_rebuildHeadersRequested, false))
    {
        QueueRebuildHeaders();
    }
}

void TableView::RebuildHeadersCore()
{
    m_headerSortSpaceArmedColumn = nullptr;
    auto const removedFocusColumn = m_headerFocusRemovedColumn.get();
    auto const fallbackFocusColumn = m_headerFocusFallbackColumn.get();
    auto removedFocusState = m_headerFocusRemovedState;
    m_headerFocusRemovedColumn = nullptr;
    m_headerFocusFallbackColumn = nullptr;
    auto host = m_headerHost.get();
    if (!host)
    {
        return;
    }

    // Clearing the host drops focus with the cell; remember which header held it.
    winrt::FocusState focusedHeaderState = winrt::FocusState::Unfocused;
    auto focusedHeaderColumn = GetFocusedHeaderColumn(host, focusedHeaderState);
    // A removed column that came back (a move) gets back the focus that left its header. Focus
    // still parked on a removed column that did not come back moves to the first header or the body.
    bool focusBody = false;
    if (removedFocusColumn && focusedHeaderColumn
        && (focusedHeaderColumn == fallbackFocusColumn || focusedHeaderColumn == removedFocusColumn))
    {
        if (winrt::get_self<TableViewColumn>(removedFocusColumn)->GetOwningTableView() == *this)
        {
            focusedHeaderColumn = removedFocusColumn;
            focusedHeaderState = removedFocusState;
        }
        else if (focusedHeaderColumn == removedFocusColumn)
        {
            focusedHeaderColumn = nullptr;
            focusBody = true;
        }
    }
    // Replacing the Columns vector skips MoveHeaderFocusOffRemovedColumn.
    if (focusedHeaderColumn && winrt::get_self<TableViewColumn>(focusedHeaderColumn)->GetOwningTableView() != *this)
    {
        removedFocusState = focusedHeaderState;
        focusedHeaderColumn = nullptr;
        focusBody = true;
    }

    // Any focus move during the rebuild other than XAML's rescue off the removed cell belongs to
    // the app or user, and the restore below must not pull focus back from it.
    bool headerFocusMovedAway = false;

    // Focus inside a header's content would leave with it, and an app moving focus as that content
    // is released could not be told from XAML's rescue. Park it on the header first; an app that
    // sends that move out of the header band has moved focus itself.
    if (focusedHeaderColumn)
    {
        if (auto const focused = winrt::FocusManager::GetFocusedElement(XamlRoot()).try_as<winrt::DependencyObject>())
        {
            winrt::DependencyObject headerCell = focused;
            for (auto parent = winrt::VisualTreeHelper::GetParent(headerCell); parent && parent != host;
                parent = winrt::VisualTreeHelper::GetParent(parent))
            {
                headerCell = parent;
            }
            if (headerCell != focused && winrt::VisualTreeHelper::GetParent(headerCell) == host)
            {
                if (auto const control = headerCell.try_as<winrt::Control>())
                {
                    control.Focus(winrt::FocusState::Programmatic);
                    winrt::FocusState ignored{};
                    headerFocusMovedAway = !GetFocusedHeaderColumn(host, ignored);
                }
            }
        }
    }

    // XAML rescues focus only while the focused element is being removed.
    bool removingFocused = false;
    winrt::DependencyObject focusedAtTeardown{ nullptr };
    winrt::FocusManager::GettingFocus_revoker headerFocusMoveRevoker{};
    if (focusedHeaderColumn || focusBody)
    {
        auto const root = XamlRoot();
        focusedAtTeardown = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>();
        headerFocusMoveRevoker = winrt::FocusManager::GettingFocus(winrt::auto_revoke,
            [&headerFocusMovedAway, &removingFocused, root, focusedAtTeardown, weakHost = winrt::make_weak(host), rescueSeen = false](
                winrt::IInspectable const&, winrt::Microsoft::UI::Xaml::Input::GettingFocusEventArgs const& args) mutable
            {
                auto const newFocus = args.NewFocusedElement().try_as<winrt::UIElement>();
                if (headerFocusMovedAway || !newFocus || newFocus.XamlRoot() != root)
                {
                    return;
                }
                auto const liveHost = weakHost.get();
                for (winrt::DependencyObject current = newFocus; current; current = winrt::VisualTreeHelper::GetParent(current))
                {
                    if (liveHost && current == liveHost)
                    {
                        return;
                    }
                }
                if (removingFocused && !rescueSeen && focusedAtTeardown && args.OldFocusedElement() == focusedAtTeardown)
                {
                    rescueSeen = true;
                    return;
                }
                headerFocusMovedAway = true;
            });
    }

    // Clearing the host under a live popup tears its child down reentrantly. Releasing the cells
    // removes only focus inside a header's content; clearing the host removes the headers too.
    removingFocused = focusedAtTeardown && winrt::VisualTreeHelper::GetParent(focusedAtTeardown) != host;
    ReleaseHeaderCells(host);
    removingFocused = true;
    host.Children().Clear();
    removingFocused = false;

    m_rebuildingHeaders = true;
    auto const rebuildingGuard = wil::scope_exit([this]() noexcept { m_rebuildingHeaders = false; });

    const winrt::Thickness cachedHeaderCellPadding = GetDensityHeaderCellPadding();
    const double cachedRowMinHeight = GetDensityRowMinHeight();
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
    auto const tableHeaderStyle = CompatibleHeaderStyle(ColumnHeaderStyle());

    if (auto columns = Columns())
    {
        for (auto const& column : columns)
        {
            if (!column || winrt::get_self<TableViewColumn>(column)->GetOwningTableView() != *this)
            {
                continue;
            }

            auto const header = winrt::make<TableViewColumnHeader>();
            auto const headerImpl = winrt::get_self<TableViewColumnHeader>(header);
            headerImpl->SetOwnerInternal(*this, column);
            header.Visibility(column.Visibility());

            auto const columnHeaderStyle = CompatibleHeaderStyle(column.HeaderStyle());
            auto const headerStyle = columnHeaderStyle ? columnHeaderStyle : tableHeaderStyle;
            // Read the Setters before applying the Style seals it: reading a sealed BasedOn Style's
            // never-accessed Setters reseals it and re-merges its BasedOn setters.
            const bool styleSetsMinHeight = StyleSetsProperty(headerStyle, winrt::FrameworkElement::MinHeightProperty());
            const bool styleSetsPadding = StyleSetsProperty(headerStyle, winrt::Control::PaddingProperty());
            if (headerStyle)
            {
                header.Style(headerStyle);
            }

            const bool headerIsResizable = CanUserResizeColumns() && column.CanUserResize();
            const bool headerIsSortable = canUserSortColumns && column.CanUserSort();
            AttachFocusCornerRadius(header);
            const winrt::hstring headerText = GetColumnHeaderText(column);
            if (!headerText.empty())
            {
                winrt::AutomationProperties::SetName(header, headerText);
            }
            winrt::AutomationProperties::SetAccessibilityView(header, winrt::AccessibilityView::Content);
            if (!styleSetsMinHeight)
            {
                header.MinHeight(cachedRowMinHeight);
            }
            if (!styleSetsPadding)
            {
                header.Padding(cachedHeaderCellPadding);
            }

            if (auto headerTemplateSelector = column.HeaderTemplateSelector())
            {
                header.Content(column.Header());
                header.ContentTemplateSelector(headerTemplateSelector);
            }
            else if (auto headerTemplate = column.HeaderTemplate())
            {
                header.Content(column.Header());
                header.ContentTemplate(headerTemplate);
            }
            else if (!headerText.empty() && IsPlainStringHeader(column))
            {
                // A ContentPresenter renders a bare string through an implicit TextBlock carrying no
                // TextTrimming, so a too-wide header hard-clips mid-glyph while its cells ellipsize.
                winrt::TextBlock headerBlock;
                headerBlock.Text(headerText);
                headerBlock.TextTrimming(winrt::TextTrimming::CharacterEllipsis);
                headerBlock.VerticalAlignment(winrt::VerticalAlignment::Center);
                // The header's peer already announces this text.
                winrt::AutomationProperties::SetAccessibilityView(headerBlock, winrt::AccessibilityView::Raw);
                header.Content(headerBlock);
                headerImpl->SetOwnsContentInternal(true);
            }
            else
            {
                header.Content(column.Header());
            }

            header.Tag(column);

            TableViewDetails::ApplyHeaderToolTip(header, column.HeaderToolTip());

            headerImpl->SetSortableInternal(headerIsSortable, cachedSortIndicatorWidth);
            headerImpl->SetSortDirectionInternal(column.SortDirection());

            if (headerIsSortable)
            {
                // Weak: the handler is owned by a visual the control also owns, so a strong
                // capture would keep the TableView alive through its own header.
                auto weakThis = get_weak();
                header.Tapped([weakThis, column](auto const&, winrt::TappedRoutedEventArgs const& args)
                {
                    if (auto strongThis = weakThis.get())
                    {
                        if (strongThis->ToggleSortDirectionFromUserInput(column))
                        {
                            args.Handled(true);
                        }
                    }
                });
            }

            // The gripper marks the press handled, which also keeps a resize drag from reaching the
            // header's Tapped handler and sorting the column.
            if (headerIsResizable)
            {
                headerImpl->SetResizeGripperInternal(
                    CreateResizeGripper(header, column, cachedResizeGripperWidth, headerText));
            }

            host.Children().Append(header);
            // Template now so the parts exist for the passes below and for callers right after.
            header.ApplyTemplate();
        }
    }

    // Pin (or refresh) leading-frozen header cells at the current scroll offset.
    RefreshFrozenColumns();
    ApplyGridLinesToHeader();
    RefreshResizeGripperAvailability();

    headerFocusMoveRevoker.revoke();
    if (focusedHeaderColumn && !headerFocusMovedAway)
    {
        RestoreHeaderFocus(focusedHeaderColumn, focusedHeaderState);
    }
    else if (focusBody && !headerFocusMovedAway)
    {
        // Focus parks only when no other header could take it, so headers built now replaced the
        // removed columns; keep focus in the header band before falling back to the body.
        winrt::TableViewColumn firstHeaderColumn{ nullptr };
        if (auto const columns = Columns())
        {
            for (auto const& column : columns)
            {
                if (column && column.Visibility() == winrt::Visibility::Visible &&
                    winrt::get_self<TableViewColumn>(column)->GetOwningTableView() == *this)
                {
                    firstHeaderColumn = column;
                    break;
                }
            }
        }
        if (firstHeaderColumn)
        {
            RestoreHeaderFocus(firstHeaderColumn, removedFocusState);
        }
        else
        {
            FocusBodyAfterHeaderRemoval(removedFocusState);
        }
    }
}

winrt::TableViewColumn TableView::GetFocusedHeaderColumn(const winrt::Panel& host, winrt::FocusState& focusState) const
{
    focusState = winrt::FocusState::Unfocused;
    auto const root = XamlRoot();
    if (!host || !root)
    {
        return nullptr;
    }

    auto const focusedElement = winrt::FocusManager::GetFocusedElement(root);
    auto current = focusedElement.try_as<winrt::DependencyObject>();
    while (current)
    {
        auto const parent = winrt::VisualTreeHelper::GetParent(current);
        if (parent == host)
        {
            winrt::TableViewColumn column{ nullptr };
            if (auto const cell = current.try_as<winrt::FrameworkElement>())
            {
                column = cell.Tag().try_as<winrt::TableViewColumn>();
            }
            if (column)
            {
                focusState = winrt::FocusState::Keyboard;
                if (auto const ui = focusedElement.try_as<winrt::UIElement>())
                {
                    focusState = ui.FocusState() == winrt::FocusState::Keyboard ? winrt::FocusState::Keyboard : winrt::FocusState::Programmatic;
                }
            }
            return column;
        }
        current = parent;
    }

    return nullptr;
}

void TableView::RestoreHeaderFocus(const winrt::TableViewColumn& column, winrt::FocusState focusState)
{
    CancelHeaderFocusRestore();
    if (TryFocusHeaderCell(column, focusState))
    {
        return;
    }

    // Whatever holds focus now is what the rebuild left behind; anything else later is a user/app move.
    winrt::weak_ref<winrt::DependencyObject> weakFocusAtSchedule{ nullptr };
    if (auto const root = XamlRoot())
    {
        if (auto const focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>())
        {
            weakFocusAtSchedule = winrt::make_weak(focused);
        }
    }

    auto weakThis = get_weak();
    m_headerFocusRestoreLayoutToken = LayoutUpdated(
        [weakThis, weakColumn = winrt::make_weak(column), focusState, weakFocusAtSchedule](winrt::IInspectable const&, winrt::IInspectable const&)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->CancelHeaderFocusRestore();
                if (!strongThis->CanRestoreHeaderFocus(weakFocusAtSchedule.get()))
                {
                    return;
                }
                if (auto const liveColumn = weakColumn.get())
                {
                    strongThis->TryFocusHeaderCell(liveColumn, focusState);
                }
            }
        });
}

// Focus the user or app moved elsewhere while the restore was pending is never pulled back.
bool TableView::CanRestoreHeaderFocus(const winrt::DependencyObject& focusAtSchedule) const
{
    auto const root = XamlRoot();
    if (!root)
    {
        return false;
    }

    auto const focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>();
    if (!focused || focused == focusAtSchedule)
    {
        return true;
    }

    auto const headerHost = m_headerHost.get();
    for (auto current = focused; current; current = winrt::VisualTreeHelper::GetParent(current))
    {
        if (headerHost && current == headerHost)
        {
            return true;
        }
    }
    return false;
}

bool TableView::TryFocusHeaderCell(const winrt::TableViewColumn& column, winrt::FocusState focusState)
{
    auto const cell = TableViewCellsPanel::CellForColumn(m_headerHost.get(), column);
    return cell && cell.Focus(focusState);
}

void TableView::CancelHeaderFocusRestore()
{
    if (m_headerFocusRestoreLayoutToken.value)
    {
        LayoutUpdated(m_headerFocusRestoreLayoutToken);
        m_headerFocusRestoreLayoutToken = {};
    }
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

void TableView::RefreshSortIndicators()
{
    auto host = m_headerHost.get();
    if (!host)
    {
        return;
    }

    for (auto const& child : host.Children())
    {
        auto const header = child.try_as<winrt::TableViewColumnHeader>();
        if (!header)
        {
            continue;
        }

        if (auto const column = header.Tag().try_as<winrt::TableViewColumn>())
        {
            winrt::get_self<TableViewColumnHeader>(header)->SetSortDirectionInternal(column.SortDirection());
        }
    }
}

void TableView::RaiseColumnSortStateAutomationChanged(const winrt::TableViewColumn& column, winrt::SortDirection oldDirection)
{
    if (!column || !winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::PropertyChanged))
    {
        return;
    }
    auto const header = TableViewCellsPanel::CellForColumn(m_headerHost.get(), column);
    if (!header)
    {
        return;
    }
    if (auto const peer = winrt::FrameworkElementAutomationPeer::FromElement(header))
    {
        // The header peer reports the sort state through its help text.
        winrt::hstring oldText;
        if (auto const headerPeer = peer.try_as<winrt::TableViewColumnHeaderAutomationPeer>())
        {
            oldText = winrt::get_self<TableViewColumnHeaderAutomationPeer>(headerPeer)->HelpTextForSortDirection(oldDirection);
        }
        auto const newText = peer.GetHelpText();
        if (oldText != newText)
        {
            peer.RaisePropertyChangedEvent(
                winrt::AutomationElementIdentifiers::HelpTextProperty(), winrt::box_value(oldText), winrt::box_value(newText));
        }
    }
}

void TableView::OnColumnHeaderStylePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    // Rebuild, not restyle in place: the style also decides whether density sets MinHeight/Padding.
    QueueRebuildHeaders();
}

void TableView::OnCanUserSortColumnsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    // The chevron and the click handler are stamped at header-build time. Like WPF's
    // DataGrid.CanUserSortColumns this gates user input only, so an applied sort is left in place.
    QueueRebuildHeaders();
}

void TableView::OnColumnCanUserSortChanged(const winrt::TableViewColumn& /*column*/)
{
    // The chevron and the click handler are stamped at header-build time. CanUserSort gates user
    // input only, so an applied sort is left in place.
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
                    TableViewTelemetry::ReportError(strongThis->m_telemetrySession, 0,
                        TableViewTip::Operation::HeaderRefresh, TableViewTip::Stage::HeaderRefresh, true);
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

void TableView::RaiseColumnsStructureChanged()
{
    // FromElement only: don't create a peer just to announce when no client is connected.
    if (auto const peer = winrt::FrameworkElementAutomationPeer::FromElement(*this).try_as<winrt::TableViewAutomationPeer>())
    {
        winrt::get_self<TableViewAutomationPeer>(peer)->RaiseStructureChangedForColumnsChange();
    }
}

void TableView::QueueRaiseColumnsStructureChanged()
{
    if (!winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::StructureChanged))
    {
        return;
    }

    if (m_columnsStructureChangedQueued)
    {
        return;
    }

    auto dispatcher = DispatcherQueue();
    if (!dispatcher)
    {
        // No dispatcher (test host / teardown): nothing to coalesce against, so raise inline.
        RaiseColumnsStructureChanged();
        return;
    }

    // Deferred so a burst of Columns VectorChanged callbacks announces once, with the final shape.
    m_columnsStructureChangedQueued = true;
    auto weakThis = get_weak();
    if (!dispatcher.TryEnqueue([weakThis]()
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->m_columnsStructureChangedQueued = false;
                try
                {
                    strongThis->RaiseColumnsStructureChanged();
                }
                catch (...)
                {
                    // Best-effort announcement; never fail-fast the dispatcher.
                }
            }
        }))
    {
        m_columnsStructureChangedQueued = false;
        RaiseColumnsStructureChanged();
    }
}

void TableView::StopAwaitingSource()
{
    if (auto const awaitedSource = m_awaitedSource.get())
    {
        winrt::get_self<::TableViewSource>(awaitedSource)->RemoveOwnerReleasedHandler(*this);
    }
    m_awaitedSource = nullptr;
}

void TableView::ReclaimItemsSource()
{
    if (m_activeSource.get() ? !m_activeSourceReleased : !ItemsSource().try_as<winrt::TableViewSource>())
    {
        return;
    }

    AdoptItemsSource(true /* reclaimActiveSource */);
    // While drained, OnRowsRepeaterLoaded re-sources the repeater.
    if (!m_rowsSourceDrained)
    {
        RefreshRowsPipeline();
    }
    // A source refused while another table owned it arrives here.
    NoteTipInitializationSource();
}

void TableView::OnTableViewUnloaded()
{
    // A re-parent can raise Loaded before the stale Unloaded; tearing down now would leave a live
    // table detached from its source, drained repeater, scrollers and ThemeSettings.
    if (IsLoaded())
    {
        return;
    }

    m_headerSortSpaceArmedColumn = nullptr;
    IgnoreTipInitializationUnloadedBeforeRender();
    IgnoreTip(TableViewTip::IgnoreReason::Unloaded);
    SampleInstanceTelemetry();
    TableViewTelemetry::EndSession(m_telemetrySession);
    // Neither a pending restore nor a remembered position may outlive this tree.
    CancelReshapeFocusRestore();
    ForgetBodyFocusPosition();
    m_reshapeFocusValid = false;
    m_reshapeFocusRow = -1;
    CancelHeaderFocusRestore();
    RevokeTipInitializationLayout();
    CancelPendingBringItemIntoView();

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
    m_pendingGroupFocusFromPointerGesture = false;

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

    // An unloaded table may stay alive until GC; let a newly loaded one bind the source meanwhile.
    // OnTableViewLoaded reclaims it.
    // Flag first: the release runs other tables' waiters synchronously.
    if (auto const activeSource = m_activeSource.get(); activeSource && !m_activeSourceReleased)
    {
        m_activeSourceReleased = true;
        winrt::get_self<::TableViewSource>(activeSource)->ReleaseOwningTableView(*this);
    }
    StopAwaitingSource();

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
    ReleaseThemeSettings();
}

void TableView::ReleaseThemeSettings()
{
    if (auto appWindow = std::exchange(m_appWindow, nullptr))
    {
        try
        {
            appWindow.Destroying(std::exchange(m_appWindowDestroyingToken, {}));
        }
        catch (...)
        {
        }
    }

    // Release can surface a teardown failure; keep it off the noexcept path.
    // Release before revoke() so a dead object's stale weak_ref makes it a no-op.
    if (auto const themeSettings = static_cast<::IUnknown*>(winrt::detach_abi(m_themeSettings)))
    {
        try
        {
            themeSettings->Release();
        }
        catch (...)
        {
        }
    }
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
        bodyScroller.ChangeView(offset, nullptr, nullptr, true /* disableAnimation */);
    }
}

bool TableView::ApplyColumnResizeDrag(TableView* owner, const winrt::TableViewColumn& col, ColumnResizeDragState& state, double totalDelta)
{
    double lo = NonNegativeFiniteOrZero(col.MinWidth());
    double hi = MaxWidthForColumn(col, lo);

    // The upper bound never falls below the width the drag started from, so a table that
    // already overflows can still shrink.
    lo = std::max(lo, state.bounds.Min);
    hi = std::max(lo, std::min(hi, std::max(state.startValue, state.bounds.Max)));

    const double next = std::clamp(state.startValue + totalDelta, lo, hi);

    // Pinned at a bound the pointer keeps moving but the width does not. Writing anyway would
    // re-run measure, and on a Star column it would also latch the width to pixels.
    auto const current = col.Width();
    const double currentValue =
        current.GridUnitType == winrt::GridUnitType::Pixel ? current.Value : col.ActualWidth();
    if (std::abs(currentValue - next) < 0.0001)
    {
        return false;
    }

    auto const columnImpl = winrt::get_self<TableViewColumn>(col);
    if (!state.didWrite && owner)
    {
        owner->FreezeColumnsBeforeResize(col, state.frozen);
    }
    auto const resizeScope = columnImpl->BeginUserResizeScope();
    col.Width(winrt::GridLengthHelper::FromPixels(next));
    state.didWrite = true;
    return true;
}

winrt::ResizeGripper TableView::CreateResizeGripper(
    const winrt::FrameworkElement& headerCell,
    const winrt::TableViewColumn& column,
    double gripperWidth,
    const winrt::hstring& headerText)
{
    auto weakThis = get_weak();
    winrt::ResizeGripper gripperVisual;
    gripperVisual.DragOrientation(winrt::Orientation::Horizontal);
    // Pointer affordance only here: the header cell owns keyboard focus, and one tab stop per
    // column would sit between the user and the data.
    gripperVisual.IsTabStop(false);
    winrt::AutomationProperties::SetAccessibilityView(gripperVisual, winrt::AccessibilityView::Raw);
    // Trailing edge in both flow directions; deliberately NOT swapped for RTL (see RebuildHeaders).
    gripperVisual.HorizontalAlignment(winrt::HorizontalAlignment::Right);
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
        ApplyColumnResizeDrag(weakThis.get().get(), col, *state, vargs.TotalDelta());
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

            // Mid-drag refreshes were suppressed, so a sibling width change during the gesture
            // still needs its availability applied.
            if (strongThis)
            {
                strongThis->RefreshResizeGripperAvailability();
            }
            return;
        }

        // Attributed to the header cell: it is the focusable element that represents the column,
        // and the one assistive technology is already on during a keyboard resize.
        if (strongThis && col && state->didDelta)
        {
            strongThis->AnnounceColumnWidth(weakHeaderCell.get(), col);
        }

        if (strongThis)
        {
            strongThis->RefreshResizeGripperAvailability();
        }
    });
    return gripperVisual;
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

// The same gates that decide whether the gripper exists and is shown, so UIA never offers a
// resize the pointer cannot perform.
bool TableView::IsColumnResizeAllowed(const winrt::TableViewColumn& column)
{
    return column &&
        CanUserResizeColumns() &&
        column.CanUserResize() &&
        column.Visibility() == winrt::Visibility::Visible &&
        winrt::get_self<TableViewColumn>(column)->GetOwningTableView() == *this;
}

winrt::ResizeGripper TableView::GetAvailableResizeGripper(const winrt::TableViewColumn& column)
{
    if (!IsColumnResizeAllowed(column))
    {
        return nullptr;
    }

    auto const host = m_headerHost.get();
    if (!host)
    {
        return nullptr;
    }

    auto const headerCell = TableViewCellsPanel::CellForColumn(host, column);
    if (!headerCell || IsColumnResizePinned(column))
    {
        return nullptr;
    }

    return FindResizeGripperInCell(headerCell);
}

bool TableView::CanResizeColumnFromAutomation(const winrt::TableViewColumn& column)
{
    return GetAvailableResizeGripper(column) != nullptr;
}

bool TableView::TryResizeColumnFromAutomation(const winrt::TableViewColumn& column, double width)
{
    auto const gripper = GetAvailableResizeGripper(column);
    if (!gripper || gripper.IsDragging())
    {
        return false;
    }

    // DragStarted anchors the gesture at ActualWidth, so the offset is taken from the same value.
    const double totalDelta = width - column.ActualWidth();

    // Rounding noise from the pixel conversion; writing it would pin an Auto or Star column.
    if (std::abs(totalDelta) < 1e-3)
    {
        return true;
    }

    gripper.BeginDrag();
    // DragStarted ends the gesture at once while another column's drag is in flight.
    if (!gripper.IsDragging())
    {
        return false;
    }

    bool applied = true;
    try
    {
        // TryDrag drops a step under its jitter deadband, but an absolute UIA width is no jitter:
        // apply it as DragDelta would, announcing only if it wrote.
        if (!gripper.TryDrag(totalDelta))
        {
            auto const state = m_activeColumnResizeDrag;
            applied = state && state->gripper.get() == gripper;
            if (applied)
            {
                state->didDelta = ApplyColumnResizeDrag(this, column, *state, totalDelta);
            }
        }
    }
    catch (...)
    {
        gripper.EndDrag(true /* canceled */);
        throw;
    }

    gripper.EndDrag(!applied /* canceled */);
    return applied;
}

winrt::ResizeGripper TableView::FindResizeGripperInCell(const winrt::FrameworkElement& headerCell) const
{
    if (auto const header = headerCell.try_as<winrt::TableViewColumnHeader>())
    {
        return winrt::get_self<TableViewColumnHeader>(header)->GetResizeGripperInternal();
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
