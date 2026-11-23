using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;
using Windows.UI;
using static RenderBench.Coverage.CoveragePageKit;

namespace RenderBench.Coverage;

// F8 controls: every MUX / MUXC control in its idle state, in light, dark and disabled variants (coverage branch).
static class CoverageControls
{
    const string F = "F8-controls";

    // ---- content groups ----

    static UIElement[][] Buttons() =>
    [
        [
            Label("Button"), new Button { Content = "Button" },
            new Button { Content = "Accent", Style = (Style)Application.Current.Resources["AccentButtonStyle"] },
            Label("RepeatButton"), new RepeatButton { Content = "Repeat" },
            Label("ToggleButton"), new ToggleButton { Content = "Off" }, new ToggleButton { Content = "On", IsChecked = true },
            Label("HyperlinkButton"), new HyperlinkButton { Content = "Hyperlink" },
        ],
        [
            Label("DropDownButton"), new DropDownButton { Content = "Drop down" },
            Label("SplitButton"), new SplitButton { Content = "Split" },
            Label("ToggleSplitButton"), new ToggleSplitButton { Content = "Toggle split", IsChecked = true },
            Label("AppBarButton / AppBarToggleButton"),
            new StackPanel
            {
                Orientation = Orientation.Horizontal,
                Children =
                {
                    new AppBarButton { Icon = new SymbolIcon(Symbol.Save), Label = "Save" },
                    new AppBarToggleButton { Icon = new SymbolIcon(Symbol.Pin), Label = "Pin", IsChecked = true },
                    new AppBarButton { Icon = new SymbolIcon(Symbol.Delete), Label = "Delete" },
                },
            },
        ],
    ];

    static UIElement[][] Toggles() =>
    [
        [
            Label("CheckBox"), new CheckBox { Content = "Unchecked" }, new CheckBox { Content = "Checked", IsChecked = true },
            new CheckBox { Content = "Indeterminate", IsThreeState = true, IsChecked = null },
            Label("RadioButton"), new RadioButton { Content = "First", GroupName = "g" },
            new RadioButton { Content = "Second", GroupName = "g", IsChecked = true },
        ],
        [
            Label("RadioButtons"), new RadioButtons { Header = "Options", ItemsSource = Words(3, "Option"), SelectedIndex = 1 },
            Label("ToggleSwitch"), new ToggleSwitch { Header = "Off" }, new ToggleSwitch { Header = "On", IsOn = true },
        ],
    ];

    static UIElement[][] Range() =>
    [
        [
            Label("Slider"), new Slider { Width = 260, Value = 40 },
            new Slider { Width = 260, Value = 70, TickFrequency = 10, TickPlacement = TickPlacement.Outside, Header = "Ticks" },
            new Slider { Height = 120, Value = 30, Orientation = Orientation.Vertical },
        ],
        [
            Label("ProgressBar (determinate)"), new ProgressBar { Width = 260, Value = 45 },
            new ProgressBar { Width = 260, Value = 80, ShowPaused = true },
            new ProgressBar { Width = 260, Value = 30, ShowError = true },
            Label("ProgressRing (determinate)"), new ProgressRing { IsIndeterminate = false, Value = 65, Width = 48, Height = 48 },
            Label("RatingControl"), new RatingControl { Value = 3.5, Caption = "3.5" },
            new RatingControl { PlaceholderValue = 2 },
        ],
    ];

    static UIElement[][] Pickers()
    {
        var date = new DateTimeOffset(2024, 3, 15, 0, 0, 0, TimeSpan.Zero);
        return
        [
            [
                Label("ComboBox (closed)"), new ComboBox { Width = 200, ItemsSource = Words(5), SelectedIndex = 2, Header = "Combo" },
                new ComboBox { Width = 200, PlaceholderText = "Pick one", ItemsSource = Words(5) },
                Label("CalendarDatePicker (closed)"), new CalendarDatePicker { Date = date, Header = "Date" },
                new CalendarDatePicker { PlaceholderText = "Pick a date" },
            ],
            [
                Label("DatePicker"), new DatePicker { Date = date, Header = "Date" },
                Label("TimePicker"), new TimePicker { Time = new TimeSpan(14, 30, 0), Header = "Time" },
            ],
        ];
    }

