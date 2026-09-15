using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Hosting;
using Windows.UI;
using Wuc = Windows.UI.Composition;

namespace SystemComponentExperiment.CSharp;

public sealed partial class CompositionScenarioPage : Page
{
    public CompositionScenarioPage()
    {
        InitializeComponent();
    }

    private void RunScenario_Click(object sender, RoutedEventArgs args)
    {
        try
        {
            Wuc.Visual visual = (Wuc.Visual)(object)ElementCompositionPreview.GetElementVisual(VisualHost);
            Wuc.Compositor compositor = visual.Compositor;
            Wuc.SpriteVisual child = compositor.CreateSpriteVisual();
            child.Size = new(120, 120);
            child.Offset = new(20, 20, 0);
            child.Brush = compositor.CreateColorBrush(Colors.CornflowerBlue);
            ElementCompositionPreview.SetElementChildVisual(
                VisualHost,
                child);
            ResultText.Text = $"Passed: {compositor.GetType().FullName}";
        }
        catch (Exception exception)
        {
            ResultText.Text = $"Failed: {exception}";
        }
    }
}
