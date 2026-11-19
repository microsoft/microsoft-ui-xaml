using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text.Json;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Windows.Graphics;

namespace TemplateDeferralBench
{
    // Usage: TemplateDeferralBench.exe <label> <scenario> <count> <output.json>
    //        TemplateDeferralBench.exe list
    // Uses the framework styles of whichever MUX/MUXC (and app .pri) sit next to the exe. A/B is done by swapping
    // binaries: loose XAML (XamlReader) ignores x:Load, so app-supplied templates can't reproduce deferral.
    public partial class App : Application
    {
        private static Scenario[] allScenarios;
        private static Scenario[] AllScenarios => allScenarios ??=
            ItemScenarios.All()
            .Concat(CommandScenarios.All())
            .Concat(SmallControlScenarios.All())
            .Concat(OneOffScenarios.All())
            .ToArray();

        private readonly Stopwatch clock = new();
        private readonly Dictionary<string, object> result = new();
        private Window window;
        private FrameworkElement root;
        private Scenario scenario;
        private string output;
        private bool rendered;

        public App() => InitializeComponent();

        [DllImport("kernel32.dll")] private static extern IntPtr GetCurrentThread();
        [DllImport("kernel32.dll")] private static extern bool GetThreadTimes(IntPtr thread, out long creation, out long exit, out long kernel, out long user);

        private static double ThreadCpuMs()
        {
            GetThreadTimes(GetCurrentThread(), out _, out _, out long kernel, out long user);
            return (kernel + user) / 10000.0;
        }

        protected override void OnLaunched(LaunchActivatedEventArgs args)
        {
            var arguments = Environment.GetCommandLineArgs();
            if (arguments.Length > 1 && arguments[1] == "list")
            {
                File.WriteAllLines(Path.Combine(AppContext.BaseDirectory, "scenarios.txt"), AllScenarios.Select(s => s.Name));
                Exit();
                return;
            }

            string label = arguments.Length > 1 ? arguments[1] : "Default";
            string name = arguments.Length > 2 ? arguments[2] : AllScenarios[0].Name;
            int count = arguments.Length > 3 ? int.Parse(arguments[3]) : 100;
            output = arguments.Length > 4 ? Path.GetFullPath(arguments[4]) :
                Path.Combine(Path.GetTempPath(), $"TemplateDeferralBench-{Environment.ProcessId}.json");
            UnhandledException += (_, e) => File.AppendAllText(output + ".error", e.Exception + Environment.NewLine);
            scenario = AllScenarios.First(s => string.Equals(s.Name, name, StringComparison.OrdinalIgnoreCase));
            result["Variant"] = label;
            result["Scenario"] = scenario.Name;
            result["Count"] = count;

            double cpu0 = ThreadCpuMs();
            clock.Start();
            root = scenario.Build(count);
            window = new Window { Title = $"Template deferral bench - {scenario.Name} - {label}", Content = root };
            window.AppWindow.Resize(new SizeInt32(1400, 1000));
            window.AppWindow.Move(new PointInt32(40, 40));
            Microsoft.UI.Xaml.Media.CompositionTarget.Rendering += OnRendering;
            window.Activate();

            void OnRendering(object sender, object e)
            {
                if (rendered || !root.IsLoaded) return;
                rendered = true;
                Microsoft.UI.Xaml.Media.CompositionTarget.Rendering -= OnRendering;
                result["FirstFrameWallMs"] = clock.Elapsed.TotalMilliseconds;
                result["FirstFrameUiCpuMs"] = ThreadCpuMs() - cpu0;
                root.DispatcherQueue.TryEnqueue(DispatcherQueuePriority.Low, () =>
                {
                    try { Measure(); }
                    catch (Exception exception)
                    {
                        File.AppendAllText(output + ".error", exception + Environment.NewLine);
                        Exit();
                    }
                });
            }
        }

        private void Measure()
        {
            var (total, parts) = CountTree();
            result["ElementsAtStartup"] = total;
            result["DeferredPartsAtStartup"] = parts;

            GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect();
            var process = Process.GetCurrentProcess();
            process.Refresh();
            result["PrivateBytesMB"] = process.PrivateMemorySize64 / 1048576.0;
            result["WorkingSetMB"] = process.WorkingSet64 / 1048576.0;

            if (scenario.Trigger != null)
            {
                result["TriggerMs"] = Time(() =>
                {
                    scenario.Trigger(root);
                    root.UpdateLayout();
                });
                (total, parts) = CountTree();
                result["ElementsAfterTrigger"] = total;
                result["DeferredPartsAfterTrigger"] = parts;
            }

            bool allPass = true;
            if (scenario.Probe != null)
            {
                foreach (var (key, value) in scenario.Probe(root))
                {
                    result["Probe." + key] = value;
                    if (value is bool b && !b) allPass = false;
                }
            }
            result["ProbesPass"] = allPass;
            result["TotalUiCpuMs"] = ThreadCpuMs();

            File.WriteAllText(output, JsonSerializer.Serialize(result, new JsonSerializerOptions { WriteIndented = true }));
            Exit();
        }

        private (int Total, int Parts) CountTree()
        {
            var names = new HashSet<string>(scenario.DeferredPartNames);
            int total = 0, parts = 0;
            foreach (var element in Bench.Descendants(root))
            {
                total++;
                if (element is FrameworkElement f && names.Contains(f.Name)) parts++;
            }
            return (total, parts);
        }

        private static double Time(Action action)
        {
            var watch = Stopwatch.StartNew();
            action();
            return watch.Elapsed.TotalMilliseconds;
        }
    }
}
