// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
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

        ReportIssueButton.Click   += async (_, __) => await LaunchAsync(ReportIssueUri);
        CopyAutoFillButton.Click  += (_, __) => CopyAutoFillToClipboard();
    }

    private static readonly Uri ReportIssueUri = new("https://github.com/microsoft/microsoft-ui-xaml/issues/new?template=bug_report.yaml");

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
