// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Data;

public static class ProcessData
{
    public static IReadOnlyList<string> Categories { get; } = new[] { "Apps", "Background processes", "Windows processes" };

    private static readonly (string Name, string Glyph, double Memory)[] s_newTasks =
    {
        ("Paint", "\uE790", 62.4),
        ("Snipping Tool", "\uE722", 38.9),
        ("Calculator", "\uE8EF", 21.7),
        ("Clock", "\uE823", 17.3),
        ("Photos", "\uEB9F", 148.2),
        ("Media Player", "\uE8D6", 74.6),
    };

    public static object GroupKeyOf(ProcessItem? process, string key)
    {
        var value = key == nameof(ProcessItem.StatusText) ? process?.StatusText : process?.Category;
        return string.IsNullOrWhiteSpace(value) ? SampleShaping.NoneKey : value;
    }

    public static ProcessItem NewTask(int index)
    {
        var template = s_newTasks[index % s_newTasks.Length];
        var round = (index / s_newTasks.Length) + 1;
        return new ProcessItem
        {
            Name = round == 1 ? template.Name : string.Format(CultureInfo.CurrentCulture, "{0} ({1})", template.Name, round),
            Category = "Apps",
            IconGlyph = template.Glyph,
            MemoryBaseline = template.Memory,
            StatusText = ProcessItem.Running,
        };
    }

