using System.Globalization;
using System.Text;
using Microsoft.UI.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Windows.UI;

namespace RenderBench;

// S8: data grid. An ItemsRepeater of Items rows (each a 50-cell row of bordered TextBlocks) inside a ScrollViewer that
// scrolls both ways, with a frozen header row in a separate ScrollViewer that follows horizontally only. Updates
// rewrite 2% of the visible cells at UpdateHz; input scrolls vertically/horizontally in turn at InputHz (animated
// ChangeView, or wheel notches in SendInput mode).
sealed class DataGridScene : IScene
{
    public const int Columns = 50;
    const double CellWidth = 96, CellHeight = 28;

    readonly double[,] _values;
    readonly ItemsRepeater _repeater;
    readonly ScrollViewer _viewer, _header;
    readonly Dictionary<int, StackPanel> _rows = [];
    readonly Random _random;
    int _scrolls;

    public string Name => "DataGrid";
    public bool IsDynamic => true;
    public FrameworkElement Root { get; }
    public IReadOnlyList<FrameworkElement> InputTargets => [];

    public DataGridScene(BenchOptions options)
    {
        int rows = Math.Max(1, options.Items);
        _random = new Random(options.Seed);
        var data = new Random(options.Seed);
        _values = new double[rows, Columns];
        for (int r = 0; r < rows; r++)
        {
            for (int c = 0; c < Columns; c++)
            {
                _values[r, c] = Math.Round(data.NextDouble() * 10000, 2);
            }
        }

        _repeater = new ItemsRepeater
        {
            ItemsSource = Enumerable.Range(0, rows).ToList(),
            ItemTemplate = (DataTemplate)XamlReader.Load(RowTemplate()),
        };
        _repeater.ElementPrepared += (_, args) => Fill((StackPanel)args.Element, args.Index);
        _repeater.ElementClearing += (_, args) =>
        {
            foreach (var (index, row) in _rows)
            {
                if (ReferenceEquals(row, args.Element))
                {
                    _rows.Remove(index);
                    break;
                }
            }
        };

        _viewer = new ScrollViewer
        {
            Content = _repeater,
            HorizontalScrollMode = ScrollMode.Enabled,
            HorizontalScrollBarVisibility = ScrollBarVisibility.Auto,
            VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
        };
        var headerRow = new StackPanel { Orientation = Orientation.Horizontal, Background = new SolidColorBrush(Color.FromArgb(255, 0xEE, 0xEE, 0xF2)) };
        var line = new SolidColorBrush(Color.FromArgb(255, 0xC8, 0xC8, 0xD0));
        for (int c = 0; c < Columns; c++)
        {
            headerRow.Children.Add(new Border
            {
                Width = CellWidth,
                Height = CellHeight + 4,
                BorderBrush = line,
                BorderThickness = new Thickness(0, 0, 1, 1),
                Child = new TextBlock { Text = ColumnName(c), FontWeight = FontWeights.SemiBold, Margin = new Thickness(6, 0, 6, 0),
                    VerticalAlignment = VerticalAlignment.Center },
            });
        }
        // Frozen header: horizontally scrollable only through code, never vertically.
        _header = new ScrollViewer
        {
            Content = headerRow,
            HorizontalScrollMode = ScrollMode.Disabled,
            HorizontalScrollBarVisibility = ScrollBarVisibility.Hidden,
            VerticalScrollMode = ScrollMode.Disabled,
            VerticalScrollBarVisibility = ScrollBarVisibility.Disabled,
        };
        _viewer.ViewChanging += (_, e) => _header.ChangeView(e.NextView.HorizontalOffset, null, null, disableAnimation: true);
        _viewer.ViewChanged += (_, _) => _header.ChangeView(_viewer.HorizontalOffset, null, null, disableAnimation: true);

        var root = new Grid { Background = new SolidColorBrush(Colors()) };
        root.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        root.RowDefinitions.Add(new RowDefinition());
        Grid.SetRow(_viewer, 1);
        root.Children.Add(_header);
        root.Children.Add(_viewer);
        Root = root;
    }

    static Color Colors() => Color.FromArgb(255, 0xFF, 0xFF, 0xFF);

    static string ColumnName(int c) => c < 26 ? ((char)('A' + c)).ToString() : "A" + (char)('A' + c - 26);

