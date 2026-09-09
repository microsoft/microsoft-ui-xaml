using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace SystemComponentExperiment.CSharp;

public sealed partial class StartupScenarioPage : Page
{
    private static readonly HashSet<string> ForbiddenModules = new(
        [
            "CoreMessagingXP.dll",
            "dcompi.dll",
            "DwmSceneI.dll",
            "dwmcorei.dll",
            "marshal.dll",
            "Microsoft.UI.Composition.OSSupport.dll",
            "wuceffectsi.dll"
        ],
        StringComparer.OrdinalIgnoreCase);

    public StartupScenarioPage()
    {
        InitializeComponent();
        Loaded += OnLoaded;
    }

    private void OnLoaded(object sender, RoutedEventArgs args)
    {
        try
        {
            string[] modules = Process.GetCurrentProcess()
                .Modules
                .Cast<ProcessModule>()
                .Select(module => module.ModuleName)
                .Where(name => name is not null)
                .OrderBy(name => name, StringComparer.OrdinalIgnoreCase)
                .ToArray()!;
            string[] forbiddenModules = modules
                .Where(ForbiddenModules.Contains)
                .ToArray();

            StatusText.Text = forbiddenModules.Length == 0 ? "Passed" : "Failed";
            DetailsText.Text = string.Join(
                Environment.NewLine,
                $"OS: {RuntimeInformation.OSDescription}",
                $"Architecture: {RuntimeInformation.ProcessArchitecture}",
                $"Packaged: {Windows.ApplicationModel.Package.Current is not null}",
                $"Forbidden modules: {(forbiddenModules.Length == 0 ? "none" : string.Join(", ", forbiddenModules))}",
                string.Empty,
                "Loaded modules:",
                string.Join(Environment.NewLine, modules));
        }
        catch (Exception exception)
        {
            StatusText.Text = "Failed";
            DetailsText.Text = exception.ToString();
        }
    }
}
