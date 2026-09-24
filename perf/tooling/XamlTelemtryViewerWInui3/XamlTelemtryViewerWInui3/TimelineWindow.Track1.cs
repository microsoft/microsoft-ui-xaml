using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;
using Windows.UI.Input;
using Microsoft.UI.Dispatching;
using XamlTelemtryViewerWInui3.Helpers;
using XamlTelemtryViewerWInui3.Models;
using XamlTelemtryViewerWInui3.Models.Timeline;
using XamlTelemtryViewerWInui3.ViewModels;
using XamlTelemtryViewerWInui3.Services;
using static XamlTelemtryViewerWInui3.Helpers.TimelineMath;
using static XamlTelemtryViewerWInui3.Helpers.TimelineLayout;
using static XamlTelemtryViewerWInui3.Helpers.TimelineColorHelper;

namespace XamlTelemtryViewerWInui3;

// Track 1 (main process-lifecycle timeline): drawing, input, zoom/pan view-state, and export.
public sealed partial class TimelineWindow : Window
{
    // Per-lane vertical layout (multi-trace stacking). Each trace gets one band:
    // [label] over [marker zone] over [phase segment], with a gap below.
    private const double LaneLabelHeight = 16.0;
    private const double LaneMarkerZone = 10.0;
    private const double LaneSegmentHeight = 45.0;
    private const double LaneGap = 10.0;
    private const double LaneTotal = LaneLabelHeight + LaneMarkerZone + LaneSegmentHeight + LaneGap;

    // Common time scale (ms) shared by every lane: max lane duration. All lanes are
    // aligned at their own launch start (t=0), so a longer launch extends further right.
    private double _track1GlobalDurationMs = 0;

    private void OnResetZoom(object sender, RoutedEventArgs e)
    {
        _track1ViewStart = 0.0;
        _track1ViewEnd = 1.0;
        Redraw();
    }

    /// <summary>
    /// Entry point: User clicked a Track 1 item (phase or marker)
    /// Extracts special children in that time range and shows Track 2
    /// </summary>
    private void OnTrack1ItemClick(TimelineLane lane, TimelineItem selectedItem)
    {
        _currentTrack2Parent = selectedItem;
        RenderMultiTraceTrack2(selectedItem.Name);

        DrawTrack1(); // Redraw Track1 to show parent boundary markers
    }

    private void OnCanvasSizeChanged(object sender, SizeChangedEventArgs e)
    {
        Redraw();
    }

    private void OnCanvasMouseWheel(object sender, PointerRoutedEventArgs e)
    {
        var model = _viewModel.Model;
        if (model is null) return;
        if (sender is not FrameworkElement container) return;

        var point = e.GetCurrentPoint(container);
        var delta = point.Properties.MouseWheelDelta;
        if (delta == 0) return;

        // Only zoom if Ctrl is held
        var keyStates = Microsoft.UI.Input.InputKeyboardSource.GetKeyStateForCurrentThread(Windows.System.VirtualKey.Control);
        if ((keyStates & Windows.UI.Core.CoreVirtualKeyStates.Down) == 0) return; // Ctrl not pressed, let scroll happen

        var mouseX = point.Position.X;
        var containerWidth = container.ActualWidth;
        if (containerWidth <= 0) return;

        var viewSpan = _track1ViewEnd - _track1ViewStart;
        var cursorFraction = _track1ViewStart + (mouseX / containerWidth) * viewSpan;
        var zoomFactor = delta > 0 ? 0.8 : 1.25;
        var newSpan = Math.Clamp(viewSpan * zoomFactor, 0.0001, 1.0);

        var ratio = (cursorFraction - _track1ViewStart) / viewSpan;
        _track1ViewStart = cursorFraction - ratio * newSpan;
        _track1ViewEnd = _track1ViewStart + newSpan;

        ClampTrack1View();
        Redraw();
        e.Handled = true;
    }

