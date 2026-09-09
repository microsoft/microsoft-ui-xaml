using System.Text;
using Windows.Foundation.Metadata;
using Muc = Microsoft.UI.Composition;
using MucBackdrops = Microsoft.UI.Composition.SystemBackdrops;
using Mux = Microsoft.UI.Xaml;

namespace SystemComponentExperiment.CSharp;

public sealed partial class ClosureProbeScenarioPage : Mux.Controls.Page
{
    private static readonly Type[] PublicXamlCompositionTypes =
    [
        typeof(Muc.AnimationPropertyInfo),
        typeof(Muc.CompositionBrush),
        typeof(Muc.CompositionEasingFunction),
        typeof(Muc.CompositionLight),
        typeof(Muc.CompositionPropertySet),
        typeof(Muc.Compositor),
        typeof(Muc.IAnimationObject),
        typeof(Muc.ICompositionAnimationBase),
        typeof(Muc.ICompositionSupportsSystemBackdrop),
        typeof(Muc.ICompositionSurface),
        typeof(Muc.IVisualElement),
        typeof(Muc.IVisualElement2),
        typeof(Muc.Visual),
        typeof(MucBackdrops.SystemBackdropConfiguration)
    ];

    private static readonly string[] CandidateSystemTypes =
    [
        "Windows.UI.Composition.AnimationPropertyInfo",
        "Windows.UI.Composition.CompositionBrush",
        "Windows.UI.Composition.CompositionEasingFunction",
        "Windows.UI.Composition.CompositionLight",
        "Windows.UI.Composition.CompositionPropertySet",
        "Windows.UI.Composition.Compositor",
        "Windows.UI.Composition.IAnimationObject",
        "Windows.UI.Composition.ICompositionAnimationBase",
        "Windows.UI.Composition.ICompositionSupportsSystemBackdrop",
        "Windows.UI.Composition.ICompositionSurface",
        "Windows.UI.Composition.IVisualElement",
        "Windows.UI.Composition.IVisualElement2",
        "Windows.UI.Composition.Visual",
        "Windows.System.DispatcherQueue"
    ];

    public ClosureProbeScenarioPage()
    {
        InitializeComponent();
    }

    private void RunProbes_Click(object sender, Mux.RoutedEventArgs args)
    {
        StringBuilder result = new();
        result.AppendLine($"Compile-probed lifted XAML types: {PublicXamlCompositionTypes.Length}");

        foreach (string typeName in CandidateSystemTypes)
        {
            result.AppendLine(
                $"{typeName}: {(ApiInformation.IsTypePresent(typeName) ? "present" : "absent")}");
        }

        try
        {
            Windows.UI.Composition.Compositor compositor = new();
            result.AppendLine($"System Compositor activation: passed ({compositor.GetType().FullName})");
        }
        catch (Exception exception)
        {
            result.AppendLine($"System Compositor activation: failed ({exception.HResult:X8})");
        }

        Windows.System.DispatcherQueue? queue =
            Windows.System.DispatcherQueue.GetForCurrentThread();
        result.AppendLine($"System DispatcherQueue current thread: {(queue is null ? "absent" : "present")}");

        ResultText.Text = result.ToString();
    }
}
