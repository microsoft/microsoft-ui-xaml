using Microsoft.UI.Xaml.Controls;

namespace WinUIProfilerHarness.ControlDemo;

internal static class ScenarioWorkload
{
    internal static void Populate(StackPanel panel, int count)
    {
        for (int index = 0; index < count; index++)
        {
            panel.Children.Add(new Button { Content = $"Button {index}" });
            panel.UpdateLayout();
        }
    }
}
