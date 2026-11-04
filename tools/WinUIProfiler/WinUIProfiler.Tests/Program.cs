// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Runtime.InteropServices;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Engine.Target;
using XamlTimeline;

const long element = 0x1234;
var path = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-EventCompatibility-{Guid.NewGuid():N}.sqlite");
try
{
    using (var writer = SessionWriter.Create(path, new(1, "CompatibilityTests", null, 1_000_000, 0, SessionCoverage.Trace)))
    {
        ProfilerEvent Event(string name, bool traceLogging, IReadOnlyList<EventField> fields,
            XamlProvider provider = XamlProvider.Xaml, int opcode = 0) =>
            new(1, 0.001, 1, 1, provider, traceLogging, name, opcode, fields);

        EventField[] observation =
        [
            new("IsEnabled", false), new("ElementId", element), new("Name", "Items"),
            new("ClassType", "ListView"), new("PropertyType", "ItemsPanel")
        ];
        writer.Process(1, Event("GenerateContainer/Start", true, [], opcode: 1));
        writer.Process(2, Event("GenerateContainer/Stop", true, [new("ElementId", element)], opcode: 2));
        writer.Process(3, Event("VirtualizationIsEnabledByLayout", true,
        [
            new("ElementId", element), new("IsEnabled", false), new("Name", "Items"),
            new("ClassType", "ListView"), new("PropertyType", "ItemsPanel")
        ]));
        writer.Process(4, Event("VirtualizationIsEnabledByLayout", false, [new("IsEnabled", false)]));
        writer.Process(5, Event("BeginStoryboard/Start", false, [new("ElementId", element)]));
        writer.Process(6, Event("VirtualizationIsEnabledByLayout", false, observation,
            provider: XamlProvider.XamlDiagnostics));
        writer.Process(7, Event("VirtualizationIsEnabledByLayout", false, observation));
    }

    var objects = SessionFile.ReadObjects(path);
    if (objects.Count != 1 || objects[0].Address != element || objects[0].Events != 3)
    {
        throw new InvalidOperationException("The migrated activity and new/legacy virtualization observations must share one element.");
    }

    var mentions = new List<string>();
    SessionFile.ReadTimeline(path, objects[0].ObjectId, 0, -1,
        (profilerEvent, _, _) => mentions.Add(profilerEvent.Name));
    if (!mentions.SequenceEqual(new[]
        { "GenerateContainer/Stop", "VirtualizationIsEnabledByLayout", "VirtualizationIsEnabledByLayout" }))
    {
        throw new InvalidOperationException("Unrelated manifest events must not be added to the element's timeline.");
    }

    Console.WriteLine("Passed: TraceLogging activity identity and new/legacy virtualization observations share one timeline.");
    Console.WriteLine("Passed: version-0 observations, storyboard IDs and other providers are not indexed as that element.");

    var regions = new RegionsLoader().Load(Path.Combine(AppContext.BaseDirectory, "XAML.regions.complete.with.startup.xml"));
    string[] activities =
    [
        "VirtualizationMeasure", "VirtualizationCleanup", "VirtualizationAdd", "GenerateContainer",
        "MeasureChild", "PrepareContainer", "UpdateLogicalScrollData", "SetupCCC", "GenerateItems",
        "GenerateMCContainer", "PlaceElement", "GetElementCount"
    ];
    foreach (var activity in activities)
    {
        var current = regions.Single(region => region.Name == activity && region.Start?.Opcode == 1);
        var start = new TelemetryEvent { ProviderGuid = XamlProviders.Xaml, EventName = activity + "/Start", Opcode = 1 };
        var stop = new TelemetryEvent { ProviderGuid = XamlProviders.Xaml, EventName = activity + "/Stop", Opcode = 2 };
        if (current.Start is null || current.Stop is null ||
            !current.Start.Matches(start) || current.Start.Matches(stop) ||
            !current.Stop.Matches(stop) || current.Stop.Matches(start))
        {
            throw new InvalidOperationException($"{activity} must distinguish TraceLogging Start and Stop opcodes.");
        }

        var legacy = regions.Single(region => region.Name == activity && region.Start?.Opcode is null);
        if (legacy.Start?.EventId is null || legacy.Stop?.EventId is null)
        {
            throw new InvalidOperationException($"{activity} must retain its legacy manifest selectors.");
        }

        var legacyStart = new TelemetryEvent
        {
            ProviderGuid = XamlProviders.Xaml,
            EventId = legacy.Start.EventId.Value,
            EventName = activity + "/Start",
            Opcode = 1
        };
        var legacyStop = new TelemetryEvent
        {
            ProviderGuid = XamlProviders.Xaml,
            EventId = legacy.Stop.EventId.Value,
            EventName = activity + "/Stop",
            Opcode = 2
        };
        if (current.Start.Matches(legacyStart) || current.Stop.Matches(legacyStop) ||
            legacy.Start.Matches(start) || legacy.Stop.Matches(stop) ||
            !legacy.Start.Matches(legacyStart) || !legacy.Stop.Matches(legacyStop))
        {
            throw new InvalidOperationException($"{activity} must not produce duplicate regions in new or legacy traces.");
        }
    }

    try
    {
        new RegionsLoader().LoadFromString("""
            <InstrumentationManifest><Instrumentation><Regions><RegionRoot Name="Invalid">
              <Region Name="InvalidOpcode">
                <Start><Event Provider="{531a35ab-63ce-4bcf-aa98-f88c7a89e455}" Name="GenerateContainer" Opcode="256" /></Start>
                <Stop><Event Provider="{531a35ab-63ce-4bcf-aa98-f88c7a89e455}" Name="GenerateContainer" Opcode="2" /></Stop>
              </Region>
            </RegionRoot></Regions></Instrumentation></InstrumentationManifest>
            """);
        throw new InvalidOperationException("An out-of-range opcode must be rejected.");
    }
    catch (InvalidDataException)
    {
    }

    Console.WriteLine("Passed: all 12 migrated regions distinguish Start/Stop without duplicating legacy manifest regions.");
    Console.WriteLine("Passed: invalid region opcodes are rejected.");
}
finally
{
    foreach (var file in new[] { path, path + "-wal", path + "-shm" })
    {
        File.Delete(file);
    }
}

