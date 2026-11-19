using System.Collections.Generic;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;

namespace TemplateDeferralBench
{
    public static class SmallControlScenarios
    {
        public static IEnumerable<Scenario> All()
        {
            // CheckGlyph (AnimatedIcon) is only needed once the box is checked/indeterminate or hovered/pressed.
            yield return new Scenario
            {
                Name = "CheckBox",
                Build = count => Bench.Rows(count, i => new CheckBox { Content = $"Item {i}", Width = 120 }),
                DeferredPartNames = new[] { "CheckGlyph" },
                Trigger = root =>
                {
                    foreach (var checkBox in Bench.All<CheckBox>(root).ToList()) checkBox.IsChecked = true;
                },
                Probe = root =>
                {
                    var boxes = Bench.All<CheckBox>(root).ToList();
                    var glyphs = boxes.Select(b => Bench.Named(b, "CheckGlyph")).ToList();
                    return new Dictionary<string, object>
                    {
                        ["AllChecked"] = boxes.All(b => b.IsChecked == true),
                        ["GlyphRealizedForAll"] = glyphs.All(g => g != null),
                        ["GlyphHasSize"] = glyphs.All(g => g != null && g.ActualWidth > 0 && g.ActualHeight > 0),
                        ["GlyphHighContrastAdjustmentNone"] = glyphs.All(g => g != null && g.HighContrastAdjustment == ElementHighContrastAdjustment.None),
                    };
                },
            };

            // PressedCheckGlyph is only targeted by the Pressed and Checked states.
            yield return new Scenario
            {
                Name = "RadioButton",
                Build = count => Bench.Rows(count, i => new RadioButton { Content = $"Item {i}", GroupName = $"Group{i}", Width = 120 }),
                DeferredPartNames = new[] { "PressedCheckGlyph" },
                Trigger = root =>
                {
                    foreach (var radioButton in Bench.All<RadioButton>(root).ToList()) radioButton.IsChecked = true;
                },
                Probe = root =>
                {
                    var buttons = Bench.All<RadioButton>(root).ToList();
                    return new Dictionary<string, object>
                    {
                        ["AllChecked"] = buttons.All(b => b.IsChecked == true),
                        ["PressedGlyphRealizedForAll"] = buttons.All(b => Bench.Named(b, "PressedCheckGlyph") != null),
                        ["CheckGlyphPresent"] = buttons.All(b => Bench.Named(b, "CheckGlyph") != null),
                    };
                },
            };

            // HighlightBackground and Pill are only targeted by the keyboard Focused/FocusedPressed states.
            yield return new Scenario
            {
                Name = "ComboBox",
                Build = count => Bench.Rows(count, i =>
                {
                    var comboBox = new ComboBox { Width = 120, SelectedIndex = 0 };
                    comboBox.Items.Add($"Item {i}");
                    comboBox.Items.Add("Second");
                    comboBox.Items.Add("Third");
                    return comboBox;
                }),
                DeferredPartNames = new[] { "HighlightBackground", "Pill" },
                Trigger = root =>
                {
                    foreach (var comboBox in Bench.All<ComboBox>(root).ToList()) VisualStateManager.GoToState(comboBox, "Focused", false);
                },
                Probe = root =>
                {
                    var boxes = Bench.All<ComboBox>(root).ToList();
                    return new Dictionary<string, object>
                    {
                        ["HighlightRealizedForAll"] = boxes.All(b => Bench.Named(b, "HighlightBackground") is FrameworkElement h && h.ActualWidth > 0),
                        ["PillRealizedForAll"] = boxes.All(b => Bench.Named(b, "Pill") is FrameworkElement p && p.ActualHeight > 0),
                        ["SizeUnchanged"] = boxes.All(b => b.ActualWidth == 120),
                    };
                },
            };

            // UserIconBox is only shown when IconSource is set (UserIconVisible state).
            yield return new Scenario
            {
                Name = "InfoBar",
                Build = count => Bench.Rows(count, i => new InfoBar { IsOpen = true, Title = $"Title {i}", Message = "Message", IsClosable = false, Width = 300 }, perRow: 4),
                DeferredPartNames = new[] { "UserIconBox" },
                Trigger = root =>
                {
                    foreach (var infoBar in Bench.All<InfoBar>(root).ToList()) infoBar.IconSource = new SymbolIconSource { Symbol = Symbol.Accept };
                },
                Probe = root =>
                {
                    var bars = Bench.All<InfoBar>(root).ToList();
                    return new Dictionary<string, object>
                    {
                        ["UserIconShown"] = bars.All(b => Bench.Named(b, "UserIconBox") is FrameworkElement u && Bench.IsShown(u, b) && u.ActualWidth > 0),
                        ["StandardIconHidden"] = bars.All(b => Bench.Named(b, "StandardIconArea") is FrameworkElement s && s.Visibility == Visibility.Collapsed),
                    };
                },
            };
        }
    }
}
