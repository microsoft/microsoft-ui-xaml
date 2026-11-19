using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;

namespace RenderBench;

// What a scene can ask of the harness while it runs (implemented by BenchRunner).
interface ISceneHost
{
    BenchOptions Options { get; }
    Window Window { get; }
    nint Hwnd { get; }
    // Resolved input path for this run; never Auto.
    InputMode InputMode { get; }
    DispatcherQueue Queue { get; }

    // SendInput mode: a real click at the target's center. Programmatic mode: IScene.Activate(target), i.e. the same
    // state change through the XAML API. Emits InputInjected either way and returns its id.
    long Activate(FrameworkElement target, string kind = "click");

    // Emits InputInjected. Programmatic state changes get the "programmatic:" kind prefix (excluded from R1); pass
    // programmatic explicitly when a scene uses the other path than the run's InputMode.
    long MarkInput(string kind, string target, bool? programmatic = null);

    // S9 lifecycle step: samples memory (closing the previous step), then emits SceneStep.
    void Step(int step, string name, int windows);

    // ApplyUpdate + UpdateApplied marker.
    void Update();

    // Runs onFrame(seconds since the previous frame) on every CompositionTarget.Rendering until cancelled.
    Task EveryFrame(Action<double> onFrame, CancellationToken token);

    // Runs action on the UI thread at hz until cancelled.
    Task Every(double hz, Action action, CancellationToken token);

    (int X, int Y) ScreenPoint(FrameworkElement element, double fx = 0.5, double fy = 0.5);
}

static class ProgrammaticInput
{
    // Generic programmatic equivalent of a click on the common controls used by the scenes.
    public static void Activate(FrameworkElement target)
    {
        switch (target)
        {
            case ToggleSwitch toggle:
                toggle.IsOn = !toggle.IsOn;
                break;
            case RadioButton radio:
                radio.IsChecked = true;
                break;
            case ToggleButton toggle: // CheckBox, ToggleButton
                toggle.IsChecked = toggle.IsChecked != true;
                break;
            case Slider slider:
                slider.Value = slider.Value >= (slider.Minimum + slider.Maximum) / 2 ? slider.Minimum + (slider.Maximum - slider.Minimum) * 0.25
                    : slider.Minimum + (slider.Maximum - slider.Minimum) * 0.75;
                break;
            case Control control:
                control.Focus(FocusState.Programmatic);
                break;
        }
    }
}

static class Tree
{
    public static T? Find<T>(DependencyObject root) where T : DependencyObject
    {
        int count = VisualTreeHelper.GetChildrenCount(root);
        for (int i = 0; i < count; i++)
        {
            var child = VisualTreeHelper.GetChild(root, i);
            if (child is T match)
            {
                return match;
            }
            if (Find<T>(child) is { } nested)
            {
                return nested;
            }
        }
        return null;
    }
}

// Constant-speed programmatic scrolling (ChangeView without animation, once per frame), reversing at either end.
sealed class ScrollDriver(ScrollViewer viewer, double dipsPerSecond, bool horizontal = false)
{
    int _direction = 1;
    double _offset = double.NaN;

    public void Advance(double seconds)
    {
        double extent = horizontal ? viewer.ScrollableWidth : viewer.ScrollableHeight;
        if (extent <= 0)
        {
            return;
        }
        if (double.IsNaN(_offset))
        {
            _offset = horizontal ? viewer.HorizontalOffset : viewer.VerticalOffset;
        }
        // Clamp so a stalled frame doesn't jump more than ~100 ms of travel.
        _offset += _direction * dipsPerSecond * Math.Min(seconds, 0.1);
        if (_offset >= extent)
        {
            _offset = extent;
            _direction = -1;
        }
        else if (_offset <= 0)
        {
            _offset = 0;
            _direction = 1;
        }
        if (horizontal)
        {
            viewer.ChangeView(_offset, null, null, disableAnimation: true);
        }
        else
        {
            viewer.ChangeView(null, _offset, null, disableAnimation: true);
        }
    }

    // SendInput variant: one wheel notch toward the current direction, reversing at the ends.
    public void Wheel(ISceneHost host, FrameworkElement over)
    {
        double offset = horizontal ? viewer.HorizontalOffset : viewer.VerticalOffset;
        double extent = horizontal ? viewer.ScrollableWidth : viewer.ScrollableHeight;
        if (offset >= extent - 1)
        {
            _direction = -1;
        }
        else if (offset <= 1)
        {
            _direction = 1;
        }
        var (x, y) = host.ScreenPoint(over);
        host.MarkInput("wheel", over.GetType().Name, programmatic: false);
        // WHEEL_DELTA 120 per notch; positive scrolls up (vertical) or right (horizontal).
        Win32.Wheel(x, y, horizontal ? 120 * _direction : -120 * _direction, horizontal);
    }
}