var commandLine = ProfilerCommandLine.Parse(
[
    "--launch-exe", @"C:\Apps\App.exe", "--arguments", "--mode \"a b\"", "--private-runtime", @"C:\Private\runtime",
    "--runtime-source", @"C:\Apps", "--redirect-controls", "--launch-directory", @"C:\Private\launch", "--trust-local-binaries",
    "--connect"
]);
Expect(commandLine == new ProfilerCommandLine(TargetLaunchKind.Unpackaged, @"C:\Apps\App.exe", "--mode \"a b\"",
        @"C:\Private\runtime", @"C:\Apps", true, @"C:\Private\launch", true, true) &&
    commandLine.GetPrivateRuntime(@"C:\Profiler\NativeLauncher") == new PrivateRuntimeLaunch(@"C:\Private\runtime", @"C:\Apps", true,
        @"C:\Private\launch", @"C:\Profiler\NativeLauncher"),
    "The command line must select the app and its private WinUI DLLs.");
var packagedCommandLine = ProfilerCommandLine.Parse(["--launch-aumid", "Fabrikam.App_8wekyb3d8bbwe!App", "--connect"]);
Expect(packagedCommandLine == new ProfilerCommandLine(TargetLaunchKind.Packaged, "Fabrikam.App_8wekyb3d8bbwe!App", "", null, null,
        false, null, false, true) && packagedCommandLine.GetPrivateRuntime(@"C:\Profiler\NativeLauncher") is null &&
    ProfilerCommandLine.Parse([]) == ProfilerCommandLine.Empty,
    "A packaged app, or no app, must be read without private WinUI DLLs.");
string[] privateFolders = ["--private-runtime", @"C:\Private\runtime", "--runtime-source", @"C:\Apps", "--launch-directory", @"C:\L"];
string[][] invalidCommandLines =
[
    ["--launch-exe"],
    ["--launch"],
    ["--launch-exe", @"C:\Apps\A.exe", "--launch-exe", @"C:\Apps\B.exe"],
    ["--launch-exe", @"C:\Apps\A.exe", "--launch-aumid", "Fabrikam.App_8wekyb3d8bbwe!App"],
    ["--launch-exe", "A.exe"],
    ["--connect"],
    ["--launch-exe", @"C:\Apps\A.exe", "--private-runtime", @"C:\Private\runtime"],
    ["--launch-exe", @"C:\Apps\A.exe", "--private-runtime", "runtime", "--runtime-source", @"C:\Apps", "--launch-directory", @"C:\L"],
    ["--launch-exe", @"C:\Apps\A.exe", "--redirect-controls"],
    ["--launch-aumid", "Fabrikam.App_8wekyb3d8bbwe!App", "--arguments", "x", .. privateFolders]
];
foreach (var arguments in invalidCommandLines)
{
    ExpectThrows<ArgumentException>(() => ProfilerCommandLine.Parse(arguments), $"Must reject: {string.Join(' ', arguments)}");
}

Console.WriteLine("Passed: the command line selects the app and its private WinUI DLLs, and rejects incomplete or conflicting options.");

