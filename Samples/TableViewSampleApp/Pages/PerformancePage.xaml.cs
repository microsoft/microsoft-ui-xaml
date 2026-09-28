// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using TableViewSampleApp.Data;

namespace TableViewSampleApp.Pages;

public sealed partial class PerformancePage : Page
{
    public sealed class PerfRow
    {
        public int Id { get; set; }
        public string Name { get; set; } = string.Empty;
        public string Department { get; set; } = string.Empty;
        public int Salary { get; set; }
    }

    // We hold the working dataset in a plain List<T> (off the bind path) so
    // measurements include ONLY the cost of binding + first-layout, not the
    // cost of building the data. Real-world apps prep data on a background
    // thread and bind on the UI thread, which is the shape this mimics.
    private List<PerfRow> _allRows = new();
    private ObservableCollection<PerfRow> _displayRows = new();
    private long _baselineWorkingSet;
    private long _baselineManagedHeap;
    private bool _hasBaseline;

    public PerformancePage()
    {
        InitializeComponent();
        PerfTable.ItemsSource = _displayRows;
        CaptureBaseline();
    }

    private void CaptureBaseline()
    {
        // Force two GC passes so the baseline reflects steady-state heap, not
        // the spike from page construction.
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
        _baselineWorkingSet = Process.GetCurrentProcess().WorkingSet64;
        _baselineManagedHeap = GC.GetTotalMemory(forceFullCollection: true);
        _hasBaseline = true;
    }

    // ----- Methodology --------------------------------------------------
    //
    // Every scenario:
    //   1. PRE-CLOCK: build the data + force a GC pass so the measurement
    //      window starts from a clean heap and no JIT-pending code paths.
    //   2. CLOCK BEGIN: Stopwatch.StartNew()
    //   3. Mutate the bound collection (Clear + repopulate, or swap
    //      ItemsSource).
    //   4. UpdateLayout(): SYNCHRONOUSLY runs measure+arrange on the
    //      TableView so the elapsed window covers the user-visible "I see
    //      the new rows" moment, not just the C# data assignment.
    //   5. CLOCK END: Stopwatch.Stop()
    //   6. Report elapsed milliseconds.
    //
    // Why this matches user-perceived performance:
    //   - Data prep happens off the clock, mimicking apps that load data
    //     on a background thread.
    //   - UpdateLayout() drains the layout queue immediately so we don't
    //     stop the clock before the rows are visible.
    //   - Two GC passes pre-clock isolate the measurement from background
    //     finalizer noise from prior scenarios.
    //
    // Caveats:
    //   - Debug builds are ~3-5x slower than Release; partner-share
    //     timings should use the Release MSIX.
    //   - First run of any scenario includes JIT warmup; the readout
    //     shows the most recent run, so click twice and use the second
    //     number.
    //   - Memory deltas are reported as managed-heap-only AND total
    //     working set. Managed heap is deterministic (GC.GetTotalMemory
    //     with forceFullCollection); working set is OS-lazy and may lag.

    private long TimedMutation(Action mutation)
    {
        // Pre-clock: settle the heap so background finalizers / GC don't
        // bleed into the measurement window.
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();

        var sw = Stopwatch.StartNew();
        mutation();
        // Drain pending layout work synchronously so the clock includes
        // the first user-visible measure+arrange pass.
        PerfTable.UpdateLayout();
        sw.Stop();
        return sw.ElapsedMilliseconds;
    }

    private static List<PerfRow> GeneratePeople(int count)
    {
        var seed = PersonData.All;
        var depts = new[] { "Sales", "Engineering", "Design", "Finance", "HR", "Marketing", "Support", "Legal" };
        var list = new List<PerfRow>(capacity: count);
        for (int i = 0; i < count; i++)
        {
            var p = seed[i % seed.Count];
            list.Add(new PerfRow
            {
                Id = i,
                Name = $"{p.FirstName} {p.LastName} #{i:D6}",
                Department = depts[i % depts.Length],
                Salary = 40_000 + (i * 137) % 160_000,
            });
        }
        return list;
    }

    private void Repopulate(IEnumerable<PerfRow> rows)
    {
        // Single bound reassignment is faster than per-item Add but loses
        // bound-collection identity. Real ObservableCollection consumers
        // typically Clear() + AddRange-equivalent. We do the same so the
        // CollectionChanged events fire and ItemsRepeater realizes rows
        // through its standard incremental path.
        _displayRows.Clear();
        foreach (var r in rows)
        {
            _displayRows.Add(r);
        }
    }

