using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;

namespace XamlProfiler.Services;

/// <summary>
/// The ETW providers the profiler enables, and how their events map onto <see cref="EtwEvent"/>
/// for <see cref="ProfilerEventRouter"/>. Has no WinUI dependency, so an .etl can be replayed
/// through the same mapping offline.
/// </summary>
public static class ProfilerProviders
{
    // "Microsoft-Windows-XAML-Profiler" (XamlProfilerTracing.cpp), whose GUID is the EventSource
    // name hash of the provider name.
    public static readonly Guid XamlProfilerGuid = new("26f28a5c-7d30-54c5-40f4-fff6d3beda7d");

    // XamlProfilerKeywords (XamlProfilerTracing.h): XamlTree (0x01) | CompTree (0x02) | WucTree (0x04).
    // The router only consumes the tree events, so the surface, texture, heap-snapshot and
    // GPU-snapshot groups (and the work behind them) stay off in the target.
    public const ulong TreeKeywords = 0x01 | 0x02 | 0x04;

    // Microsoft-Windows-XAML, the framework's manifest provider. Its ElementSetNameInfo event (213)
    // carries the x:Name that ElementEnteredTree doesn't, and ElementCreatedInfo (61) marks a new
    // element, so a name held for a destroyed element at the same address is dropped. Both are in the
    // Detailed keyword; the event-ID filter keeps the keyword's other events (layout, parsing,
    // ...) out of the session. The target still pays for the EventWrite calls ETW drops.
    public static readonly Guid XamlGuid = new("531a35ab-63ce-4bcf-aa98-f88c7a89e455");
    private const ulong XamlDetailedKeyword = 0x1;
    private const int ElementCreatedId = 61;
    private const int ElementSetNameId = 213;

    // Names the router dispatches the two Microsoft-Windows-XAML events under.
    public const string ElementCreatedEvent = "Microsoft-Windows-XAML/ElementCreated";
    public const string ElementSetNameEvent = "Microsoft-Windows-XAML/ElementSetName";

    /// <summary>
    /// Enable both providers in <paramref name="session"/>, scoped to processes running
    /// <paramref name="processName"/> (a file name such as "MyApp.exe"), else to
    /// <paramref name="processId"/>, else to every process. Re-enabling replaces the scope.
    /// </summary>
    public static void Enable(TraceEventSession session, string? processName, int processId)
    {
        session.EnableProvider(XamlProfilerGuid, TraceEventLevel.Verbose, TreeKeywords,
            Scope(new TraceEventProviderOptions(), processName, processId));

        var xamlOptions = new TraceEventProviderOptions
        {
            EventIDsToEnable = new List<int> { ElementCreatedId, ElementSetNameId },
        };
        session.EnableProvider(XamlGuid, TraceEventLevel.Verbose, XamlDetailedKeyword,
            Scope(xamlOptions, processName, processId));
    }

    /// <summary>
    /// Copy an event the router consumes into an <see cref="EtwEvent"/>. Returns false for any
    /// other event. Profiler events keep their name and every payload value by position; the
    /// Microsoft-Windows-XAML events are mapped by field name.
    /// </summary>
    public static bool TryCapture(TraceEvent evt, out EtwEvent captured)
    {
        var provider = evt.ProviderGuid;
        if (provider == XamlProfilerGuid)
        {
            int count = evt.PayloadNames?.Length ?? 0;
            var payload = new object?[count];
            for (int i = 0; i < count; i++)
            {
                try { payload[i] = evt.PayloadValue(i); }
                catch { payload[i] = null; }
            }
            captured = new EtwEvent(evt.EventName, payload);
            return true;
        }

        if (provider == XamlGuid)
        {
            switch ((int)evt.ID)
            {
                case ElementCreatedId:
                    captured = new EtwEvent(ElementCreatedEvent, new[] { evt.PayloadByName("ElementId") });
                    return true;
                case ElementSetNameId:
                    captured = new EtwEvent(ElementSetNameEvent, new[] { evt.PayloadByName("ElementId"), evt.PayloadByName("Name") });
                    return true;
            }
        }

        captured = default;
        return false;
    }

    private static TraceEventProviderOptions Scope(TraceEventProviderOptions options, string? processName, int processId)
    {
        if (!string.IsNullOrEmpty(processName))
        {
            options.ProcessNameFilter = new List<string> { processName };
        }
        else if (processId != 0)
        {
            options.ProcessIDFilter = new List<int> { processId };
        }
        return options;
    }
}