    /// <summary>
    /// Handle touchpad Ctrl+pinch-to-zoom gesture on canvases.
    /// </summary>
    private void OnCanvasManipulationDelta(object sender, ManipulationDeltaRoutedEventArgs e)
    {
        var model = _viewModel.Model;
        if (model is null) return;
        if (sender is not FrameworkElement container) return;

        // Scale factor from pinch gesture (only zoom if scale changed significantly)
        var scale = e.Delta.Scale;
        if (Math.Abs(scale - 1.0) < 0.01) return;

        var centerX = e.Position.X;
        var containerWidth = container.ActualWidth;
        if (containerWidth <= 0) return;

        var viewSpan = _track1ViewEnd - _track1ViewStart;
        var centerFraction = _track1ViewStart + (centerX / containerWidth) * viewSpan;

        // Zoom: scale < 1 = zoom out, scale > 1 = zoom in
        var zoomFactor = 1.0 / scale;
        var newSpan = Math.Clamp(viewSpan * zoomFactor, 0.0001, 1.0);

        var ratio = (centerFraction - _track1ViewStart) / viewSpan;
        _track1ViewStart = centerFraction - ratio * newSpan;
        _track1ViewEnd = _track1ViewStart + newSpan;

        ClampTrack1View();
        Redraw();
        e.Handled = true;
    }

    private void OnCanvasPointerDown(object sender, PointerRoutedEventArgs e)
    {
        if (e.GetCurrentPoint((UIElement)sender).Properties.IsLeftButtonPressed)
        {
            _isPanning = true;
            _panOrigin = e.GetCurrentPoint((UIElement)sender).Position;
            _panStartViewStart = _track1ViewStart;
            _panStartViewEnd = _track1ViewEnd;
            (sender as UIElement)?.CapturePointer(e.Pointer);
            e.Handled = true;
        }
    }

    private void OnCanvasPointerMoved(object sender, PointerRoutedEventArgs e)
    {
        if (!_isPanning) return;
        if (sender is not FrameworkElement container) return;

        var pos = e.GetCurrentPoint((UIElement)sender).Position;
        var dx = pos.X - _panOrigin.X;
        var containerWidth = container.ActualWidth;
        if (containerWidth <= 0) return;

        var fractionDelta = -(dx / containerWidth) * (_panStartViewEnd - _panStartViewStart);
        _track1ViewStart = _panStartViewStart + fractionDelta;
        _track1ViewEnd = _panStartViewEnd + fractionDelta;

        ClampTrack1View();
        Redraw();
    }

    private void OnCanvasPointerUp(object sender, PointerRoutedEventArgs e)
    {
        if (_isPanning)
        {
            _isPanning = false;
            (sender as UIElement)?.ReleasePointerCapture(e.Pointer);
            e.Handled = true;
        }
    }

    private void ClampTrack1View()
    {
        if (_track1ViewStart < 0) { _track1ViewEnd -= _track1ViewStart; _track1ViewStart = 0; }
        if (_track1ViewEnd > 1) { _track1ViewStart -= (_track1ViewEnd - 1); _track1ViewEnd = 1; }
        _track1ViewStart = Math.Max(0, _track1ViewStart);
        _track1ViewEnd = Math.Min(1, _track1ViewEnd);
    }

    private List<TimelineLane> ActiveLanes()
    {
        var lanes = _viewModel?.Lanes;
        var result = new List<TimelineLane>();
        if (lanes is null) return result;
        // One lane per loaded trace, even if it has zero Track 1 items, so the user
        // always sees as many lanes as traces (empty ones render label-only).
        foreach (var lane in lanes)
        {
            if (lane.Model is not null)
            {
                result.Add(lane);
            }
        }
        return result;
    }

    private static bool HasItems(TimelineLane lane) => lane.Model is not null && lane.Model.TrackOneItems.Count > 0;

    private static (DateTime start, DateTime end) LaneBounds(TimelineLane lane)
    {
        var items = lane.Model!.TrackOneItems;
        if (items.Count == 0) return (DateTime.MinValue, DateTime.MinValue);
        var start = items.Min(i => i.Start);
        var end = items.Max(i => i.End ?? i.Start);
        return (start, end);
    }