var runtime = new PrivateRuntimeLaunch(@"C:\Private Runtime\runtime", @"C:\Apps", true, @"C:\L", @"C:\Profiler\NativeLauncher");
const string log = @"C:\L\1\redirect.jsonl";
const string result = @"C:\L\1\launch.json";
if (OperatingSystem.IsWindows())
{
    string[] words = ["", "plain", "has space", @"C:\Program Files\App\", "say \"hi\"", @"back\\""quote", "tab\there", @"\\server\share\",
        "--mode \"a b\""];
    Expect(SplitCommandLine(string.Join(' ', words.Select(NativeLaunchProtocol.Quote))).SequenceEqual(words),
        "Launcher arguments must reach the launcher unchanged.");
    Expect(SplitCommandLine(NativeLaunchProtocol.BuildArguments(@"C:\Apps\App.exe", runtime, "--mode \"a b\"", log, result, null, null))
        .SequenceEqual(["--exe", @"C:\Apps\App.exe", "--source", @"C:\Apps", "--private", @"C:\Private Runtime\runtime", "--controls", "1",
            "--log", log, "--result", result, "--arguments", "--mode \"a b\""]),
        "An unpackaged launch must pass the app's arguments.");
    Expect(SplitCommandLine(NativeLaunchProtocol.BuildArguments(@"C:\Apps\App.exe", runtime with { RedirectControls = false }, "", log,
            result, "Fabrikam.App_8wekyb3d8bbwe!App", "Fabrikam.App_1.0.0.0_x64__8wekyb3d8bbwe"))
        .SequenceEqual(["--exe", @"C:\Apps\App.exe", "--source", @"C:\Apps", "--private", @"C:\Private Runtime\runtime", "--controls", "0",
            "--log", log, "--result", result, "--aumid", "Fabrikam.App_8wekyb3d8bbwe!App", "--package",
            "Fabrikam.App_1.0.0.0_x64__8wekyb3d8bbwe", "--expected-aumid", "Fabrikam.App_8wekyb3d8bbwe!App"]),
        "A packaged launch must name the package and the expected application.");
    Console.WriteLine("Passed: launcher arguments are quoted so that CommandLineToArgvW reads them back unchanged.");
}

ExpectThrows<ArgumentException>(() => NativeLaunchProtocol.BuildArguments(@"C:\Apps\App.exe", runtime, "x", log, result,
    "Fabrikam.App_8wekyb3d8bbwe!App", "Fabrikam.App_1.0.0.0_x64__8wekyb3d8bbwe"), "A packaged launch must not take arguments.");
ExpectThrows<ArgumentException>(() => NativeLaunchProtocol.BuildArguments(@"C:\Apps\App.exe", runtime, "", log, result,
    "Fabrikam.App_8wekyb3d8bbwe!App", null), "A packaged launch must name its package.");
Expect(NativeLaunchProtocol.ParseResult(
        """{"schemaVersion":1,"pid":1234,"error":0,"stage":"ready","packageFullName":"P","aumid":"F!A"}""") ==
    new NativeLaunchResult(1234, 0, "ready", "P", "F!A") &&
    NativeLaunchProtocol.ParseResult("""{"schemaVersion":1,"pid":0,"error":2147942405,"stage":"package_activate"}""") ==
    new NativeLaunchResult(0, 0x80070005, "package_activate", "", "") &&
    NativeLaunchProtocol.ParseOwner(
        """{"schemaVersion":1,"pid":42,"error":0,"stage":"launcher","packageFullName":"","aumid":""}""") == 42,
    "Launcher results must be read.");
string[] invalidResults =
[
    """{"schemaVersion":2,"pid":1,"error":0,"stage":"ready"}""",
    """{"schemaVersion":1,"pid":0,"error":0,"stage":"ready"}""",
    """{"schemaVersion":1,"pid":1,"error":0,"stage":"helper_ready"}""",
    """{"schemaVersion":1,"pid":-1,"error":5,"stage":"configuration"}""",
    """{"schemaVersion":1,"pid":"1","error":0,"stage":"ready"}""",
    """{"schemaVersion":1,"pid":1,"error":5,"stage":""}""",
    "[]"
];
foreach (var json in invalidResults)
{
    ExpectThrows<InvalidDataException>(() => NativeLaunchProtocol.ParseResult(json), $"Must reject the result {json}");
}

ExpectThrows<InvalidDataException>(() => NativeLaunchProtocol.ParseOwner("""{"schemaVersion":1,"pid":42,"error":0,"stage":"ready"}"""),
    "An owner file must come from the launcher stage.");
Console.WriteLine("Passed: launcher results are read, and invalid or inconsistent results are rejected.");

static void Expect(bool condition, string message)
{
    if (!condition)
    {
        throw new InvalidOperationException(message);
    }
}

static void ExpectThrows<T>(Action action, string message) where T : Exception
{
    try
    {
        action();
    }
    catch (T)
    {
        return;
    }

    throw new InvalidOperationException(message);
}

// The arguments CommandLineToArgvW, which the launcher uses, reads from a command line after the program name.
static string[] SplitCommandLine(string arguments)
{
    var argv = CommandLineToArgvW("launcher " + arguments, out var count);
    if (argv == IntPtr.Zero)
    {
        throw new System.ComponentModel.Win32Exception();
    }

    try
    {
        return [.. Enumerable.Range(1, count - 1).Select(index => Marshal.PtrToStringUni(Marshal.ReadIntPtr(argv, index * IntPtr.Size))!)];
    }
    finally
    {
        LocalFree(argv);
    }
}

[DllImport("shell32.dll", CharSet = CharSet.Unicode, ExactSpelling = true, SetLastError = true)]
[DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
static extern IntPtr CommandLineToArgvW(string commandLine, out int count);

[DllImport("kernel32.dll", ExactSpelling = true)]
[DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
static extern IntPtr LocalFree(IntPtr memory);
