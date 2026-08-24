using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;
using XamlTelemtryViewerWInui3.Models;
using XamlTelemtryViewerWInui3.Models.Timeline;
using XamlTelemtryViewerWInui3.Services;
using static XamlTelemtryViewerWInui3.Helpers.TimelineMath;
using static XamlTelemtryViewerWInui3.Helpers.TimelineColorHelper;

namespace XamlTelemtryViewerWInui3;

// Multi-trace Track 2: grouped PROVIDER -> TRACE. One collapsible section per provider;
// inside it one sub-lane per trace (time axis + bars), each scaled to that trace''s own
// region window with its own Ctrl+scroll zoom / drag pan. Only the first provider is
// expanded; others draw lazily so the UI thread never builds many heavy views at once.
public sealed partial class TimelineWindow : Window
{
    private const double T2AxisHeight = 20;
    private const double T2LaneHeight = 45;

    private sealed class TraceSection
    {
        public string TraceLabel = string.Empty;
        public DateTime Start;
        public DateTime End;
        public double StartOffsetMs;  // region start relative to this trace's launch (t=0)
        public List<SpecialChildrenByProvider> Groups = new();
        public Dictionary<string, string> Colors = new();
    }

    // Zoom/pan view [0..1] per (provider, trace) sub-lane.
    private readonly Dictionary<string, (double start, double end)> _t2View = new();

    // Active region name for multi-trace Track 2 (null = nothing selected yet).
    private string? _multiTrack2Region;

    private async void RenderMultiTraceTrack2(string regionName)
    {
        var lanes = ActiveLanes();
        Track2ProviderStackPanel.Children.Clear();
        _t2View.Clear();
        _viewModel.IsTrack2Visible = true;
        _multiTrack2Region = regionName;

        var loading = new TextBlock
        {
            Text = "Computing providers across traces...",
            Foreground = new SolidColorBrush(Windows.UI.Color.FromArgb(255, 0xAA, 0xAA, 0xAA)),
        };
        Track2ProviderStackPanel.Children.Add(loading);

        var tasks = new List<Task<TraceSection>>();
        foreach (var lane in lanes)
        {
            var item = lane.Model?.TrackOneItems.FirstOrDefault(i => i.Name == regionName);
            if (item is null) continue;
            var end = item.End ?? item.Start.AddMilliseconds(1);
            var dur = end - item.Start;
            var def = GetPhaseDefinitionForItem(item);
            var pid = lane.SelectedProcess?.Id;
            var events = lane.Trace.Events.ToList();
            var label = lane.TraceLabel;
            var traceStart = lane.Model!.TrackOneItems.Min(i => i.Start);
            var offsetMs = (item.Start - traceStart).TotalMilliseconds;
            tasks.Add(Task.Run(() =>
            {
                var g = new SpecialChildrenBuilder().BuildGroupedByProvider(events, item.Start, end, dur, def, pid);
                return new TraceSection { TraceLabel = label, Start = item.Start, End = end, StartOffsetMs = offsetMs, Groups = g, Colors = AssignColorsPerProvider(g) };
            }));
        }

        var sections = await Task.WhenAll(tasks);
        Track2ProviderStackPanel.Children.Remove(loading);

        // Prioritized = SpecialProviders OR cumulative duration over threshold% in any trace.
        var phaseDef = lanes.Count > 0
            ? GetPhaseDefinitionForItem(lanes[0].Model!.TrackOneItems.First(i => i.Name == regionName))
            : null;
        var specialSet = new HashSet<string>(phaseDef?.SpecialProviders ?? new System.Collections.Generic.List<string>(), StringComparer.OrdinalIgnoreCase);
        var pct = phaseDef?.ThresholdPercentage ?? 5.0;
        bool IsPrioritized(string p) => specialSet.Contains(p) || sections.Any(s =>
        {
            var g = s.Groups.FirstOrDefault(x => x.ProviderName.Equals(p, StringComparison.OrdinalIgnoreCase));
            if (g is null) return false;
            return g.TotalCumulativeDuration.TotalMilliseconds >= (s.End - s.Start).TotalMilliseconds * pct / 100.0;
        });

        var allProviders = sections
            .SelectMany(s => s.Groups.Select(g => g.ProviderName))
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .OrderByDescending(p => sections.Sum(s => s.Groups
                .Where(g => g.ProviderName.Equals(p, StringComparison.OrdinalIgnoreCase))
                .Sum(g => g.SpecialChildren.Sum(sc => sc.Occurrences.Count))))
            .ToList();

        var providers = _viewModel.ShowAllProviders ? allProviders : allProviders.Where(IsPrioritized).ToList();
        var hasRemaining = allProviders.Count > providers.Count;
        ShowRemainingProvidersButton.Visibility = (hasRemaining || _viewModel.ShowAllProviders) ? Visibility.Visible : Visibility.Collapsed;
        ShowRemainingProvidersButton.Content = _viewModel.ShowAllProviders ? "Show Only Prioritized Providers" : "Show All Providers";

        if (providers.Count == 0)
        {
            Track2ProviderStackPanel.Children.Add(new TextBlock
            {
                Text = "No provider events in this region for any trace.",
                Foreground = new SolidColorBrush(Windows.UI.Color.FromArgb(255, 0xAA, 0xAA, 0xAA)),
            });
            return;
        }

        bool first = true;
        foreach (var provider in providers)
        {
            int total = sections.Sum(s => s.Groups
                .Where(g => g.ProviderName.Equals(provider, StringComparison.OrdinalIgnoreCase))
                .Sum(g => g.SpecialChildren.Sum(sc => sc.Occurrences.Count)));

            var expander = new Expander
            {
                HorizontalAlignment = HorizontalAlignment.Stretch,
                HorizontalContentAlignment = HorizontalAlignment.Stretch,
                Margin = new Thickness(0, 0, 0, 8),
                Header = $"{provider} ({total} events)   Ctrl+scroll to zoom, drag to pan",
                IsExpanded = first,
            };
            var content = new StackPanel { Orientation = Orientation.Vertical, Spacing = 6 };
            expander.Content = content;

            var providerLocal = provider;
            var built = false;
            void Build()
            {
                if (built) return;
                built = true;
                BuildProviderAcrossTraces(providerLocal, sections, content);
            }
            if (first) { Build(); first = false; }
            expander.Expanding += (_, _) => Build();

            Track2ProviderStackPanel.Children.Add(expander);
        }
    }

