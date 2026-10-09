// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Runtime.InteropServices;
using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.Pages;

public sealed partial class PerformancePage
{
    private long _baselineWorkingSet;
    private long _baselineManagedHeap;

    private static bool IsDebugBuild
    {
        get
        {
#if DEBUG
            return true;
#else
            return false;
#endif
        }
    }

    protected override void RefreshReadouts()
    {
        BuildText.Text = string.Format(CultureInfo.CurrentCulture, "{0}, {1}", IsDebugBuild ? "Debug" : "Release", RuntimeInformation.ProcessArchitecture);
        Status.Rows = SampleShaping.RowCountText(_people.Count);
    }

    private void ShowMemory(long workingSet, long heap)
    {
        const double MB = 1024 * 1024;
        MemoryText.Text = string.Format(
            CultureInfo.CurrentCulture,
            "Managed heap {0:N0} MB ({1:+0;-0;0} MB), working set {2:N0} MB ({3:+0;-0;0} MB)",
            heap / MB,
            (heap - _baselineManagedHeap) / MB,
            workingSet / MB,
            (workingSet - _baselineWorkingSet) / MB);
    }
}