    static UIElement[][] Calendar() =>
    [
        [
            Label("CalendarView"),
            new CalendarView
            {
                Width = 300, Height = 340,
                MinDate = new DateTimeOffset(2024, 1, 1, 0, 0, 0, TimeSpan.Zero),
                MaxDate = new DateTimeOffset(2024, 12, 31, 0, 0, 0, TimeSpan.Zero),
                IsTodayHighlighted = false,
                DisplayMode = CalendarViewDisplayMode.Month,
            }.With(c =>
            {
                var day = new DateTimeOffset(2024, 3, 15, 0, 0, 0, TimeSpan.Zero);
                c.SetDisplayDate(day);
                c.SelectedDates.Add(day);
            }),
        ],
        [
            Label("ColorPicker"),
            new ColorPicker { Color = Color.FromArgb(255, 0x3A, 0x7B, 0xD5), IsAlphaEnabled = true, IsMoreButtonVisible = false },
        ],
    ];

    static UIElement[][] Collections()
    {
        var list = new ListView { Width = 200, Height = 180, ItemsSource = Words(12), SelectedIndex = 1, BorderThickness = new Thickness(1) };
        list.BorderBrush = ThemeBrush("ControlStrokeColorDefaultBrush");
        var grid = new GridView
        {
            Width = 260, Height = 200, ItemsSource = Words(9), SelectedIndex = 0,
            ItemTemplate = Template("<Border Width='70' Height='50' Background='{ThemeResource AccentFillColorDefaultBrush}'>" +
                "<TextBlock Text='{Binding}' Foreground='White' FontSize='11' VerticalAlignment='Center' HorizontalAlignment='Center'/></Border>"),
        };
        var tree = new TreeView { Width = 220, Height = 200 };
        var root = new TreeViewNode { Content = "Root", IsExpanded = true };
        for (int i = 1; i <= 3; i++)
        {
            var child = new TreeViewNode { Content = $"Folder {i}", IsExpanded = i == 1 };
            for (int j = 1; j <= 2; j++)
            {
                child.Children.Add(new TreeViewNode { Content = $"File {i}.{j}" });
            }
            root.Children.Add(child);
        }
        tree.RootNodes.Add(root);
        var repeater = new ItemsRepeater
        {
            ItemsSource = Words(6),
            Layout = new StackLayout { Spacing = 4 },
            ItemTemplate = Template("<Border Padding='6,2' Background='{ThemeResource CardBackgroundFillColorDefaultBrush}' " +
                "BorderBrush='{ThemeResource CardStrokeColorDefaultBrush}' BorderThickness='1' CornerRadius='4'><TextBlock Text='{Binding}'/></Border>"),
        };
        var itemsView = new ItemsView
        {
            Width = 220, Height = 160, ItemsSource = Words(8),
            ItemTemplate = Template("<ItemContainer><TextBlock Margin='8,4' Text='{Binding}'/></ItemContainer>"),
        };
        return [[Label("ListView"), list, Label("GridView"), grid], [Label("TreeView"), tree, Label("ItemsRepeater"), repeater], [Label("ItemsView"), itemsView]];
    }

