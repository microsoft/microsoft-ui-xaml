// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Runtime.Versioning;
using Microsoft.Diagnostics.Tracing;
using WinUIProfiler.Engine.Events;

internal static class SurfaceTextureTraceIntegration
{
    [SupportedOSPlatform("windows")]
    internal static void Run(string enabledTrace, int enabledPid, string disabledTrace, int disabledPid)
    {
        var enabled = Read(enabledTrace, enabledPid);
        var disabled = Read(disabledTrace, disabledPid);
        var matched = Validate(enabled, disabled);
        Console.WriteLine($"Passed: {enabled.Observations.Count} opt-in surface observations; " +
            $"{matched} matched Direct3D11 texture observations; " +
            $"{enabled.Bindings.Count(binding => binding.Surface != 0)} element-attributed visual/surface bindings; " +
            "disabled-provider control.");
    }

    internal static void RunRegressionTests()
    {
        var enabled = new Capture();
        enabled.Surfaces.UnionWith([1, 2, 3]);
        enabled.Resources.UnionWith([10, 20]);
        enabled.Observations.AddRange([(1, 10), (2, 10), (3, 20)]);
        RecordVisualSurface(enabled, Binding(100, 1, 0, ulong.MaxValue));
        RecordVisualSurface(enabled, Binding(100, 2, 1, ulong.MaxValue));
        RecordVisualSurface(enabled, Binding(100, 3, 0, 2000));
        RecordVisualSurface(enabled, Binding(100, 0, 1, 2000));
        RecordVisualSurface(enabled, Binding(100, 0, 0, 0));
        RecordVisualSurface(enabled, Binding(100, 0, 1, 0));
        Expect(enabled.Bindings.Select(binding => binding.Element).SequenceEqual(new ulong[]
            { ulong.MaxValue, ulong.MaxValue, 2000, 2000, 0, 0 }),
            "Visual bindings must retain full-width element IDs through reuse, surface clearing and unlinking.");
        var disabled = new Capture { XamlEvents = 1 };
        disabled.Resources.Add(30);
        Expect(Validate(enabled, disabled) == 3,
            "Every observation must join, including surfaces that share an atlas texture.");

        enabled.Resources.Remove(20);
        var rejected = false;
        try
        {
            Validate(enabled, disabled);
        }
        catch (InvalidOperationException)
        {
            rejected = true;
        }
        Expect(rejected, "Matching texture observations must not hide an unmatched resource identity.");
        ExpectRejected(() => RecordVisualSurface(enabled, Binding(100, 1, 0, 1000)[..3]),
            "Fresh visual/surface observations must include UIElementId.");
        var wrongType = Binding(100, 1, 0, 1000);
        wrongType[3] = new("UIElementId", 1000u);
        ExpectRejected(() => RecordVisualSurface(enabled, wrongType),
            "A 32-bit value must not masquerade as a core element address.");
        ExpectRejected(() => RecordVisualSurface(enabled, Binding(100, 1, 0, 0)),
            "A surface binding without an element must not pass attribution validation.");
        Console.WriteLine("Passed: surface-to-texture validation accepts shared textures and rejects partial resource joins.");
        Console.WriteLine("Passed: visual/surface bindings carry core element IDs through brush/mask reuse and clearing.");
    }

    private static int Validate(Capture enabled, Capture disabled)
    {
        Expect(enabled.Observations.Count > 0, "The opt-in capture must contain real surface-to-texture observations.");
        Expect(enabled.Observations.All(item => enabled.Surfaces.Contains(item.Surface)),
            "Observed surfaces must join DCompSurfaceInit in the same process.");
        var matched = enabled.Observations.Count(item => enabled.Resources.Contains(item.Resource));
        Expect(matched == enabled.Observations.Count,
            $"Every observed resource ID must match a real Direct3D11 texture resource identity ({matched}/{enabled.Observations.Count} matched).");
        var bindings = enabled.Bindings.Where(binding => binding.Surface != 0).ToArray();
        Expect(bindings.Length > 0 && bindings.All(binding => enabled.Surfaces.Contains(binding.Surface)),
            "Primitive visual/surface bindings must identify an element and join initialized surfaces.");
        Expect(enabled.Observations.Any(observation => bindings.Any(binding => binding.Surface == observation.Surface)),
            "A fresh capture must link an element-attributed primitive to an observed texture.");
        Expect(disabled.XamlEvents > 0 && disabled.Resources.Count > 0,
            "The disabled-provider control must still record an app that renders XAML and creates textures.");
        Expect(disabled.ProfilerEvents == 0,
            "The control without the opt-in provider must not contain profiler events.");
        return matched;
    }

