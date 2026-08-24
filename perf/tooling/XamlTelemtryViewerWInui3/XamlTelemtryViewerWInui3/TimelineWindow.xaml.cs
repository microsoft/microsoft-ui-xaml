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

public sealed partial class TimelineWindow : Window
{
    private readonly TimelineViewModel _viewModel;
    private TraceData? _trace;
    private IReadOnlyList<PhaseDefinition> _definitions = new List<PhaseDefinition>();

    // Track 1 zoom/pan state as [0..1] fraction
    private double _track1ViewStart = 0.0;
    private double _track1ViewEnd = 1.0;
    private double Track1ViewSpan => _track1ViewEnd - _track1ViewStart;

    // Track 2 bounds (multi-trace path lives in TimelineWindow.Track2Multi.cs)
    private DateTime? _firstMarkerTime = null;
    private DateTime? _lastMarkerTime = null;
    private TimelineItem? _currentTrack2Parent = null;

    // Pan state
    private bool _isPanning = false;
    private Windows.Foundation.Point _panOrigin;
    private double _panStartViewStart;
    private double _panStartViewEnd;

    public TimelineWindow()
    {
        InitializeComponent();
    }

    public TimelineWindow(TimelineViewModel viewModel, TraceData? trace, IReadOnlyList<PhaseDefinition> definitions, Window? parentWindow = null)
    {
        InitializeComponent();
        _viewModel = viewModel;
        _trace = trace;
        _definitions = definitions;
        this.Title = "Timeline View — Process Lifecycle";

        if (this.Content is Grid rootGrid)
        {
            rootGrid.DataContext = viewModel;
        }

        // When parent window closes, close this window too
        if (parentWindow != null)
        {
            parentWindow.Closed += (_, _) =>
            {
                this.Close();
            };
        }

        // Listen to ViewModel property changes to redraw when data changes
        _viewModel.PropertyChanged += (sender, e) =>
        {
            if (e.PropertyName == nameof(TimelineViewModel.Model))
            {
                // Reset zoom when process changes
                _track1ViewStart = 0.0;
                _track1ViewEnd = 1.0;
                _currentTrack2Parent = null;
                Track2ProviderStackPanel.Children.Clear();
                _viewModel.IsTrack2Visible = false;
                Redraw();
            }
        };

        // Any lane's process change rebuilds that lane; redraw all stacked lanes.
        _viewModel.LanesChanged += () => Redraw();
    }

    // Resolves a named brush from the app's (dark-only) resources for code-built canvases.
    private static SolidColorBrush AppBrush(string key) => (SolidColorBrush)Application.Current.Resources[key];

    /// <summary>
    /// ScrollViewer wheel handler: Allow scroll unless Ctrl is pressed over Track2.
    /// When Ctrl+scroll on Track2, mark as handled to prevent scroll and let Canvas zoom handlers work.
    /// </summary>
    private void OnScrollViewerPointerWheel(object sender, PointerRoutedEventArgs e)
    {
        if (sender is not ScrollViewer scrollViewer) return;

        var point = e.GetCurrentPoint(scrollViewer);
        
        // Check if Ctrl is held
        var keyStates = Microsoft.UI.Input.InputKeyboardSource.GetKeyStateForCurrentThread(Windows.System.VirtualKey.Control);
        bool isCtrlPressed = (keyStates & Windows.UI.Core.CoreVirtualKeyStates.Down) != 0;

        if (!isCtrlPressed) return; // Normal scroll, let ScrollViewer handle it

        // Ctrl is pressed - check if over a Track2 provider canvas
        var elementsAtPoint = VisualTreeHelper.FindElementsInHostCoordinates(point.Position, scrollViewer);
        
        var isOverTrack2Canvas = elementsAtPoint.Any(elem =>
            (elem is Canvas canvas && canvas.Parent is Grid) || // Track2 provider canvases
            (elem is FrameworkElement fe && fe.Name?.StartsWith("Track2Provider") == true));

        if (isOverTrack2Canvas)
        {
            // Ctrl+scroll over Track2: block scroll and let Canvas zoom handler work
            e.Handled = true;
        }
    }

    private void OnAIAnalysis(object sender, RoutedEventArgs e)
    {
        // TODO: Implement AI analysis
    }

    private void OnOpenInWPA(object sender, RoutedEventArgs e)
    {
        // TODO: Implement WPA integration
    }

    /// <summary>
    /// Legend entry (event name and color)
    /// </summary>
    private sealed record LegendEntry(string Name, SolidColorBrush ColorBrush);
}

public class LegendEntry
{
    public string Name { get; set; }
    public SolidColorBrush ColorBrush { get; set; }

    public LegendEntry(string name, SolidColorBrush colorBrush)
    {
        Name = name;
        ColorBrush = colorBrush;
    }
}