    static UIElement[][] FlipAndPivot()
    {
        var flip = new FlipView { Width = 300, Height = 160 };
        foreach (var (name, color) in new[] { ("One", Colors.SteelBlue), ("Two", Colors.IndianRed), ("Three", Colors.SeaGreen) })
        {
            flip.Items.Add(new Border
            {
                Background = new SolidColorBrush(color),
                Child = new TextBlock { Text = name, FontSize = 28, Foreground = new SolidColorBrush(Colors.White),
                    HorizontalAlignment = HorizontalAlignment.Center, VerticalAlignment = VerticalAlignment.Center },
            });
        }
        var pivot = new Pivot { Width = 360, Height = 160 };
        foreach (var header in new[] { "All", "Unread", "Flagged" })
        {
            pivot.Items.Add(new PivotItem { Header = header, Content = new TextBlock { Text = $"{header} content" } });
        }
        return
        [
            [Label("FlipView"), flip, Label("PipsPager"), new PipsPager { NumberOfPages = 5, SelectedPageIndex = 2,
                PreviousButtonVisibility = PipsPagerButtonVisibility.Visible, NextButtonVisibility = PipsPagerButtonVisibility.Visible }],
            [Label("Pivot"), pivot],
        ];
    }

    static UIElement[][] Tabs()
    {
        var tabs = new TabView { Width = 560, Height = 200 };
        for (int i = 1; i <= 3; i++)
        {
            tabs.TabItems.Add(new TabViewItem { Header = $"Document {i}", IconSource = new SymbolIconSource { Symbol = Symbol.Document },
                Content = new TextBlock { Text = $"Tab {i} content", Margin = new Thickness(12) } });
        }
        tabs.SelectedIndex = 1;
        return [[Label("TabView"), tabs]];
    }

    static NavigationView Nav(NavigationViewPaneDisplayMode mode)
    {
        var nav = new NavigationView
        {
            PaneDisplayMode = mode, IsBackButtonVisible = NavigationViewBackButtonVisible.Collapsed,
            Width = 900, Height = 560, IsSettingsVisible = true, Header = "Header", IsPaneOpen = true,
            Content = new TextBlock { Text = "Content", Margin = new Thickness(12) },
        };
        foreach (var (text, symbol) in new[] { ("Home", Symbol.Home), ("Mail", Symbol.Mail), ("Calendar", Symbol.Calendar), ("People", Symbol.People) })
        {
            nav.MenuItems.Add(new NavigationViewItem { Content = text, Icon = new SymbolIcon(symbol) });
        }
        nav.SelectedItem = nav.MenuItems[1];
        return nav;
    }

    static UIElement[][] NavLeft() => [[Nav(NavigationViewPaneDisplayMode.Left)]];
    static UIElement[][] NavTop() => [[Nav(NavigationViewPaneDisplayMode.Top)]];
    static UIElement[][] NavMinimal() => [[Nav(NavigationViewPaneDisplayMode.LeftMinimal)]];

    static UIElement[][] Bars()
    {
        var menu = new MenuBar();
        foreach (var title in new[] { "File", "Edit", "View" })
        {
            var item = new MenuBarItem { Title = title };
            item.Items.Add(new MenuFlyoutItem { Text = "Open" });
            menu.Items.Add(item);
        }
        var commandBar = new CommandBar { IsOpen = false, DefaultLabelPosition = CommandBarDefaultLabelPosition.Right };
        commandBar.PrimaryCommands.Add(new AppBarButton { Icon = new SymbolIcon(Symbol.Add), Label = "Add" });
        commandBar.PrimaryCommands.Add(new AppBarButton { Icon = new SymbolIcon(Symbol.Edit), Label = "Edit" });
        commandBar.PrimaryCommands.Add(new AppBarSeparator());
        commandBar.PrimaryCommands.Add(new AppBarToggleButton { Icon = new SymbolIcon(Symbol.Favorite), Label = "Favorite", IsChecked = true });
        commandBar.SecondaryCommands.Add(new AppBarButton { Label = "Settings" });
        var crumbs = new BreadcrumbBar { ItemsSource = new[] { "Home", "Documents", "Design", "Specs" } };
        return [[Label("BreadcrumbBar"), crumbs, Label("MenuBar"), menu, Label("CommandBar"), commandBar]];
    }

