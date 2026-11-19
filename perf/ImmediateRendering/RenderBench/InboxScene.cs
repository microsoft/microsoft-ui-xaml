using Microsoft.UI.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;
using Windows.UI;

namespace RenderBench;

// S3: inbox. A virtualized ListView of seeded mail items (avatar ellipse + initials, sender, trimmed subject,
// timestamp). Interaction scrolls continuously: programmatic ChangeView once per frame at ScrollSpeed DIPs/s, or
// mouse-wheel notches at InputHz in SendInput mode; both reverse at the ends. Container content is filled in
// ContainerContentChanging (no bindings), as list-heavy apps do.
sealed class InboxScene : IScene
{
    sealed record Mail(string Sender, string Initials, string Subject, string Time, Color Avatar);

    const string Template = """
        <DataTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
          <Grid Height="60" ColumnSpacing="12" Padding="0,6">
            <Grid.ColumnDefinitions><ColumnDefinition Width="40"/><ColumnDefinition Width="*"/><ColumnDefinition Width="Auto"/></Grid.ColumnDefinitions>
            <Grid.RowDefinitions><RowDefinition/><RowDefinition/></Grid.RowDefinitions>
            <Ellipse Grid.RowSpan="2" Width="40" Height="40" VerticalAlignment="Center"/>
            <TextBlock Grid.RowSpan="2" HorizontalAlignment="Center" VerticalAlignment="Center" Foreground="White" FontSize="14"/>
            <TextBlock Grid.Column="1" FontWeight="SemiBold" FontSize="14"/>
            <TextBlock Grid.Column="1" Grid.Row="1" FontSize="13" TextTrimming="CharacterEllipsis" Opacity="0.8"/>
            <TextBlock Grid.Column="2" FontSize="12" Opacity="0.7" HorizontalAlignment="Right"/>
          </Grid>
        </DataTemplate>
        """;

    static readonly string[] s_first = ["Ada", "Ben", "Chloe", "Dmitri", "Elena", "Farid", "Grace", "Hiro", "Ines", "Jonas", "Kemal", "Lena", "Mateo", "Nora", "Omar", "Priya"];
    static readonly string[] s_last = ["Ito", "Novak", "Okafor", "Larsen", "Silva", "Kowalski", "Haddad", "Meyer", "Tanaka", "Moreau", "Rossi", "Nguyen"];
    static readonly string[] s_words = ["quarterly", "review", "draft", "budget", "meeting", "notes", "launch", "plan", "follow-up", "design", "update", "invoice", "travel", "offsite", "proposal", "feedback", "release", "schedule", "metrics", "agenda"];

    readonly List<Mail> _mails = [];
    readonly ListView _list;
    readonly Dictionary<Color, SolidColorBrush> _brushes = [];
    readonly double _speed;
    ScrollViewer? _viewer;
    ScrollDriver? _driver;

    public string Name => "Inbox";
    public bool IsDynamic => false;
    public bool ContinuousInteraction => true;
    public FrameworkElement Root { get; }
    public IReadOnlyList<FrameworkElement> InputTargets => [];

    public InboxScene(BenchOptions options)
    {
        _speed = options.ScrollSpeed;
        var random = new Random(options.Seed);
        Color[] palette = [Color.FromArgb(255, 0x00, 0x67, 0xC0), Color.FromArgb(255, 0xC2, 0x39, 0xB3), Color.FromArgb(255, 0x10, 0x7C, 0x10),
            Color.FromArgb(255, 0xCA, 0x50, 0x10), Color.FromArgb(255, 0x88, 0x17, 0x98), Color.FromArgb(255, 0x00, 0x78, 0x7C)];
        var start = new DateTime(2024, 6, 1, 18, 0, 0);
        for (int i = 0; i < options.Items; i++)
        {
            string first = s_first[random.Next(s_first.Length)], last = s_last[random.Next(s_last.Length)];
            int words = 4 + random.Next(10);
            string subject = string.Join(' ', Enumerable.Range(0, words).Select(_ => s_words[random.Next(s_words.Length)]));
            var time = start.AddMinutes(-i * 17 - random.Next(17));
            _mails.Add(new Mail($"{first} {last}", $"{first[0]}{last[0]}", char.ToUpperInvariant(subject[0]) + subject[1..],
                time.ToString(i < 60 ? "HH:mm" : "MMM d", System.Globalization.CultureInfo.InvariantCulture), palette[random.Next(palette.Length)]));
        }

        _list = new ListView
        {
            ItemsSource = _mails,
            ItemTemplate = (DataTemplate)XamlReader.Load(Template),
            SelectionMode = ListViewSelectionMode.Single,
        };
        _list.ContainerContentChanging += OnContainerContentChanging;

        var header = new TextBlock { Text = "Inbox", FontSize = 20, FontWeight = FontWeights.SemiBold, Margin = new Thickness(16, 12, 16, 8) };
        var root = new Grid { Background = new SolidColorBrush(Color.FromArgb(255, 0xFA, 0xFA, 0xFA)) };
        root.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        root.RowDefinitions.Add(new RowDefinition());
        Grid.SetRow(_list, 1);
        root.Children.Add(header);
        root.Children.Add(_list);
        Root = root;
    }

    void OnContainerContentChanging(ListViewBase sender, ContainerContentChangingEventArgs args)
    {
        if (args.InRecycleQueue || args.ItemContainer.ContentTemplateRoot is not Grid grid)
        {
            return;
        }
        var mail = (Mail)args.Item;
        if (!_brushes.TryGetValue(mail.Avatar, out var brush))
        {
            _brushes[mail.Avatar] = brush = new SolidColorBrush(mail.Avatar);
        }
        ((Ellipse)grid.Children[0]).Fill = brush;
        ((TextBlock)grid.Children[1]).Text = mail.Initials;
        ((TextBlock)grid.Children[2]).Text = mail.Sender;
        ((TextBlock)grid.Children[3]).Text = mail.Subject;
        ((TextBlock)grid.Children[4]).Text = mail.Time;
        args.Handled = true;
    }

    public int ApplyUpdate() => 0;

    public void Attach(ISceneHost host)
    {
        _viewer = Tree.Find<ScrollViewer>(_list);
        if (_viewer is not null)
        {
            _driver = new ScrollDriver(_viewer, _speed);
        }
    }

    // Warmup scrolls a little each way so templates, glyphs and brushes for scrolled-in items exist.
    public Task? WarmupAsync(ISceneHost host) => _viewer is null ? null : WarmupScrollAsync(_viewer);

    static async Task WarmupScrollAsync(ScrollViewer viewer)
    {
        viewer.ChangeView(null, 2000, null, disableAnimation: true);
        await Task.Delay(300);
        viewer.ChangeView(null, 0, null, disableAnimation: true);
        await Task.Delay(300);
    }

    public Task? RunInteractionAsync(ISceneHost host, CancellationToken token)
    {
        if (_driver is null)
        {
            return null;
        }
        if (host.InputMode == InputMode.SendInput)
        {
            return host.Every(Math.Max(1, host.Options.InputHz), () => _driver.Wheel(host, _list), token);
        }
        return host.EveryFrame(_driver.Advance, token);
    }

    public IEnumerable<(string Name, double Value)> Counters()
    {
        if (_list.ItemsPanelRoot is { } panel)
        {
            yield return ("RealizedItems", panel.Children.Count);
        }
    }
}
