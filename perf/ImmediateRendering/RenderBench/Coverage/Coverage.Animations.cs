using System.Numerics;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Animation;
using Microsoft.UI.Xaml.Navigation;
using Microsoft.UI.Xaml.Shapes;
using Windows.UI;

namespace RenderBench.Coverage;

// F10 animations and transitions (transitions branch). Every page runs an XAML-owned animation in its Exercise step:
// property/brush transitions, ConnectedAnimation, theme transitions, theme animations, visual state transitions and
// MUXC control animations. None may fall back to Composition. Pages that can be stopped at a fixed point (Storyboard
// Seek + Pause) capture mid-animation in every mode; the others capture the settled end state. Mid-animation frames of
// XAML-evaluated animations (transitions, ConnectedAnimation) can be held at a progress in immediate modes with
// WINUI_IMMEDIATE_ANIMATION_HOLD=<0..1> (proposal doc, "Full XAML coverage: transitions").
static class CoverageAnimations
{
    const string F = "F10-animations";
    const string Crossfade = "ConnectedAnimation cross-fade:approximated";

    // ---- helpers (self-contained: pages compile without the coverage branch's kit) ----

    static Grid Page(params UIElement[] children)
    {
        var grid = new Grid { RequestedTheme = ElementTheme.Light, Padding = new Thickness(24) };
        grid.SetValue(Panel.BackgroundProperty, Application.Current.Resources["ApplicationPageBackgroundThemeBrush"]);
        foreach (var child in children)
        {
            grid.Children.Add(child);
        }
        return grid;
    }

    static Border Tile(Color color, string text, double width = 160, double height = 100) => new()
    {
        Width = width,
        Height = height,
        CornerRadius = new CornerRadius(8),
        Background = new LinearGradientBrush { StartPoint = new(0, 0), EndPoint = new(1, 1), GradientStops = { new GradientStop { Color = color, Offset = 0 }, new GradientStop { Color = Colors.White, Offset = 1 } } },
        Child = new TextBlock { Text = text, Margin = new Thickness(12), FontSize = 18, Foreground = new SolidColorBrush(Colors.Black) },
    };