    static UIElement[][] Status()
    {
        InfoBar Bar(InfoBarSeverity severity) => new()
        {
            Width = 420, IsOpen = true, Severity = severity, Title = severity.ToString(), Message = "A short message.",
        };
        var badgeHost = new Grid { Width = 40, Height = 32 };
        badgeHost.Children.Add(new SymbolIcon(Symbol.Mail));
        badgeHost.Children.Add(new InfoBadge { Value = 5, HorizontalAlignment = HorizontalAlignment.Right, VerticalAlignment = VerticalAlignment.Top });
        return
        [
            [Label("InfoBar"), Bar(InfoBarSeverity.Informational), Bar(InfoBarSeverity.Success), Bar(InfoBarSeverity.Warning), Bar(InfoBarSeverity.Error)],
            [
                Label("InfoBadge"),
                new StackPanel { Orientation = Orientation.Horizontal, Spacing = 12, Children = { badgeHost, new InfoBadge(), new InfoBadge { Value = 42 },
                    new InfoBadge { IconSource = new SymbolIconSource { Symbol = Symbol.Important } } } },
                Label("Expander"),
                new Expander { Header = "Collapsed", Content = "Hidden", Width = 300 },
                new Expander { Header = "Expanded", Content = new TextBlock { Text = "Expanded content" }, IsExpanded = true, Width = 300 },
            ],
        ];
    }

    static UIElement[][] Panes()
    {
        Border Fill(string text, string brush) => new()
        {
            Background = ThemeBrush(brush),
            Child = new TextBlock { Text = text, Margin = new Thickness(8) },
        };
        var split = new SplitView
        {
            Width = 420, Height = 200, IsPaneOpen = true, DisplayMode = SplitViewDisplayMode.Inline, OpenPaneLength = 140,
            Pane = Fill("Pane", "LayerFillColorDefaultBrush"), Content = Fill("SplitView content", "CardBackgroundFillColorDefaultBrush"),
        };
        var twoPane = new TwoPaneView
        {
            Width = 420, Height = 200, Pane1 = Fill("Pane 1", "LayerFillColorDefaultBrush"),
            Pane2 = Fill("Pane 2", "CardBackgroundFillColorDefaultBrush"), WideModeConfiguration = TwoPaneViewWideModeConfiguration.LeftRight,
            MinWideModeWidth = 300,
        };
        return [[Label("SplitView (inline, open)"), split, Label("TwoPaneView"), twoPane]];
    }