    private void Redraw()
    {
        var lanes = ActiveLanes();

        // Global time scale = longest lane duration; every lane aligned at its own t=0.
        _track1GlobalDurationMs = 0;
        foreach (var lane in lanes)
        {
            if (!HasItems(lane)) continue;
            var (s, e) = LaneBounds(lane);
            _track1GlobalDurationMs = Math.Max(_track1GlobalDurationMs, (e - s).TotalMilliseconds);
        }

        // Primary lane bounds drive the single-trace Track 2 boundary overlay.
        var primary = lanes.FirstOrDefault(HasItems);
        if (primary is not null)
        {
            var (ps, pe) = LaneBounds(primary);
            _firstMarkerTime = ps;
            _lastMarkerTime = pe;
        }

        DrawTimeAxis();
        DrawTrack1();
        UpdateLegend();
    }

    private void DrawTimeAxis()
    {
        TimeAxisCanvas.Children.Clear();
        if (_track1GlobalDurationMs <= 0) return;

        var w = TimeAxisCanvas.ActualWidth;
        var h = TimeAxisCanvas.ActualHeight;
        if (w <= 0 || h <= 0) return;

        var timelineDurationMs = _track1GlobalDurationMs;

        var viewStart = _track1ViewStart;
        var viewEnd = _track1ViewEnd;
        var visibleStartMs = viewStart * timelineDurationMs;
        var visibleEndMs = viewEnd * timelineDurationMs;
        var visibleSpanMs = visibleEndMs - visibleStartMs;

        var tickCount = Math.Max(2, (int)(w / 120));
        var tickInterval = NiceInterval(visibleSpanMs / tickCount);
        var firstTick = Math.Ceiling(visibleStartMs / tickInterval) * tickInterval;

        var tickBrush = new SolidColorBrush(Windows.UI.Color.FromArgb(255, 0x88, 0x88, 0x88));
        var labelBrush = AppBrush("AppText");

        double lastLabelRightEdge = -100; // Track last label's right edge to avoid overlaps

        for (var ms = firstTick; ms <= visibleEndMs; ms += tickInterval)
        {
            // Skip if this tick is very close to the actual end time (within 5ms)
            if (Math.Abs(ms - timelineDurationMs) < 5) continue;

            var fraction = ms / timelineDurationMs;
            var x = FractionToPixelTrack1(fraction, w);
            if (x < 0 || x > w) continue;

            var line = new Line
            {
                X1 = x,
                X2 = x,
                Y1 = 12,
                Y2 = 20,
                Stroke = tickBrush,
                StrokeThickness = 1,
            };
            TimeAxisCanvas.Children.Add(line);

            var label = new TextBlock
            {
                Text = FormatMs(ms),
                Foreground = labelBrush,
                FontSize = 9,
            };
            
            // Skip label if it would overlap with previous label (need ~50px minimum spacing)
            if (x + 2 < lastLabelRightEdge + 15) continue;

            Canvas.SetLeft(label, x + 2);
            Canvas.SetTop(label, 0);
            TimeAxisCanvas.Children.Add(label);
            
            // Estimate label width (~40px for typical timestamp) to track overlap
            lastLabelRightEdge = x + 2 + 40;
        }

        // Always draw end marker at timeline end (fraction = 1.0)
        var endX = FractionToPixelTrack1(1.0, w);
        if (endX >= 0 && endX <= w)
        {
            var endLine = new Line
            {
                X1 = endX,
                X2 = endX,
                Y1 = 12,
                Y2 = 20,
                Stroke = tickBrush,
                StrokeThickness = 1,
            };
            TimeAxisCanvas.Children.Add(endLine);

            var endLabel = new TextBlock
            {
                Text = FormatMs(timelineDurationMs),
                Foreground = labelBrush,
                FontSize = 9,
                TextAlignment = TextAlignment.Right,
            };
            Canvas.SetLeft(endLabel, Math.Max(0, endX - 50)); // Right-align within 50px width
            Canvas.SetTop(endLabel, 0);
            TimeAxisCanvas.Children.Add(endLabel);
        }
    }

