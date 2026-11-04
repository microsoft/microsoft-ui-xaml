// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;

[assembly: System.Runtime.Versioning.SupportedOSPlatform("windows10.0.17763.0")]

internal static class Program
{
    [DllImport("Microsoft.UI.Xaml.Controls.dll", ExactSpelling = true)]
    internal static extern void SendTelemetryOnSuspend();

    [STAThread]
    private static void Main(string[] args)
    {
        try
        {
            WinRT.ComWrappersSupport.InitializeComWrappers();
            if (args.Contains("--load-only"))
            {
                NativeLibrary.Load("Microsoft.UI.Xaml.dll");
                WriteModules();
                return;
            }
            Application.Start(parameters =>
            {
                SynchronizationContext.SetSynchronizationContext(
                    new DispatcherQueueSynchronizationContext(DispatcherQueue.GetForCurrentThread()));
                _ = new ProbeApp(args);
            });
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine(ex);
            Environment.ExitCode = 1;
        }
    }

    internal static void WriteModules()
    {
        foreach (ProcessModule module in Process.GetCurrentProcess().Modules)
        {
            if (module.ModuleName.StartsWith("Microsoft.UI.Xaml", StringComparison.OrdinalIgnoreCase))
            {
                Console.WriteLine($"MODULE {module.FileName} {Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(module.FileName)))}");
            }
        }
    }
}

internal sealed class ProbeApp : Application, IXamlMetadataProvider
{
    private readonly int primaryThread = Environment.CurrentManagedThreadId;
    private readonly Microsoft.UI.Xaml.XamlTypeInfo.XamlControlsXamlMetaDataProvider metadata = new();
    private readonly List<Window> windows = new();
    private DispatcherQueueTimer timer;
    private readonly string[] arguments;

    internal ProbeApp(string[] arguments)
    {
        this.arguments = arguments;
    }

    public IXamlType GetXamlType(Type type) => metadata.GetXamlType(type);
    public IXamlType GetXamlType(string name) => metadata.GetXamlType(name);
    public XmlnsDefinition[] GetXmlnsDefinitions() => metadata.GetXmlnsDefinitions();

    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        if (Environment.CurrentManagedThreadId != primaryThread)
        {
            return;
        }
        Resources.MergedDictionaries.Add(new XamlControlsResources());
        int controlsIndex = Array.IndexOf(arguments, "--controls");
        if (controlsIndex >= 0)
        {
            try
            {
                Resources.MergedDictionaries.Add(new Microsoft.UI.Xaml.Controls.Tabular.TabularControlsResources());
                ControlProfilerProbe.Run(arguments[controlsIndex + 1]);
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine(ex);
                Environment.ExitCode = 1;
            }
            Exit();
            return;
        }
        for (int i = 0; i < 3; i++)
        {
            var panel = new StackPanel();
            panel.Children.Add(new Button { Content = $"Button {i}" });
            panel.Children.Add(new TextBlock { Text = "Telemetry probe" });
            panel.Children.Add(new NavigationView { Content = new TextBlock { Text = "Navigation" } });
            panel.Children.Add(new TabView());
            var window = new Window { Title = $"Telemetry probe {i}", Content = panel };
            window.Activate();
            windows.Add(window);
        }
        Program.WriteModules();
        timer = DispatcherQueue.GetForCurrentThread().CreateTimer();
        timer.Interval = TimeSpan.FromSeconds(3);
        timer.Tick += async (_, _) =>
        {
            timer.Stop();
            try
            {
                for (int round = 0; round < 2; round++)
                {
                    var queues = new List<DispatcherQueueController>();
                    var completions = new List<Task>();
                    for (int i = 0; i < 2; i++)
                    {
                        var queue = DispatcherQueueController.CreateOnDedicatedThread();
                        queues.Add(queue);
                        var completion = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
                        completions.Add(completion.Task);
                        if (!queue.DispatcherQueue.TryEnqueue(() =>
                        {
                            try
                            {
                                using var manager = Microsoft.UI.Xaml.Hosting.WindowsXamlManager.InitializeForCurrentThread();
                                using var nested = Microsoft.UI.Xaml.Hosting.WindowsXamlManager.InitializeForCurrentThread();
                                var button = new Button { Content = "Secondary UI thread" };
                                Console.WriteLine($"PASS: nested XAML managers on UI thread {Environment.CurrentManagedThreadId}");
                                completion.SetResult();
                            }
                            catch (Exception ex) { completion.SetException(ex); }
                        }))
                        {
                            throw new InvalidOperationException("Failed to enqueue UI thread work.");
                        }
                    }
                    await Task.WhenAll(completions);
                    foreach (var queue in queues)
                    {
                        await queue.ShutdownQueueAsync();
                    }
                }
                Program.SendTelemetryOnSuspend();
                Console.WriteLine("PASS: three windows, four additional UI threads, nested XAML managers, and controls telemetry flush.");
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine(ex);
                Environment.ExitCode = 1;
            }
            finally
            {
                foreach (var window in windows)
                {
                    window.Close();
                }
                Exit();
            }
        };
        timer.Start();
    }
}