    [SupportedOSPlatform("windows")]
    private static Capture Read(string path, int pid)
    {
        if (pid <= 0) throw new ArgumentOutOfRangeException(nameof(pid));
        var capture = new Capture();
        using var source = new ETWTraceEventSource(path);
        source.Dynamic.All += data =>
        {
            if (data.ProcessID != pid) return;
            if (data.ProviderGuid == XamlProviders.Direct3D11 && (int)data.ID is 12 or 14)
                capture.Resources.Add(Handle(data.PayloadByName("pID3D11Resource")));
            if (data.ProviderGuid == XamlProviders.Xaml)
            {
                capture.XamlEvents++;
                Expect(data.EventName != "DCompSurfaceTextureObserved",
                    "Texture observations must not migrate to the default XAML provider.");
            }
            if (data.ProviderGuid != XamlProviders.XamlProfiler) return;
            capture.ProfilerEvents++;
            if (data.EventName == "DCompSurfaceInit")
                capture.Surfaces.Add(Handle(data.PayloadByName("SurfaceId")));
            if (data.EventName == "VisualSurfaceChanged")
                RecordVisualSurface(capture, data.PayloadNames.Select(name => new EventField(name, data.PayloadByName(name))).ToArray());
            if (data.EventName != "DCompSurfaceTextureObserved") return;
            Expect(data.PayloadNames.SequenceEqual(new[]
            {
                "SurfaceId", "ResourceId", "Width", "Height", "MipLevels", "ArraySize",
                "Format", "SampleCount", "OffsetX", "OffsetY"
            }), "Real observations must preserve the surface/texture event's ordered payload contract.");
            var surface = data.PayloadByName("SurfaceId");
            var resource = data.PayloadByName("ResourceId");
            Expect(surface is ulong or long && resource is ulong or long &&
                Handle(surface) != 0 && Handle(resource) != 0,
                "Surface and resource identities must be nonzero 64-bit values.");
            foreach (var field in new[] { "Width", "Height", "MipLevels", "ArraySize", "Format", "SampleCount" })
                Expect(data.PayloadByName(field) is uint and > 0 or int and > 0,
                    $"Observed {field} must describe the real D3D11 texture.");
            Expect(data.PayloadByName("OffsetX") is int && data.PayloadByName("OffsetY") is int,
                "Atlas offsets must retain their signed 32-bit representation.");
            capture.Observations.Add((Handle(surface), Handle(resource)));
        };
        source.Process();
        Expect(source.EventsLost == 0, "Integration captures must not lose events.");
        return capture;
    }

    private static EventField[] Binding(ulong visual, ulong surface, uint role, ulong element) =>
    [
        new("VisualId", visual), new("SurfaceId", surface), new("Role", role), new("UIElementId", element)
    ];

    private static void RecordVisualSurface(Capture capture, IReadOnlyList<EventField> fields)
    {
        Expect(fields.Select(field => field.Name).SequenceEqual(new[] { "VisualId", "SurfaceId", "Role", "UIElementId" }),
            "Fresh VisualSurfaceChanged events must carry VisualId, SurfaceId, Role and UIElementId in order.");
        Expect(fields[0].Value is ulong or long && fields[1].Value is ulong or long && fields[3].Value is ulong or long,
            "Visual, surface and core element identities must retain their 64-bit representation.");
        var visual = Handle(fields[0].Value);
        var surface = Handle(fields[1].Value);
        var element = Handle(fields[3].Value);
        var role = fields[2].Value switch
        {
            uint value => value,
            int value when value >= 0 => (uint)value,
            _ => throw new InvalidOperationException("The surface role must be an unsigned 32-bit value.")
        };
        Expect(visual != 0 && role is 0 or 1, "A visual/surface binding must identify a primitive and a brush/mask role.");
        Expect(surface == 0 || element != 0, "A nonempty primitive surface binding must identify its core UI element.");
        capture.Bindings.Add((visual, surface, role, element));
    }

    private static void ExpectRejected(Action action, string message)
    {
        try
        {
            action();
        }
        catch (InvalidOperationException)
        {
            return;
        }
        throw new InvalidOperationException(message);
    }

    private static ulong Handle(object? value) => value switch
    {
        ulong handle => handle,
        long handle => unchecked((ulong)handle),
        IntPtr handle => unchecked((ulong)handle.ToInt64()),
        _ => throw new InvalidOperationException("The capture did not contain a 64-bit graphics identity.")
    };

    private static void Expect(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }

    private sealed class Capture
    {
        internal int XamlEvents;
        internal int ProfilerEvents;
        internal readonly HashSet<ulong> Surfaces = [];
        internal readonly HashSet<ulong> Resources = [];
        internal readonly List<(ulong Surface, ulong Resource)> Observations = [];
        internal readonly List<(ulong Visual, ulong Surface, uint Role, ulong Element)> Bindings = [];
    }
}