    private void BuildProviderAcrossTraces(string provider, TraceSection[] sections, StackPanel content)
    {
        // One color map per provider, shared by all traces, so a given event name is the
        // same color in every trace's lane — and one legend covers all of them.
        var eventColors = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        var palette = new[] { "#56B4E9", "#E69F00", "#009E73", "#F0E442", "#0072B2", "#D55E00", "#CC79A7", "#7FB3D5", "#A569BD", "#52BE80", "#F39C12", "#5DADE2" };
        var idx = 0;
        foreach (var s in sections)
        {
            var pg = s.Groups.FirstOrDefault(x => x.ProviderName.Equals(provider, StringComparison.OrdinalIgnoreCase));
            if (pg is null) continue;
            foreach (var sc in pg.SpecialChildren)
                if (!eventColors.ContainsKey(sc.EventName)) eventColors[sc.EventName] = palette[idx++ % palette.Length];
        }

        // One legend per provider; ticking a chip selects an event to plot in every lane.
        var selected = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var chips = new Dictionary<string, ToggleButton>(StringComparer.OrdinalIgnoreCase);
        if (eventColors.Count > 0)
        {
            var legend = new CommunityToolkit.WinUI.Controls.WrapPanel { HorizontalSpacing = 8, VerticalSpacing = 4, Margin = new Thickness(0, 0, 0, 4) };
            foreach (var kv in eventColors.OrderBy(k => k.Key))
            {
                var inner = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 4 };
                inner.Children.Add(new Border { Width = 14, Height = 14, CornerRadius = new CornerRadius(2), Background = new SolidColorBrush(ParseColor(kv.Value)), VerticalAlignment = VerticalAlignment.Center });
                inner.Children.Add(new TextBlock { Text = kv.Key, FontSize = 13, Foreground = AppBrush("AppText") });
                var chip = new ToggleButton { Content = inner, Padding = new Thickness(10, 5, 10, 5), FontSize = 13 };
                chips[kv.Key] = chip;
                legend.Children.Add(chip);
            }
            content.Children.Add(legend);
        }

