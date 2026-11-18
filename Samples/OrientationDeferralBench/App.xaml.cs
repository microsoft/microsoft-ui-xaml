using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text.Json;
using Microsoft.UI;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Windows.Graphics;

namespace OrientationDeferralBench
{
    // Usage: OrientationDeferralBench.exe <label> <count> <output.json>
    // Uses the framework ScrollBar/Slider styles from whichever MUX/MUXC sit next to the exe. A/B is done by
    // swapping binaries: loose XAML (XamlReader) ignores x:Load, so app-supplied templates can't reproduce deferral.
    public partial class App : Application
    {
        private readonly Stopwatch clock = new();
        private readonly Dictionary<string, object> result = new();
        private readonly List<Slider> sliders = new();
        private readonly List<ScrollViewer> scrollViewers = new();
        private Window window;
        private StackPanel root;
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
            string variant = arguments.Length > 1 ? arguments[1] : "Default";
            int count = arguments.Length > 2 ? int.Parse(arguments[2]) : 200;
            output = arguments.Length > 3 ? Path.GetFullPath(arguments[3]) :
                Path.Combine(Path.GetTempPath(), $"OrientationDeferralBench-{Environment.ProcessId}.json");
            UnhandledException += (_, e) => File.WriteAllText(output + ".error", e.Exception.ToString());
            result["Variant"] = variant;
            result["Count"] = count;

            double cpu0 = ThreadCpuMs();
            clock.Start();
            root = new StackPanel { Background = new SolidColorBrush(Colors.White) };
            StackPanel row = null;
            for (int i = 0; i < count; i++)
            {
                if (i % 10 == 0)
                {
                    row = new StackPanel { Orientation = Orientation.Horizontal };
                    root.Children.Add(row);
                }
                var viewer = new ScrollViewer
                {
                    Width = 90, Height = 60, Margin = new Thickness(2),
                    HorizontalScrollBarVisibility = ScrollBarVisibility.Visible,
                    VerticalScrollBarVisibility = ScrollBarVisibility.Visible,
                    HorizontalScrollMode = ScrollMode.Enabled,
                    Content = new Border { Width = 300, Height = 300, Background = new SolidColorBrush(Colors.LightGray) }
                };
                scrollViewers.Add(viewer);
                var slider = new Slider { Width = 90, Margin = new Thickness(2), Value = i % 100 };
                sliders.Add(slider);
                row.Children.Add(viewer);
                row.Children.Add(slider);
            }

            window = new Window { Title = "Orientation deferral bench - " + variant, Content = root };
            window.AppWindow.Resize(new SizeInt32(1200, 900));
            window.AppWindow.Move(new PointInt32(60, 60));
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
                        File.WriteAllText(output + ".error", exception.ToString());
                        throw;
                    }
                });
            }
        }

        private void Measure()
        {
            var counts = CountTree(root);
            result["ElementsAtStartup"] = counts.Total;
            result["SliderNames"] = string.Join(",", Descendants(sliders[0]).OfType<FrameworkElement>().Select(f => f.Name).Where(n => n.Length > 0));
            result["NamedOrientationRootsAtStartup"] = counts.OrientationRoots;

            GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect();
            var process = Process.GetCurrentProcess();
            process.Refresh();
            result["PrivateBytesMB"] = process.PrivateMemorySize64 / 1048576.0;
            result["WorkingSetMB"] = process.WorkingSet64 / 1048576.0;

            // First ScrollBar visual-state change (exercises storyboards whose targets may be deferred).
            var scrollBars = scrollViewers.SelectMany(v => Descendants(v).OfType<ScrollBar>()).ToList();
            result["ScrollBars"] = scrollBars.Count;
            result["IndicatorChangeMs"] = Time(() =>
            {
                foreach (var bar in scrollBars) bar.IndicatorMode = ScrollingIndicatorMode.MouseIndicator;
                root.UpdateLayout();
            });

            // Orientation switch: realizes the other orientation's parts in the deferred variant.
            result["OrientationSwitchMs"] = Time(() =>
            {
                foreach (var slider in sliders) slider.Orientation = Orientation.Vertical;
                root.UpdateLayout();
            });
            result["OrientationSwitchBackMs"] = Time(() =>
            {
                foreach (var slider in sliders) slider.Orientation = Orientation.Horizontal;
                root.UpdateLayout();
            });
            foreach (var slider in sliders) slider.Orientation = Orientation.Vertical;
            root.UpdateLayout();

            // Correctness probes after the switch.
            var probe = sliders[0];
            var vertical = Descendants(probe).OfType<FrameworkElement>().FirstOrDefault(f => f.Name == "VerticalTemplate");
            var horizontal = Descendants(probe).OfType<FrameworkElement>().FirstOrDefault(f => f.Name == "HorizontalTemplate");
            result["SliderVerticalVisible"] = vertical?.Visibility == Visibility.Visible;
            result["SliderHorizontalCollapsed"] = horizontal == null || horizontal.Visibility == Visibility.Collapsed;
            result["SliderVerticalThumbHeight"] = Descendants(probe).OfType<Thumb>().FirstOrDefault(t => t.Name == "VerticalThumb")?.ActualHeight ?? -1;
            var bar0 = scrollBars.FirstOrDefault(b => b.Orientation == Orientation.Vertical);
            var barRoot = bar0 == null ? null : Descendants(bar0).OfType<FrameworkElement>().FirstOrDefault(f => f.Name == "VerticalRoot");
            result["ScrollBarVerticalRootVisible"] = barRoot?.Visibility == Visibility.Visible;
            result["ElementsAfterSwitch"] = CountTree(root).Total;
            result["TotalUiCpuMs"] = ThreadCpuMs();

            File.WriteAllText(output, JsonSerializer.Serialize(result, new JsonSerializerOptions { WriteIndented = true }));
            Exit();
        }

        private static double Time(Action action)
        {
            var watch = Stopwatch.StartNew();
            action();
            return watch.Elapsed.TotalMilliseconds;
        }

        private static (int Total, int OrientationRoots) CountTree(DependencyObject start)
        {
            int total = 0, roots = 0;
            foreach (var element in Descendants(start))
            {
                total++;
                if (element is FrameworkElement f && f.Name is "HorizontalRoot" or "VerticalRoot" or "HorizontalPanningRoot"
                    or "VerticalPanningRoot" or "HorizontalTemplate" or "VerticalTemplate")
                    roots++;
            }
            return (total, roots);
        }

        private static IEnumerable<DependencyObject> Descendants(DependencyObject start)
        {
            var pending = new Stack<DependencyObject>();
            pending.Push(start);
            while (pending.Count != 0)
            {
                var current = pending.Pop();
                yield return current;
                int children = VisualTreeHelper.GetChildrenCount(current);
                for (int i = 0; i < children; i++) pending.Push(VisualTreeHelper.GetChild(current, i));
            }
        }
    }
}
