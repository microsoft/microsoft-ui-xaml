// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using System.Runtime.InteropServices;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;

internal static class ControlProfilerProbe
{
    [DllImport("combase.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
    private static extern int WindowsCreateString(string value, int length, out IntPtr result);

    [DllImport("combase.dll", ExactSpelling = true)]
    private static extern int WindowsDeleteString(IntPtr value);

    [DllImport("combase.dll", ExactSpelling = true)]
    private static extern int RoGetActivationFactory(IntPtr name, ref Guid iid, out IntPtr factory);

    [DllImport("Microsoft.UI.Xaml.Controls.Tabular.dll", EntryPoint = "SendTelemetryOnSuspend", ExactSpelling = true)]
    private static extern void FlushTabular();

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate int ActivateInstance(IntPtr factory, out IntPtr instance);

    private static IntPtr Activate(string name)
    {
        Marshal.ThrowExceptionForHR(WindowsCreateString(name, name.Length, out var className));
        IntPtr factory = IntPtr.Zero;
        try
        {
            var iid = new Guid("00000035-0000-0000-C000-000000000046");
            Marshal.ThrowExceptionForHR(RoGetActivationFactory(className, ref iid, out factory));
            // IActivationFactory::ActivateInstance follows the six IInspectable slots.
            var vtable = Marshal.ReadIntPtr(factory);
            var activate = Marshal.GetDelegateForFunctionPointer<ActivateInstance>(
                Marshal.ReadIntPtr(vtable, 6 * IntPtr.Size));
            Marshal.ThrowExceptionForHR(activate(factory, out var instance));
            return instance;
        }
        finally
        {
            if (factory != IntPtr.Zero) { Marshal.Release(factory); }
            WindowsDeleteString(className);
        }
    }

    internal static void Run(string catalogPath)
    {
        var catalog = File.ReadAllLines(catalogPath).Skip(1).Select(line => line.Split(',')[1]).ToArray();
        var nativeObjects = new List<IntPtr>();
        var sources = new List<object>();
        int failures = 0;
        try
        {
            for (int round = 1; round <= 2; round++)
            {
                foreach (var name in catalog)
                {
                    Console.WriteLine($"CONSTRUCTING {round} {name}");
                    try
                    {
                        if (name == "GroupedSourceAdapter")
                        {
                            sources.Add(TableViewSource.From(new ObservableCollection<object> { "root", "child" })
                                .GroupBy(item => "group"));
                        }
                        else if (name == "HierarchicalSourceAdapter")
                        {
                            sources.Add(TableViewSource.From(new ObservableCollection<object> { "root", "child" })
                                .ParentBy(item => item, item => (string)item == "child" ? "root" : null));
                        }
                        else
                        {
                            var projectedType = typeof(Button).Assembly.GetType(name);
                            if (projectedType?.GetConstructor(Type.EmptyTypes) != null)
                            {
                                sources.Add(Activator.CreateInstance(projectedType));
                            }
                            else
                            {
                                nativeObjects.Add(Activate(name));
                            }
                        }
                        Console.WriteLine($"CONSTRUCTED {round} {name}");
                    }
                    catch (Exception ex)
                    {
                        failures++;
                        Console.Error.WriteLine($"FAILED {round} {name}: {ex}");
                    }
                }
                Program.SendTelemetryOnSuspend();
                FlushTabular();
                // With no new construction, these flushes must not re-emit the previous counters.
                Program.SendTelemetryOnSuspend();
                FlushTabular();
            }
            Program.WriteModules();
            if (failures != 0) { throw new InvalidOperationException($"{failures} control activation failures."); }
            Console.WriteLine($"PASS: constructed {catalog.Length} catalog entries twice; issued populated and empty flushes.");
        }
        finally
        {
            foreach (var instance in nativeObjects) { Marshal.Release(instance); }
            GC.KeepAlive(sources);
        }
    }
}