    static UIElement[][] Panels()
    {
        Rectangle Box(Color c, double w = 40, double h = 30) => new() { Width = w, Height = h, Fill = new SolidColorBrush(c) };
        var relative = new RelativePanel { Width = 200, Height = 100, Background = ThemeBrush("CardBackgroundFillColorDefaultBrush") };
        var a = Box(Colors.SteelBlue); var b = Box(Colors.IndianRed); var c = Box(Colors.SeaGreen);
        relative.Children.Add(a); relative.Children.Add(b); relative.Children.Add(c);
        RelativePanel.SetRightOf(b, a); RelativePanel.SetBelow(c, b); RelativePanel.SetAlignRightWithPanel(c, true);
        var stack = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 6, Children = { Box(Colors.Orange), Box(Colors.Purple), Box(Colors.Teal) } };
        var grid = new Grid { Width = 200, Height = 100, RowSpacing = 4, ColumnSpacing = 4 };
        grid.RowDefinitions.Add(new RowDefinition()); grid.RowDefinitions.Add(new RowDefinition());
        grid.ColumnDefinitions.Add(new ColumnDefinition()); grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(2, GridUnitType.Star) });
        for (int i = 0; i < 4; i++)
        {
            var cell = Box(Color.FromArgb(255, (byte)(60 * i), 120, (byte)(200 - 40 * i)), double.NaN, double.NaN);
            Grid.SetRow(cell, i / 2); Grid.SetColumn(cell, i % 2); grid.Children.Add(cell);
        }
        var canvas = new Canvas { Width = 200, Height = 100, Background = ThemeBrush("CardBackgroundFillColorDefaultBrush") };
        var e = new Ellipse { Width = 50, Height = 50, Fill = new SolidColorBrush(Colors.Goldenrod) };
        Canvas.SetLeft(e, 30); Canvas.SetTop(e, 20); canvas.Children.Add(e);
        var r = Box(Colors.SlateBlue, 60, 40); Canvas.SetLeft(r, 110); Canvas.SetTop(r, 40); Canvas.SetZIndex(r, -1); canvas.Children.Add(r);
        var wrap = new VariableSizedWrapGrid { ItemWidth = 40, ItemHeight = 30, MaximumRowsOrColumns = 4, Orientation = Orientation.Horizontal, Width = 200 };
        for (int i = 0; i < 7; i++)
        {
            var item = Box(Color.FromArgb(255, 30, (byte)(40 + 30 * i), 160), double.NaN, double.NaN);
            if (i == 0) { VariableSizedWrapGrid.SetColumnSpan(item, 2); }
            wrap.Children.Add(item);
        }
        return
        [
            [Label("RelativePanel"), relative, Label("StackPanel"), stack, Label("Grid"), grid],
            [Label("Canvas"), canvas, Label("VariableSizedWrapGrid"), wrap],
        ];
    }

    static UIElement[][] Content()
    {
        var viewbox = new Viewbox { Width = 160, Height = 80, Child = new TextBlock { Text = "Viewbox" } };
        var icon = new AnimatedIcon
        {
            Width = 32, Height = 32,
            Source = new Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedSettingsVisualSource(),
            FallbackIconSource = new SymbolIconSource { Symbol = Symbol.Setting },
        };
        return
        [
            [
                Label("Viewbox"), viewbox,
                Label("PersonPicture"),
                new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8, Children =
                    { new PersonPicture { DisplayName = "Ada Lovelace", Width = 64, Height = 64 }, new PersonPicture { Width = 64, Height = 64 },
                      new PersonPicture { IsGroup = true, Width = 64, Height = 64 }, new PersonPicture { Initials = "GH", BadgeNumber = 3, Width = 64, Height = 64 } } },
                Label("AnimatedIcon (idle)"), icon,
            ],
            [
                Label("Border / ContentControl"),
                new Border { Width = 220, Padding = new Thickness(12), CornerRadius = new CornerRadius(8), BorderThickness = new Thickness(2),
                    BorderBrush = ThemeBrush("AccentFillColorDefaultBrush"), Background = ThemeBrush("CardBackgroundFillColorDefaultBrush"),
                    Child = new ContentControl { Content = "Content in a Border" } },
                new ContentPresenter { Content = "ContentPresenter", Padding = new Thickness(8), BorderThickness = new Thickness(1),
                    BorderBrush = ThemeBrush("ControlStrokeColorDefaultBrush"), CornerRadius = new CornerRadius(4) },
                Label("Icons"),
                new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8, Children =
                    { new SymbolIcon(Symbol.Globe), new FontIcon { Glyph = "\uE734" },
                      new PathIcon { Data = (Geometry)Microsoft.UI.Xaml.Markup.XamlBindingHelper.ConvertValue(typeof(Geometry), "M0,0 L16,8 L0,16 Z") } } },
            ],
        ];
    }

    static UIElement[][] TextIdle() =>
    [
        [
            Label("TextBox"), new TextBox { Width = 240, Header = "Header", Text = "Some text" },
            new TextBox { Width = 240, PlaceholderText = "Placeholder" },
            Label("PasswordBox"), new PasswordBox { Width = 240, Password = "secret" },
            Label("NumberBox"), new NumberBox { Width = 240, Value = 42, SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Inline },
        ],
        [
            Label("AutoSuggestBox"), new AutoSuggestBox { Width = 240, PlaceholderText = "Search", QueryIcon = new SymbolIcon(Symbol.Find) },
            Label("RichEditBox"), new RichEditBox { Width = 240, Height = 80 },
            Label("RichTextBlock"), new RichTextBlock { Width = 240, Blocks = { new Microsoft.UI.Xaml.Documents.Paragraph {
                Inlines = { new Microsoft.UI.Xaml.Documents.Run { Text = "Rich " }, new Microsoft.UI.Xaml.Documents.Bold { Inlines = { new Microsoft.UI.Xaml.Documents.Run { Text = "text" } } } } } } },
        ],
    ];

    static UIElement[][] Indeterminate() =>
    [
        [
            Label("ProgressBar (indeterminate)"), new ProgressBar { Width = 300, IsIndeterminate = true },
            Label("ProgressRing (indeterminate)"), new ProgressRing { IsActive = true, Width = 48, Height = 48 },
        ],
    ];

    // ---- pages ----

    [CoveragePage(F, "buttons-light")] public static CoveragePage ButtonsLight(CoverageContext c) => Light(Buttons);
    [CoveragePage(F, "buttons-dark")] public static CoveragePage ButtonsDark(CoverageContext c) => Dark(Buttons);
    [CoveragePage(F, "buttons-disabled")] public static CoveragePage ButtonsDisabled(CoverageContext c) => Disabled(Buttons);
    [CoveragePage(F, "toggles-light")] public static CoveragePage TogglesLight(CoverageContext c) => Light(Toggles);
    [CoveragePage(F, "toggles-dark")] public static CoveragePage TogglesDark(CoverageContext c) => Dark(Toggles);
    [CoveragePage(F, "toggles-disabled")] public static CoveragePage TogglesDisabled(CoverageContext c) => Disabled(Toggles);
    [CoveragePage(F, "range-light")] public static CoveragePage RangeLight(CoverageContext c) => Light(Range);
    [CoveragePage(F, "range-dark")] public static CoveragePage RangeDark(CoverageContext c) => Dark(Range);
    [CoveragePage(F, "range-disabled")] public static CoveragePage RangeDisabled(CoverageContext c) => Disabled(Range);
    [CoveragePage(F, "pickers-light")] public static CoveragePage PickersLight(CoverageContext c) => Light(Pickers);
    [CoveragePage(F, "pickers-dark")] public static CoveragePage PickersDark(CoverageContext c) => Dark(Pickers);
    [CoveragePage(F, "pickers-disabled")] public static CoveragePage PickersDisabled(CoverageContext c) => Disabled(Pickers);
    [CoveragePage(F, "calendar-colorpicker-light")] public static CoveragePage CalendarLight(CoverageContext c) => Light(Calendar);
    [CoveragePage(F, "calendar-colorpicker-dark")] public static CoveragePage CalendarDark(CoverageContext c) => Dark(Calendar);
    [CoveragePage(F, "calendar-colorpicker-disabled")] public static CoveragePage CalendarDisabled(CoverageContext c) => Disabled(Calendar);
    [CoveragePage(F, "collections-light")] public static CoveragePage CollectionsLight(CoverageContext c) => Light(Collections);
    [CoveragePage(F, "collections-dark")] public static CoveragePage CollectionsDark(CoverageContext c) => Dark(Collections);
    [CoveragePage(F, "collections-disabled")] public static CoveragePage CollectionsDisabled(CoverageContext c) => Disabled(Collections);
    [CoveragePage(F, "flipview-pivot-light")] public static CoveragePage FlipLight(CoverageContext c) => Light(FlipAndPivot);
    [CoveragePage(F, "flipview-pivot-dark")] public static CoveragePage FlipDark(CoverageContext c) => Dark(FlipAndPivot);
    [CoveragePage(F, "flipview-pivot-disabled")] public static CoveragePage FlipDisabled(CoverageContext c) => Disabled(FlipAndPivot);
    [CoveragePage(F, "tabview-light")] public static CoveragePage TabsLight(CoverageContext c) => Light(Tabs);
    [CoveragePage(F, "tabview-dark")] public static CoveragePage TabsDark(CoverageContext c) => Dark(Tabs);
    [CoveragePage(F, "tabview-disabled")] public static CoveragePage TabsDisabled(CoverageContext c) => Disabled(Tabs);
    [CoveragePage(F, "navigationview-left-light")] public static CoveragePage NavLeftLight(CoverageContext c) => Light(NavLeft);
    [CoveragePage(F, "navigationview-left-dark")] public static CoveragePage NavLeftDark(CoverageContext c) => Dark(NavLeft);
    [CoveragePage(F, "navigationview-left-disabled")] public static CoveragePage NavLeftDisabled(CoverageContext c) => Disabled(NavLeft);
    [CoveragePage(F, "navigationview-top-light")] public static CoveragePage NavTopLight(CoverageContext c) => Light(NavTop);
    [CoveragePage(F, "navigationview-top-dark")] public static CoveragePage NavTopDark(CoverageContext c) => Dark(NavTop);
    [CoveragePage(F, "navigationview-minimal-light")] public static CoveragePage NavMinimalLight(CoverageContext c) => Light(NavMinimal);
    [CoveragePage(F, "navigationview-minimal-dark")] public static CoveragePage NavMinimalDark(CoverageContext c) => Dark(NavMinimal);
    [CoveragePage(F, "bars-light")] public static CoveragePage BarsLight(CoverageContext c) => Light(Bars);
    [CoveragePage(F, "bars-dark")] public static CoveragePage BarsDark(CoverageContext c) => Dark(Bars);
    [CoveragePage(F, "bars-disabled")] public static CoveragePage BarsDisabled(CoverageContext c) => Disabled(Bars);
    [CoveragePage(F, "status-light")] public static CoveragePage StatusLight(CoverageContext c) => Light(Status);
    [CoveragePage(F, "status-dark")] public static CoveragePage StatusDark(CoverageContext c) => Dark(Status);
    [CoveragePage(F, "status-disabled")] public static CoveragePage StatusDisabled(CoverageContext c) => Disabled(Status);
    [CoveragePage(F, "panes-light")] public static CoveragePage PanesLight(CoverageContext c) => Light(Panes);
    [CoveragePage(F, "panes-dark")] public static CoveragePage PanesDark(CoverageContext c) => Dark(Panes);
    [CoveragePage(F, "panels-light")] public static CoveragePage PanelsLight(CoverageContext c) => Light(Panels);
    [CoveragePage(F, "panels-dark")] public static CoveragePage PanelsDark(CoverageContext c) => Dark(Panels);
    [CoveragePage(F, "content-light")] public static CoveragePage ContentLight(CoverageContext c) => Light(Content);
    [CoveragePage(F, "content-dark")] public static CoveragePage ContentDark(CoverageContext c) => Dark(Content);
    [CoveragePage(F, "content-disabled")] public static CoveragePage ContentDisabled(CoverageContext c) => Disabled(Content);
    [CoveragePage(F, "text-idle-light")] public static CoveragePage TextLight(CoverageContext c) => Light(TextIdle);
    [CoveragePage(F, "text-idle-dark")] public static CoveragePage TextDark(CoverageContext c) => Dark(TextIdle);
    [CoveragePage(F, "text-idle-disabled")] public static CoveragePage TextDisabled(CoverageContext c) => Disabled(TextIdle);
    // Endless animations: the capture lands on an arbitrary animation phase, so only a loose threshold is meaningful.
    [CoveragePage(F, "progress-indeterminate", MaxMeanAbsDiff = 0.03)] public static CoveragePage IndeterminateLight(CoverageContext c) => Light(Indeterminate);
}

static class CoverageExtensions
{
    public static T With<T>(this T value, Action<T> configure)
    {
        configure(value);
        return value;
    }
}
