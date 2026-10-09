using System;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Windows.Storage;

namespace TableViewSampleApp.SelfChecks;

// Scripted checks for TableViewSource.ParentBy / ClearParentBy, driven through the public API.
// Runs from the button, or unattended when a file named "autorun-selfcheck" exists in the app's
// data directory (see DataDirectory): results go to selfcheck-results.txt and the app exits afterwards.
public sealed partial class HierarchySelfCheckPage : Page
{
    public const string TriggerFileName = "autorun-selfcheck";
    public const string ResultsFileName = "selfcheck-results.txt";

    public static bool AutoRun { get; set; }

    // LocalFolder when the app has package identity; otherwise the exe's directory (the build
    // produces an unpackaged app, where ApplicationData.Current throws).
    public static string DataDirectory
    {
        get
        {
            try
            {
                return ApplicationData.Current.LocalFolder.Path;
            }
            catch (Exception)
            {
                return AppContext.BaseDirectory;
            }
        }
    }

    private readonly ObservableCollection<string> _lines = new();
    private bool _running;

    public HierarchySelfCheckPage()
    {
        this.InitializeComponent();
        Results.ItemsSource = _lines;
        Loaded += async (_, _) =>
        {
            if (AutoRun)
            {
                AutoRun = false;
                await RunAsync();
                Application.Current.Exit();
            }
        };
    }

    private async void Run_Click(object sender, RoutedEventArgs e) => await RunAsync();

    private async Task RunAsync()
    {
        if (_running)
        {
            return;
        }

        _running = true;
        RunButton.IsEnabled = false;
        _lines.Clear();
        Summary.Text = "running...";

        var lines = new System.Collections.Generic.List<string>();
        try
        {
            var check = new HierarchySelfCheck(Host, DispatcherQueue);
            var results = await check.RunAllAsync();
            foreach (var r in results)
            {
                lines.Add($"{(r.Passed ? "PASS" : "FAIL")} {r.Name} | expected: {r.Expected} | actual: {r.Actual}");
            }

            int pass = results.Count(r => r.Passed);
            lines.Add($"SUMMARY PASS {pass} / FAIL {results.Count - pass}");
        }
        catch (Exception ex)
        {
            lines.Add($"SUMMARY HARNESS ERROR {ex}");
        }

        foreach (var l in lines)
        {
            _lines.Add(l);
        }

        Summary.Text = lines[^1].Replace("SUMMARY ", "");
        try
        {
            var path = Path.Combine(DataDirectory, ResultsFileName);
            File.WriteAllLines(path, lines, new UTF8Encoding(false));
        }
        catch (Exception ex)
        {
            _lines.Add("could not write results file: " + ex.Message);
        }

        RunButton.IsEnabled = true;
        _running = false;
    }
}
