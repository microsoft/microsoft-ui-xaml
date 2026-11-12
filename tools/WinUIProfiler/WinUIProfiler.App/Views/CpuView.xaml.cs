// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using Microsoft.UI;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.Windows.Storage.Pickers;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Engine.Symbols;

namespace WinUIProfiler.App.Views;

// A row of the CPU tab's function list: a function the samples ran, with the share of the samples that were running it
// (self) and that had it anywhere on the stack (total).
public sealed class CpuFunctionRow(CpuFunctionSamples item, string self, string total, string toolTip)
{
    internal CpuFunctionSamples Item { get; } = item;

    public string Function => Item.Frame;

    public string Self { get; } = self;

    public string Total { get; } = total;

    public string ToolTip { get; } = toolTip;

    // Addresses the symbol path didn't name are dimmed.
    public double Opacity => Item.Named ? 1 : 0.6;

    public override string ToString() => Function;
}

// A node of the CPU tab's call tree, held as its TreeViewNode's content. A label with no node sums the children past the
// tree's limit.
public sealed class CpuTreeLabel(CpuCallTreeNode? node, string function, string total, string self, string ms, double opacity,
    string toolTip)
{
    internal CpuCallTreeNode? Node { get; } = node;

    public string Function { get; } = function;

    public string Total { get; } = total;

    public string Self { get; } = self;

    public string Ms { get; } = ms;

    public double Opacity { get; } = opacity;

    public string ToolTip { get; } = toolTip;

    public override string ToString() => Function;
}

// The CPU tab: the functions the session's sampled call stacks ran, hottest first, and the call tree of all the samples
// or the callers or callees of the selected function, as in WPA. While a session records, the samples saved so far are
// shown, and a symbol path names their functions as their PDBs are found (SessionSymbolizer stores the names when it
// ends); without one they're module!0xaddress.
public sealed partial class CpuView : UserControl
{
    // A node's children past this many are summed into one row, so a node of thousands of unnamed addresses opens quickly.
    private const int MaxChildren = 300;

    // How many levels of the heaviest path, the children with at least half of the samples, open with the tree.
    private const int MaxHeavyDepth = 40;

    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();
    private readonly DispatcherQueueTimer searchTimer;
    private readonly DispatcherQueueTimer liveTimer;
    private readonly List<int?> threadIds = [];
    private SessionSource? source;
    private string? path;
    private bool live;
    // The samples the recording had saved at the last count, while the shown ones are older.
    private long? liveSamples;
    private bool counting;
    private bool symbolizing;
    private int? threadId;
    private CpuStacks? stacks;
    private IReadOnlyList<CpuFunctionSamples> functions = [];
    private List<CpuFunctionRow> rows = [];
    private bool read;
    private bool stale = true;
    private bool reading;
    private bool readAgain;
    private CancellationTokenSource? readCancellation;
    private int shown;
    private int treeVersion;
    private string? selectedFrame;
    private bool populating;
    private string? symbolPath;
    // Names the recording's functions in memory while it records; see StartNaming.
    private LiveSymbolNamer? namer;
    private Task<int>? naming;
    private CancellationTokenSource? namingCancellation;
    private bool namerFailed;
    // The next read is the one a naming round asked for, to show its names; it asks for no more.
    private bool namesRead;

    public CpuView()
    {
        InitializeComponent();
        searchTimer = DispatcherQueue.CreateTimer();
        searchTimer.Interval = TimeSpan.FromMilliseconds(300);
        searchTimer.IsRepeating = false;
        searchTimer.Tick += (_, _) => ShowFunctions();
        liveTimer = DispatcherQueue.CreateTimer();
        liveTimer.Interval = TimeSpan.FromSeconds(5);
        liveTimer.Tick += (_, _) => _ = CountLiveAsync();
        SortOrder.SelectedIndex = 0;
        TreeMode.SelectedIndex = 0;
        SortOrder.SelectionChanged += (_, _) => ShowFunctions();
        TreeMode.SelectionChanged += (_, _) => ShowTree();
        ThreadFilter.SelectionChanged += ThreadFilter_SelectionChanged;

        // The tab's content unloads while another tab is shown, so the file is read when it's shown, and again while it
        // records.
        Loaded += (_, _) =>
        {
            stale |= live;
            namesRead = false;
            UpdateLiveTimer();
            _ = RefreshAsync();
        };
        Unloaded += (_, _) => liveTimer.Stop();
        Show(null);
    }

    public WindowId Owner { get; set; }

