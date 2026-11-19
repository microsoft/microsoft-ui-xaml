using System.Collections.Generic;
using System.Linq;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;

namespace TemplateDeferralBench
{
    public static class OneOffScenarios
    {
        public static IEnumerable<Scenario> All()
        {
            // The optional transport buttons are hidden by default (Is*ButtonVisible=false) and are only fetched
            // by MediaTransportControls once their visibility property is set.
            var mtcButtons = new[]
            {
                "StopButton", "SkipBackwardButton", "PreviousTrackButton", "RewindButton",
                "FastForwardButton", "NextTrackButton", "SkipForwardButton", "PlaybackRateButton",
            };
            yield return new Scenario
            {
                Name = "MediaTransportControls",
                Build = count => Bench.Rows(count, i => new MediaTransportControls { Width = 1400 }, perRow: 1),
                DeferredPartNames = mtcButtons,
                Trigger = root =>
                {
                    foreach (var mtc in Bench.All<MediaTransportControls>(root).ToList())
                    {
                        mtc.IsStopButtonVisible = true;
                        mtc.IsSkipBackwardButtonVisible = true;
                        mtc.IsPreviousTrackButtonVisible = true;
                        mtc.IsFastRewindButtonVisible = true;
                        mtc.IsFastForwardButtonVisible = true;
                        mtc.IsNextTrackButtonVisible = true;
                        mtc.IsSkipForwardButtonVisible = true;
                        mtc.IsPlaybackRateButtonVisible = true;
                    }
                },
                Probe = root =>
                {
                    var controls = Bench.All<MediaTransportControls>(root).ToList();
                    var buttons = controls.SelectMany(c => mtcButtons.Select(n => Bench.Named(c, n))).ToList();
                    return new Dictionary<string, object>
                    {
                        ["ButtonsRealizedForAll"] = buttons.All(b => b != null),
                        ["ButtonsVisible"] = buttons.All(b => b != null && b.Visibility == Visibility.Visible),
                        ["ButtonsHaveAutomationName"] = buttons.All(b => b != null && !string.IsNullOrEmpty(Microsoft.UI.Xaml.Automation.AutomationProperties.GetName(b))),
                        ["ShownButtonCount"] = buttons.Count(b => b != null && Bench.IsShown(b)),
                    };
                },
            };

            // HsvPanel (3 TextBoxes + 3 labels) is only shown when the user picks "HSV" in the color representation ComboBox.
            yield return new Scenario
            {
                Name = "ColorPicker",
                Build = count => Bench.Rows(count, i => new ColorPicker { Color = Windows.UI.Color.FromArgb(255, 0, 128, 255), Width = 400 }, perRow: 4),
                DeferredPartNames = new[] { "HsvPanel" },
                Trigger = root =>
                {
                    foreach (var picker in Bench.All<ColorPicker>(root).ToList())
                    {
                        if (Bench.Named(picker, "ColorRepresentationComboBox") is ComboBox comboBox) comboBox.SelectedIndex = 1;
                    }
                },
                Probe = root =>
                {
                    var pickers = Bench.All<ColorPicker>(root).ToList();
                    var panels = pickers.Select(p => Bench.Named(p, "HsvPanel")).ToList();
                    TextBox Box(ColorPicker p, string name) => Bench.Named(p, name) as TextBox;
                    return new Dictionary<string, object>
                    {
                        ["HsvPanelShownForAll"] = panels.All(p => p != null && Bench.IsShown(p)),
                        ["RgbPanelHidden"] = pickers.All(p => Bench.Named(p, "RgbPanel")?.Visibility == Visibility.Collapsed),
                        ["HueText"] = pickers.All(p => Box(p, "HueTextBox")?.Text == "210"),
                        ["SaturationText"] = pickers.All(p => Box(p, "SaturationTextBox")?.Text == "100"),
                        ["ValueText"] = pickers.All(p => Box(p, "ValueTextBox")?.Text == "100"),
                        ["HueBoxHookedUp"] = pickers.All(p => Box(p, "HueTextBox") is TextBox b && !string.IsNullOrEmpty(Microsoft.UI.Xaml.Automation.AutomationProperties.GetName(b))),
                        ["HsvPanelHasSize"] = panels.All(p => p != null && p.ActualWidth > 0 && p.ActualHeight > 0),
                    };
                },
            };

            // In the default (Auto, few pages) mode only the ComboBox is used; the NumberBox and the button panel
            // (ItemsRepeater + selection indicator) are only needed when their display mode is active.
            yield return new Scenario
            {
                Name = "PagerControl",
                Build = count => Bench.Rows(count, i => new PagerControl { NumberOfPages = 5, Width = 400 }, perRow: 3),
                DeferredPartNames = new[] { "NumberBoxDisplay", "NumberPanelItemsRepeater", "NumberPanelCurrentPageIndicator" },
                Trigger = root =>
                {
                    foreach (var pager in Bench.All<PagerControl>(root).ToList())
                    {
                        pager.SelectedPageIndex = 2;
                        pager.DisplayMode = PagerControlDisplayMode.NumberBox;
                        pager.UpdateLayout();
                        pager.DisplayMode = PagerControlDisplayMode.ButtonPanel;
                    }
                },
                Probe = root =>
                {
                    var pagers = Bench.All<PagerControl>(root).ToList();
                    // ItemsRepeater only realizes buttons inside the effective viewport, so layout-dependent
                    // checks only apply to pagers that are within the window.
                    double windowHeight = root.XamlRoot.Size.Height;
                    var inView = pagers.Where(p => p.TransformToVisual(root).TransformPoint(default).Y + p.ActualHeight <= windowHeight).ToList();
                    return new Dictionary<string, object>
                    {
                        ["NumberBoxValue"] = pagers.All(p => (Bench.Named(p, "NumberBoxDisplay") as NumberBox)?.Value == 3),
                        ["NumberBoxMaximum"] = pagers.All(p => (Bench.Named(p, "NumberBoxDisplay") as NumberBox)?.Maximum == 5),
                        ["NumberPanelShown"] = pagers.All(p => Bench.IsShown(Bench.Named(p, "NumberPanelItemsRepeater"))),
                        ["PagersInView"] = $"{inView.Count}/{pagers.Count}",
                        ["PagersWithButtons"] = pagers.Count(p => Bench.Named(p, "NumberPanelItemsRepeater") is ItemsRepeater r && Bench.All<Button>(r).Count() == 5),
                        ["NumberPanelHasButtons"] = pagers.Count > 0 && Bench.Named(pagers[0], "NumberPanelItemsRepeater") is ItemsRepeater r0 && Bench.All<Button>(r0).Count() == 5,
                        ["IndicatorShown"] = pagers.All(p => Bench.IsShown(Bench.Named(p, "NumberPanelCurrentPageIndicator"))),
                        ["IndicatorHasWidth"] = inView.Count > 0 && inView.All(p => Bench.Named(p, "NumberPanelCurrentPageIndicator")?.ActualWidth > 0),
                        ["SelectedPageIndex"] = pagers.All(p => p.SelectedPageIndex == 2),
                    };
                },
            };
        }
    }
}