    public static ObservableCollection<ProcessItem> All()
    {
        var processes = new ObservableCollection<ProcessItem>();

        processes.Add(new ProcessItem
        {
            Name = "Calendar",
            Category = "Apps",
            IconGlyph = "\uE787",
            MemoryBaseline = 28.4,
            Children =
            [
                new() { Name = "Calendar", Category = "Apps", IconGlyph = "\uE787", MemoryBaseline = 10.7 },
                new() { Name = "Crashpad", Category = "Apps", IconGlyph = "\uE7BA", MemoryBaseline = 1.3 },
                new() { Name = "WebView2 GPU Process", Category = "Apps", IconGlyph = "\uE943", MemoryBaseline = 1.7 },
                new() { Name = "WebView2 Manager", Category = "Apps", IconGlyph = "\uE912", MemoryBaseline = 10.9 },
                new() { Name = "WebView2 Utility: Network Service", Category = "Apps", IconGlyph = "\uE968", MemoryBaseline = 2.4 },
                new() { Name = "WebView2 Utility: Storage Service", Category = "Apps", IconGlyph = "\uEDA2", MemoryBaseline = 1.4 },
                new() { Name = "WebView2: Calendar in Taskbar", Category = "Apps", IconGlyph = "\uE774", MemoryBaseline = 0.8, StatusText = ProcessItem.EfficiencyMode, IsEfficiency = true },
            ]
        });

        processes.Add(new ProcessItem
        {
            Name = "Files",
            Category = "Apps",
            IconGlyph = "\uE8B7",
            MemoryBaseline = 34.6,
            Children =
            [
                new() { Name = "Crashpad", Category = "Apps", IconGlyph = "\uE7BA", MemoryBaseline = 1.5 },
                new() { Name = "Files", Category = "Apps", IconGlyph = "\uE8B7", MemoryBaseline = 13.0 },
                new() { Name = "WebView2 GPU Process", Category = "Apps", IconGlyph = "\uE943", MemoryBaseline = 1.7 },
                new() { Name = "WebView2 Manager", Category = "Apps", IconGlyph = "\uE912", MemoryBaseline = 12.7 },
                new() { Name = "WebView2 Utility: Network Service", Category = "Apps", IconGlyph = "\uE968", MemoryBaseline = 3.9 },
                new() { Name = "WebView2 Utility: Storage Service", Category = "Apps", IconGlyph = "\uEDA2", MemoryBaseline = 1.8 },
                new() { Name = "WebView2: Files Search App", Category = "Apps", IconGlyph = "\uE774", MemoryBaseline = 0.5, StatusText = ProcessItem.EfficiencyMode, IsEfficiency = true },
            ]
        });

        processes.Add(new ProcessItem
        {
            Name = "Microsoft Edge",
            Category = "Apps",
            IconGlyph = "\uE774",
            MemoryBaseline = 2371.1,
            IsHighCpu = true,
            IsHighNetwork = true,
            Children =
            [
                new() { Name = "Browser", Category = "Apps", IconGlyph = "\uE774", MemoryBaseline = 320, IsHighCpu = true },
                new() { Name = "GPU Process", Category = "Apps", IconGlyph = "\uE943", MemoryBaseline = 198 },
                new() { Name = "Utility: Audio Service", Category = "Apps", IconGlyph = "\uE8D6", MemoryBaseline = 42 },
                new() { Name = "Utility: Network Service", Category = "Apps", IconGlyph = "\uE968", MemoryBaseline = 56, IsHighNetwork = true },
                new() { Name = "Utility: Storage Service", Category = "Apps", IconGlyph = "\uEDA2", MemoryBaseline = 28 },
                new() { Name = "Extension: uBlock Origin", Category = "Apps", IconGlyph = "\uE71B", MemoryBaseline = 45 },
                new() { Name = "Tab: GitHub", Category = "Apps", IconGlyph = "\uE8A7", MemoryBaseline = 180 },
                new() { Name = "Tab: Stack Overflow", Category = "Apps", IconGlyph = "\uE8A7", MemoryBaseline = 155 },
                new() { Name = "Tab: YouTube", Category = "Apps", IconGlyph = "\uE8A7", MemoryBaseline = 340, IsHighCpu = true },
                new() { Name = "Tab: Microsoft Learn", Category = "Apps", IconGlyph = "\uE8A7", MemoryBaseline = 120 },
                new() { Name = "Tab: Outlook", Category = "Apps", IconGlyph = "\uE8A7", MemoryBaseline = 205 },
                new() { Name = "Tab: Twitter", Category = "Apps", IconGlyph = "\uE8A7", MemoryBaseline = 175 },
                new() { Name = "Tab: Azure Portal", Category = "Apps", IconGlyph = "\uE8A7", MemoryBaseline = 260, IsHighCpu = true },
                new() { Name = "Service Worker", Category = "Apps", IconGlyph = "\uE713", MemoryBaseline = 35 },
                new() { Name = "Crashpad Handler", Category = "Apps", IconGlyph = "\uE7BA", MemoryBaseline = 2.4 },
            ]
        });

        processes.Add(new ProcessItem
        {
            Name = "Microsoft Excel",
            Category = "Apps",
            IconGlyph = "\uE9F9",
            MemoryBaseline = 246.8,
        });

        processes.Add(new ProcessItem
        {
            Name = "Microsoft OneNote",
            Category = "Apps",
            IconGlyph = "\uE70B",
            MemoryBaseline = 166.9,
            IsHighNetwork = true,
        });

        processes.Add(new ProcessItem
        {
            Name = "Microsoft Teams",
            Category = "Apps",
            IconGlyph = "\uE902",
            MemoryBaseline = 264.8,
            IsHighCpu = true,
            IsHighNetwork = true,
            StatusText = ProcessItem.EfficiencyMode,
            IsEfficiency = true,
            Children =
            [
                new() { Name = "Teams Main", Category = "Apps", IconGlyph = "\uE902", MemoryBaseline = 180, IsHighCpu = true },
                new() { Name = "Teams GPU", Category = "Apps", IconGlyph = "\uE943", MemoryBaseline = 32 },
                new() { Name = "Teams Utility", Category = "Apps", IconGlyph = "\uE713", MemoryBaseline = 18 },
                new() { Name = "Teams Media", Category = "Apps", IconGlyph = "\uE8D6", MemoryBaseline = 34.8, IsHighNetwork = true },
            ]
        });

        processes.Add(new ProcessItem
        {
            Name = "Microsoft Visual Studio 2022",
            Category = "Apps",
            IconGlyph = "\uE7C3",
            MemoryBaseline = 642.8,
            IsHighCpu = true,
        });

        processes.Add(new ProcessItem
        {
            Name = "Microsoft Visual Studio 2022",
            Category = "Apps",
            IconGlyph = "\uE7C3",
            MemoryBaseline = 1159.1,
            IsHighCpu = true,
            IsHighDisk = true,
        });

        processes.Add(new ProcessItem
        {
            Name = "Microsoft Visual Studio 2022",
            Category = "Apps",
            IconGlyph = "\uE7C3",
            MemoryBaseline = 962.4,
            IsHighCpu = true,
        });

        processes.Add(new ProcessItem
        {
            Name = "Microsoft Visual Studio 2022",
            Category = "Apps",
            IconGlyph = "\uE7C3",
            MemoryBaseline = 1301.1,
            IsHighCpu = true,
        });

        processes.Add(new ProcessItem
        {
            Name = "Microsoft Word",
            Category = "Apps",
            IconGlyph = "\uE8D2",
            MemoryBaseline = 360.3,
            IsHighCpu = true,
        });

        processes.Add(new ProcessItem
        {
            Name = "MUXControlsTestApp",
            Category = "Apps",
            IconGlyph = "\uE737",
            MemoryBaseline = 170.1,
        });

        processes.Add(new ProcessItem
        {
            Name = "Notepad.exe",
            Category = "Apps",
            IconGlyph = "\uE70F",
            MemoryBaseline = 57.3,
            Children =
            [
                new() { Name = "Notepad", Category = "Apps", IconGlyph = "\uE70F", MemoryBaseline = 32 },
                new() { Name = "Notepad", Category = "Apps", IconGlyph = "\uE70F", MemoryBaseline = 25.3 },
            ]
        });

        processes.Add(new ProcessItem
        {
            Name = "Outlook",
            Category = "Apps",
            IconGlyph = "\uE715",
            MemoryBaseline = 456.4,
            IsHighCpu = true,
            IsHighNetwork = true,
            Children =
            [
                new() { Name = "Outlook", Category = "Apps", IconGlyph = "\uE715", MemoryBaseline = 220 },
                new() { Name = "Crashpad", Category = "Apps", IconGlyph = "\uE7BA", MemoryBaseline = 1.8 },
                new() { Name = "GPU Process", Category = "Apps", IconGlyph = "\uE943", MemoryBaseline = 45 },
                new() { Name = "Utility: Network Service", Category = "Apps", IconGlyph = "\uE968", MemoryBaseline = 32, IsHighNetwork = true },
                new() { Name = "Utility: Storage Service", Category = "Apps", IconGlyph = "\uEDA2", MemoryBaseline = 18 },
                new() { Name = "Service Worker", Category = "Apps", IconGlyph = "\uE713", MemoryBaseline = 14 },
                new() { Name = "Renderer: Mail", Category = "Apps", IconGlyph = "\uE8A7", MemoryBaseline = 68 },
                new() { Name = "Renderer: Calendar", Category = "Apps", IconGlyph = "\uE8A7", MemoryBaseline = 42 },
                new() { Name = "Manager", Category = "Apps", IconGlyph = "\uE912", MemoryBaseline = 15.6 },
            ]
        });

        processes.Add(new ProcessItem
        {
            Name = "Task Manager",
            Category = "Apps",
            IconGlyph = "\uE9D9",
            MemoryBaseline = 38.2,
            IsHighCpu = true,
        });

        processes.Add(new ProcessItem
        {
            Name = "Antimalware Service Executable",
            Category = "Background processes",
            IconGlyph = "\uE74D",
            MemoryBaseline = 210.8,
            IsHighCpu = true,
            IsHighDisk = true,
        });

        processes.Add(new ProcessItem
        {
            Name = "Application Frame Host",
            Category = "Background processes",
            IconGlyph = "\uE737",
            MemoryBaseline = 14.2,
        });

        processes.Add(new ProcessItem
        {
            Name = "COM Surrogate",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 3.1,
        });

        processes.Add(new ProcessItem
        {
            Name = "COM Surrogate",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 2.8,
        });

        processes.Add(new ProcessItem
        {
            Name = "CTF Loader",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 5.4,
        });

        processes.Add(new ProcessItem
        {
            Name = "Desktop Window Manager",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 124.6,
            IsHighCpu = true,
        });

        processes.Add(new ProcessItem
        {
            Name = "Device Census",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 6.2,
        });

        processes.Add(new ProcessItem
        {
            Name = "Host Process for Windows Tasks",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 8.8,
        });

        processes.Add(new ProcessItem
        {
            Name = "Microsoft Distributed Transaction Coordinator",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 4.1,
        });

        processes.Add(new ProcessItem
        {
            Name = "Microsoft Text Input Application",
            Category = "Background processes",
            IconGlyph = "\uE765",
            MemoryBaseline = 42.3,
        });

        processes.Add(new ProcessItem
        {
            Name = "Registry",
            Category = "Background processes",
            IconGlyph = "\uE74C",
            MemoryBaseline = 58.0,
        });

        processes.Add(new ProcessItem
        {
            Name = "Runtime Broker",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 22.5,
        });

        processes.Add(new ProcessItem
        {
            Name = "Runtime Broker",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 15.8,
        });

        processes.Add(new ProcessItem
        {
            Name = "Search Host",
            Category = "Background processes",
            IconGlyph = "\uE721",
            MemoryBaseline = 92.3,
            IsHighCpu = true,
        });

        processes.Add(new ProcessItem
        {
            Name = "Security Health Service",
            Category = "Background processes",
            IconGlyph = "\uE74D",
            MemoryBaseline = 7.6,
        });

        processes.Add(new ProcessItem
        {
            Name = "Shell Infrastructure Host",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 18.4,
        });

        processes.Add(new ProcessItem
        {
            Name = "StartMenuExperienceHost.exe",
            Category = "Background processes",
            IconGlyph = "\uE700",
            MemoryBaseline = 56.7,
        });

        processes.Add(new ProcessItem
        {
            Name = "System",
            Category = "Background processes",
            IconGlyph = "\uE770",
            MemoryBaseline = 0.1,
        });

        processes.Add(new ProcessItem
        {
            Name = "System Idle Process",
            Category = "Background processes",
            IconGlyph = "\uE770",
            MemoryBaseline = 0.0,
        });

        processes.Add(new ProcessItem
        {
            Name = "TextInputHost.exe",
            Category = "Background processes",
            IconGlyph = "\uE765",
            MemoryBaseline = 68.9,
        });

        processes.Add(new ProcessItem
        {
            Name = "User Manager",
            Category = "Background processes",
            IconGlyph = "\uE7BA",
            MemoryBaseline = 5.2,
        });

        processes.Add(new ProcessItem
        {
            Name = "Widget Platform",
            Category = "Background processes",
            IconGlyph = "\uE71D",
            MemoryBaseline = 45.1,
        });

        processes.Add(new ProcessItem
        {
            Name = "Windows Security",
            Category = "Background processes",
            IconGlyph = "\uE74D",
            MemoryBaseline = 0.6,
        });

        processes.Add(new ProcessItem
        {
            Name = "Client Server Runtime Process",
            Category = "Windows processes",
            IconGlyph = "\uE770",
            MemoryBaseline = 2.1,
        });

        processes.Add(new ProcessItem
        {
            Name = "Client Server Runtime Process",
            Category = "Windows processes",
            IconGlyph = "\uE770",
            MemoryBaseline = 1.8,
        });

        processes.Add(new ProcessItem
        {
            Name = "Local Security Authority Process",
            Category = "Windows processes",
            IconGlyph = "\uE72E",
            MemoryBaseline = 18.4,
        });

        processes.Add(new ProcessItem
        {
            Name = "Service Host: Local System",
            Category = "Windows processes",
            IconGlyph = "\uE770",
            MemoryBaseline = 12.3,
            Children =
            [
                new() { Name = "Background Intelligent Transfer Service", Category = "Windows processes", IconGlyph = "\uE770", MemoryBaseline = 2.1 },
                new() { Name = "Cryptographic Services", Category = "Windows processes", IconGlyph = "\uE72E", MemoryBaseline = 4.6 },
                new() { Name = "Windows Update", Category = "Windows processes", IconGlyph = "\uE895", MemoryBaseline = 5.6 },
            ]
        });

        processes.Add(new ProcessItem
        {
            Name = "Service Host: Network Service",
            Category = "Windows processes",
            IconGlyph = "\uE770",
            MemoryBaseline = 8.7,
            Children =
            [
                new() { Name = "DNS Client", Category = "Windows processes", IconGlyph = "\uE968", MemoryBaseline = 3.2 },
                new() { Name = "Network List Service", Category = "Windows processes", IconGlyph = "\uE968", MemoryBaseline = 5.5 },
            ]
        });

        processes.Add(new ProcessItem
        {
            Name = "Services",
            Category = "Windows processes",
            IconGlyph = "\uE770",
            MemoryBaseline = 9.4,
        });

        processes.Add(new ProcessItem
        {
            Name = "Windows Logon Application",
            Category = "Windows processes",
            IconGlyph = "\uE770",
            MemoryBaseline = 4.2,
        });

        processes.Add(new ProcessItem
        {
            Name = "Windows Start",
            Category = "Windows processes",
            IconGlyph = "\uE700",
            MemoryBaseline = 32.4,
        });

        processes.Add(new ProcessItem
        {
            Name = "Windows Explorer",
            Category = "Windows processes",
            IconGlyph = "\uE8B7",
            MemoryBaseline = 112.5,
            IsHighCpu = true,
        });

        // Task Manager shows Running unless a process is suspended or in efficiency mode.
        foreach (var process in processes.Concat(processes.SelectMany(parent => parent.Children)))
        {
            process.StatusText = process.IsEfficiency
                ? ProcessItem.EfficiencyMode
                : process.Name is "Search Host" or "Widget Platform" ? ProcessItem.Suspended : ProcessItem.Running;
        }

        return processes;
    }
}
