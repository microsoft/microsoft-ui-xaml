using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Security.Principal;
using Microsoft.Win32;
using ProfilerHarness.Driver;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Engine.Target;

internal static class DriverTests
{
    [STAThread]
    private static async Task Main()
    {
        if (!Guid.TryParse(Environment.GetEnvironmentVariable("PROFILER_HARNESS_HOST_MACHINE_GUID"), out var host) ||
            !Guid.TryParse(Environment.GetEnvironmentVariable("PROFILER_HARNESS_GUEST_MACHINE_GUID"), out var guest) ||
            host == Guid.Empty || guest == Guid.Empty || host == guest ||
            !Guid.TryParse(Registry.GetValue(@"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Cryptography", "MachineGuid", null)?.ToString(),
                out var machine) || machine != guest)
        {
            throw new InvalidOperationException("Tests require approved, distinct host/guest identities and must run in the guest.");
        }
        using var guestKey = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\Virtual Machine\Guest\Parameters");
        Check(guestKey is not null, "Tests require a Hyper-V guest.");

        var options = new TargetConnectionOptions(TargetLaunchKind.Packaged, "Family!App", "", null, @"C:\Payload", true);
        Check(!options.RestartPackagedApp, "Ordinary connections must not terminate existing packaged apps.");
        TargetConnection.ValidateOptions(options with { RestartPackagedApp = true });
        Throws<ArgumentException>(() =>
            TargetConnection.ValidateOptions(options with { Kind = TargetLaunchKind.Unpackaged, RestartPackagedApp = true }));

        using var process = Process.GetCurrentProcess();
        using var identity = WindowsIdentity.GetCurrent();
        var ownerCheck = typeof(TargetLauncher).GetMethod("IsCurrentUserProcess", BindingFlags.NonPublic | BindingFlags.Static)
            ?? throw new InvalidOperationException("The process owner/session guard was not found.");
        Check((bool)ownerCheck.Invoke(null, [process, process.SessionId, identity.User!])!, "The current process should match.");
        Check(!(bool)ownerCheck.Invoke(null, [process, process.SessionId + 1, identity.User!])!, "Other sessions must not match.");
        Check(!(bool)ownerCheck.Invoke(null, [process, process.SessionId, new SecurityIdentifier(WellKnownSidType.NullSid, null)])!,
            "Other owners must not match.");

        var selector = new AutomationSelector(null, "Search", null, "Edit", null, null);
        Throws<InvalidDataException>(() => new AutomationStep("setText", selector, null, null, null, null, null).Validate(0));
        Throws<InvalidDataException>(() => new AutomationStep("sendKeys", null, null, " ", null, null, null).Validate(0));
        new AutomationStep("setText", selector, "", null, null, null, null).Validate(0);
        var wait = new AutomationScenario(1, "bounded wait", null,
            [new("wait", null, null, null, 300_000, null, null)]);
        await ThrowsAsync<TimeoutException>(() => UiAutomationRunner.RunAsync(process.Id, wait, TimeSpan.FromMilliseconds(1)));
        var immediate = wait with { Steps = [new("wait", null, null, null, 0, null, null)] };
        Check((await UiAutomationRunner.RunAsync(process.Id, immediate, TimeSpan.FromSeconds(1))).Count == 1,
            "A wait within the budget should succeed.");
        await TestTextFocusAsync(process.Id);

        var root = Path.Combine(Path.GetTempPath(), $"ProfilerHarness-frames-{Guid.NewGuid():N}");
        Directory.CreateDirectory(root);
        try
        {
            var path = Path.Combine(root, "session.sqlite");
            using (var writer = SessionWriter.Create(path, new(process.Id, "test", null, 1000, 0, SessionCoverage.Trace)))
            {
                void Tick(long sequence, long time, uint frame, bool start) =>
                    writer.Process(sequence, new(time, time, process.Id, 1, XamlProvider.MicrosoftUIXaml, false, "CoreServices_Frame", 0,
                        [new("FrameNumber", frame), new("IsStart", start)]));
                Tick(1, 100, 10, true);
                Tick(2, 110, 10, false);
                Tick(3, 210, 10, true);
                Tick(4, 220, 10, false);
                Tick(5, 230, 11, true);
                Tick(6, 240, 11, false);
                writer.Close("test", 2, 3);
                Check(writer.Failure is null, "The fixture recording failed.");
            }
            var frames = SessionFile.ReadFramesBetween(path, 200, 225).Where(frame => frame.Drawn).ToArray();
            Check(frames.Length == 1 && frames[0].FrameNumber == 10,
                "The first scenario render must be included even when its frame ID existed before the scenario.");
            Check(SessionFile.ReadFramesBetween(path, 221, 225).Count == 0, "Frames outside the scenario must be excluded.");
            Throws<ArgumentOutOfRangeException>(() => SessionFile.ReadFramesBetween(path, 225, 200));
            var info = SessionFile.ReadInfo(path);
            Check(info.EndedUtc is not null && info.StoredEvents == 6 && info.LostByEtw == 2 && info.DroppedByProfiler == 3,
                "Finalized evidence must include queued events and final loss/drop counters.");
        }
        finally
        {
            Directory.Delete(root, recursive: true);
        }
        Console.WriteLine("PASS driver regression tests");
    }

    private static async Task TestTextFocusAsync(int processId)
    {
        var completion = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var thread = new Thread(() =>
        {
            try
            {
                using var form = new System.Windows.Forms.Form { Text = "Harness focus regression" };
                using var search = new System.Windows.Forms.TextBox { Name = $"Search{Guid.NewGuid():N}", Top = 40 };
                using var other = new System.Windows.Forms.Button { Text = "Other" };
                form.Controls.Add(search);
                form.Controls.Add(other);
                form.Shown += async (_, _) =>
                {
                    try
                    {
                        other.Focus();
                        Check(other.Focused, "The fixture must initially focus another control.");
                        var handle = search.Handle;
                        Check(await Task.Run(() =>
                            System.Windows.Automation.AutomationElement.FromHandle(handle).TryGetCurrentPattern(
                                System.Windows.Automation.ValuePattern.Pattern, out _)), "The fixture requires ValuePattern.");
                        var scenario = new AutomationScenario(1, "setText focus", null,
                            [new("setText", new(search.Name, null, null, "Edit", null, null), "value", null, null, null, null)]);
                        await UiAutomationRunner.RunAsync(processId, scenario, TimeSpan.FromSeconds(5));
                        Check(search.Focused && search.Text == "value", "ValuePattern.SetValue must focus the selected control.");
                    }
                    catch (Exception error)
                    {
                        completion.TrySetException(error);
                    }
                    finally
                    {
                        form.Close();
                    }
                };
                System.Windows.Forms.Application.Run(form);
                completion.TrySetResult();
            }
            catch (Exception error)
            {
                completion.TrySetException(error);
            }
        }) { IsBackground = true };
        thread.SetApartmentState(ApartmentState.STA);
        thread.Start();
        await completion.Task.WaitAsync(TimeSpan.FromSeconds(15));
    }

    private static void Check(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }

    private static void Throws<T>(Action action) where T : Exception
    {
        try { action(); }
        catch (T) { return; }
        throw new InvalidOperationException($"Expected {typeof(T).Name}.");
    }

    private static async Task ThrowsAsync<T>(Func<Task> action) where T : Exception
    {
        try { await action(); }
        catch (T) { return; }
        throw new InvalidOperationException($"Expected {typeof(T).Name}.");
    }
}
