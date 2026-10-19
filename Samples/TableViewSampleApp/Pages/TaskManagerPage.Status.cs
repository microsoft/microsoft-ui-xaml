// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Globalization;
using System.Linq;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

public sealed partial class TaskManagerPage
{
    private const int SimulatedCores = 12;
    private const double SimulatedMemoryMB = 16 * 1024;

    protected override void RefreshReadouts()
    {
        var visible = VisibleCount();
        Status.Rows = visible == Processes.Count
            ? SampleShaping.RowCountText(visible)
            : string.Format(CultureInfo.CurrentCulture, "{0:N0} of {1:N0} (search)", visible, Processes.Count);
        SelectedProcessText.Text = ProcessTable.SelectedItem is ProcessItem process ? process.Name : "(none)";
        EndTaskButton.IsEnabled = ProcessTable.SelectedItem is ProcessItem;

        LiveText.Text = !LiveUpdatesToggle.IsOn
            ? "Paused"
            : string.Format(CultureInfo.CurrentCulture, "Every {0}", SampleShaping.Label(UpdateIntervalSelector).ToLower(CultureInfo.CurrentCulture));
    }

    // Totals change on every tick, so they are written by the tick (not by RefreshReadouts).
    private void UpdateTotals()
    {
        // CpuPercent is per core, so overall utilization is the sum over the core count. Child process
        // rows are already summed into their app.
        var apps = Processes.Where(p => p.ParentPid is null).ToList();
        TotalsText.Text = string.Format(
            CultureInfo.CurrentCulture,
            "CPU {0:N0}%, memory {1:N0}%, disk {2:N1} MB/s, network {3:N1} Mbps",
            Math.Min(apps.Sum(p => p.CpuPercent) / SimulatedCores, 100),
            Math.Min(apps.Sum(p => p.MemoryMB) / SimulatedMemoryMB * 100, 100),
            apps.Sum(p => p.DiskMBps),
            apps.Sum(p => p.NetworkMbps));
    }
}