    private void OnLoad10kClick(object sender, RoutedEventArgs e)
    {
        var data = GeneratePeople(10_000);                  // off-clock
        var ms = TimedMutation(() =>                          // on-clock
        {
            _allRows = data;
            Repopulate(_allRows);
        });
        Load10kResult.Text = $"{ms} ms  ·  {_allRows.Count:N0} rows bound + first layout";
    }

    private void OnLoad100kClick(object sender, RoutedEventArgs e)
    {
        var data = GeneratePeople(100_000);
        var ms = TimedMutation(() =>
        {
            _allRows = data;
            Repopulate(_allRows);
        });
        Load100kResult.Text = $"{ms} ms  ·  {_allRows.Count:N0} rows bound + first layout";
    }

    private void OnSortClick(object sender, RoutedEventArgs e)
    {
        if (_allRows.Count == 0) { SortResult.Text = "Run a Load scenario first"; return; }
        // Sort happens off-clock (pure C#); on-clock we time only the
        // collection swap + layout. That mirrors "user clicked a header,
        // the sort key was already computed, now repopulate".
        var sorted = _allRows.OrderBy(r => r.Name, StringComparer.Ordinal).ToList();
        var ms = TimedMutation(() => Repopulate(sorted));
        SortResult.Text = $"{ms} ms  ·  {sorted.Count:N0} rows repopulated post-sort";
    }

    private void OnFilterClick(object sender, RoutedEventArgs e)
    {
        if (_allRows.Count == 0) { FilterResult.Text = "Run a Load scenario first"; return; }
        var filtered = _allRows.Where(r => r.Salary >= 100_000).ToList();
        var ms = TimedMutation(() => Repopulate(filtered));
        FilterResult.Text = $"{ms} ms  ·  {filtered.Count:N0} / {_allRows.Count:N0} rows match predicate";
    }

    private void OnClearFilterClick(object sender, RoutedEventArgs e)
    {
        if (_allRows.Count == 0) { ClearFilterResult.Text = "Nothing to clear"; return; }
        var ms = TimedMutation(() => Repopulate(_allRows));
        ClearFilterResult.Text = $"{ms} ms  ·  restored {_allRows.Count:N0} rows";
    }

    private void OnSnapshotClick(object sender, RoutedEventArgs e)
    {
        if (!_hasBaseline) CaptureBaseline();
        // Force two GC passes; managed heap is then deterministic. Working
        // set is OS-lazy so the delta is informative but not exact.
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
        var ws = Process.GetCurrentProcess().WorkingSet64;
        var heap = GC.GetTotalMemory(forceFullCollection: true);
        var wsDelta = ws - _baselineWorkingSet;
        var heapDelta = heap - _baselineManagedHeap;
        var sign = (long v) => v >= 0 ? "+" : "";
        SnapshotResult.Text =
            $"WorkingSet {ws / (1024 * 1024)} MB (Δ {sign(wsDelta)}{wsDelta / (1024 * 1024)} MB)" +
            $"  ·  Managed heap {heap / (1024 * 1024)} MB (Δ {sign(heapDelta)}{heapDelta / (1024 * 1024)} MB)";
    }

    private void OnRebaselineClick(object sender, RoutedEventArgs e)
    {
        CaptureBaseline();
        SnapshotResult.Text = $"Baseline reset @ {DateTime.Now:HH:mm:ss}. New deltas will be measured from here.";
    }

    private void OnRunAllClick(object sender, RoutedEventArgs e)
    {
        OnLoad10kClick(sender, e);
        OnLoad100kClick(sender, e);
        OnSortClick(sender, e);
        OnFilterClick(sender, e);
        OnClearFilterClick(sender, e);
        OnSnapshotClick(sender, e);
        RunAllResult.Text =
            $"Completed at {DateTime.Now:HH:mm:ss}. See per-row timings above.  " +
            $"Build flavor: {(IsDebugBuild() ? "Debug (numbers are ~3-5x slower than Release)" : "Release")}";
    }

    private static bool IsDebugBuild()
    {
#if DEBUG
        return true;
#else
        return false;
#endif
    }
}
