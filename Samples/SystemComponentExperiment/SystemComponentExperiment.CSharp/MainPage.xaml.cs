using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace SystemComponentExperiment.CSharp;

public sealed partial class MainPage : Page
{
    public IReadOnlyList<ScenarioDefinition> Scenarios => ScenarioCatalog.All;

    public MainPage()
    {
        InitializeComponent();
        Loaded += OnLoaded;
    }

    private void OnLoaded(object sender, RoutedEventArgs args)
    {
        ScenarioList.SelectedIndex = 0;
    }

    private void ScenarioList_SelectionChanged(object sender, SelectionChangedEventArgs args)
    {
        if (ScenarioList.SelectedItem is ScenarioDefinition scenario)
        {
            ScenarioFrame.Navigate(scenario.PageType);
        }
    }
}
