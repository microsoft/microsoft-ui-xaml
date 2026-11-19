using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Windows.Graphics;
using Windows.UI;

namespace RenderBench;

// S9: many windows. Interaction runs a fixed lifecycle, one step every StepSeconds with a SceneStep marker (and a
// memory sample) per step: baseline, open-1..N (a new Window per S1 Settings instance, seeded Seed+i), minimize-all,
// restore-all, close-all, settled (after a full GC). --tabs instead hosts 20 S1 tabs in one TabView: baseline (tab 1),
// open-tab-2..20 (visits each tab), switch (cycles back through), close-tabs (removes all but the first), settled.
// The sequence always runs to completion, so Interaction lasts max(--interaction, steps x StepSeconds).
sealed class MultiWindowScene : IScene
{
    public const int TabCount = 20;

    readonly BenchOptions _options;
    readonly List<Window> _windows = [];
    readonly TextBlock _status = new() { FontSize = 16 };
    readonly TabView? _tabs;

    public string Name => _options.Tabs ? "MultiWindow-Tabs" : "MultiWindow";
    public bool IsDynamic => false;
    public FrameworkElement Root { get; }
    public IReadOnlyList<FrameworkElement> InputTargets => [];

    public MultiWindowScene(BenchOptions options)
    {
        _options = options;
        if (options.Tabs)
        {
            _tabs = new TabView { IsAddTabButtonVisible = false, TabWidthMode = TabViewWidthMode.Compact };
            for (int i = 0; i < TabCount; i++)
            {
                // Content is created lazily on first visit so open-tab-k measures realizing tab k.
                _tabs.TabItems.Add(new TabViewItem { Header = $"Settings {i + 1}", IsClosable = false,
                    Content = i == 0 ? Settings(0) : null });
            }
            _tabs.SelectedIndex = 0;
            Root = _tabs;
            return;
        }
        _status.Text = $"Hosting up to {options.Windows} Settings windows";
        Root = new Grid
        {
            Background = new SolidColorBrush(Color.FromArgb(255, 0xFA, 0xFA, 0xFA)),
            Children = { new StackPanel { Padding = new Thickness(24), Spacing = 8, Children =
                { new TextBlock { Text = "Multiwindow", Style = SettingsScene.Style("TitleTextBlockStyle") }, _status } } },
        };
    }

    FrameworkElement Settings(int index) => new SettingsScene(_options with { Seed = _options.Seed + index }).Root;

    public int ApplyUpdate() => 0;

    // Warmup realizes one Settings tree in-process (templates, styles) and discards it, so open-1 isn't paying for
    // first-use costs that the in-window baseline didn't.
    public Task? WarmupAsync(ISceneHost host) => _tabs is null ? WarmupWindowAsync() : null;

    async Task WarmupWindowAsync()
    {
        var window = NewWindow(-1);
        await Task.Delay(500);
        window.Close();
        await Task.Delay(200);
    }

    Window NewWindow(int index)
    {
        var window = new Window { Title = $"RenderBench Settings {index + 1}", Content = Settings(Math.Max(0, index)) };
        int offset = 30 * Math.Max(0, index);
        window.AppWindow.MoveAndResize(new RectInt32(80 + offset, 80 + offset, Math.Min(_options.Width, 900), Math.Min(_options.Height, 700)));
        window.Activate();
        return window;
    }

    public Task? RunInteractionAsync(ISceneHost host, CancellationToken token) => _tabs is null ? RunWindowsAsync(host) : RunTabsAsync(host, _tabs);

    async Task RunWindowsAsync(ISceneHost host)
    {
        var wait = TimeSpan.FromSeconds(_options.StepSeconds);
        int step = 0;
        host.Step(step++, "baseline", 0);
        await Task.Delay(wait);
        for (int i = 0; i < _options.Windows; i++)
        {
            _windows.Add(NewWindow(i));
            _status.Text = $"{_windows.Count} windows open";
            await Task.Delay(wait);
            host.Step(step++, $"open-{i + 1}", _windows.Count);
        }
        await Task.Delay(wait);
        ForEachPresenter(p => p.Minimize());
        host.Step(step++, "minimize-all", _windows.Count);
        await Task.Delay(wait);
        ForEachPresenter(p => p.Restore());
        host.Step(step++, "restore-all", _windows.Count);
        await Task.Delay(wait);
        CloseAll();
        _status.Text = "All windows closed";
        host.Step(step++, "close-all", 0);
        await Task.Delay(wait);
        Collect();
        host.Step(step, "settled", 0);
        await Task.Delay(wait);
    }

    async Task RunTabsAsync(ISceneHost host, TabView tabs)
    {
        var wait = TimeSpan.FromSeconds(_options.StepSeconds);
        int step = 0;
        host.Step(step++, "baseline", 1);
        await Task.Delay(wait);
        for (int i = 1; i < TabCount; i++)
        {
            var item = (TabViewItem)tabs.TabItems[i];
            item.Content ??= Settings(i);
            host.MarkInput("tab", "TabViewItem");
            tabs.SelectedIndex = i;
            await Task.Delay(wait);
            host.Step(step++, $"open-tab-{i + 1}", i + 1);
        }
        // Switching between already-realized tabs.
        for (int i = TabCount - 2; i >= 0; i -= 3)
        {
            host.MarkInput("tab", "TabViewItem");
            tabs.SelectedIndex = i;
            await Task.Delay(TimeSpan.FromMilliseconds(250));
        }
        host.Step(step++, "switch", TabCount);
        await Task.Delay(wait);
        tabs.SelectedIndex = 0;
        while (tabs.TabItems.Count > 1)
        {
            tabs.TabItems.RemoveAt(tabs.TabItems.Count - 1);
        }
        host.Step(step++, "close-tabs", 1);
        await Task.Delay(wait);
        Collect();
        host.Step(step, "settled", 1);
        await Task.Delay(wait);
    }

    void ForEachPresenter(Action<OverlappedPresenter> action)
    {
        foreach (var window in _windows)
        {
            if (window.AppWindow.Presenter is OverlappedPresenter presenter)
            {
                action(presenter);
            }
        }
    }

    void CloseAll()
    {
        foreach (var window in _windows)
        {
            window.Close();
        }
        _windows.Clear();
    }

    static void Collect()
    {
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
    }

    public IEnumerable<(string Name, double Value)> Counters()
    {
        yield return ("OpenWindows", _windows.Count + 1);
    }

    public void Detach() => CloseAll();
}
