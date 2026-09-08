// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Net;
using System.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Windows.ApplicationModel.DataTransfer;
using Windows.System;

namespace TableViewSampleApp.Pages;

public sealed partial class AboutPage : Page
{
    public AboutPage()
    {
        InitializeComponent();
        // Populate Build row from MSBuild-generated BuildInfo (see csproj
        // _GenerateBuildInfo target). Re-emits on every build so the
        // commit SHA + subject in the installed app always matches source.
        var buildDate = BuildInfo.BuildTimestamp.Length >= 10 ? BuildInfo.BuildTimestamp[..10] : BuildInfo.BuildTimestamp;
        BuildLineText.Text          = $"{buildDate} · {BuildInfo.BuildFlavor}";
        BuildCommitText.Text        = $"Commit {BuildInfo.CommitShaShort} on {BuildInfo.Branch}  ({BuildInfo.CommitTimestamp})";
        BuildCommitSubjectText.Text = $"\u201C{BuildInfo.CommitSubject}\u201D";

        ReportEmailButton.Click   += async (_, __) => await LaunchAsync(BuildMailtoUri());
        CopyAutoFillButton.Click  += (_, __) => CopyAutoFillToClipboard();
        ReportLoopButton.Click    += async (_, __) => await LaunchAsync(new Uri(LoopReportUri));
    }

    // Shared Loop page for logging TableView samples issues (the preferred triage channel).
    private const string LoopReportUri = "https://loop.cloud.microsoft/p/eyJ1IjoiaHR0cHM6Ly9taWNyb3NvZnQuc2hhcmVwb2ludC1kZi5jb20vdGVhbXMvU2hlbGxGVEw%2FbmF2PWN6MGxNa1owWldGdGN5VXlSbE5vWld4c1JsUk1KbVE5WWlFdFFWUTBiSEZaV0U5clpUZGZTakJ5T0ZGclJrSklabkpCVXpkclNWOTRUbk13WVRob1JXcDVNMkY1U0dvMlVHWkxURmgxVWpkTllURlZhVVJDZUcxMUptWTlNREV5UzFsVlYweEhWRW8wVUVORFRrRmFVbFpJV1ZvMFJWbEdWamMyVFUxV1RTWmpQU1V5UmlabWJIVnBaRDB4Sm1FOVZHVmhiWE1tY0QwbE5EQm1iSFZwWkhnbE1rWnNiMjl3TFhCaFoyVXRZMjl1ZEdGcGJtVnkifQ%3D%3D";

    private static string GetReporter() => Environment.UserName;

    private static string GetTheme()
    {
        // Read app-level theme via Application.Current.RequestedTheme is the
        // global default; users may also be on "system" if SystemSettings inherits.
        try { return Application.Current.RequestedTheme.ToString(); } catch { return "(unknown)"; }
    }

    private string BuildAutoFillMarkdown()
    {
        var sb = new StringBuilder();
        sb.AppendLine("**TableViewSampleApp bug report**");
        sb.AppendLine();
        sb.AppendLine($"- Reporter: `{GetReporter()}@microsoft.com`");
        sb.AppendLine($"- Reported: `{DateTime.UtcNow:yyyy-MM-ddTHH:mm:ssZ}` UTC");
        sb.AppendLine($"- Sample commit: `{BuildInfo.CommitShaShort}` on `{BuildInfo.Branch}`");
        sb.AppendLine($"- Commit subject: \u201C{BuildInfo.CommitSubject}\u201D");
        sb.AppendLine($"- Build: `{buildFlavorTimestamp()}`");
        sb.AppendLine($"- Theme: `{GetTheme()}`");
        sb.AppendLine($"- OS: `{Environment.OSVersion.VersionString}`  ·  .NET: `{Environment.Version}`");
        sb.AppendLine();
        sb.AppendLine("**Page**: `<paste page name here>`");
        sb.AppendLine();
        sb.AppendLine("**Repro steps**");
        sb.AppendLine("1. ");
        sb.AppendLine("2. ");
        sb.AppendLine("3. ");
        sb.AppendLine();
        sb.AppendLine("**Expected**: ");
        sb.AppendLine("**Actual**: ");
        sb.AppendLine();
        sb.AppendLine("**Crash exception** (if applicable): paste the exception type + message here.");
        return sb.ToString();

        string buildFlavorTimestamp() => $"{BuildInfo.BuildTimestamp} · {BuildInfo.BuildFlavor}";
    }

    private Uri BuildMailtoUri()
    {
        var subject = WebUtility.UrlEncode($"TableViewSampleApp bug — {BuildInfo.CommitShaShort} — <one-line summary>");
        var body    = WebUtility.UrlEncode(BuildAutoFillMarkdown());
        return new Uri($"mailto:hik@microsoft.com?subject={subject}&body={body}");
    }

    private void CopyAutoFillToClipboard()
    {
        var dp = new DataPackage();
        dp.SetText(BuildAutoFillMarkdown());
        Clipboard.SetContent(dp);
        // Cheap visual ack — relabel briefly.
        var orig = CopyAutoFillButton.Content;
        CopyAutoFillButton.Content = "✓ Copied";
        DispatcherQueue.TryEnqueue(async () => { await System.Threading.Tasks.Task.Delay(1500); CopyAutoFillButton.Content = orig; });
    }

    private static async System.Threading.Tasks.Task LaunchAsync(Uri uri)
    {
        try { await Launcher.LaunchUriAsync(uri); } catch { /* user-cancelled or no handler */ }
    }
}