    public SessionSource? Source
    {
        get => source;
        set
        {
            if (source is not null)
            {
                source.Changed -= Source_Changed;
            }

            source = value;
            if (source is not null)
            {
                source.Changed += Source_Changed;
                Source_Changed(source, EventArgs.Empty);
            }
        }
    }

    // Whether the app is naming the functions of the session file's samples. The file isn't read meanwhile, since the
    // names are written to it module by module, and it's read again when they're done.
    public bool Symbolizing
    {
        get => symbolizing;
        set
        {
            if (symbolizing == value)
            {
                return;
            }

            symbolizing = value;
            if (value)
            {
                readCancellation?.Cancel();
                StopNamer();
            }

            stale = true;
            ShowNotice();
            _ = RefreshAsync();
        }
    }

    // The symbol path that names the functions of a recording's samples while it records, or null.
    public string? SymbolPath
    {
        get => symbolPath;
        set
        {
            if (symbolPath == value)
            {
                return;
            }

            symbolPath = value;
            namerFailed = false;
            StopNamer();
            if (live)
            {
                stale = true;
                _ = RefreshAsync();
            }
        }
    }

    private string Text(string key) => strings.GetString(key);

    private string Format(string key, params object?[] values) => string.Format(CultureInfo.CurrentCulture, Text(key), values);

    private static string Percent(CpuStacks value, long samples) =>
        value.Samples == 0 ? string.Empty : (samples / (double)value.Samples).ToString("P1", CultureInfo.CurrentCulture);

    private static string Milliseconds(CpuStacks value, long samples) =>
        value.Ms(samples) is { } ms ? ms.ToString("N1", CultureInfo.CurrentCulture) : string.Empty;

    private void Source_Changed(object? sender, EventArgs args)
    {
        if (source!.Path != path)
        {
            live = source.Live;
            Show(source.Path);
        }
        else if (live != source.Live)
        {
            // The recording's end completes the file's samples.
            live = source.Live;
            stale = true;
            if (!live)
            {
                StopNamer();
            }
        }

        UpdateLiveTimer();
        ShowNotice();
        _ = RefreshAsync();
    }

    // Shows a session file, or none; it's read when the tab is shown.
    private void Show(string? file)
    {
        path = file;
        shown++;
        readCancellation?.Cancel();
        StopNamer();
        threadId = null;
        selectedFrame = null;
        liveSamples = null;
        stale = true;
        ReadError.IsOpen = false;
        ToolTipService.SetToolTip(FileSummary, file);
        SetStacks(null, [], false);
    }

    private void SetStacks(CpuStacks? value, IReadOnlyList<CpuFunctionSamples> items, bool loaded)
    {
        stacks = value;
        functions = items;
        read = loaded;
        rows = value is null ? [] : items.Select(item => Row(value, item)).ToList();
        ShowSummary();
        ShowThreads();
        ShowNotice();
        ShowButtons();
        ShowFunctions();
        ShowTree();
    }

    private CpuFunctionRow Row(CpuStacks value, CpuFunctionSamples item)
    {
        var self = Percent(value, item.Self);
        var total = Percent(value, item.Total);
        return new CpuFunctionRow(item, self, total,
            Format("CpuViewFunctionToolTipFormat", item.Frame, item.Self, self, item.Total, total));
    }

    private void ShowSummary()
    {
        if (path is null)
        {
            FileSummary.Text = Text("FramesNoFile");
            return;
        }

        var name = Path.GetFileName(path);
        FileSummary.Text = stacks?.Sampling is not { Samples: > 0 } sampling ? name
            : sampling.SymbolizedUtc is null || string.IsNullOrEmpty(sampling.SymbolPath) ? Format("CpuViewSummaryFormat", name, sampling.Samples)
            : Format("CpuViewSymbolsFormat", name, sampling.Samples, sampling.SymbolPath);
    }

    // Lists the sampled threads, busiest first, after all of them together.
    private void ShowThreads()
    {
        populating = true;
        try
        {
            ThreadFilter.Items.Clear();
            threadIds.Clear();
            if (stacks is { Threads.Count: > 0 } value)
            {
                ThreadFilter.Items.Add(Format("CpuViewAllThreadsFormat", value.Threads.Sum(thread => thread.Samples)));
                threadIds.Add(null);
                foreach (var thread in value.Threads)
                {
                    ThreadFilter.Items.Add(Format(thread.UiThread ? "CpuViewUiThreadFormat" : "CpuViewThreadFormat",
                        thread.ThreadId, thread.Samples));
                    threadIds.Add(thread.ThreadId);
                }

                ThreadFilter.SelectedIndex = Math.Max(0, threadIds.IndexOf(threadId));
            }

            ThreadFilter.IsEnabled = threadIds.Count > 2;
        }
        finally
        {
            populating = false;
        }
    }