    private void DrawTrack1()
    {
        Track1Canvas.Children.Clear();
        var lanes = ActiveLanes();
        if (lanes.Count == 0) return;
        if (_track1GlobalDurationMs <= 0) return;

        var w = Track1Canvas.ActualWidth;
        if (w <= 0) return;

        // Grow the canvas to hold every lane (label + marker zone + segment + gap each).
        var desiredHeight = lanes.Count * LaneTotal;
        if (double.IsNaN(Track1Canvas.Height) || Math.Abs(Track1Canvas.Height - desiredHeight) > 0.5)
        {
            Track1Canvas.Height = desiredHeight;
        }

        for (var i = 0; i < lanes.Count; i++)
        {
            DrawTrack1Lane(lanes[i], i * LaneTotal, w, interactive: true);
        }

        // Track 2 parent boundaries: single-trace drill-down only (Track 2 is deferred for
        // multi-trace). Spans the one lane's segment band.
        if (lanes.Count == 1 && _currentTrack2Parent is not null && _firstMarkerTime is not null)
        {
            var boundaryStartTime = _firstMarkerTime.Value;
            if (_track1GlobalDurationMs > 0)
            {
                var parentStart = _currentTrack2Parent.Start;
                var parentEnd = _currentTrack2Parent.End ?? _currentTrack2Parent.Start.AddMilliseconds(1);
                var startX = FractionToPixelTrack1((parentStart - boundaryStartTime).TotalMilliseconds / _track1GlobalDurationMs, w);
                var endX = FractionToPixelTrack1((parentEnd - boundaryStartTime).TotalMilliseconds / _track1GlobalDurationMs, w);
                var h = LaneTotal;
                var boundaryBrush = new SolidColorBrush(Windows.UI.Color.FromArgb(255, 255, 215, 0));

                if (startX >= 0 && startX <= w)
                    Track1Canvas.Children.Add(new Line { X1 = startX, X2 = startX, Y1 = 0, Y2 = h, Stroke = boundaryBrush, StrokeThickness = 2, Opacity = 0.7 });
                if (endX >= 0 && endX <= w)
                    Track1Canvas.Children.Add(new Line { X1 = endX, X2 = endX, Y1 = 0, Y2 = h, Stroke = boundaryBrush, StrokeThickness = 2, Opacity = 0.7 });
                if (startX >= 0 && endX >= 0 && startX < endX && endX <= w)
                {
                    var overlay = new Rectangle { Width = endX - startX, Height = h, Fill = new SolidColorBrush(Windows.UI.Color.FromArgb(40, 255, 215, 0)) };
                    Canvas.SetLeft(overlay, startX);
                    Canvas.SetTop(overlay, 0);
                    Track1Canvas.Children.Insert(0, overlay);
                }
            }
        }
    }