    static StackPanel Row(params UIElement[] children)
    {
        var row = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 24 };
        foreach (var child in children)
        {
            row.Children.Add(child);
        }
        return row;
    }

    static Task Settle(int ms = 900) => Task.Delay(ms);

    // Waits for the first matching event (or throws after the timeout: the animation never completed).
    static async Task WithTimeout(Task task, string what, int ms = 5000)
    {
        if (await Task.WhenAny(task, Task.Delay(ms)) != task)
        {
            throw new TimeoutException($"{what} did not complete within {ms} ms");
        }
    }

    // A storyboard held at a fixed point: identical in every mode.
    static async Task SeekAndPause(Storyboard storyboard, double milliseconds)
    {
        storyboard.Begin();
        storyboard.Seek(TimeSpan.FromMilliseconds(milliseconds));
        storyboard.Pause();
        await Settle(400);
    }

    // ---- 1. XAML-owned property and brush transitions ----

    [CoveragePage(F, "ScalarTransition-Opacity")]
    public static CoveragePage OpacityTransition(CoverageContext context)
    {
        var tile = Tile(Colors.SteelBlue, "Opacity");
        tile.OpacityTransition = new ScalarTransition { Duration = TimeSpan.FromMilliseconds(300) };
        return new() { Root = Page(Row(tile)), Exercise = async () => { tile.Opacity = 0.3; await Settle(); } };
    }

    [CoveragePage(F, "Vector3Transition-ScaleTranslation")]
    public static CoveragePage Vector3Transitions(CoverageContext context)
    {
        var scaled = Tile(Colors.Orange, "Scale");
        var moved = Tile(Colors.SeaGreen, "Translation");
        var rotated = Tile(Colors.MediumPurple, "Rotation");
        scaled.ScaleTransition = new Vector3Transition { Duration = TimeSpan.FromMilliseconds(300) };
        moved.TranslationTransition = new Vector3Transition { Duration = TimeSpan.FromMilliseconds(300) };
        rotated.RotationTransition = new ScalarTransition { Duration = TimeSpan.FromMilliseconds(300) };
        rotated.CenterPoint = new Vector3(80, 50, 0);
        return new()
        {
            Root = Page(Row(scaled, moved, rotated)),
            Exercise = async () =>
            {
                scaled.Scale = new Vector3(1.3f, 0.8f, 1);
                moved.Translation = new Vector3(30, 60, 0);
                rotated.Rotation = 30;
                await Settle();
            },
        };
    }

    [CoveragePage(F, "BrushTransition-Background")]
    public static CoveragePage BrushTransitions(CoverageContext context)
    {
        var grid = new Grid { Width = 160, Height = 100, Background = new SolidColorBrush(Colors.LightGray) };
        var border = new Border { Width = 160, Height = 100, Background = new SolidColorBrush(Colors.LightGray) };
        var presenter = new ContentPresenter { Width = 160, Height = 100, Background = new SolidColorBrush(Colors.LightGray), Content = "Presenter" };
        grid.BackgroundTransition = new BrushTransition { Duration = TimeSpan.FromMilliseconds(300) };
        border.BackgroundTransition = new BrushTransition { Duration = TimeSpan.FromMilliseconds(300) };
        presenter.BackgroundTransition = new BrushTransition { Duration = TimeSpan.FromMilliseconds(300) };
        return new()
        {
            Root = Page(Row(grid, border, presenter)),
            Exercise = async () =>
            {
                grid.Background = new SolidColorBrush(Colors.Tomato);
                border.Background = new SolidColorBrush(Colors.CornflowerBlue);
                presenter.Background = new SolidColorBrush(Colors.Gold);
                await Settle();
            },
        };
    }

    // ---- 2. ConnectedAnimation ----

    static CoveragePage Connected(Func<ConnectedAnimationConfiguration?> configuration, bool coordinated = false, bool hideSource = true)
    {
        var source = Tile(Colors.DodgerBlue, "Source", 120, 80);
        var destination = Tile(Colors.DodgerBlue, "Destination", 280, 180);
        var caption = new TextBlock { Text = "Coordinated caption", FontSize = 16 };
        destination.Visibility = Visibility.Collapsed;
        caption.Visibility = Visibility.Collapsed;
        var right = new StackPanel { Spacing = 12, Children = { destination, caption } };
        var root = Page(Row(source, right));
        return new()
        {
            Root = root,
            Exercise = async () =>
            {
                var service = ConnectedAnimationService.GetForCurrentView();
                var animation = service.PrepareToAnimate("f10", source);
                if (configuration() is { } config)
                {
                    animation.Configuration = config;
                }
                var completed = new TaskCompletionSource();
                animation.Completed += (_, _) => completed.TrySetResult();
                if (hideSource)
                {
                    source.Opacity = 0;
                }
                destination.Visibility = Visibility.Visible;
                caption.Visibility = Visibility.Visible;
                await Task.Yield();
                bool started = coordinated ? animation.TryStart(destination, [caption]) : animation.TryStart(destination);
                if (!started)
                {
                    throw new InvalidOperationException("ConnectedAnimation.TryStart returned false");
                }
                await WithTimeout(completed.Task, "ConnectedAnimation");
                await Settle(300);
            },
        };
    }

    [CoveragePage(F, "ConnectedAnimation-Gravity", ExpectedUnsupported = [Crossfade])]
    public static CoveragePage ConnectedGravity(CoverageContext context) => Connected(() => null);

    [CoveragePage(F, "ConnectedAnimation-Direct", ExpectedUnsupported = [Crossfade])]
    public static CoveragePage ConnectedDirect(CoverageContext context) => Connected(() => new DirectConnectedAnimationConfiguration());

    [CoveragePage(F, "ConnectedAnimation-Basic", ExpectedUnsupported = [Crossfade])]
    public static CoveragePage ConnectedBasic(CoverageContext context) => Connected(() => new BasicConnectedAnimationConfiguration());

    [CoveragePage(F, "ConnectedAnimation-Coordinated", ExpectedUnsupported = [Crossfade])]
    public static CoveragePage ConnectedCoordinated(CoverageContext context) => Connected(() => null, coordinated: true);

    // ---- 3. Theme transitions (end state), theme animations and visual state transitions (held mid-way) ----

    static CoveragePage ChildrenTransition(Transition transition, Action<Panel>? afterLoad = null)
    {
        var panel = new StackPanel { Spacing = 8, Width = 240 };
        panel.ChildrenTransitions = new TransitionCollection { transition };
        return new()
        {
            Root = Page(panel),
            Exercise = async () =>
            {
                for (int i = 0; i < 4; i++)
                {
                    panel.Children.Add(Tile(Colors.Teal, $"Item {i + 1}", 240, 44));
                }
                await Settle();
                afterLoad?.Invoke(panel);
                await Settle();
            },
        };
    }

    [CoveragePage(F, "EntranceThemeTransition")]
    public static CoveragePage Entrance(CoverageContext context) => ChildrenTransition(new EntranceThemeTransition());

    [CoveragePage(F, "AddDeleteThemeTransition")]
    public static CoveragePage AddDelete(CoverageContext context) => ChildrenTransition(new AddDeleteThemeTransition(), p => p.Children.RemoveAt(1));

    [CoveragePage(F, "RepositionThemeTransition")]
    public static CoveragePage Reposition(CoverageContext context) => ChildrenTransition(new RepositionThemeTransition(), p => p.Children.Insert(0, new Border { Height = 30 }));

    [CoveragePage(F, "ContentThemeTransition")]
    public static CoveragePage ContentTransition(CoverageContext context)
    {
        var host = new ContentControl { ContentTransitions = new TransitionCollection { new ContentThemeTransition() }, Content = Tile(Colors.Gray, "Old") };
        return new() { Root = Page(host), Exercise = async () => { host.Content = Tile(Colors.Crimson, "New"); await Settle(); } };
    }

    [CoveragePage(F, "EdgeUIThemeTransition")]
    public static CoveragePage EdgeUI(CoverageContext context) => ChildrenTransition(new EdgeUIThemeTransition { Edge = EdgeTransitionLocation.Left });

    [CoveragePage(F, "PaneThemeTransition")]
    public static CoveragePage PaneTransition(CoverageContext context) => ChildrenTransition(new PaneThemeTransition { Edge = EdgeTransitionLocation.Right });

    [CoveragePage(F, "PopupThemeTransition")]
    public static CoveragePage PopupTransition(CoverageContext context)
    {
        var popup = new Popup
        {
            HorizontalOffset = 40,
            VerticalOffset = 40,
            ChildTransitions = new TransitionCollection { new PopupThemeTransition() },
            Child = Tile(Colors.Goldenrod, "Popup"),
        };
        var root = Page(popup);
        return new() { Root = root, Exercise = async () => { popup.IsOpen = true; await Settle(); }, Detach = () => popup.IsOpen = false };
    }

    // NavigationThemeTransition on a content host (the transition Frame uses), with the navigation info as its default.
    static CoveragePage NavigationPage(NavigationTransitionInfo info)
    {
        var host = new ContentControl
        {
            Width = 400,
            Height = 240,
            ContentTransitions = new TransitionCollection { new NavigationThemeTransition { DefaultNavigationTransitionInfo = info } },
            Content = Tile(Colors.Gray, "Previous page", 300, 180),
        };
        return new()
        {
            Root = Page(host),
            Exercise = async () =>
            {
                await Settle(400);
                host.Content = Tile(Colors.SlateBlue, "Next page", 300, 180);
                await Settle();
            },
        };
    }

    [CoveragePage(F, "NavigationThemeTransition-DrillIn")]
    public static CoveragePage NavigationDrillIn(CoverageContext context) => NavigationPage(new DrillInNavigationTransitionInfo());

    [CoveragePage(F, "NavigationThemeTransition-Slide")]
    public static CoveragePage NavigationSlide(CoverageContext context) => NavigationPage(new SlideNavigationTransitionInfo { Effect = SlideNavigationTransitionEffect.FromRight });

    [CoveragePage(F, "NavigationThemeTransition-Suppress")]
    public static CoveragePage NavigationSuppress(CoverageContext context) => NavigationPage(new SuppressNavigationTransitionInfo());

    static CoveragePage HeldThemeAnimation(Func<string, Timeline> animation, double atMs)
    {
        var tile = Tile(Colors.DarkCyan, "Theme animation");
        tile.Name = "Target";
        var storyboard = new Storyboard();
        var root = Page(Row(tile));
        root.Resources["sb"] = storyboard;
        storyboard.Children.Add(animation("Target"));
        return new() { Root = root, Exercise = () => SeekAndPause(storyboard, atMs), Detach = storyboard.Stop };
    }

    [CoveragePage(F, "FadeOutThemeAnimation-Held")]
    public static CoveragePage FadeOut(CoverageContext context) => HeldThemeAnimation(t => new FadeOutThemeAnimation { TargetName = t }, 80);

    [CoveragePage(F, "PointerDownThemeAnimation-Held")]
    public static CoveragePage PointerDown(CoverageContext context) => HeldThemeAnimation(t => new PointerDownThemeAnimation { TargetName = t }, 50);

    [CoveragePage(F, "PopInThemeAnimation-Held")]
    public static CoveragePage PopIn(CoverageContext context) => HeldThemeAnimation(t => new PopInThemeAnimation { TargetName = t, FromHorizontalOffset = 60 }, 150);

    [CoveragePage(F, "SwipeHintThemeAnimation-Held")]
    public static CoveragePage SwipeHint(CoverageContext context) => HeldThemeAnimation(t => new SwipeHintThemeAnimation { TargetName = t, ToHorizontalOffset = 40 }, 100);

    [CoveragePage(F, "DrillInThemeAnimation-Held")]
    public static CoveragePage DrillIn(CoverageContext context)
    {
        var entering = Tile(Colors.Indigo, "Entering");
        var exiting = Tile(Colors.Gray, "Exiting");
        entering.Name = "Entering";
        exiting.Name = "Exiting";
        var storyboard = new Storyboard { Children = { new DrillInThemeAnimation { EntranceTargetName = "Entering", ExitTargetName = "Exiting" } } };
        var root = Page(Row(exiting, entering));
        root.Resources["sb"] = storyboard;
        return new() { Root = root, Exercise = () => SeekAndPause(storyboard, 120), Detach = storyboard.Stop };
    }

    [CoveragePage(F, "Storyboard-TransformOpacity-Held")]
    public static CoveragePage StoryboardHeld(CoverageContext context)
    {
        var tile = Tile(Colors.OrangeRed, "Storyboard");
        var transform = new CompositeTransform();
        tile.RenderTransform = transform;
        var storyboard = new Storyboard();
        void Add(DependencyObject target, string path, double to)
        {
            var a = new DoubleAnimation { To = to, Duration = TimeSpan.FromSeconds(1) };
            Storyboard.SetTarget(a, target);
            Storyboard.SetTargetProperty(a, path);
            storyboard.Children.Add(a);
        }
        Add(transform, "TranslateX", 200);
        Add(transform, "Rotation", 45);
        Add(tile, "Opacity", 0.2);
        return new() { Root = Page(Row(tile)), Exercise = () => SeekAndPause(storyboard, 500), Detach = storyboard.Stop };
    }

    // Visual state changes with generated/explicit VisualTransitions (ToggleSwitch knob, CheckBox glyph, Expander content).
    static CoveragePage VisualStatePage(FrameworkElement control, Action change) => new()
    {
        Root = Page(new StackPanel { Spacing = 16, Children = { control } }),
        Exercise = async () => { await Settle(300); change(); await Settle(1200); },
    };

    // A template's VisualTransition with a generated duration (VSM builds the transition storyboard: color, opacity
    // and transform). Programmatic ToggleSwitch.IsOn crashes the harness in every mode (0xC000027B), so the generated
    // transition is exercised with an app template instead.
    [CoveragePage(F, "VisualStateTransition-Generated")]
    public static CoveragePage VisualStateGenerated(CoverageContext context)
    {
        var control = new ContentControl
        {
            Template = (ControlTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load("""
                <ControlTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' TargetType='ContentControl'>
                  <Grid Width='240' Height='120'>
                    <VisualStateManager.VisualStateGroups>
                      <VisualStateGroup>
                        <VisualStateGroup.Transitions>
                          <VisualTransition GeneratedDuration='0:0:0.4'/>
                        </VisualStateGroup.Transitions>
                        <VisualState x:Name='A' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'/>
                        <VisualState x:Name='B' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                          <VisualState.Setters>
                            <Setter Target='Knob.Opacity' Value='0.5'/>
                          </VisualState.Setters>
                          <Storyboard>
                            <DoubleAnimation Storyboard.TargetName='Move' Storyboard.TargetProperty='X' To='120' Duration='0'/>
                            <ColorAnimation Storyboard.TargetName='Fill' Storyboard.TargetProperty='Color' To='Crimson' Duration='0'/>
                          </Storyboard>
                        </VisualState>
                      </VisualStateGroup>
                    </VisualStateManager.VisualStateGroups>
                    <Border x:Name='Knob' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Width='100' Height='100' CornerRadius='50' HorizontalAlignment='Left'>
                      <Border.Background><SolidColorBrush x:Name='Fill' Color='SteelBlue'/></Border.Background>
                      <Border.RenderTransform><TranslateTransform x:Name='Move'/></Border.RenderTransform>
                    </Border>
                  </Grid>
                </ControlTemplate>
                """),
        };
        return VisualStatePage(control, () => VisualStateManager.GoToState(control, "B", true));
    }

    // The CheckBox glyph and the Expander chevron are AnimatedIcons (PathIcon fallback in immediate modes).
    [CoveragePage(F, "VisualStateTransition-CheckBox", ExpectedUnsupported = ["PathIcon AnimatedIcon:approximated"])]
    public static CoveragePage VisualStateCheck(CoverageContext context)
    {
        var check = new CheckBox { Content = "CheckBox" };
        return VisualStatePage(check, () => check.IsChecked = true);
    }

    [CoveragePage(F, "Expander-Expand", ExpectedUnsupported = ["PathIcon AnimatedIcon:approximated"])]
    public static CoveragePage ExpanderExpand(CoverageContext context)
    {
        var expander = new Microsoft.UI.Xaml.Controls.Expander { Header = "Expander", Content = Tile(Colors.Olive, "Content", 200, 60), Width = 260 };
        return VisualStatePage(expander, () => expander.IsExpanded = true);
    }

    // ---- 4. MUXC controls that animate themselves ----

    // The pane toggle button uses AnimatedIcon, which paints its PathIcon fallback in immediate modes.
    [CoveragePage(F, "NavigationView-SelectionIndicator", ExpectedUnsupported = ["PathIcon AnimatedIcon:approximated"])]
    public static CoveragePage NavigationViewIndicator(CoverageContext context)
    {
        var nav = new NavigationView { PaneDisplayMode = NavigationViewPaneDisplayMode.Left, IsSettingsVisible = false, IsBackButtonVisible = NavigationViewBackButtonVisible.Collapsed, Width = 640, Height = 420 };
        var first = new NavigationViewItem { Content = "Home", Icon = new SymbolIcon(Symbol.Home) };
        var second = new NavigationViewItem { Content = "Mail", Icon = new SymbolIcon(Symbol.Mail) };
        var third = new NavigationViewItem { Content = "Calendar", Icon = new SymbolIcon(Symbol.Calendar) };
        nav.MenuItems.Add(first);
        nav.MenuItems.Add(second);
        nav.MenuItems.Add(third);
        nav.SelectedItem = first;
        return new() { Root = Page(nav), Exercise = async () => { await Settle(400); nav.SelectedItem = third; await Settle(); } };
    }

    [CoveragePage(F, "TeachingTip-Open")]
    public static CoveragePage TeachingTipOpen(CoverageContext context)
    {
        var target = new Button { Content = "Target", Margin = new Thickness(40, 40, 0, 0) };
        var tip = new TeachingTip { Title = "Teaching tip", Subtitle = "Opens with its own animation.", Target = target, PreferredPlacement = TeachingTipPlacementMode.Bottom };
        var root = Page(target, tip);
        return new() { Root = root, Exercise = async () => { tip.IsOpen = true; await Settle(); }, Detach = () => tip.IsOpen = false };
    }

    [CoveragePage(F, "ItemsRepeater-Animator")]
    public static CoveragePage RepeaterAnimator(CoverageContext context)
    {
        var items = new System.Collections.ObjectModel.ObservableCollection<string>(Enumerable.Range(1, 6).Select(i => $"Item {i}"));
        var repeater = new ItemsRepeater
        {
            ItemsSource = items,
            Width = 260,
            ItemTemplate = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                "<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><Border Height='36' Margin='0,0,0,4' Background='LightSteelBlue'><TextBlock Text='{Binding}' Margin='8'/></Border></DataTemplate>"),
        };
        return new()
        {
            Root = Page(repeater),
            Exercise = async () =>
            {
                await Settle(400);
                items.RemoveAt(1);
                items.Insert(3, "Inserted");
                await Settle();
            },
        };
    }

    // LinedFlowLayout's item transition provider (what ItemsView uses) animates adds/removes/moves through its
    // immediate-aware Storyboard path. Hosted in a bare ItemsRepeater: ItemsView's ScrollView is the scrolling family's.
    [CoveragePage(F, "LinedFlowLayout-ItemTransitions")]
    public static CoveragePage LinedFlowTransitions(CoverageContext context)
    {
        var items = new System.Collections.ObjectModel.ObservableCollection<string>(Enumerable.Range(1, 8).Select(i => $"Item {i}"));
        var view = new ItemsRepeater
        {
            ItemsSource = items,
            Width = 480,
            Layout = new LinedFlowLayout { LineHeight = 48, LineSpacing = 6, MinItemSpacing = 6 },
            ItemTransitionProvider = new LinedFlowLayoutItemCollectionTransitionProvider(),
            ItemTemplate = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                "<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><Border Width='100' Background='LightSteelBlue'><TextBlock Text='{Binding}' Margin='8'/></Border></DataTemplate>"),
        };
        return new()
        {
            Root = Page(view),
            Exercise = async () =>
            {
                await Settle(400);
                items.RemoveAt(1);
                items.Insert(4, "Inserted");
                await Settle(1500);
            },
        };
    }
    [CoveragePage(F, "InfoBar-Open")]
    public static CoveragePage InfoBarOpen(CoverageContext context)
    {
        var info = new InfoBar { Title = "InfoBar", Message = "Opened by the page.", Severity = InfoBarSeverity.Success, IsOpen = false, Width = 420 };
        return new() { Root = Page(info), Exercise = async () => { info.IsOpen = true; await Settle(); } };
    }

    [CoveragePage(F, "RatingControl-Value")]
    public static CoveragePage Rating(CoverageContext context)
    {
        var rating = new RatingControl { Value = 2 };
        return new() { Root = Page(rating), Exercise = async () => { rating.Value = 4; await Settle(); } };
    }
}

