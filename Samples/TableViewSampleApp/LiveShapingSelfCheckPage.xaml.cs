using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace TableViewSampleApp;

// Runs LiveShapingSelfCheck from the button, or unattended when a file named "autorun-livecheck"
// exists in HierarchyPage.DataDirectory: results go to livecheck-results.txt and the app exits.
public sealed partial class LiveShapingSelfCheckPage : Page
{
    public const string TriggerFileName = "autorun-livecheck";
    public const string ResultsFileName = "livecheck-results.txt";

    public static bool AutoRun { get; set; }

    private readonly ObservableCollection<string> _lines = new();
    private bool _running;

    public LiveShapingSelfCheckPage()
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

        var lines = new List<string>();
        try
        {
            var results = await new LiveShapingSelfCheck(Host, DispatcherQueue).RunAllAsync();
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

        foreach (var line in lines)
        {
            _lines.Add(line);
        }

        Summary.Text = lines[^1].Replace("SUMMARY ", "");
        try
        {
            File.WriteAllLines(Path.Combine(HierarchyPage.DataDirectory, ResultsFileName), lines, new UTF8Encoding(false));
        }
        catch (Exception ex)
        {
            _lines.Add("could not write results file: " + ex.Message);
        }

        RunButton.IsEnabled = true;
        _running = false;
    }
}