    static string RowTemplate()
    {
        var cells = new StringBuilder();
        for (int c = 0; c < Columns; c++)
        {
            cells.Append(string.Create(CultureInfo.InvariantCulture,
                $"<Border Width=\"{CellWidth}\" Height=\"{CellHeight}\" BorderBrush=\"#FFE0E0E6\" BorderThickness=\"0,0,1,1\">" +
                $"<TextBlock Margin=\"6,0\" VerticalAlignment=\"Center\" HorizontalAlignment=\"Right\" FontSize=\"13\"/></Border>"));
        }
        return "<DataTemplate xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\">" +
            $"<StackPanel Orientation=\"Horizontal\">{cells}</StackPanel></DataTemplate>";
    }

    static TextBlock Cell(StackPanel row, int column) => (TextBlock)((Border)row.Children[column]).Child;

    void Fill(StackPanel row, int index)
    {
        _rows[index] = row;
        for (int c = 0; c < Columns; c++)
        {
            Cell(row, c).Text = Format(_values[index, c]);
        }
    }

    static string Format(double value) => value.ToString("N2", CultureInfo.InvariantCulture);

    // Rewrites 2% of the cells currently in the viewport (realized rows x visible columns).
    public int ApplyUpdate()
    {
        int firstColumn = Math.Clamp((int)(_viewer.HorizontalOffset / CellWidth), 0, Columns - 1);
        int lastColumn = Math.Clamp((int)((_viewer.HorizontalOffset + _viewer.ViewportWidth) / CellWidth), firstColumn, Columns - 1);
        int firstRow = (int)(_viewer.VerticalOffset / CellHeight);
        int lastRow = (int)((_viewer.VerticalOffset + _viewer.ViewportHeight) / CellHeight);
        var visibleRows = _rows.Keys.Where(r => r >= firstRow && r <= lastRow).Order().ToList();
        if (visibleRows.Count == 0)
        {
            return 0;
        }
        int columns = lastColumn - firstColumn + 1;
        int count = Math.Max(1, (int)Math.Round(visibleRows.Count * columns * 0.02));
        for (int i = 0; i < count; i++)
        {
            int r = visibleRows[_random.Next(visibleRows.Count)];
            int c = firstColumn + _random.Next(columns);
            _values[r, c] = Math.Round(_values[r, c] + (_random.NextDouble() - 0.5) * 100, 2);
            Cell(_rows[r], c).Text = Format(_values[r, c]);
        }
        return count;
    }

    public Task? WarmupAsync(ISceneHost host) => WarmupAsync();

    async Task WarmupAsync()
    {
        _viewer.ChangeView(_viewer.ScrollableWidth, _viewer.ScrollableHeight / 4, null, disableAnimation: true);
        await Task.Delay(300);
        _viewer.ChangeView(0, 0, null, disableAnimation: true);
        await Task.Delay(300);
        for (int i = 0; i < 5; i++)
        {
            ApplyUpdate();
            await Task.Delay(50);
        }
    }

    public Task? RunInteractionAsync(ISceneHost host, CancellationToken token)
    {
        if (host.Options.InputHz <= 0)
        {
            return null;
        }
        var vertical = new ScrollDriver(_viewer, 0);
        var horizontal = new ScrollDriver(_viewer, 0, horizontal: true);
        return host.Every(host.Options.InputHz, () => Scroll(host, vertical, horizontal), token);
    }

    // Alternates vertical and horizontal scrolls; programmatic mode uses an animated ChangeView (the same path as a
    // scroll-bar or keyboard page scroll), SendInput mode a burst of wheel notches.
    void Scroll(ISceneHost host, ScrollDriver vertical, ScrollDriver horizontal)
    {
        bool isHorizontal = _scrolls++ % 2 == 1;
        if (host.InputMode == InputMode.SendInput)
        {
            for (int i = 0; i < 3; i++)
            {
                (isHorizontal ? horizontal : vertical).Wheel(host, _viewer);
            }
            return;
        }
        host.MarkInput("scroll", "ScrollViewer");
        if (isHorizontal)
        {
            _viewer.ChangeView(_random.NextDouble() * _viewer.ScrollableWidth, null, null, disableAnimation: false);
        }
        else
        {
            _viewer.ChangeView(null, _random.NextDouble() * _viewer.ScrollableHeight, null, disableAnimation: false);
        }
    }

    public IEnumerable<(string Name, double Value)> Counters()
    {
        yield return ("RealizedRows", _rows.Count);
        yield return ("RealizedCells", _rows.Count * Columns);
    }
}