    // Says why there are no call stacks, or that their functions are addresses until a symbol path names them.
    private void ShowNotice()
    {
        var message = NoticeText();
        Notice.Message = message ?? string.Empty;
        Notice.IsOpen = message is not null;
    }

    private string? NoticeText()
    {
        if (path is null || (!live && !symbolizing && !read))
        {
            return null;
        }

        if (live)
        {
            var shownSamples = stacks?.Sampling.Samples ?? 0;
            return liveSamples is { } saved && saved != shownSamples
                ? Format("CpuViewLiveChangedFormat", saved, shownSamples)
                : Text("CpuViewLive");
        }

        if (symbolizing)
        {
            return Text("CpuViewSymbolizing");
        }

        if (stacks is not { } value)
        {
            return Text("CpuViewNoSampling");
        }

        if (value.Samples == 0)
        {
            return value.Sampling.Note ?? Text("CpuNoSamples");
        }

        return value.Sampling.SymbolizedUtc is null ? Text("CpuViewNotSymbolized") : value.Sampling.Note;
    }

    private void ShowButtons() => RefreshButton.IsEnabled = path is not null && !reading && !symbolizing;

    // While the shown session records, its saved samples are counted every few seconds. The first ones are shown as they
    // come; later ones are offered by the notice, so the lists and tree don't change while they're read.
    private void UpdateLiveTimer()
    {
        if (live && path is not null && IsLoaded)
        {
            if (!liveTimer.IsRunning)
            {
                liveTimer.Start();
            }
        }
        else
        {
            liveTimer.Stop();
        }
    }

    private async Task CountLiveAsync()
    {
        if (counting || reading || !live || symbolizing || path is not { } file || !IsLoaded)
        {
            return;
        }

        counting = true;
        var current = shown;
        try
        {
            var saved = await Task.Run(() => CpuProfileReader.ReadSampling(file)?.Samples ?? 0);
            if (current != shown || !live || reading || symbolizing)
            {
                return;
            }

            if (saved > 0 && stacks is not { Sampling.Samples: > 0 })
            {
                stale = true;
                _ = RefreshAsync();
                return;
            }

            liveSamples = saved;
            ShowNotice();
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            if (current == shown)
            {
                ShowError(error);
            }
        }
        finally
        {
            counting = false;
        }
    }

    // Reads the session file's call stacks, and the functions they ran, on a worker thread.
    private async Task RefreshAsync()
    {
        if (reading)
        {
            readAgain = true;
            return;
        }

        if (!stale || path is not { } file || !IsLoaded || symbolizing)
        {
            return;
        }

        reading = true;
        ShowButtons();
        using var cancellation = new CancellationTokenSource();
        readCancellation = cancellation;
        var current = shown;
        var thread = threadId;
        // While the session records, a read shows the names found so far and asks the namer for the rest, except a read
        // that shows a naming round's names, which asks for no more.
        var forNames = namesRead;
        namesRead = false;
        Func<long, string?>? functionName = null;
        if (Namer(file) is { } active)
        {
            functionName = forNames ? active.NameOf : active.Request;
        }

        try
        {
            var loaded = await Task.Run(() =>
            {
                var value = CpuProfileReader.ReadStacks(file, thread, cancellation.Token, functionName);
                IReadOnlyList<CpuFunctionSamples> items = value?.Functions() ?? [];
                return (Stacks: value, Functions: items);
            }, cancellation.Token);
            // A thread picked meanwhile is read next.
            if (current != shown || thread != threadId || cancellation.IsCancellationRequested)
            {
                return;
            }

            // A naming round that ended meanwhile reads again, to show its names.
            stale = namesRead;
            liveSamples = null;
            ReadError.IsOpen = false;
            SetStacks(loaded.Stacks, loaded.Functions, true);
            if (namer is { Requested: true })
            {
                StartNaming();
            }
        }
        catch (OperationCanceledException) when (cancellation.IsCancellationRequested)
        {
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            if (current == shown && !cancellation.IsCancellationRequested)
            {
                stale = false;
                ShowError(error);
            }
        }
        finally
        {
            readCancellation = null;
            reading = false;
            ShowButtons();
            if (readAgain)
            {
                readAgain = false;
                _ = RefreshAsync();
            }
        }
    }

