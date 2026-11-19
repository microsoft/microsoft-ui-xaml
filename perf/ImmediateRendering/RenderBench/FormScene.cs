using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Windows.UI;

namespace RenderBench;

// S10: form. Ten focusable controls (3 TextBoxes, ComboBox, CheckBox, ToggleSwitch, Slider, Button+Flyout,
// Button+ToolTip, Submit). Interaction steps through a fixed script at InputHz: typing (one character per step; the
// caret is visible), Tab focus moves (keyboard focus visuals), hover on/off, ComboBox open/close, Flyout open/close
// and ToolTip open/close. SendInput mode types with real keystrokes ("key", measured by R1), tabs with VK_TAB, hovers
// with mouse moves and opens popups with clicks / closes with Escape; programmatic mode drives the same changes via
// the XAML API. Popup opens emit PopupOpened with the triggering input id (popup open time).
sealed class FormScene : IScene
{
    enum Step { Type, Tab, Hover, Unhover, OpenCombo, CloseCombo, OpenFlyout, CloseFlyout, ShowTip, HideTip }

    static readonly Step[] s_script =
    [
        Step.Type, Step.Type, Step.Type, Step.Type, Step.Type, Step.Type, Step.Tab, Step.Hover, Step.Unhover,
        Step.OpenCombo, Step.CloseCombo, Step.Type, Step.Type, Step.Type, Step.Type, Step.Tab, Step.OpenFlyout,
        Step.CloseFlyout, Step.ShowTip, Step.HideTip, Step.Tab,
    ];
    const string Alphabet = "the quick brown fox jumps over the lazy dog 0123456789 ";

    readonly List<Control> _controls = [];
    readonly TextBox[] _textBoxes;
    readonly ComboBox _combo;
    readonly Button _flyoutButton, _tipButton, _submit;
    readonly Flyout _flyout;
    readonly ToolTip _tip;
    readonly Random _random;
    readonly List<FrameworkElement> _targets = [];
    ISceneHost? _host;
    int _position, _focus, _typedBox;
    long _pendingPopup;

    public string Name => "Form";
    public bool IsDynamic => false;
    public FrameworkElement Root { get; }
    public IReadOnlyList<FrameworkElement> InputTargets => _targets;

    public FormScene(BenchOptions options)
    {
        _random = new Random(options.Seed);
        var panel = new StackPanel { Spacing = 12, Padding = new Thickness(32), MaxWidth = 560, HorizontalAlignment = HorizontalAlignment.Left };
        panel.Children.Add(new TextBlock { Text = "Contact details", Style = SettingsScene.Style("TitleTextBlockStyle") });

        _textBoxes = [new TextBox { Header = "Name" }, new TextBox { Header = "Email" }, new TextBox { Header = "Notes", AcceptsReturn = false }];
        _combo = new ComboBox { Header = "Country", Width = 240 };
        foreach (string country in new[] { "Canada", "Denmark", "Japan", "Kenya", "Mexico", "Norway", "Portugal", "Vietnam" })
        {
            _combo.Items.Add(country);
        }
        _combo.SelectedIndex = 0;
        var check = new CheckBox { Content = "Subscribe to updates" };
        var toggle = new ToggleSwitch { Header = "Notifications" };
        var slider = new Slider { Header = "Priority", Minimum = 0, Maximum = 10, Value = 5, Width = 240, HorizontalAlignment = HorizontalAlignment.Left };

        _flyout = new Flyout { Content = new TextBlock { Text = "Fields marked * are required.", Width = 220, TextWrapping = TextWrapping.Wrap } };
        _flyoutButton = new Button { Content = "Help", Flyout = _flyout };
        _tip = new ToolTip { Content = "Clears every field" };
        _tipButton = new Button { Content = "Reset" };
        ToolTipService.SetToolTip(_tipButton, _tip);
        _submit = new Button { Content = "Submit", Style = SettingsScene.Style("AccentButtonStyle") };

        _controls.AddRange(_textBoxes);
        _controls.AddRange([_combo, check, toggle, slider, _flyoutButton, _tipButton, _submit]);
        foreach (var textBox in _textBoxes)
        {
            panel.Children.Add(textBox);
        }
        panel.Children.Add(_combo);
        panel.Children.Add(check);
        panel.Children.Add(toggle);
        panel.Children.Add(slider);
        panel.Children.Add(new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8, Children = { _flyoutButton, _tipButton, _submit } });
        _targets.AddRange([check, toggle]);

        _combo.DropDownOpened += (_, _) => PopupOpened("ComboBox");
        _flyout.Opened += (_, _) => PopupOpened("Flyout");
        _tip.Opened += (_, _) => PopupOpened("ToolTip");

