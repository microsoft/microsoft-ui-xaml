using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Animation;
using Windows.UI;

namespace RenderBench;

// S6: always-animating controls. ProgressRing (AnimatedVisualPlayer: a Lottie Composition visual; --no-progressring
// drops it), indeterminate ProgressBar, an InfoBar whose Opacity fades in/out forever (Storyboard), an Expander that
// expands/collapses every 2 s and ToggleSwitches toggled by input (knob animation). It animates during Steady by
// design: Steady CPU is the "animation tax" (E1) and the idle gate is waived.
sealed class AnimatedScene : IScene
{
    readonly List<FrameworkElement> _targets = [];
    readonly InfoBar _info;
    readonly Expander _expander;
    readonly Storyboard _fade;
    DispatcherQueueTimer? _expanderTimer;
    int _updates;

    public string Name => "Animated";
    public bool IsDynamic => true;
    public bool AnimatesWhileIdle => true;
    public bool ContinuousInteraction => true;
    public FrameworkElement Root { get; }
    public IReadOnlyList<FrameworkElement> InputTargets => _targets;

    public AnimatedScene(BenchOptions options)
    {
        var random = new Random(options.Seed);
        var panel = new StackPanel { Spacing = 16, Padding = new Thickness(24), MaxWidth = 720, HorizontalAlignment = HorizontalAlignment.Left };
        panel.Children.Add(new TextBlock { Text = "Activity", Style = SettingsScene.Style("TitleTextBlockStyle") });

        var progress = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 24 };
        if (!options.NoProgressRing)
        {
            progress.Children.Add(new ProgressRing { IsActive = true, Width = 56, Height = 56 });
        }
        progress.Children.Add(new ProgressBar { IsIndeterminate = true, Width = 420, VerticalAlignment = VerticalAlignment.Center });
        panel.Children.Add(progress);

        _info = new InfoBar
        {
            IsOpen = true,
            IsClosable = false,
            Title = "Syncing",
            Message = "Synchronizing 0 items",
            Severity = InfoBarSeverity.Informational,
        };
        panel.Children.Add(_info);
        var fade = new DoubleAnimation { From = 1, To = 0.15, Duration = TimeSpan.FromSeconds(1.5), AutoReverse = true,
            RepeatBehavior = RepeatBehavior.Forever, EnableDependentAnimation = false };
        Storyboard.SetTarget(fade, _info);
        Storyboard.SetTargetProperty(fade, "Opacity");
        _fade = new Storyboard { Children = { fade } };

        var details = new StackPanel { Spacing = 4 };
        for (int i = 0; i < 4; i++)
        {
            details.Children.Add(new TextBlock { Text = $"Folder {i + 1}: {random.Next(10, 999)} files, {random.Next(1, 90)} MB" });
        }
        _expander = new Expander { Header = "Details", Content = details, HorizontalAlignment = HorizontalAlignment.Stretch,
            HorizontalContentAlignment = HorizontalAlignment.Left };
        panel.Children.Add(_expander);

        var toggles = new StackPanel { Spacing = 4 };
        for (int i = 0; i < 4; i++)
        {
            var toggle = new ToggleSwitch { Header = $"Sync option {i + 1}", IsOn = random.Next(2) == 0 };
            _targets.Add(toggle);
            toggles.Children.Add(toggle);
        }
        panel.Children.Add(toggles);

        Root = new Grid { Background = new SolidColorBrush(Color.FromArgb(255, 0xFA, 0xFA, 0xFA)), Children = { panel } };
    }

    public void Attach(ISceneHost host)
    {
        _fade.Begin();
        _expanderTimer = host.Queue.CreateTimer();
        _expanderTimer.Interval = TimeSpan.FromSeconds(2);
        _expanderTimer.IsRepeating = true;
        _expanderTimer.Tick += (_, _) => _expander.IsExpanded = !_expander.IsExpanded;
        _expanderTimer.Start();
    }

    public int ApplyUpdate()
    {
        _info.Message = $"Synchronizing {++_updates * 7} items";
        return 1;
    }

    public void Detach()
    {
        _expanderTimer?.Stop();
        _fade.Stop();
    }
}