        // Per-trace lanes: empty until the user ticks legend chips. Each lane draws only the
        // selected events as bars (hover shows event name + occurrence start/end). Chips toggle
        // selection and redraw every lane for this provider.
        var laneRedraws = new List<Action>();
        foreach (var s in sections)
        {
            var g = s.Groups.FirstOrDefault(x => x.ProviderName.Equals(provider, StringComparison.OrdinalIgnoreCase));
            if (g is null) continue;

            var totalMs = (s.End - s.Start).TotalMilliseconds;
            if (totalMs <= 0) totalMs = 1;
            var ev = g.SpecialChildren.Sum(sc => sc.Occurrences.Count);
            var key = provider + "||" + s.TraceLabel;
            _t2View[key] = (0.0, 1.0);

            content.Children.Add(new TextBlock
            {
                Text = $"{s.TraceLabel} ({ev} events)",
                Foreground = AppBrush("SubText"),
                FontSize = 10,
                Margin = new Thickness(0, 2, 0, 0),
            });

            var box = new Border
            {
                Background = AppBrush("LaneBg"),
                CornerRadius = new CornerRadius(4),
                Padding = new Thickness(2),
                Margin = new Thickness(0, 0, 0, 6),
            };
            var grid = new Grid();
            grid.RowDefinitions.Add(new RowDefinition { Height = new GridLength(T2AxisHeight) });
            grid.RowDefinitions.Add(new RowDefinition { Height = new GridLength(T2LaneHeight) });

            var axis = new Canvas { Height = T2AxisHeight, Background = AppBrush("PanelBg") };
            var bars = new Canvas { Height = T2LaneHeight, Background = AppBrush("LaneBg2") };
            Grid.SetRow(axis, 0); Grid.SetRow(bars, 1);
            grid.Children.Add(axis); grid.Children.Add(bars);
            box.Child = grid;
            content.Children.Add(box);

            // Per-lane info panel: total occurrences + total time for each ticked event.
            var info = new StackPanel { Orientation = Orientation.Vertical, Spacing = 1, Margin = new Thickness(2, 0, 0, 8) };
            content.Children.Add(info);

            var gLocal = g; var startLocal = s.Start; var msLocal = totalMs; var colLocal = eventColors; var keyLocal = key; var offLocal = s.StartOffsetMs; var infoLocal = info;
            void Redraw()
            {
                DrawT2Axis(axis, msLocal, keyLocal, offLocal);
                DrawSelectedBars(bars, gLocal, startLocal, msLocal, colLocal, keyLocal, offLocal, selected);
                UpdateLaneInfo(infoLocal, gLocal, colLocal, selected);
            }
            axis.SizeChanged += (_, _) => Redraw();
            bars.SizeChanged += (_, _) => Redraw();
            AttachT2Zoom(bars, keyLocal, Redraw);
            AttachT2Zoom(axis, keyLocal, Redraw);
            laneRedraws.Add(Redraw);
        }