        Root = new Grid { Background = new SolidColorBrush(Color.FromArgb(255, 0xFA, 0xFA, 0xFA)), Children = { new ScrollViewer { Content = panel } } };
    }

    void PopupOpened(string popup)
    {
        if (_pendingPopup != 0)
        {
            BenchEventSource.Log.PopupOpened(_pendingPopup, popup);
            _pendingPopup = 0;
        }
    }

    public int ApplyUpdate() => 0;

    public void Attach(ISceneHost host) => _host = host;

    public Task? WarmupAsync(ISceneHost host) => WarmupAsync();

    async Task WarmupAsync()
    {
        foreach (var step in s_script)
        {
            Run(step, warmup: true);
            await Task.Delay(60);
        }
        Park();
    }

    // Steady and Recovery start from a parked form: popups closed, programmatic focus (no caret, no focus visual).
    public bool OnPhase(BenchPhase phase)
    {
        if (phase is BenchPhase.Steady or BenchPhase.Recovery)
        {
            Park();
            return true;
        }
        return false;
    }

    void Park()
    {
        _combo.IsDropDownOpen = false;
        _flyout.Hide();
        _tip.IsOpen = false;
        VisualStateManager.GoToState(_submit, "Normal", false);
        _submit.Focus(FocusState.Programmatic);
    }

    public Task? RunInteractionAsync(ISceneHost host, CancellationToken token) =>
        host.Every(Math.Max(0.5, host.Options.InputHz), () => Run(s_script[_position++ % s_script.Length], warmup: false), token);

    void Run(Step step, bool warmup)
    {
        var host = _host!;
        bool real = !warmup && host.InputMode == InputMode.SendInput;
        long Mark(string kind, string target) => warmup ? 0 : host.MarkInput(kind, target, programmatic: !real);
        switch (step)
        {
            case Step.Type:
            {
                if (_controls[_focus] is not TextBox box)
                {
                    box = _textBoxes[_typedBox++ % _textBoxes.Length];
                    _focus = _controls.IndexOf(box);
                }
                if (box.FocusState == FocusState.Unfocused)
                {
                    box.Focus(FocusState.Keyboard);
                }
                if (box.Text.Length > 48)
                {
                    box.Text = "";
                }
                char ch = Alphabet[_random.Next(Alphabet.Length)];
                if (real)
                {
                    Mark("key", "TextBox");
                    Win32.TypeChar(ch);
                }
                else
                {
                    Mark("type", "TextBox");
                    box.SelectionStart = box.Text.Length;
                    box.SelectedText = ch.ToString();
                    box.SelectionStart = box.Text.Length;
                }
                break;
            }
            case Step.Tab:
                // Walks the ten controls in tab order, wrapping; a Type step on a non-TextBox moves to the next box.
                _focus = (_focus + 1) % _controls.Count;
                Mark("tab", _controls[_focus].GetType().Name);
                if (real)
                {
                    Win32.Key(Win32.VK_TAB);
                }
                else
                {
                    _controls[_focus].Focus(FocusState.Keyboard);
                }
                break;
            case Step.Hover:
                Mark("hover", "Button");
                if (real)
                {
                    var (x, y) = host.ScreenPoint(_submit);
                    Win32.MouseMove(x, y);
                }
                else
                {
                    VisualStateManager.GoToState(_submit, "PointerOver", true);
                }
                break;
            case Step.Unhover:
                Mark("unhover", "Button");
                if (real)
                {
                    var (x, y) = host.ScreenPoint((FrameworkElement)Root, 0.95, 0.95);
                    Win32.MouseMove(x, y);
                }
                else
                {
                    VisualStateManager.GoToState(_submit, "Normal", true);
                }
                break;
            case Step.OpenCombo:
                _pendingPopup = Mark("open", "ComboBox");
                if (real)
                {
                    var (x, y) = host.ScreenPoint(_combo);
                    Win32.Click(x, y);
                }
                else
                {
                    _combo.IsDropDownOpen = true;
                }
                break;
            case Step.CloseCombo:
                Close("ComboBox", () => _combo.IsDropDownOpen = false, real);
                break;
            case Step.OpenFlyout:
                _pendingPopup = Mark("open", "Flyout");
                if (real)
                {
                    var (x, y) = host.ScreenPoint(_flyoutButton);
                    Win32.Click(x, y);
                }
                else
                {
                    _flyout.ShowAt(_flyoutButton);
                }
                break;
            case Step.CloseFlyout:
                Close("Flyout", _flyout.Hide, real);
                break;
            case Step.ShowTip:
                // ToolTips open on a hover delay; open it directly in both modes so the open time is the popup's own.
                _pendingPopup = warmup ? 0 : host.MarkInput("open", "ToolTip", programmatic: true);
                _tip.IsOpen = true;
                break;
            case Step.HideTip:
                if (!warmup)
                {
                    host.MarkInput("close", "ToolTip", programmatic: true);
                }
                _tip.IsOpen = false;
                break;
        }

        void Close(string target, Action close, bool sendInput)
        {
            Mark("close", target);
            if (sendInput)
            {
                Win32.Key(Win32.VK_ESCAPE);
            }
            else
            {
                close();
            }
        }
    }

    public IEnumerable<(string Name, double Value)> Counters()
    {
        yield return ("TypedChars", _textBoxes.Sum(t => t.Text.Length));
    }
}