    // Draws one trace's Track 1 (phases + markers) into a vertical band starting at baseY.
    // All lanes share _track1GlobalDurationMs for scale and align at their own launch start.
    private void DrawTrack1Lane(TimelineLane lane, double baseY, double w, bool interactive)
    {
        var model = lane.Model!;
        var (timelineStart, _) = LaneBounds(lane);
        var timelineDurationMs = _track1GlobalDurationMs;

        const double markerZone = LaneMarkerZone;
        var labelTop = baseY;
        var markerTop = baseY + LaneLabelHeight;
        var segmentTop = markerTop + markerZone;
        var segmentHeight = LaneSegmentHeight;

        // Lane label (trace + process).
        var laneLabel = new TextBlock
        {
            Text = lane.Label,
            Foreground = AppBrush("AppText"),
            FontSize = 11,
            FontWeight = Microsoft.UI.Text.FontWeights.SemiBold,
        };
        Canvas.SetLeft(laneLabel, 2);
        Canvas.SetTop(laneLabel, labelTop);
        Track1Canvas.Children.Add(laneLabel);

        var ordered = new List<TimelineItem>(model.TrackOneItems);
        ordered.Sort((a, b) => a.Start.CompareTo(b.Start));

        // Draw phases (items with End time)
        foreach (var item in ordered.Where(i => i.End.HasValue))
        {
            var startFraction = (item.Start - timelineStart).TotalMilliseconds / timelineDurationMs;
            var endFraction = ((item.End ?? item.Start) - timelineStart).TotalMilliseconds / timelineDurationMs;

            var x1 = FractionToPixelTrack1(startFraction, w);
            var x2 = FractionToPixelTrack1(endFraction, w);
            var width = Math.Max(1, x2 - x1);

            var startMs = (item.Start - timelineStart).TotalMilliseconds;
            var endMs = (item.End.HasValue ? (item.End.Value - timelineStart).TotalMilliseconds : startMs);

            var color = ParseColor(item.Color);
            var rect = new Rectangle
            {
                Width = width,
                Height = segmentHeight,
                Fill = new SolidColorBrush(color),
                StrokeThickness = 0.5,
                Stroke = new SolidColorBrush(Windows.UI.Color.FromArgb(255, 100, 100, 100)),
            };

            // Add tooltip for phase
            var tooltip = new StackPanel { MaxWidth = 300 };
            tooltip.Children.Add(new TextBlock
            {
                Text = item.Name,
                FontWeight = Microsoft.UI.Text.FontWeights.Bold,
                FontSize = 12,
                TextWrapping = TextWrapping.Wrap,
            });
            
            // Start event info
            if (item.StartEvent?.ProviderName != null)
            {
                var startEventText = $"{item.StartEvent.ProviderName} / {item.StartEvent.EventName}";
                tooltip.Children.Add(new TextBlock
                {
                    Text = $"Start: {startEventText}",
                    FontSize = 11,
                    Margin = new Thickness(0, 4, 0, 0),
                    Foreground = new SolidColorBrush(Windows.UI.Color.FromArgb(255, 0xB0, 0xB0, 0xB0)),
                });
            }
            
            // End event info
            if (item.EndEvent?.ProviderName != null)
            {
                var endEventText = $"{item.EndEvent.ProviderName} / {item.EndEvent.EventName}";
                tooltip.Children.Add(new TextBlock
                {
                    Text = $"Stop: {endEventText}",
                    FontSize = 11,
                    Foreground = new SolidColorBrush(Windows.UI.Color.FromArgb(255, 0xB0, 0xB0, 0xB0)),
                });
            }
            
            tooltip.Children.Add(new TextBlock
            {
                Text = $"Start: {FormatMs(startMs)}",
                FontSize = 11,
                Margin = new Thickness(0, 4, 0, 0),
            });
            tooltip.Children.Add(new TextBlock
            {
                Text = $"End: {FormatMs(endMs)}",
                FontSize = 11,
            });
            tooltip.Children.Add(new TextBlock
            {
                Text = $"Duration: {FormatMs(endMs - startMs)}",
                FontSize = 11,
            });
            
            ToolTipService.SetToolTip(rect, tooltip);
            ToolTipService.SetPlacement(rect, Microsoft.UI.Xaml.Controls.Primitives.PlacementMode.Bottom);

            Canvas.SetLeft(rect, x1);
            Canvas.SetTop(rect, segmentTop);
            
            // Add click handler to open Track 2 (single-trace only)
            if (interactive)
            {
                rect.PointerPressed += (s, e) => OnTrack1ItemClick(lane, item);
            }
            
            Track1Canvas.Children.Add(rect);

            // Draw label if space permits
            if (width > 40)
            {
                var label = new TextBlock
                {
                    Text = item.Name,
                    Foreground = new SolidColorBrush(Windows.UI.Color.FromArgb(255, 255, 255, 255)),
                    FontSize = 10,
                };
                Canvas.SetLeft(label, x1 + 3);
                Canvas.SetTop(label, segmentTop + 2);
                Track1Canvas.Children.Add(label);
            }
        }

        // Draw markers (items without End time) as small downward triangles
        var markerItems = ordered.Where(i => !i.End.HasValue).ToList();
        var timestampGroups = GroupItemsByTimestamp(markerItems);
        
        foreach (var item in markerItems)
        {
            var fraction = (item.Start - timelineStart).TotalMilliseconds / timelineDurationMs;
            if (fraction < _track1ViewStart || fraction > _track1ViewEnd) continue;
            
            var x = FractionToPixelTrack1(fraction, w);
            // Include markers at exact boundaries (use <= for upper bound to include edge markers)
            if (x < 0 || x > w) continue;

            var (yOffset, _) = CalculateMarkerYOffset(item.Start, item, timestampGroups, markerZone);
            var color = ParseColor(item.Color);
            var brush = new SolidColorBrush(color);

            // Triangle marker with white stroke
            var triangle = new Polygon
            {
                Points = new()
                {
                    new Windows.Foundation.Point(x - 4, markerTop + yOffset),
                    new Windows.Foundation.Point(x + 4, markerTop + yOffset),
                    new Windows.Foundation.Point(x, markerTop + markerZone - 1 + yOffset),
                },
                Fill = brush,
                Stroke = new SolidColorBrush(Windows.UI.Color.FromArgb(255, 255, 255, 255)),
                StrokeThickness = 0.5,
            };
            Track1Canvas.Children.Add(triangle);

            // Vertical line extending from marker down to segment top, visible on top of phases
            var dashArray = new Microsoft.UI.Xaml.Media.DoubleCollection { 2, 2 };
            var verticalLine = new Line
            {
                X1 = x,
                X2 = x,
                Y1 = markerTop + markerZone + yOffset,  // End of triangle
                Y2 = segmentTop,                        // Top of phase segment
                Stroke = brush,
                StrokeThickness = 2,
                StrokeDashArray = dashArray,  // Dashed line for visibility
            };
            Track1Canvas.Children.Add(verticalLine);

            // 1px notch on the segment's top edge for visual alignment
            var notch = new Line
            {
                X1 = x,
                X2 = x,
                Y1 = segmentTop,
                Y2 = segmentTop + 3,
                Stroke = brush,
                StrokeThickness = 1,
            };
            Track1Canvas.Children.Add(notch);

            // Invisible hit pad for easy hover/click interaction
            var padHeight = Math.Min(12, markerZone + 3);
            var pad = new Rectangle
            {
                Width = 10,
                Height = padHeight,
                Fill = new SolidColorBrush(Windows.UI.Color.FromArgb(0, 0, 0, 0)),
            };
            Canvas.SetLeft(pad, x - 5);
            Canvas.SetTop(pad, markerTop + yOffset);
            
            var tooltip = new StackPanel();
            tooltip.Children.Add(new TextBlock 
            { 
                Text = item.Name, 
                FontWeight = Microsoft.UI.Text.FontWeights.Bold, 
                FontSize = 12,
            });
            
            // Event info
            if (item.StartEvent?.ProviderName != null)
            {
                var eventText = $"{item.StartEvent.ProviderName} / {item.StartEvent.EventName}";
                tooltip.Children.Add(new TextBlock 
                { 
                    Text = eventText, 
                    FontSize = 11,
                    Margin = new Thickness(0, 4, 0, 0),
                    Foreground = new SolidColorBrush(Windows.UI.Color.FromArgb(255, 0xB0, 0xB0, 0xB0)),
                });
            }
            
            tooltip.Children.Add(new TextBlock 
            { 
                Text = $"Time: {FormatMs((item.Start - timelineStart).TotalMilliseconds)}", 
                FontSize = 11,
                Margin = new Thickness(0, 4, 0, 0),
            });
            
            ToolTipService.SetToolTip(pad, tooltip);
            ToolTipService.SetPlacement(pad, Microsoft.UI.Xaml.Controls.Primitives.PlacementMode.Bottom);
            
            // Add click handler to open Track 2 (single-trace only)
            if (interactive)
            {
                pad.PointerPressed += (s, e) => OnTrack1ItemClick(lane, item);
            }
            
            Track1Canvas.Children.Add(pad);
        }
    }

    private void UpdateLegend()
    {
        var seen = new HashSet<string>();
        var entries = new List<LegendEntry>();
        foreach (var lane in ActiveLanes())
        {
            foreach (var item in lane.Model!.TrackOneItems)
            {
                if (seen.Add(item.Name))
                {
                    var color = ParseColor(item.Color);
                    entries.Add(new LegendEntry(item.Name, new SolidColorBrush(color)));
                }
            }
        }
        LegendList.ItemsSource = entries;
    }

    private double FractionToPixelTrack1(double fraction, double width)
    {
        const double padding = 16.0;  // 16px padding on each side for marker visibility
        var usableWidth = Math.Max(1, width - 2 * padding);
        var viewSpan = Track1ViewSpan;
        if (viewSpan <= 0) return padding;
        return padding + (fraction - _track1ViewStart) / viewSpan * usableWidth;
    }
}