    // The namer of the recording's file, made when a symbol path is set; null when the file isn't recording.
    private LiveSymbolNamer? Namer(string file)
    {
        if (!live || symbolizing || namerFailed || string.IsNullOrWhiteSpace(symbolPath))
        {
            return null;
        }

        if (namer is { } existing && existing.SessionPath == file)
        {
            return existing;
        }

        StopNamer();
        try
        {
            var created = new LiveSymbolNamer(file, symbolPath);
            namer = created;
            return created;
        }
        catch (ArgumentException)
        {
            // A malformed symbol path names nothing; the symbolizing at the recording's end reports it.
            namerFailed = true;
            return null;
        }
    }

    // Names the functions the reads asked for on a background thread, then reads the file again to show them. While the
    // session records, its file is written meanwhile, so the names are kept in memory until SessionSymbolizer stores them.
    private void StartNaming()
    {
        if (naming is not null || namer is not { } current || symbolizing || !live)
        {
            return;
        }

        var cancellation = new CancellationTokenSource();
        namingCancellation = cancellation;
        var round = Task.Run(() => current.Name(cancellation.Token), cancellation.Token);
        naming = round;
        _ = FinishNamingAsync(round, current, cancellation);
    }

    private async Task FinishNamingAsync(Task<int> round, LiveSymbolNamer current, CancellationTokenSource cancellation)
    {
        int? count = null;
        try
        {
            count = await round;
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            // A stopped or failed round names nothing; the next read asks again.
        }
        finally
        {
            if (naming == round)
            {
                naming = null;
                namingCancellation = null;
            }

            cancellation.Dispose();
        }

        if (count is not { } named || namer != current || !live || symbolizing)
        {
            return;
        }

        if (named > 0)
        {
            // The read that shows the names starts the next round, for the addresses still asked for.
            namesRead = true;
            stale = true;
            _ = RefreshAsync();
        }
        else if (current.Requested)
        {
            // A read asked for more while the round ran.
            StartNaming();
        }
    }

    // Stops naming the recording's functions and forgets the names found.
    private void StopNamer()
    {
        namingCancellation?.Cancel();
        namingCancellation = null;
        naming = null;
        namesRead = false;
        var old = namer;
        namer = null;
        old?.Dispose();
    }

    // Lists the functions that match the search, in the chosen order, keeping the selected one selected.
    private void ShowFunctions()
    {
        searchTimer.Stop();
        IEnumerable<CpuFunctionRow> items = rows;
        var search = SearchBox.Text.Trim();
        if (search.Length > 0)
        {
            items = items.Where(row => row.Function.Contains(search, StringComparison.OrdinalIgnoreCase));
        }

        if (SortOrder.SelectedIndex == 1)
        {
            items = items.OrderByDescending(row => row.Item.Total).ThenByDescending(row => row.Item.Self)
                .ThenBy(row => row.Function, StringComparer.Ordinal);
        }

        var list = items.ToList();
        var index = selectedFrame is null ? -1 : list.FindIndex(row => row.Function == selectedFrame);
        populating = true;
        try
        {
            FunctionList.ItemsSource = list;
            FunctionList.SelectedIndex = index;
        }
        finally
        {
            populating = false;
        }

        if (index >= 0)
        {
            FunctionList.ScrollIntoView(list[index]);
        }
    }

    // Shows the call tree of all the samples, or the callers or callees of the selected function, opening its heaviest
    // path: the children with at least half of the samples.
    private async void ShowTree()
    {
        var version = ++treeVersion;
        CallTree.RootNodes.Clear();
        var mode = TreeMode.SelectedIndex;
        var selected = selectedFrame is null ? null : functions.FirstOrDefault(item => item.Frame == selectedFrame);
        var prompt = stacks is { Samples: > 0 } && mode > 0 && selected is null;
        TreePrompt.Visibility = prompt ? Visibility.Visible : Visibility.Collapsed;
        if (prompt || stacks is not { Samples: > 0 } value)
        {
            return;
        }

        try
        {
            var (root, heavy) = await Task.Run(() =>
            {
                var top = mode switch
                {
                    1 => value.Callers(selected!.FrameIndex),
                    2 => value.Callees(selected!.FrameIndex),
                    _ => value.TopDown(),
                };
                var chain = new List<CpuCallTreeNode>();
                var node = top;
                while (chain.Count < MaxHeavyDepth && node.Children.Count > 0 && node.Children[0].Samples * 2 >= top.Samples)
                {
                    node = node.Children[0];
                    chain.Add(node);
                }

                // The last node's children are listed too, so it opens without waiting.
                _ = node.Children;
                return (top, chain);
            });
            if (version != treeVersion)
            {
                return;
            }

            var current = CreateNode(value, root, root.FrameIndex < 0 ? Text("CpuViewAllSamples") : null);
            CallTree.RootNodes.Add(current);
            Populate(value, current);
            current.IsExpanded = true;
            foreach (var step in heavy)
            {
                if (current.Children.FirstOrDefault(child => ReferenceEquals((child.Content as CpuTreeLabel)?.Node, step)) is not { } next)
                {
                    break;
                }

                Populate(value, next);
                next.IsExpanded = true;
                current = next;
            }
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            ShowError(error);
        }
    }