        // Wire legend chips now that the lanes exist: toggling redraws all lanes.
        foreach (var kv in chips)
        {
            var name = kv.Key;
            kv.Value.Checked += (_, _) => { selected.Add(name); foreach (var r in laneRedraws) r(); };
            kv.Value.Unchecked += (_, _) => { selected.Remove(name); foreach (var r in laneRedraws) r(); };
        }
    }

    // Draws only the legend-selected events as bars; hover shows event name + start/end.
    private void DrawSelectedBars(Canvas canvas, SpecialChildrenByProvider group, DateTime start, double totalMs, Dictionary<string, string> colors, string key, double offsetMs, HashSet<string> selected)
    {
        canvas.Children.Clear();
        var w = canvas.ActualWidth; var h = canvas.ActualHeight; if (w <= 0 || h <= 0) return;
        if (selected.Count == 0) return;  // empty until events are ticked
        var (vs, ve) = _t2View[key]; var span = ve - vs; if (span <= 0) return;
        var drawn = 0;
        foreach (var sc in group.SpecialChildren)
        {
            if (!selected.Contains(sc.EventName)) continue;
            var brush = new SolidColorBrush(colors.TryGetValue(sc.EventName, out var c) ? ParseColor(c) : ParseColor("#56B4E9"));
            foreach (var (a, b) in sc.Occurrences)
            {
                var f1 = (a - start).TotalMilliseconds / totalMs; var f2 = (b - start).TotalMilliseconds / totalMs;
                var x1 = (f1 - vs) / span * w; var x2 = (f2 - vs) / span * w;
                if (x2 < 0 || x1 > w) continue;
                var left = Math.Max(0, x1); var right = Math.Min(w, x2);
                var rect = new Rectangle { Width = Math.Max(1, right - left), Height = h - 6, Fill = brush };
                var tip = $"{sc.EventName}\nStart: {FormatMs((a - start).TotalMilliseconds + offsetMs)}\nEnd: {FormatMs((b - start).TotalMilliseconds + offsetMs)}";
                ToolTipService.SetToolTip(rect, new ToolTip { Content = tip });
                Canvas.SetLeft(rect, left); Canvas.SetTop(rect, 3); canvas.Children.Add(rect);
                if (++drawn >= 1500) return;
            }
        }
    }

    private void AttachT2Zoom(Canvas canvas, string key, Action redraw)
    {
        canvas.PointerWheelChanged += (s, e) =>
        {
            var ctrl = (Microsoft.UI.Input.InputKeyboardSource.GetKeyStateForCurrentThread(Windows.System.VirtualKey.Control) & Windows.UI.Core.CoreVirtualKeyStates.Down) != 0;
            if (!ctrl) return;
            var p = e.GetCurrentPoint(canvas); var w = canvas.ActualWidth; if (w <= 0) return;
            var (vs, ve) = _t2View[key]; var span = ve - vs;
            var cursor = vs + (p.Position.X / w) * span;
            var span2 = Math.Clamp(span * (p.Properties.MouseWheelDelta > 0 ? 0.8 : 1.25), 0.0005, 1.0);
            var r = (cursor - vs) / span; vs = cursor - r * span2; ve = vs + span2;
            if (vs < 0) { ve -= vs; vs = 0; } if (ve > 1) { vs -= (ve - 1); ve = 1; }
            _t2View[key] = (Math.Max(0, vs), Math.Min(1, ve)); redraw(); e.Handled = true;
        };
        Windows.Foundation.Point origin = default; double os = 0, oe = 0; bool pan = false;
        canvas.PointerPressed += (s, e) => { pan = true; origin = e.GetCurrentPoint(canvas).Position; (os, oe) = _t2View[key]; canvas.CapturePointer(e.Pointer); };
        canvas.PointerMoved += (s, e) => { if (!pan) return; var w = canvas.ActualWidth; if (w <= 0) return; var dx = e.GetCurrentPoint(canvas).Position.X - origin.X; var d = -(dx / w) * (oe - os); var vs = os + d; var ve = oe + d; if (vs < 0) { ve -= vs; vs = 0; } if (ve > 1) { vs -= (ve - 1); ve = 1; } _t2View[key] = (Math.Max(0, vs), Math.Min(1, ve)); redraw(); };
        canvas.PointerReleased += (s, e) => { pan = false; canvas.ReleasePointerCapture(e.Pointer); };
    }

    private void DrawT2Axis(Canvas canvas, double totalMs, string key, double offsetMs)
    {
        canvas.Children.Clear();
        var w = canvas.ActualWidth; if (w <= 0 || totalMs <= 0) return;
        var (vs, ve) = _t2View[key]; var span = (ve - vs) * totalMs;
        var ticks = Math.Max(2, (int)(w / 120)); var interval = NiceInterval(span / ticks);
        var startMs = vs * totalMs; var first = Math.Ceiling(startMs / interval) * interval;
        var tb = new SolidColorBrush(Windows.UI.Color.FromArgb(255, 0x88, 0x88, 0x88));
        var lb = AppBrush("AppText");
        for (var ms = first; ms <= ve * totalMs; ms += interval)
        {
            var x = (ms / totalMs - vs) / (ve - vs) * w; if (x < 0 || x > w) continue;
            canvas.Children.Add(new Line { X1 = x, X2 = x, Y1 = 12, Y2 = 20, Stroke = tb, StrokeThickness = 1 });
            canvas.Children.Add(Pos(new TextBlock { Text = FormatMs(ms + offsetMs), Foreground = lb, FontSize = 9 }, x + 2, 0));
        }
    }

    private PhaseDefinition? GetPhaseDefinitionForItem(TimelineItem item)
        => _definitions.FirstOrDefault(d => string.Equals(d.Name, item.Name, StringComparison.OrdinalIgnoreCase));

    private void OnShowRemainingProviders(object sender, RoutedEventArgs e)
    {
        _viewModel.ShowAllProviders = !_viewModel.ShowAllProviders;
        if (_multiTrack2Region is not null) RenderMultiTraceTrack2(_multiTrack2Region);
    }

    private void OnCloseTrack2(object sender, RoutedEventArgs e)
    {
        Track2ProviderStackPanel.Children.Clear();
        _multiTrack2Region = null;
        _currentTrack2Parent = null;
        _viewModel.IsTrack2Visible = false;
        DrawTrack1();
    }

    // Per-lane summary: occurrences + total time consumed for each selected event.
    private void UpdateLaneInfo(StackPanel info, SpecialChildrenByProvider group, Dictionary<string, string> colors, HashSet<string> selected)
    {
        info.Children.Clear();
        if (selected.Count == 0) return;
        foreach (var sc in group.SpecialChildren)
        {
            if (!selected.Contains(sc.EventName)) continue;
            var row = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 6 };
            row.Children.Add(new Border { Width = 10, Height = 10, CornerRadius = new CornerRadius(2), VerticalAlignment = VerticalAlignment.Center, Background = new SolidColorBrush(colors.TryGetValue(sc.EventName, out var c) ? ParseColor(c) : ParseColor("#56B4E9")) });
            row.Children.Add(new TextBlock
            {
                Text = $"{sc.EventName}: {sc.Occurrences.Count} occ, {sc.CumulativeDuration.TotalMilliseconds:F2} ms total",
                FontSize = 11,
                Foreground = AppBrush("SubText"),
            });
            info.Children.Add(row);
        }
    }

    private static T Pos<T>(T el, double left, double top) where T : UIElement { Canvas.SetLeft(el, left); Canvas.SetTop(el, top); return el; }
}