    private TreeViewNode CreateNode(CpuStacks value, CpuCallTreeNode node, string? function = null)
    {
        var name = function ?? node.Frame;
        var total = Percent(value, node.Samples);
        var label = new CpuTreeLabel(node, name, total, node.Self is { } self ? Percent(value, self) : string.Empty,
            Milliseconds(value, node.Samples), node.Named || node.FrameIndex < 0 ? 1 : 0.6,
            Format("CpuViewNodeToolTipFormat", name, node.Samples, total));
        return new TreeViewNode { Content = label, HasUnrealizedChildren = node.HasChildren };
    }

    // Adds a node's children, which are computed by then, summing those past the limit into one row.
    private void Populate(CpuStacks value, TreeViewNode node)
    {
        if (!node.HasUnrealizedChildren || node.Content is not CpuTreeLabel { Node: { } item })
        {
            return;
        }

        var children = item.Children;
        var count = Math.Min(children.Count, MaxChildren);
        for (var i = 0; i < count; i++)
        {
            node.Children.Add(CreateNode(value, children[i]));
        }

        if (children.Count > count)
        {
            var rest = children.Skip(count).Sum(child => child.Samples);
            var name = Format("CpuViewMoreFormat", children.Count - count);
            var total = Percent(value, rest);
            node.Children.Add(new TreeViewNode
            {
                Content = new CpuTreeLabel(null, name, total, string.Empty, Milliseconds(value, rest), 1,
                    Format("CpuViewNodeToolTipFormat", name, rest, total)),
            });
        }

        node.HasUnrealizedChildren = false;
    }

    // Computes an opened node's children on a worker thread, since a big tree's take a while.
    private async void CallTree_Expanding(TreeView sender, TreeViewExpandingEventArgs args)
    {
        var node = args.Node;
        if (!node.HasUnrealizedChildren || node.Content is not CpuTreeLabel { Node: { } item } || stacks is not { } value)
        {
            return;
        }

        var version = treeVersion;
        try
        {
            await Task.Run(() => item.Children);
            if (version == treeVersion)
            {
                Populate(value, node);
            }
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            ShowError(error);
        }
    }

    private void FunctionList_SelectionChanged(object sender, SelectionChangedEventArgs args)
    {
        if (populating || FunctionList.SelectedItem is not CpuFunctionRow row || row.Function == selectedFrame)
        {
            return;
        }

        selectedFrame = row.Function;
        if (TreeMode.SelectedIndex == 0)
        {
            // Selecting a function asks where it's called from, so the tree turns to its callers.
            TreeMode.SelectedIndex = 1;
        }
        else
        {
            ShowTree();
        }
    }

    private void ThreadFilter_SelectionChanged(object sender, SelectionChangedEventArgs args)
    {
        if (populating || ThreadFilter.SelectedIndex < 0 || threadIds[ThreadFilter.SelectedIndex] == threadId)
        {
            return;
        }

        threadId = threadIds[ThreadFilter.SelectedIndex];
        stale = true;
        _ = RefreshAsync();
    }

    private void SearchBox_TextChanged(object sender, TextChangedEventArgs args)
    {
        searchTimer.Stop();
        searchTimer.Start();
    }

    private void Refresh_Click(object sender, RoutedEventArgs args)
    {
        stale = true;
        _ = RefreshAsync();
    }

    private async void Open_Click(object sender, RoutedEventArgs args)
    {
        try
        {
            var picker = new FileOpenPicker(Owner);
            picker.FileTypeFilter.Add(".sqlite");
            if (await picker.PickSingleFileAsync() is not { } file)
            {
                return;
            }

            source?.Open(file.Path);
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            ShowError(error);
        }
    }

    private void ShowError(Exception error)
    {
        ReadError.Message = error.Message;
        ReadError.IsOpen = true;
    }
}
